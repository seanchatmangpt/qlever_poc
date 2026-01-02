// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadReplayEngine implementation for deterministic replay.
// Part of EPIC 4 Subsystem 2: Workload Replay Subsystem.

#include "engine/readPlane/WorkloadReplayEngine.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/Result.h"
#include "global/Epoch.h"
#include "util/Log.h"

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace readPlane {

// =============================================================================
// WorkloadReplayEngine::Impl - Private implementation
// =============================================================================

struct WorkloadReplayEngine::Impl {
  // Source manifest (not owned)
  const WorkloadManifest* manifest = nullptr;

  // Configuration
  ReplayConfiguration config;

  // Callbacks
  ReplayCallbacks callbacks;

  // Execution context (not owned)
  QueryExecutionContext* qec = nullptr;
  Server* server = nullptr;
  const Index* index = nullptr;
  QueryResultCache* cache = nullptr;

  // Replay epoch
  EpochKey replay_epoch;

  // Progress tracking (thread-safe)
  ad_utility::Synchronized<ReplayProgress> progress;

  // Running state
  std::atomic<bool> is_running{false};
  std::atomic<bool> abort_requested{false};

  // Run ID generator
  static std::atomic<uint64_t> run_id_counter;
};

std::atomic<uint64_t> WorkloadReplayEngine::Impl::run_id_counter{0};

// =============================================================================
// WorkloadReplayEngine constructor/destructor
// =============================================================================

WorkloadReplayEngine::WorkloadReplayEngine(const WorkloadManifest* manifest)
    : impl_(std::make_unique<Impl>()) {
  impl_->manifest = manifest;

  // Set default hostname
#ifdef __linux__
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    impl_->config.hostname = hostname;
  } else {
    impl_->config.hostname = "unknown";
  }
#else
  impl_->config.hostname = "unknown";
#endif
}

WorkloadReplayEngine::WorkloadReplayEngine(const WorkloadManifest* manifest,
                                           ReplayConfiguration config)
    : impl_(std::make_unique<Impl>()) {
  impl_->manifest = manifest;
  impl_->config = std::move(config);
}

WorkloadReplayEngine::~WorkloadReplayEngine() = default;

WorkloadReplayEngine::WorkloadReplayEngine(WorkloadReplayEngine&&) noexcept =
    default;
WorkloadReplayEngine& WorkloadReplayEngine::operator=(
    WorkloadReplayEngine&&) noexcept = default;

// =============================================================================
// Configuration methods
// =============================================================================

void WorkloadReplayEngine::setConfiguration(ReplayConfiguration config) {
  impl_->config = std::move(config);
}

const ReplayConfiguration& WorkloadReplayEngine::getConfiguration() const {
  return impl_->config;
}

void WorkloadReplayEngine::setCallbacks(ReplayCallbacks callbacks) {
  impl_->callbacks = std::move(callbacks);
}

// =============================================================================
// Context setup methods
// =============================================================================

void WorkloadReplayEngine::setQueryExecutionContext(
    QueryExecutionContext* qec) {
  impl_->qec = qec;
}

void WorkloadReplayEngine::setServer(Server* server) { impl_->server = server; }

void WorkloadReplayEngine::setIndex(const Index* index) {
  impl_->index = index;
}

void WorkloadReplayEngine::setCache(QueryResultCache* cache) {
  impl_->cache = cache;
}

// =============================================================================
// Progress and status methods
// =============================================================================

ReplayProgress WorkloadReplayEngine::getProgress() const {
  return impl_->progress.withReadLock(
      [](const ReplayProgress& p) { return p; });
}

bool WorkloadReplayEngine::isRunning() const {
  return impl_->is_running.load();
}

void WorkloadReplayEngine::requestAbort() {
  impl_->abort_requested.store(true);
}

// =============================================================================
// Epoch management methods
// =============================================================================

void WorkloadReplayEngine::setReplayEpoch(const EpochKey& epoch) {
  impl_->replay_epoch = epoch;
}

const EpochKey& WorkloadReplayEngine::getReplayEpoch() const {
  return impl_->replay_epoch;
}

bool WorkloadReplayEngine::isCrossEpoch() const {
  if (!impl_->manifest) return false;
  return impl_->replay_epoch.epoch_id != impl_->manifest->getEpochId();
}

// =============================================================================
// Main replay methods
// =============================================================================

ReplayRun WorkloadReplayEngine::replayAll() {
  // Check preconditions
  if (!impl_->manifest) {
    throw std::runtime_error("No manifest set for replay");
  }

  if (!impl_->manifest->isValid()) {
    throw std::runtime_error(
        "INVARIANT VIOLATION: Manifest is invalid or contains inconsistent "
        "epochs");
  }

  // Mark as running
  impl_->is_running.store(true);
  impl_->abort_requested.store(false);

  // Initialize replay run
  ReplayRun run = initializeReplayRun();

  // Get snapshot of records
  auto recordsSnapshot = impl_->manifest->getRecordsSnapshot();

  // Initialize progress
  impl_->progress.withWriteLock([&](ReplayProgress& p) {
    p.total_records = recordsSnapshot->size();
    p.completed_records = 0;
    p.successful_records = 0;
    p.divergent_records = 0;
    p.is_aborted = false;
    p.aborted_at_sequence_id = 0;
  });

  // Sort records by sequence_id for deterministic ordering
  std::vector<const WorkloadRecord*> sorted_records;
  sorted_records.reserve(recordsSnapshot->size());
  for (const auto& record : *recordsSnapshot) {
    sorted_records.push_back(&record);
  }
  std::sort(sorted_records.begin(), sorted_records.end(),
            [](const WorkloadRecord* a, const WorkloadRecord* b) {
              return a->sequence_id < b->sequence_id;
            });

  // Apply max_records limit if configured
  if (impl_->config.max_records > 0 &&
      sorted_records.size() > impl_->config.max_records) {
    sorted_records.resize(impl_->config.max_records);
  }

  // Process records in sequence_id order (single-threaded, deterministic)
  for (const auto* record : sorted_records) {
    // Check for abort request
    if (impl_->abort_requested.load()) {
      run.overall_status = ReplayStatus::ABORT;
      break;
    }

    // Replay single record
    ReplayResult result = replaySingle(*record);
    result.replay_run_id = run.run_id;

    // Update progress
    updateProgress(result);

    // Invoke callback if set
    if (impl_->callbacks.onRecordCompleted) {
      impl_->callbacks.onRecordCompleted(result);
    }

    // Progress callback
    if (impl_->callbacks.onProgressUpdate) {
      impl_->callbacks.onProgressUpdate(getProgress());
    }

    // Add result to run
    run.results.push_back(std::move(result));

    // Check for divergence - fail-closed behavior
    if (run.results.back().execution_status == ReplayStatus::DIVERGENCE) {
      if (impl_->config.abort_on_divergence) {
        run.overall_status = ReplayStatus::DIVERGENCE;
        run.aborted_at_sequence_id = record->sequence_id;

        // Invoke divergence callback
        if (impl_->callbacks.onDivergenceDetected &&
            run.results.back().divergence_artifact.has_value()) {
          impl_->callbacks.onDivergenceDetected(
              *run.results.back().divergence_artifact);
        }

        // Mark remaining records as aborted
        impl_->progress.withWriteLock([&](ReplayProgress& p) {
          p.is_aborted = true;
          p.aborted_at_sequence_id = record->sequence_id;
        });

        break;  // ABORT - do NOT continue with remaining records
      }
    }
  }

  // Finalize run
  run.end_timestamp_ns = getCurrentTimestampNs();

  // Compute final statistics
  for (const auto& result : run.results) {
    switch (result.execution_status) {
      case ReplayStatus::SUCCESS:
        run.stats.successful_replays++;
        break;
      case ReplayStatus::DIVERGENCE:
        run.stats.divergent_replays++;
        break;
      case ReplayStatus::ABORT:
        run.stats.aborted_replays++;
        break;
      case ReplayStatus::ERROR:
        run.stats.error_replays++;
        break;
    }
    if (result.is_cross_epoch_reproducible) {
      run.stats.cross_epoch_reproducible++;
    }
    run.stats.total_execution_time_ns += result.execution_duration_ns;
  }
  run.stats.total_records = run.results.size();

  // If no divergence and not aborted, mark as success
  if (run.overall_status != ReplayStatus::DIVERGENCE &&
      run.overall_status != ReplayStatus::ABORT) {
    run.overall_status = ReplayStatus::SUCCESS;
  }

  // Mark as not running
  impl_->is_running.store(false);

  // Invoke completion callback
  if (impl_->callbacks.onReplayCompleted) {
    impl_->callbacks.onReplayCompleted(run);
  }

  return run;
}

ReplayResult WorkloadReplayEngine::replaySingle(const WorkloadRecord& record) {
  ReplayResult result;
  result.workload_record_id = record.sequence_id;
  result.replayed_on_epoch = impl_->replay_epoch;
  result.replayed_on_hostname = impl_->config.hostname;
  result.expected_digest = record.fingerprint_sha256;

  // Collect trace events if configured
  std::vector<ExecutionTraceEvent> trace;
  if (impl_->config.collect_trace_events) {
    trace.push_back(ExecutionTraceEvent::now(TraceEventType::QUERY_START,
                                             "Starting query replay"));
  }

  auto start_time = std::chrono::steady_clock::now();

  try {
    // Track resources during execution
    ResourceMetrics resources;

    // Execute query and compute digest
    ExecutionDigest actual_digest =
        executeQueryAndComputeDigest(record, resources, trace);

    result.execution_digest = actual_digest;

    // Compare digests
    DigestComparisonResult comparison =
        compareDigests(record.fingerprint_sha256, actual_digest,
                       record.epoch_key.epoch_id, impl_->replay_epoch.epoch_id);

    if (comparison.isSuccess()) {
      result.execution_status = ReplayStatus::SUCCESS;
      result.digest_matches = true;

      if (comparison.outcome ==
          DigestComparisonOutcome::CROSS_EPOCH_REPRODUCIBLE) {
        result.is_cross_epoch_reproducible = true;
        AD_LOG_INFO << "CROSS_EPOCH_REPRODUCIBLE: Record " << record.sequence_id
                    << " digest matches across epochs" << std::endl;
      }
    } else {
      // DIVERGENCE - fail-closed
      result.execution_status = ReplayStatus::DIVERGENCE;
      result.digest_matches = false;

      // Build divergence artifact
      result.divergence_artifact = buildDivergenceArtifact(
          record, record.fingerprint_sha256, actual_digest, trace);

      AD_LOG_ERROR << "DIVERGENCE DETECTED: Record " << record.sequence_id
                   << " - " << comparison.divergence_details << std::endl;
    }

  } catch (const std::exception& e) {
    result.execution_status = ReplayStatus::ERROR;
    result.digest_matches = false;

    if (impl_->config.collect_trace_events) {
      trace.push_back(ExecutionTraceEvent::now(
          TraceEventType::QUERY_COMPLETE, std::string("Error: ") + e.what()));
    }

    AD_LOG_ERROR << "Error replaying record " << record.sequence_id << ": "
                 << e.what() << std::endl;
  }

  auto end_time = std::chrono::steady_clock::now();
  result.execution_duration_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                           start_time)
          .count();

  if (impl_->config.collect_trace_events) {
    trace.push_back(
        ExecutionTraceEvent::now(TraceEventType::QUERY_COMPLETE, "Completed"));
    result.trace_events = std::move(trace);
  }

  return result;
}

// =============================================================================
// Private helper methods
// =============================================================================

ReplayRun WorkloadReplayEngine::initializeReplayRun() {
  ReplayRun run;
  run.run_id = generateRunId();
  run.format_version = "v1.0";
  run.source_manifest_id = impl_->manifest->getId();
  run.source_manifest_digest = impl_->manifest->getDigest();
  run.replay_epoch = impl_->replay_epoch;
  run.replay_hostname = impl_->config.hostname;
  run.start_timestamp_ns = getCurrentTimestampNs();
  run.overall_status = ReplayStatus::SUCCESS;
  return run;
}

ExecutionDigest WorkloadReplayEngine::executeQueryAndComputeDigest(
    const WorkloadRecord& record, ResourceMetrics& resources,
    std::vector<ExecutionTraceEvent>& trace) {
  // Reconstruct query from record
  std::string query_text = reconstructQuery(record);

  if (impl_->config.collect_trace_events) {
    trace.push_back(
        ExecutionTraceEvent::now(TraceEventType::CACHE_LOOKUP, "Cache lookup"));
  }

  // Parse and plan query
  auto qet = parseAndPlanQuery(query_text, record);
  if (!qet.has_value()) {
    throw std::runtime_error("Failed to parse/plan query");
  }

  if (impl_->config.collect_trace_events) {
    trace.push_back(ExecutionTraceEvent::now(TraceEventType::PLAN_COMPILE,
                                             "Plan compiled"));
  }

  // Track resource usage before execution
  resources = trackResourceUsage();

  if (impl_->config.collect_trace_events) {
    trace.push_back(ExecutionTraceEvent::now(TraceEventType::EXECUTION_START,
                                             "Execution started"));
  }

  // Execute query
  auto result = executeQuery(*qet);
  if (!result.has_value()) {
    throw std::runtime_error("Failed to execute query");
  }

  if (impl_->config.collect_trace_events) {
    trace.push_back(ExecutionTraceEvent::now(TraceEventType::EXECUTION_END,
                                             "Execution completed"));
  }

  // Track resource usage after execution
  ResourceMetrics post_resources = trackResourceUsage();

  // Update resources with post-execution values
  resources.result_bytes_written = post_resources.result_bytes_written;

  if (impl_->config.collect_trace_events) {
    trace.push_back(ExecutionTraceEvent::now(TraceEventType::RESULT_SERIALIZE,
                                             "Result serialized"));
  }

  // Build plan info for digest computation
  // EPIC 10.1: Extract operator topology ONLY (no cost/size estimates)
  // Uses PlanInfo::extractTopology() for full tree traversal with:
  // - Operator sequence (depth-first)
  // - Variable bindings (sorted, deterministic)
  // - Join keys, scan patterns, grouping/order/limit
  // Excludes: cost estimates, cardinality, timing (optimization metadata)
  PlanInfo plan_info = PlanInfo::extractTopology(*(*qet));

  // Build result metadata
  ResultMetadata result_meta;
  if ((*result)->isFullyMaterialized()) {
    const auto& table = (*result)->idTable();
    result_meta.row_count = table.size();
    result_meta.column_count = table.numColumns();
    for (size_t i = 0; i < table.numColumns(); ++i) {
      result_meta.column_types.push_back("Id");
    }
  }

  // Compute execution digest using the existing ExecutionDigest::compute
  return ExecutionDigest::compute(record.query_fp, plan_info, resources,
                                  result_meta, result_meta.row_count);
}

DigestComparisonResult WorkloadReplayEngine::compareDigests(
    const std::string& expected_digest, const ExecutionDigest& actual_digest,
    ad_utility::EpochId capture_epoch, ad_utility::EpochId replay_epoch) {
  // Create expected digest from string
  ExecutionDigest expected;
  expected.digest_hash = expected_digest;

  return DigestComparisonResult::compare(expected, actual_digest, capture_epoch,
                                         replay_epoch);
}

DivergenceArtifact WorkloadReplayEngine::buildDivergenceArtifact(
    const WorkloadRecord& record, const std::string& expected_digest,
    const ExecutionDigest& actual_digest,
    const std::vector<ExecutionTraceEvent>& trace) {
  DivergenceArtifact artifact;
  artifact.query_fingerprint_sha256 = record.fingerprint_sha256;
  artifact.workload_record_sequence_id = record.sequence_id;
  artifact.expected_digest = expected_digest;
  artifact.actual_digest = actual_digest.digest_hash;
  artifact.actual_plan_hash = actual_digest.plan_hash;
  artifact.actual_result_shape_hash = actual_digest.result_shape_hash;
  artifact.actual_resource_signature = actual_digest.resource_signature;
  artifact.trace_events = trace;
  artifact.detection_timestamp_ns = getCurrentTimestampNs();
  return artifact;
}

std::string WorkloadReplayEngine::reconstructQuery(
    const WorkloadRecord& record) {
  // Use the stored query fingerprint to reconstruct query
  // In practice, the actual query text should be stored in replay_params
  auto it = record.replay_params.find("query_text");
  if (it != record.replay_params.end()) {
    return it->second;
  }

  // Fallback: use normalized text hash (would need lookup)
  throw std::runtime_error("Query text not available in record replay_params");
}

std::optional<std::shared_ptr<QueryExecutionTree>>
WorkloadReplayEngine::parseAndPlanQuery(const std::string& query_text,
                                        const WorkloadRecord& record) {
  // This requires integration with the Server/QueryPlanner
  // The actual implementation would:
  // 1. Parse the SPARQL query using the SPARQL parser
  // 2. Plan the query using QueryPlanner
  // 3. Return the QueryExecutionTree
  //
  // For now, we indicate that server integration is needed
  if (!impl_->server || !impl_->qec) {
    throw std::runtime_error(
        "Query parsing requires Server and QueryExecutionContext - "
        "use setServer() and setQueryExecutionContext()");
  }

  // Placeholder - actual implementation would use Server::planQuery
  throw std::runtime_error(
      "Query planning not yet implemented - requires full Server integration");
}

std::optional<std::shared_ptr<const Result>> WorkloadReplayEngine::executeQuery(
    const std::shared_ptr<QueryExecutionTree>& qet) {
  // Execute the query through the QueryExecutionTree
  try {
    // Mark as root for proper execution
    auto mutableQet = std::const_pointer_cast<QueryExecutionTree>(qet);
    mutableQet->isRoot() = true;

    // Get result (fully materialized for digest computation)
    auto result = qet->getResult(false /* not lazy */);
    return result;
  } catch (const std::exception& e) {
    AD_LOG_ERROR << "Query execution failed: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::string WorkloadReplayEngine::computePlanHash(
    const QueryExecutionTree& qet) {
  // Get the cache key from the query execution tree
  // This is deterministic and doesn't include pointer values
  std::ostringstream oss;
  oss << "cache_key=" << qet.getCacheKey() << ";";

  if (qet.getRootOperation()) {
    oss << "descriptor=" << qet.getRootOperation()->getDescriptor() << ";";
    oss << "result_width=" << qet.getResultWidth() << ";";

    // Include sort order
    const auto& sorted_on = qet.resultSortedOn();
    oss << "sorted_on=[";
    for (size_t i = 0; i < sorted_on.size(); ++i) {
      if (i > 0) oss << ",";
      oss << sorted_on[i];
    }
    oss << "];";
  }

  // Would use SHA256 here in production
  return oss.str();
}

ResourceMetrics WorkloadReplayEngine::trackResourceUsage() {
  ResourceMetrics metrics;

#ifdef __linux__
  // Get memory usage via rusage
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    metrics.memory_pages_accessed = usage.ru_minflt + usage.ru_majflt;
  }
#endif

  // Cache statistics would come from ReadCacheManager
  // For now, we use placeholder values
  // Actual implementation would observe cache stats without modifying caches

  return metrics;
}

void WorkloadReplayEngine::updateProgress(const ReplayResult& result) {
  impl_->progress.withWriteLock([&](ReplayProgress& p) {
    p.completed_records++;

    switch (result.execution_status) {
      case ReplayStatus::SUCCESS:
        p.successful_records++;
        break;
      case ReplayStatus::DIVERGENCE:
        p.divergent_records++;
        break;
      default:
        break;
    }
  });
}

uint64_t WorkloadReplayEngine::generateRunId() {
  // Generate a unique run ID using atomic counter + timestamp
  uint64_t counter = Impl::run_id_counter.fetch_add(1);
  uint64_t timestamp = getCurrentTimestampNs();
  return (timestamp << 20) | (counter & 0xFFFFF);
}

uint64_t WorkloadReplayEngine::getCurrentTimestampNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// =============================================================================
// Factory functions
// =============================================================================

std::unique_ptr<WorkloadReplayEngine> createReplayEngineFromFile(
    const std::string& manifest_path) {
  // Read manifest file
  std::ifstream file(manifest_path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open manifest file: " + manifest_path);
  }

  nlohmann::json json;
  file >> json;

  return createReplayEngineFromJson(json);
}

std::unique_ptr<WorkloadReplayEngine> createReplayEngineFromJson(
    const nlohmann::json& manifest_json) {
  // Create a new WorkloadManifest from the JSON
  // Note: This creates a manifest that must be kept alive while the engine
  // is in use. In practice, the manifest should be managed by the caller.

  auto epochId = manifest_json.value("epoch_id", uint64_t{0});
  auto epochManifestSha256 =
      manifest_json.value("epoch_manifest_sha256", std::string{});
  auto hostname = manifest_json.value("captured_on_hostname", std::string{});

  // Create manifest - note: this is a simplified version
  // Full implementation would parse all records
  auto manifest = new WorkloadManifest(epochId, epochManifestSha256, hostname);

  // Create engine with the manifest
  // Note: The manifest is leaked here - in production, use proper lifetime
  // management
  auto engine = std::make_unique<WorkloadReplayEngine>(manifest);
  return engine;
}

}  // namespace readPlane

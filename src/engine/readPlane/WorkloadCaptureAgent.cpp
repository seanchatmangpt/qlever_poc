// Copyright 2025, University of Freiburg,
//                 Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadCaptureAgent implementation (EPIC 4 - Subsystem 1)

#include "engine/readPlane/WorkloadCaptureAgent.h"

#include <stdexcept>

#include "util/Log.h"

namespace readPlane {

// ============================================================================
// WorkloadCaptureAgent - Singleton Implementation
// ============================================================================

WorkloadCaptureAgent::WorkloadCaptureAgent() : determinism_classifier_() {
  // Initialize with empty manifest
  auto initialManifest = std::make_shared<WorkloadManifest>();
  current_manifest_.store(initialManifest, std::memory_order_release);
}

WorkloadCaptureAgent& WorkloadCaptureAgent::getInstance() {
  static WorkloadCaptureAgent instance;
  return instance;
}

// ============================================================================
// Hook Interface Implementation
// ============================================================================

void WorkloadCaptureAgent::onQueryExecuted(const QueryContext& context,
                                           const ExecutionResult& result) {
  // Check if capture is enabled
  if (!config_.enabled) {
    return;
  }

  // Validate context
  if (!context.parsed_query) {
    AD_LOG_WARN << "WorkloadCaptureAgent: null parsed_query in context"
                << std::endl;
    return;
  }

  // Step 1: Check determinism via DeterminismClassifier
  auto determinismFeatures =
      determinism_classifier_.analyze(*context.parsed_query);

  if (!determinismFeatures.isDeterministic()) {
    // Non-deterministic query - log to excluded section (NOT fail-closed)
    if (config_.log_excluded_queries) {
      WorkloadExcludedRecord excluded;
      excluded.sequence_id = globalWorkloadSequenceCounter.next();
      excluded.epoch_key = context.epoch_key;
      excluded.shape_sha256 = context.fingerprint.shape_sha256;

      // Determine specific reason
      if (determinismFeatures.hasNow) {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::NON_DETERMINISTIC_FUNCTION;
        excluded.reason_details = "NOW()";
      } else if (determinismFeatures.hasRand) {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::NON_DETERMINISTIC_FUNCTION;
        excluded.reason_details = "RAND()";
      } else if (determinismFeatures.hasUuid) {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::NON_DETERMINISTIC_FUNCTION;
        excluded.reason_details = "UUID()";
      } else if (determinismFeatures.hasBnode) {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::NON_DETERMINISTIC_FUNCTION;
        excluded.reason_details = "BNODE()";
      } else if (determinismFeatures.hasService) {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::SERVICE_CLAUSE;
        excluded.reason_details = "SERVICE clause";
      } else {
        excluded.reason =
            WorkloadExcludedRecord::ExclusionReason::NON_DETERMINISTIC_FUNCTION;
        excluded.reason_details = "Other non-deterministic function";
      }

      // Append to manifest's excluded section
      auto manifest = current_manifest_.load(std::memory_order_acquire);
      if (manifest) {
        manifest->appendExcluded(std::move(excluded));
      }

      // Update metrics
      metrics_.withWriteLock(
          [](Metrics& m) { m.queries_excluded++; });
    }

    // Query continues to execute - NOT fail-closed for non-determinism
    return;
  }

  // Step 2: Create WorkloadRecord for deterministic query
  WorkloadRecord record;
  record.sequence_id = globalWorkloadSequenceCounter.next();
  record.capture_timestamp_epoch = record.sequence_id;  // Epoch-local counter
  record.epoch_key = context.epoch_key;
  record.query_fp = context.fingerprint;
  record.execution_class = result.execution_class;
  record.observed_latency_ns = result.latency.count();

  // Extract replay-relevant parameters (subset of QueryFingerprint)
  // Only include parameters that affect execution path
  record.replay_params["shape_sha256"] = context.fingerprint.shape_sha256;
  record.replay_params["params_sha256"] = context.fingerprint.params_sha256;

  // Add request parameters that affect execution
  for (const auto& [key, value] : context.request_params) {
    record.replay_params[key] = value;
  }

  // Step 3: Compute fingerprint (deterministic)
  try {
    record.fingerprint_sha256 = record.computeFingerprint();
  } catch (const std::exception& e) {
    // Serialization error - fail-closed
    if (config_.fail_on_serialization_error) {
      AD_LOG_ERROR << "WorkloadCaptureAgent: fingerprint computation failed: "
                   << e.what() << std::endl;
      metrics_.withWriteLock(
          [](Metrics& m) { m.serialization_errors++; });
      throw std::runtime_error(
          "ABORT: Workload capture serialization error: " +
          std::string(e.what()));
    }
    return;
  }

  // Step 4: Validate JSON-LD serialization (fail-closed on error)
  if (config_.fail_on_serialization_error) {
    try {
      std::string jsonld = record.toJsonLD();
      if (jsonld.empty()) {
        throw std::runtime_error("Empty JSON-LD serialization");
      }
    } catch (const std::exception& e) {
      AD_LOG_ERROR << "WorkloadCaptureAgent: JSON-LD serialization failed: "
                   << e.what() << std::endl;
      metrics_.withWriteLock(
          [](Metrics& m) { m.serialization_errors++; });
      throw std::runtime_error(
          "ABORT: Workload capture JSON-LD serialization error: " +
          std::string(e.what()));
    }
  }

  // Step 5: Append to manifest (atomic)
  auto manifest = current_manifest_.load(std::memory_order_acquire);
  if (manifest) {
    bool appended = manifest->append(std::move(record));
    if (!appended) {
      AD_LOG_WARN << "WorkloadCaptureAgent: epoch mismatch, record not appended"
                  << std::endl;
      return;
    }

    // Update metrics
    metrics_.withWriteLock(
        [](Metrics& m) { m.queries_captured++; });
  }
}

// ============================================================================
// Manifest Access
// ============================================================================

std::shared_ptr<WorkloadManifest> WorkloadCaptureAgent::getCurrentManifest()
    const {
  return current_manifest_.load(std::memory_order_acquire);
}

std::shared_ptr<std::vector<WorkloadRecord>>
WorkloadCaptureAgent::getRecordsSnapshot() const {
  auto manifest = current_manifest_.load(std::memory_order_acquire);
  if (manifest) {
    return manifest->getRecordsSnapshot();
  }
  return std::make_shared<std::vector<WorkloadRecord>>();
}

// ============================================================================
// Configuration
// ============================================================================

void WorkloadCaptureAgent::setConfig(const WorkloadCaptureConfig& config) {
  config_ = config;
}

WorkloadCaptureConfig WorkloadCaptureAgent::getConfig() const {
  return config_;
}

void WorkloadCaptureAgent::setEnabled(bool enabled) {
  config_.enabled = enabled;
}

bool WorkloadCaptureAgent::isEnabled() const {
  return config_.enabled;
}

// ============================================================================
// Epoch Integration
// ============================================================================

void WorkloadCaptureAgent::onEpochPromoted(
    ad_utility::EpochId oldEpochId,
    ad_utility::EpochId newEpochId,
    const std::string& newEpochManifestSha256) {
  AD_LOG_INFO << "WorkloadCaptureAgent: epoch promoted from " << oldEpochId
              << " to " << newEpochId << std::endl;

  // Create new manifest for new epoch
  auto newManifest = std::make_shared<WorkloadManifest>(
      newEpochId, newEpochManifestSha256, config_.hostname);

  // Atomically swap manifests
  // Old manifest remains valid for readers holding shared_ptr
  current_manifest_.store(newManifest, std::memory_order_release);

  // Update metrics
  metrics_.withWriteLock([](Metrics& m) { m.epoch_transitions++; });
}

void WorkloadCaptureAgent::setupEpochHooks() {
  // Register with global epoch cache invalidation registry
  ad_utility::globalEpochCacheInvalidationRegistry.withWriteLock(
      [this](auto& registry) {
        registry.registerHandler(
            std::make_unique<WorkloadCaptureEpochHandler>(*this));
      });

  AD_LOG_INFO << "WorkloadCaptureAgent: registered with epoch hook registry"
              << std::endl;
}

// ============================================================================
// Metrics
// ============================================================================

WorkloadCaptureAgent::CaptureMetrics WorkloadCaptureAgent::getMetrics() const {
  CaptureMetrics result;
  metrics_.withReadLock([&result](const Metrics& m) {
    result.queries_captured = m.queries_captured;
    result.queries_excluded = m.queries_excluded;
    result.serialization_errors = m.serialization_errors;
    result.epoch_transitions = m.epoch_transitions;
  });

  auto manifest = current_manifest_.load(std::memory_order_acquire);
  result.current_manifest_size = manifest ? manifest->size() : 0;

  return result;
}

void WorkloadCaptureAgent::resetMetrics() {
  metrics_.withWriteLock([](Metrics& m) {
    m.queries_captured = 0;
    m.queries_excluded = 0;
    m.serialization_errors = 0;
    m.epoch_transitions = 0;
  });
}

// ============================================================================
// Testing Support
// ============================================================================

void WorkloadCaptureAgent::resetManifestForTesting(
    ad_utility::EpochId epochId,
    const std::string& epochManifestSha256) {
  auto newManifest = std::make_shared<WorkloadManifest>(
      epochId, epochManifestSha256, config_.hostname);
  current_manifest_.store(newManifest, std::memory_order_release);
  globalWorkloadSequenceCounter.reset();
  resetMetrics();
}

// ============================================================================
// WorkloadCaptureEpochHandler Implementation
// ============================================================================

void WorkloadCaptureEpochHandler::onEpochPromoted(
    const ad_utility::EpochPromotionEvent& event) {
  // Get epoch manifest hash from new epoch
  std::string manifestHash;
  ad_utility::globalEpochManager.withReadLock([&](const auto& mgr) {
    auto opt = mgr.getCurrentEpochManifest();
    if (opt) {
      manifestHash = opt->getManifestHash();
    }
  });

  // Delegate to agent
  agent_.onEpochPromoted(event.oldEpochId_, event.newEpochId_, manifestHash);
}

// ============================================================================
// WorkloadCaptureHook Implementation
// ============================================================================

QueryContext WorkloadCaptureHook::createContext(
    const ParsedQuery& parsedQuery,
    const queryCanonical::QueryFingerprint& fingerprint,
    ad_utility::EpochId epochId,
    const std::string& epochManifestSha256,
    const std::map<std::string, std::string>& params) {
  QueryContext ctx;
  ctx.parsed_query = &parsedQuery;
  ctx.fingerprint = fingerprint;
  ctx.epoch_key = EpochKey{epochId, epochManifestSha256};
  ctx.request_params = params;
  return ctx;
}

ExecutionResult WorkloadCaptureHook::createResult(
    ExecutionClass executionClass,
    std::chrono::nanoseconds latency,
    size_t resultRows,
    size_t resultBytes,
    bool success,
    const std::string& errorMessage) {
  ExecutionResult res;
  res.execution_class = executionClass;
  res.latency = latency;
  res.result_rows = resultRows;
  res.result_bytes = resultBytes;
  res.success = success;
  res.error_message = errorMessage;
  return res;
}

}  // namespace readPlane

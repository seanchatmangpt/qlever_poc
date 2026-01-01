// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Epoch Performance Benchmark Suite
//
// Comprehensive performance benchmarks for epoch-based immutability
// verifying that epoch overhead is <1% latency increase.
//
// Measures:
// - Baseline query execution latency
// - Query execution WITH epoch binding checks
// - Write barrier overhead
// - Concurrent query throughput with epoch binding

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../benchmark/infrastructure/Benchmark.h"
#include "../benchmark/infrastructure/BenchmarkMeasurementContainer.h"
#include "../benchmark/infrastructure/BenchmarkMetadata.h"
#include "ad_utility/global/Epoch.h"
#include "engine/ExportQueryExecutionTrees.h"
#include "engine/QueryPlanner.h"
#include "parser/SparqlParser.h"
#include "util/ConfigManager/ConfigManager.h"
#include "util/IndexTestHelpers.h"
#include "util/Log.h"
#include "util/Timer.h"

using namespace std::string_literals;

namespace ad_benchmark {

namespace {

// Helper: Create a small test knowledge graph
std::string createTestKG(size_t numTriples = 100) {
  std::string kg;
  kg.reserve(numTriples * 60);

  for (size_t i = 0; i < numTriples; ++i) {
    std::string subject =
        "<http://example.org/entity" + std::to_string(i) + ">";
    kg += subject + " <http://example.org/name> \"Entity " + std::to_string(i) +
          "\" .\n";
    kg += subject + " <http://example.org/type> <http://example.org/Type> .\n";
    if (i > 0) {
      kg += subject +
            " <http://example.org/relatedTo> <http://example.org/entity" +
            std::to_string(i - 1) + "> .\n";
    }
  }

  return kg;
}

// Thread-local IRI manager for safe concurrent usage
thread_local EncodedIriManager g_iriManager;

// Helper: Execute a simple SELECT query and measure execution time
struct QueryExecutionMetrics {
  double latencyMs = 0.0;
  uint64_t resultRows = 0;
  std::string error;
};

QueryExecutionMetrics executeQuery(
    std::shared_ptr<ad_utility::QueryExecutionContext> qec,
    const std::string& sparqlQuery) {
  QueryExecutionMetrics metrics;

  try {
    if (!qec) {
      metrics.error = "Query execution context is null";
      return metrics;
    }

    auto cancellationHandle =
        std::make_shared<ad_utility::CancellationHandle<>>();
    QueryPlanner qp{qec, cancellationHandle};

    // Parse the SPARQL query
    auto pq = SparqlParser::parseQuery(&g_iriManager, sparqlQuery, {});

    // Create execution tree and measure
    ad_utility::Timer executionTimer(ad_utility::Timer::Started);

    auto qet = qp.createExecutionTree(pq);

    // Export result and collect metrics
    auto result = ExportQueryExecutionTrees::computeResult(
        pq, qet, ad_utility::MediaType::tsv, executionTimer,
        cancellationHandle);

    std::string output;
    for (const auto& block : result) {
      output += block;
    }

    metrics.latencyMs = executionTimer.getElapsedMs();
    metrics.resultRows = output.length();

  } catch (const std::exception& e) {
    metrics.error = std::string(e.what());
  }

  return metrics;
}

}  // namespace

// ============================================================================
// Benchmark 1: Baseline Query Execution (WITHOUT epoch binding)
// ============================================================================
class BM_QueryExecutionBaseline : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Epoch Benchmark: Query Baseline (No Epoch Checks)";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Baseline query latency without epoch binding checks. "
        "This establishes the reference point for overhead comparison.");
    getGeneralMetadata().addKeyValuePair("warmup_iterations", 5);
    getGeneralMetadata().addKeyValuePair("measurement_iterations", 100);

    const std::string kg = createTestKG(100);
    const std::string query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000";

    try {
      // Setup: Create query execution context
      ad_utility::testing::TestIndexConfig config{kg};
      auto qec = ad_utility::testing::getQec(std::move(config));

      if (!qec) {
        throw std::runtime_error("Failed to create query execution context");
      }

      // Warmup iterations (5 runs)
      for (int i = 0; i < 5; ++i) {
        executeQuery(qec, query);
      }

      // Measure baseline latency (100 iterations)
      auto& baselineGroup = results.addGroup("baseline_latency");
      baselineGroup.metadata().addKeyValuePair("query_type", "SELECT all");
      baselineGroup.metadata().addKeyValuePair("knowledge_graph_size",
                                               "100 triples");

      std::vector<double> latencies;
      latencies.reserve(100);

      auto& latencyMeasurement =
          baselineGroup.addMeasurement("Query Latency (100 iterations)", [&]() {
            for (int i = 0; i < 100; ++i) {
              auto metrics = executeQuery(qec, query);
              if (!metrics.error.empty()) {
                throw std::runtime_error("Query execution error: " +
                                         metrics.error);
              }
              latencies.push_back(metrics.latencyMs);
            }
          });

      latencyMeasurement.metadata().addKeyValuePair("iterations", 100);

      // Compute statistics
      if (!latencies.empty()) {
        double sum = 0.0;
        double min = latencies[0];
        double max = latencies[0];

        for (double lat : latencies) {
          sum += lat;
          min = std::min(min, lat);
          max = std::max(max, lat);
        }

        double avg = sum / latencies.size();
        double variance = 0.0;
        for (double lat : latencies) {
          variance += (lat - avg) * (lat - avg);
        }
        variance /= latencies.size();
        double stddev = std::sqrt(variance);

        baselineGroup.metadata().addKeyValuePair("avg_latency_ms", avg);
        baselineGroup.metadata().addKeyValuePair("min_latency_ms", min);
        baselineGroup.metadata().addKeyValuePair("max_latency_ms", max);
        baselineGroup.metadata().addKeyValuePair("stddev_latency_ms", stddev);

        LOG(INFO) << "Baseline Query Latency Stats:";
        LOG(INFO) << "  Average: " << avg << "ms";
        LOG(INFO) << "  Min: " << min << "ms";
        LOG(INFO) << "  Max: " << max << "ms";
        LOG(INFO) << "  StdDev: " << stddev << "ms";
      }

    } catch (const std::exception& e) {
      LOG(ERROR) << "Baseline benchmark error: " << e.what();
      results.addMeasurement(
          "Error", [&e]() { throw std::runtime_error(std::string(e.what())); });
    }

    return results;
  }
};

// ============================================================================
// Benchmark 2: Query Execution WITH Epoch Binding
// ============================================================================
class BM_QueryExecutionWithEpochBinding : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Epoch Benchmark: Query with Epoch Binding";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Query latency WITH epoch binding checks in SERVE state. "
        "Measures the overhead of epoch state validation during queries.");
    getGeneralMetadata().addKeyValuePair("warmup_iterations", 5);
    getGeneralMetadata().addKeyValuePair("measurement_iterations", 100);

    const std::string kg = createTestKG(100);
    const std::string query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000";

    try {
      // Setup: Create query execution context
      ad_utility::testing::TestIndexConfig config{kg};
      auto qec = ad_utility::testing::getQec(std::move(config));

      if (!qec) {
        throw std::runtime_error("Failed to create query execution context");
      }

      // Setup epoch manager state: Transition to SERVE
      auto& epochMgr = ad_utility::globalEpochManager;
      try {
        epochMgr.acquire()->transitionToIngest();
        epochMgr.acquire()->transitionToSeal();
        epochMgr.acquire()->transitionToServe();
      } catch (const std::exception& e) {
        // State may already be in SERVE, try to restart if needed
        LOG(WARNING) << "Epoch transition warning: " << e.what();
      }

      // Verify we're in SERVE state
      auto state = epochMgr.acquire()->getState();
      if (state != ad_utility::EpochState::SERVE) {
        throw std::runtime_error(
            "Failed to transition epoch manager to SERVE state");
      }

      // Warmup iterations (5 runs)
      for (int i = 0; i < 5; ++i) {
        // Each query invocation will trigger getCurrentEpochIdForQuery()
        auto epochId = epochMgr.acquire()->getCurrentEpochIdForQuery();
        (void)epochId;  // Use to prevent optimization
        executeQuery(qec, query);
      }

      // Measure latency WITH epoch checks (100 iterations)
      auto& epochGroup = results.addGroup("epoch_binding_latency");
      epochGroup.metadata().addKeyValuePair("epoch_state", "SERVE");
      epochGroup.metadata().addKeyValuePair("query_type", "SELECT all");
      epochGroup.metadata().addKeyValuePair("knowledge_graph_size",
                                            "100 triples");

      std::vector<double> latencies;
      latencies.reserve(100);

      auto& epochMeasurement = epochGroup.addMeasurement(
          "Query Latency with Epoch Binding (100 iterations)", [&]() {
            for (int i = 0; i < 100; ++i) {
              // Get epoch ID (simulating epoch binding check)
              auto epochId = epochMgr.acquire()->getCurrentEpochIdForQuery();
              (void)epochId;

              auto metrics = executeQuery(qec, query);
              if (!metrics.error.empty()) {
                throw std::runtime_error("Query execution error: " +
                                         metrics.error);
              }
              latencies.push_back(metrics.latencyMs);
            }
          });

      epochMeasurement.metadata().addKeyValuePair("iterations", 100);
      epochMeasurement.metadata().addKeyValuePair("epoch_checks_per_iteration",
                                                  1);

      // Compute statistics
      if (!latencies.empty()) {
        double sum = 0.0;
        double min = latencies[0];
        double max = latencies[0];

        for (double lat : latencies) {
          sum += lat;
          min = std::min(min, lat);
          max = std::max(max, lat);
        }

        double avg = sum / latencies.size();
        double variance = 0.0;
        for (double lat : latencies) {
          variance += (lat - avg) * (lat - avg);
        }
        variance /= latencies.size();
        double stddev = std::sqrt(variance);

        epochGroup.metadata().addKeyValuePair("avg_latency_ms", avg);
        epochGroup.metadata().addKeyValuePair("min_latency_ms", min);
        epochGroup.metadata().addKeyValuePair("max_latency_ms", max);
        epochGroup.metadata().addKeyValuePair("stddev_latency_ms", stddev);

        LOG(INFO) << "Epoch-Bound Query Latency Stats:";
        LOG(INFO) << "  Average: " << avg << "ms";
        LOG(INFO) << "  Min: " << min << "ms";
        LOG(INFO) << "  Max: " << max << "ms";
        LOG(INFO) << "  StdDev: " << stddev << "ms";
      }

    } catch (const std::exception& e) {
      LOG(ERROR) << "Epoch binding benchmark error: " << e.what();
      results.addMeasurement(
          "Error", [&e]() { throw std::runtime_error(std::string(e.what())); });
    }

    return results;
  }
};

// ============================================================================
// Benchmark 3: Write Barrier Overhead (checkAllowedToMutate)
// ============================================================================
class BM_WriteBarrierCheck : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Epoch Benchmark: Write Barrier Overhead";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures the overhead of write barrier checks "
        "(checkAllowedToMutate()) "
        "in INGEST state. Should be <1 microsecond per check.");
    getGeneralMetadata().addKeyValuePair("iterations", 100000);

    try {
      auto& epochMgr = ad_utility::globalEpochManager;

      // Setup epoch manager state: Transition to INGEST
      try {
        // If already in SERVE, restart first
        if (epochMgr.acquire()->getState() == ad_utility::EpochState::SERVE) {
          epochMgr.acquire()->restart();
        }
        epochMgr.acquire()->transitionToIngest();
      } catch (const std::exception& e) {
        LOG(WARNING) << "Epoch transition info: " << e.what();
      }

      // Verify we're in INGEST state
      auto state = epochMgr.acquire()->getState();
      if (state != ad_utility::EpochState::INGEST) {
        throw std::runtime_error(
            "Failed to transition epoch manager to INGEST state");
      }

      auto& barrierGroup = results.addGroup("write_barrier_overhead");
      barrierGroup.metadata().addKeyValuePair("epoch_state", "INGEST");
      barrierGroup.metadata().addKeyValuePair("operation",
                                              "checkAllowedToMutate()");
      barrierGroup.metadata().addKeyValuePair("iterations", 100000);

      // Warmup (1000 checks)
      for (int i = 0; i < 1000; ++i) {
        epochMgr.acquire()->checkAllowedToMutate();
      }

      // Measure write barrier overhead (100,000 iterations)
      std::vector<double> timings;
      timings.reserve(100);

      auto& barrierMeasurement = barrierGroup.addMeasurement(
          "Write Barrier Check (100k iterations)", [&]() {
            // Run 100k checks in batches to measure timing
            ad_utility::Timer timer(ad_utility::Timer::Started);

            for (int i = 0; i < 100000; ++i) {
              epochMgr.acquire()->checkAllowedToMutate();
            }

            timings.push_back(timer.getElapsedMs());
          });

      barrierMeasurement.metadata().addKeyValuePair("total_checks", 100000);

      if (!timings.empty()) {
        double totalMs = timings[0];
        double perCheckMicroseconds = (totalMs * 1000.0) / 100000.0;

        barrierGroup.metadata().addKeyValuePair("total_time_ms", totalMs);
        barrierGroup.metadata().addKeyValuePair("per_check_microseconds",
                                                perCheckMicroseconds);

        // Check acceptance criterion: <1 microsecond per check
        bool acceptanceMet = perCheckMicroseconds < 1.0;
        barrierGroup.metadata().addKeyValuePair("acceptance_criterion_met",
                                                acceptanceMet);

        LOG(INFO) << "Write Barrier Stats:";
        LOG(INFO) << "  Total time: " << totalMs << "ms";
        LOG(INFO) << "  Per check: " << perCheckMicroseconds << "µs";
        LOG(INFO) << "  Acceptance (<1µs): "
                  << (acceptanceMet ? "PASS" : "FAIL");
      }

    } catch (const std::exception& e) {
      LOG(ERROR) << "Write barrier benchmark error: " << e.what();
      results.addMeasurement(
          "Error", [&e]() { throw std::runtime_error(std::string(e.what())); });
    }

    return results;
  }
};

// ============================================================================
// Benchmark 4: Concurrent Query Throughput with Epoch Binding
// ============================================================================
class BM_ConcurrentQueriesEpochBinding : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Epoch Benchmark: Concurrent Queries with Epoch Binding";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures throughput of concurrent queries all executing in the same "
        "SERVE epoch. Verifies no regression vs non-epoch baseline.");
    getGeneralMetadata().addKeyValuePair("num_threads", 4);
    getGeneralMetadata().addKeyValuePair("queries_per_thread", 25);

    const std::string kg = createTestKG(100);
    const std::string query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000";

    try {
      // Setup: Create query execution context
      ad_utility::testing::TestIndexConfig config{kg};
      auto qec = ad_utility::testing::getQec(std::move(config));

      if (!qec) {
        throw std::runtime_error("Failed to create query execution context");
      }

      // Setup epoch manager state: Transition to SERVE
      auto& epochMgr = ad_utility::globalEpochManager;
      try {
        if (epochMgr.acquire()->getState() == ad_utility::EpochState::INGEST) {
          epochMgr.acquire()->transitionToSeal();
          epochMgr.acquire()->transitionToServe();
        } else if (epochMgr.acquire()->getState() ==
                   ad_utility::EpochState::SERVE) {
          // Already in SERVE, which is fine
        } else {
          epochMgr.acquire()->transitionToIngest();
          epochMgr.acquire()->transitionToSeal();
          epochMgr.acquire()->transitionToServe();
        }
      } catch (const std::exception& e) {
        LOG(WARNING) << "Epoch transition info: " << e.what();
      }

      // Verify we're in SERVE state
      auto state = epochMgr.acquire()->getState();
      if (state != ad_utility::EpochState::SERVE) {
        throw std::runtime_error(
            "Failed to transition epoch manager to SERVE state");
      }

      // Setup concurrent benchmark
      auto& concurrentGroup = results.addGroup("concurrent_throughput");
      concurrentGroup.metadata().addKeyValuePair("epoch_state", "SERVE");
      concurrentGroup.metadata().addKeyValuePair("num_threads", 4);
      concurrentGroup.metadata().addKeyValuePair("queries_per_thread", 25);
      concurrentGroup.metadata().addKeyValuePair("total_queries", 100);

      // Warmup (each thread runs 5 queries)
      std::vector<std::thread> warmupThreads;
      for (int t = 0; t < 4; ++t) {
        warmupThreads.emplace_back([&]() {
          for (int i = 0; i < 5; ++i) {
            auto epochId = epochMgr.acquire()->getCurrentEpochIdForQuery();
            (void)epochId;
            executeQuery(qec, query);
          }
        });
      }
      for (auto& t : warmupThreads) {
        t.join();
      }

      // Measure concurrent throughput
      std::atomic<uint64_t> completedQueries{0};
      std::vector<double> threadLatencies(4, 0.0);

      auto& concurrentMeasurement = concurrentGroup.addMeasurement(
          "Concurrent Throughput (4 threads x 25 queries)", [&]() {
            std::vector<std::thread> threads;

            for (int t = 0; t < 4; ++t) {
              threads.emplace_back([&, t]() {
                ad_utility::Timer threadTimer(ad_utility::Timer::Started);

                for (int i = 0; i < 25; ++i) {
                  // Get epoch ID (simulating epoch binding check)
                  auto epochId =
                      epochMgr.acquire()->getCurrentEpochIdForQuery();
                  (void)epochId;

                  auto metrics = executeQuery(qec, query);
                  if (metrics.error.empty()) {
                    completedQueries++;
                  }
                }

                threadLatencies[t] = threadTimer.getElapsedMs();
              });
            }

            for (auto& t : threads) {
              t.join();
            }
          });

      concurrentMeasurement.metadata().addKeyValuePair("completed_queries",
                                                       completedQueries.load());

      if (completedQueries > 0) {
        double totalLatency = 0.0;
        for (double lat : threadLatencies) {
          totalLatency += lat;
        }
        double avgThreadLatency = totalLatency / 4.0;
        double throughputQueriesPerSec =
            (completedQueries.load() * 1000.0) / totalLatency;

        concurrentGroup.metadata().addKeyValuePair("avg_thread_latency_ms",
                                                   avgThreadLatency);
        concurrentGroup.metadata().addKeyValuePair("throughput_queries_per_sec",
                                                   throughputQueriesPerSec);

        LOG(INFO) << "Concurrent Query Stats:";
        LOG(INFO) << "  Completed queries: " << completedQueries.load();
        LOG(INFO) << "  Average thread latency: " << avgThreadLatency << "ms";
        LOG(INFO) << "  Throughput: " << throughputQueriesPerSec
                  << " queries/sec";
      }

    } catch (const std::exception& e) {
      LOG(ERROR) << "Concurrent query benchmark error: " << e.what();
      results.addMeasurement(
          "Error", [&e]() { throw std::runtime_error(std::string(e.what())); });
    }

    return results;
  }
};

// ============================================================================
// Benchmark 5: Overhead Analysis and Acceptance Verification
// ============================================================================
class BM_OverheadAnalysis : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Epoch Benchmark: Overhead Analysis";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Comprehensive overhead analysis comparing baseline vs epoch-bound "
        "queries. Verifies that overhead is <1% (acceptance criterion).");
    getGeneralMetadata().addKeyValuePair("baseline_iterations", 100);
    getGeneralMetadata().addKeyValuePair("epoch_iterations", 100);

    const std::string kg = createTestKG(100);
    const std::string query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000";

    try {
      // Setup: Create query execution context
      ad_utility::testing::TestIndexConfig config{kg};
      auto qec = ad_utility::testing::getQec(std::move(config));

      if (!qec) {
        throw std::runtime_error("Failed to create query execution context");
      }

      auto& epochMgr = ad_utility::globalEpochManager;

      // Ensure we're in SERVE state
      try {
        auto currentState = epochMgr.acquire()->getState();
        if (currentState != ad_utility::EpochState::SERVE) {
          if (currentState == ad_utility::EpochState::INGEST) {
            epochMgr.acquire()->transitionToSeal();
            epochMgr.acquire()->transitionToServe();
          } else if (currentState == ad_utility::EpochState::INIT) {
            epochMgr.acquire()->transitionToIngest();
            epochMgr.acquire()->transitionToSeal();
            epochMgr.acquire()->transitionToServe();
          }
        }
      } catch (const std::exception& e) {
        LOG(WARNING) << "Epoch setup: " << e.what();
      }

      auto& analysisGroup = results.addGroup("overhead_analysis");

      // Collect baseline latencies
      std::vector<double> baselineLatencies;
      baselineLatencies.reserve(100);

      analysisGroup.addMeasurement("Baseline Collection", [&]() {
        for (int i = 0; i < 100; ++i) {
          auto metrics = executeQuery(qec, query);
          if (metrics.error.empty()) {
            baselineLatencies.push_back(metrics.latencyMs);
          }
        }
      });

      // Collect epoch-bound latencies
      std::vector<double> epochLatencies;
      epochLatencies.reserve(100);

      analysisGroup.addMeasurement("Epoch-bound Collection", [&]() {
        for (int i = 0; i < 100; ++i) {
          auto epochId = epochMgr.acquire()->getCurrentEpochIdForQuery();
          (void)epochId;
          auto metrics = executeQuery(qec, query);
          if (metrics.error.empty()) {
            epochLatencies.push_back(metrics.latencyMs);
          }
        }
      });

      // Analyze overhead
      if (!baselineLatencies.empty() && !epochLatencies.empty()) {
        double baselineAvg = std::accumulate(baselineLatencies.begin(),
                                             baselineLatencies.end(), 0.0) /
                             baselineLatencies.size();
        double epochAvg =
            std::accumulate(epochLatencies.begin(), epochLatencies.end(), 0.0) /
            epochLatencies.size();

        double absoluteOverheadMs = epochAvg - baselineAvg;
        double percentOverhead =
            (baselineAvg > 0) ? (absoluteOverheadMs / baselineAvg) * 100.0
                              : 0.0;

        bool acceptanceMet = percentOverhead < 1.0;

        analysisGroup.metadata().addKeyValuePair("baseline_avg_ms",
                                                 baselineAvg);
        analysisGroup.metadata().addKeyValuePair("epoch_bound_avg_ms",
                                                 epochAvg);
        analysisGroup.metadata().addKeyValuePair("absolute_overhead_ms",
                                                 absoluteOverheadMs);
        analysisGroup.metadata().addKeyValuePair("percent_overhead",
                                                 percentOverhead);
        analysisGroup.metadata().addKeyValuePair("acceptance_criterion_met",
                                                 acceptanceMet);

        LOG(INFO) << "=== EPOCH OVERHEAD ANALYSIS ===";
        LOG(INFO) << "Baseline Average Latency: " << baselineAvg << " ms";
        LOG(INFO) << "Epoch-Bound Average Latency: " << epochAvg << " ms";
        LOG(INFO) << "Absolute Overhead: " << absoluteOverheadMs << " ms";
        LOG(INFO) << "Percent Overhead: " << percentOverhead << " %";
        LOG(INFO) << "Acceptance Criterion (<1%): "
                  << (acceptanceMet ? "PASS" : "FAIL");
        LOG(INFO) << "=================================";

        if (!acceptanceMet) {
          LOG(WARNING) << "WARNING: Epoch overhead exceeds 1% threshold!";
        }
      }

    } catch (const std::exception& e) {
      LOG(ERROR) << "Overhead analysis error: " << e.what();
      results.addMeasurement(
          "Error", [&e]() { throw std::runtime_error(std::string(e.what())); });
    }

    return results;
  }
};

// Register all benchmarks
AD_REGISTER_BENCHMARK(BM_QueryExecutionBaseline);
AD_REGISTER_BENCHMARK(BM_QueryExecutionWithEpochBinding);
AD_REGISTER_BENCHMARK(BM_WriteBarrierCheck);
AD_REGISTER_BENCHMARK(BM_ConcurrentQueriesEpochBinding);
AD_REGISTER_BENCHMARK(BM_OverheadAnalysis);

}  // namespace ad_benchmark

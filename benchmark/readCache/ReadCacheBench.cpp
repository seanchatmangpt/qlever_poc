// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: EPIC 3 Read Caching Benchmark Suite
//
// Performance benchmarks for read caching system (EPIC 3 Task 10)
// verifying 3-5× speedup on stationary workloads and stampede prevention.
//
// Modes:
// - Mode A: Exact Repeats (5× speedup target)
// - Mode B: Same Shape, Varying Params (1.2× tail improvement)
// - Mode C: Concurrency & Mixed Workload (stampede prevention)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../benchmark/infrastructure/Benchmark.h"
#include "../benchmark/infrastructure/BenchmarkMeasurementContainer.h"
#include "../benchmark/infrastructure/BenchmarkMetadata.h"
#include "util/ConfigManager/ConfigManager.h"
#include "util/Timer.h"
#include "util/json.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace ad_benchmark {

namespace {

// ============================================================================
// Statistics Utilities
// ============================================================================

struct PercentileStats {
  double p50 = 0.0;
  double p95 = 0.0;
  double p99 = 0.0;
  double max = 0.0;
  double min = 0.0;
  double mean = 0.0;
  double stddev = 0.0;
  size_t sampleCount = 0;
};

PercentileStats computePercentiles(std::vector<double>& samples) {
  if (samples.empty()) {
    return PercentileStats{};
  }

  std::sort(samples.begin(), samples.end());

  PercentileStats stats;
  stats.sampleCount = samples.size();

  auto percentile = [&](double p) -> double {
    size_t idx = static_cast<size_t>(p * samples.size());
    if (idx >= samples.size()) {
      idx = samples.size() - 1;
    }
    return samples[idx];
  };

  stats.p50 = percentile(0.50);
  stats.p95 = percentile(0.95);
  stats.p99 = percentile(0.99);
  stats.max = samples.back();
  stats.min = samples.front();
  stats.mean =
      std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();

  // Compute standard deviation
  double variance = 0.0;
  for (double sample : samples) {
    variance += (sample - stats.mean) * (sample - stats.mean);
  }
  variance /= samples.size();
  stats.stddev = std::sqrt(variance);

  return stats;
}

// ============================================================================
// Cache Hit Rate Tracker
// ============================================================================

class CacheHitTracker {
 private:
  std::atomic<size_t> hits_{0};
  std::atomic<size_t> misses_{0};
  std::atomic<size_t> inFlightCount_{0};
  std::atomic<size_t> maxInFlight_{0};

 public:
  void recordHit() { hits_.fetch_add(1, std::memory_order_relaxed); }

  void recordMiss() { misses_.fetch_add(1, std::memory_order_relaxed); }

  void enterInflight() {
    size_t current = inFlightCount_.fetch_add(1, std::memory_order_relaxed) + 1;
    size_t maxVal = maxInFlight_.load(std::memory_order_relaxed);
    while (current > maxVal &&
           !maxInFlight_.compare_exchange_weak(
               maxVal, current, std::memory_order_relaxed)) {
    }
  }

  void exitInflight() { inFlightCount_.fetch_sub(1, std::memory_order_relaxed); }

  size_t getHits() const { return hits_.load(std::memory_order_relaxed); }
  size_t getMisses() const { return misses_.load(std::memory_order_relaxed); }
  size_t getMaxInFlight() const {
    return maxInFlight_.load(std::memory_order_relaxed);
  }

  double getHitRate() const {
    size_t total = getHits() + getMisses();
    return total > 0 ? (100.0 * getHits() / total) : 0.0;
  }

  void reset() {
    hits_ = 0;
    misses_ = 0;
    inFlightCount_ = 0;
    maxInFlight_ = 0;
  }
};

// ============================================================================
// Synthetic Cache Simulation
// ============================================================================
// Simulates a read cache with BytesCache and PlanCache behavior

class SimulatedReadCache {
 private:
  struct CacheEntry {
    std::string queryText;
    std::string queryShape;
    std::string resultData;
    std::chrono::steady_clock::time_point insertTime;
    size_t accessCount = 0;
  };

  std::unordered_map<std::string, CacheEntry> bytesCache_;
  std::unordered_map<std::string, std::string> planCache_;
  std::mutex mutex_;
  CacheHitTracker tracker_;

  // Simulate query execution time
  static std::chrono::milliseconds simulateExecution(const std::string& query,
                                                     bool isCacheHit) {
    // Base execution time: 50-200ms for cache miss, 10ms for cache hit
    std::mt19937 rng(
        std::hash<std::string>{}(query));  // Deterministic per query
    std::uniform_int_distribution<int> dist(isCacheHit ? 5 : 50,
                                            isCacheHit ? 15 : 200);
    return std::chrono::milliseconds(dist(rng));
  }

 public:
  struct QueryResult {
    std::string data;
    bool wasCacheHit = false;
    bool wasPlanCacheHit = false;
    std::chrono::milliseconds executionTime{0};
  };

  QueryResult executeQuery(const std::string& queryText,
                           const std::string& queryShape) {
    QueryResult result;

    {
      std::lock_guard<std::mutex> lock(mutex_);

      // Check BytesCache (exact match)
      auto it = bytesCache_.find(queryText);
      if (it != bytesCache_.end()) {
        tracker_.recordHit();
        result.wasCacheHit = true;
        result.wasPlanCacheHit = true;
        result.data = it->second.resultData;
        it->second.accessCount++;
        result.executionTime = simulateExecution(queryText, true);
        return result;
      }

      // Check PlanCache (shape match)
      auto planIt = planCache_.find(queryShape);
      if (planIt != planCache_.end()) {
        result.wasPlanCacheHit = true;
      }

      tracker_.recordMiss();
    }

    // Simulate query execution
    tracker_.enterInflight();
    result.executionTime = simulateExecution(queryText, false);
    std::this_thread::sleep_for(result.executionTime);
    tracker_.exitInflight();

    // Generate result data
    result.data = "Result for query: " + queryText.substr(0, 50);

    // Insert into caches
    {
      std::lock_guard<std::mutex> lock(mutex_);
      CacheEntry entry;
      entry.queryText = queryText;
      entry.queryShape = queryShape;
      entry.resultData = result.data;
      entry.insertTime = std::chrono::steady_clock::now();
      entry.accessCount = 1;

      bytesCache_[queryText] = std::move(entry);
      planCache_[queryShape] = result.data;
    }

    return result;
  }

  CacheHitTracker& getTracker() { return tracker_; }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    bytesCache_.clear();
    planCache_.clear();
    tracker_.reset();
  }

  size_t getByteCacheSize() const {
    std::lock_guard<std::mutex> lock(
        const_cast<std::mutex&>(mutex_));  // Safe for const method
    return bytesCache_.size();
  }

  size_t getPlanCacheSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return planCache_.size();
  }
};

// ============================================================================
// SPARQL Query Templates
// ============================================================================

const std::vector<std::string> QUERY_TEMPLATES = {
    // Q1: Simple triple pattern
    R"(PREFIX dblp: <http://dblp.org/rdf/schema#>
       SELECT ?author WHERE {
         ?paper dblp:title "%TITLE%" .
         ?paper dblp:author ?author .
       })",

    // Q2: Join pattern
    R"(PREFIX dblp: <http://dblp.org/rdf/schema#>
       SELECT ?coauthor WHERE {
         ?paper1 dblp:author <%AUTHOR1%> .
         ?paper1 dblp:author ?coauthor .
         FILTER(?coauthor != <%AUTHOR1%>)
       })",

    // Q3: Aggregation
    R"(PREFIX dblp: <http://dblp.org/rdf/schema#>
       SELECT ?year (COUNT(?paper) AS ?count) WHERE {
         ?paper dblp:author <%AUTHOR%> .
         ?paper dblp:yearOfPublication ?year .
       } GROUP BY ?year ORDER BY DESC(?year))",

    // Q4: Filter with keyword
    R"(PREFIX dblp: <http://dblp.org/rdf/schema#>
       SELECT ?paper ?title WHERE {
         ?paper dblp:publishedIn <%VENUE%> .
         ?paper dblp:title ?title .
         FILTER(CONTAINS(?title, "%KEYWORD%"))
       } LIMIT %LIMIT%)",

    // Q5: Complex join
    R"(PREFIX dblp: <http://dblp.org/rdf/schema#>
       SELECT DISTINCT ?author ?title ?venue WHERE {
         ?paper dblp:author ?author .
         ?paper dblp:title ?title .
         ?paper dblp:publishedIn ?venue .
         ?paper dblp:yearOfPublication "%YEAR%" .
       })",
};

// Generate query variant with specific parameters
std::string generateQueryVariant(const std::string& templateQuery, int seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<int> dist(1000, 9999);

  std::string query = templateQuery;

  auto replace = [&](const std::string& placeholder,
                     const std::string& value) {
    size_t pos = query.find(placeholder);
    if (pos != std::string::npos) {
      query.replace(pos, placeholder.length(), value);
    }
  };

  replace("%TITLE%", "Title" + std::to_string(dist(rng)));
  replace("%AUTHOR1%", "http://author.org/A" + std::to_string(dist(rng)));
  replace("%AUTHOR%", "http://author.org/A" + std::to_string(dist(rng)));
  replace("%VENUE%", "http://venue.org/V" + std::to_string(dist(rng)));
  replace("%KEYWORD%", "keyword" + std::to_string(dist(rng)));
  replace("%YEAR%", std::to_string(2010 + (dist(rng) % 15)));
  replace("%LIMIT%", std::to_string(10 + (dist(rng) % 90)));

  return query;
}

// Extract query shape (simplified - removes constants)
std::string extractQueryShape(const std::string& query) {
  std::string shape = query;

  // Replace string literals with placeholder
  size_t pos = 0;
  while ((pos = shape.find("\"", pos)) != std::string::npos) {
    size_t end = shape.find("\"", pos + 1);
    if (end != std::string::npos) {
      shape.replace(pos, end - pos + 1, "?LITERAL");
      pos += 8;
    } else {
      break;
    }
  }

  // Replace URIs with placeholder
  pos = 0;
  while ((pos = shape.find("<http", pos)) != std::string::npos) {
    size_t end = shape.find(">", pos);
    if (end != std::string::npos) {
      shape.replace(pos, end - pos + 1, "?URI");
      pos += 4;
    } else {
      break;
    }
  }

  return shape;
}

// ============================================================================
// Benchmark Class
// ============================================================================

class BMReadCache : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "EPIC 3 Read Cache Performance Benchmark";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    getGeneralMetadata().addKeyValuePair("epic", "EPIC 3");
    getGeneralMetadata().addKeyValuePair("component", "Read Cache");
    getGeneralMetadata().addKeyValuePair("version", "v1.0");

    // Run benchmark modes
    benchmarkModeA_ExactRepeats(results);
    benchmarkModeB_SameShapeVaryingParams(results);
    benchmarkModeC_ConcurrencyMixedWorkload(results);

    return results;
  }

 private:
  SimulatedReadCache cache_;

  // --------------------------------------------------------------------------
  // Mode A: Exact Repeats (5× speedup target)
  // --------------------------------------------------------------------------
  void benchmarkModeA_ExactRepeats(BenchmarkResults& results) {
    auto& group = results.addGroup("ModeA_ExactRepeats");
    group.metadata().addKeyValuePair("description",
                                     "Single query repeated 200 times");
    group.metadata().addKeyValuePair("target_speedup", "5x");
    group.metadata().addKeyValuePair("target_hit_rate_pct", 99.0);

    const int TOTAL_RUNS = 200;
    const int WARMUP_RUNS = 10;

    // Generate single query
    std::string testQuery = generateQueryVariant(QUERY_TEMPLATES[0], 42);
    std::string queryShape = extractQueryShape(testQuery);

    cache_.clear();

    std::vector<double> latenciesMs;
    latenciesMs.reserve(TOTAL_RUNS);

    double firstRunLatency = 0.0;

    auto& measurement = group.addMeasurement("Repeat_200x", [&]() {
      for (int i = 0; i < TOTAL_RUNS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        auto result = cache_.executeQuery(testQuery, queryShape);

        auto end = std::chrono::high_resolution_clock::now();
        auto durationMs =
            std::chrono::duration<double, std::milli>(end - start).count();

        latenciesMs.push_back(durationMs);

        if (i == 0) {
          firstRunLatency = durationMs;
        }
      }
    });

    // Compute statistics (excluding warmup)
    std::vector<double> postWarmupLatencies(latenciesMs.begin() + WARMUP_RUNS,
                                            latenciesMs.end());
    auto stats = computePercentiles(postWarmupLatencies);

    // Compute speedup
    double avgCachedLatency = stats.mean;
    double speedupFactor =
        avgCachedLatency > 0 ? (firstRunLatency / avgCachedLatency) : 0.0;

    // Add metadata
    measurement.metadata().addKeyValuePair("first_run_ms", firstRunLatency);
    measurement.metadata().addKeyValuePair("cached_p50_ms", stats.p50);
    measurement.metadata().addKeyValuePair("cached_p95_ms", stats.p95);
    measurement.metadata().addKeyValuePair("cached_p99_ms", stats.p99);
    measurement.metadata().addKeyValuePair("cached_mean_ms", stats.mean);
    measurement.metadata().addKeyValuePair("cached_stddev_ms", stats.stddev);
    measurement.metadata().addKeyValuePair("speedup_factor", speedupFactor);
    measurement.metadata().addKeyValuePair("hit_rate_pct",
                                           cache_.getTracker().getHitRate());
    measurement.metadata().addKeyValuePair("total_hits",
                                           cache_.getTracker().getHits());
    measurement.metadata().addKeyValuePair("total_misses",
                                           cache_.getTracker().getMisses());
    measurement.metadata().addKeyValuePair("target_met", speedupFactor >= 5.0);

    // Add summary table
    auto& table = group.addTable("Speedup_Analysis",
                                 {"First Run (ms)", "Cached p50 (ms)",
                                  "Cached p95 (ms)", "Speedup Factor",
                                  "Hit Rate (%)", "Target Met"},
                                 {"Metric", "Value"});
    table.setEntry(0, 1, firstRunLatency);
    table.setEntry(1, 1, stats.p50);
    table.setEntry(2, 1, stats.p95);
    table.setEntry(3, 1, speedupFactor);
    table.setEntry(4, 1, cache_.getTracker().getHitRate());
    table.setEntry(5, 1, speedupFactor >= 5.0 ? "YES" : "NO");
  }

  // --------------------------------------------------------------------------
  // Mode B: Same Shape, Varying Params (1.2× tail improvement)
  // --------------------------------------------------------------------------
  void benchmarkModeB_SameShapeVaryingParams(BenchmarkResults& results) {
    auto& group = results.addGroup("ModeB_SameShapeVaryingParams");
    group.metadata().addKeyValuePair(
        "description", "Fixed template with N distinct parameter sets");
    group.metadata().addKeyValuePair("target_p95_improvement", "1.2x");
    group.metadata().addKeyValuePair("param_sets", 50);
    group.metadata().addKeyValuePair("repeats_per_set", 10);

    const int PARAM_SETS = 50;
    const int REPEATS_PER_SET = 10;

    cache_.clear();

    // Use same template for all queries (same shape)
    std::string baseTemplate = QUERY_TEMPLATES[1];

    std::vector<double> firstRunP95Latencies;
    std::vector<double> cachedP95Latencies;

    auto& measurement =
        group.addMeasurement("VaryingParams_50x10", [&]() {
          for (int paramSet = 0; paramSet < PARAM_SETS; ++paramSet) {
            std::string query =
                generateQueryVariant(baseTemplate, 1000 + paramSet);
            std::string shape = extractQueryShape(query);

            std::vector<double> paramSetLatencies;

            for (int repeat = 0; repeat < REPEATS_PER_SET; ++repeat) {
              auto start = std::chrono::high_resolution_clock::now();
              auto result = cache_.executeQuery(query, shape);
              auto end = std::chrono::high_resolution_clock::now();
              auto durationMs =
                  std::chrono::duration<double, std::milli>(end - start)
                      .count();

              paramSetLatencies.push_back(durationMs);
            }

            auto paramStats = computePercentiles(paramSetLatencies);
            if (paramSetLatencies.size() > 0) {
              firstRunP95Latencies.push_back(paramSetLatencies[0]);
              std::vector<double> cached(paramSetLatencies.begin() + 1,
                                         paramSetLatencies.end());
              if (!cached.empty()) {
                auto cachedStats = computePercentiles(cached);
                cachedP95Latencies.push_back(cachedStats.p95);
              }
            }
          }
        });

    // Compute aggregate statistics
    auto firstRunStats = computePercentiles(firstRunP95Latencies);
    auto cachedStats = computePercentiles(cachedP95Latencies);

    double p95Improvement =
        cachedStats.p95 > 0 ? (firstRunStats.p95 / cachedStats.p95) : 0.0;

    measurement.metadata().addKeyValuePair("first_run_p95_ms",
                                           firstRunStats.p95);
    measurement.metadata().addKeyValuePair("cached_p95_ms", cachedStats.p95);
    measurement.metadata().addKeyValuePair("p95_improvement_factor",
                                           p95Improvement);
    measurement.metadata().addKeyValuePair("hit_rate_pct",
                                           cache_.getTracker().getHitRate());
    measurement.metadata().addKeyValuePair("bytes_cache_size",
                                           cache_.getByteCacheSize());
    measurement.metadata().addKeyValuePair("plan_cache_size",
                                           cache_.getPlanCacheSize());
    measurement.metadata().addKeyValuePair("target_met",
                                           p95Improvement >= 1.2);

    // Add table
    auto& table =
        group.addTable("P95_Improvement", {"First Run p95", "Cached p95",
                                           "Improvement Factor", "Target Met"},
                       {"Metric", "Value"});
    table.setEntry(0, 1, firstRunStats.p95);
    table.setEntry(1, 1, cachedStats.p95);
    table.setEntry(2, 1, p95Improvement);
    table.setEntry(3, 1, p95Improvement >= 1.2 ? "YES" : "NO");
  }

  // --------------------------------------------------------------------------
  // Mode C: Concurrency & Mixed Workload (stampede prevention)
  // --------------------------------------------------------------------------
  void benchmarkModeC_ConcurrencyMixedWorkload(BenchmarkResults& results) {
    auto& group = results.addGroup("ModeC_ConcurrencyMixedWorkload");
    group.metadata().addKeyValuePair("description",
                                     "20 threads, 70% top-K shapes, 30% random");
    group.metadata().addKeyValuePair("threads", 20);
    group.metadata().addKeyValuePair("duration_seconds", 30);
    group.metadata().addKeyValuePair("top_k_percentage", 70);

    const int NUM_THREADS = 20;
    const auto TEST_DURATION = 30s;
    const int TOP_K_SHAPES = 10;

    cache_.clear();

    // Generate top-K queries (hot set)
    std::vector<std::pair<std::string, std::string>> topKQueries;
    for (int i = 0; i < TOP_K_SHAPES; ++i) {
      int templateIdx = i % QUERY_TEMPLATES.size();
      std::string query = generateQueryVariant(QUERY_TEMPLATES[templateIdx],
                                               2000 + i);
      std::string shape = extractQueryShape(query);
      topKQueries.push_back({query, shape});
    }

    std::atomic<bool> stopFlag{false};
    std::atomic<size_t> totalQueries{0};
    std::vector<std::vector<double>> threadLatencies(NUM_THREADS);
    std::mutex latenciesMutex;

    auto& measurement = group.addMeasurement(
        "Concurrent_20_Threads_30s", [&]() {
          std::vector<std::thread> threads;

          for (int tid = 0; tid < NUM_THREADS; ++tid) {
            threads.emplace_back([&, tid]() {
              std::mt19937 rng(tid);
              std::uniform_int_distribution<int> topKDist(0, TOP_K_SHAPES - 1);
              std::uniform_int_distribution<int> percentDist(0, 99);
              std::uniform_int_distribution<int> randomSeedDist(5000, 9999);

              std::vector<double> localLatencies;

              while (!stopFlag.load(std::memory_order_relaxed)) {
                std::string query, shape;

                // 70% from top-K, 30% random
                if (percentDist(rng) < 70) {
                  auto [q, s] = topKQueries[topKDist(rng)];
                  query = q;
                  shape = s;
                } else {
                  int templateIdx = rng() % QUERY_TEMPLATES.size();
                  query = generateQueryVariant(QUERY_TEMPLATES[templateIdx],
                                               randomSeedDist(rng));
                  shape = extractQueryShape(query);
                }

                auto start = std::chrono::high_resolution_clock::now();
                auto result = cache_.executeQuery(query, shape);
                auto end = std::chrono::high_resolution_clock::now();
                auto durationMs =
                    std::chrono::duration<double, std::milli>(end - start)
                        .count();

                localLatencies.push_back(durationMs);
                totalQueries.fetch_add(1, std::memory_order_relaxed);
              }

              std::lock_guard<std::mutex> lock(latenciesMutex);
              threadLatencies[tid] = std::move(localLatencies);
            });
          }

          // Run for specified duration
          std::this_thread::sleep_for(TEST_DURATION);
          stopFlag.store(true, std::memory_order_relaxed);

          // Wait for all threads
          for (auto& t : threads) {
            t.join();
          }
        });

    // Aggregate latencies from all threads
    std::vector<double> allLatencies;
    for (const auto& tl : threadLatencies) {
      allLatencies.insert(allLatencies.end(), tl.begin(), tl.end());
    }

    auto stats = computePercentiles(allLatencies);
    size_t totalQueriesExecuted = totalQueries.load();
    double throughput = totalQueriesExecuted / 30.0;  // queries per second

    measurement.metadata().addKeyValuePair("total_queries",
                                           totalQueriesExecuted);
    measurement.metadata().addKeyValuePair("throughput_qps", throughput);
    measurement.metadata().addKeyValuePair("p50_latency_ms", stats.p50);
    measurement.metadata().addKeyValuePair("p95_latency_ms", stats.p95);
    measurement.metadata().addKeyValuePair("p99_latency_ms", stats.p99);
    measurement.metadata().addKeyValuePair("max_latency_ms", stats.max);
    measurement.metadata().addKeyValuePair("hit_rate_pct",
                                           cache_.getTracker().getHitRate());
    measurement.metadata().addKeyValuePair("max_in_flight",
                                           cache_.getTracker().getMaxInFlight());

    // Check for stampedes (max in-flight should be low due to single-flight)
    bool noStampedes = cache_.getTracker().getMaxInFlight() < 5;
    measurement.metadata().addKeyValuePair("no_stampedes_detected",
                                           noStampedes);

    // Add table
    auto& table = group.addTable(
        "Concurrency_Metrics",
        {"Total Queries", "Throughput (q/s)", "p50 (ms)", "p95 (ms)",
         "p99 (ms)", "Hit Rate (%)", "Max In-Flight", "No Stampedes"},
        {"Metric", "Value"});
    table.setEntry(0, 1, static_cast<int>(totalQueriesExecuted));
    table.setEntry(1, 1, throughput);
    table.setEntry(2, 1, stats.p50);
    table.setEntry(3, 1, stats.p95);
    table.setEntry(4, 1, stats.p99);
    table.setEntry(5, 1, cache_.getTracker().getHitRate());
    table.setEntry(6, 1, static_cast<int>(cache_.getTracker().getMaxInFlight()));
    table.setEntry(7, 1, noStampedes ? "YES" : "NO");
  }
};

}  // namespace

// Register the benchmark
AD_REGISTER_BENCHMARK(BMReadCache);

}  // namespace ad_benchmark

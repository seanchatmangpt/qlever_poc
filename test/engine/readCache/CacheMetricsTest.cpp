// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant

#include "engine/readCache/CacheMetrics.h"

#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "engine/readCache/MetricsExport.h"
#include "global/EpochMetrics.h"

namespace readCache {

class CacheMetricsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset metrics before each test
    ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
      const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics).reset();
    });
  }

  void TearDown() override {
    // Reset metrics after each test
    ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
      const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics).reset();
    });
  }

  ad_utility::ReadCacheMetrics getMetrics() {
    return ad_utility::globalReadCacheMetrics.withReadLock(
        [](const auto& collector) { return collector.getMetrics(); });
  }
};

// ============================================================================
// Basic metric recording tests
// ============================================================================

TEST_F(CacheMetricsTest, BytesCacheMetrics) {
  CacheMetrics::recordBytesHit(1024);
  CacheMetrics::recordBytesHit(2048);
  CacheMetrics::recordBytesMiss();
  CacheMetrics::recordBytesInsert(512);
  CacheMetrics::recordBytesEvict(256);

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.bytes_hits, 2);
  EXPECT_EQ(metrics.bytes_misses, 1);
  EXPECT_EQ(metrics.bytes_inserts, 1);
  EXPECT_EQ(metrics.bytes_evicts, 1);
  EXPECT_EQ(metrics.bytes_served_from_cache, 3072);  // 1024 + 2048
}

TEST_F(CacheMetricsTest, PlanCacheMetrics) {
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordPlanMiss();
  CacheMetrics::recordPlanInsert();
  CacheMetrics::recordPlanEvict();

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.plan_hits, 3);
  EXPECT_EQ(metrics.plan_misses, 1);
  EXPECT_EQ(metrics.plan_inserts, 1);
  EXPECT_EQ(metrics.plan_evicts, 1);
}

TEST_F(CacheMetricsTest, NegativeCacheMetrics) {
  CacheMetrics::recordNegativeHit();
  CacheMetrics::recordNegativeHit();
  CacheMetrics::recordNegativeInsert();

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.neg_hits, 2);
  EXPECT_EQ(metrics.neg_inserts, 1);
}

TEST_F(CacheMetricsTest, InflightWaiters) {
  CacheMetrics::recordInflightWaiterAdd();
  CacheMetrics::recordInflightWaiterAdd();
  CacheMetrics::recordInflightWaiterAdd();

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.inflight_waiters, 3);

  CacheMetrics::recordInflightWaiterRemove();
  metrics = getMetrics();
  EXPECT_EQ(metrics.inflight_waiters, 2);

  CacheMetrics::recordInflightWaiterRemove();
  CacheMetrics::recordInflightWaiterRemove();
  metrics = getMetrics();
  EXPECT_EQ(metrics.inflight_waiters, 0);

  // Test underflow protection
  CacheMetrics::recordInflightWaiterRemove();
  metrics = getMetrics();
  EXPECT_EQ(metrics.inflight_waiters, 0);  // Should not go negative
}

TEST_F(CacheMetricsTest, ShapeLatency) {
  std::string shape1 = "0123456789abcdef0123456789abcdef";
  std::string shape2 = "fedcba9876543210fedcba9876543210";

  CacheMetrics::recordShapeLatency(shape1, 1000);
  CacheMetrics::recordShapeLatency(shape1, 2000);
  CacheMetrics::recordShapeLatency(shape1, 3000);
  CacheMetrics::recordShapeLatency(shape2, 5000);

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.shape_latency.size(), 2);

  auto& stats1 = metrics.shape_latency[shape1];
  EXPECT_EQ(stats1.count_, 3);
  EXPECT_EQ(stats1.avg(), 2000);  // (1000 + 2000 + 3000) / 3
  EXPECT_EQ(stats1.min_us_, 1000);
  EXPECT_EQ(stats1.max_us_, 3000);

  auto& stats2 = metrics.shape_latency[shape2];
  EXPECT_EQ(stats2.count_, 1);
  EXPECT_EQ(stats2.avg(), 5000);
}

TEST_F(CacheMetricsTest, EpochPromote) {
  CacheMetrics::recordEpochPromote(1234);
  CacheMetrics::recordEpochPromote(5678);

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.epoch_promote_count, 2);
  EXPECT_EQ(metrics.prewarm_duration_ms, 5678);  // Last value
}

// ============================================================================
// Latency statistics tests
// ============================================================================

TEST_F(CacheMetricsTest, LatencyPercentiles) {
  std::string shape = "test_shape";

  // Record 100 samples with known distribution
  for (int i = 1; i <= 100; ++i) {
    CacheMetrics::recordShapeLatency(shape, i * 100);  // 100us, 200us, ...,
                                                        // 10000us
  }

  auto metrics = getMetrics();
  auto& stats = metrics.shape_latency[shape];

  EXPECT_EQ(stats.count_, 100);
  EXPECT_EQ(stats.avg(), 5050);  // Average of 100..10000
  EXPECT_EQ(stats.min_us_, 100);
  EXPECT_EQ(stats.max_us_, 10000);

  // Check percentiles (approximate due to integer division)
  EXPECT_NEAR(stats.p50(), 5000, 500);
  EXPECT_NEAR(stats.p95(), 9500, 500);
  EXPECT_NEAR(stats.p99(), 9900, 500);
}

// ============================================================================
// Thread safety tests
// ============================================================================

TEST_F(CacheMetricsTest, ConcurrentBytesMetrics) {
  constexpr int NUM_THREADS = 10;
  constexpr int OPS_PER_THREAD = 1000;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([&]() {
      for (int i = 0; i < OPS_PER_THREAD; ++i) {
        CacheMetrics::recordBytesHit(100);
        CacheMetrics::recordBytesMiss();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.bytes_hits, NUM_THREADS * OPS_PER_THREAD);
  EXPECT_EQ(metrics.bytes_misses, NUM_THREADS * OPS_PER_THREAD);
  EXPECT_EQ(metrics.bytes_served_from_cache,
            NUM_THREADS * OPS_PER_THREAD * 100);
}

TEST_F(CacheMetricsTest, ConcurrentPlanMetrics) {
  constexpr int NUM_THREADS = 10;
  constexpr int OPS_PER_THREAD = 1000;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([&]() {
      for (int i = 0; i < OPS_PER_THREAD; ++i) {
        CacheMetrics::recordPlanHit();
        CacheMetrics::recordPlanMiss();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto metrics = getMetrics();
  EXPECT_EQ(metrics.plan_hits, NUM_THREADS * OPS_PER_THREAD);
  EXPECT_EQ(metrics.plan_misses, NUM_THREADS * OPS_PER_THREAD);
}

TEST_F(CacheMetricsTest, ConcurrentShapeLatency) {
  constexpr int NUM_THREADS = 10;
  constexpr int OPS_PER_THREAD = 100;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([t]() {
      std::string shape = "shape_" + std::to_string(t % 3);
      for (int i = 0; i < OPS_PER_THREAD; ++i) {
        CacheMetrics::recordShapeLatency(shape, 1000 + i);
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  auto metrics = getMetrics();
  // We have 3 unique shapes (shape_0, shape_1, shape_2)
  EXPECT_EQ(metrics.shape_latency.size(), 3);

  // Each shape should have approximately NUM_THREADS/3 * OPS_PER_THREAD
  // samples However, due to modulo distribution, we check total count
  uint64_t total_count = 0;
  for (const auto& [shape, stats] : metrics.shape_latency) {
    total_count += stats.count_;
  }
  EXPECT_EQ(total_count, NUM_THREADS * OPS_PER_THREAD);
}

// ============================================================================
// Metrics export tests
// ============================================================================

TEST_F(CacheMetricsTest, JsonExport) {
  CacheMetrics::recordBytesHit(1024);
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordNegativeHit();
  CacheMetrics::recordShapeLatency("test_shape", 5000);

  std::string json = exportMetricsJson();

  // Verify JSON contains expected fields
  EXPECT_NE(json.find("\"bytes\""), std::string::npos);
  EXPECT_NE(json.find("\"plan\""), std::string::npos);
  EXPECT_NE(json.find("\"negative\""), std::string::npos);
  EXPECT_NE(json.find("\"shape_latency\""), std::string::npos);
  EXPECT_NE(json.find("\"hits\": 1"), std::string::npos);
}

TEST_F(CacheMetricsTest, PrometheusExport) {
  CacheMetrics::recordBytesHit(1024);
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordNegativeHit();

  std::string prom = exportMetricsPrometheus();

  // Verify Prometheus format
  EXPECT_NE(prom.find("# HELP"), std::string::npos);
  EXPECT_NE(prom.find("# TYPE"), std::string::npos);
  EXPECT_NE(prom.find("qlever_read_cache_bytes_hits_total"),
            std::string::npos);
  EXPECT_NE(prom.find("qlever_read_cache_plan_hits_total"), std::string::npos);
  EXPECT_NE(prom.find("qlever_read_cache_negative_hits_total"),
            std::string::npos);
}

TEST_F(CacheMetricsTest, PrometheusHitRates) {
  // Record some hits and misses
  for (int i = 0; i < 80; ++i) CacheMetrics::recordBytesHit(100);
  for (int i = 0; i < 20; ++i) CacheMetrics::recordBytesMiss();

  for (int i = 0; i < 90; ++i) CacheMetrics::recordPlanHit();
  for (int i = 0; i < 10; ++i) CacheMetrics::recordPlanMiss();

  std::string prom = exportMetricsPrometheus();

  // Should have hit rate metrics
  EXPECT_NE(prom.find("qlever_read_cache_bytes_hit_rate"), std::string::npos);
  EXPECT_NE(prom.find("qlever_read_cache_plan_hit_rate"), std::string::npos);

  // Hit rates should be 0.8 and 0.9 respectively
  EXPECT_NE(prom.find("0.8"), std::string::npos);
  EXPECT_NE(prom.find("0.9"), std::string::npos);
}

// ============================================================================
// Reset functionality test
// ============================================================================

TEST_F(CacheMetricsTest, Reset) {
  CacheMetrics::recordBytesHit(1024);
  CacheMetrics::recordPlanHit();
  CacheMetrics::recordNegativeHit();
  CacheMetrics::recordShapeLatency("test_shape", 5000);

  auto metrics = getMetrics();
  EXPECT_GT(metrics.bytes_hits, 0);

  // Reset
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& collector) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(collector).reset();
  });

  metrics = getMetrics();
  EXPECT_EQ(metrics.bytes_hits, 0);
  EXPECT_EQ(metrics.plan_hits, 0);
  EXPECT_EQ(metrics.neg_hits, 0);
  EXPECT_EQ(metrics.shape_latency.size(), 0);
}

// ============================================================================
// Logging tests (verify no crashes, sampling is hard to test deterministically)
// ============================================================================

TEST_F(CacheMetricsTest, LoggingDoesNotCrash) {
  // These should not crash even if called many times
  for (int i = 0; i < 1000; ++i) {
    CacheMetrics::logCacheHit("bytes", "test_key");
    CacheMetrics::logCacheMiss("plan", "test_key");
    CacheMetrics::logAdmissionDecision("negative", true, "test reason");
  }

  // These always log (not sampled)
  CacheMetrics::logEpochPromote(1234);
  CacheMetrics::logPrewarmComplete(5678, 100);
}

}  // namespace readCache

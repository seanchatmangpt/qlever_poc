// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant

#include "engine/readCache/CacheMetrics.h"

#include "global/EpochMetrics.h"

namespace readCache {

// Thread-local RNG for sampling
thread_local std::mt19937 CacheMetrics::rng_{std::random_device{}()};
thread_local std::uniform_int_distribution<int> CacheMetrics::dist_{
    0, LOG_SAMPLE_RATE - 1};

// ============================================================================
// Metric recording methods (delegate to global metrics collector)
// ============================================================================

void CacheMetrics::recordBytesHit(uint64_t bytes) {
  ad_utility::globalReadCacheMetrics.withReadLock(
      [bytes](const auto& metrics) {
        const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
            .recordBytesHit(bytes);
      });
}

void CacheMetrics::recordBytesMiss() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordBytesMiss();
  });
}

void CacheMetrics::recordBytesInsert(uint64_t bytes) {
  ad_utility::globalReadCacheMetrics.withReadLock(
      [bytes](const auto& metrics) {
        const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
            .recordBytesInsert(bytes);
      });
}

void CacheMetrics::recordBytesEvict(uint64_t bytes) {
  ad_utility::globalReadCacheMetrics.withReadLock(
      [bytes](const auto& metrics) {
        const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
            .recordBytesEvict(bytes);
      });
}

void CacheMetrics::recordPlanHit() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics).recordPlanHit();
  });
}

void CacheMetrics::recordPlanMiss() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordPlanMiss();
  });
}

void CacheMetrics::recordPlanInsert() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordPlanInsert();
  });
}

void CacheMetrics::recordPlanEvict() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordPlanEvict();
  });
}

void CacheMetrics::recordNegativeHit() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordNegativeHit();
  });
}

void CacheMetrics::recordNegativeInsert() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordNegativeInsert();
  });
}

void CacheMetrics::recordInflightWaiterAdd() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordInflightWaiterAdd();
  });
}

void CacheMetrics::recordInflightWaiterRemove() {
  ad_utility::globalReadCacheMetrics.withReadLock([](const auto& metrics) {
    const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
        .recordInflightWaiterRemove();
  });
}

void CacheMetrics::recordShapeLatency(const std::string& shape_sha256,
                                      uint64_t latency_us) {
  ad_utility::globalReadCacheMetrics.withReadLock(
      [&shape_sha256, latency_us](const auto& metrics) {
        const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
            .recordShapeLatency(shape_sha256, latency_us);
      });
}

void CacheMetrics::recordEpochPromote(uint64_t prewarm_ms) {
  ad_utility::globalReadCacheMetrics.withReadLock(
      [prewarm_ms](const auto& metrics) {
        const_cast<ad_utility::ReadCacheMetricsCollector&>(metrics)
            .recordEpochPromote(prewarm_ms);
      });
}

// ============================================================================
// Logging helpers with sampling
// ============================================================================

bool CacheMetrics::shouldLog() { return dist_(rng_) == 0; }

void CacheMetrics::logCacheHit(const std::string& cache_type,
                               const std::string& key_preview) {
  if (shouldLog()) {
    AD_LOG_DEBUG << "Cache HIT [" << cache_type << "] key=" << key_preview
                 << std::endl;
  }
}

void CacheMetrics::logCacheMiss(const std::string& cache_type,
                                const std::string& key_preview) {
  if (shouldLog()) {
    AD_LOG_DEBUG << "Cache MISS [" << cache_type << "] key=" << key_preview
                 << std::endl;
  }
}

void CacheMetrics::logAdmissionDecision(const std::string& cache_type,
                                        bool admitted,
                                        const std::string& reason) {
  if (shouldLog()) {
    AD_LOG_DEBUG << "Cache admission [" << cache_type
                 << "] admitted=" << (admitted ? "YES" : "NO")
                 << " reason=" << reason << std::endl;
  }
}

void CacheMetrics::logEpochPromote(uint64_t prewarm_ms) {
  AD_LOG_INFO << "Cache epoch promote completed in " << prewarm_ms << "ms"
              << std::endl;
}

void CacheMetrics::logPrewarmComplete(uint64_t duration_ms,
                                      size_t items_warmed) {
  AD_LOG_INFO << "Cache prewarm completed: " << items_warmed << " items in "
              << duration_ms << "ms" << std::endl;
}

}  // namespace readCache

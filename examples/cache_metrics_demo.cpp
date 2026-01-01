// Demo program showing cache metrics usage
// This demonstrates how to use the cache metrics system

#include <iostream>
#include "engine/readCache/CacheMetrics.h"
#include "engine/readCache/MetricsExport.h"
#include "global/EpochMetrics.h"

int main() {
  std::cout << "=== Cache Metrics Demo ===\n\n";

  // Simulate some cache operations
  std::cout << "Recording cache operations...\n";

  // Bytes cache
  readCache::CacheMetrics::recordBytesHit(1024);
  readCache::CacheMetrics::recordBytesHit(2048);
  readCache::CacheMetrics::recordBytesHit(4096);
  readCache::CacheMetrics::recordBytesMiss();
  readCache::CacheMetrics::recordBytesMiss();
  readCache::CacheMetrics::recordBytesInsert(8192);

  // Plan cache
  readCache::CacheMetrics::recordPlanHit();
  readCache::CacheMetrics::recordPlanHit();
  readCache::CacheMetrics::recordPlanMiss();
  readCache::CacheMetrics::recordPlanInsert();

  // Negative cache
  readCache::CacheMetrics::recordNegativeHit();
  readCache::CacheMetrics::recordNegativeInsert();

  // Shape latency
  readCache::CacheMetrics::recordShapeLatency("abc123def456", 1500);
  readCache::CacheMetrics::recordShapeLatency("abc123def456", 2500);
  readCache::CacheMetrics::recordShapeLatency("abc123def456", 3500);
  readCache::CacheMetrics::recordShapeLatency("xyz789uvw012", 5000);

  // Epoch promote
  readCache::CacheMetrics::recordEpochPromote(12345);

  std::cout << "\n=== JSON Export ===\n";
  std::string json = readCache::exportMetricsJson();
  std::cout << json << "\n";

  std::cout << "\n=== Prometheus Export ===\n";
  std::string prom = readCache::exportMetricsPrometheus();
  std::cout << prom << "\n";

  std::cout << "\n=== Detailed Report ===\n";
  auto report = ad_utility::globalReadCacheMetrics.withReadLock(
      [](const auto& collector) {
        return collector.getDetailedMetricsReport();
      });
  std::cout << report << "\n";

  return 0;
}

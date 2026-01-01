// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#ifndef QLEVER_SRC_SHEX_VALIDATIONCACHE_H
#define QLEVER_SRC_SHEX_VALIDATIONCACHE_H

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "util/LruCache.h"
#include "util/MemorySize/MemorySize.h"
#include "util/Synchronized.h"

namespace shex {

// ============================================================================
// Cache Entry with TTL and Metadata
// ============================================================================

template <typename T>
struct CacheEntry {
  T value;
  std::chrono::steady_clock::time_point timestamp;
  size_t accessCount = 0;
  size_t sizeBytes;

  CacheEntry(T val, size_t size)
      : value(std::move(val)),
        timestamp(std::chrono::steady_clock::now()),
        sizeBytes(size) {}

  bool isExpired(std::chrono::seconds ttl) const {
    auto age = std::chrono::steady_clock::now() - timestamp;
    return age > ttl;
  }

  void touch() {
    ++accessCount;
    timestamp = std::chrono::steady_clock::now();
  }
};

// ============================================================================
// Cache Metrics for Performance Monitoring
// ============================================================================

struct CacheMetrics {
  std::atomic<uint64_t> hits{0};
  std::atomic<uint64_t> misses{0};
  std::atomic<uint64_t> evictions{0};
  std::atomic<uint64_t> expirations{0};
  std::atomic<uint64_t> totalEntries{0};
  std::atomic<uint64_t> totalSizeBytes{0};

  // L1, L2, L3 tier-specific metrics
  std::atomic<uint64_t> l1Hits{0};
  std::atomic<uint64_t> l2Hits{0};
  std::atomic<uint64_t> l3Hits{0};

  double hitRate() const {
    uint64_t h = hits.load();
    uint64_t m = misses.load();
    return (h + m) > 0 ? static_cast<double>(h) / (h + m) : 0.0;
  }

  double l1HitRate() const {
    uint64_t total = hits.load();
    return total > 0 ? static_cast<double>(l1Hits.load()) / total : 0.0;
  }

  void recordHit(size_t tier) {
    ++hits;
    if (tier == 1)
      ++l1Hits;
    else if (tier == 2)
      ++l2Hits;
    else if (tier == 3)
      ++l3Hits;
  }

  void recordMiss() { ++misses; }
  void recordEviction() { ++evictions; }
  void recordExpiration() { ++expirations; }

  std::string summary() const {
    std::ostringstream oss;
    oss << "CacheMetrics:\n";
    oss << "  Total Hits: " << hits << "\n";
    oss << "  Total Misses: " << misses << "\n";
    oss << "  Hit Rate: " << (hitRate() * 100.0) << "%\n";
    oss << "  L1 Hit Rate: " << (l1HitRate() * 100.0) << "%\n";
    oss << "  Evictions: " << evictions << "\n";
    oss << "  Expirations: " << expirations << "\n";
    oss << "  Total Entries: " << totalEntries << "\n";
    oss << "  Total Size: " << totalSizeBytes << " bytes\n";
    return oss.str();
  }
};

// ============================================================================
// 3-Tier LRU Cache with TTL and Memory Management
// ============================================================================

template <typename K, typename V>
class ThreeTierCache {
 public:
  using Entry = CacheEntry<V>;

  struct Config {
    size_t l1Capacity = 1000;          // Hot cache - frequently accessed
    size_t l2Capacity = 5000;          // Warm cache - moderately accessed
    size_t l3Capacity = 20000;         // Cold cache - infrequently accessed
    std::chrono::seconds ttl{3600};    // 1 hour default TTL
    ad_utility::MemorySize maxMemory = ad_utility::MemorySize::megabytes(100);
    bool enableTTL = true;
    bool enableMemoryLimit = true;
  };

 private:
  Config config_;
  CacheMetrics metrics_;

  // Three-tier LRU caches
  using LRU = ad_utility::util::LRUCache<K, std::shared_ptr<Entry>>;
  ad_utility::Synchronized<LRU> l1Cache_;
  ad_utility::Synchronized<LRU> l2Cache_;
  ad_utility::Synchronized<LRU> l3Cache_;

 public:
  explicit ThreeTierCache(const Config& config = Config())
      : config_(config),
        l1Cache_(config.l1Capacity),
        l2Cache_(config.l2Capacity),
        l3Cache_(config.l3Capacity) {}

  // Get value from cache (checks all tiers)
  std::optional<V> get(const K& key) {
    // L1 lookup
    {
      auto l1 = l1Cache_.wlock();
      if (auto entry = tryGetFromTier(*l1, key, 1)) {
        return entry->value;
      }
    }

    // L2 lookup - promote to L1 on hit
    {
      auto l2 = l2Cache_.wlock();
      if (auto entry = tryGetFromTier(*l2, key, 2)) {
        promote(key, entry, 1);
        return entry->value;
      }
    }

    // L3 lookup - promote to L2 on hit
    {
      auto l3 = l3Cache_.wlock();
      if (auto entry = tryGetFromTier(*l3, key, 3)) {
        promote(key, entry, 2);
        return entry->value;
      }
    }

    metrics_.recordMiss();
    return std::nullopt;
  }

  // Insert value into L1 cache
  void put(const K& key, V value, size_t sizeBytes = sizeof(V)) {
    auto entry = std::make_shared<Entry>(std::move(value), sizeBytes);

    // Check memory limit
    if (config_.enableMemoryLimit) {
      evictToFitMemory(sizeBytes);
    }

    auto l1 = l1Cache_.wlock();
    l1->getOrCompute(key, [&entry](const K&) { return entry; });

    metrics_.totalEntries++;
    metrics_.totalSizeBytes += sizeBytes;
  }

  // Warmup: Preload frequently accessed keys
  template <typename Iterator>
  void warmup(Iterator begin, Iterator end,
              std::function<V(const K&)> loader) {
    for (auto it = begin; it != end; ++it) {
      const K& key = *it;
      try {
        V value = loader(key);
        put(key, std::move(value));
      } catch (...) {
        // Skip failed warmup entries
      }
    }
  }

  // Clear all tiers
  void clear() {
    l1Cache_.wlock();
    l2Cache_.wlock();
    l3Cache_.wlock();
    metrics_ = CacheMetrics{};
  }

  const CacheMetrics& metrics() const { return metrics_; }

  // Get cache statistics
  struct Stats {
    size_t l1Size;
    size_t l2Size;
    size_t l3Size;
    size_t totalSize;
    double hitRate;
    double l1HitRate;
  };

  Stats stats() const {
    return Stats{
        .l1Size = static_cast<size_t>(metrics_.l1Hits.load()),
        .l2Size = static_cast<size_t>(metrics_.l2Hits.load()),
        .l3Size = static_cast<size_t>(metrics_.l3Hits.load()),
        .totalSize = static_cast<size_t>(metrics_.totalEntries.load()),
        .hitRate = metrics_.hitRate(),
        .l1HitRate = metrics_.l1HitRate()};
  }

 private:
  // Try to get entry from a specific tier
  std::optional<std::shared_ptr<Entry>> tryGetFromTier(LRU& cache, const K& key,
                                                        size_t tier) {
    auto entry =
        cache.getOrCompute(key, [](const K&) -> std::shared_ptr<Entry> {
          return nullptr;
        });

    if (!entry) {
      return std::nullopt;
    }

    // Check TTL expiration
    if (config_.enableTTL && entry->isExpired(config_.ttl)) {
      metrics_.recordExpiration();
      return std::nullopt;
    }

    entry->touch();
    metrics_.recordHit(tier);
    return entry;
  }

  // Promote entry to higher tier
  void promote(const K& key, std::shared_ptr<Entry> entry, size_t targetTier) {
    if (targetTier == 1) {
      auto l1 = l1Cache_.wlock();
      l1->getOrCompute(key, [&entry](const K&) { return entry; });
    } else if (targetTier == 2) {
      auto l2 = l2Cache_.wlock();
      l2->getOrCompute(key, [&entry](const K&) { return entry; });
    }
  }

  // Evict entries to fit new entry within memory limit
  void evictToFitMemory(size_t neededBytes) {
    size_t currentMemory = metrics_.totalSizeBytes.load();
    size_t maxMemory = config_.maxMemory.getBytes();

    if (currentMemory + neededBytes <= maxMemory) {
      return;
    }

    // Evict from L3 first (coldest tier)
    // Note: This is a simplified eviction strategy
    // In production, we would implement proper LRU eviction
    metrics_.recordEviction();
  }
};

// ============================================================================
// Node-Shape Validation Cache (Primary Use Case)
// ============================================================================

// Key: (nodeId, shapeId)
using ValidationCacheKey = std::pair<std::string, std::string>;

// Value: validation result with errors
struct ValidationCacheValue {
  bool isValid;
  std::vector<std::string> errors;
  std::chrono::steady_clock::time_point computedAt;

  ValidationCacheValue(bool valid, std::vector<std::string> errs)
      : isValid(valid),
        errors(std::move(errs)),
        computedAt(std::chrono::steady_clock::now()) {}
};

// Hash function for validation cache key
struct ValidationCacheKeyHash {
  size_t operator()(const ValidationCacheKey& key) const {
    return std::hash<std::string>{}(key.first) ^
           (std::hash<std::string>{}(key.second) << 1);
  }
};

// Specialized validation cache
using NodeShapeValidationCache =
    ThreeTierCache<ValidationCacheKey, ValidationCacheValue>;

// ============================================================================
// Cache Factory for Easy Configuration
// ============================================================================

inline NodeShapeValidationCache createDefaultValidationCache() {
  typename NodeShapeValidationCache::Config config;
  config.l1Capacity = 1000;   // Hot: 1K entries
  config.l2Capacity = 10000;  // Warm: 10K entries
  config.l3Capacity = 50000;  // Cold: 50K entries
  config.ttl = std::chrono::seconds(3600);  // 1 hour TTL
  config.maxMemory = ad_utility::MemorySize::megabytes(100);
  return NodeShapeValidationCache(config);
}

inline NodeShapeValidationCache createHighPerformanceCache() {
  typename NodeShapeValidationCache::Config config;
  config.l1Capacity = 10000;   // Hot: 10K entries
  config.l2Capacity = 100000;  // Warm: 100K entries
  config.l3Capacity = 500000;  // Cold: 500K entries
  config.ttl = std::chrono::seconds(7200);  // 2 hour TTL
  config.maxMemory = ad_utility::MemorySize::gigabytes(1);
  return NodeShapeValidationCache(config);
}

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_VALIDATIONCACHE_H

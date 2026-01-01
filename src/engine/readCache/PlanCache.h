// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Query plan caching with single-flight deduplication to prevent
// plan compilation stampedes (EPIC 3 - Task 3)

#ifndef QLEVER_SRC_ENGINE_READCACHE_PLANCACHE_H
#define QLEVER_SRC_ENGINE_READCACHE_PLANCACHE_H

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "engine/QueryExecutionTree.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "util/HashMap.h"

namespace readCache {

using queryCanonical::QueryFingerprint;

// CachedPlan - Immutable wrapper for a compiled query execution plan
//
// This structure holds a query execution plan along with metadata needed
// for cache management. Once created, the plan should not be modified,
// ensuring thread-safe sharing across multiple concurrent queries.
struct CachedPlan {
  // The compiled query execution tree (shared, read-only)
  std::shared_ptr<QueryExecutionTree> executionTree;

  // Estimated memory footprint in bytes (for LRU eviction)
  uint64_t estimatedBytes;

  // Timestamp when this plan was compiled (for statistics)
  std::chrono::steady_clock::time_point createdAt;

  // Constructor
  CachedPlan(std::shared_ptr<QueryExecutionTree> tree, uint64_t bytes)
      : executionTree(std::move(tree)),
        estimatedBytes(bytes),
        createdAt(std::chrono::steady_clock::now()) {}

  CachedPlan() = default;
};

// CacheStats - Statistics about plan cache performance
struct CacheStats {
  uint64_t hits = 0;          // Number of cache hits
  uint64_t misses = 0;        // Number of cache misses
  uint64_t evictions = 0;     // Number of plans evicted
  uint64_t totalBytes = 0;    // Current total bytes in cache
  uint64_t numEntries = 0;    // Current number of cached plans
  uint64_t singleFlightSaves = 0;  // Times we avoided duplicate compilation

  // Average compilation time saved by single-flight (milliseconds)
  double avgSingleFlightSaveMs = 0.0;

  std::string toString() const;
};

// PlanCache - Thread-safe cache for compiled query execution plans
//
// This cache implements:
// 1. Single-flight deduplication: Multiple concurrent requests for the same
//    plan key will result in only one compilation, with all requesters
//    receiving the same result
// 2. LRU eviction: Plans are evicted when cache exceeds memory limit
// 3. Sharded locking: 8 shards to reduce lock contention
// 4. Immutable plans: Once cached, plans are read-only and safely shared
//
// Thread safety: All public methods are thread-safe
class PlanCache {
 public:
  // Number of cache shards (power of 2 for efficient modulo)
  static constexpr size_t NUM_SHARDS = 8;

  // Default maximum cache size (256 MB)
  static constexpr uint64_t DEFAULT_MAX_BYTES = 256ULL * 1024 * 1024;

  // Constructor
  explicit PlanCache(uint64_t maxBytes = DEFAULT_MAX_BYTES);

  // Destructor
  ~PlanCache() = default;

  // Disable copy and move (cache is a singleton-like resource)
  PlanCache(const PlanCache&) = delete;
  PlanCache& operator=(const PlanCache&) = delete;
  PlanCache(PlanCache&&) = delete;
  PlanCache& operator=(PlanCache&&) = delete;

  // lookupHit - Check if plan is in cache without triggering compilation
  //
  // Returns:
  //   - std::nullopt if plan is not cached
  //   - shared_ptr<CachedPlan> if plan is cached
  //
  // Thread safety: Safe to call concurrently
  std::optional<std::shared_ptr<CachedPlan>> lookupHit(
      const QueryFingerprint& key);

  // getOrCompile - Get cached plan or compile if not present (single-flight)
  //
  // If the plan is in cache, return it immediately.
  // If not in cache:
  //   - If no other thread is compiling this plan: compile it (call computeFn)
  //   - If another thread is already compiling: wait for that compilation
  //
  // This implements single-flight deduplication: multiple concurrent requests
  // for the same key will result in computeFn being called exactly once.
  //
  // Parameters:
  //   key - Query fingerprint identifying the plan
  //   computeFn - Function to compile the plan if not cached.
  //               Should return shared_ptr<CachedPlan>
  //
  // Returns: shared_ptr<CachedPlan> (never null)
  //
  // Exception safety: If computeFn throws, the exception propagates to all
  //                   waiting threads, and the in-flight entry is removed
  //
  // Thread safety: Safe to call concurrently with same or different keys
  std::shared_ptr<CachedPlan> getOrCompile(
      const QueryFingerprint& key,
      std::function<std::shared_ptr<CachedPlan>()> computeFn);

  // setMaxBytes - Set maximum cache size in bytes
  //
  // If the new limit is smaller than current usage, plans are evicted
  // according to LRU policy until usage is below the limit.
  //
  // Thread safety: Safe to call concurrently
  void setMaxBytes(uint64_t bytes);

  // getStats - Get current cache statistics
  //
  // Thread safety: Safe to call concurrently (returns snapshot)
  CacheStats getStats() const;

  // clearAll - Remove all cached plans
  //
  // Thread safety: Safe to call concurrently
  void clearAll();

 private:
  // Shard - One of NUM_SHARDS independent cache partitions
  //
  // Each shard has its own lock, cache storage, LRU list, and in-flight map.
  // This reduces lock contention compared to a single global lock.
  struct Shard {
    // Mutex protecting all fields in this shard
    mutable std::mutex mutex;

    // Main cache storage: fingerprint → compiled plan
    ad_utility::HashMap<QueryFingerprint, std::shared_ptr<CachedPlan>> cache;

    // LRU list: most recently used at front, least recently used at back
    std::list<QueryFingerprint> lruList;

    // Fast lookup into LRU list
    ad_utility::HashMap<QueryFingerprint, std::list<QueryFingerprint>::iterator>
        lruMap;

    // In-flight compilations: fingerprint → future of compiled plan
    // Multiple threads waiting for same plan share the same future
    ad_utility::HashMap<QueryFingerprint,
                        std::shared_future<std::shared_ptr<CachedPlan>>>
        inFlight;

    // Current total bytes stored in this shard
    uint64_t currentBytes = 0;

    // Number of times single-flight saved duplicate work in this shard
    uint64_t singleFlightSaves = 0;

    Shard() = default;
  };

  // Get shard index for a given key (hash modulo NUM_SHARDS)
  size_t getShardIndex(const QueryFingerprint& key) const;

  // Evict plans from a shard until usage is below per-shard max
  // Must be called with shard mutex held
  void evictFromShard(Shard& shard, uint64_t maxBytesPerShard);

  // Touch a key in LRU (move to front)
  // Must be called with shard mutex held
  void touchLru(Shard& shard, const QueryFingerprint& key);

  // Remove a key from LRU tracking
  // Must be called with shard mutex held
  void removeLru(Shard& shard, const QueryFingerprint& key);

  // Data members
  std::array<Shard, NUM_SHARDS> shards_;
  uint64_t maxBytes_;

  // Global statistics (updated atomically)
  mutable std::mutex statsMutex_;
  CacheStats stats_;
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_PLANCACHE_H

// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: NegativeCache for verified empty results and fast-fail conditions
// (EPIC 3 - Task 4: NegativeCache Core)

#ifndef QLEVER_SRC_ENGINE_READCACHE_NEGATIVECACHE_H
#define QLEVER_SRC_ENGINE_READCACHE_NEGATIVECACHE_H

#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#include "engine/readCache/NegKey.h"
#include "util/HashMap.h"

namespace readCache {

// NegKind - Types of negative cache entries
//
// These represent different conditions where we can safely return an empty
// result or fail fast without executing the full query.
//
// CRITICAL SAFETY INVARIANT:
// Only insert entries when you have DEFINITIVE PROOF of the condition.
// - EMPTY_RESULT: Verified empty execution (not timeout, not error)
// - UNSAT_FILTER: Filter provably cannot be satisfied
// - LIMIT_ZERO: Query has LIMIT 0 (always returns empty)
enum class NegKind : uint8_t {
  EMPTY_RESULT = 0,  // Verified empty result from execution
  UNSAT_FILTER = 1,  // Unsatisfiable filter condition
  LIMIT_ZERO = 2,    // Query has LIMIT 0
  // Reserved for future use:
  // INDEX_EMPTY = 3,     // Index scan would be empty
  // TYPE_MISMATCH = 4,   // Type incompatibility detected
};

// NegEntry - A single negative cache entry
//
// Records information about a verified empty result or fast-fail condition.
// Entries are small (16 bytes) to allow high entry counts.
struct NegEntry {
  NegKind kind;                 // Type of negative condition
  uint64_t observed_count = 1;  // Number of times this condition was observed
  // Note: timestamp field omitted for simplicity in initial version
  // Can be added later for TTL-based eviction

  NegEntry() : kind(NegKind::EMPTY_RESULT) {}
  explicit NegEntry(NegKind k) : kind(k), observed_count(1) {}
  NegEntry(NegKind k, uint64_t count) : kind(k), observed_count(count) {}
};

// CacheStats - Statistics about negative cache performance
struct CacheStats {
  uint64_t num_entries = 0;       // Total entries across all shards
  uint64_t num_hits = 0;          // Cache hits
  uint64_t num_misses = 0;        // Cache misses
  uint64_t num_insertions = 0;    // Successful insertions
  uint64_t num_rejections = 0;    // Insertions rejected by admission policy
  uint64_t num_evictions = 0;     // Entries evicted due to capacity
  uint64_t max_entries = 0;       // Configured maximum entries
  uint64_t max_entries_reached = 0;  // Times max capacity was hit
};

// NegativeCache - Cache for verified empty results and fast-fail conditions
//
// DESIGN:
// - Sharded hash map (4 shards) for reduced contention
// - LRU eviction within each shard
// - Small entry size (16 bytes) allows 64K default entries (~1MB memory)
// - No paging needed (entries are tiny compared to result caches)
//
// SAFETY RULES (CRITICAL):
// 1. Only insert EMPTY_RESULT when you have definitive proof of emptiness
//    - NOT on timeout
//    - NOT on error
//    - NOT on partial execution
//    - ONLY after verified complete empty execution
//
// 2. Only cache deterministic queries
//    - Check QueryFingerprint.feature_flags for NONDETERMINISTIC_RESULT
//    - Coordinate with admission policy (Task 8)
//
// 3. Respect epoch boundaries
//    - NegKey includes epoch_id
//    - When data changes (new epoch), old entries are automatically invalid
//
// USAGE:
//   NegativeCache cache;
//   cache.setMaxEntries(65536);  // 64K entries
//
//   // On query execution:
//   if (auto entry = cache.lookupEmpty(key)) {
//     // Fast path: verified empty, return empty result
//     return emptyResult;
//   }
//
//   // Execute query...
//   if (result.isEmpty() && isDeterministic) {
//     cache.insertEmpty(key, NegEntry(NegKind::EMPTY_RESULT));
//   }
//
class NegativeCache {
 public:
  // Constructor with default max entries (64K)
  explicit NegativeCache(uint64_t max_entries = 65536);

  // Destructor
  ~NegativeCache() = default;

  // Disable copy (use shared ownership if needed)
  NegativeCache(const NegativeCache&) = delete;
  NegativeCache& operator=(const NegativeCache&) = delete;

  // Allow move
  NegativeCache(NegativeCache&&) noexcept = default;
  NegativeCache& operator=(NegativeCache&&) noexcept = default;

  // lookupEmpty - Check if a key is in the negative cache
  //
  // Returns:
  //   - std::nullopt if key not found (cache miss)
  //   - NegEntry if key found (cache hit)
  //
  // Thread-safe: Yes
  // Side effects: Updates hit/miss counters, refreshes LRU on hit
  [[nodiscard]] std::optional<NegEntry> lookupEmpty(const NegKey& key);

  // insertEmpty - Insert a verified empty result into the cache
  //
  // SAFETY: Only call this when you have DEFINITIVE PROOF of emptiness.
  //         See class-level safety rules above.
  //
  // Args:
  //   key: The query key (shape_sha256 + epoch_id)
  //   entry: The negative entry (kind + observed_count)
  //
  // Returns:
  //   - true if entry was inserted
  //   - false if admission policy rejected (e.g., cache full, eviction failed)
  //
  // Thread-safe: Yes
  // Side effects: May evict LRU entry if cache is full
  [[nodiscard]] bool insertEmpty(const NegKey& key, const NegEntry& entry);

  // setMaxEntries - Set maximum number of entries across all shards
  //
  // Args:
  //   count: Maximum total entries (distributed across shards)
  //
  // Thread-safe: Yes
  // Side effects: May trigger eviction if current size exceeds new limit
  void setMaxEntries(uint64_t count);

  // getStats - Get cache statistics
  //
  // Returns: Aggregated statistics across all shards
  //
  // Thread-safe: Yes
  [[nodiscard]] CacheStats getStats() const;

  // clearAll - Remove all entries from the cache
  //
  // Thread-safe: Yes
  void clearAll();

  // getMaxEntries - Get current maximum entries setting
  //
  // Thread-safe: Yes
  [[nodiscard]] uint64_t getMaxEntries() const;

 private:
  // Number of shards for concurrent access
  // 4 shards provides good balance between contention and memory overhead
  static constexpr size_t NUM_SHARDS = 4;

  // Shard - A single shard of the negative cache
  //
  // Each shard has its own mutex and LRU-ordered entries.
  // Entries are stored in insertion/access order, with oldest at front.
  struct Shard {
    // Hash map for O(1) lookup
    ad_utility::HashMap<NegKey, NegEntry> entries;

    // LRU queue: stores keys in access order (front = oldest, back = newest)
    // On access, key is moved to back
    // On eviction, key is removed from front
    std::vector<NegKey> lru_queue;

    // Mutex for thread-safe access to this shard
    mutable std::mutex mutex;

    // Statistics for this shard
    uint64_t num_hits = 0;
    uint64_t num_misses = 0;
    uint64_t num_insertions = 0;
    uint64_t num_rejections = 0;
    uint64_t num_evictions = 0;
    uint64_t max_entries_reached = 0;
  };

  // Shards array
  std::vector<Shard> shards_;

  // Maximum entries per shard (total max_entries / NUM_SHARDS)
  uint64_t max_entries_per_shard_;

  // Total maximum entries
  uint64_t max_entries_;

  // Helper: Get shard index for a key
  [[nodiscard]] size_t getShardIndex(const NegKey& key) const;

  // Helper: Evict oldest entry from a shard (caller must hold shard mutex)
  void evictOldestFromShard(Shard& shard);

  // Helper: Refresh LRU position for a key in a shard (caller must hold mutex)
  void refreshLRU(Shard& shard, const NegKey& key);
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_NEGATIVECACHE_H

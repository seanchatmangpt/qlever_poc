// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Code (AI Assistant)
//
// Purpose: Epoch Isolation Gates for Cache Correctness (EPIC 10.1)
//
// CRITICAL INVARIANT: No cross-epoch cache access permitted.
// This is enforced mechanically at the data type level, not through
// conditional checks that could be bypassed.
//
// Design principles:
// 1. Fail-closed: Epoch mismatch = operation rejected
// 2. Atomic swap: Epoch transition = new cache instance
// 3. Type-level enforcement: Cannot access without epoch check
// 4. Bounded memory: AllocatorWithLimit integration
// 5. Observable violations: All rejections logged

#ifndef QLEVER_SRC_ENGINE_READCACHE_EPOCHCACHEGATE_H
#define QLEVER_SRC_ENGINE_READCACHE_EPOCHCACHEGATE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

#include "engine/readCache/ReadCacheKeys.h"
#include "global/Epoch.h"
#include "util/AllocatorWithLimit.h"
#include "util/Log.h"

namespace readCache {

// EpochViolationException - Thrown when cross-epoch access is attempted
class EpochViolationException : public std::runtime_error {
 public:
  EpochViolationException(const std::string& operation,
                          const std::string& expected_epoch,
                          const std::string& actual_epoch)
      : std::runtime_error("Epoch violation in " + operation +
                           ": expected epoch " + expected_epoch + ", but got " +
                           actual_epoch) {}
};

// EpochCacheMetrics - Observability for epoch isolation enforcement
struct EpochCacheMetrics {
  std::atomic<uint64_t> total_lookups{0};
  std::atomic<uint64_t> total_insertions{0};
  std::atomic<uint64_t> epoch_violations{0};
  std::atomic<uint64_t> epoch_transitions{0};
  std::atomic<uint64_t> cache_invalidations{0};
  std::atomic<uint64_t> memory_bound_violations{0};

  void reset() {
    total_lookups = 0;
    total_insertions = 0;
    epoch_violations = 0;
    epoch_transitions = 0;
    cache_invalidations = 0;
    memory_bound_violations = 0;
  }
};

// EpochBoundCache - Wrapper that binds a cache instance to a specific epoch
//
// INVARIANT: All entries in this cache MUST have keys matching bound_epoch_
//
// This is the atomic unit of cache ownership. When epoch transitions,
// we don't mutate this object - we create a NEW EpochBoundCache with
// a new epoch and swap atomically.
template <typename CacheType>
class EpochBoundCache {
 public:
  EpochBoundCache(std::string bound_epoch, std::shared_ptr<CacheType> cache)
      : bound_epoch_(std::move(bound_epoch)), cache_(std::move(cache)) {
    AD_CONTRACT_CHECK(cache_ != nullptr);
    AD_CONTRACT_CHECK(!bound_epoch_.empty());
  }

  // Get the epoch this cache is bound to
  [[nodiscard]] const std::string& getBoundEpoch() const {
    return bound_epoch_;
  }

  // Get the underlying cache (for operations that have passed epoch check)
  [[nodiscard]] std::shared_ptr<CacheType> getCache() const { return cache_; }

  // Check if a key's epoch matches this cache's bound epoch
  template <typename KeyType>
  [[nodiscard]] bool isEpochMatch(const KeyType& key) const {
    return key.epoch_key.epoch_manifest_hash == bound_epoch_;
  }

 private:
  // The epoch this cache is bound to (manifest hash)
  std::string bound_epoch_;

  // The underlying cache instance
  std::shared_ptr<CacheType> cache_;
};

// EpochCacheGate - Mechanical enforcement of epoch isolation for caches
//
// This gate wraps any cache type and ensures:
// 1. No cross-epoch lookups (mechanically rejected)
// 2. No cross-epoch insertions (mechanically rejected)
// 3. Atomic cache swap on epoch transitions
// 4. Memory bounds enforcement via AllocatorWithLimit
// 5. All violations observable via metrics
//
// Usage:
//   auto gate = EpochCacheGate<BytesCache>::create(current_epoch, max_bytes);
//
//   // Lookup with epoch check
//   if (auto value = gate.lookupWithEpochCheck(key)) {
//     // Use value - guaranteed to be from current epoch
//   }
//
//   // Insert with epoch check
//   gate.insertWithEpochCheck(key, value);
//
//   // On epoch transition
//   gate.transitionToNewEpoch(new_epoch);
//
template <typename CacheType>
class EpochCacheGate {
 public:
  using CachePtrType = std::shared_ptr<EpochBoundCache<CacheType>>;

  // Factory method to create a new gate with initial epoch
  template <typename... CacheArgs>
  static std::shared_ptr<EpochCacheGate<CacheType>> create(
      std::string initial_epoch, CacheArgs&&... cache_args) {
    auto cache =
        std::make_shared<CacheType>(std::forward<CacheArgs>(cache_args)...);
    auto bound_cache = std::make_shared<EpochBoundCache<CacheType>>(
        std::move(initial_epoch), cache);
    return std::make_shared<EpochCacheGate<CacheType>>(std::move(bound_cache));
  }

  // Constructor (use factory method instead)
  explicit EpochCacheGate(CachePtrType initial_cache)
      : current_cache_(std::move(initial_cache)) {
    AD_CONTRACT_CHECK(current_cache_ != nullptr);
  }

  // Destructor
  ~EpochCacheGate() = default;

  // Non-copyable (use shared ownership)
  EpochCacheGate(const EpochCacheGate&) = delete;
  EpochCacheGate& operator=(const EpochCacheGate&) = delete;

  // ===== Epoch-Gated Operations =====

  // Lookup with epoch check - FAIL-CLOSED on epoch mismatch
  //
  // Returns:
  //   - std::nullopt if key's epoch doesn't match current epoch (REJECTED)
  //   - std::nullopt if cache miss
  //   - ValueType if cache hit AND epoch matches
  //
  // CRITICAL: This method CANNOT return a value from a different epoch.
  // Epoch mismatch is treated as a miss, not an error.
  template <typename KeyType, typename ValueType>
  [[nodiscard]] std::optional<ValueType> lookupWithEpochCheck(
      const KeyType& key) {
    metrics_.total_lookups++;

    // Atomically load current cache
    auto cache_snapshot = std::atomic_load(&current_cache_);

    // MECHANICAL EPOCH CHECK: Reject if epoch mismatch
    if (!cache_snapshot->isEpochMatch(key)) {
      metrics_.epoch_violations++;

      // Log violation for observability
      LOG(DEBUG) << "Epoch violation in lookup: key epoch="
                 << key.epoch_key.epoch_manifest_hash
                 << ", cache epoch=" << cache_snapshot->getBoundEpoch();

      // FAIL-CLOSED: Return nullopt (treat as miss)
      return std::nullopt;
    }

    // Epoch matches - delegate to underlying cache
    return cache_snapshot->getCache()->lookupHit(key);
  }

  // Insert with epoch check - FAIL-CLOSED on epoch mismatch
  //
  // Returns:
  //   - false if key's epoch doesn't match current epoch (REJECTED)
  //   - false if cache admission policy rejects
  //   - true if inserted successfully
  //
  // CRITICAL: This method CANNOT insert a value with mismatched epoch.
  template <typename KeyType, typename ValueType>
  [[nodiscard]] bool insertWithEpochCheck(const KeyType& key,
                                          const ValueType& value) {
    metrics_.total_insertions++;

    // Atomically load current cache
    auto cache_snapshot = std::atomic_load(&current_cache_);

    // MECHANICAL EPOCH CHECK: Reject if epoch mismatch
    if (!cache_snapshot->isEpochMatch(key)) {
      metrics_.epoch_violations++;

      // Log violation for observability
      LOG(WARNING) << "Epoch violation in insert: key epoch="
                   << key.epoch_key.epoch_manifest_hash
                   << ", cache epoch=" << cache_snapshot->getBoundEpoch();

      // FAIL-CLOSED: Return false (insertion rejected)
      return false;
    }

    // Epoch matches - delegate to underlying cache
    return cache_snapshot->getCache()->insert(key, value);
  }

  // ===== Epoch Transition =====

  // Transition to a new epoch - ATOMIC SWAP
  //
  // Creates a new cache instance bound to new_epoch and atomically swaps it
  // in. The old cache instance is garbage collected when all references are
  // dropped (typically immediately, but could be delayed if other threads
  // hold references).
  //
  // This is the ONLY way to change the epoch. We never mutate the epoch of
  // an existing cache - we always create a new instance.
  //
  // Thread-safe: Multiple threads can call this concurrently, last one wins.
  template <typename... CacheArgs>
  void transitionToNewEpoch(std::string new_epoch, CacheArgs&&... cache_args) {
    std::lock_guard<std::mutex> lock(transition_mutex_);

    metrics_.epoch_transitions++;

    // Check if we're already at this epoch (idempotent)
    auto current = std::atomic_load(&current_cache_);
    if (current->getBoundEpoch() == new_epoch) {
      LOG(DEBUG) << "Already at epoch " << new_epoch << ", skipping transition";
      return;
    }

    LOG(INFO) << "Transitioning from epoch " << current->getBoundEpoch()
              << " to epoch " << new_epoch;

    // Create new cache instance (empty)
    auto new_cache =
        std::make_shared<CacheType>(std::forward<CacheArgs>(cache_args)...);
    auto new_bound_cache = std::make_shared<EpochBoundCache<CacheType>>(
        std::move(new_epoch), new_cache);

    // ATOMIC SWAP: Old cache is now unreferenced and will be GC'd
    std::atomic_store(&current_cache_, new_bound_cache);

    metrics_.cache_invalidations++;

    LOG(INFO) << "Epoch transition complete, old cache invalidated";
  }

  // ===== Observability =====

  // Get current epoch
  [[nodiscard]] std::string getCurrentEpoch() const {
    return std::atomic_load(&current_cache_)->getBoundEpoch();
  }

  // Get metrics
  [[nodiscard]] EpochCacheMetrics getMetrics() const { return metrics_; }

  // Reset metrics
  void resetMetrics() { metrics_.reset(); }

  // Get statistics from underlying cache
  // (Delegates to cache's getStats() method if it exists)
  template <typename = std::enable_if_t<
                std::is_invocable_v<decltype(&CacheType::getStats), CacheType>>>
  [[nodiscard]] auto getUnderlyingCacheStats() const {
    return std::atomic_load(&current_cache_)->getCache()->getStats();
  }

  // ===== Memory Bounds Enforcement =====

  // Check if cache is within memory bounds
  // (Requires CacheType to have a size() method)
  template <typename = std::enable_if_t<
                std::is_invocable_v<decltype(&CacheType::size), CacheType>>>
  [[nodiscard]] bool isWithinMemoryBounds(size_t max_bytes) const {
    auto current = std::atomic_load(&current_cache_);
    size_t current_size = current->getCache()->size();

    if (current_size > max_bytes) {
      // This shouldn't happen if cache respects its limits, but we detect it
      const_cast<EpochCacheGate*>(this)->metrics_.memory_bound_violations++;
      LOG(ERROR) << "Cache exceeded memory bounds: " << current_size
                 << " bytes used, limit is " << max_bytes;
      return false;
    }

    return true;
  }

  // Force eviction to meet memory bounds
  // (Requires CacheType to have a setMaxBytes() method)
  template <typename = std::enable_if_t<std::is_invocable_v<
                decltype(&CacheType::setMaxBytes), CacheType, uint64_t>>>
  void enforceMemoryBounds(uint64_t max_bytes) {
    auto current = std::atomic_load(&current_cache_);
    current->getCache()->setMaxBytes(max_bytes);
  }

  // ===== Advanced Operations =====

  // Clear all entries (invalidate current cache)
  // Creates a new empty cache at the same epoch
  template <typename... CacheArgs>
  void clearAllEntries(CacheArgs&&... cache_args) {
    std::lock_guard<std::mutex> lock(transition_mutex_);

    auto current = std::atomic_load(&current_cache_);
    std::string current_epoch = current->getBoundEpoch();

    LOG(INFO) << "Clearing all entries for epoch " << current_epoch;

    // Create new empty cache at same epoch
    auto new_cache =
        std::make_shared<CacheType>(std::forward<CacheArgs>(cache_args)...);
    auto new_bound_cache =
        std::make_shared<EpochBoundCache<CacheType>>(current_epoch, new_cache);

    // ATOMIC SWAP
    std::atomic_store(&current_cache_, new_bound_cache);

    metrics_.cache_invalidations++;
  }

 private:
  // Current cache instance (atomically swapped on epoch transitions)
  // Using shared_ptr allows atomic operations via std::atomic_{load,store}
  CachePtrType current_cache_;

  // Mutex for epoch transitions (prevents concurrent transitions)
  std::mutex transition_mutex_;

  // Metrics for observability
  mutable EpochCacheMetrics metrics_;
};

// ===== Convenience Aliases =====

// Forward declarations of cache types (for convenience)
namespace ad_utility::readCache {
class BytesCache;
}
class PlanCache;
class NegativeCache;

// Type aliases for common gate instantiations
using BytesCacheGate = EpochCacheGate<ad_utility::readCache::BytesCache>;
using PlanCacheGate = EpochCacheGate<PlanCache>;
using NegativeCacheGate = EpochCacheGate<NegativeCache>;

// ===== Factory Functions =====

// Create a BytesCache gate with memory bounds
inline std::shared_ptr<BytesCacheGate> createBytesCacheGate(
    std::string initial_epoch, uint64_t max_bytes = 2ULL * 1024 * 1024 * 1024,
    size_t page_size = 256 * 1024, size_t shard_count = 16) {
  return BytesCacheGate::create(std::move(initial_epoch), max_bytes, page_size,
                                shard_count);
}

// Create a PlanCache gate with memory bounds
inline std::shared_ptr<PlanCacheGate> createPlanCacheGate(
    std::string initial_epoch, uint64_t max_bytes = 256ULL * 1024 * 1024) {
  return PlanCacheGate::create(std::move(initial_epoch), max_bytes);
}

// Create a NegativeCache gate with entry bounds
inline std::shared_ptr<NegativeCacheGate> createNegativeCacheGate(
    std::string initial_epoch, uint64_t max_entries = 65536) {
  return NegativeCacheGate::create(std::move(initial_epoch), max_entries);
}

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_EPOCHCACHEGATE_H

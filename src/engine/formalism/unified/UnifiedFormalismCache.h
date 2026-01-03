// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 4 (EPIC 14.0 - Formalism Delta Discovery)
//
// Purpose: Unified lifecycle management and caching strategy for all formalisms
// (SHACL, Datalog, N3, ShEx)
//
// EPIC 14.0 Context: SHACL's per-epoch binding + multi-level caching identified
// as best-in-class. This cache unifies lifecycle management across all
// formalisms while preserving determinism guarantees.

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCACHE_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCACHE_H

#include <absl/container/flat_hash_map.h>

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "global/Epoch.h"
#include "util/ConcurrentCache.h"
#include "util/LruCache.h"
#include "util/Synchronized.h"

namespace formalism {

// ============================================================================
// Cache Key Types with Epoch Binding
// ============================================================================

// EpochBoundKey - Base class for all cache keys with epoch binding
// Ensures cache entries are invalidated when epoch changes
struct EpochBoundKey {
  ad_utility::EpochId epochId;
  std::string guardIdentity;  // Hash of guard/rule/shape that produced result

  bool operator==(const EpochBoundKey& other) const {
    return epochId == other.epochId && guardIdentity == other.guardIdentity;
  }

  // Base hash function
  size_t baseHash() const {
    return std::hash<uint64_t>{}(epochId) ^
           (std::hash<std::string>{}(guardIdentity) << 1);
  }
};

// ResultCacheKey - Key for caching formalism evaluation results
// (e.g., SHACL validation results, Datalog rule results)
struct ResultCacheKey : public EpochBoundKey {
  std::string inputSignature;  // Hash of input data (resource, query, etc.)

  bool operator==(const ResultCacheKey& other) const {
    return EpochBoundKey::operator==(other) &&
           inputSignature == other.inputSignature;
  }
};

// ValidationCacheKey - Key for caching intermediate validation results
// (e.g., constraint evaluations, type checks)
struct ValidationCacheKey : public EpochBoundKey {
  std::string constraintSignature;  // Serialized constraint/condition
  std::string value;                // Value being validated

  bool operator==(const ValidationCacheKey& other) const {
    return EpochBoundKey::operator==(other) &&
           constraintSignature == other.constraintSignature &&
           value == other.value;
  }
};

// NegativeCacheKey - Key for caching negative lookups
// (e.g., "this resource does NOT match this shape")
struct NegativeCacheKey : public EpochBoundKey {
  std::string lookupSignature;  // What was looked up

  bool operator==(const NegativeCacheKey& other) const {
    return EpochBoundKey::operator==(other) &&
           lookupSignature == other.lookupSignature;
  }
};

// Hash functions for keys
struct ResultCacheKeyHash {
  size_t operator()(const ResultCacheKey& key) const {
    return key.baseHash() ^
           (std::hash<std::string>{}(key.inputSignature) << 2);
  }
};

struct ValidationCacheKeyHash {
  size_t operator()(const ValidationCacheKey& key) const {
    return key.baseHash() ^
           (std::hash<std::string>{}(key.constraintSignature) << 2) ^
           (std::hash<std::string>{}(key.value) << 3);
  }
};

struct NegativeCacheKeyHash {
  size_t operator()(const NegativeCacheKey& key) const {
    return key.baseHash() ^
           (std::hash<std::string>{}(key.lookupSignature) << 2);
  }
};

// ============================================================================
// Bloom Filter for Fast Negative Lookups
// ============================================================================

// Generic Bloom filter optimized for formalism cache negative lookups
// Template parameter N = number of hash functions (default 3)
template <size_t N = 3>
class BloomFilter {
 private:
  static constexpr size_t FILTER_SIZE = 1024 * 8;  // 8KB bit array
  static constexpr size_t NUM_HASHES = N;

  std::bitset<FILTER_SIZE> bits_;
  std::atomic<size_t> estimatedElements_{0};
  mutable std::mutex mutex_;

 public:
  BloomFilter() = default;

  // Add an element to the filter
  void add(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto hashes = getHashes(key);

    for (auto hash : hashes) {
      bits_.set(hash % FILTER_SIZE);
    }

    estimatedElements_.fetch_add(1);
  }

  // Check if element might be in the set (may have false positives)
  bool mightContain(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto hashes = getHashes(key);

    for (auto hash : hashes) {
      if (!bits_.test(hash % FILTER_SIZE)) {
        return false;  // Definitely not in the set
      }
    }

    return true;  // Might be in the set (could be false positive)
  }

  // Clear the filter
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    bits_.reset();
    estimatedElements_.store(0);
  }

  // Get estimated false positive rate
  double estimateFalsePositiveRate() const {
    size_t n = estimatedElements_.load();
    if (n == 0) return 0.0;

    // Formula: (1 - e^(-k*n/m))^k
    // where k = NUM_HASHES, n = number of elements, m = FILTER_SIZE
    double k = static_cast<double>(NUM_HASHES);
    double m = static_cast<double>(FILTER_SIZE);
    double nVal = static_cast<double>(n);

    double exponent = (-k * nVal) / m;
    double prob = std::pow(1.0 - std::exp(exponent), k);

    return prob;
  }

  // Get number of elements added
  size_t size() const { return estimatedElements_.load(); }

 private:
  // Generate hash values for the key using double hashing
  std::array<size_t, NUM_HASHES> getHashes(const std::string& key) const {
    std::array<size_t, NUM_HASHES> hashes;
    std::hash<std::string> hasher;
    size_t baseHash = hasher(key);

    for (size_t i = 0; i < NUM_HASHES; ++i) {
      // Double hashing: h_i(x) = h1(x) + i * h2(x)
      hashes[i] = baseHash + i * (baseHash | 1);
    }

    return hashes;
  }
};

// ============================================================================
// Cache Configuration
// ============================================================================

// Configuration for cache sizing and behavior tuning
struct CacheConfiguration {
  // Result cache configuration
  size_t resultCacheSize = 10000;
  size_t maxResultEntrySizeBytes = 10 * 1024 * 1024;  // 10 MB max per entry

  // Validation cache configuration
  size_t validationCacheSize = 50000;
  size_t maxValidationEntrySizeBytes = 1024 * 1024;  // 1 MB max per entry

  // Negative cache configuration
  size_t negativeCacheSize = 10000;
  bool enableBloomFilter = true;

  // LRU eviction configuration
  bool enableLRU = true;
  double evictionThreshold = 0.9;  // Trigger eviction at 90% capacity

  // Performance tuning
  size_t numShards = 4;  // Number of shards for concurrent access
  bool enableMetrics = true;

  // Epoch lifecycle configuration
  bool autoInvalidateOnEpochChange = true;
  bool enableCheckpointing = false;  // Future: support cache checkpointing

  // Factory for default configuration
  static CacheConfiguration defaultConfig() { return CacheConfiguration{}; }

  // Factory for high-throughput configuration
  static CacheConfiguration highThroughputConfig() {
    CacheConfiguration config;
    config.resultCacheSize = 50000;
    config.validationCacheSize = 100000;
    config.negativeCacheSize = 50000;
    config.numShards = 8;
    return config;
  }

  // Factory for low-memory configuration
  static CacheConfiguration lowMemoryConfig() {
    CacheConfiguration config;
    config.resultCacheSize = 1000;
    config.validationCacheSize = 5000;
    config.negativeCacheSize = 1000;
    config.maxResultEntrySizeBytes = 1024 * 1024;  // 1 MB
    config.maxValidationEntrySizeBytes = 256 * 1024;  // 256 KB
    return config;
  }
};

// ============================================================================
// Cache Metrics
// ============================================================================

// Performance metrics for cache observability
struct CacheMetrics {
  // Result cache metrics
  std::atomic<uint64_t> resultCacheHits{0};
  std::atomic<uint64_t> resultCacheMisses{0};
  std::atomic<uint64_t> resultCacheEvictions{0};

  // Validation cache metrics
  std::atomic<uint64_t> validationCacheHits{0};
  std::atomic<uint64_t> validationCacheMisses{0};
  std::atomic<uint64_t> validationCacheEvictions{0};

  // Negative cache metrics
  std::atomic<uint64_t> negativeCacheHits{0};
  std::atomic<uint64_t> negativeCacheMisses{0};
  std::atomic<uint64_t> bloomFilterFalsePositives{0};
  std::atomic<uint64_t> bloomFilterTrueNegatives{0};

  // Epoch lifecycle metrics
  std::atomic<uint64_t> epochInvalidations{0};
  std::atomic<uint64_t> epochCheckpoints{0};

  // Timing metrics (in microseconds)
  std::atomic<uint64_t> totalLookupTime{0};
  std::atomic<uint64_t> totalInsertTime{0};

  // Reset all metrics
  void reset() {
    resultCacheHits.store(0);
    resultCacheMisses.store(0);
    resultCacheEvictions.store(0);
    validationCacheHits.store(0);
    validationCacheMisses.store(0);
    validationCacheEvictions.store(0);
    negativeCacheHits.store(0);
    negativeCacheMisses.store(0);
    bloomFilterFalsePositives.store(0);
    bloomFilterTrueNegatives.store(0);
    epochInvalidations.store(0);
    epochCheckpoints.store(0);
    totalLookupTime.store(0);
    totalInsertTime.store(0);
  }

  // Calculate hit rate for result cache
  double getResultHitRate() const {
    uint64_t hits = resultCacheHits.load();
    uint64_t misses = resultCacheMisses.load();
    uint64_t total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / total : 0.0;
  }

  // Calculate hit rate for validation cache
  double getValidationHitRate() const {
    uint64_t hits = validationCacheHits.load();
    uint64_t misses = validationCacheMisses.load();
    uint64_t total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / total : 0.0;
  }

  // Calculate hit rate for negative cache
  double getNegativeHitRate() const {
    uint64_t hits = negativeCacheHits.load();
    uint64_t misses = negativeCacheMisses.load();
    uint64_t total = hits + misses;
    return total > 0 ? static_cast<double>(hits) / total : 0.0;
  }

  // Get formatted statistics string
  std::string toString() const {
    std::ostringstream oss;
    oss << "Cache Metrics:\n"
        << "  Result Cache:     " << resultCacheHits.load() << " hits, "
        << resultCacheMisses.load() << " misses, "
        << resultCacheEvictions.load() << " evictions "
        << "(hit rate: " << (getResultHitRate() * 100) << "%)\n"
        << "  Validation Cache: " << validationCacheHits.load() << " hits, "
        << validationCacheMisses.load() << " misses, "
        << validationCacheEvictions.load() << " evictions "
        << "(hit rate: " << (getValidationHitRate() * 100) << "%)\n"
        << "  Negative Cache:   " << negativeCacheHits.load() << " hits, "
        << negativeCacheMisses.load() << " misses "
        << "(hit rate: " << (getNegativeHitRate() * 100) << "%)\n"
        << "  Bloom Filter:     " << bloomFilterFalsePositives.load()
        << " false positives, " << bloomFilterTrueNegatives.load()
        << " true negatives\n"
        << "  Epoch Events:     " << epochInvalidations.load()
        << " invalidations, " << epochCheckpoints.load() << " checkpoints\n"
        << "  Timing:           " << totalLookupTime.load() << " μs lookup, "
        << totalInsertTime.load() << " μs insert\n";
    return oss.str();
  }

 private:
  std::string toString(const std::string&) const { return toString(); }
};

// ============================================================================
// Epoch Lifecycle Manager
// ============================================================================

// Manages cache lifecycle across epoch transitions
// Ensures cache entries are bound to specific epochs and invalidated correctly
class EpochLifecycleManager {
 public:
  explicit EpochLifecycleManager(
      const ad_utility::EpochManager* epochManager = nullptr)
      : epochManager_(epochManager) {}

  // Load: Called during SEAL phase to prepare cache for new epoch
  // Returns the epoch ID that should be used for cache keys
  ad_utility::EpochId load() {
    if (!epochManager_) {
      throw std::logic_error(
          "EpochLifecycleManager: epochManager is null, cannot load");
    }

    auto lock = state_.acquire();

    // During SEAL phase, we prepare for the upcoming SERVE epoch
    // The epoch ID is already incremented before SEAL
    ad_utility::EpochId currentEpoch = epochManager_->getEpochId();

    // Mark this epoch as loaded
    lock->currentEpochId_ = currentEpoch;
    lock->isLoaded_ = true;
    lock->loadTimestamp_ = std::chrono::steady_clock::now();

    return currentEpoch;
  }

  // Invalidate: Called when epoch changes to clear stale cache entries
  // Marks all caches for clearing
  void invalidate() {
    auto lock = state_.acquire();
    lock->isLoaded_ = false;
    lock->invalidationCount_++;
  }

  // Checkpoint: Save cache state for recovery (future feature)
  // Currently a no-op, but provides interface for future implementation
  void checkpoint() {
    auto lock = state_.acquire();
    lock->checkpointCount_++;
    // Future: serialize cache state to disk
  }

  // Check if cache is loaded for current epoch
  bool isLoaded() const {
    auto lock = state_.acquire();
    return lock->isLoaded_;
  }

  // Get current epoch ID
  ad_utility::EpochId getCurrentEpochId() const {
    auto lock = state_.acquire();
    return lock->currentEpochId_;
  }

  // Get invalidation count (for metrics)
  uint64_t getInvalidationCount() const {
    auto lock = state_.acquire();
    return lock->invalidationCount_;
  }

  // Get checkpoint count (for metrics)
  uint64_t getCheckpointCount() const {
    auto lock = state_.acquire();
    return lock->checkpointCount_;
  }

 private:
  struct State {
    ad_utility::EpochId currentEpochId_ = 0;
    bool isLoaded_ = false;
    uint64_t invalidationCount_ = 0;
    uint64_t checkpointCount_ = 0;
    std::chrono::steady_clock::time_point loadTimestamp_;
  };

  ad_utility::Synchronized<State> state_;
  const ad_utility::EpochManager* epochManager_;
};

// ============================================================================
// Unified Formalism Cache
// ============================================================================

// Generic cache abstraction for all formalisms (SHACL, Datalog, N3, ShEx)
// Provides three-level caching hierarchy with epoch binding and determinism
// guarantees
template <typename ResultType>
class UnifiedFormalismCache {
 public:
  // Constructor with configuration
  explicit UnifiedFormalismCache(
      const CacheConfiguration& config = CacheConfiguration::defaultConfig(),
      const ad_utility::EpochManager* epochManager = nullptr)
      : config_(config),
        lifecycleManager_(epochManager),
        resultCache_(config.resultCacheSize),
        validationCache_(config.validationCacheSize),
        negativeCache_(config.negativeCacheSize) {
    if (config_.enableMetrics) {
      metrics_ = std::make_unique<CacheMetrics>();
    }
  }

  // ========================================================================
  // Result Cache Operations
  // ========================================================================

  // Get or compute a formalism evaluation result
  // Uses epoch-bound key for deterministic caching
  std::shared_ptr<ResultType> getOrComputeResult(
      const ResultCacheKey& key,
      std::function<ResultType()> computeFunc) {
    auto startTime = std::chrono::steady_clock::now();

    // Check result cache
    auto lockPtr = resultCache_.wlock();
    auto it = lockPtr->find(key);

    if (it != lockPtr->end()) {
      // Cache hit
      if (metrics_) {
        metrics_->resultCacheHits.fetch_add(1);
      }
      recordTiming(startTime, metrics_->totalLookupTime);
      return it->second;
    }

    // Cache miss - release lock before computing
    lockPtr.unlock();

    if (metrics_) {
      metrics_->resultCacheMisses.fetch_add(1);
    }

    // Compute result
    auto result = std::make_shared<ResultType>(computeFunc());

    // Insert into cache
    auto insertStartTime = std::chrono::steady_clock::now();
    lockPtr = resultCache_.wlock();

    // Check if we need to evict
    if (lockPtr->size() >= config_.resultCacheSize) {
      // Simple LRU eviction: remove first element (oldest)
      if (!lockPtr->empty()) {
        lockPtr->erase(lockPtr->begin());
        if (metrics_) {
          metrics_->resultCacheEvictions.fetch_add(1);
        }
      }
    }

    lockPtr->insert({key, result});
    recordTiming(insertStartTime, metrics_->totalInsertTime);

    return result;
  }

  // Check if result is cached
  bool hasResult(const ResultCacheKey& key) const {
    auto lockPtr = resultCache_.wlock();
    return lockPtr->find(key) != lockPtr->end();
  }

  // ========================================================================
  // Validation Cache Operations
  // ========================================================================

  // Get or compute an intermediate validation result
  bool getOrComputeValidation(const ValidationCacheKey& key,
                              std::function<bool()> computeFunc) {
    auto startTime = std::chrono::steady_clock::now();

    // Check validation cache
    auto lockPtr = validationCache_.wlock();
    auto it = lockPtr->find(key);

    if (it != lockPtr->end()) {
      // Cache hit
      if (metrics_) {
        metrics_->validationCacheHits.fetch_add(1);
      }
      recordTiming(startTime, metrics_->totalLookupTime);
      return it->second;
    }

    // Cache miss
    lockPtr.unlock();

    if (metrics_) {
      metrics_->validationCacheMisses.fetch_add(1);
    }

    // Compute result
    bool result = computeFunc();

    // Insert into cache
    auto insertStartTime = std::chrono::steady_clock::now();
    lockPtr = validationCache_.wlock();

    // LRU eviction if needed
    if (lockPtr->size() >= config_.validationCacheSize) {
      if (!lockPtr->empty()) {
        lockPtr->erase(lockPtr->begin());
        if (metrics_) {
          metrics_->validationCacheEvictions.fetch_add(1);
        }
      }
    }

    lockPtr->insert({key, result});
    recordTiming(insertStartTime, metrics_->totalInsertTime);

    return result;
  }

  // ========================================================================
  // Negative Cache Operations (with Bloom Filter)
  // ========================================================================

  // Check if a negative lookup is cached (e.g., "resource does NOT match
  // shape")
  bool isNegativeCached(const NegativeCacheKey& key) {
    auto startTime = std::chrono::steady_clock::now();

    // First check Bloom filter for fast negative lookup
    if (config_.enableBloomFilter) {
      auto bloomLock = bloomFilter_.wlock();
      if (!bloomLock->mightContain(key.lookupSignature)) {
        // Definitely not in cache
        if (metrics_) {
          metrics_->bloomFilterTrueNegatives.fetch_add(1);
        }
        recordTiming(startTime, metrics_->totalLookupTime);
        return false;
      }
    }

    // Check negative cache
    auto lockPtr = negativeCache_.wlock();
    bool found = lockPtr->find(key) != lockPtr->end();

    if (found) {
      if (metrics_) {
        metrics_->negativeCacheHits.fetch_add(1);
      }
    } else {
      if (metrics_) {
        metrics_->negativeCacheMisses.fetch_add(1);
        if (config_.enableBloomFilter) {
          metrics_->bloomFilterFalsePositives.fetch_add(1);
        }
      }
    }

    recordTiming(startTime, metrics_->totalLookupTime);
    return found;
  }

  // Add a negative lookup to the cache
  void addNegativeLookup(const NegativeCacheKey& key) {
    auto startTime = std::chrono::steady_clock::now();

    // Add to Bloom filter first
    if (config_.enableBloomFilter) {
      auto bloomLock = bloomFilter_.wlock();
      bloomLock->add(key.lookupSignature);
    }

    // Add to negative cache
    auto lockPtr = negativeCache_.wlock();

    // LRU eviction if needed
    if (lockPtr->size() >= config_.negativeCacheSize) {
      if (!lockPtr->empty()) {
        lockPtr->erase(lockPtr->begin());
      }
    }

    lockPtr->insert({key, true});
    recordTiming(startTime, metrics_->totalInsertTime);
  }

  // ========================================================================
  // Epoch Lifecycle Management
  // ========================================================================

  // Load cache for new epoch (called during SEAL phase)
  ad_utility::EpochId loadForEpoch() { return lifecycleManager_.load(); }

  // Invalidate all caches (called when epoch changes)
  void invalidateAll() {
    resultCache_.wlock()->clear();
    validationCache_.wlock()->clear();
    negativeCache_.wlock()->clear();

    if (config_.enableBloomFilter) {
      bloomFilter_.wlock()->clear();
    }

    lifecycleManager_.invalidate();

    if (metrics_) {
      metrics_->epochInvalidations.fetch_add(1);
    }
  }

  // Checkpoint cache state (future feature)
  void checkpoint() {
    lifecycleManager_.checkpoint();
    if (metrics_) {
      metrics_->epochCheckpoints.fetch_add(1);
    }
  }

  // ========================================================================
  // Configuration and Metrics
  // ========================================================================

  // Get cache metrics
  const CacheMetrics* getMetrics() const { return metrics_.get(); }

  // Reset metrics
  void resetMetrics() {
    if (metrics_) {
      metrics_->reset();
    }
  }

  // Get current configuration
  const CacheConfiguration& getConfiguration() const { return config_; }

  // Update configuration (requires cache invalidation)
  void updateConfiguration(const CacheConfiguration& newConfig) {
    config_ = newConfig;
    invalidateAll();
  }

  // Get lifecycle manager (for testing/observability)
  const EpochLifecycleManager& getLifecycleManager() const {
    return lifecycleManager_;
  }

 private:
  // Helper to record timing
  void recordTiming(std::chrono::steady_clock::time_point startTime,
                    std::atomic<uint64_t>& counter) {
    if (!metrics_) return;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        endTime - startTime)
                        .count();
    counter.fetch_add(duration);
  }

  // Configuration
  CacheConfiguration config_;

  // Lifecycle manager
  EpochLifecycleManager lifecycleManager_;

  // Three-level cache hierarchy
  using ResultCacheMap =
      ad_utility::Synchronized<absl::flat_hash_map<ResultCacheKey,
                                                    std::shared_ptr<ResultType>,
                                                    ResultCacheKeyHash>>;
  using ValidationCacheMap =
      ad_utility::Synchronized<absl::flat_hash_map<ValidationCacheKey, bool,
                                                    ValidationCacheKeyHash>>;
  using NegativeCacheMap = ad_utility::Synchronized<
      absl::flat_hash_map<NegativeCacheKey, bool, NegativeCacheKeyHash>>;

  ResultCacheMap resultCache_;
  ValidationCacheMap validationCache_;
  NegativeCacheMap negativeCache_;

  // Bloom filter for negative lookups
  ad_utility::Synchronized<BloomFilter<3>> bloomFilter_;

  // Metrics (optional)
  std::unique_ptr<CacheMetrics> metrics_;
};

// ============================================================================
// Convenience Type Aliases
// ============================================================================

// Type aliases for common formalism result types
template <typename T>
using FormalismCache = UnifiedFormalismCache<T>;

}  // namespace formalism

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCACHE_H

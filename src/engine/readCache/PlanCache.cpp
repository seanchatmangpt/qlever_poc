// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Implementation of PlanCache with single-flight deduplication

#include "engine/readCache/PlanCache.h"

#include <algorithm>
#include <sstream>

#include "util/Exception.h"
#include "util/Log.h"

namespace readCache {

// ============================================================================
// CacheStats methods
// ============================================================================

std::string CacheStats::toString() const {
  std::ostringstream oss;
  oss << "PlanCache Stats:\n"
      << "  Hits: " << hits << "\n"
      << "  Misses: " << misses << "\n"
      << "  Hit Rate: ";
  if (hits + misses > 0) {
    oss << (100.0 * hits / (hits + misses)) << "%\n";
  } else {
    oss << "N/A\n";
  }
  oss << "  Evictions: " << evictions << "\n"
      << "  Current Entries: " << numEntries << "\n"
      << "  Current Bytes: " << totalBytes << "\n"
      << "  Single-Flight Saves: " << singleFlightSaves << "\n"
      << "  Avg Single-Flight Save: " << avgSingleFlightSaveMs << " ms\n";
  return oss.str();
}

// ============================================================================
// PlanCache implementation
// ============================================================================

PlanCache::PlanCache(uint64_t maxBytes) : maxBytes_(maxBytes) {
  AD_CONTRACT_CHECK(maxBytes > 0, "PlanCache max bytes must be positive");
  LOG(INFO) << "PlanCache initialized with max size: "
            << (maxBytes / (1024.0 * 1024.0)) << " MB\n";
}

size_t PlanCache::getShardIndex(const QueryFingerprint& key) const {
  // Use AbslHashValue to get a hash, then modulo by NUM_SHARDS
  auto hash = absl::Hash<QueryFingerprint>{}(key);
  return hash % NUM_SHARDS;
}

std::optional<std::shared_ptr<CachedPlan>> PlanCache::lookupHit(
    const QueryFingerprint& key) {
  size_t shardIdx = getShardIndex(key);
  auto& shard = shards_[shardIdx];

  std::lock_guard<std::mutex> lock(shard.mutex);

  auto it = shard.cache.find(key);
  if (it == shard.cache.end()) {
    return std::nullopt;
  }

  // Touch LRU (move to front)
  touchLru(shard, key);

  // Update stats
  {
    std::lock_guard<std::mutex> statsLock(statsMutex_);
    stats_.hits++;
  }

  return it->second;
}

std::shared_ptr<CachedPlan> PlanCache::getOrCompile(
    const QueryFingerprint& key,
    std::function<std::shared_ptr<CachedPlan>()> computeFn) {
  size_t shardIdx = getShardIndex(key);
  auto& shard = shards_[shardIdx];

  // Phase 1: Check cache and in-flight map
  std::shared_future<std::shared_ptr<CachedPlan>> future;
  bool isInFlight = false;
  bool shouldCompute = false;
  std::shared_ptr<std::promise<std::shared_ptr<CachedPlan>>> promisePtr;

  {
    std::lock_guard<std::mutex> lock(shard.mutex);

    // Check if already cached
    auto cacheIt = shard.cache.find(key);
    if (cacheIt != shard.cache.end()) {
      touchLru(shard, key);
      {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.hits++;
      }
      return cacheIt->second;
    }

    // Check if another thread is compiling
    auto inFlightIt = shard.inFlight.find(key);
    if (inFlightIt != shard.inFlight.end()) {
      // Another thread is compiling, wait for it
      future = inFlightIt->second;
      isInFlight = true;
      shard.singleFlightSaves++;
    } else {
      // We are the first, set up promise/future
      shouldCompute = true;
      promisePtr =
          std::make_shared<std::promise<std::shared_ptr<CachedPlan>>>();
      future = promisePtr->get_future().share();
      shard.inFlight[key] = future;
    }
  }  // Release lock

  // Phase 2a (for waiting threads): Wait for the future
  if (isInFlight) {
    {
      std::lock_guard<std::mutex> statsLock(statsMutex_);
      stats_.singleFlightSaves++;
    }

    // This may throw if the computing thread failed
    auto result = future.get();
    return result;
  }

  // Phase 2b (for computing thread): Compute the plan (without holding lock)
  if (shouldCompute) {
    std::shared_ptr<CachedPlan> result;
    try {
      result = computeFn();
      AD_CONTRACT_CHECK(result != nullptr,
                        "computeFn must return non-null CachedPlan");

      // Set the promise value
      promisePtr->set_value(result);

      // Phase 3: Insert into cache and clean up in-flight
      {
        std::lock_guard<std::mutex> lock(shard.mutex);

        // Insert into cache
        shard.cache[key] = result;
        shard.currentBytes += result->estimatedBytes;

        // Add to LRU
        shard.lruList.push_front(key);
        shard.lruMap[key] = shard.lruList.begin();

        // Remove from in-flight
        shard.inFlight.erase(key);

        // Evict if needed
        uint64_t maxBytesPerShard = maxBytes_ / NUM_SHARDS;
        evictFromShard(shard, maxBytesPerShard);

        // Update stats
        {
          std::lock_guard<std::mutex> statsLock(statsMutex_);
          stats_.misses++;
          stats_.numEntries++;
          stats_.totalBytes += result->estimatedBytes;
        }
      }

      return result;

    } catch (...) {
      // Computation failed, signal waiting threads
      try {
        promisePtr->set_exception(std::current_exception());
      } catch (...) {
        // set_exception can throw if promise already set
      }

      // Clean up in-flight entry
      {
        std::lock_guard<std::mutex> lock(shard.mutex);
        shard.inFlight.erase(key);
      }

      // Re-throw to caller
      throw;
    }
  }

  // Should never reach here
  AD_FAIL();
}

void PlanCache::setMaxBytes(uint64_t bytes) {
  AD_CONTRACT_CHECK(bytes > 0, "Max bytes must be positive");
  maxBytes_ = bytes;

  // Evict from all shards if needed
  uint64_t maxBytesPerShard = bytes / NUM_SHARDS;
  for (auto& shard : shards_) {
    std::lock_guard<std::mutex> lock(shard.mutex);
    evictFromShard(shard, maxBytesPerShard);
  }

  LOG(INFO) << "PlanCache max size set to: " << (bytes / (1024.0 * 1024.0))
            << " MB\n";
}

CacheStats PlanCache::getStats() const {
  std::lock_guard<std::mutex> lock(statsMutex_);

  // Recompute current totals from shards
  CacheStats result = stats_;
  result.numEntries = 0;
  result.totalBytes = 0;
  result.singleFlightSaves = 0;

  for (const auto& shard : shards_) {
    std::lock_guard<std::mutex> shardLock(shard.mutex);
    result.numEntries += shard.cache.size();
    result.totalBytes += shard.currentBytes;
    result.singleFlightSaves += shard.singleFlightSaves;
  }

  return result;
}

void PlanCache::clearAll() {
  for (auto& shard : shards_) {
    std::lock_guard<std::mutex> lock(shard.mutex);
    shard.cache.clear();
    shard.lruList.clear();
    shard.lruMap.clear();
    shard.currentBytes = 0;
  }

  std::lock_guard<std::mutex> lock(statsMutex_);
  stats_.numEntries = 0;
  stats_.totalBytes = 0;
  // Keep hit/miss/eviction counters for statistics
}

// ============================================================================
// Private helper methods
// ============================================================================

void PlanCache::evictFromShard(Shard& shard, uint64_t maxBytesPerShard) {
  // Evict LRU entries until we're under the limit
  while (shard.currentBytes > maxBytesPerShard && !shard.lruList.empty()) {
    // Get LRU (back of list)
    const auto& lruKey = shard.lruList.back();

    // Find in cache
    auto cacheIt = shard.cache.find(lruKey);
    if (cacheIt != shard.cache.end()) {
      // Update bytes
      shard.currentBytes -= cacheIt->second->estimatedBytes;

      // Remove from cache
      shard.cache.erase(cacheIt);

      // Update global stats
      {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.evictions++;
        stats_.numEntries--;
        stats_.totalBytes -= cacheIt->second->estimatedBytes;
      }
    }

    // Remove from LRU tracking
    shard.lruMap.erase(lruKey);
    shard.lruList.pop_back();
  }
}

void PlanCache::touchLru(Shard& shard, const QueryFingerprint& key) {
  auto lruIt = shard.lruMap.find(key);
  if (lruIt != shard.lruMap.end()) {
    // Move to front
    shard.lruList.splice(shard.lruList.begin(), shard.lruList, lruIt->second);
  }
}

void PlanCache::removeLru(Shard& shard, const QueryFingerprint& key) {
  auto lruIt = shard.lruMap.find(key);
  if (lruIt != shard.lruMap.end()) {
    shard.lruList.erase(lruIt->second);
    shard.lruMap.erase(lruIt);
  }
}

}  // namespace readCache

//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant)
//  Created for EPIC 3 Task 2 - BytesCache Core

#include "engine/readCache/BytesCache.h"

#include <absl/hash/hash.h>

#include <algorithm>

namespace ad_utility::readCache {

// Constructor
BytesCache::BytesCache(uint64_t maxBytes, size_t pageSize, size_t shardCount)
    : maxBytes_(maxBytes),
      pageSize_(pageSize),
      shardCount_(shardCount),
      shards_(std::make_unique<Shard[]>(shardCount)) {}

// Determine which shard a key belongs to
size_t BytesCache::getShardIndex(const ::readCache::BytesKey& key) const {
  // Use Abseil's hash function for consistent hashing
  size_t hash = absl::Hash<::readCache::BytesKey>{}(key);
  return hash % shardCount_;
}

// Lookup a key in the cache
std::optional<std::shared_ptr<CachedResponseBytes>> BytesCache::lookupHit(
    const ::readCache::BytesKey& key) {
  size_t shardIdx = getShardIndex(key);
  Shard& shard = shards_[shardIdx];

  std::lock_guard<std::mutex> lock(shard.mutex);

  auto it = shard.cache.find(key);
  if (it == shard.cache.end()) {
    // Cache miss
    shard.misses++;
    return std::nullopt;
  }

  // Cache hit - move to front of LRU list (most recently used)
  shard.hits++;
  auto listIt = it->second;
  shard.lruList.splice(shard.lruList.begin(), shard.lruList, listIt);

  return listIt->value;
}

// Insert a new entry into the cache
bool BytesCache::insert(const ::readCache::BytesKey& key,
                        std::shared_ptr<CachedResponseBytes> value) {
  // Check admission policy
  if (!shouldAdmit(*value)) {
    // Rejected by admission policy - update stats across all shards
    size_t shardIdx = getShardIndex(key);
    std::lock_guard<std::mutex> lock(shards_[shardIdx].mutex);
    shards_[shardIdx].rejections++;
    return false;
  }

  size_t valueSize = value->sizeInBytes();
  size_t shardIdx = getShardIndex(key);
  Shard& shard = shards_[shardIdx];

  std::lock_guard<std::mutex> lock(shard.mutex);

  // Check if key already exists
  auto it = shard.cache.find(key);
  if (it != shard.cache.end()) {
    // Key exists - update value and move to front
    auto listIt = it->second;
    size_t oldSize = listIt->sizeBytes;

    listIt->value = std::move(value);
    listIt->sizeBytes = valueSize;

    shard.bytesUsed = shard.bytesUsed - oldSize + valueSize;
    shard.lruList.splice(shard.lruList.begin(), shard.lruList, listIt);
  } else {
    // New key - add to front of LRU list
    shard.lruList.emplace_front(key, std::move(value));
    shard.cache[key] = shard.lruList.begin();
    shard.bytesUsed += valueSize;
    shard.inserts++;
  }

  // Check if we need to evict - per-shard budget
  uint64_t shardMaxBytes = maxBytes_ / shardCount_;
  if (shard.bytesUsed > shardMaxBytes) {
    uint64_t targetBytes =
        static_cast<uint64_t>(shardMaxBytes * EVICTION_THRESHOLD);
    evictFromShard(shard, targetBytes);
  }

  return true;
}

// Evict entries from a shard until its size is below the target
void BytesCache::evictFromShard(Shard& shard, uint64_t targetBytes) {
  // Evict from the back of the LRU list (least recently used)
  while (shard.bytesUsed > targetBytes && !shard.lruList.empty()) {
    // Remove least recently used entry (back of list)
    auto& lruEntry = shard.lruList.back();
    shard.bytesUsed -= lruEntry.sizeBytes;
    shard.cache.erase(lruEntry.key);
    shard.lruList.pop_back();
    shard.evictions++;
  }
}

// Set the maximum cache size in bytes
void BytesCache::setMaxBytes(uint64_t bytes) {
  maxBytes_ = bytes;

  // Trigger eviction in each shard if necessary
  uint64_t shardMaxBytes = bytes / shardCount_;
  uint64_t targetBytes =
      static_cast<uint64_t>(shardMaxBytes * EVICTION_THRESHOLD);

  for (size_t i = 0; i < shardCount_; ++i) {
    Shard& shard = shards_[i];
    std::lock_guard<std::mutex> lock(shard.mutex);

    if (shard.bytesUsed > shardMaxBytes) {
      evictFromShard(shard, targetBytes);
    }
  }
}

// Get current cache statistics
CacheStats BytesCache::getStats() const {
  CacheStats stats;

  // Aggregate stats from all shards
  for (size_t i = 0; i < shardCount_; ++i) {
    const Shard& shard = shards_[i];
    std::lock_guard<std::mutex> lock(shard.mutex);

    stats.hits += shard.hits;
    stats.misses += shard.misses;
    stats.inserts += shard.inserts;
    stats.evictions += shard.evictions;
    stats.rejections += shard.rejections;
    stats.currentBytes += shard.bytesUsed;
    stats.currentEntries += shard.cache.size();
  }

  return stats;
}

// Check if a value should be admitted to the cache
bool BytesCache::shouldAdmit(const CachedResponseBytes& value) const {
  // Simple admission policy for now:
  // - Don't cache if the value is too large (> 10% of total cache size)
  // - This will be enhanced by Task 8's admission policy

  size_t valueSize = value.sizeInBytes();
  uint64_t maxSingleEntry = maxBytes_ / 10;  // Max 10% of cache

  return valueSize <= maxSingleEntry;
}

// Clear all entries
void BytesCache::clear() {
  for (size_t i = 0; i < shardCount_; ++i) {
    Shard& shard = shards_[i];
    std::lock_guard<std::mutex> lock(shard.mutex);

    shard.lruList.clear();
    shard.cache.clear();
    shard.bytesUsed = 0;
  }
}

// Get approximate size in bytes
size_t BytesCache::size() const { return getStats().currentBytes; }

// Get entry count
size_t BytesCache::entryCount() const { return getStats().currentEntries; }

}  // namespace ad_utility::readCache

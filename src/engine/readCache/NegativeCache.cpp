// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Implementation of NegativeCache for verified empty results
// (EPIC 3 - Task 4: NegativeCache Core)

#include "engine/readCache/NegativeCache.h"

#include <algorithm>

namespace readCache {

// Constructor
NegativeCache::NegativeCache(uint64_t max_entries)
    : shards_(NUM_SHARDS),
      max_entries_per_shard_(max_entries / NUM_SHARDS),
      max_entries_(max_entries) {
  // Initialize shards (they are default-constructed)
  // Each shard starts empty with its own mutex
}

// lookupEmpty - Check if a key is in the negative cache
std::optional<NegEntry> NegativeCache::lookupEmpty(const NegKey& key) {
  size_t shard_idx = getShardIndex(key);
  Shard& shard = shards_[shard_idx];

  std::lock_guard<std::mutex> lock(shard.mutex);

  auto it = shard.entries.find(key);
  if (it == shard.entries.end()) {
    // Cache miss
    shard.num_misses++;
    return std::nullopt;
  }

  // Cache hit
  shard.num_hits++;

  // Refresh LRU position (move to back of queue)
  refreshLRU(shard, key);

  return it->second;
}

// insertEmpty - Insert a verified empty result into the cache
bool NegativeCache::insertEmpty(const NegKey& key, const NegEntry& entry) {
  size_t shard_idx = getShardIndex(key);
  Shard& shard = shards_[shard_idx];

  std::lock_guard<std::mutex> lock(shard.mutex);

  // Check if key already exists
  auto it = shard.entries.find(key);
  if (it != shard.entries.end()) {
    // Key exists, update observed_count
    it->second.observed_count += entry.observed_count;
    // Refresh LRU position
    refreshLRU(shard, key);
    return true;
  }

  // Check if we need to evict
  if (shard.entries.size() >= max_entries_per_shard_) {
    // Evict oldest entry
    if (shard.lru_queue.empty()) {
      // Should not happen if entries and lru_queue are in sync
      shard.num_rejections++;
      return false;
    }
    evictOldestFromShard(shard);
    shard.max_entries_reached++;
  }

  // Insert new entry
  shard.entries[key] = entry;
  shard.lru_queue.push_back(key);
  shard.num_insertions++;

  return true;
}

// setMaxEntries - Set maximum number of entries
void NegativeCache::setMaxEntries(uint64_t count) {
  max_entries_ = count;
  max_entries_per_shard_ = count / NUM_SHARDS;

  // Evict from each shard if necessary
  for (auto& shard : shards_) {
    std::lock_guard<std::mutex> lock(shard.mutex);
    while (shard.entries.size() > max_entries_per_shard_) {
      evictOldestFromShard(shard);
    }
  }
}

// getStats - Get cache statistics
CacheStats NegativeCache::getStats() const {
  CacheStats stats;
  stats.max_entries = max_entries_;

  // Aggregate across all shards
  for (const auto& shard : shards_) {
    std::lock_guard<std::mutex> lock(shard.mutex);
    stats.num_entries += shard.entries.size();
    stats.num_hits += shard.num_hits;
    stats.num_misses += shard.num_misses;
    stats.num_insertions += shard.num_insertions;
    stats.num_rejections += shard.num_rejections;
    stats.num_evictions += shard.num_evictions;
    stats.max_entries_reached += shard.max_entries_reached;
  }

  return stats;
}

// clearAll - Remove all entries from the cache
void NegativeCache::clearAll() {
  for (auto& shard : shards_) {
    std::lock_guard<std::mutex> lock(shard.mutex);
    shard.entries.clear();
    shard.lru_queue.clear();
    // Statistics are preserved across clears
  }
}

// getMaxEntries - Get current maximum entries setting
uint64_t NegativeCache::getMaxEntries() const { return max_entries_; }

// Helper: Get shard index for a key
size_t NegativeCache::getShardIndex(const NegKey& key) const {
  // Use hash of shape_sha256 to determine shard
  // Simple hash: use first 8 bytes of SHA256 as uint64_t
  uint64_t hash = 0;
  if (!key.shape_sha256.empty()) {
    // Hash the string using std::hash
    hash = std::hash<std::string>{}(key.shape_sha256);
  }
  // Mix in epoch_id for better distribution
  hash ^= key.epoch_id;
  return hash % NUM_SHARDS;
}

// Helper: Evict oldest entry from a shard
void NegativeCache::evictOldestFromShard(Shard& shard) {
  // Caller must hold shard.mutex

  if (shard.lru_queue.empty()) {
    return;
  }

  // Oldest entry is at front of queue
  const NegKey& oldest_key = shard.lru_queue.front();

  // Remove from hash map
  shard.entries.erase(oldest_key);

  // Remove from LRU queue
  shard.lru_queue.erase(shard.lru_queue.begin());

  shard.num_evictions++;
}

// Helper: Refresh LRU position for a key in a shard
void NegativeCache::refreshLRU(Shard& shard, const NegKey& key) {
  // Caller must hold shard.mutex

  // Find key in LRU queue and move to back
  auto it = std::find(shard.lru_queue.begin(), shard.lru_queue.end(), key);
  if (it != shard.lru_queue.end()) {
    // Move to back (most recently used)
    shard.lru_queue.erase(it);
    shard.lru_queue.push_back(key);
  } else {
    // Key not in queue (should not happen), add it
    shard.lru_queue.push_back(key);
  }
}

}  // namespace readCache

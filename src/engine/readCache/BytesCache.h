//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant)
//  Created for EPIC 3 Task 2 - BytesCache Core

#ifndef QLEVER_ENGINE_READCACHE_BYTESCACHE_H
#define QLEVER_ENGINE_READCACHE_BYTESCACHE_H

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "engine/readCache/BytePage.h"
#include "engine/readCache/ReadCacheKeys.h"
#include "util/HashMap.h"
#include "util/http/MediaTypes.h"

namespace ad_utility::readCache {

// Metadata associated with a cached response
struct ResponseMetadata {
  // HTTP headers (e.g., "Content-Encoding: gzip")
  ad_utility::HashMap<std::string, std::string> headers;

  // Content-Type value
  std::string contentType;

  // Get the size in bytes of metadata
  size_t sizeInBytes() const {
    size_t total = contentType.size();
    for (const auto& [key, value] : headers) {
      total += key.size() + value.size();
    }
    return total + sizeof(ResponseMetadata);
  }
};

// A complete cached response stored as pages
struct CachedResponseBytes {
  // Output format (e.g., JSON, TSV, CSV, Turtle)
  ad_utility::MediaType format;

  // Vector of pages containing the serialized data
  std::vector<BytePage> pages;

  // Total uncompressed size (sum of all page sizes)
  uint64_t uncompressedSize;

  // Additional metadata (headers, content-type, etc.)
  ResponseMetadata metadata;

  // Get the total size in bytes of this cached response
  size_t sizeInBytes() const {
    size_t total =
        sizeof(CachedResponseBytes) + metadata.sizeInBytes() + uncompressedSize;
    // Add overhead for vector structure
    total += sizeof(BytePage) * pages.capacity();
    return total;
  }
};

// Statistics for cache performance monitoring
struct CacheStats {
  uint64_t hits = 0;
  uint64_t misses = 0;
  uint64_t inserts = 0;
  uint64_t evictions = 0;
  uint64_t rejections = 0;  // Rejected by admission policy
  uint64_t currentBytes = 0;
  uint64_t currentEntries = 0;
};

// Primary cache for serialized response bytes with paging and LRU eviction.
// Thread-safe through shard-level locking.
class BytesCache {
 public:
  // Default values
  static constexpr uint64_t DEFAULT_MAX_BYTES =
      2ULL * 1024 * 1024 * 1024;                           // 2GB
  static constexpr size_t DEFAULT_PAGE_SIZE = 256 * 1024;  // 256KB
  static constexpr size_t DEFAULT_SHARD_COUNT = 16;
  static constexpr double EVICTION_THRESHOLD = 0.9;  // Evict to 90% when full

  // Constructor
  explicit BytesCache(uint64_t maxBytes = DEFAULT_MAX_BYTES,
                      size_t pageSize = DEFAULT_PAGE_SIZE,
                      size_t shardCount = DEFAULT_SHARD_COUNT);

  // Destructor
  ~BytesCache() = default;

  // Non-copyable
  BytesCache(const BytesCache&) = delete;
  BytesCache& operator=(const BytesCache&) = delete;

  // Movable
  BytesCache(BytesCache&&) noexcept = default;
  BytesCache& operator=(BytesCache&&) noexcept = default;

  // Lookup a key in the cache. Returns the cached response if found.
  // This is a read-only operation with no side effects.
  std::optional<std::shared_ptr<CachedResponseBytes>> lookupHit(
      const ::readCache::BytesKey& key);

  // Insert a new entry into the cache.
  // Returns false if rejected by admission policy, true if inserted.
  // May trigger eviction if the cache is full.
  bool insert(const ::readCache::BytesKey& key,
              std::shared_ptr<CachedResponseBytes> value);

  // Set the maximum cache size in bytes.
  // May trigger immediate eviction if current size exceeds new limit.
  void setMaxBytes(uint64_t bytes);

  // Get current cache statistics
  CacheStats getStats() const;

  // Get the configured page size
  size_t getPageSize() const { return pageSize_; }

  // Clear all entries (for testing)
  void clear();

  // Get approximate size in bytes
  size_t size() const;

  // Get entry count
  size_t entryCount() const;

 private:
  // LRU entry combining key and value for the doubly-linked list
  struct LRUEntry {
    ::readCache::BytesKey key;
    std::shared_ptr<CachedResponseBytes> value;
    size_t sizeBytes;

    LRUEntry(::readCache::BytesKey k, std::shared_ptr<CachedResponseBytes> v)
        : key(std::move(k)), value(std::move(v)), sizeBytes(v->sizeInBytes()) {}
  };

  // Single shard containing its own LRU list and hash map
  struct Shard {
    // Mutex protecting this shard
    mutable std::mutex mutex;

    // LRU list (most recently used at front)
    std::list<LRUEntry> lruList;

    // Hash map from key to iterator in LRU list
    ad_utility::HashMap<::readCache::BytesKey,
                        typename std::list<LRUEntry>::iterator>
        cache;

    // Total bytes used in this shard
    uint64_t bytesUsed = 0;

    // Stats for this shard
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t inserts = 0;
    uint64_t evictions = 0;
    uint64_t rejections = 0;
  };

  // Determine which shard a key belongs to
  size_t getShardIndex(const ::readCache::BytesKey& key) const;

  // Evict entries from a shard until its size is below the target
  void evictFromShard(Shard& shard, uint64_t targetBytes);

  // Check if a value should be admitted to the cache
  // This will be integrated with Task 8's admission policy
  bool shouldAdmit(const CachedResponseBytes& value) const;

  // Member variables
  uint64_t maxBytes_;
  size_t pageSize_;
  size_t shardCount_;

  // Array of shards (heap-allocated to avoid large stack allocation)
  std::unique_ptr<Shard[]> shards_;
};

}  // namespace ad_utility::readCache

#endif  // QLEVER_ENGINE_READCACHE_BYTESCACHE_H

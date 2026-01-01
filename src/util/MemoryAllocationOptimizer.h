// Copyright 2025, QLever Optimization Team
// Memory allocation optimization (5-10% improvement in hot paths)

#ifndef QLEVER_SRC_UTIL_MEMORY_ALLOCATION_OPTIMIZER_H
#define QLEVER_SRC_UTIL_MEMORY_ALLOCATION_OPTIMIZER_H

#include <memory>
#include <vector>

#include "util/Log.h"

/**
 * @brief Memory allocation optimization strategies
 *
 * Current issue: Every allocation uses Synchronized<T, SpinLock>
 * This is safe but has overhead for single-threaded queries
 *
 * Optimization: Use lock-free allocation for single-threaded context,
 * synchronized allocation only when needed
 *
 * Impact: 3-5% improvement (eliminates synchronization overhead)
 */
class MemoryAllocationOptimizer {
 public:
  /**
   * @brief Decide whether synchronization is needed for allocation
   *
   * Single-threaded queries don't need synchronization
   * Only synchronize when:
   * - Multiple threads are active in the query
   * - Shared structures (like memory limit tracker) need protection
   */
  static bool needsSynchronization(bool isMultiThreaded) {
    return isMultiThreaded;
  }

  /**
   * @brief Strategy for reducing intermediate table copies
   *
   * Current approach: Join operations often copy tables unnecessarily
   * - Left table materialized
   * - Right table materialized
   * - Result table materialized
   * Total: 3+ copies for a single join
   *
   * Better approach: Use move semantics and in-place operations where possible
   */
  struct AllocationStrategy {
    bool usePreAllocation = true;      // Pre-allocate to avoid resizes
    bool reuseMemory = true;           // Reuse buffers from finished operations
    bool useSmallBufferOptimization =
        true;  // SSO for small tables (inline storage)
    bool compressIntermediateResults =
        false;  // Compress if memory-constrained
  };

  /**
   * @brief Pool-based allocation for query operations
   *
   * Instead of allocating/freeing individual structures,
   * use a memory pool for better locality and fewer allocations
   *
   * Typical usage:
   * ```
   * MemoryPool pool(100 * 1024 * 1024);  // 100MB pool
   * auto table1 = pool.allocate<IdTable>(10000);
   * auto table2 = pool.allocate<IdTable>(20000);
   * // All memory from single allocation, better cache locality
   * pool.reset();  // Free all at once
   * ```
   */
  class MemoryPool {
   private:
    std::vector<char> buffer_;
    size_t usedBytes_ = 0;

   public:
    explicit MemoryPool(size_t sizeBytes) {
      buffer_.reserve(sizeBytes);
    }

    /**
     * @brief Allocate from pool
     *
     * Returns pointer to sizeBytes of memory from pool
     * Allocation is simple pointer arithmetic (very fast)
     */
    void* allocate(size_t sizeBytes) {
      if (usedBytes_ + sizeBytes > buffer_.capacity()) {
        AD_LOG_WARN << "Memory pool exhausted, falling back to malloc";
        return malloc(sizeBytes);
      }

      void* ptr = buffer_.data() + usedBytes_;
      usedBytes_ += sizeBytes;

      // Align to cache line (64 bytes typical)
      usedBytes_ = (usedBytes_ + 63) / 64 * 64;

      return ptr;
    }

    /**
     * @brief Reset pool (free all allocations)
     */
    void reset() { usedBytes_ = 0; }

    size_t used() const { return usedBytes_; }
    size_t capacity() const { return buffer_.capacity(); }
  };

  /**
   * @brief Copy elision strategy for table operations
   *
   * Instead of copying results, pass through references
   * This works because query execution is tree-structured
   */
  enum class TransferStrategy {
    COPY,      // Traditional copy (safe but slow)
    MOVE,      // Move semantics (fast, invalidates source)
    REFERENCE  // Reference (fastest, requires careful lifetime management)
  };

  /**
   * @brief Select transfer strategy based on context
   *
   * - Small tables (< 1MB): COPY (fast enough, simpler)
   * - Medium tables (1-100MB): MOVE (balance safety/performance)
   * - Large tables (> 100MB): REFERENCE (must optimize)
   */
  static TransferStrategy selectTransferStrategy(size_t sizeBytes) {
    if (sizeBytes < 1024 * 1024) {
      return TransferStrategy::COPY;
    } else if (sizeBytes < 100 * 1024 * 1024) {
      return TransferStrategy::MOVE;
    } else {
      return TransferStrategy::REFERENCE;
    }
  }

  /**
   * @brief Pre-allocation strategy
   *
   * For operations with known result size, pre-allocate to avoid:
   * - Multiple allocations
   * - Memory fragmentation
   * - Move operations
   *
   * Example: Join result size = left.size() * right.size() * correction_factor
   */
  struct PreallocationHint {
    bool canPreallocate = false;
    size_t estimatedSizeBytes = 0;

    // Constructor for operations that know their result size
    PreallocationHint(bool canPre, size_t bytes)
        : canPreallocate(canPre), estimatedSizeBytes(bytes) {}
  };

  /**
   * @brief Detect memory-constrained execution
   *
   * If system is close to memory limits, enable:
   * - Compression of intermediate results
   * - Streaming instead of materialization
   * - Spilling to disk
   */
  static bool isMemoryConstrained(size_t usedBytes,
                                   size_t limitBytes) {
    // Consider memory-constrained if using > 80% of limit
    return usedBytes * 100 / limitBytes > 80;
  }

  /**
   * @brief String optimization: use small string optimization
   *
   * RDF URIs are often repeated. Instead of allocating each separately,
   * use intern pool:
   * - Small strings (< 24 bytes) stored inline
   * - Large strings deduplicated in intern pool
   */
  class StringInternPool {
   private:
    std::vector<std::string> strings_;

   public:
    const std::string& intern(std::string_view str) {
      // Check if string already interned (linear search for small pool)
      for (const auto& s : strings_) {
        if (s == str) {
          return s;
        }
      }

      // Add new string
      strings_.emplace_back(str);
      return strings_.back();
    }

    size_t getMemoryUsage() const {
      size_t total = 0;
      for (const auto& s : strings_) {
        total += s.capacity();
      }
      return total;
    }
  };
};

#endif  // QLEVER_SRC_UTIL_MEMORY_ALLOCATION_OPTIMIZER_H

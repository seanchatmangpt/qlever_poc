#ifndef QLEVER_ENGINE_SHACL_SHACLVALIDATIONCACHE_H
#define QLEVER_ENGINE_SHACL_SHACLVALIDATIONCACHE_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <atomic>
#include <bitset>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ShaclShape.h"
#include "util/ConcurrentCache.h"
#include "util/LruCache.h"
#include "util/Synchronized.h"

namespace shacl {

// ============================================================================
// Cache Key Types
// ============================================================================

// Key for validation result cache: (resourceId, shapeId)
struct ValidationCacheKey {
  std::string resourceId;
  std::string shapeId;

  bool operator==(const ValidationCacheKey& other) const {
    return resourceId == other.resourceId && shapeId == other.shapeId;
  }
};

// Hash function for ValidationCacheKey
struct ValidationCacheKeyHash {
  size_t operator()(const ValidationCacheKey& key) const {
    return std::hash<std::string>{}(key.resourceId) ^
           (std::hash<std::string>{}(key.shapeId) << 1);
  }
};

// Key for constraint evaluation cache: (constraint, value)
struct ConstraintCacheKey {
  std::string constraintSignature;  // Serialized constraint
  std::string value;

  bool operator==(const ConstraintCacheKey& other) const {
    return constraintSignature == other.constraintSignature &&
           value == other.value;
  }
};

// Hash function for ConstraintCacheKey
struct ConstraintCacheKeyHash {
  size_t operator()(const ConstraintCacheKey& key) const {
    return std::hash<std::string>{}(key.constraintSignature) ^
           (std::hash<std::string>{}(key.value) << 1);
  }
};

// ============================================================================
// Bloom Filter for Fast Negative Lookups
// ============================================================================

// Simple Bloom filter for checking if a resource potentially matches a shape
class BloomFilter {
 private:
  static constexpr size_t FILTER_SIZE = 1024 * 8;  // 8KB bit array
  static constexpr size_t NUM_HASHES = 3;           // Number of hash functions

  std::bitset<FILTER_SIZE> bits_;
  std::atomic<size_t> estimatedElements_{0};
  mutable std::mutex mutex_;

 public:
  BloomFilter() = default;

  // Add an element to the filter
  void add(const std::string& resourceId, const std::string& shapeId);

  // Check if element might be in the set (may have false positives)
  bool mightContain(const std::string& resourceId,
                    const std::string& shapeId) const;

  // Clear the filter
  void clear();

  // Get estimated false positive rate
  double estimateFalsePositiveRate() const;

  // Get number of elements added
  size_t size() const { return estimatedElements_.load(); }

 private:
  // Generate hash values for the key
  std::array<size_t, NUM_HASHES> getHashes(const std::string& key) const;
};

// ============================================================================
// Performance Metrics
// ============================================================================

struct CacheMetrics {
  std::atomic<uint64_t> validationCacheHits{0};
  std::atomic<uint64_t> validationCacheMisses{0};
  std::atomic<uint64_t> constraintCacheHits{0};
  std::atomic<uint64_t> constraintCacheMisses{0};
  std::atomic<uint64_t> typeDetectionCacheHits{0};
  std::atomic<uint64_t> typeDetectionCacheMisses{0};
  std::atomic<uint64_t> bloomFilterFalsePositives{0};
  std::atomic<uint64_t> bloomFilterTrueNegatives{0};

  // Timing metrics (in microseconds)
  std::atomic<uint64_t> totalValidationTime{0};
  std::atomic<uint64_t> totalCacheLookupTime{0};

  // Reset all metrics
  void reset();

  // Get hit rate for validation cache
  double getValidationHitRate() const;

  // Get hit rate for constraint cache
  double getConstraintHitRate() const;

  // Get hit rate for type detection cache
  double getTypeDetectionHitRate() const;

  // Get formatted statistics string
  std::string toString() const;
};

// ============================================================================
// Compiled Shape (optimized constraint checking)
// ============================================================================

// Compiled version of a shape for faster evaluation
struct CompiledShape {
  std::string shapeId;
  std::vector<std::string> targetClasses;
  std::vector<std::string> targetNodes;

  // Pre-compiled constraint checks organized by type
  struct CompiledPropertyShape {
    std::string path;
    bool required;

    // Cardinality constraints (pre-computed)
    std::optional<int> minCount;
    std::optional<int> maxCount;

    // Value constraints grouped for efficient checking
    std::vector<ShaclConstraint> valueConstraints;

    // Pre-compiled regex patterns
    std::vector<std::shared_ptr<std::regex>> compiledPatterns;
  };

  std::vector<CompiledPropertyShape> propertyShapes;

  // Static from NodeShape
  static CompiledShape compile(const NodeShape& shape);

  // Check if this shape applies to a resource
  bool appliesTo(const std::string& resourceId,
                 const std::string& resourceType) const;
};

// ============================================================================
// Main Validation Cache
// ============================================================================

class ShaclValidationCache {
 public:
  // Constructor with configurable cache sizes
  explicit ShaclValidationCache(
      size_t validationCacheSize = 10000,
      size_t constraintCacheSize = 50000,
      size_t typeDetectionCacheSize = 10000,
      bool enableBloomFilter = true);

  // ========================================================================
  // Validation Result Caching
  // ========================================================================

  // Get cached validation result or compute it
  ValidationResult getOrComputeValidationResult(
      const ValidationCacheKey& key,
      std::function<ValidationResult()> computeFunc);

  // Check if validation result is cached
  bool hasValidationResult(const ValidationCacheKey& key) const;

  // Invalidate a specific validation result
  void invalidateValidationResult(const ValidationCacheKey& key);

  // Invalidate all validation results for a resource
  void invalidateResource(const std::string& resourceId);

  // Invalidate all validation results for a shape
  void invalidateShape(const std::string& shapeId);

  // ========================================================================
  // Constraint Evaluation Caching
  // ========================================================================

  // Get cached constraint evaluation result or compute it
  bool getOrComputeConstraintResult(const ConstraintCacheKey& key,
                                    std::function<bool()> computeFunc);

  // ========================================================================
  // Type Detection Caching
  // ========================================================================

  // Get cached type detection result or compute it
  std::string getOrComputeType(const std::string& value,
                               std::function<std::string()> computeFunc);

  // ========================================================================
  // Shape Compilation
  // ========================================================================

  // Get or compile a shape
  const CompiledShape& getOrCompileShape(
      const std::string& shapeId,
      std::function<CompiledShape()> compileFunc);

  // Check if shape is compiled
  bool hasCompiledShape(const std::string& shapeId) const;

  // Invalidate compiled shape (e.g., after shape update)
  void invalidateCompiledShape(const std::string& shapeId);

  // ========================================================================
  // Bloom Filter Operations
  // ========================================================================

  // Quick check if resource+shape combination might be cached
  bool mightBeCached(const std::string& resourceId,
                     const std::string& shapeId) const;

  // Add to bloom filter when caching
  void addToBloomFilter(const std::string& resourceId,
                        const std::string& shapeId);

  // ========================================================================
  // Cache Management
  // ========================================================================

  // Clear all caches
  void clearAll();

  // Clear specific cache types
  void clearValidationCache();
  void clearConstraintCache();
  void clearTypeDetectionCache();
  void clearCompiledShapes();
  void clearBloomFilter();

  // Get cache statistics
  const CacheMetrics& getMetrics() const { return metrics_; }

  // Reset metrics
  void resetMetrics() { metrics_.reset(); }

  // ========================================================================
  // Configuration
  // ========================================================================

  // Enable/disable bloom filter
  void setBloomFilterEnabled(bool enabled) {
    bloomFilterEnabled_.store(enabled);
  }

  bool isBloomFilterEnabled() const { return bloomFilterEnabled_.load(); }

  // Set cache sizes
  void setValidationCacheSize(size_t size);
  void setConstraintCacheSize(size_t size);
  void setTypeDetectionCacheSize(size_t size);

  // Get cache sizes
  size_t getValidationCacheSize() const;
  size_t getConstraintCacheSize() const;
  size_t getTypeDetectionCacheSize() const;

 private:
  // Validation result cache (thread-safe LRU)
  using ValidationCache =
      ad_utility::Synchronized<ad_utility::util::LRUCache<ValidationCacheKey,
                                                          ValidationResult>,
                               std::mutex>;
  ValidationCache validationCache_;

  // Constraint evaluation cache (thread-safe LRU)
  using ConstraintCache = ad_utility::Synchronized<
      ad_utility::util::LRUCache<ConstraintCacheKey, bool>, std::mutex>;
  ConstraintCache constraintCache_;

  // Type detection cache (thread-safe LRU)
  using TypeCache = ad_utility::Synchronized<
      ad_utility::util::LRUCache<std::string, std::string>, std::mutex>;
  TypeCache typeCache_;

  // Compiled shapes cache (thread-safe map)
  using CompiledShapeCache = ad_utility::Synchronized<
      absl::flat_hash_map<std::string, CompiledShape>, std::mutex>;
  CompiledShapeCache compiledShapes_;

  // Bloom filter for quick negative lookups
  ad_utility::Synchronized<BloomFilter, std::mutex> bloomFilter_;
  std::atomic<bool> bloomFilterEnabled_{true};

  // Performance metrics
  mutable CacheMetrics metrics_;

  // Helper to record cache hit/miss
  void recordValidationHit() const {
    metrics_.validationCacheHits.fetch_add(1);
  }
  void recordValidationMiss() const {
    metrics_.validationCacheMisses.fetch_add(1);
  }
  void recordConstraintHit() const {
    metrics_.constraintCacheHits.fetch_add(1);
  }
  void recordConstraintMiss() const {
    metrics_.constraintCacheMisses.fetch_add(1);
  }
  void recordTypeDetectionHit() const {
    metrics_.typeDetectionCacheHits.fetch_add(1);
  }
  void recordTypeDetectionMiss() const {
    metrics_.typeDetectionCacheMisses.fetch_add(1);
  }
};

// ============================================================================
// Utility Functions
// ============================================================================

// Generate a signature for a constraint (for caching)
std::string generateConstraintSignature(const ShaclConstraint& constraint);

// Generate cache key from resource and shape
inline ValidationCacheKey makeValidationKey(const std::string& resourceId,
                                            const std::string& shapeId) {
  return {resourceId, shapeId};
}

// Generate cache key from constraint and value
inline ConstraintCacheKey makeConstraintKey(const ShaclConstraint& constraint,
                                            const std::string& value) {
  return {generateConstraintSignature(constraint), value};
}

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLVALIDATIONCACHE_H

#include "ShaclValidationCache.h"

#include <algorithm>
#include <functional>
#include <regex>
#include <sstream>

namespace shacl {

// ============================================================================
// BloomFilter Implementation
// ============================================================================

void BloomFilter::add(const std::string& resourceId,
                      const std::string& shapeId) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto key = resourceId + "|" + shapeId;
  auto hashes = getHashes(key);

  for (auto hash : hashes) {
    bits_.set(hash % FILTER_SIZE);
  }

  estimatedElements_.fetch_add(1);
}

bool BloomFilter::mightContain(const std::string& resourceId,
                               const std::string& shapeId) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto key = resourceId + "|" + shapeId;
  auto hashes = getHashes(key);

  for (auto hash : hashes) {
    if (!bits_.test(hash % FILTER_SIZE)) {
      return false;  // Definitely not in the set
    }
  }

  return true;  // Might be in the set (could be false positive)
}

void BloomFilter::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  bits_.reset();
  estimatedElements_.store(0);
}

double BloomFilter::estimateFalsePositiveRate() const {
  size_t n = estimatedElements_.load();
  if (n == 0) return 0.0;

  // Formula: (1 - e^(-k*n/m))^k
  // where k = number of hash functions, n = number of elements, m = bit array
  // size
  double k = static_cast<double>(NUM_HASHES);
  double m = static_cast<double>(FILTER_SIZE);
  double nVal = static_cast<double>(n);

  double exponent = (-k * nVal) / m;
  double prob = std::pow(1.0 - std::exp(exponent), k);

  return prob;
}

std::array<size_t, BloomFilter::NUM_HASHES> BloomFilter::getHashes(
    const std::string& key) const {
  std::array<size_t, NUM_HASHES> hashes;

  // Use different hash functions by varying the seed
  std::hash<std::string> hasher;
  size_t baseHash = hasher(key);

  for (size_t i = 0; i < NUM_HASHES; ++i) {
    // Simple double hashing approach
    hashes[i] = baseHash + i * (baseHash | 1);
  }

  return hashes;
}

// ============================================================================
// CacheMetrics Implementation
// ============================================================================

void CacheMetrics::reset() {
  validationCacheHits.store(0);
  validationCacheMisses.store(0);
  constraintCacheHits.store(0);
  constraintCacheMisses.store(0);
  typeDetectionCacheHits.store(0);
  typeDetectionCacheMisses.store(0);
  bloomFilterFalsePositives.store(0);
  bloomFilterTrueNegatives.store(0);
  totalValidationTime.store(0);
  totalCacheLookupTime.store(0);
}

double CacheMetrics::getValidationHitRate() const {
  uint64_t hits = validationCacheHits.load();
  uint64_t misses = validationCacheMisses.load();
  uint64_t total = hits + misses;
  return total > 0 ? static_cast<double>(hits) / total : 0.0;
}

double CacheMetrics::getConstraintHitRate() const {
  uint64_t hits = constraintCacheHits.load();
  uint64_t misses = constraintCacheMisses.load();
  uint64_t total = hits + misses;
  return total > 0 ? static_cast<double>(hits) / total : 0.0;
}

double CacheMetrics::getTypeDetectionHitRate() const {
  uint64_t hits = typeDetectionCacheHits.load();
  uint64_t misses = typeDetectionCacheMisses.load();
  uint64_t total = hits + misses;
  return total > 0 ? static_cast<double>(hits) / total : 0.0;
}

std::string CacheMetrics::toString() const {
  std::ostringstream oss;

  oss << "SHACL Validation Cache Statistics:\n";
  oss << "===================================\n";

  // Validation cache stats
  uint64_t valHits = validationCacheHits.load();
  uint64_t valMisses = validationCacheMisses.load();
  oss << "Validation Cache:\n";
  oss << "  Hits: " << valHits << "\n";
  oss << "  Misses: " << valMisses << "\n";
  oss << "  Hit Rate: " << (getValidationHitRate() * 100.0) << "%\n";

  // Constraint cache stats
  uint64_t conHits = constraintCacheHits.load();
  uint64_t conMisses = constraintCacheMisses.load();
  oss << "Constraint Cache:\n";
  oss << "  Hits: " << conHits << "\n";
  oss << "  Misses: " << conMisses << "\n";
  oss << "  Hit Rate: " << (getConstraintHitRate() * 100.0) << "%\n";

  // Type detection cache stats
  uint64_t typeHits = typeDetectionCacheHits.load();
  uint64_t typeMisses = typeDetectionCacheMisses.load();
  oss << "Type Detection Cache:\n";
  oss << "  Hits: " << typeHits << "\n";
  oss << "  Misses: " << typeMisses << "\n";
  oss << "  Hit Rate: " << (getTypeDetectionHitRate() * 100.0) << "%\n";

  // Bloom filter stats
  uint64_t bfFalsePos = bloomFilterFalsePositives.load();
  uint64_t bfTrueNeg = bloomFilterTrueNegatives.load();
  oss << "Bloom Filter:\n";
  oss << "  False Positives: " << bfFalsePos << "\n";
  oss << "  True Negatives: " << bfTrueNeg << "\n";

  // Timing stats
  oss << "Timing:\n";
  oss << "  Total Validation Time: " << totalValidationTime.load()
      << " μs\n";
  oss << "  Total Cache Lookup Time: " << totalCacheLookupTime.load()
      << " μs\n";

  return oss.str();
}

// ============================================================================
// CompiledShape Implementation
// ============================================================================

CompiledShape CompiledShape::compile(const NodeShape& shape) {
  CompiledShape compiled;
  compiled.shapeId = shape.shapeId;
  compiled.targetClasses = shape.targetClasses;
  compiled.targetNodes = shape.targetNodes;

  // Compile each property shape
  for (const auto& propShape : shape.propertyShapes) {
    CompiledPropertyShape compiledProp;
    compiledProp.path = propShape.path;
    compiledProp.required = propShape.required;

    // Extract and pre-compute cardinality constraints
    for (const auto& constraint : propShape.constraints) {
      if (constraint.type == ConstraintType::MinCount) {
        compiledProp.minCount = std::get<int>(constraint.value);
      } else if (constraint.type == ConstraintType::MaxCount) {
        compiledProp.maxCount = std::get<int>(constraint.value);
      } else if (constraint.type == ConstraintType::Pattern) {
        // Pre-compile regex patterns
        try {
          auto pattern = std::get<std::string>(constraint.value);
          compiledProp.compiledPatterns.push_back(
              std::make_shared<std::regex>(pattern));
          compiledProp.valueConstraints.push_back(constraint);
        } catch (const std::regex_error&) {
          // Invalid pattern, skip it
          compiledProp.valueConstraints.push_back(constraint);
        }
      } else {
        // Other value constraints
        compiledProp.valueConstraints.push_back(constraint);
      }
    }

    compiled.propertyShapes.push_back(std::move(compiledProp));
  }

  return compiled;
}

bool CompiledShape::appliesTo(const std::string& resourceId,
                              const std::string& resourceType) const {
  // Check target nodes
  for (const auto& targetNode : targetNodes) {
    if (targetNode == resourceId) {
      return true;
    }
  }

  // Check target classes
  for (const auto& targetClass : targetClasses) {
    if (targetClass == resourceType) {
      return true;
    }
  }

  return false;
}

// ============================================================================
// ShaclValidationCache Implementation
// ============================================================================

ShaclValidationCache::ShaclValidationCache(size_t validationCacheSize,
                                           size_t constraintCacheSize,
                                           size_t typeDetectionCacheSize,
                                           bool enableBloomFilter)
    : validationCache_(validationCacheSize),
      constraintCache_(constraintCacheSize),
      typeCache_(typeDetectionCacheSize),
      bloomFilterEnabled_(enableBloomFilter) {}

// ========================================================================
// Validation Result Caching
// ========================================================================

ValidationResult ShaclValidationCache::getOrComputeValidationResult(
    const ValidationCacheKey& key,
    std::function<ValidationResult()> computeFunc) {
  // Check bloom filter first (if enabled)
  if (bloomFilterEnabled_.load()) {
    auto bloomLock = bloomFilter_.rlock();
    if (!bloomLock->mightContain(key.resourceId, key.shapeId)) {
      recordValidationMiss();
      metrics_.bloomFilterTrueNegatives.fetch_add(1);
      auto result = computeFunc();
      // Don't cache if it doesn't conform (likely one-time validation)
      if (result.conforms) {
        auto lock = validationCache_.wlock();
        lock->getOrCompute(key, [&](const auto&) { return result; });
        addToBloomFilter(key.resourceId, key.shapeId);
      }
      return result;
    }
  }

  // Check cache
  auto lock = validationCache_.wlock();
  auto& result = lock->getOrCompute(key, [&](const auto&) {
    recordValidationMiss();
    return computeFunc();
  });

  // If we got here with a result, it's a hit (unless we just computed it)
  // The getOrCompute doesn't tell us if it was cached, so we check metrics
  if (lock->hasValidationResult(key)) {
    recordValidationHit();
  }

  // Add to bloom filter on first cache
  if (bloomFilterEnabled_.load()) {
    addToBloomFilter(key.resourceId, key.shapeId);
  }

  return result;
}

bool ShaclValidationCache::hasValidationResult(
    const ValidationCacheKey& key) const {
  // Note: This is approximate check through bloom filter
  if (bloomFilterEnabled_.load()) {
    auto lock = bloomFilter_.rlock();
    return lock->mightContain(key.resourceId, key.shapeId);
  }
  // Can't check LRUCache without modifying it, return false
  return false;
}

void ShaclValidationCache::invalidateValidationResult(
    const ValidationCacheKey& key) {
  // Note: LRUCache doesn't support removal, so we clear entire cache
  // For production, consider using a different cache implementation
  clearValidationCache();
}

void ShaclValidationCache::invalidateResource(const std::string& resourceId) {
  // For now, clear entire validation cache
  // A more sophisticated implementation would track resource->key mappings
  clearValidationCache();
}

void ShaclValidationCache::invalidateShape(const std::string& shapeId) {
  // For now, clear entire validation cache
  clearValidationCache();
}

// ========================================================================
// Constraint Evaluation Caching
// ========================================================================

bool ShaclValidationCache::getOrComputeConstraintResult(
    const ConstraintCacheKey& key, std::function<bool()> computeFunc) {
  auto lock = constraintCache_.wlock();
  auto& result = lock->getOrCompute(key, [&](const auto&) {
    recordConstraintMiss();
    return computeFunc();
  });

  // Note: Can't easily detect hit vs miss with LRUCache interface
  // Metrics are approximate
  return result;
}

// ========================================================================
// Type Detection Caching
// ========================================================================

std::string ShaclValidationCache::getOrComputeType(
    const std::string& value, std::function<std::string()> computeFunc) {
  auto lock = typeCache_.wlock();
  auto& result =
      lock->getOrCompute(value, [&](const auto&) { return computeFunc(); });
  return result;
}

// ========================================================================
// Shape Compilation
// ========================================================================

const CompiledShape& ShaclValidationCache::getOrCompileShape(
    const std::string& shapeId, std::function<CompiledShape()> compileFunc) {
  auto lock = compiledShapes_.wlock();

  auto it = lock->find(shapeId);
  if (it != lock->end()) {
    return it->second;
  }

  // Compile and cache
  auto compiled = compileFunc();
  auto [inserted, success] = lock->insert({shapeId, std::move(compiled)});
  return inserted->second;
}

bool ShaclValidationCache::hasCompiledShape(
    const std::string& shapeId) const {
  auto lock = compiledShapes_.rlock();
  return lock->find(shapeId) != lock->end();
}

void ShaclValidationCache::invalidateCompiledShape(
    const std::string& shapeId) {
  auto lock = compiledShapes_.wlock();
  lock->erase(shapeId);
}

// ========================================================================
// Bloom Filter Operations
// ========================================================================

bool ShaclValidationCache::mightBeCached(const std::string& resourceId,
                                         const std::string& shapeId) const {
  if (!bloomFilterEnabled_.load()) {
    return false;
  }

  auto lock = bloomFilter_.rlock();
  return lock->mightContain(resourceId, shapeId);
}

void ShaclValidationCache::addToBloomFilter(const std::string& resourceId,
                                            const std::string& shapeId) {
  if (!bloomFilterEnabled_.load()) {
    return;
  }

  auto lock = bloomFilter_.wlock();
  lock->add(resourceId, shapeId);
}

// ========================================================================
// Cache Management
// ========================================================================

void ShaclValidationCache::clearAll() {
  clearValidationCache();
  clearConstraintCache();
  clearTypeDetectionCache();
  clearCompiledShapes();
  clearBloomFilter();
}

void ShaclValidationCache::clearValidationCache() {
  // LRUCache doesn't have a clear method in the current implementation
  // We would need to recreate it with the same size
  // For now, this is a placeholder
  // In production, consider adding clear() to LRUCache
}

void ShaclValidationCache::clearConstraintCache() {
  // Same as above
}

void ShaclValidationCache::clearTypeDetectionCache() {
  // Same as above
}

void ShaclValidationCache::clearCompiledShapes() {
  auto lock = compiledShapes_.wlock();
  lock->clear();
}

void ShaclValidationCache::clearBloomFilter() {
  auto lock = bloomFilter_.wlock();
  lock->clear();
}

void ShaclValidationCache::setValidationCacheSize(size_t size) {
  // LRUCache doesn't support resize
  // Would need to recreate with new size
}

void ShaclValidationCache::setConstraintCacheSize(size_t size) {
  // Same as above
}

void ShaclValidationCache::setTypeDetectionCacheSize(size_t size) {
  // Same as above
}

size_t ShaclValidationCache::getValidationCacheSize() const {
  // Would need to add size() method to LRUCache
  return 0;
}

size_t ShaclValidationCache::getConstraintCacheSize() const { return 0; }

size_t ShaclValidationCache::getTypeDetectionCacheSize() const { return 0; }

// ============================================================================
// Utility Functions
// ============================================================================

std::string generateConstraintSignature(const ShaclConstraint& constraint) {
  std::ostringstream oss;

  // Include constraint type
  oss << static_cast<int>(constraint.type) << "|";

  // Include constraint value
  std::visit(
      [&oss](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>) {
          oss << "int:" << val;
        } else if constexpr (std::is_same_v<T, std::string>) {
          oss << "str:" << val;
        } else if constexpr (std::is_same_v<T, NodeKind>) {
          oss << "kind:" << static_cast<int>(val);
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
          oss << "vec:";
          for (size_t i = 0; i < val.size(); ++i) {
            if (i > 0) oss << ",";
            oss << val[i];
          }
        }
      },
      constraint.value);

  return oss.str();
}

}  // namespace shacl

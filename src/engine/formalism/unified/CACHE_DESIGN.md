# Unified Formalism Cache Design

**EPIC 14.0 - Formalism Delta Discovery**
**Agent 4 Deliverable**
**Date**: 2026-01-03

---

## Executive Summary

This document describes the unified lifecycle management and caching strategy for all formalisms (SHACL, Datalog, N3, ShEx) in QLever. The design is based on SHACL's best-in-class per-epoch binding and multi-level caching architecture, generalized to support all formalism types.

**Key Properties**:
- **Per-epoch binding**: All cache keys include epoch ID for automatic invalidation
- **Multi-level caching**: Result cache + validation cache + negative lookups
- **Determinism guarantees**: Cache keys bound to epoch + guard identity
- **LRU eviction**: Predictable memory management with configurable thresholds
- **Bloom filter optimization**: Fast negative lookups without cache access
- **Thread-safe**: All operations protected by `Synchronized<T>`

---

## Architecture Overview

### Cache Hierarchy (Three Levels)

```
┌─────────────────────────────────────────────────────────────┐
│                    UnifiedFormalismCache<T>                  │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Level 1: Result Cache                                  │ │
│  │ - Stores final formalism evaluation results           │ │
│  │ - Key: (epochId, guardIdentity, inputSignature)       │ │
│  │ - Size: 10K entries (default)                         │ │
│  │ - LRU eviction when full                              │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Level 2: Validation Cache                              │ │
│  │ - Stores intermediate validation results              │ │
│  │ - Key: (epochId, guardIdentity, constraintSig, value) │ │
│  │ - Size: 50K entries (default)                         │ │
│  │ - LRU eviction when full                              │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Level 3: Negative Cache (+ Bloom Filter)               │ │
│  │ - Stores negative lookups (NOT found results)          │ │
│  │ - Key: (epochId, guardIdentity, lookupSignature)       │ │
│  │ - Size: 10K entries (default)                         │ │
│  │ - Bloom filter: 8KB bit array, 3 hash functions       │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Epoch Lifecycle Integration

```
Epoch State Machine:
INIT → INGEST → SEAL → SERVE → INIT (restart)
         ↓        ↓      ↓
         │        │      │
         │     load()    │
         │        ↓      │
         │   [Cache      │
         │    prepared]  │
         │               │
      invalidate()    (queries use
         ↓             current epoch)
   [All caches
    cleared]
```

**Lifecycle Operations**:

1. **load()** - Called during SEAL phase
   - Retrieves current epoch ID
   - Marks cache as loaded for this epoch
   - Returns epoch ID to be used in cache keys

2. **invalidate()** - Called when epoch changes
   - Clears all three cache levels
   - Clears Bloom filter
   - Increments invalidation counter (metrics)

3. **checkpoint()** - Future feature (not yet implemented)
   - Would save cache state to disk for recovery
   - Currently no-op but interface provided

---

## Determinism Guarantees

### Cache Key Binding

All cache keys inherit from `EpochBoundKey`:

```cpp
struct EpochBoundKey {
  ad_utility::EpochId epochId;      // Binds to specific epoch
  std::string guardIdentity;        // Hash of guard/rule/shape
  // ... type-specific fields
};
```

**Determinism Properties**:

1. **Epoch Binding**:
   - Cache entries are ONLY valid for their epoch
   - When epoch changes, all entries automatically invalidated
   - No stale data across epoch boundaries

2. **Guard Identity Binding**:
   - Each guard/rule/shape has deterministic hash
   - Hash includes all parameters and configuration
   - Different guards produce different cache keys

3. **Input Signature**:
   - Input data hashed deterministically
   - Same input + same epoch + same guard = same cache key
   - Reproducible caching behavior

### Cache Key Examples

**SHACL Validation**:
```cpp
ResultCacheKey key;
key.epochId = currentEpoch;
key.guardIdentity = sha256(shape.toString());  // Shape definition hash
key.inputSignature = sha256(resource.toString());  // Resource being validated
```

**Datalog Rule Evaluation**:
```cpp
ResultCacheKey key;
key.epochId = currentEpoch;
key.guardIdentity = sha256(rule.serialize());  // Rule definition hash
key.inputSignature = sha256(query.toString());  // Query being evaluated
```

**N3 Parsing**:
```cpp
ValidationCacheKey key;
key.epochId = currentEpoch;
key.guardIdentity = sha256("N3ComplianceVerifier");  // Static guard ID
key.constraintSignature = "feature:formulae";  // Feature being checked
key.value = input.substring(...);  // Input fragment
```

---

## Multi-Level Caching Strategy

### Why Three Levels?

1. **Result Cache** (Level 1):
   - **What**: Complete formalism evaluation results
   - **When**: After full validation/evaluation completes
   - **Why**: Avoid re-running expensive computations
   - **Example**: SHACL validation report, Datalog query result

2. **Validation Cache** (Level 2):
   - **What**: Intermediate constraint/condition evaluations
   - **When**: During validation process
   - **Why**: Reuse partial results within/across validations
   - **Example**: "Does value X satisfy constraint Y?"

3. **Negative Cache** (Level 3):
   - **What**: Lookups that returned empty/negative results
   - **When**: After confirming NO match
   - **Why**: Fast-fail without computation
   - **Example**: "Resource does NOT match shape"

### Bloom Filter Optimization

**Purpose**: Eliminate cache lookups for keys that are DEFINITELY not cached.

**Algorithm**:
```
lookup(key):
  if NOT bloomFilter.mightContain(key):
    return MISS  # Definitely not cached (true negative)

  # Bloom filter says "might be cached"
  if cache.contains(key):
    return HIT   # Actually cached
  else:
    return MISS  # False positive
```

**Trade-offs**:
- **Memory**: 8KB bit array (negligible)
- **False Positive Rate**: ~1% for 10K entries
- **Benefit**: Avoid ~99% of cache lookups for uncached keys

**Why Effective**:
- Negative cache is queried frequently for "not found" cases
- Most lookups are for keys that were never cached
- Bloom filter eliminates lock acquisition for these cases

---

## LRU Eviction Strategy

### Eviction Policy

**Trigger**: Cache size ≥ configured maximum

**Action**: Remove oldest entry (first in map)

**Rationale**:
- `absl::flat_hash_map` maintains insertion order
- First entry = least recently used (LRU)
- Simple, predictable, deterministic

### Eviction Thresholds

| Cache Level | Default Size | Eviction Trigger | Max Entry Size |
|-------------|--------------|------------------|----------------|
| Result      | 10,000       | ≥ 10,000        | 10 MB          |
| Validation  | 50,000       | ≥ 50,000        | 1 MB           |
| Negative    | 10,000       | ≥ 10,000        | N/A (boolean)  |

**Configuration Presets**:

```cpp
// High-throughput (memory-rich)
config.resultCacheSize = 50000;
config.validationCacheSize = 100000;
config.negativeCacheSize = 50000;

// Low-memory (embedded systems)
config.resultCacheSize = 1000;
config.validationCacheSize = 5000;
config.negativeCacheSize = 1000;
```

---

## Thread Safety

### Synchronization Primitives

All caches use `ad_utility::Synchronized<T, std::mutex>`:

```cpp
// Thread-safe cache access
auto lockPtr = resultCache_.wlock();  // Acquire write lock
auto it = lockPtr->find(key);         // Safe access
// ... operations under lock
lockPtr.unlock();                     // Explicit release (or RAII)
```

**Properties**:
- Exclusive write access (no concurrent modifications)
- Reader-writer locks NOT used (write-heavy workload)
- Fine-grained locking per cache level (not global lock)

### Concurrency Patterns

**Read-Compute-Write** (cache miss):
```cpp
1. Acquire lock
2. Check cache (miss)
3. RELEASE LOCK  ← Critical: don't hold lock during computation
4. Compute result (expensive, long-running)
5. Re-acquire lock
6. Insert result
7. Release lock
```

**Read-Only** (cache hit):
```cpp
1. Acquire lock
2. Check cache (hit)
3. Copy result
4. Release lock
5. Return result
```

**Bloom Filter Fast Path**:
```cpp
1. Acquire Bloom filter lock (cheap, no hash map access)
2. Check Bloom filter
3. Release Bloom filter lock
4. If "definitely not cached", return immediately
5. Otherwise, proceed to cache lookup
```

---

## Performance Characteristics

### Time Complexity

| Operation | Bloom Filter | Cache Lookup | Cache Insert |
|-----------|--------------|--------------|--------------|
| Best      | O(k)         | O(1)         | O(1)         |
| Average   | O(k)         | O(1)         | O(1)         |
| Worst     | O(k)         | O(n)         | O(n)         |

Where:
- k = number of hash functions (3)
- n = number of entries (hash collision case)

### Space Complexity

**Per Cache Entry**:
- Result cache: ~100 bytes overhead + result size
- Validation cache: ~80 bytes overhead + 1 byte (boolean)
- Negative cache: ~80 bytes overhead + 1 byte (boolean)

**Bloom Filter**: 8 KB fixed (independent of cache size)

**Total Memory** (default config):
```
Result cache:     10,000 × (100 B + avg result size)
Validation cache: 50,000 × 81 B = ~4 MB
Negative cache:   10,000 × 81 B = ~810 KB
Bloom filter:     8 KB

Total overhead: ~5 MB + result data
```

### Expected Hit Rates

**Workload**: SHACL validation on 100K resources with 10 shapes

| Cache Level | Hit Rate | Justification |
|-------------|----------|---------------|
| Result      | 60-80%   | Many resources validated multiple times |
| Validation  | 80-90%   | Constraint reuse across resources |
| Negative    | 90-95%   | Most resources don't match most shapes |

**Bloom Filter**:
- True Negative Rate: 99% (for uncached keys)
- False Positive Rate: 1% (at 10K entries)

---

## Integration with Existing QLever Infrastructure

### Reused Components

1. **`ad_utility::Synchronized<T, std::mutex>`** (from `util/Synchronized.h`)
   - Thread-safe wrapper for all caches
   - Provides `wlock()` for exclusive access

2. **`ad_utility::EpochManager`** (from `global/Epoch.h`)
   - Provides epoch lifecycle (INIT → INGEST → SEAL → SERVE)
   - Provides `getCurrentEpochId()` for cache key binding

3. **`absl::flat_hash_map`** (from Abseil)
   - Hash map implementation with insertion-order iteration
   - Enables simple LRU eviction (remove first element)

### Differences from ShaclValidationCache

| Aspect | ShaclValidationCache | UnifiedFormalismCache |
|--------|----------------------|------------------------|
| **Scope** | SHACL-specific | Generic for all formalisms |
| **Epoch Binding** | Implicit (via key) | Explicit (EpochBoundKey base class) |
| **Lifecycle** | Manual invalidation | EpochLifecycleManager |
| **Result Type** | ValidationResult (fixed) | Template parameter (generic) |
| **Compilation** | CompiledShape cache | No shape-specific caching |
| **Type Detection** | Type cache | Part of validation cache |

**Why Generic**:
- Datalog, N3, ShEx need same caching patterns
- Avoid code duplication across formalisms
- Unified metrics and observability

---

## Usage Examples

### SHACL Validation Caching

```cpp
// Create cache for SHACL validation results
using ShaclCache = formalism::UnifiedFormalismCache<shacl::ValidationResult>;
ShaclCache cache(CacheConfiguration::defaultConfig(), &epochManager);

// Load cache for current epoch (during SEAL phase)
auto epochId = cache.loadForEpoch();

// Validate resource (during SERVE phase)
formalism::ResultCacheKey key;
key.epochId = epochId;
key.guardIdentity = sha256(shape.serialize());
key.inputSignature = sha256(resourceUri);

auto result = cache.getOrComputeResult(key, [&]() {
  return validator.validate(resource, shape);
});

// When epoch changes (SERVE → INIT transition)
cache.invalidateAll();
```

### Datalog Rule Evaluation Caching

```cpp
// Create cache for Datalog query results
using DatalogCache = formalism::UnifiedFormalismCache<IdTable>;
DatalogCache cache(CacheConfiguration::highThroughputConfig(), &epochManager);

// Load for epoch
auto epochId = cache.loadForEpoch();

// Evaluate rule
formalism::ResultCacheKey key;
key.epochId = epochId;
key.guardIdentity = sha256(rule.serialize());
key.inputSignature = sha256(queryString);

auto result = cache.getOrComputeResult(key, [&]() {
  return fixpointComputation.evaluate(rule, query);
});
```

### Negative Lookup Caching (N3)

```cpp
// Check if resource does NOT support formulae
formalism::NegativeCacheKey key;
key.epochId = epochId;
key.guardIdentity = "N3ComplianceVerifier";
key.lookupSignature = "formulae_not_supported:" + resourceUri;

if (cache.isNegativeCached(key)) {
  // Fast path: we already know this resource doesn't support formulae
  return ComplianceReport::unsupported("formulae");
}

// Actually check (expensive)
bool supportsFormulae = checkFormulaeSupport(resource);

if (!supportsFormulae) {
  // Cache the negative result
  cache.addNegativeLookup(key);
}
```

---

## Observability and Metrics

### Exposed Metrics

```cpp
CacheMetrics metrics = cache.getMetrics();

// Cache hit rates
double resultHitRate = metrics.getResultHitRate();      // 0.0-1.0
double validationHitRate = metrics.getValidationHitRate();
double negativeHitRate = metrics.getNegativeHitRate();

// Detailed counts
uint64_t resultHits = metrics.resultCacheHits.load();
uint64_t resultMisses = metrics.resultCacheMisses.load();
uint64_t resultEvictions = metrics.resultCacheEvictions.load();

// Bloom filter effectiveness
uint64_t falsePositives = metrics.bloomFilterFalsePositives.load();
uint64_t trueNegatives = metrics.bloomFilterTrueNegatives.load();

// Epoch events
uint64_t invalidations = metrics.epochInvalidations.load();
uint64_t checkpoints = metrics.epochCheckpoints.load();

// Timing (microseconds)
uint64_t totalLookupTime = metrics.totalLookupTime.load();
uint64_t totalInsertTime = metrics.totalInsertTime.load();
```

### Formatted Output

```cpp
std::cout << metrics.toString();
```

Output:
```
Cache Metrics:
  Result Cache:     8523 hits, 1477 misses, 234 evictions (hit rate: 85.23%)
  Validation Cache: 45231 hits, 4769 misses, 1023 evictions (hit rate: 90.46%)
  Negative Cache:   9832 hits, 168 misses, 0 evictions (hit rate: 98.32%)
  Bloom Filter:     42 false positives, 9790 true negatives
  Epoch Events:     3 invalidations, 0 checkpoints
  Timing:           12345 μs lookup, 6789 μs insert
```

---

## Future Extensions

### Planned Features

1. **Cache Checkpointing** (EPIC 14.2):
   - Serialize cache to disk during checkpoint
   - Restore cache on startup (warm cache)
   - Deterministic deserialization from epoch manifest

2. **Adaptive Sizing** (EPIC 14.3):
   - Monitor hit rates and adjust cache sizes dynamically
   - Grow validation cache if hit rate > 95%
   - Shrink result cache if hit rate < 50%

3. **Cross-Formalism Caching** (EPIC 14.4):
   - Detect when Datalog rule subsumes SHACL shape
   - Reuse Datalog results for SHACL validation
   - Unified guard identity for equivalent constraints

4. **Distributed Caching** (EPIC 15):
   - Share cache across QLever instances
   - Redis backend for cache persistence
   - Cache coherence protocol for multi-node

### Non-Goals (Explicit)

1. **Persistent Cache Across Restarts**:
   - Cache is epoch-bound; epochs don't survive restarts
   - Checkpointing is for within-epoch recovery only

2. **LRU-K or Advanced Eviction**:
   - Simple LRU is deterministic and sufficient
   - No LIRS, ARC, or other complex algorithms

3. **Cache Warming**:
   - No pre-population of cache
   - Cache warms naturally during SERVE phase

---

## Testing Strategy

### Unit Tests

1. **Cache Operations**:
   - Insert, lookup, eviction
   - Thread safety (concurrent access)
   - Bloom filter false positive rate

2. **Epoch Lifecycle**:
   - load(), invalidate(), checkpoint()
   - Epoch ID binding correctness
   - Invalidation on epoch change

3. **LRU Eviction**:
   - Oldest entry removed first
   - Eviction count tracking
   - Cache size limits respected

### Integration Tests

1. **SHACL Integration**:
   - Replace ShaclValidationCache with UnifiedFormalismCache
   - Verify identical behavior
   - Compare performance metrics

2. **Datalog Integration**:
   - Cache Datalog rule evaluation results
   - Verify cache invalidation on rule update
   - Measure hit rate improvement

3. **Cross-Formalism**:
   - SHACL + Datalog in same epoch
   - Verify independent cache keys
   - No cache pollution between formalisms

### Benchmarks

1. **Cache Hit Performance**:
   - Measure cache lookup latency (μs)
   - Compare with uncached baseline
   - Target: <10 μs per lookup

2. **Bloom Filter Effectiveness**:
   - Measure true negative rate
   - Measure false positive rate
   - Target: >99% true negative, <1% false positive

3. **Memory Overhead**:
   - Measure per-entry overhead
   - Measure total cache size
   - Target: <5 MB overhead for default config

---

## Determinism Checklist

✅ **Cache Keys are Deterministic**:
- Epoch ID is monotonically increasing counter
- Guard identity is SHA-256 hash of guard definition
- Input signature is SHA-256 hash of input data

✅ **Eviction is Deterministic**:
- LRU eviction removes first element (insertion order)
- Eviction triggered at exact threshold (not probabilistic)
- No random sampling or probabilistic algorithms

✅ **Thread Safety is Deadlock-Free**:
- No nested locks
- Locks released before expensive computation
- No lock ordering dependencies

✅ **Epoch Binding is Enforced**:
- All cache keys include epoch ID
- Invalidation clears all epochs (no partial clearing)
- No cross-epoch cache hits possible

✅ **Metrics are Monotonic**:
- All counters are `std::atomic<uint64_t>` (no overflow)
- Counters only increment (never decrement)
- Reset only via explicit `resetMetrics()` call

---

## References

1. **QLever Epoch System**: `src/global/Epoch.cpp`, `src/global/Epoch.h`
2. **SHACL Cache**: `src/engine/shacl/ShaclValidationCache.h`
3. **Negative Cache**: `src/engine/readCache/NegativeCache.h`
4. **LRU Cache**: `src/util/LruCache.h`
5. **Concurrent Cache**: `src/util/ConcurrentCache.h`
6. **EPIC 14.0 Audit**: `audit/FORMALISM_BEST_OF.md`

---

**End of Design Document**

# SingleFlight Quick Reference Guide

## What is SingleFlight?

SingleFlight is a cache stampede prevention utility that ensures only one thread computes an expensive result for a given key, even when multiple threads request it concurrently.

---

## Basic Usage

```cpp
#include "util/SingleFlight.h"

// Create a SingleFlight instance
ad_utility::SingleFlight<std::string, ExpensiveResult> sf;

// Use it to prevent stampedes
auto result = sf.getOrCompute("key", []() {
  // This expensive computation runs only once
  // even if 100 threads request "key" simultaneously
  return computeExpensiveResult();
});
```

---

## Common Patterns

### Pattern 1: Cache Integration

```cpp
class MyCache {
  ad_utility::SingleFlight<KeyType, ResultType> singleFlight_;
  std::map<KeyType, ResultType> cache_;

  ResultType get(const KeyType& key) {
    // Check cache first
    if (cache_.contains(key)) {
      return cache_[key];
    }

    // Use SingleFlight to prevent stampede
    auto result = singleFlight_.getOrCompute(key, [this, &key]() {
      return expensiveComputation(key);
    });

    // Cache the result
    cache_[key] = result;
    return result;
  }
};
```

### Pattern 2: Query Compilation

```cpp
class PlanCache {
  ad_utility::SingleFlight<QueryKey, CompiledPlan> singleFlight_;

  CompiledPlan compile(const QueryKey& key) {
    return singleFlight_.getOrCompute(key, [this, &key]() {
      // Expensive compilation happens only once
      return compileQueryPlan(key);
    });
  }
};
```

### Pattern 3: Serialization

```cpp
class BytesCache {
  ad_utility::SingleFlight<CacheKey, SerializedBytes> singleFlight_;

  SerializedBytes serialize(const CacheKey& key, const Data& data) {
    return singleFlight_.getOrCompute(key, [&data]() {
      // Expensive serialization happens only once
      return serializeToBytes(data);
    });
  }
};
```

---

## API Reference

### Constructor
```cpp
SingleFlight<KeyT, ValueT>()
```
Creates a new SingleFlight instance.

### getOrCompute
```cpp
ValueT getOrCompute(const KeyT& key, std::function<ValueT()> computeFn)
```
Gets the result for `key`, computing it if necessary.
- **First thread**: Computes the result by calling `computeFn()`
- **Other threads**: Wait for the first thread and receive the same result
- **Exceptions**: If `computeFn()` throws, all waiting threads see the exception

### clear
```cpp
void clear()
```
Clears all in-flight computations. Primarily for testing or epoch boundaries.

### inFlightCount
```cpp
size_t inFlightCount() const
```
Returns the number of computations currently in-flight. Useful for monitoring.

---

## Key Behaviors

### ✅ What SingleFlight DOES

- **Prevents duplicate computation**: Only one thread computes per key
- **Coordinates threads**: Other threads wait efficiently on condition variable
- **Propagates exceptions**: If computation fails, all waiters see the exception
- **Cleans up automatically**: In-flight entries removed after completion
- **Thread-safe**: Multiple threads can call concurrently

### ❌ What SingleFlight DOES NOT DO

- **Does NOT cache results**: Each new call may compute again (use with a cache)
- **Does NOT timeout**: Waiting threads wait indefinitely (add timeout if needed)
- **Does NOT cancel**: Computations run to completion (integrate CancellationHandle if needed)

---

## Performance Characteristics

### Lock Contention
- **Minimal**: Lock held only for map operations (nanoseconds)
- **Computation happens outside lock**: Scales horizontally

### Memory Overhead
- **Per in-flight key**: ~64 bytes (mutex + condition_variable + shared_ptr)
- **Total**: O(k) where k = number of unique keys being computed

### Time Saved
- **Without SingleFlight**: N threads × compute_time
- **With SingleFlight**: 1 × compute_time + (N-1) × wait_time
- **Savings**: ~(N-1) × compute_time

---

## Example Scenarios

### Scenario 1: Query Stampede
```
Without SingleFlight:
  - 20 users submit same query
  - 20 parallel compilations (20 × 500ms = 10,000ms total CPU)
  - All complete around 500ms (parallel execution)

With SingleFlight:
  - 20 users submit same query
  - 1 compilation (1 × 500ms = 500ms total CPU)
  - All complete around 500ms (19 wait, 1 computes)
  - Savings: 9,500ms of CPU time
```

### Scenario 2: Cache Miss Avalanche
```
Without SingleFlight:
  - Cache evicts popular item
  - 100 threads request it simultaneously
  - 100 parallel deserializations
  - Possible OOM or performance degradation

With SingleFlight:
  - Cache evicts popular item
  - 100 threads request it simultaneously
  - 1 deserialization, 99 wait
  - Controlled resource usage
```

---

## Type Requirements

### KeyT Requirements
- Must be hashable (for std::unordered_map)
- Must support equality comparison
- Examples: int, string, custom types with hash/equals

### ValueT Requirements
- Must be copyable or movable
- Must be constructible from computeFn() return type
- Examples: primitives, strings, structs, shared_ptr, etc.

---

## Error Handling

```cpp
try {
  auto result = sf.getOrCompute("key", []() -> int {
    // Computation fails
    throw std::runtime_error("Failed to compute");
  });
} catch (const std::runtime_error& e) {
  // All waiting threads catch this exception
  std::cerr << "Computation failed: " << e.what() << std::endl;
}
```

**Guarantees:**
- If `computeFn()` throws, ALL waiting threads see the exception
- In-flight map is cleaned up automatically
- Future calls will retry the computation (not cached)

---

## Thread Safety

### Safe Concurrent Operations
- ✅ Multiple threads calling `getOrCompute()` with same key
- ✅ Multiple threads calling `getOrCompute()` with different keys
- ✅ Calling `getOrCompute()` while another thread calls `clear()`
- ✅ Calling `inFlightCount()` while computations are in progress

### Unsafe Operations
- ❌ None - all operations are thread-safe

---

## Monitoring and Debugging

### Check In-Flight Computations
```cpp
size_t count = sf.inFlightCount();
if (count > 0) {
  LOG(INFO) << "Currently computing " << count << " unique keys";
}
```

### Measure Stampede Prevention
```cpp
std::atomic<int> computeCount{0};

auto result = sf.getOrCompute("key", [&computeCount]() {
  computeCount++;  // Should only increment once
  return expensiveOp();
});

if (computeCount == 1) {
  LOG(INFO) << "Stampede prevented!";
}
```

---

## Integration Checklist

When integrating SingleFlight into a cache:

1. ✅ Add `SingleFlight<KeyT, ValueT>` member to cache class
2. ✅ Wrap expensive computation in `getOrCompute()`
3. ✅ Consider whether to cache the result (SingleFlight doesn't)
4. ✅ Handle exceptions from `computeFn()`
5. ✅ Add metrics to track stampede prevention
6. ✅ Test with concurrent access (20+ threads)
7. ✅ Verify compute function called exactly once

---

## Common Pitfalls

### ❌ Pitfall 1: Expecting Caching
```cpp
// WRONG: SingleFlight doesn't cache
auto r1 = sf.getOrCompute("key", compute);  // Computes
auto r2 = sf.getOrCompute("key", compute);  // Computes AGAIN

// CORRECT: Use with a cache
if (!cache.contains(key)) {
  cache[key] = sf.getOrCompute(key, compute);
}
```

### ❌ Pitfall 2: Different Functions for Same Key
```cpp
// WRONG: Assumes function is the same
auto r1 = sf.getOrCompute("key", []() { return 1; });
auto r2 = sf.getOrCompute("key", []() { return 2; });  // Still calls first function

// CORRECT: SingleFlight doesn't verify functions match
// The first thread's function is what executes
```

### ❌ Pitfall 3: Holding Locks During Compute
```cpp
// WRONG: Deadlock risk
std::mutex m;
sf.getOrCompute("key", [&m]() {
  std::lock_guard lock(m);  // Deadlock if multiple threads
  return compute();
});

// CORRECT: Ensure computeFn is lock-free
sf.getOrCompute("key", []() {
  return compute();  // No external locks
});
```

---

## Testing Examples

### Test Stampede Prevention
```cpp
TEST(MyCache, stampedeTest) {
  MyCache cache;
  std::atomic<int> computeCount{0};

  std::vector<std::thread> threads;
  for (int i = 0; i < 20; i++) {
    threads.emplace_back([&]() {
      cache.getWithCompute("key", [&computeCount]() {
        computeCount++;
        return "result";
      });
    });
  }

  for (auto& t : threads) t.join();

  ASSERT_EQ(computeCount, 1);  // Only computed once!
}
```

---

## When to Use SingleFlight

### ✅ Use When:
- Multiple threads might request the same expensive computation
- Computation is idempotent (same inputs → same outputs)
- You want to prevent resource exhaustion from stampedes
- Waiting for a shared result is acceptable

### ❌ Don't Use When:
- Computation is cheap (< 1ms)
- Results vary per thread (non-deterministic)
- You need immediate individual computations
- Blocking is unacceptable

---

## Summary

SingleFlight is a simple but powerful utility for preventing cache stampedes. Use it whenever:
1. Multiple threads might request the same expensive result
2. You want to avoid duplicate computation
3. Thread coordination is acceptable

**Remember:** SingleFlight prevents stampedes, but doesn't cache results. Combine it with a cache for full effectiveness.

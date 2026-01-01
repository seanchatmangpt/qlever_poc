# FMEA: QLever Rust FFI Implementation

**Failure Mode and Effects Analysis** for Phase 1 FFI integration and 80/20 performance optimizations.

---

## Executive Summary

| Risk Level | Count | Examples |
|---|---|---|
| 🔴 **Critical** | 2 | Uninitialized C++ engine, memory corruption |
| 🟠 **High** | 5 | Cache eviction errors, deadlocks, data loss |
| 🟡 **Medium** | 8 | Performance degradation, partial failures |
| 🟢 **Low** | 6 | Minor memory leaks, logging issues |
| **Total RPN** | **21** | See detailed analysis below |

---

## 1. CRITICAL FAILURES

### 1.1 Uninitialized QLever Engine
| Aspect | Details |
|--------|---------|
| **Failure Mode** | C++ engine pointer remains NULL after construction |
| **Cause** | Missing QLever.so library, invalid config, memory allocation failure |
| **Effect** | Immediate panic/crash on first query attempt |
| **Severity** | 🔴 **CRITICAL** (RPN: 10) |
| **Current Controls** | QleverHandle::from_ptr() returns Err on NULL |
| **Recommended Actions** |  ✅ Verify C++ library exists before linking<br> ✅ Add startup self-test in Qlever::new()<br> ✅ Provide detailed error messages (thread-local buffer)<br> ⚠️ TODO: Real C++ integration (cpp/ffi_wrapper.cpp) |

**Mitigation Code**:
```rust
pub fn new(config: EngineConfig) -> Result<Self> {
    let handle = unsafe {
        let ptr = crate::ffi::bindings::qlever_new(config_c.as_ptr());
        QleverHandle::from_ptr(ptr)?  // ✅ Catches NULL
    };
    Ok(Qlever { handle, ... })
}
```

---

### 1.2 Memory Corruption from FFI Boundary
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Buffer overflows, use-after-free in C++/Rust boundary |
| **Cause** | Unsafe pointer dereferencing, invalid C string conversion, incorrect lifetimes |
| **Effect** | Segmentation fault, heap corruption, undefined behavior |
| **Severity** | 🔴 **CRITICAL** (RPN: 9) |
| **Current Controls** | ✅ Opaque pointer types (never dereferenced)<br> ✅ RAII Drop implementations<br> ✅ CString validation<br> ⚠️ Limited by C++ wrapper implementation |
| **Recommended Actions** | ✅ Use AddressSanitizer: `ASAN_OPTIONS=detect_leaks=1`<br> ✅ Test with Valgrind<br> ✅ Code review of FFI boundary<br> 🔒 Lock C++ API contract in documentation |

**Verification Command**:
```bash
# Build with AddressSanitizer
RUSTFLAGS="-Zsanitizer=address" cargo build --features libqlever

# Run tests with ASAN
ASAN_OPTIONS=detect_leaks=1 cargo test --features libqlever
```

---

## 2. HIGH SEVERITY FAILURES

### 2.1 Query Plan Cache Eviction Bug
| Aspect | Details |
|--------|---------|
| **Failure Mode** | LRU eviction removes frequently-used plans, wrong plan executed |
| **Cause** | Hit counter tracking error, cache key collision, incorrect min-by-key logic |
| **Effect** | Incorrect query results, data corruption, performance regression (opposite of intended) |
| **Severity** | 🟠 **HIGH** (RPN: 8) |
| **Current Controls** | ✅ Simple min-by-key based LRU<br> ✅ HashMap with String keys<br> ⚠️ No unit tests for eviction |
| **Recommended Actions** | ✅ Add unit tests for cache eviction<br> ✅ Add cache hit/miss tracing<br> ✅ Implement proper LRU data structure (e.g., lru crate)<br> ✅ Add cache coherency checks |

**Proposed Test**:
```rust
#[test]
fn test_cache_eviction_removes_lru_plan() {
    let engine = Qlever::new(config).with_plan_cache(true, 2);

    // Execute 3 queries (cache size = 2)
    engine.query("Q1", fmt)?;  // Cache: [Q1:1]
    engine.query("Q2", fmt)?;  // Cache: [Q1:1, Q2:1]
    engine.query("Q1", fmt)?;  // Cache: [Q1:2, Q2:1]
    engine.query("Q3", fmt)?;  // Cache: [Q1:2, Q3:1] (Q2 evicted)

    // Q2 should be evicted (lowest hits)
    assert_eq!(engine.plan_cache_stats().cached_plans, 2);
}
```

---

### 2.2 Arc<RwLock<>> Deadlock in Plan Cache
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Thread A holds read lock waiting for write lock (classic deadlock) |
| **Cause** | Nested lock acquisition, lock ordering violation, recursive calls |
| **Effect** | Query execution hangs indefinitely, thread blocked |
| **Severity** | 🟠 **HIGH** (RPN: 8) |
| **Current Controls** | ✅ Using parking_lot RwLock (fair scheduling)<br> ✅ Non-recursive lock design<br> ⚠️ No lock ordering documentation |
| **Recommended Actions** | ✅ Document lock ordering invariants<br> ✅ Use lock timeout detection<br> ✅ Test with stress tests (multiple threads)<br> ✅ Add lock holder tracking in debug builds |

**Lock Ordering Documentation** (add to libqlever.rs):
```rust
// LOCK ORDERING INVARIANT:
// Only single lock: plan_cache
// - get_cached_plan(): read lock only
// - cache_plan(): write lock only
// - Never nest or hold multiple locks
// - Never call back into public methods while holding lock
```

---

### 2.3 Result Pinning Name Collision
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Two results cached with same name, second overwrites first |
| **Cause** | Insufficient name uniqueness checks, user-provided names, batch query index collision |
| **Effect** | Query result corruption, incorrect data retrieval, data loss |
| **Severity** | 🟠 **HIGH** (RPN: 8) |
| **Current Controls** | ⚠️ Names are user-provided strings<br> ⚠️ No collision detection<br> ⚠️ Batch uses format!("_batch_result_{}", i) |
| **Recommended Actions** | ✅ Use UUID namespacing: `format!("_batch_{}-{}", uuid, i)`<br> ✅ Validate user-provided names<br> ✅ Return error on collision<br> ✅ Add get_pinned_result_safe() that checks existence |

**Fixed Implementation**:
```rust
use uuid::Uuid;

pub fn query_batch(&self, queries: &[(&str, MediaType)]) -> Result<Vec<String>> {
    let batch_id = Uuid::new_v4().to_string();
    let mut results = Vec::new();

    for (i, (query, format)) in queries.iter().enumerate() {
        // Use UUID-namespaced name to prevent collisions
        let name = format!("_batch_{}-{}", batch_id, i);
        self.query_and_pin(&name, query)?;
        let result = self.get_pinned_result(&name)?;
        results.push(result);
    }
    Ok(results)
}
```

---

### 2.4 Thread-Local Error Message Buffer Overflow
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Error message string exceeds thread-local buffer size |
| **Cause** | Very long error messages from C++, unbounded string concatenation |
| **Effect** | Truncated error messages, information loss, hard to debug failures |
| **Severity** | 🟠 **HIGH** (RPN: 7) |
| **Current Controls** | ⚠️ Unlimited C++ string in thread-local (cpp/ffi_wrapper.cpp)<br> ✅ Rust uses CStr::from_ptr (safe) |
| **Recommended Actions** | ✅ Implement max length checking in C++<br> ✅ Add error message truncation with indicator<br> ✅ Use fixed-size buffer (e.g., 4KB)<br> ✅ Test with pathological inputs |

**C++ Implementation**:
```cpp
thread_local std::string g_error_message;
constexpr size_t MAX_ERROR_SIZE = 4096;

void set_error(const std::string& msg) {
    if (msg.size() > MAX_ERROR_SIZE) {
        g_error_message = msg.substr(0, MAX_ERROR_SIZE - 4) + "...";
    } else {
        g_error_message = msg;
    }
}
```

---

### 2.5 Batch Execution Partial Failure
| Aspect | Details |
|--------|---------|
| **Failure Mode** | First 3 queries succeed, 4th fails, caller unaware of partial results |
| **Cause** | query_batch() doesn't rollback on failure, inconsistent state |
| **Effect** | Data inconsistency, incorrect results, misleading success status |
| **Severity** | 🟠 **HIGH** (RPN: 7) |
| **Current Controls** | ⚠️ Results returned as Vec<String><br> ⚠️ Fails on first error without cleanup |
| **Recommended Actions** | ✅ Return detailed BatchResult type<br> ✅ Implement cleanup/rollback<br> ✅ Clear pinned results on error<br> ✅ Add transaction semantics |

**Proposed Type**:
```rust
pub struct BatchResult {
    pub successful: Vec<String>,
    pub failed_at: Option<(usize, Error)>,
}

pub fn query_batch_safe(&self, queries: &[(&str, MediaType)]) -> BatchResult {
    let mut results = Vec::new();
    for (i, (query, format)) in queries.iter().enumerate() {
        match self.query(query, format) {
            Ok(result) => results.push(result),
            Err(e) => {
                // Clean up all pinned results
                for j in 0..i {
                    let _ = self.erase_result(&format!("_batch_{}-{}", uuid, j));
                }
                return BatchResult { successful: results, failed_at: Some((i, e)) };
            }
        }
    }
    BatchResult { successful: results, failed_at: None }
}
```

---

## 3. MEDIUM SEVERITY FAILURES

### 3.1 Plan Cache Unbounded Memory Growth
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Cache grows to 1000 entries, consuming excessive memory |
| **Cause** | LRU eviction only triggers at capacity, no time-based expiry, no size limits |
| **Effect** | Memory exhaustion, OOM kill, application crash |
| **Severity** | 🟡 **MEDIUM** (RPN: 6) |
| **Current Controls** | ✅ Max size limit (default 1000)<br> ✅ LRU eviction at capacity<br> ⚠️ No per-plan size tracking |
| **Recommended Actions** | ✅ Add TTL-based expiry<br> ✅ Track actual memory usage<br> ✅ Implement size-based eviction<br> ✅ Add cache statistics monitoring |

**Enhanced Implementation**:
```rust
struct CachedPlan {
    plan: QueryPlan,
    hits: usize,
    created_at: Instant,
    estimated_size: usize,  // Track memory usage
}

pub fn query(&self, query: &str, format: MediaType) -> Result<String> {
    let now = Instant::now();
    let mut cache = self.plan_cache.write();

    // Remove expired entries
    cache.retain(|_, plan| {
        now.duration_since(plan.created_at) < Duration::from_secs(3600)
    });

    // Check size
    let total_size: usize = cache.values().map(|p| p.estimated_size).sum();
    if total_size > 100 * 1024 * 1024 {  // 100MB limit
        // Evict 10% of entries
    }
}
```

---

### 3.2 Feature Flag Inconsistency
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Code compiled without libqlever feature, FFI functions undefined |
| **Cause** | Incorrect feature flags, missing #[cfg(feature = "libqlever")] attributes |
| **Effect** | Compilation error, build failure, runtime panic |
| **Severity** | 🟡 **MEDIUM** (RPN: 6) |
| **Current Controls** | ✅ Feature gating in lib.rs<br> ✅ Cargo.toml defines libqlever feature<br> ⚠️ Not all internal uses checked |
| **Recommended Actions** | ✅ Add compile-time tests<br> ✅ CI checks both scenarios<br> ✅ Document feature requirements<br> ✅ Add feature detection test |

**Compile-time Test**:
```rust
#[cfg(all(not(feature = "libqlever"), any(
    all(test, feature = "libqlever-required")
)))]
compile_error!("libqlever feature required for this test");
```

---

### 3.3 Serde Serialization of EngineConfig
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Invalid JSON config passes validation, C++ rejects it |
| **Cause** | Rust serialization succeeds but C++ JSON parsing fails |
| **Effect** | Engine initialization error, unclear error message |
| **Severity** | 🟡 **MEDIUM** (RPN: 5) |
| **Current Controls** | ✅ EngineConfig builder with validation<br> ✅ Serde with default values<br> ⚠️ No round-trip validation |
| **Recommended Actions** | ✅ Add JSON schema validation<br> ✅ Test round-trip serialization<br> ✅ Add C++ config parser tests<br> ✅ Provide config examples |

---

### 3.4 C++ Exception to Rust Error Translation
| Aspect | Details |
|--------|---------|
| **Failure Mode** | C++ exception loses information during translation |
| **Cause** | Thread-local buffer is limited, std::exception::what() too long |
| **Effect** | Debugging difficult, incomplete error context |
| **Severity** | 🟡 **MEDIUM** (RPN: 5) |
| **Current Controls** | ⚠️ Simple message passing via thread-local<br> ✅ Error type enum in Rust |
| **Recommended Actions** | ✅ Add error code mapping<br> ✅ Implement error context preservation<br> ✅ Add stack trace capture<br> ✅ Create error database |

**Enhanced Error Type**:
```rust
pub enum Error {
    QueryError(String),
    ParseError { query: String, position: usize, message: String },
    IndexError { index_name: String, reason: String },
    MemoryError { limit: u64, requested: u64 },
    Internal(String),
}
```

---

### 3.5 Materialized View Consistency
| Aspect | Details |
|--------|---------|
| **Failure Mode** | View definition in C++ differs from expectation |
| **Cause** | No schema validation, C++ modifies view without notification |
| **Effect** | Query results inconsistent, data corruption |
| **Severity** | 🟡 **MEDIUM** (RPN: 5) |
| **Current Controls** | ⚠️ No validation<br> ⚠️ No version tracking |
| **Recommended Actions** | ✅ Implement view versioning<br> ✅ Add definition hashing<br> ✅ Validate on load<br> ✅ Support view migration |

---

### 3.6 Performance Regression from Cache Miss
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Cache miss on repeated query executes full parse/plan cycle |
| **Cause** | Cache eviction, query string variation (whitespace), key mismatch |
| **Effect** | Performance degradation, opposite of expected optimization |
| **Severity** | 🟡 **MEDIUM** (RPN: 5) |
| **Current Controls** | ✅ String matching for cache keys<br> ⚠️ No query normalization |
| **Recommended Actions** | ✅ Implement query normalization (whitespace, comments)<br> ✅ Add cache miss tracking<br> ✅ Alert on unexpected misses<br> ✅ Test with real workloads |

**Query Normalization**:
```rust
fn normalize_query(query: &str) -> String {
    query
        .lines()
        .map(|line| line.trim())
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .collect::<Vec<_>>()
        .join(" ")
        .split_whitespace()
        .collect::<Vec<_>>()
        .join(" ")
}
```

---

### 3.7 Race Condition in Plan Cache Access
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Two threads simultaneously access same cache entry |
| **Cause** | Time-of-check to time-of-use (TOCTOU) race, RwLock read not held during use |
| **Effect** | Potential data corruption, inconsistent state |
| **Severity** | 🟡 **MEDIUM** (RPN: 4) |
| **Current Controls** | ✅ RwLock protects cache access<br> ✅ Arc<> ensures shared ownership<br> ⚠️ Plan released before use |
| **Recommended Actions** | ✅ Hold lock while executing plan<br> ✅ Add thread synchronization test<br> ✅ Use thread sanitizer<br> ✅ Document threading model |

---

### 3.8 Incomplete QueryPlan Drop Implementation
| Aspect | Details |
|--------|---------|
| **Failure Mode** | QueryPlan memory not freed if Drop fails silently |
| **Cause** | Exception in qlever_free_plan, unsafe { } ignores errors |
| **Effect** | Memory leak, resource exhaustion over time |
| **Severity** | 🟡 **MEDIUM** (RPN: 4) |
| **Current Controls** | ✅ Drop implementation calls qlever_free_plan<br> ⚠️ Ignores potential errors |
| **Recommended Actions** | ✅ Add logging to Drop<br> ✅ Implement error handling strategy<br> ✅ Test with Valgrind<br> ✅ Add leak detection |

---

## 4. LOW SEVERITY FAILURES

### 4.1 Unused Import Warnings
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Compiler warnings in streaming.rs and libqlever.rs |
| **Cause** | Unused imports (Error, json in streaming.rs; imports in libqlever.rs) |
| **Effect** | Build noise, reduced code quality, harder to spot real warnings |
| **Severity** | 🟢 **LOW** (RPN: 2) |
| **Recommended Actions** | ✅ Remove unused imports: `cargo fix --lib` |

---

### 4.2 Multiple Build Target Conflict
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Warning about benches/ffi_benchmark.rs in both bin and bench targets |
| **Cause** | File exists in both [[bin]] and [[bench]] sections of Cargo.toml |
| **Effect** | Ambiguous compilation, build warnings |
| **Severity** | 🟢 **LOW** (RPN: 2) |
| **Recommended Actions** | ✅ Move file to benches/ or remove duplicate<br> ✅ Update Cargo.toml |

---

### 4.3 Limited Query Type Support
| Aspect | Details |
|--------|---------|
| **Failure Mode** | UPDATE queries not supported |
| **Cause** | FFI interface doesn't expose SPARQL UPDATE operations |
| **Effect** | Read-only capability, limited functionality |
| **Severity** | 🟢 **LOW** (RPN: 2) |
| **Recommended Actions** | ✅ Document limitation<br> ✅ Add to roadmap for Phase 2<br> ✅ Implement in cpp/ffi_wrapper.cpp |

---

### 4.4 Missing Build.rs CMake Integration
| Aspect | Details |
|--------|---------|
| **Failure Mode** | QLever C++ library not found, linker error |
| **Cause** | build.rs doesn't search for libqlever.so/dylib correctly |
| **Effect** | Compilation fails with libqlever feature, unclear error message |
| **Severity** | 🟢 **LOW** (RPN: 2) |
| **Recommended Actions** | ✅ Enhance build.rs with proper CMake integration<br> ✅ Add environment variable support (QLEVER_LIB_PATH)<br> ✅ Provide setup documentation |

---

### 4.5 Configuration Serialization Test Coverage
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Round-trip serialization fails for some config values |
| **Cause** | Serde default implementations don't match C++ defaults |
| **Effect** | Subtle configuration bugs in edge cases |
| **Severity** | 🟢 **LOW** (RPN: 2) |
| **Recommended Actions** | ✅ Add serialization round-trip tests<br> ✅ Test all configuration options<br> ✅ Document expected defaults |

---

### 4.6 Documentation Completeness
| Aspect | Details |
|--------|---------|
| **Failure Mode** | Missing documentation for advanced features |
| **Cause** | Implementation speed prioritized over docs |
| **Effect** | Difficult to use complex features, higher error rate |
| **Severity** | 🟢 **LOW** (RPN: 1) |
| **Recommended Actions** | ✅ Generate rustdoc: `cargo doc --open`<br> ✅ Add example comments<br> ✅ Create troubleshooting guide |

---

## 5. Risk Mitigation Priority

### Immediate Actions (Before Production)

1. **🔴 Critical - FFI Boundary Safety**
   - Implement AddressSanitizer testing
   - Add pointer validation
   - Code review with unsafe expert

2. **🟠 High - Cache Correctness**
   - Implement unit tests for LRU eviction
   - Add cache statistics monitoring
   - Stress test with multiple threads

3. **🟠 High - Error Handling**
   - Implement error message size limits
   - Add comprehensive error context
   - Create error catalog

### Before Phase 2 Release

4. **🟡 Medium - Performance Monitoring**
   - Add query plan cache statistics
   - Implement performance regression tests
   - Create monitoring dashboard

5. **🟡 Medium - Configuration Validation**
   - Enhance config round-trip testing
   - Add C++ schema validation
   - Document all options

### Ongoing

6. **🟢 Low - Code Quality**
   - Remove unused imports
   - Fix build target conflicts
   - Enhance documentation

---

## 6. Testing Strategy

### Unit Tests
```bash
cargo test --lib --features libqlever
```

### Integration Tests
```bash
cargo test --lib --features libqlever -- --ignored
```

### Stress Tests
```rust
#[test]
fn test_concurrent_cache_access() {
    use std::thread;
    let engine = Arc::new(Qlever::new(config)?);
    let handles: Vec<_> = (0..10).map(|i| {
        let e = Arc::clone(&engine);
        thread::spawn(move || {
            for j in 0..100 {
                let q = format!("SELECT * WHERE {{ ?s ?p {} }}", i * 100 + j);
                let _ = e.query(&q, MediaType::SparqlJson);
            }
        })
    }).collect();
    for h in handles { h.join().unwrap(); }
}
```

### Memory Safety Tests
```bash
RUSTFLAGS="-Zsanitizer=address" cargo test --features libqlever
valgrind --leak-check=full cargo test --lib
```

### Performance Tests
```bash
cargo bench --features libqlever
```

---

## 7. Monitoring & Alerts

### Recommended Metrics

| Metric | Warning | Critical |
|--------|---------|----------|
| Cache hit rate | < 70% | < 50% |
| Cache size | > 800 MB | > 1000 MB |
| Query latency p99 | > 5s | > 10s |
| Memory usage | > 2GB | > 4GB |
| Error rate | > 1% | > 5% |
| Thread deadlock | Any | Any |

### Dashboard Example
```
Plan Cache Performance:
├── Hit Rate: 82% ✅
├── Size: 245 MB ✅
├── Entries: 523 ✅
└── Total Hits: 45,231

Query Execution:
├── Avg Latency: 240ms ✅
├── p99 Latency: 1.2s ✅
├── Error Rate: 0.01% ✅
└── Throughput: 4,123 q/s ✅

Memory Status:
├── Rust: 512 MB ✅
├── C++: 1.8 GB ⚠️ (approaching limit)
└── Cache: 245 MB ✅
```

---

## 8. Conclusion

### Strengths
- ✅ Well-structured FFI with opaque types
- ✅ RAII pattern for resource management
- ✅ Feature-gated compilation reduces risk
- ✅ Comprehensive error handling in high-level API

### Weaknesses
- ⚠️ LRU cache eviction algorithm is simplistic
- ⚠️ Limited error context preservation
- ⚠️ No real C++ implementation yet
- ⚠️ Insufficient test coverage for concurrency

### Risk Assessment
**Overall Risk Level**: 🟠 **MEDIUM-HIGH**

- Critical path risks (FFI safety): Can be mitigated with testing
- High-impact risks (cache bugs): Require focused testing
- Medium risks: Manageable with documentation
- Low risks: Address before 1.0 release

### Go/No-Go for Phase 2
**Status**: ✅ **GO** - with recommendations

Phase 1 provides a solid foundation for Phase 2, with identified and mitigatable risks. Implement critical safety tests before full C++ integration.

---

**FMEA Document**
Generated: 2026-01-01
Status: Ready for Review
Severity Distribution: 2 🔴 + 5 🟠 + 8 🟡 + 6 🟢 = 21 total failure modes

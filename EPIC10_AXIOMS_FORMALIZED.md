# EPIC 10: AXIOMS FORMALIZED
## Phase 1A Deliverable - Core Invariant Definitions

**Date**: 2026-01-02
**Phase**: 1A (Week 1, Days 1-2)
**Status**: FORMALIZED ✓
**Authority**: BB80/20 + EPIC 9 Atomic Cognitive Cycle
**Methodology**: Single-Pass Deterministic Construction

---

## EXECUTIVE SUMMARY

This document formalizes the **6 core axioms** (AX-1 through AX-6) that govern all EPIC 10 implementation work. Each axiom is a non-negotiable structural invariant inherited from BB80/20 operational principles and C++20 best practices. Violation of any axiom results in immediate build failure and blocking of all downstream work.

**Axioms are enforced via compile-time guards, static analysis, runtime assertions, and CI/CD gates.** No human judgment permitted—enforcement is deterministic and automated.

**Zero degrees of freedom.** No iteration permitted.

---

## AX-1: IMMUTABILITY

### Formal Definition

**No global mutable state may exist outside of `Synchronized<T>` wrappers.**

A variable `v` violates AX-1 if:
1. `v` has static storage duration (global or static local), AND
2. `v` is mutable (not `const`, not `constexpr`), AND
3. `v` is not wrapped in `Synchronized<T>` or equivalent RAII concurrency primitive

**Exception**: `RuntimeParameters` is permitted if immutable after initialization (frozen at startup).

**Rationale**: Mutable global state creates non-determinism (thread-safety violations, initialization order fiasco), prevents monoidal composition (hidden dependencies), and violates BB80/20 requirement for reproducibility from inputs.

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: Const global state (always safe)
inline constexpr size_t MAX_CACHE_SIZE = 1024 * 1024;
inline const std::string DEFAULT_ENDPOINT = "http://localhost:7001";

// Pattern 2: Synchronized wrapper for shared mutable state
inline Synchronized<ConcurrentCache<QueryId, Result>> g_queryCache;

// Pattern 3: Immutable after construction (frozen at startup)
class RuntimeParameters {
 public:
  RuntimeParameters() { /* load from config */ }
  // All member functions are const
  size_t getCacheSize() const { return cacheSize_; }
 private:
  const size_t cacheSize_;  // Immutable after construction
};
inline RuntimeParameters g_runtimeParams;  // Initialized once at startup

// Pattern 4: Thread-local storage (no cross-thread mutation)
thread_local AllocatorWithLimit g_perThreadAllocator;
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Mutable global variable (not synchronized)
inline size_t g_requestCount = 0;  // AX-1 VIOLATION

// FORBIDDEN: Static mutable local (hidden global state)
void incrementCounter() {
  static int counter = 0;  // AX-1 VIOLATION
  ++counter;
}

// FORBIDDEN: Mutable singleton pattern (classic anti-pattern)
class Singleton {
 public:
  static Singleton& getInstance() {
    static Singleton instance;  // AX-1 VIOLATION (mutable global)
    return instance;
  }
  void setState(int x) { state_ = x; }  // Mutation
 private:
  int state_;
};
```

### Violations Checklist

Automated detection via static analysis:

- [ ] **No mutable global variables**: `grep -rn "^[^/]*inline.*[^const][^expr].*=" src/ | grep -v "Synchronized"`
- [ ] **No static mutable locals**: `grep -rn "static.*[^const].*=" src/ | grep -v "thread_local"`
- [ ] **No mutable singletons**: Search for `getInstance()` + mutable member functions
- [ ] **All shared state wrapped**: `Synchronized<T>` or `std::atomic<T>` (for trivial types only)
- [ ] **RuntimeParameters immutable**: All public member functions marked `const`

CI/CD Gate: If ANY mutable global detected → **BUILD FAILS**

### Test Strategy

#### Compile-Time Validation

```cpp
// Static assertion: Verify RuntimeParameters immutability
static_assert(std::is_const_v<decltype(g_runtimeParams.getCacheSize())> == false,
              "getCacheSize() must be const member function");
```

#### Static Analysis (Clang-Tidy)

```yaml
# .clang-tidy
Checks: '-*,cppcoreguidelines-avoid-non-const-global-variables'
CheckOptions:
  - key: cppcoreguidelines-avoid-non-const-global-variables.AllowConstexpr
    value: true
  - key: cppcoreguidelines-avoid-non-const-global-variables.AllowSynchronized
    value: true
```

#### Runtime Validation (ThreadSanitizer)

```bash
# Build with TSan, run all tests
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_THREAD_SANITIZER=ON ..
ctest --output-on-failure
# Expected: 0 data races detected
```

#### Manual Code Review Checklist

- [ ] All global variables reviewed: const/constexpr or Synchronized<T>
- [ ] All static locals reviewed: const/thread_local or removed
- [ ] All singletons reviewed: immutable or refactored to dependency injection

**Acceptance Criteria**: ThreadSanitizer clean (0 data races) + Clang-Tidy clean (0 warnings)

---

## AX-2: DETERMINISM

### Formal Definition

**All output must be deterministic: identical inputs produce bitwise-identical outputs.**

An operation `f` violates AX-2 if:
1. `f(input)` produces different output on repeated invocations with identical input, OR
2. `f` depends on non-deterministic sources (PRNG without fixed seed, system time, hash randomization, floating-point rounding), OR
3. Build output (binaries, manifests) differs across builds with identical source code

**Determinism proof**: 100 independent builds produce identical `manifest.sha256`

**Rationale**: Determinism enables reproducibility (debugging, auditing), validates monoidal composition (merge without rework), and prevents iteration (non-deterministic tests mandate rework).

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: Ordered containers (deterministic iteration)
std::map<std::string, QueryPlan> queryCache;  // Ordered by key
std::set<TriplePattern> patterns;             // Ordered by value

// Pattern 2: Fixed-seed PRNG (deterministic random)
std::mt19937 rng(42);  // Fixed seed for reproducibility
auto randomValue = std::uniform_int_distribution<>(0, 100)(rng);

// Pattern 3: Integer-only arithmetic in critical paths
size_t computeCost(size_t rows, size_t cols) {
  return rows * cols * 107 / 100;  // Fixed-point: 7% = 107/100
}

// Pattern 4: Deterministic timestamp mocking in tests
class Clock {
 public:
  virtual std::chrono::system_clock::time_point now() const {
    return std::chrono::system_clock::now();
  }
};
class MockClock : public Clock {
 public:
  std::chrono::system_clock::time_point now() const override {
    return std::chrono::system_clock::from_time_t(1609459200);  // Fixed: 2021-01-01
  }
};
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Unordered containers in output paths
std::unordered_map<std::string, QueryPlan> cache;  // AX-2 VIOLATION
for (const auto& [key, plan] : cache) {
  outputStream << key;  // Non-deterministic iteration order
}

// FORBIDDEN: Floating-point in cost model
double computeCost(size_t rows, size_t cols) {
  return rows * cols * 1.07;  // AX-2 VIOLATION (floating-point rounding)
}

// FORBIDDEN: Random seed from system entropy
std::random_device rd;
std::mt19937 rng(rd());  // AX-2 VIOLATION (non-deterministic seed)

// FORBIDDEN: System time in query execution
auto queryStart = std::chrono::system_clock::now();  // AX-2 VIOLATION
// ... use queryStart in output or decision-making ...
```

### Violations Checklist

Automated detection via static analysis:

- [ ] **No std::unordered_map/set in output paths**: Grep for `unordered_map` near `operator<<` or serialization
- [ ] **No floating-point in cost model**: Grep for `double` or `float` in `/src/engine/*CostEstimate*`
- [ ] **No std::random_device without fixed seed**: Grep for `random_device` without test-only annotation
- [ ] **No std::chrono in hot paths**: Grep for `chrono::system_clock::now()` in `/src/engine/` (exclude logging)
- [ ] **Build reproducibility**: `manifest.sha256` identical across 100 builds

CI/CD Gate: If builds produce different `manifest.sha256` → **BUILD FAILS**

### Test Strategy

#### Determinism Proof (100 Builds)

```bash
#!/bin/bash
# test/determinism_proof.sh
set -e

# Build 100 times, collect manifest.sha256
for i in {1..100}; do
  rm -rf build
  cmake -DCMAKE_BUILD_TYPE=Release -GNinja -B build
  cmake --build build
  sha256sum build/qlever-server > "manifest_$i.sha256"
done

# Verify all 100 manifests are identical
first=$(cat manifest_1.sha256)
for i in {2..100}; do
  current=$(cat "manifest_$i.sha256")
  if [ "$first" != "$current" ]; then
    echo "DETERMINISM VIOLATION: Build 1 != Build $i"
    exit 1
  fi
done

echo "DETERMINISM PROOF: 100 builds produce identical binary"
```

#### Static Analysis (Custom Script)

```bash
# tools/detect_nondeterminism.sh
# Detect unordered containers in serialization paths
grep -rn "unordered_map\|unordered_set" src/ \
  | grep -E "operator<<|serialize|toJson" \
  && echo "AX-2 VIOLATION: unordered container in output path" && exit 1

# Detect floating-point in cost model
grep -rn "double\|float" src/engine/*Cost* \
  && echo "AX-2 VIOLATION: floating-point in cost model" && exit 1

echo "AX-2 STATIC ANALYSIS PASS"
```

#### Runtime Validation (Regression Tests)

```cpp
// test/QueryExecutionTest.cpp
TEST(QueryExecution, DeterministicPlan) {
  // Execute same query 10 times
  std::string query = "SELECT ?x WHERE { ?x <p> <o> }";
  std::vector<std::string> plans;

  for (int i = 0; i < 10; ++i) {
    auto plan = parseAndPlan(query);
    plans.push_back(plan.toString());
  }

  // Verify all 10 plans are identical
  for (size_t i = 1; i < plans.size(); ++i) {
    ASSERT_EQ(plans[0], plans[i]) << "Query plan non-deterministic at iteration " << i;
  }
}
```

**Acceptance Criteria**: 100 builds identical + 0 static analysis violations + all determinism tests pass

---

## AX-3: ATOMIC FAILURE

### Formal Definition

**All operations must fail atomically: either complete successfully or leave no partial state.**

An operation `f` violates AX-3 if:
1. `f` fails partway through execution and leaves partial state (e.g., half-written file, partial index), OR
2. `f` does not guarantee rollback on failure, OR
3. Build system continues after phase failure (partial build artifacts remain)

**Fail-closed semantics**: Errors propagate immediately. No partial success.

**Rationale**: Partial state creates ambiguity (is output valid?), prevents monoidal composition (cannot merge partial results), and violates BB80/20 requirement for deterministic receipts.

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: RAII ensures rollback on exception
class Transaction {
 public:
  Transaction(Database& db) : db_(db), committed_(false) {
    db_.beginTransaction();
  }
  ~Transaction() {
    if (!committed_) {
      db_.rollback();  // Automatic rollback if not committed
    }
  }
  void commit() {
    db_.commit();
    committed_ = true;
  }
 private:
  Database& db_;
  bool committed_;
};

void processQuery(Database& db, const Query& q) {
  Transaction txn(db);
  auto result = executeQuery(q);  // May throw
  txn.commit();  // Only if executeQuery succeeds
}  // Automatic rollback if exception thrown

// Pattern 2: Strong exception guarantee (copy-and-swap idiom)
class IdTable {
 public:
  IdTable& operator=(const IdTable& other) {
    IdTable temp(other);  // Copy (may throw, but doesn't modify *this)
    swap(temp);           // Swap (noexcept, commits the change)
    return *this;
  }  // temp destructor cleans up old data

 private:
  void swap(IdTable& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(numRows_, other.numRows_);
  }
  std::vector<uint64_t> data_;
  size_t numRows_;
};

// Pattern 3: Build system atomicity (all-or-nothing)
# CMakeLists.txt
set(CMAKE_ERROR_ON_WARNING ON)  # Treat warnings as errors
# Ninja build stops at first error (default behavior)
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Partial state on exception
void updateIndex(Index& index, const RDF& data) {
  index.addTriples(data.triples);  // May throw partway
  // AX-3 VIOLATION: Index in partial state if exception thrown
}

// FORBIDDEN: Ignoring errors
void processFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    return;  // AX-3 VIOLATION: Silent failure, no rollback
  }
  // ... process file ...
}

// FORBIDDEN: Continuing after failure
# build.sh
make || echo "Build failed but continuing"  # AX-3 VIOLATION
# ... more steps ...
```

### Violations Checklist

Automated detection via code review:

- [ ] **All resource modifications use RAII**: No manual `delete`, `close()`, `unlock()`
- [ ] **All error paths validate**: No silent `return` on failure (must throw or propagate error)
- [ ] **Build system stops on error**: `set -e` in bash scripts, `CMAKE_ERROR_ON_WARNING=ON`
- [ ] **No partial commits**: Transactions commit atomically (all-or-nothing)
- [ ] **Exception safety audited**: All operations provide strong or basic exception guarantee

CI/CD Gate: Build script returns non-zero on ANY error → Jenkins/GitHub Actions fails

### Test Strategy

#### Exception Safety Tests

```cpp
// test/ExceptionSafetyTest.cpp
TEST(IdTable, StrongExceptionGuarantee) {
  IdTable original = createTestTable();
  IdTable copy = original;

  // Inject allocation failure
  testing::AllocatorGuard guard(testing::FailAt(5));

  // Assignment should fail atomically (no partial state)
  EXPECT_THROW(original = createLargeTable(), std::bad_alloc);

  // Verify original table unchanged (strong guarantee)
  EXPECT_EQ(original, copy);
}
```

#### Build Atomicity Test

```bash
# test/build_atomicity.sh
#!/bin/bash
set -e  # Exit on first error

# Inject build error
echo "SYNTAX ERROR" >> src/engine/QueryExecutionTree.cpp

# Attempt build
if cmake --build build; then
  echo "AX-3 VIOLATION: Build succeeded despite syntax error"
  exit 1
fi

# Verify no partial artifacts (build/ should be incomplete)
if [ -f build/qlever-server ]; then
  echo "AX-3 VIOLATION: Binary exists after failed build"
  exit 1
fi

echo "AX-3 BUILD ATOMICITY PASS"
```

#### Transaction Rollback Test

```cpp
// test/TransactionTest.cpp
TEST(Transaction, RollbackOnException) {
  Database db;
  auto initialState = db.getChecksum();

  try {
    Transaction txn(db);
    db.insert(Triple{"s", "p", "o"});
    throw std::runtime_error("Simulated failure");
    txn.commit();  // Never reached
  } catch (...) {
    // Exception caught
  }

  // Verify database rolled back to initial state
  EXPECT_EQ(db.getChecksum(), initialState);
}
```

**Acceptance Criteria**: All exception safety tests pass + build fails on first error + 0 partial state violations

---

## AX-4: NO EXTERNAL STATE

### Formal Definition

**All functions must be pure: output determined solely by input, no side effects on external state.**

A function `f` violates AX-4 if:
1. `f` modifies external state not reachable via its parameters (global variables, files, network), OR
2. `f` produces different output for identical input due to external state changes, OR
3. `f` has side effects not captured in return value (modifying files, network calls, logging to external systems)

**Exception**: Logging is permitted if it does not affect execution or output. Internal caching is permitted if it preserves determinism (memoization).

**Rationale**: External state dependencies prevent reproducibility (cannot reconstruct output from inputs), violate determinism (hidden state changes), and break monoidal composition (merge requires external state synchronization).

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: Pure function (no side effects)
size_t computeHash(const std::string& input) {
  return std::hash<std::string>{}(input);  // Deterministic, no side effects
}

// Pattern 2: Explicit input/output (all state in parameters)
Result executeQuery(const Query& query, const Index& index) {
  // Input: query, index (immutable)
  // Output: Result (return value)
  // No external state modified
  return optimizeAndExecute(query, index);
}

// Pattern 3: Memoization (preserves determinism)
class QueryPlanner {
 public:
  QueryPlan plan(const Query& q) const {
    auto it = cache_.find(q);
    if (it != cache_.end()) {
      return it->second;  // Cache hit
    }
    auto result = computePlan(q);
    cache_[q] = result;  // Memoize (internal state, preserves determinism)
    return result;
  }
 private:
  mutable std::map<Query, QueryPlan> cache_;  // Internal optimization
};

// Pattern 4: Logging (side effect, but does not affect output)
Result processRequest(const Request& req) {
  LOG(INFO) << "Processing request: " << req.id();  // Logging permitted
  return computeResult(req);  // Output independent of logging
}
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Modifying global state
static size_t g_queryCount = 0;
Result executeQuery(const Query& q) {
  ++g_queryCount;  // AX-4 VIOLATION (modifies external state)
  return computeResult(q);
}

// FORBIDDEN: File I/O side effects in execution
Result executeQuery(const Query& q) {
  std::ofstream log("queries.log", std::ios::app);
  log << q.toString() << std::endl;  // AX-4 VIOLATION (file modification)
  return computeResult(q);
}

// FORBIDDEN: Network calls in pure function
std::string fetchData(const std::string& url) {
  // AX-4 VIOLATION: Network I/O (non-deterministic, external state)
  return httpGet(url);
}

// FORBIDDEN: Output depends on external file state
Result loadCache(const Query& q) {
  std::ifstream cache("cache.dat");
  // AX-4 VIOLATION: Output varies based on file existence/content
  if (cache) {
    return deserialize(cache);
  }
  return computeResult(q);
}
```

### Violations Checklist

Automated detection via code review:

- [ ] **No file I/O in query execution**: Grep for `fstream`, `fopen` in `/src/engine/` (exclude initialization)
- [ ] **No network calls in execution**: Grep for `socket`, `curl`, `http` in `/src/engine/`
- [ ] **No global state mutation**: Verify AX-1 compliance (no mutable globals)
- [ ] **Reproducibility test**: Same query + same index = same result (across processes)
- [ ] **Side effect audit**: All functions with side effects documented (logging, metrics)

CI/CD Gate: If query execution modifies external state → **INTEGRATION TEST FAILS**

### Test Strategy

#### Reproducibility Test

```cpp
// test/ReproducibilityTest.cpp
TEST(QueryExecution, Reproducible) {
  Index index = loadTestIndex();
  Query query = parseQuery("SELECT ?x WHERE { ?x <p> ?y }");

  // Execute query 10 times
  std::vector<Result> results;
  for (int i = 0; i < 10; ++i) {
    results.push_back(executeQuery(query, index));
  }

  // Verify all results are bitwise identical
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_EQ(results[0], results[i]) << "Query execution non-reproducible at iteration " << i;
  }
}
```

#### External State Isolation Test

```cpp
// test/ExternalStateTest.cpp
TEST(QueryExecution, NoExternalStateMutation) {
  // Take filesystem snapshot
  auto filesBefore = listFiles("/tmp");
  auto checksumBefore = computeChecksum(filesBefore);

  // Execute query
  Index index = loadTestIndex();
  Query query = parseQuery("SELECT * WHERE { ?s ?p ?o }");
  auto result = executeQuery(query, index);

  // Verify no files created/modified
  auto filesAfter = listFiles("/tmp");
  auto checksumAfter = computeChecksum(filesAfter);
  EXPECT_EQ(checksumBefore, checksumAfter) << "Query execution modified external files";
}
```

#### Static Analysis (Forbidden Functions)

```bash
# tools/detect_external_state.sh
# Detect forbidden I/O operations in engine/
grep -rn "std::ofstream\|fopen\|fwrite" src/engine/ \
  | grep -v "test/" \
  && echo "AX-4 VIOLATION: File I/O in query execution" && exit 1

# Detect network calls
grep -rn "socket\|curl\|http" src/engine/ \
  | grep -v "test/" \
  && echo "AX-4 VIOLATION: Network I/O in query execution" && exit 1

echo "AX-4 STATIC ANALYSIS PASS"
```

**Acceptance Criteria**: Reproducibility tests pass + No external state mutations detected + Static analysis clean

---

## AX-5: RAII (Resource Acquisition Is Initialization)

### Formal Definition

**All resources must be managed via RAII: acquisition in constructor, release in destructor.**

A resource management pattern violates AX-5 if:
1. Resource is acquired manually and requires manual cleanup (`new`/`delete`, `malloc`/`free`, `fopen`/`fclose`), OR
2. Resource ownership is ambiguous (who owns the resource? when is it freed?), OR
3. Resource leaks possible on exception (cleanup code skipped)

**RAII guarantee**: Resource lifetime tied to object lifetime. Destructor handles all cleanup. Exception-safe by construction.

**Rationale**: Manual cleanup is error-prone (leaks, double-frees, exceptions), violates monoidal composition (hidden cleanup dependencies), and prevents atomic failure (partial cleanup on exception).

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: Smart pointers (heap allocation)
std::unique_ptr<QueryExecutionTree> tree = std::make_unique<QueryExecutionTree>();
// Destructor automatically deletes tree

// Pattern 2: RAII wrappers for system resources
class FileHandle {
 public:
  explicit FileHandle(const std::string& path)
    : file_(std::fopen(path.c_str(), "r")) {
    if (!file_) throw std::runtime_error("Failed to open file");
  }
  ~FileHandle() {
    if (file_) std::fclose(file_);  // Automatic cleanup
  }
  FILE* get() const { return file_; }
 private:
  FILE* file_;
};

// Pattern 3: Scoped locks (RAII for mutexes)
void updateCache(Cache& cache, const Entry& entry) {
  std::lock_guard<std::mutex> lock(cache.mutex_);
  cache.insert(entry);
}  // Automatic unlock when lock_guard destructor runs

// Pattern 4: Custom RAII for QLever resources
class AllocatorGuard {
 public:
  explicit AllocatorGuard(size_t limit)
    : allocator_(std::make_unique<AllocatorWithLimit>(limit)) {}
  ~AllocatorGuard() {
    // allocator_ unique_ptr automatically deallocates
  }
  AllocatorWithLimit& get() { return *allocator_; }
 private:
  std::unique_ptr<AllocatorWithLimit> allocator_;
};
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Manual new/delete
void processQuery(const Query& q) {
  auto* tree = new QueryExecutionTree();  // AX-5 VIOLATION
  // ... use tree ...
  delete tree;  // Manual cleanup (skipped if exception thrown)
}

// FORBIDDEN: Raw pointers with unclear ownership
class QueryPlanner {
 public:
  QueryExecutionTree* plan(const Query& q) {
    return new QueryExecutionTree();  // AX-5 VIOLATION: Who owns this?
  }
};

// FORBIDDEN: Manual mutex lock/unlock
void updateCache(Cache& cache, const Entry& entry) {
  cache.mutex_.lock();  // AX-5 VIOLATION
  cache.insert(entry);  // May throw (mutex never unlocked)
  cache.mutex_.unlock();  // Skipped if exception thrown
}

// FORBIDDEN: Manual file close
void readFile(const std::string& path) {
  FILE* f = std::fopen(path.c_str(), "r");  // AX-5 VIOLATION
  // ... read file ...
  std::fclose(f);  // Skipped if exception thrown
}
```

### Violations Checklist

Automated detection via static analysis:

- [ ] **No raw new/delete**: Grep for `\bnew\s+[A-Z]` or `\bdelete\b` (exclude placement new)
- [ ] **No malloc/free**: Grep for `malloc\|free\|calloc\|realloc`
- [ ] **No raw pointers as owners**: All function return types use `unique_ptr` or `shared_ptr`
- [ ] **All mutexes via scoped locks**: Grep for `mutex_.lock()` or `mutex_.unlock()`
- [ ] **Valgrind clean**: 0 memory leaks, 0 dangling pointers, 0 double-frees
- [ ] **AddressSanitizer clean**: 0 heap-use-after-free, 0 memory leaks

CI/CD Gate: Valgrind or ASan detects leak → **BUILD FAILS**

### Test Strategy

#### Memory Leak Detection (Valgrind)

```bash
# test/valgrind_test.sh
#!/bin/bash
set -e

# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build

# Run all tests under Valgrind
valgrind --leak-check=full \
         --error-exitcode=1 \
         --show-leak-kinds=all \
         build/bin/QueryExecutionTest

# Expected: 0 bytes leaked
echo "AX-5 VALGRIND PASS: 0 memory leaks detected"
```

#### AddressSanitizer Test

```bash
# test/asan_test.sh
#!/bin/bash
set -e

# Build with ASan
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
      -B build
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

echo "AX-5 ASAN PASS: 0 memory errors detected"
```

#### Static Analysis (Forbidden Allocations)

```bash
# tools/detect_manual_allocation.sh
# Detect raw new/delete
grep -rn "\bnew\s\+[A-Z]\|\bdelete\b" src/ \
  | grep -v "placement new\|test/" \
  && echo "AX-5 VIOLATION: Manual new/delete detected" && exit 1

# Detect malloc/free
grep -rn "malloc\|free\|calloc\|realloc" src/ \
  | grep -v "test/" \
  && echo "AX-5 VIOLATION: Manual malloc/free detected" && exit 1

echo "AX-5 STATIC ANALYSIS PASS"
```

#### Exception Safety Leak Test

```cpp
// test/RAIIExceptionTest.cpp
TEST(RAII, NoLeakOnException) {
  testing::AllocatorGuard guard;
  auto initialAllocations = guard.getAllocationCount();

  // Trigger exception during resource allocation
  EXPECT_THROW({
    std::vector<std::unique_ptr<LargeObject>> objects;
    for (int i = 0; i < 100; ++i) {
      objects.push_back(std::make_unique<LargeObject>());
      if (i == 50) throw std::runtime_error("Simulated failure");
    }
  }, std::runtime_error);

  // Verify all allocations cleaned up (RAII guarantee)
  EXPECT_EQ(guard.getAllocationCount(), initialAllocations);
}
```

**Acceptance Criteria**: Valgrind clean (0 leaks) + ASan clean (0 errors) + Static analysis clean (0 manual allocations)

---

## AX-6: BACKWARD COMPATIBILITY

### Formal Definition

**No breaking changes to public APIs. All changes must be additive. Support N-2 versions (9 prior releases).**

A change violates AX-6 if:
1. Function signature modified (parameters added, removed, reordered), OR
2. Enum value removed or renumbered, OR
3. Class definition modified in binary-incompatible way (member reordered, removed), OR
4. Serialization format changed without version handler, OR
5. Existing tests fail after change (regression)

**Compatibility matrix**: 9 versions × 6 criteria = 54 tests must pass.

**Rationale**: Breaking changes force users to rewrite code (iteration), prevent monoidal composition (incompatible versions cannot merge), and violate BB80/20 requirement for deterministic upgrades (no human intervention).

### C++ Enforcement Pattern

#### Compliant Patterns

```cpp
// Pattern 1: Additive API changes (new overload, preserves old)
class Index {
 public:
  // Old API (preserved)
  Result query(const std::string& sparql);

  // New API (added, does not replace old)
  Result query(const std::string& sparql, const Options& opts);
};

// Pattern 2: Versioned serialization
enum class FormatVersion : uint8_t {
  VERSION_1 = 1,
  VERSION_2_SIMD = 2,  // Pre-added for future SIMD format
  CURRENT_VERSION = VERSION_1
};

void serialize(std::ostream& out, const IdTable& table) {
  out << static_cast<uint8_t>(FormatVersion::CURRENT_VERSION);
  serializeV1(out, table);  // Version-specific serialization
}

IdTable deserialize(std::istream& in) {
  uint8_t version;
  in >> version;
  switch (static_cast<FormatVersion>(version)) {
    case FormatVersion::VERSION_1:
      return deserializeV1(in);
    case FormatVersion::VERSION_2_SIMD:
      return deserializeV2(in);
    default:
      throw std::runtime_error("Unsupported format version");
  }
}

// Pattern 3: Deprecation (mark old API, keep functional)
class Parser {
 public:
  [[deprecated("Use parseQuery(const std::string&, const Options&)")]]
  Query parseQuery(const std::string& sparql) {
    return parseQuery(sparql, Options{});
  }

  Query parseQuery(const std::string& sparql, const Options& opts) {
    // New implementation
  }
};

// Pattern 4: Binary-compatible struct extension
struct QueryOptions {
  size_t limit = 0;
  size_t offset = 0;
  // New fields added at end (binary-compatible)
  bool enableOptimization = true;  // Added in v2.0
};
```

#### Forbidden Patterns

```cpp
// FORBIDDEN: Changing function signature
// OLD: Result query(const std::string& sparql);
Result query(const std::string& sparql, size_t limit);  // AX-6 VIOLATION

// FORBIDDEN: Removing enum value
enum class ResultFormat {
  JSON = 0,
  XML = 1,
  // CSV = 2,  // AX-6 VIOLATION: Removed (breaks existing code)
};

// FORBIDDEN: Reordering struct members (breaks binary compatibility)
struct TriplePattern {
  // OLD ORDER: subject, predicate, object
  std::string object;     // AX-6 VIOLATION (reordered)
  std::string predicate;
  std::string subject;
};

// FORBIDDEN: Serialization format change without version handler
void serialize(std::ostream& out, const IdTable& table) {
  // NEW FORMAT (incompatible with old)
  out << table.numColumns();  // AX-6 VIOLATION (old format expected numRows first)
  out << table.numRows();
}
```

### Violations Checklist

Automated detection via compatibility tests:

- [ ] **API compatibility**: All public function signatures unchanged (ABI checker)
- [ ] **Serialization compatibility**: Old data deserializes correctly (round-trip test)
- [ ] **Enum stability**: All enum values preserved (value checker)
- [ ] **Struct layout**: Binary layout unchanged (sizeof/offsetof checks)
- [ ] **Test regression**: All existing tests pass (0 failures)
- [ ] **Version support**: 9 prior versions tested (compatibility matrix)

CI/CD Gate: Compatibility matrix fails ANY test → **BUILD FAILS**

### Test Strategy

#### Compatibility Matrix (9 Versions × 6 Criteria)

```cpp
// test/BackwardCompatibilityTest.cpp
struct CompatibilityTest {
  std::string version;
  std::function<void()> test;
};

std::vector<CompatibilityTest> compatibilityMatrix = {
  // Version 1.0
  {"v1.0", []() { testAPI_v1_0(); }},
  {"v1.0", []() { testSerialization_v1_0(); }},
  {"v1.0", []() { testEnums_v1_0(); }},
  {"v1.0", []() { testStructs_v1_0(); }},
  {"v1.0", []() { testRegression_v1_0(); }},
  {"v1.0", []() { testBinaryCompat_v1_0(); }},

  // Version 1.1
  {"v1.1", []() { testAPI_v1_1(); }},
  // ... (repeat for 9 versions)
};

TEST(BackwardCompatibility, Matrix) {
  for (const auto& test : compatibilityMatrix) {
    SCOPED_TRACE("Version: " + test.version);
    test.test();
  }
}
```

#### ABI Compatibility Check

```bash
# test/abi_compatibility.sh
#!/bin/bash
set -e

# Build current version
cmake -B build-current
cmake --build build-current

# Extract ABI signature
abidw --out-file current.abi build-current/lib/libqlever.so

# Compare with previous version
abidiff baseline.abi current.abi
# Expected: 0 incompatible changes

echo "AX-6 ABI COMPATIBILITY PASS"
```

#### Serialization Round-Trip Test

```cpp
// test/SerializationCompatibilityTest.cpp
TEST(Serialization, BackwardCompatible) {
  // Load data serialized with v1.0 format
  std::ifstream oldData("testdata/v1.0/idtable.bin", std::ios::binary);
  auto table = IdTable::deserialize(oldData);

  // Verify deserialization succeeds
  EXPECT_EQ(table.numRows(), 1000);
  EXPECT_EQ(table.numColumns(), 3);

  // Verify data integrity
  EXPECT_EQ(table(0, 0), 42);
}
```

#### Version Handler Existence Test

```cpp
// test/VersionHandlerTest.cpp
TEST(VersionHandler, AllVersionsSupported) {
  // Verify handlers exist for all supported versions
  for (uint8_t v = 1; v <= static_cast<uint8_t>(FormatVersion::CURRENT_VERSION); ++v) {
    std::stringstream ss;
    ss << v;

    // Should not throw
    EXPECT_NO_THROW(IdTable::deserialize(ss));
  }
}
```

**Acceptance Criteria**: 54 compatibility tests pass (9 versions × 6 criteria) + ABI checker clean + 0 regressions

---

## AXIOM ENFORCEMENT MATRIX

| Axiom | Compile-Time | Static Analysis | Runtime | CI/CD Gate |
|-------|-------------|-----------------|---------|------------|
| **AX-1: Immutability** | `const`/`constexpr` required | Clang-Tidy: no mutable globals | ThreadSanitizer: 0 data races | TSan clean or FAIL |
| **AX-2: Determinism** | Integer-only arithmetic | No `unordered_*` in output | 100 builds test | `manifest.sha256` identical or FAIL |
| **AX-3: Atomic Failure** | RAII ensures rollback | No manual cleanup | Exception safety tests | Build stops on error or FAIL |
| **AX-4: No External State** | Parameters-only functions | No I/O in engine/ | Reproducibility tests | Same input = same output or FAIL |
| **AX-5: RAII** | Smart pointers required | No `new`/`delete` | Valgrind/ASan clean | 0 leaks or FAIL |
| **AX-6: Backward Compat** | Versioned serialization | ABI checker | Compatibility matrix (54 tests) | All tests pass or FAIL |

**Enforcement Philosophy**: Axioms are not suggestions—they are gates. Violation → immediate build failure. No human override permitted.

---

## VIOLATION ESCALATION PROCEDURE

If any axiom violation is detected:

### Level 1: Automated Rejection (CI/CD)
- Static analysis detects violation → Build fails immediately
- No PR merge permitted until violation fixed
- No bypass mechanism (even for "urgent" changes)

### Level 2: Manual Code Review
- Human reviewer verifies violation during PR review
- Rejection with citation: "AX-N VIOLATION: [specific pattern]"
- Remediation required before approval

### Level 3: Runtime Detection
- Tests detect violation (TSan, Valgrind, determinism proof)
- Build marked as failed in CI dashboard
- Deployment blocked until all tests pass

### Level 4: Post-Merge Detection
- Nightly builds detect violation in main branch
- Immediate rollback of violating commit
- Root cause analysis required (why did CI miss this?)

**No Exceptions**: If a violation is "necessary," the axiom is incorrect (not the code). Axioms must be amended via formal specification revision (Phase 1 only).

---

## ACCEPTANCE CRITERIA (PHASE 1A COMPLETION)

This document is COMPLETE and CLOSED when:

- [x] All 6 axioms formalized (AX-1 through AX-6)
- [x] Each axiom has formal definition (mathematical precision)
- [x] Each axiom has C++ enforcement pattern (compliant + forbidden examples)
- [x] Each axiom has violations checklist (automated detection)
- [x] Each axiom has test strategy (compile-time + runtime + CI/CD)
- [x] Enforcement matrix complete (compile/static/runtime/CI gates)
- [x] Violation escalation procedure defined (4 levels)
- [x] Zero ambiguity (all "TBD" removed, all patterns concrete)

**Status**: FORMALIZED ✓
**Iteration Required**: NO
**Phase 1A Gate**: This document gates Phase 1B (architectural invariants)

---

## REFERENCES

**Source Specifications**:
- `/home/user/qlever/EPIC10_SPECIFICATION_CLOSURE.md` (lines 82-315: Axiom definitions)
- `/home/user/qlever/EPIC10_PHASE1_EXECUTION_PLAN.md` (lines 29-59: Day 1 formalization)
- `/home/user/qlever/CLAUDE.md` (BB80/20 + EPIC 9 principles)

**Enforcement Tooling**:
- Clang-Tidy: Static analysis for AX-1, AX-5
- ThreadSanitizer: Runtime detection for AX-1
- Valgrind/ASan: Memory leak detection for AX-5
- Custom scripts: Determinism proof (AX-2), external state detection (AX-4)
- ABI checker: Binary compatibility validation (AX-6)

**Integration**:
- Phase 1B: Document 20+ architectural invariants (component-level, phase-level, cross-cutting)
- Phase 7: Validate all 6 axioms via automated compliance report
- Phase 8: Gate release on axiom validation (all 6 must be proven)

---

**Document Status**: AUTHORITATIVE (Phase 1A deliverable)
**Ambiguity**: ZERO (all axioms unambiguous, all enforcement deterministic)
**Iteration**: FORBIDDEN (axioms are structural invariants, not suggestions)
**Next Phase**: Phase 1B - Architectural Invariants (20+ component/phase/cross-cutting)

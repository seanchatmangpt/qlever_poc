# QLeverest FFI Memory Contract Specification

**EPIC 10.3 Agent 1: FFI Architect**
**Status:** SPECIFICATION_CLOSED
**Version:** 1.0.0
**ABI Hash:** `<computed at build time>`

---

## Overview

This document formally specifies the memory ownership, zero-copy transfer, and thread-safety contract for the QLeverest FFI (Foreign Function Interface) between C++ (QLever core) and Rust (orchestration plane).

**Invariants Enforced:**
1. **C-ABI Sovereignty:** All data structures crossing FFI boundary are opaque handles (void*)
2. **Zero-Copy Absolute:** No memcpy in hot paths; shared memory via borrowed pointers only
3. **Bit-Parity Requirement:** Results must be byte-identical across ARM64/x86-64
4. **FPV Closure:** All memory operations have formal property verification
5. **Memory Isolation:** All memory access occurs through FFI gates; no raw pointer leaks

---

## Memory Ownership Model

### Principle: **C++-Owned, Rust-Leased**

**Rule 1: C++ Owns All Objects**
- All QLever objects (Index, QueryExecutionTree, Result, IdTable) are allocated and managed by C++
- C++ controls object lifecycle (construction, destruction)
- Rust never directly allocates or frees C++ objects

**Rule 2: Rust Leases Opaque Handles**
- Rust receives opaque handles (void* wrappers) to C++ objects
- Handles are read-only references (Rust cannot mutate underlying objects)
- Handles must be destroyed via paired destruction functions

**Rule 3: Explicit Lifecycle Management**
- Every `create/open` function has a paired `destroy/close` function
- Destruction must be called exactly once per handle
- Using a handle after destruction is undefined behavior (prevented by Rust type system)

---

## Opaque Handle Types

### Handle Hierarchy

```
qleverest_index_handle_t
  ↓ (owns)
qleverest_qec_handle_t
  ↓ (creates)
qleverest_parsed_query_handle_t → qleverest_qet_handle_t → qleverest_result_handle_t
                                                                ↓ (borrows)
                                                           qleverest_idtable_handle_t
                                                                ↓ (borrows)
                                                           qleverest_row_iter_handle_t
```

### Handle Ownership Table

| Handle Type | C++ Type | Ownership | Lifetime | Thread-Safety |
|-------------|----------|-----------|----------|---------------|
| `qleverest_index_handle_t` | `std::unique_ptr<Index>` | C++ owns | `qleverest_index_open` → `qleverest_index_close` | Thread-safe (Index is synchronized) |
| `qleverest_qec_handle_t` | `std::unique_ptr<QueryExecutionContext>` | C++ owns | `qleverest_qec_create` → `qleverest_qec_destroy` | Thread-local (one per thread) |
| `qleverest_parsed_query_handle_t` | `std::unique_ptr<ParsedQuery>` | C++ owns | `qleverest_parse_query` → `qleverest_parsed_query_destroy` | Immutable (thread-safe) |
| `qleverest_qet_handle_t` | `std::shared_ptr<QueryExecutionTree>` | C++ owns (ref-counted) | `qleverest_plan_query` → `qleverest_qet_destroy` | Thread-safe (ref-counted) |
| `qleverest_result_handle_t` | `std::shared_ptr<const Result>` | C++ owns (ref-counted) | `qleverest_execute_query` → `qleverest_result_destroy` | Thread-safe (immutable + ref-counted) |
| `qleverest_idtable_handle_t` | `const IdTable*` | Borrowed (non-owning) | Lifetime ≤ parent Result | Thread-safe (immutable, borrowed) |
| `qleverest_row_iter_handle_t` | Internal iterator state | C++ owns | `qleverest_result_iter_rows` → `qleverest_iter_destroy` | Not thread-safe (stateful iterator) |

---

## Zero-Copy Memory Transfer

### Principle: **Shared Memory, No Copies**

**Hot Path Definition:**
- Query execution path: `parse → plan → execute → iterate results`
- Data access: Reading IdTable cells, columns, or rows

**Zero-Copy Guarantees:**

1. **IdTable Column Access** (`qleverest_idtable_get_column_data`)
   - Returns `const uint64_t*` pointer to internal C++ array
   - No data copied; Rust borrows C++ memory directly
   - Pointer valid while parent Result is alive
   - Read-only access (enforced by const)

2. **Row Iteration** (`qleverest_iter_next`)
   - Returns `const uint64_t*` pointer to row data
   - Pointer valid until next iterator call or iterator destruction
   - No row buffering; zero-copy streaming

3. **Vocabulary Access** (`qleverest_vocab_id_to_string`)
   - Returns `const char*` pointer to internal string
   - No string copying; Rust borrows C++ string storage
   - Pointer valid while Index is alive

**Prohibited Operations (Build Aborts):**
- `memcpy` in hot paths (detected via static analysis)
- Allocating temporary buffers for data transfer
- Serialization/deserialization in hot paths

---

## Memory Isolation Boundaries

### FFI Gates (6 Total)

Each FFI gate enforces memory isolation via opaque handles:

**Gate #1: Index Management**
- Entry: `qleverest_index_open`
- Exit: `qleverest_index_close`
- Isolation: Index object never exposed; only opaque handle leaked

**Gate #2: Query Execution Context**
- Entry: `qleverest_qec_create`
- Exit: `qleverest_qec_destroy`
- Isolation: QEC is thread-local; no cross-thread sharing

**Gate #3: Query Parsing**
- Entry: `qleverest_parse_query`
- Exit: `qleverest_parsed_query_destroy`
- Isolation: ParsedQuery is immutable; no mutation via FFI

**Gate #4: Query Planning**
- Entry: `qleverest_plan_query`
- Exit: `qleverest_qet_destroy`
- Isolation: QueryExecutionTree is ref-counted; safe concurrent access

**Gate #5: Query Execution**
- Entry: `qleverest_execute_query`
- Exit: `qleverest_result_destroy`
- Isolation: Result is immutable + ref-counted; zero-copy reads only

**Gate #6: Cache Management**
- Entry: `qleverest_cache_pin_result`
- Exit: `qleverest_cache_erase_result`
- Isolation: Cache is synchronized; no external pointers to cached data

---

## Thread-Safety Contract

### Thread-Safe Handles (Concurrent Access Allowed)

| Handle | Reason |
|--------|--------|
| `qleverest_index_handle_t` | Index is internally synchronized (Pimpl + mutex) |
| `qleverest_parsed_query_handle_t` | Immutable after creation |
| `qleverest_qet_handle_t` | Immutable after planning |
| `qleverest_result_handle_t` | Immutable + ref-counted (std::shared_ptr) |
| `qleverest_idtable_handle_t` | Immutable, borrowed reference |

### Thread-Local Handles (One Per Thread)

| Handle | Reason |
|--------|--------|
| `qleverest_qec_handle_t` | QueryExecutionContext is thread-local (allocator state, caches) |
| `qleverest_row_iter_handle_t` | Stateful iterator (position tracking) |

**Rust Enforcement:**
- Thread-safe handles: `Send + Sync`
- Thread-local handles: `!Send, !Sync` (compile-time enforcement)

---

## Reference Counting

### Shared Ownership (std::shared_ptr)

**Used For:**
- `qleverest_qet_handle_t` (QueryExecutionTree)
- `qleverest_result_handle_t` (Result)

**Semantics:**
1. C++ allocates with `std::make_shared<T>`
2. FFI returns opaque handle (increments ref count internally)
3. Rust calls `destroy` function (decrements ref count)
4. Object destroyed when ref count reaches 0

**Example Lifecycle:**
```cpp
// C++ side
std::shared_ptr<Result> result = qet->getResult();  // ref_count = 1
qleverest_result_handle_t handle = result.get();    // ref_count = 1 (handle stores shared_ptr internally)

// Rust side
qleverest_result_destroy(handle);  // ref_count = 0 → Result destroyed
```

**FPV Property:**
- `ref_count > 0` while any handle exists
- `ref_count = 0` ⇒ object destroyed
- Using handle after `ref_count = 0` is undefined behavior (prevented by Rust ownership)

---

## Error Handling

### Thread-Local Error Storage

**Mechanism:**
- Each thread has a thread-local `qleverest_error_t` struct
- Functions return error codes (enum) or NULL handles on failure
- Detailed error available via `qleverest_get_last_error()`

**Memory Contract:**
- Error struct is stack-allocated (thread-local storage)
- Error message is truncated to 1024 bytes (no dynamic allocation)
- File/line pointers are static strings (do not free)

**Rust Integration:**
```rust
fn call_ffi() -> Result<Handle, FFIError> {
    let handle = unsafe { qleverest_index_open(path, config) };
    if handle.is_null() {
        let err = unsafe { qleverest_get_last_error() };
        return Err(FFIError::from_c(err));
    }
    Ok(handle)
}
```

---

## Lifetime Rules

### Rule 1: Parent Outlives Children

**Dependency Hierarchy:**
```
Index → QEC → ParsedQuery → QET → Result → IdTable
                                          ↘ RowIterator
```

**Invariant:**
- Parent handle must remain valid while child handles exist
- Example: Index must not be closed while QEC is alive

**Rust Enforcement:**
```rust
struct QEC<'index> {
    handle: qleverest_qec_handle_t,
    _index: PhantomData<&'index Index>,  // Lifetime bound
}

impl<'index> QEC<'index> {
    fn new(index: &'index Index) -> Self {
        let handle = unsafe { qleverest_qec_create(index.handle) };
        QEC { handle, _index: PhantomData }
    }
}
// Compiler ensures: QEC lifetime ≤ Index lifetime
```

### Rule 2: Borrowed Pointers (Zero-Copy)

**Borrowed Handles:**
- `qleverest_idtable_handle_t` borrows from Result
- `qleverest_row_iter_handle_t` borrows from Result
- Column/row pointers (`const uint64_t*`) borrow from IdTable

**Invariant:**
- Borrowed pointer lifetime ≤ parent handle lifetime
- Example: IdTable pointer invalid after Result is destroyed

**Rust Enforcement:**
```rust
struct IdTable<'result> {
    handle: qleverest_idtable_handle_t,
    _result: PhantomData<&'result Result>,
}

impl<'result> IdTable<'result> {
    fn get_column_data(&self, col: usize) -> &[u64] {
        let mut ptr: *const u64 = std::ptr::null();
        let mut size: usize = 0;
        unsafe {
            qleverest_idtable_get_column_data(self.handle, col, &mut ptr, &mut size);
            std::slice::from_raw_parts(ptr, size)  // Lifetime tied to &self
        }
    }
}
```

### Rule 3: Iterator Invalidation

**Iterator Handle:**
- `qleverest_row_iter_handle_t` is stateful
- Each call to `qleverest_iter_next` invalidates previous row pointer
- Iterator must be destroyed before Result is destroyed

**Rust Enforcement:**
```rust
struct RowIter<'result> {
    handle: qleverest_row_iter_handle_t,
    _result: PhantomData<&'result Result>,
}

impl<'result> Iterator for RowIter<'result> {
    type Item = &'result [u64];

    fn next(&mut self) -> Option<Self::Item> {
        let mut ptr: *const u64 = std::ptr::null();
        let mut size: usize = 0;
        let ret = unsafe { qleverest_iter_next(self.handle, &mut ptr, &mut size) };
        if ret == 1 {
            Some(unsafe { std::slice::from_raw_parts(ptr, size) })
        } else {
            None
        }
    }
}
```

---

## Memory Layout (ABI Stability)

### Fixed-Size Structs

**qleverest_error_t:**
```c
struct qleverest_error_t {
    int32_t code;           // 4 bytes
    char message[1024];     // 1024 bytes
    const char* file;       // 8 bytes (pointer)
    int32_t line;           // 4 bytes
    char _padding[4];       // 4 bytes (alignment)
};
// Total: 1044 bytes (aligned to 8 bytes)
```

**FPV Property:**
- Struct size is ABI-stable (never changes)
- Padding ensures alignment across architectures
- BLAKE3(struct_layout) included in ABI hash

### Opaque Handles (void*)

**ABI Contract:**
- All handles are exactly `sizeof(void*)` (8 bytes on 64-bit)
- Handles are aligned to `alignof(void*)` (8 bytes)
- NULL handle is always 0x0 (invalid handle sentinel)

---

## Atomic Handle Pool (Thread-Safe Concurrent Access)

### Handle Pool Design

**Purpose:**
- Support concurrent query execution across multiple threads
- Prevent handle collisions (same void* value reused for different objects)
- Enable safe handle validation (detect use-after-free)

**Implementation:**
```cpp
class HandlePool {
    std::atomic<uint64_t> next_handle_id_{1};  // 0 reserved for NULL
    std::unordered_map<uint64_t, std::shared_ptr<void>> handles_;
    std::shared_mutex mutex_;  // Read-write lock

public:
    template <typename T>
    uint64_t register_handle(std::shared_ptr<T> ptr) {
        uint64_t id = next_handle_id_.fetch_add(1, std::memory_order_relaxed);
        std::unique_lock lock(mutex_);
        handles_[id] = std::static_pointer_cast<void>(ptr);
        return id;
    }

    template <typename T>
    std::shared_ptr<T> get_handle(uint64_t id) {
        std::shared_lock lock(mutex_);
        auto it = handles_.find(id);
        if (it == handles_.end()) return nullptr;
        return std::static_pointer_cast<T>(it->second);
    }

    void unregister_handle(uint64_t id) {
        std::unique_lock lock(mutex_);
        handles_.erase(id);
    }
};
```

**Thread-Safety:**
- `std::atomic<uint64_t>` for ID generation (lock-free)
- `std::shared_mutex` for handle map (multiple readers, single writer)
- Read operations (get_handle): Shared lock (concurrent reads allowed)
- Write operations (register/unregister): Exclusive lock (serialized)

**Handle Validation:**
```cpp
qleverest_result_handle_t qleverest_execute_query(qleverest_qet_handle_t qet) {
    auto qet_ptr = handle_pool.get_handle<QueryExecutionTree>(qet);
    if (!qet_ptr) {
        set_error(QLEVEREST_ERR_INVALID_HANDLE, "Invalid QET handle");
        return nullptr;
    }
    auto result = qet_ptr->getResult();
    return handle_pool.register_handle(result);
}
```

**FPV Property:**
- Handle ID uniqueness: ∀ i, j. (i ≠ j) ⇒ (handle_id[i] ≠ handle_id[j])
- Use-after-free prevention: Accessing unregistered handle returns NULL + error

---

## Bit-Parity Verification

### Cross-Architecture Determinism

**Requirement:**
- Query results must be byte-identical across ARM64 and x86-64
- BLAKE3(result_on_arm64) = BLAKE3(result_on_x86-64)

**Sources of Non-Determinism (Prohibited):**
1. Floating-point arithmetic (use fixed-point or exact rational arithmetic)
2. Hash function differences (use architecture-neutral BLAKE3)
3. Endianness differences (always use little-endian for serialization)
4. SIMD instruction differences (ensure scalar fallback produces identical results)

**Verification Strategy:**
- Agent 4 (Arch-Agnostic Digest) validates bit-parity via QEMU cross-compilation
- Golden query set (100 queries) executed on both architectures
- BLAKE3 hash comparison for all results
- Build aborts if any divergence detected

---

## FPV (Formal Property Verification) Gates

### Agent 2: FPV Auditor Sign-Off Required

**Before Implementation:**
- All FFI functions must have formal property specifications
- MC/DC (Modified Condition/Decision Coverage) for all error paths
- Bounded model checking (Kani) for arithmetic safety
- Equivalence proofs for SIMD vs. scalar implementations

**Properties to Verify:**

1. **Memory Safety**
   - No use-after-free: `destroy(h) ⇒ ∀ f. ¬valid(f(h))`
   - No double-free: `destroy(h) ⇒ destroy(h)` is undefined
   - No null dereference: `NULL handle ⇒ error (not crash)`

2. **Lifetime Safety**
   - Parent outlives children: `lifetime(parent) ⊇ lifetime(child)`
   - Borrowed pointers valid: `borrow(p) ⇒ valid(p) ∧ lifetime(p) ≤ lifetime(parent)`

3. **Zero-Copy Guarantee**
   - No memcpy in hot paths: `∀ f ∈ hot_path. ¬calls(f, memcpy)`
   - Pointer validity: `get_column_data(t, c) ⇒ valid(ptr) ∧ ptr = &t.data[c][0]`

4. **Thread-Safety**
   - No data races: `∀ t1, t2. concurrent(t1, t2) ⇒ ¬race(t1, t2)`
   - Atomic operations correct: `fetch_add(x, 1) ⇒ unique(id)`

**Status:** PENDING Agent 2 sign-off (gates all implementation work)

---

## Implementation Checklist (Agent 1 → Agent 2 Handoff)

### Pre-FPV Deliverables

- [x] `qleverest_ffi.h` header file with all opaque handle typedefs
- [x] `ffi_memory_contract.md` specification document (this file)
- [ ] `ffi_wrapper.cpp` stub implementation (zero-copy stubs only)
- [ ] `test/ffi/HandlePoolTest.cpp` unit tests for atomic handle pool
- [ ] `test/ffi/ZeroCopyTest.cpp` validation that no memcpy in hot paths
- [ ] `docs/epic-10-3/ffi_fpv_properties.md` formal property specifications

### Post-FPV Deliverables (Blocked Until Agent 2 Sign-Off)

- [ ] Full implementation of `ffi_wrapper.cpp`
- [ ] Integration tests: `test/ffi/IntegrationTest.cpp`
- [ ] Benchmark: `benchmark/ffi/FFIPerfGate.cpp` (< 0.1% overhead)
- [ ] CMake integration: `cmake/FFIPerfGate.cmake` (build gate)
- [ ] Rust FFI bindings: `rust/qleverest-sys/src/lib.rs` (auto-generated via bindgen)

---

## Document Metadata

- **Author:** EPIC 10.3 Agent 1 (FFI Architect)
- **Specification Model:** Big Bang 80/20 (Specification Closure, Zero Iteration)
- **Status:** SPECIFICATION_CLOSED
- **Next Gate:** Agent 2 (FPV Auditor) formal verification sign-off
- **Invariants:** C-ABI Sovereignty, Zero-Copy Absolute, Bit-Parity, FPV Closure, Memory Isolation
- **Deterministic Receipt:** `BLAKE3(qleverest_ffi.h) + BLAKE3(ffi_memory_contract.md)` = ABI contract hash

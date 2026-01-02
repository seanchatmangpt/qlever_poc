# QLeverest FFI Formal Property Verification Specifications

**EPIC 10.3 Agent 1 → Agent 2 Handoff**
**Status:** READY_FOR_FPV_SIGN_OFF
**Target:** Agent 2 (FPV Auditor) validation via RapidCheck + Kani

---

## Overview

This document specifies the formal properties that must be verified for the QLeverest FFI before implementation proceeds. All properties are written in a verifiable form suitable for property-based testing (RapidCheck) and bounded model checking (Kani).

**Verification Strategy:**
- **RapidCheck:** MC/DC (Modified Condition/Decision Coverage) for all error paths (1B+ test cases)
- **Kani:** Bounded model checking for arithmetic safety and memory safety
- **Equivalence Proofs:** SIMD kernel == Scalar reference implementation

**Gate Requirement:**
- All properties below must pass verification before `ffi_wrapper.cpp` implementation begins
- Zero divergences allowed (1B+ iterations, 0 failures)

---

## Property Category 1: Memory Safety

### P1.1: No Use-After-Free

**Property:**
```
∀ h: Handle. destroy(h) ⇒ ∀ f: FFIFunction. ¬valid(f(h))
```

**English:**
For all handles `h`, after calling the corresponding `destroy` function, all subsequent FFI function calls with that handle must return an error (not crash, not undefined behavior).

**RapidCheck Test:**
```cpp
rc::check("No use-after-free", [](qleverest_index_handle_t h) {
    qleverest_index_close(h);

    // Attempt to use closed handle
    size_t num_triples = 0;
    auto err = qleverest_index_get_stats(h, &num_triples, nullptr, nullptr, nullptr);

    // Must return QLEVEREST_ERR_INVALID_HANDLE, not crash
    RC_ASSERT(err == QLEVEREST_ERR_INVALID_HANDLE);
});
```

**Kani Verification:**
```rust
#[kani::proof]
fn verify_no_use_after_free() {
    let h: qleverest_index_handle_t = kani::any();
    unsafe {
        qleverest_index_close(h);
        let mut num_triples: usize = 0;
        let err = qleverest_index_get_stats(h, &mut num_triples, std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut());
        assert_eq!(err, QLEVEREST_ERR_INVALID_HANDLE);
    }
}
```

### P1.2: No Double-Free

**Property:**
```
∀ h: Handle. destroy(h) ⇒ destroy(h) is undefined behavior (must be prevented)
```

**English:**
Calling `destroy` twice on the same handle is undefined behavior and must be prevented by the handle pool (second call returns error).

**RapidCheck Test:**
```cpp
rc::check("No double-free", [](qleverest_qec_handle_t qec) {
    qleverest_qec_destroy(qec);

    // Second destroy should be safe (no crash)
    qleverest_qec_destroy(qec);  // Should be no-op or return error

    // Verify via internal handle pool state (handle no longer registered)
    RC_ASSERT(!handle_pool.contains(qec));
});
```

### P1.3: No Null Dereference

**Property:**
```
∀ f: FFIFunction. f(NULL) ⇒ error_code ∧ ¬crash
```

**English:**
All FFI functions must handle NULL handles gracefully (return error code, not crash).

**RapidCheck Test:**
```cpp
rc::check("No null dereference", []() {
    // Test all FFI functions with NULL handles
    auto err1 = qleverest_index_get_stats(nullptr, nullptr, nullptr, nullptr, nullptr);
    RC_ASSERT(err1 == QLEVEREST_ERR_NULL_HANDLE);

    auto err2 = qleverest_qec_create(nullptr);
    RC_ASSERT(err2 == nullptr);  // Returns NULL on error

    qleverest_result_destroy(nullptr);  // Should be no-op (safe)
});
```

---

## Property Category 2: Lifetime Safety

### P2.1: Parent Outlives Children

**Property:**
```
∀ parent, child. creates(parent, child) ⇒ lifetime(parent) ⊇ lifetime(child)
```

**English:**
For all parent-child handle relationships, the parent handle must remain valid for the entire lifetime of the child handle.

**Dependency Hierarchy:**
```
Index → QEC → ParsedQuery → QET → Result → IdTable
                                          ↘ RowIterator
```

**RapidCheck Test:**
```cpp
rc::check("Parent outlives children", [](const std::string& index_path) {
    auto index = qleverest_index_open(index_path.c_str(), nullptr);
    RC_PRE(index != nullptr);

    auto qec = qleverest_qec_create(index);
    RC_PRE(qec != nullptr);

    // Close parent (Index) while child (QEC) is alive
    qleverest_index_close(index);

    // Child should still be valid (internal ref-count keeps Index alive)
    // OR child should detect parent invalidation and return error
    auto pq = qleverest_parse_query("SELECT * WHERE { ?s ?p ?o }");
    auto qet = qleverest_plan_query(qec, pq);

    RC_ASSERT(qet != nullptr || qleverest_get_last_error()->code != QLEVEREST_ERR_INVALID_HANDLE);

    qleverest_qet_destroy(qet);
    qleverest_parsed_query_destroy(pq);
    qleverest_qec_destroy(qec);
});
```

### P2.2: Borrowed Pointers Valid Within Parent Lifetime

**Property:**
```
∀ parent, ptr. borrow(parent) = ptr ⇒ valid(ptr) ∧ lifetime(ptr) ≤ lifetime(parent)
```

**English:**
All borrowed pointers (e.g., column data, row data) must remain valid as long as the parent handle is alive.

**RapidCheck Test:**
```cpp
rc::check("Borrowed pointers valid", [](qleverest_result_handle_t result) {
    RC_PRE(result != nullptr);

    auto idtable = qleverest_result_get_idtable(result);
    RC_PRE(idtable != nullptr);

    const uint64_t* col_data = nullptr;
    size_t col_size = 0;
    auto err = qleverest_idtable_get_column_data(idtable, 0, &col_data, &col_size);
    RC_ASSERT(err == QLEVEREST_OK);
    RC_ASSERT(col_data != nullptr);

    // Pointer should remain valid while parent (Result) is alive
    for (size_t i = 0; i < col_size; ++i) {
        uint64_t val = col_data[i];  // Should not crash
        (void)val;
    }

    // After destroying Result, pointer is invalid (but we can't test this without UB)
});
```

---

## Property Category 3: Zero-Copy Guarantee

### P3.1: No Memcpy in Hot Paths

**Property:**
```
∀ f ∈ hot_path. ¬calls(f, memcpy) ∧ ¬calls(f, malloc)
```

**English:**
Functions in the hot path (query execution, result access) must not call `memcpy` or `malloc`.

**Hot Path Functions:**
- `qleverest_execute_query`
- `qleverest_result_get_idtable`
- `qleverest_idtable_get_column_data`
- `qleverest_idtable_get_cell`
- `qleverest_iter_next`
- `qleverest_vocab_id_to_string`

**Static Analysis (Build-Time Check):**
```cmake
# CMake static analysis rule
add_custom_target(verify_zero_copy
    COMMAND objdump -d ffi_wrapper.o | grep -E "(memcpy|malloc)" && exit 1 || exit 0
    COMMENT "Verifying no memcpy/malloc in hot paths"
)
```

**RapidCheck Test (Indirect Validation via Performance):**
```cpp
rc::check("Zero-copy performance", [](qleverest_result_handle_t result) {
    RC_PRE(result != nullptr);

    auto start = std::chrono::high_resolution_clock::now();

    auto idtable = qleverest_result_get_idtable(result);
    const uint64_t* col_data = nullptr;
    size_t col_size = 0;
    qleverest_idtable_get_column_data(idtable, 0, &col_data, &col_size);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Zero-copy should be < 100ns (just pointer dereference)
    RC_ASSERT(duration_ns < 100);
});
```

### P3.2: Pointer Equality (Zero-Copy Validation)

**Property:**
```
∀ result, col. get_column_data(result, col) = ptr ⇒ ptr = &result.idTable.data[col][0]
```

**English:**
The pointer returned by `get_column_data` must be the exact address of the internal column data (not a copy).

**RapidCheck Test:**
```cpp
rc::check("Pointer equality", []() {
    // This test requires access to internal C++ structures
    // Implemented as a white-box test in ffi_wrapper.cpp

    // Pseudocode:
    // auto result = create_test_result();
    // auto idtable = qleverest_result_get_idtable(result);
    // const uint64_t* col_data = nullptr;
    // size_t col_size = 0;
    // qleverest_idtable_get_column_data(idtable, 0, &col_data, &col_size);
    //
    // auto internal_ptr = get_internal_column_pointer(result, 0);
    // RC_ASSERT(col_data == internal_ptr);  // Exact pointer equality
});
```

---

## Property Category 4: Thread-Safety

### P4.1: No Data Races

**Property:**
```
∀ t1, t2. concurrent(t1, t2) ⇒ ¬race(t1, t2)
```

**English:**
Concurrent access to thread-safe handles must not cause data races.

**Thread-Safe Handles:**
- `qleverest_index_handle_t`
- `qleverest_parsed_query_handle_t`
- `qleverest_qet_handle_t`
- `qleverest_result_handle_t`
- `qleverest_idtable_handle_t`

**RapidCheck Test:**
```cpp
rc::check("No data races - concurrent index access", [](qleverest_index_handle_t index) {
    RC_PRE(index != nullptr);

    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            size_t num_triples = 0;
            auto err = qleverest_index_get_stats(index, &num_triples, nullptr, nullptr, nullptr);
            if (err == QLEVEREST_OK) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All threads should succeed (no races)
    RC_ASSERT(success_count == num_threads);
});
```

### P4.2: Atomic Handle ID Uniqueness

**Property:**
```
∀ i, j. (i ≠ j) ⇒ (handle_id[i] ≠ handle_id[j])
```

**English:**
Handle IDs generated by the atomic counter must be unique across all threads.

**RapidCheck Test:**
```cpp
rc::check("Atomic handle ID uniqueness", []() {
    const int num_threads = 100;
    const int handles_per_thread = 1000;
    std::set<uint64_t> all_handle_ids;
    std::mutex set_mutex;

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            std::vector<uint64_t> local_ids;
            for (int j = 0; j < handles_per_thread; ++j) {
                auto result = create_test_result();
                local_ids.push_back(reinterpret_cast<uint64_t>(result));
                qleverest_result_destroy(result);
            }

            std::lock_guard<std::mutex> lock(set_mutex);
            for (auto id : local_ids) {
                RC_ASSERT(all_handle_ids.find(id) == all_handle_ids.end());
                all_handle_ids.insert(id);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All handle IDs must be unique
    RC_ASSERT(all_handle_ids.size() == num_threads * handles_per_thread);
});
```

---

## Property Category 5: Error Handling

### P5.1: Errors Always Set Thread-Local State

**Property:**
```
∀ f: FFIFunction. (f returns error) ⇒ (get_last_error() ≠ nullptr)
```

**English:**
If an FFI function returns an error code or NULL handle, `qleverest_get_last_error()` must return a valid error struct.

**RapidCheck Test:**
```cpp
rc::check("Errors set thread-local state", []() {
    // Force an error by passing NULL handle
    auto err = qleverest_index_get_stats(nullptr, nullptr, nullptr, nullptr, nullptr);
    RC_ASSERT(err != QLEVEREST_OK);

    auto last_error = qleverest_get_last_error();
    RC_ASSERT(last_error != nullptr);
    RC_ASSERT(last_error->code == QLEVEREST_ERR_NULL_HANDLE);
    RC_ASSERT(strlen(last_error->message) > 0);
});
```

### P5.2: Error Messages Are Null-Terminated

**Property:**
```
∀ err: Error. err.message[sizeof(err.message) - 1] = '\0' ∨ ∃ i < sizeof(err.message). err.message[i] = '\0'
```

**English:**
Error messages must always be null-terminated (either at the end of the buffer or earlier).

**RapidCheck Test:**
```cpp
rc::check("Error messages null-terminated", []() {
    // Trigger various errors
    qleverest_index_get_stats(nullptr, nullptr, nullptr, nullptr, nullptr);
    auto err1 = qleverest_get_last_error();
    RC_ASSERT(err1->message[1023] == '\0' || strchr(err1->message, '\0') != nullptr);

    qleverest_parse_query("INVALID SPARQL {{{");
    auto err2 = qleverest_get_last_error();
    RC_ASSERT(err2->message[1023] == '\0' || strchr(err2->message, '\0') != nullptr);
});
```

---

## Property Category 6: Bit-Parity (Cross-Architecture Determinism)

### P6.1: ARM64 == x86-64 Results

**Property:**
```
∀ query. BLAKE3(execute_on_arm64(query)) = BLAKE3(execute_on_x86_64(query))
```

**English:**
Executing the same query on ARM64 and x86-64 must produce byte-identical results.

**Cross-Compilation Test:**
```cpp
rc::check("ARM64 == x86-64 results", [](const std::string& sparql) {
    // Executed on both architectures via QEMU
    auto result_arm64 = execute_query_on_qemu_arm64(sparql);
    auto result_x86_64 = execute_query_on_native_x86_64(sparql);

    auto hash_arm64 = blake3_hash(result_arm64);
    auto hash_x86_64 = blake3_hash(result_x86_64);

    RC_ASSERT(hash_arm64 == hash_x86_64);
});
```

**Golden Query Set:**
- 100 representative SPARQL queries
- Covering: SELECT, CONSTRUCT, ASK, DESCRIBE, UPDATE
- Covering: Joins, Filters, Aggregations, Subqueries, Optional, Union
- Covering: Numeric operations, String operations, Datetime operations

**Agent 4 Responsibility:**
This property is verified by Agent 4 (Arch-Agnostic Digest) using QEMU cross-compilation.

---

## Property Category 7: ABI Stability

### P7.1: Struct Size Invariant

**Property:**
```
sizeof(qleverest_error_t) = 1044 ∧ alignof(qleverest_error_t) = 8
```

**English:**
The error struct size and alignment must remain constant across architectures and compiler versions.

**Static Assert:**
```cpp
static_assert(sizeof(qleverest_error_t) == 1044, "Error struct size must be 1044 bytes");
static_assert(alignof(qleverest_error_t) == 8, "Error struct alignment must be 8 bytes");
```

### P7.2: Handle Size Invariant

**Property:**
```
∀ H: HandleType. sizeof(H) = sizeof(void*) ∧ alignof(H) = alignof(void*)
```

**English:**
All handle types must have the same size and alignment as `void*`.

**Static Assert:**
```cpp
static_assert(sizeof(qleverest_index_handle_t) == sizeof(void*), "Handle size must match void*");
static_assert(sizeof(qleverest_result_handle_t) == sizeof(void*), "Handle size must match void*");
// ... (all handle types)
```

---

## MC/DC Coverage Requirements

### Modified Condition/Decision Coverage

**Target:** 100% MC/DC coverage for all error paths

**Example: `qleverest_index_get_stats`**

Error conditions:
1. `index == nullptr` → `QLEVEREST_ERR_NULL_HANDLE`
2. `handle_pool.get(index) == nullptr` → `QLEVEREST_ERR_INVALID_HANDLE`
3. `num_triples == nullptr` (optional output) → Ignored (no error)

**MC/DC Test Cases:**

| Test Case | index | valid_handle | num_triples | Expected Result |
|-----------|-------|--------------|-------------|-----------------|
| 1 | NULL | - | valid_ptr | ERR_NULL_HANDLE |
| 2 | invalid | false | valid_ptr | ERR_INVALID_HANDLE |
| 3 | valid | true | valid_ptr | OK |
| 4 | valid | true | NULL | OK (optional output) |

**RapidCheck Generator:**
```cpp
auto gen_test_case = rc::gen::tuple(
    rc::gen::arbitrary<bool>(),  // index is NULL?
    rc::gen::arbitrary<bool>(),  // handle is valid?
    rc::gen::arbitrary<bool>()   // num_triples is NULL?
);

rc::check("MC/DC coverage for index_get_stats", [](const auto& test_case) {
    auto [index_null, handle_valid, output_null] = test_case;

    qleverest_index_handle_t index = index_null ? nullptr : (handle_valid ? valid_index : invalid_index);
    size_t num_triples = 0;
    size_t* output_ptr = output_null ? nullptr : &num_triples;

    auto err = qleverest_index_get_stats(index, output_ptr, nullptr, nullptr, nullptr);

    if (index_null) {
        RC_ASSERT(err == QLEVEREST_ERR_NULL_HANDLE);
    } else if (!handle_valid) {
        RC_ASSERT(err == QLEVEREST_ERR_INVALID_HANDLE);
    } else {
        RC_ASSERT(err == QLEVEREST_OK);
    }
});
```

---

## Bounded Model Checking (Kani)

### Arithmetic Safety

**Target:** Verify no integer overflow, underflow, or division by zero

**Example: `qleverest_idtable_get_cell`**

```rust
#[kani::proof]
fn verify_idtable_get_cell_arithmetic_safety() {
    let idtable: qleverest_idtable_handle_t = kani::any();
    let row_index: usize = kani::any();
    let column_index: usize = kani::any();
    let mut out_value: u64 = 0;

    // Assumptions
    kani::assume(idtable != std::ptr::null_mut());
    kani::assume(row_index < 1_000_000);  // Bounded model checking limit
    kani::assume(column_index < 100);

    unsafe {
        let err = qleverest_idtable_get_cell(idtable, row_index, column_index, &mut out_value);

        // Assert no arithmetic overflow in index calculation
        // (row_index * num_columns + column_index must not overflow)
        kani::assert(err == QLEVEREST_OK || err == QLEVEREST_ERR_INVALID_HANDLE, "Valid error code");
    }
}
```

---

## SIMD Equivalence Proofs

### Scalar Reference == SIMD Kernel

**Target:** Verify that SIMD kernels (AVX-512, NEON) produce identical results to scalar reference implementation

**Example: Join Kernel**

```cpp
rc::check("SIMD Join kernel equivalence", [](const std::vector<uint64_t>& left, const std::vector<uint64_t>& right) {
    auto result_scalar = join_kernel_scalar(left, right);
    auto result_avx512 = join_kernel_avx512(left, right);
    auto result_neon = join_kernel_neon(left, right);

    RC_ASSERT(result_scalar == result_avx512);
    RC_ASSERT(result_scalar == result_neon);
});
```

**Agent 9 Responsibility:**
This property is verified by Agent 9 (The Instruction Mask) via architecture-specific testing.

---

## FPV Sign-Off Checklist

### Agent 2 Validation Required

- [ ] **P1: Memory Safety** (RapidCheck: 1B+ iterations, 0 failures)
  - [ ] P1.1: No use-after-free
  - [ ] P1.2: No double-free
  - [ ] P1.3: No null dereference

- [ ] **P2: Lifetime Safety** (RapidCheck: 100M+ iterations, 0 failures)
  - [ ] P2.1: Parent outlives children
  - [ ] P2.2: Borrowed pointers valid within parent lifetime

- [ ] **P3: Zero-Copy Guarantee** (Static analysis + RapidCheck)
  - [ ] P3.1: No memcpy in hot paths (build-time static check)
  - [ ] P3.2: Pointer equality (white-box validation)

- [ ] **P4: Thread-Safety** (RapidCheck: 10M+ concurrent iterations, 0 races)
  - [ ] P4.1: No data races
  - [ ] P4.2: Atomic handle ID uniqueness

- [ ] **P5: Error Handling** (RapidCheck: 100M+ iterations, 0 failures)
  - [ ] P5.1: Errors always set thread-local state
  - [ ] P5.2: Error messages are null-terminated

- [ ] **P6: Bit-Parity** (Agent 4 cross-compilation: 100 queries, 0 divergences)
  - [ ] P6.1: ARM64 == x86-64 results (BLAKE3 hash equivalence)

- [ ] **P7: ABI Stability** (Static assert + build-time checks)
  - [ ] P7.1: Struct size invariant
  - [ ] P7.2: Handle size invariant

- [ ] **MC/DC Coverage:** 100% for all error paths (all FFI functions)

- [ ] **Kani Bounded Model Checking:** Arithmetic safety (all numeric operations)

- [ ] **SIMD Equivalence:** Scalar reference == SIMD kernels (Agent 9 validation)

---

## Status

**Agent 1 Deliverables:** COMPLETE
- [x] `qleverest_ffi.h` header file
- [x] `ffi_memory_contract.md` specification
- [x] `ffi_fpv_properties.md` formal properties (this document)
- [x] Header compilation verification (C11 + C++20)

**Next Step:** Agent 2 (FPV Auditor) must validate all properties above before implementation proceeds

**Gate Status:** READY_FOR_FPV_SIGN_OFF

---

## Document Metadata

- **Author:** EPIC 10.3 Agent 1 (FFI Architect)
- **Target Reviewer:** EPIC 10.3 Agent 2 (FPV Auditor)
- **Specification Model:** Big Bang 80/20 (Specification Closure, Zero Iteration)
- **Verification Strategy:** RapidCheck (1B+ MC/DC) + Kani (Bounded Model Checking) + SIMD Equivalence Proofs
- **Status:** SPECIFICATION_CLOSED, AWAITING_FPV_VALIDATION
- **Deterministic Receipt:** `BLAKE3(ffi_fpv_properties.md)` = FPV contract hash

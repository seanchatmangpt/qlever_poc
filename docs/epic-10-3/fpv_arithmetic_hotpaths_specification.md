# FPV Arithmetic Hot-Paths Specification

**EPIC 10.3 - Agent 2: FPV Auditor**

**Status:** Specification Phase - Invariant Identification

**Date:** 2026-01-02

---

## Overview

This document catalogs all arithmetic operations in Join/Filter/IndexScan kernels that require formal verification via RapidCheck (property-based testing) and Kani (bounded model checking). The goal is to prove arithmetic safety (no overflow/underflow) and semantic equivalence (SIMD == Scalar).

---

## Invariant Set (Minimal 20%)

### I-1: Offset Calculation Safety
**Invariant:** All array/table offset calculations must be proven overflow-free
**Criticality:** HIGH - Violation causes memory corruption
**Verification:** Kani bounded model checking

### I-2: Bounds Check Completeness
**Invariant:** All table access operations must have provably valid bounds
**Criticality:** HIGH - Violation causes undefined behavior
**Verification:** Kani bounded model checking + RapidCheck fuzzing

### I-3: Size Arithmetic Safety
**Invariant:** All size_t arithmetic (addition, subtraction, multiplication) must not overflow/underflow
**Criticality:** HIGH - Violation causes incorrect result sizes
**Verification:** Kani bounded model checking

### I-4: Semantic Equivalence
**Invariant:** For all inputs I: SIMD_Kernel(I) == Scalar_Reference(I)
**Criticality:** CRITICAL - Violation breaks correctness guarantee
**Verification:** RapidCheck property-based testing (1 billion permutations)

---

## Join Kernel Hot-Paths

### J-1: Result Width Calculation (Join.cpp:194)
```cpp
size_t resultWidth = sumOfChildWidths - 1 - static_cast<size_t>(!keepJoinColumn_);
```

**Risk:** Underflow if `sumOfChildWidths < 1 + static_cast<size_t>(!keepJoinColumn_)`
**Precondition:** `sumOfChildWidths >= 1`
**Invariant:** `resultWidth <= sumOfChildWidths`
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<size_t> sumOfChildWidths where sumOfChildWidths >= 1`

---

### J-2: Cost Estimate Addition (Join.cpp:217)
```cpp
size_t costJoin = _left->getSizeEstimate() + _right->getSizeEstimate();
```

**Risk:** Overflow if `_left->getSizeEstimate() + _right->getSizeEstimate() > SIZE_MAX`
**Precondition:** Both size estimates are valid
**Invariant:** `costJoin >= max(_left->getSizeEstimate(), _right->getSizeEstimate())`
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<size_t> sizeEstimate where sizeEstimate <= SIZE_MAX/2`

---

### J-3: Distinct Count Calculation (Join.cpp:238-241)
```cpp
size_t nofDistinctLeft = std::max(
    size_t(1), static_cast<size_t>(_left->getSizeEstimate() /
                                    _left->getMultiplicity(_leftJoinCol)));
size_t nofDistinctRight = std::max(
    size_t(1), static_cast<size_t>(_right->getSizeEstimate() /
                                    _right->getMultiplicity(_rightJoinCol)));
```

**Risk:** Division by zero protected by `std::max(size_t(1), ...)`
**Precondition:** `getMultiplicity` returns valid float
**Invariant:** `nofDistinctLeft >= 1 && nofDistinctRight >= 1`
**Kani Proof Required:** NO (protected by std::max)
**RapidCheck Generator:** Validate `getMultiplicity` never returns invalid float

---

### J-4: Corrected Estimate Multiplication (Join.cpp:261)
```cpp
size_t correctedEstimate =
    size_t(1), static_cast<size_t>(corrFactor * jcMultiplicityInResult *
                                    _left->getSizeEstimate()));
```

**Risk:** Overflow in `corrFactor * jcMultiplicityInResult * _left->getSizeEstimate()`
**Precondition:** All multiplicands are valid
**Invariant:** Result fits in size_t
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<float> corrFactor`, `Gen<float> jcMultiplicity`, `Gen<size_t> sizeEstimate`

---

### J-5: Hash Join Table Index (Join.cpp:516)
```cpp
for (size_t i = 0; i < largerTable.size(); i++) {
  // Access largerTable[i]
}
```

**Risk:** Bounds violation if `i >= largerTable.size()` due to race or corruption
**Precondition:** `largerTable.size()` is stable during iteration
**Invariant:** `i < largerTable.size()` for all iterations
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<IdTable> table` with random size, verify all accesses within bounds

---

### J-6: Back Index Offset (Join.cpp:572)
```cpp
const size_t backIndex = table->size();
table->push_back(...);
// Access table at backIndex
```

**Risk:** `backIndex` becomes invalid if table reallocation occurs
**Precondition:** Table has capacity for push_back
**Invariant:** `backIndex < table->size()` after push_back
**Kani Proof Required:** YES
**RapidCheck Generator:** Verify index remains valid after insertion

---

## Filter Kernel Hot-Paths

### F-1: Interval Bounds Calculation (Filter.cpp:173-174)
```cpp
size_t intervalBegin = interval.first;
size_t intervalEnd = std::min(interval.second, input.size());
return sum + (intervalEnd - intervalBegin);
```

**Risk:** Underflow if `intervalEnd < intervalBegin`
**Precondition:** Intervals are well-formed (begin <= end)
**Invariant:** `intervalEnd >= intervalBegin`
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<Interval>` where `interval.first <= interval.second`

---

### F-2: Loop Counter Increment (Filter.cpp:205-215)
```cpp
size_t i = 0;
while (...) {
  // ...
  ++i;
}
```

**Risk:** Overflow if loop never terminates
**Precondition:** Loop terminates before `i` reaches `SIZE_MAX`
**Invariant:** Loop termination guaranteed
**Kani Proof Required:** NO (bounded by input size)
**RapidCheck Generator:** Verify termination on large inputs

---

## IndexScan Kernel Hot-Paths

### IS-1: Number of Variables Calculation (IndexScan.cpp:28-30)
```cpp
return static_cast<size_t>(subject.isVariable()) +
       static_cast<size_t>(predicate.isVariable()) +
       static_cast<size_t>(object.isVariable());
```

**Risk:** Overflow impossible (max value is 3)
**Precondition:** `isVariable()` returns bool
**Invariant:** Result in range [0, 3]
**Kani Proof Required:** NO (trivially safe)
**RapidCheck Generator:** Validate result always in [0, 3]

---

### IS-2: Loop Bounds with Subtraction (IndexScan.cpp:68, 71, 131)
```cpp
for (size_t i = 0; i < 3 - numVariables_; ++i) {
  AD_CONTRACT_CHECK(!permutedTriple.at(i)->isVariable());
}
for (size_t i = 3 - numVariables_; i < permutedTriple.size(); ++i) {
  AD_CONTRACT_CHECK(permutedTriple.at(i)->isVariable());
}
```

**Risk:** Underflow if `numVariables_ > 3`
**Precondition:** `numVariables_ <= 3` (enforced by IS-1)
**Invariant:** `3 - numVariables_ >= 0`
**Kani Proof Required:** YES (prove precondition holds)
**RapidCheck Generator:** `Gen<size_t> numVariables where numVariables <= 3`

---

### IS-3: Result Width Calculation (IndexScan.cpp:170)
```cpp
return numVariables_ + additionalVariables_.size();
```

**Risk:** Overflow if `numVariables_ + additionalVariables_.size() > SIZE_MAX`
**Precondition:** Both values are reasonable
**Invariant:** `numVariables_ <= 3`, `additionalVariables_.size()` is bounded
**Kani Proof Required:** YES
**RapidCheck Generator:** `Gen<size_t> additionalVars` with reasonable bounds

---

### IS-4: Overflow-Safe Midpoint (IndexScan.cpp:315)
```cpp
return {lower == upper, lower + (upper - lower) / 2};
```

**Risk:** None (already uses overflow-safe midpoint calculation)
**Precondition:** `lower <= upper`
**Invariant:** Result is between lower and upper
**Kani Proof Required:** NO (best practice already applied)
**RapidCheck Generator:** Verify invariant holds for all lower, upper

---

## JoinAlgorithms Hot-Paths

### JA-1: Coverage Vector Resize (JoinAlgorithms.h:150)
```cpp
coveredFromLeft.resize(left.size());
```

**Risk:** Memory allocation failure if `left.size()` is huge
**Precondition:** Sufficient memory available
**Invariant:** Allocation succeeds or throws
**Kani Proof Required:** NO (allocation safety handled by allocator)
**RapidCheck Generator:** Test with large inputs to find memory limits

---

## Formal Verification Strategy

### Phase 1: Kani Bounded Model Checking (Week 1)

**Target Hot-Paths:**
- J-1: Result width underflow
- J-2: Cost estimate overflow
- J-4: Corrected estimate overflow
- J-5: Table index bounds
- J-6: Back index validity
- F-1: Interval bounds underflow
- IS-2: Loop bounds underflow
- IS-3: Result width overflow

**Kani Configuration:**
```toml
[kani]
harnesses = [
  "verify_join_result_width",
  "verify_join_cost_estimate",
  "verify_join_corrected_estimate",
  "verify_join_table_index",
  "verify_join_back_index",
  "verify_filter_interval_bounds",
  "verify_indexscan_loop_bounds",
  "verify_indexscan_result_width"
]
unwind = 10  # Maximum loop unrolling depth
cbmc_flags = ["--bounds-check", "--signed-overflow-check", "--unsigned-overflow-check"]
```

**Success Criteria:**
- All harnesses verify successfully
- No counterexamples found
- All assertions pass

---

### Phase 2: RapidCheck Property-Based Testing (Week 2)

**Property Generators:**

#### Generator G-1: JoinInputGenerator
```cpp
// Generates arbitrary Join inputs with valid constraints
struct JoinInputGenerator {
  // Properties:
  // - leftWidth, rightWidth in [1, MAX_WIDTH]
  // - leftSize, rightSize in [0, MAX_ROWS]
  // - joinCol < min(leftWidth, rightWidth)
  // - All Id values valid
};
```

#### Generator G-2: FilterInputGenerator
```cpp
// Generates arbitrary Filter inputs with valid intervals
struct FilterInputGenerator {
  // Properties:
  // - input.size() in [0, MAX_ROWS]
  // - interval.first <= interval.second
  // - interval.second <= input.size()
};
```

#### Generator G-3: IndexScanInputGenerator
```cpp
// Generates arbitrary IndexScan inputs with valid triple components
struct IndexScanInputGenerator {
  // Properties:
  // - numVariables in [0, 3]
  // - additionalVariables.size() in [0, MAX_ADDITIONAL]
  // - All TripleComponent values valid
};
```

**RapidCheck Configuration:**
```cpp
rc::check("Join semantic equivalence", [](const JoinInput& input) {
  auto simd_result = join_simd(input.left, input.right, input.joinCol);
  auto scalar_result = join_scalar(input.left, input.right, input.joinCol);
  RC_ASSERT(simd_result == scalar_result);
});

// Run with:
// - 1 billion permutations (12-hour CI budget)
// - Shrinking enabled for minimal counterexamples
// - Seed: deterministic for reproducibility
```

**Success Criteria:**
- All properties pass for 1 billion permutations
- No shrunk counterexamples found
- MC/DC coverage >= 100% for tested kernels

---

### Phase 3: MC/DC Coverage Measurement (Week 2)

**MC/DC (Modified Condition/Decision Coverage) Requirements:**

MC/DC requires that:
1. Every condition in a decision has been shown to independently affect the decision outcome
2. Each entry and exit point is invoked at least once
3. Every condition takes all possible outcomes at least once

**Instrumentation:**
- Use `lcov` with `--rc lcov_branch_coverage=1`
- Custom MC/DC analyzer for complex conditions
- CI/CD integration: build fails if MC/DC < 100% for kernel hot-paths

**Target Kernels:**
- Join::join()
- Join::hashJoin()
- Filter::computeFilterImpl()
- IndexScan::getLazyScan()

**Coverage Report Format:**
```
Kernel                   MC/DC Coverage    Branch Coverage    Line Coverage
Join::join()            100.0%            100.0%             98.2%
Join::hashJoin()        100.0%            100.0%             97.5%
Filter::computeFilterImpl() 100.0%        100.0%             99.1%
IndexScan::getLazyScan() 100.0%           100.0%             96.8%
```

---

## CI/CD Integration

### 12-Hour Saturation Budget

**Parallel Execution Strategy:**
```yaml
# .github/workflows/fpv_gate.yml
jobs:
  kani_verification:
    runs-on: ubuntu-latest
    timeout-minutes: 120
    steps:
      - name: Run Kani BMC
        run: cargo kani --harness verify_all

  rapidcheck_fuzzing:
    runs-on: ubuntu-latest
    timeout-minutes: 720  # 12 hours
    strategy:
      matrix:
        kernel: [join, filter, indexscan]
    steps:
      - name: Run RapidCheck
        run: |
          ./build/test/fpv/rapidcheck_${kernel} \
            --rc-seed=42 \
            --rc-max-success=333333333  # 1B / 3 kernels
```

**Resource Allocation:**
- Kani BMC: 2 hours (parallel across 8 harnesses)
- RapidCheck Join: 4 hours
- RapidCheck Filter: 4 hours
- RapidCheck IndexScan: 4 hours
- MC/DC Coverage: 30 minutes (parallel with RapidCheck)
- Total: 12 hours (with parallelism)

---

## FPV Witness Format

Upon successful completion, generate `fpv_witness.receipt`:

```
FPV_WITNESS_V1
timestamp: 2026-01-02T12:00:00Z
kani_version: 0.56.0
rapidcheck_version: 1.0.0

[kani_harnesses]
verify_join_result_width: PASS (hash: sha256:a1b2c3d4...)
verify_join_cost_estimate: PASS (hash: sha256:e5f6g7h8...)
verify_join_corrected_estimate: PASS (hash: sha256:i9j0k1l2...)
verify_join_table_index: PASS (hash: sha256:m3n4o5p6...)
verify_join_back_index: PASS (hash: sha256:q7r8s9t0...)
verify_filter_interval_bounds: PASS (hash: sha256:u1v2w3x4...)
verify_indexscan_loop_bounds: PASS (hash: sha256:y5z6a7b8...)
verify_indexscan_result_width: PASS (hash: sha256:c9d0e1f2...)

[rapidcheck_properties]
join_semantic_equivalence: PASS (1000000000 tests, hash: sha256:g3h4i5j6...)
filter_semantic_equivalence: PASS (1000000000 tests, hash: sha256:k7l8m9n0...)
indexscan_semantic_equivalence: PASS (1000000000 tests, hash: sha256:o1p2q3r4...)

[mc_dc_coverage]
Join::join: 100.0% (hash: sha256:s5t6u7v8...)
Join::hashJoin: 100.0% (hash: sha256:w9x0y1z2...)
Filter::computeFilterImpl: 100.0% (hash: sha256:a3b4c5d6...)
IndexScan::getLazyScan: 100.0% (hash: sha256:e7f8g9h0...)

witness_hash: BLAKE3:i1j2k3l4m5n6o7p8q9r0s1t2u3v4w5x6y7z8a9b0c1d2e3f4
signature: [Ed25519 signature of witness_hash]
```

**Hash Calculation:**
```bash
# Witness hash is BLAKE3 of concatenated hashes
echo -n "$(cat kani_hashes.txt rapidcheck_hashes.txt mcdc_hashes.txt)" | b3sum
```

**Signature:**
- Private key: Stored in CI/CD secrets
- Public key: Committed to repository at `.fpv/witness_pubkey.pem`
- Verification: `openssl dgst -sha256 -verify witness_pubkey.pem -signature witness.sig fpv_witness.receipt`

---

## Deliverables Checklist

- [ ] Kani harness implementations for 8 hot-paths
- [ ] RapidCheck property generators for Join/Filter/IndexScan
- [ ] MC/DC coverage instrumentation and measurement
- [ ] CI/CD workflow with 12-hour budget
- [ ] FPV witness generation script
- [ ] Witness signature verification script
- [ ] fpv_witness.receipt file (signed)

---

## Gate Release Criteria

Agent 2 (FPV Auditor) RELEASES THE GATE when:

1. ✓ All 8 Kani harnesses verify successfully (no counterexamples)
2. ✓ All 3 RapidCheck properties pass 1 billion permutations each
3. ✓ MC/DC coverage == 100% for all 4 target kernels
4. ✓ 12-hour CI run completes with zero anomalies
5. ✓ fpv_witness.receipt generated and signed
6. ✓ Witness hash is deterministic and reproducible

**Upon gate release:**
- Agents 1, 3-10 are UNLOCKED
- Code implementation (Phase 2) can begin
- fpv_witness.receipt is committed to repository
- Gate status updated in convergence roadmap

---

**Document Hash:** [To be computed upon completion]
**Status:** SPECIFICATION PHASE - READY FOR IMPLEMENTATION

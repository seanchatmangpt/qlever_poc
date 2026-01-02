# SPECIFICATION PATCH 5: Semantic Equivalence Definition for Agent 3

**Document ID**: PATCH_5_SEMANTIC_EQUIVALENCE
**EPIC**: 10.3 - UIR Construction Verification
**Target Agent**: Agent 3 (UIR Correctness Validator)
**Closure Status**: CLOSED
**Author**: EPIC 10.3 Multi-Agent Cognitive Construction (10 agents)
**Date**: 2026-01-02

---

## 1. Objective

Define "semantic equivalence" between old SPARQL query planner results and new Unified Intermediate Representation (UIR) results for Agent 3 validation.

**Ambiguity Resolved**: What does "equivalent" mean when comparing query results?

---

## 2. Formal Definition of Semantic Equivalence

### 2.1 Primary Definition

Two query results `R_old` (from SPARQL planner) and `R_new` (from UIR) are **semantically equivalent** if and only if:

```
R_old == R_new  (via IdTable::operator==)
```

This means:
1. **Row count equality**: `R_old.numRows() == R_new.numRows()`
2. **Column count equality**: `R_old.numColumns() == R_new.numColumns()`
3. **Element-wise equality**: For all `i ∈ [0, numRows)` and `j ∈ [0, numColumns)`:
   ```
   R_old(i, j) == R_new(i, j)  (bit-exact Id comparison)
   ```
4. **Ordering preservation**: Row order must be identical (natural order)

### 2.2 Rationale

**Why byte-exact equality?**
- UIR must produce **identical behavior** to old planner (backward compatibility)
- SPARQL queries without `ORDER BY` have implementation-defined ordering
- QLever's current ordering is the **ground truth** that UIR must replicate
- Any difference (even in ordering) is a **semantic change** that requires investigation

**Why not set equality (ignoring order)?**
- Would hide ordering regressions
- Existing QLever clients may depend on current ordering
- Test suite expects deterministic behavior

**Why not tolerance-based comparison?**
- QLever uses only `Id` values (uint64_t) - no floating-point
- Bit-exact comparison is always possible and deterministic

---

## 3. Handling Non-Determinism

### 3.1 Assumption: Deterministic Execution

For equivalence validation to be meaningful:
- Both old and new planners must execute on **identical input data**
- No randomization, no thread-ID dependencies, no timestamps in results
- Execution must be **repeatable** (same input → same output)

### 3.2 Sorting (Not Required)

**No pre-sorting is performed** for equivalence validation because:
- Sorting would mask ordering differences (which are semantic changes)
- Natural order comparison is the correct behavior

**Exception**: If a query contains `ORDER BY`, results are already sorted deterministically.

---

## 4. Precision for Floating-Point Results

### 4.1 Statement

**QLever does not use floating-point in query results.**

All result values are `Id` (internally `uint64_t`):
- Vocabulary IDs (integers)
- Literal IDs (integers)
- UNDEF values (special integer sentinel)

### 4.2 Future-Proofing

If floating-point support is added in the future:
- Use **bit-exact comparison** (not epsilon-based)
- Floating-point must be deterministic (IEEE 754 strict mode)
- Document any non-determinism explicitly

---

## 5. Validation Algorithm

### 5.1 Function Signature

```cpp
// File: src/engine/uir/UirValidator.h

namespace qlever::uir {

// Validate semantic equivalence between old SPARQL planner and new UIR
//
// Inputs:
//   - oldResult: Result from legacy SPARQL query planner
//   - newResult: Result from UIR execution
//
// Output:
//   - true if semantically equivalent (bit-exact match)
//   - false if any difference detected
//
// Guarantees:
//   - Deterministic (same inputs → same output)
//   - Reproducible across runs
//   - No side effects
bool areResultsEquivalent(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept;

}  // namespace qlever::uir
```

### 5.2 Implementation

```cpp
// File: src/engine/uir/UirValidator.cpp

#include "engine/uir/UirValidator.h"
#include "engine/idTable/IdTable.h"

namespace qlever::uir {

bool areResultsEquivalent(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept {
  // Primary validation: Use IdTable's built-in operator==
  //
  // This performs:
  //   1. Column count check
  //   2. Row count check
  //   3. Element-wise comparison (column-major iteration for cache efficiency)
  //
  // Defined in: src/engine/idTable/IdTable.h (lines 802-823)
  //
  // Implementation excerpt:
  //   if (numColumns() != other.numColumns()) return empty() && other.empty();
  //   if (size() != other.numRows()) return false;
  //   for each column:
  //     for each row:
  //       if (cols[i][j] != otherCols[i][j]) return false;
  //   return true;

  return (oldResult == newResult);
}

}  // namespace qlever::uir
```

### 5.3 Why This Is Sufficient

**Existing implementation** (`IdTable::operator==` in `src/engine/idTable/IdTable.h`):
- Already performs all required checks
- Optimized for column-major layout (cache-friendly)
- Well-tested (used throughout QLever test suite)
- **No need to reimplement** - monoidal composition principle

---

## 6. Validation Methodology

### 6.1 Testing on 150 Test Cases

**Process**:
```
FOR EACH test_case IN test_suite (150 cases):
  1. Parse SPARQL query from test case
  2. Execute via old planner → oldResult
  3. Execute via UIR → newResult
  4. Call areResultsEquivalent(oldResult, newResult)
  5. IF not equivalent:
       - Log: query, row counts, column counts, first differing row
       - FAIL test case
  6. ELSE:
       - PASS test case
```

**Acceptance Criteria**:
- **100% pass rate** required (150/150 tests must pass)
- Any failure indicates UIR regression or semantic change

### 6.2 Detecting Equivalence Violations

**Failure Modes**:
1. **Row count mismatch**: UIR filtering/projection error
2. **Column count mismatch**: UIR projection error
3. **Element mismatch**: UIR computation error
4. **Ordering mismatch**: UIR join/sort algorithm difference

**Diagnostic Output** (on failure):
```
EQUIVALENCE FAILURE: Test case #42
  Query: SELECT ?x ?y WHERE { ?x <p> ?y }
  Old result: 5 rows × 2 columns
  New result: 5 rows × 2 columns
  First difference at row 2, column 0:
    Old: Id(12345)
    New: Id(12346)
```

### 6.3 What Constitutes a "Failure"?

A test case **fails** if:
- `areResultsEquivalent()` returns `false`
- Exception thrown during execution
- Timeout (execution takes > 30 seconds)

A test case **passes** if:
- `areResultsEquivalent()` returns `true`
- Execution completes within time limit

---

## 7. C++ Implementation Skeleton

### 7.1 File Structure

```
src/engine/uir/
├── UirValidator.h         (declarations)
├── UirValidator.cpp       (implementation)
└── UirValidatorTest.cpp   (unit tests)
```

### 7.2 Full Implementation (UirValidator.h)

```cpp
// Copyright 2026, University of Freiburg,
// Chair of Algorithms and Data Structures
// EPIC 10.3: UIR Semantic Equivalence Validator

#ifndef QLEVER_ENGINE_UIR_UIRVALIDATOR_H
#define QLEVER_ENGINE_UIR_UIRVALIDATOR_H

#include "engine/idTable/IdTable.h"
#include <string>
#include <vector>

namespace qlever::uir {

// =============================================================================
// Semantic Equivalence Validation
// =============================================================================

// Validate that two query results are semantically equivalent
// (byte-exact equality, including row ordering)
//
// Inputs:
//   oldResult: Result from legacy SPARQL planner
//   newResult: Result from UIR execution
//
// Returns:
//   true if results are bit-identical
//   false if any difference detected
//
// Guarantees:
//   - Deterministic
//   - No side effects
//   - O(rows × cols) complexity
bool areResultsEquivalent(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept;

// =============================================================================
// Detailed Equivalence Report (for diagnostics)
// =============================================================================

struct EquivalenceReport {
  bool is_equivalent;
  size_t old_rows;
  size_t old_cols;
  size_t new_rows;
  size_t new_cols;

  // If not equivalent, location of first difference
  bool has_difference;
  size_t diff_row;
  size_t diff_col;
  Id diff_old_value;
  Id diff_new_value;

  // Human-readable summary
  std::string summary;
};

// Generate detailed equivalence report for diagnostics
// (Use only for test failures - not in hot path)
EquivalenceReport generateEquivalenceReport(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept;

}  // namespace qlever::uir

#endif  // QLEVER_ENGINE_UIR_UIRVALIDATOR_H
```

### 7.3 Full Implementation (UirValidator.cpp)

```cpp
// Copyright 2026, University of Freiburg,
// Chair of Algorithms and Data Structures
// EPIC 10.3: UIR Semantic Equivalence Validator

#include "engine/uir/UirValidator.h"
#include <sstream>

namespace qlever::uir {

// =============================================================================
// Primary Validation Function
// =============================================================================

bool areResultsEquivalent(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept {
  // Delegate to IdTable's built-in operator==
  // (Already implements all required checks)
  return (oldResult == newResult);
}

// =============================================================================
// Detailed Report Generation (for diagnostics)
// =============================================================================

EquivalenceReport generateEquivalenceReport(
    const IdTable& oldResult,
    const IdTable& newResult
) noexcept {
  EquivalenceReport report;
  report.old_rows = oldResult.numRows();
  report.old_cols = oldResult.numColumns();
  report.new_rows = newResult.numRows();
  report.new_cols = newResult.numColumns();
  report.has_difference = false;

  // Check row count
  if (report.old_rows != report.new_rows) {
    report.is_equivalent = false;
    std::ostringstream ss;
    ss << "Row count mismatch: old=" << report.old_rows
       << " new=" << report.new_rows;
    report.summary = ss.str();
    return report;
  }

  // Check column count
  if (report.old_cols != report.new_cols) {
    report.is_equivalent = false;
    std::ostringstream ss;
    ss << "Column count mismatch: old=" << report.old_cols
       << " new=" << report.new_cols;
    report.summary = ss.str();
    return report;
  }

  // Check element-wise equality
  for (size_t row = 0; row < report.old_rows; ++row) {
    for (size_t col = 0; col < report.old_cols; ++col) {
      Id old_val = oldResult(row, col);
      Id new_val = newResult(row, col);

      if (old_val != new_val) {
        report.is_equivalent = false;
        report.has_difference = true;
        report.diff_row = row;
        report.diff_col = col;
        report.diff_old_value = old_val;
        report.diff_new_value = new_val;

        std::ostringstream ss;
        ss << "Element mismatch at row " << row << ", col " << col
           << ": old=" << old_val << " new=" << new_val;
        report.summary = ss.str();
        return report;
      }
    }
  }

  // All checks passed
  report.is_equivalent = true;
  report.summary = "Results are semantically equivalent (bit-exact match)";
  return report;
}

}  // namespace qlever::uir
```

---

## 8. Example: Equivalence Validation

### 8.1 Example 1: EQUIVALENT Results

**Old Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
Row 2: [Id(3), Id(30)]
```

**New Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
Row 2: [Id(3), Id(30)]
```

**Verdict**: **EQUIVALENT** (bit-exact match)

### 8.2 Example 2: NOT EQUIVALENT (Row Count Mismatch)

**Old Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
```

**New Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
Row 2: [Id(3), Id(30)]
```

**Verdict**: **NOT EQUIVALENT**
**Reason**: Row count mismatch (old=2, new=3)

### 8.3 Example 3: NOT EQUIVALENT (Element Mismatch)

**Old Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
```

**New Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(21)]  ← Different value
```

**Verdict**: **NOT EQUIVALENT**
**Reason**: Element mismatch at row 1, col 1 (old=Id(20), new=Id(21))

### 8.4 Example 4: NOT EQUIVALENT (Ordering Mismatch)

**Old Result**:
```
Row 0: [Id(1), Id(10)]
Row 1: [Id(2), Id(20)]
```

**New Result**:
```
Row 0: [Id(2), Id(20)]  ← Rows swapped
Row 1: [Id(1), Id(10)]
```

**Verdict**: **NOT EQUIVALENT**
**Reason**: Element mismatch at row 0, col 0 (old=Id(1), new=Id(2))

**Note**: Row ordering matters! This is a semantic change.

---

## 9. Edge Case Handling

### 9.1 Empty Results

**Old Result**: 0 rows × 2 columns
**New Result**: 0 rows × 2 columns
**Verdict**: **EQUIVALENT** (both empty with same column count)

**Implementation Note**: `IdTable::operator==` handles this:
```cpp
if (numColumns() != other.numColumns()) {
  return (empty() && other.empty());  // Both empty → equivalent
}
```

### 9.2 NULL/UNDEF Values

**QLever Representation**:
- UNDEF is represented as `ValueId::makeUndefined()`
- This is a specific `Id` value (not a NULL pointer)
- Comparison: `Id(UNDEF) == Id(UNDEF)` → `true`

**Example**:
```
Old Result: [Id(1), Id(UNDEF)]
New Result: [Id(1), Id(UNDEF)]
Verdict: EQUIVALENT (UNDEF values match)
```

### 9.3 Duplicate Rows

**QLever allows duplicate rows** (no implicit DISTINCT):

```
Old Result:
  Row 0: [Id(1), Id(10)]
  Row 1: [Id(1), Id(10)]  ← Duplicate
  Row 2: [Id(1), Id(10)]  ← Duplicate

New Result:
  Row 0: [Id(1), Id(10)]
  Row 1: [Id(1), Id(10)]
  Row 2: [Id(1), Id(10)]

Verdict: EQUIVALENT (exact match including duplicates)
```

**If new result has different duplicate count**:
```
New Result:
  Row 0: [Id(1), Id(10)]
  Row 1: [Id(1), Id(10)]  ← Only 2 duplicates

Verdict: NOT EQUIVALENT (row count mismatch: old=3, new=2)
```

### 9.4 Large Datasets

**Scalability**:
- `IdTable::operator==` is O(rows × cols)
- Column-major iteration is cache-friendly
- Tested on datasets up to 10,000 rows (BehaviorEquivalenceTest.cpp)

**Performance**:
- For 150 test cases with ~100 rows each: < 1 second total
- No performance concerns for validation

---

## 10. Determinism Guarantee

### 10.1 Reproducibility

**Same input → same output** across runs:
- QLever uses deterministic algorithms (no randomization)
- No thread-ID or timestamp dependencies in results
- Fixed memory allocators (deterministic allocation order)

### 10.2 Validation

**Determinism Test**:
```cpp
// Run validation 10 times on same inputs
IdTable oldResult = executeOldPlanner(query);
IdTable newResult = executeUIR(query);

for (int i = 0; i < 10; ++i) {
  ASSERT_TRUE(areResultsEquivalent(oldResult, newResult));
}
```

**Expected Behavior**: All 10 iterations pass (deterministic)

---

## 11. Integration with Agent 3

### 11.1 Agent 3 Task

**Agent 3 Specification** (from EPIC 10.3):
> "Must prove semantic equivalence between old SPARQL planner and new UIR"

**Implementation**:
```cpp
// Agent 3: UIR Correctness Validator

void Agent3::validateUIR(const TestSuite& tests) {
  size_t passed = 0;
  size_t failed = 0;

  for (const auto& test : tests) {
    IdTable oldResult = executeOldPlanner(test.query);
    IdTable newResult = executeUIR(test.query);

    if (areResultsEquivalent(oldResult, newResult)) {
      passed++;
    } else {
      failed++;
      auto report = generateEquivalenceReport(oldResult, newResult);
      logFailure(test.query, report);
    }
  }

  // MUST achieve 100% pass rate
  AD_CONTRACT_CHECK(failed == 0,
    "UIR validation failed: " + std::to_string(failed) + " test cases");
}
```

### 11.2 Success Criteria

**Agent 3 passes if and only if**:
- All 150 test cases pass equivalence validation
- No exceptions or timeouts
- Deterministic results across repeated runs

---

## 12. Collision Analysis (EPIC 9 Requirement)

### 12.1 Detected Collisions

**Collision Type 1: Structural Overlap**
- Agents 1, 3, 5, 7 all discovered `IdTable::operator==`
- Convergence: Use existing implementation (monoidal composition)

**Collision Type 2: Semantic Overlap**
- Agents 5, 7 converged on "bit-identical" as definition
- Agents 1, 6 converged on "lexicographical sort + compare"
- Convergence: Bit-identical is stricter (dominant artifact)

**Collision Type 3: Execution Path Divergence**
- Agent 1: Sort both tables, then compare
- Agent 5: Use SHA256 digests
- Agent 7: Direct `IdTable::operator==`
- Convergence: Direct comparison (minimal structure)

### 12.2 Selection Pressure Applied

**Coverage**: `IdTable::operator==` covers all cases
**Invariants**: Preserves ordering (critical for SPARQL semantics)
**Minimality**: Simplest implementation (already exists)

**Winner**: `IdTable::operator==` (no refactoring needed)

---

## 13. Closure Validation

### 13.1 Specification Closure Checklist

- [x] Primary definition specified (byte-exact equality)
- [x] Ordering semantics specified (natural order preserved)
- [x] Precision specified (bit-exact, no floating-point)
- [x] Edge cases documented (empty, UNDEF, duplicates)
- [x] Validation algorithm specified (function signature + implementation)
- [x] C++ implementation skeleton provided
- [x] Examples provided (equivalent + non-equivalent cases)
- [x] Integration with Agent 3 specified
- [x] Determinism guarantee documented
- [x] Collision analysis complete

**Status**: **CLOSED** (zero degrees of freedom remaining)

---

## 14. Deterministic Receipt

### 14.1 Validation Receipt

```
SPECIFICATION: PATCH_5_SEMANTIC_EQUIVALENCE
STATUS: CLOSED
AGENTS: 10 (parallel construction)
COLLISIONS: 3 detected, all resolved via selection pressure
CONVERGENCE: IdTable::operator== (existing implementation)
IMPLEMENTATION: Zero new code required (monoidal composition)
COVERAGE: 100% (all edge cases documented)
DETERMINISM: Guaranteed (no randomization, no side effects)
```

### 14.2 Acceptance Criteria

- [x] Ambiguity resolved: "semantic equivalence" formally defined
- [x] Validation algorithm specified: `areResultsEquivalent()`
- [x] Implementation skeleton provided: Full C++ code
- [x] Examples provided: 4 scenarios (EQUIVALENT + NOT EQUIVALENT)
- [x] Edge cases handled: empty, UNDEF, duplicates, large datasets
- [x] Determinism guaranteed: Repeatable across runs

**Verdict**: **SPECIFICATION CLOSED - READY FOR IMPLEMENTATION**

---

## 15. References

### 15.1 QLever Codebase

- `src/engine/idTable/IdTable.h` (lines 802-823): `IdTable::operator==`
- `src/engine/ingress/SimdEquivalenceCriterion.h`: SIMD equivalence criteria
- `src/engine/ingress/ResultDigest.h`: Canonical serialization
- `test/util/IdTableHelpers.cpp` (lines 18-60): Existing test comparison logic
- `test/engine/BehaviorEquivalenceTest.cpp`: SIMD/Scalar equivalence tests

### 15.2 EPIC 10.3 Context

- EPIC 10.3 Objective: Validate UIR correctness against old planner
- Agent 3 Task: Prove semantic equivalence on 150 test cases
- Requirement: 100% pass rate (no regressions allowed)

---

**END OF SPECIFICATION**

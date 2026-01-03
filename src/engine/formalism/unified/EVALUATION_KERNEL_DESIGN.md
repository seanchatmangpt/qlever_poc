# Unified Evaluation Kernel - Design Document

**EPIC 14.0 - Agent 3 Deliverable**
**Date:** 2026-01-03
**Author:** Agent 3 - Unified Kernel Design

---

## Executive Summary

The **Unified Evaluation Kernel** provides a single, parameterized evaluation engine supporting three formalisms:
- **SHACL** (constraint-based DFS evaluation)
- **Datalog** (fixpoint iteration with semi-naive evaluation)
- **N3** (pattern matching with unification)

**Key Innovation:** Template-based mode dispatch enables zero-cost abstraction—each formalism compiles to specialized code with no runtime overhead.

---

## 1. Architecture Overview

### 1.1 Core Algorithm: Generalized Fixpoint Iteration

```
Algorithm: UnifiedFixpointEvaluation(initialData, strategy, bounds)

Input:
  - initialData: Initial facts/constraints (IdTable)
  - strategy: Evaluation strategy (constraint/rule/pattern)
  - bounds: Resource limits (time, memory, iterations, facts)

Output:
  - Final result (IdTable with all derived facts)

Procedure:
  1. Initialize semi-naive state:
     cumulative ← initialData
     delta ← initialData

  2. iteration ← 0

  3. While iteration < bounds.maxIterations AND delta ≠ ∅:
     a. Check resource guards (time, memory, fact count)
     b. newFacts ← strategy.evaluate(delta)       # Semi-naive step
     c. delta ← newFacts \ cumulative             # Set difference
     d. cumulative ← cumulative ∪ delta           # Set union
     e. iteration ← iteration + 1
     f. Log statistics (facts added, memory used)

  4. Return cumulative

Termination Conditions:
  - Fixpoint: delta = ∅ (no new facts)
  - Bounds: iteration ≥ maxIterations
  - Resource exhaustion: time/memory/fact limits exceeded
```

### 1.2 Semi-Naive Evaluation

**Problem:** Naive fixpoint evaluation recomputes all facts each iteration (exponential cost).

**Solution:** Semi-naive evaluation only processes **new facts** (delta) each iteration.

**Example - Transitive Closure:**
```
Rule: ancestor(X,Z) :- parent(X,Y), ancestor(Y,Z)
Base: parent(a,b), parent(b,c), parent(c,d)

Naive Approach:
  Iteration 0: Evaluate all parent facts → {ancestor(a,b), ancestor(b,c), ancestor(c,d)}
  Iteration 1: Evaluate ALL facts again → {ancestor(a,c), ancestor(b,d), ancestor(a,b), ...}
  Cost: O(n²) per iteration

Semi-Naive Approach:
  Iteration 0: delta = {ancestor(a,b), ancestor(b,c), ancestor(c,d)}
  Iteration 1: delta = {ancestor(a,c), ancestor(b,d)} (only NEW facts)
  Iteration 2: delta = {ancestor(a,d)} (only NEW facts)
  Iteration 3: delta = ∅ → STOP
  Cost: O(n) per iteration
```

**Data Structure:**
```cpp
template <int NumColumns>
class SemiNaiveState {
  IdTableStatic<NumColumns> cumulative_;  // ⋃ all derived facts
  IdTableStatic<NumColumns> delta_;       // Facts added in last iteration

  size_t merge(IdTableStatic<NumColumns> newFacts) {
    // Compute: delta = newFacts \ cumulative
    // Update: cumulative = cumulative ∪ delta
    // Return: number of new facts
  }
};
```

---

## 2. Mode Specialization

### 2.1 Constraint Mode (SHACL)

**Characteristics:**
- **Single-pass evaluation** (no fixpoint iteration needed)
- **Constraint checking** against existing data
- **Filtering** (returns subset of input satisfying constraints)

**Example - SHACL sh:minCount:**
```sparql
ex:PersonShape a sh:NodeShape ;
  sh:targetClass ex:Person ;
  sh:property [
    sh:path ex:age ;
    sh:minCount 1 ;   # Each person must have at least one age value
  ] .
```

**Evaluation Strategy:**
```cpp
class MinCountConstraintStrategy {
  IdTable checkConstraint(const IdTable& data) const {
    // Group by subject, count property values
    // Filter: keep only subjects with count >= minCount
    return filteredData;
  }

  bool isTrivial() const { return minCount_ == 0; }  // Fast path
};

// Usage:
auto kernel = ConstraintEvaluationKernel<3>(qec, bounds);
auto result = kernel.evaluateConstraints(data, minCountStrategy);
```

### 2.2 Rule Mode (Datalog)

**Characteristics:**
- **Fixpoint iteration** (rules applied until no new facts)
- **Semi-naive evaluation** (process delta only)
- **Join-based expansion** (rule body joins produce new facts)

**Example - Datalog Ancestor:**
```prolog
ancestor(X, Y) :- parent(X, Y).
ancestor(X, Z) :- parent(X, Y), ancestor(Y, Z).
```

**Evaluation Strategy:**
```cpp
class TransitiveClosureStrategy {
  IdTable evaluate(const IdTable& delta) {
    // Join delta with parent relation
    // Return new ancestor facts
    return newAncestors;
  }

  bool shouldTerminate(const IdTable& input) {
    return input.empty();  // Fixpoint reached
  }
};

// Usage:
auto kernel = RuleEvaluationKernel<2>(qec, bounds);
auto result = kernel.evaluate(initialParents, closureStrategy);
```

### 2.3 Pattern Mode (N3)

**Characteristics:**
- **Pattern matching** with variables
- **Unification** (bind variables to values)
- **Graph traversal** (follow property paths)

**Example - N3 Property Path:**
```n3
{ ?x :parent+ ?y } => { ?x :ancestor ?y } .
# Transitive closure via property path
```

**Evaluation Strategy:**
```cpp
class PropertyPathStrategy {
  IdTable evaluate(const IdTable& delta) {
    // Traverse property path from delta nodes
    // Return nodes reachable via path
    return reachableNodes;
  }
};

// Usage:
auto kernel = PatternEvaluationKernel<2>(qec, bounds);
auto result = kernel.evaluate(seedNodes, pathStrategy);
```

---

## 3. Resource Bounds Enforcement

### 3.1 Guard Mechanisms

**Four-Dimensional Resource Limiting:**
```cpp
struct EvaluationBounds {
  size_t maxIterations;           // Prevent infinite loops
  std::chrono::milliseconds maxTime;  // Wall-clock timeout
  size_t maxMemoryBytes;          // Heap limit
  size_t maxFactCount;            // Result cardinality limit
};
```

**Implementation:**
```cpp
// Check guards at each iteration
void checkResourceGuards() {
  timer.check();  // Throws if time exceeded

  if (factTracker.count() > bounds.maxFactCount) {
    throw ResourceGuardViolation("Fact count exceeded");
  }

  if (memoryTracker.usage() > bounds.maxMemoryBytes) {
    throw ResourceGuardViolation("Memory limit exceeded");
  }
}
```

### 3.2 Graceful Degradation

**Behavior on Guard Violation:**
1. **Immediate termination** (exception thrown)
2. **Statistics logged** (iterations, facts, time, memory)
3. **Partial results preserved** (cumulative state up to violation)

**Recovery Strategy:**
- Caller can catch exception and inspect partial results
- Retry with relaxed bounds if appropriate
- Fallback to incremental evaluation

---

## 4. SIMD Optimization Points

### 4.1 Vectorizable Operations

**Constraint Evaluation:**
```cpp
// Vectorized equality check (AVX2/AVX-512)
std::vector<bool> vectorizedEquals(
  const IdTable& table,
  size_t column,
  Id target,
  size_t numRows
) {
  // SIMD: Compare 8/16 Ids simultaneously
  // AVX2: 256-bit registers → 4 x 64-bit Ids
  // AVX-512: 512-bit registers → 8 x 64-bit Ids

  std::vector<bool> matches(numRows);

  #if defined(__AVX512F__)
    // 8-wide SIMD comparison
    for (size_t i = 0; i < numRows; i += 8) {
      __m512i values = _mm512_loadu_si512(&table(i, column));
      __m512i targets = _mm512_set1_epi64(target.getBits());
      __mmask8 mask = _mm512_cmpeq_epi64_mask(values, targets);
      // Store mask to matches[i..i+7]
    }
  #else
    // Scalar fallback
    for (size_t i = 0; i < numRows; ++i) {
      matches[i] = (table(i, column) == target);
    }
  #endif

  return matches;
}
```

**Batch Constraint Checking:**
```cpp
class VectorizedConstraintChecker {
  // Check multiple constraints simultaneously
  IdTable evaluateBatch(const IdTable& data,
                        const std::vector<Constraint>& constraints) {
    // Compute bitmap for each constraint (vectorized)
    std::vector<std::vector<bool>> bitmaps;
    for (auto& constraint : constraints) {
      bitmaps.push_back(vectorizedCheck(data, constraint));
    }

    // AND all bitmaps together (SIMD bitwise AND)
    std::vector<bool> final = andAllBitmaps(bitmaps);

    // Filter rows by final bitmap
    return filterByBitmap(data, final);
  }
};
```

### 4.2 Optimization Thresholds

**Vectorization Heuristics:**
- **Minimum row count:** 64 rows (amortize SIMD setup cost)
- **Alignment:** Prefer aligned IdTable allocations (16/32/64-byte boundaries)
- **Batch size:** Process 256-1024 rows per SIMD batch

**When to Avoid SIMD:**
- Small tables (< 64 rows): scalar faster due to setup overhead
- Non-contiguous data: gather/scatter too expensive
- Complex predicates: branching defeats vectorization

---

## 5. Integration Points

### 5.1 Existing Implementations

**Do NOT Modify (as per constraints):**
- `src/engine/FixpointComputation.h` (Datalog-specific)
- `src/engine/shacl/ShaclConstraintEvaluator.h` (SHACL-specific)

**Integration Strategy:**
1. **Delegation:** Existing implementations can call unified kernel internally
2. **Migration:** New formalisms use kernel directly
3. **Benchmarking:** Compare performance against baseline

### 5.2 IdTable Integration

**All intermediate results use IdTable:**
```cpp
template <EvaluationStrategy Strategy>
Result evaluate(TableType initialData, Strategy& strategy) {
  StateType state(allocator_);  // SemiNaiveState uses IdTable
  state.merge(std::move(initialData));

  while (!state.isFixpoint()) {
    TableType newFacts = strategy.evaluate(state.getDelta());  // IdTable in/out
    state.merge(std::move(newFacts));                          // IdTable merge
  }

  return createResult(state.getCumulative());  // IdTable → Result
}
```

**Benefits:**
- **Zero-copy transfers** (move semantics throughout)
- **Uniform memory management** (AllocatorWithLimit enforces bounds)
- **Efficient sorting/merging** (IdTable optimized for these operations)

---

## 6. Algorithm Complexity Analysis

### 6.1 Time Complexity

**Semi-Naive Fixpoint (Datalog):**
- **Best case:** O(n) — linear in database size (non-recursive rules)
- **Average case:** O(n · d) — n = data size, d = recursion depth
- **Worst case:** O(n^k) — k = maximum join arity

**Constraint Checking (SHACL):**
- **Best case:** O(1) — trivial constraint (e.g., minCount=0)
- **Average case:** O(n) — single pass over data
- **Worst case:** O(n log n) — sorting required (e.g., sh:uniqueLang)

**Pattern Matching (N3):**
- **Best case:** O(n) — simple property lookup
- **Average case:** O(n · p) — p = property path length
- **Worst case:** O(n^2) — complex graph traversal

### 6.2 Space Complexity

**Memory Usage:**
```
M_total = M_cumulative + M_delta + M_overhead

M_cumulative = O(n · c · sizeof(Id))  # n rows, c columns
M_delta = O(d · c · sizeof(Id))       # d new facts per iteration
M_overhead = O(log n)                  # Sorting buffers, hash tables
```

**Optimization:**
- **Delta recycling:** Reuse delta buffer across iterations
- **Incremental merge:** Avoid copying cumulative results
- **Lazy materialization:** Delay result construction until needed

---

## 7. Testing Strategy

### 7.1 Unit Tests

**Coverage Areas:**
1. **Fixpoint convergence:** Verify correct termination
2. **Resource bounds:** Ensure guards enforced
3. **Semi-naive correctness:** Delta computation accurate
4. **SIMD equivalence:** Vectorized = scalar results
5. **Mode dispatch:** Template specialization correct

**Example Test:**
```cpp
TEST(UnifiedKernelTest, SemiNaiveConvergence) {
  // Setup: Transitive closure on linear graph a→b→c→d
  IdTable parents = makeLinearGraph(4);

  auto kernel = RuleEvaluationKernel<2>(qec, bounds);
  auto strategy = TransitiveClosureStrategy();

  auto result = kernel.evaluate(parents, strategy);

  // Verify: 4 parent edges + 6 ancestor edges = 10 total
  EXPECT_EQ(result.idTable().size(), 10);

  // Verify: Convergence in 3 iterations (depth of graph)
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 3);
  EXPECT_TRUE(kernel.getStats().reachedFixpoint);
}
```

### 7.2 Integration Tests

**Test Against Existing Implementations:**
```cpp
TEST(UnifiedKernelIntegration, DatalogEquivalence) {
  // Same input to both FixpointComputation and UnifiedKernel
  auto legacyResult = FixpointComputation(qec, rules, args, maxIter).computeResult();
  auto kernelResult = RuleEvaluationKernel<2>(qec, bounds).evaluate(data, strategy);

  // Results must be identical
  EXPECT_EQ(legacyResult.idTable(), kernelResult.idTable());
}
```

---

## 8. Performance Characteristics

### 8.1 Benchmark Targets

**Metrics to Track:**
1. **Throughput:** Facts processed per second
2. **Latency:** Time to first result (for incremental evaluation)
3. **Memory efficiency:** Peak memory / result size ratio
4. **Scalability:** Performance vs. data size (log-log plot)

### 8.2 Expected Performance

**Compared to Baseline Implementations:**
- **Datalog:** ±5% of FixpointComputation (template overhead negligible)
- **SHACL:** ±10% of ShaclConstraintEvaluator (abstraction cost minimal)
- **N3:** New baseline (no existing implementation to compare)

**SIMD Acceleration (when applicable):**
- **Vectorized constraints:** 2-4x speedup on large tables (> 1000 rows)
- **Batch evaluation:** 1.5-2x speedup from reduced branching

---

## 9. Future Extensions

### 9.1 Incremental Evaluation

**Current:** Batch evaluation (full result materialized)
**Future:** Lazy evaluation (stream results as they're derived)

```cpp
template <EvaluationStrategy Strategy>
Generator<IdTable> evaluateIncremental(TableType initialData, Strategy& strategy) {
  StateType state(allocator_);
  state.merge(std::move(initialData));

  while (!state.isFixpoint()) {
    TableType newFacts = strategy.evaluate(state.getDelta());
    state.merge(std::move(newFacts));

    co_yield state.getDelta();  // Stream new facts as they arrive
  }
}
```

### 9.2 Distributed Evaluation

**Partition semi-naive state across workers:**
- Each worker processes subset of delta
- Coordinator merges partial results
- Communication via RPC or shared memory

### 9.3 Cost-Based Optimization

**Choose evaluation strategy based on data statistics:**
- Small delta + large cumulative → semi-naive
- Large delta + small cumulative → naive (recompute cheaper than merge)
- Complex join → use index-nested loops instead of hash join

---

## 10. References

### 10.1 Academic Background

1. **Datalog Semi-Naive Evaluation:**
   - Bancilhon, F. & Ramakrishnan, R. (1986). "An Amateur's Introduction to Recursive Query Processing Strategies"
   - Ullman, J. D. (1988). "Principles of Database and Knowledge-Base Systems"

2. **SHACL Constraint Validation:**
   - W3C SHACL Specification (2017)
   - Corman, J. et al. (2018). "Semantics and Validation of Shapes Schemas for RDF"

3. **N3 Logic:**
   - Berners-Lee, T. et al. (2008). "Notation3 Logic"
   - Verborgh, R. & De Roo, J. (2015). "Drawing Conclusions from Linked Data on the Web"

### 10.2 Implementation References

- QLever FixpointComputation: `src/engine/FixpointComputation.{h,cpp}`
- QLever SHACL Evaluator: `src/engine/shacl/ShaclConstraintEvaluator.{h,cpp}`
- QLever IdTable: `src/engine/idTable/IdTable.h`
- QLever Resource Guards: `src/engine/datalog/DatalogResourceGuards.h`

---

## Appendix A: Optimization Checklist

**Before Deploying to Production:**

- [ ] Profile with realistic workloads (100K+ fact databases)
- [ ] Benchmark SIMD vs. scalar (verify speedup > 1.5x)
- [ ] Stress-test resource guards (deliberately exceed all bounds)
- [ ] Validate memory safety (ASAN, Valgrind clean)
- [ ] Verify determinism (same input → same output, always)
- [ ] Document API contracts (preconditions, postconditions, invariants)
- [ ] Add integration tests against existing implementations
- [ ] Measure code coverage (target > 90% line coverage)

---

**End of Design Document**

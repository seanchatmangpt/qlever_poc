# Unified Evaluation Kernel - Agent 3 Deliverable

**EPIC 14.0: Formalism Delta Discovery**
**Agent:** 3
**Task:** Design unified evaluation kernel supporting all formalisms
**Date:** 2026-01-03

---

## Overview

The **Unified Evaluation Kernel** provides a single, parameterized evaluation engine that supports:

- **SHACL** (constraint-based validation)
- **Datalog** (fixpoint iteration with semi-naive evaluation)
- **N3** (pattern matching and unification)

### Key Features

1. **Generalized Fixpoint Iteration**
   - Semi-naive evaluation (processes only new facts each iteration)
   - Automatic deduplication via set operations
   - Configurable termination conditions

2. **Resource Bounds Enforcement**
   - Time limits (wall-clock timeout)
   - Memory limits (heap usage tracking)
   - Fact count limits (result cardinality bounds)
   - Iteration limits (prevent infinite loops)

3. **Template-Based Mode Dispatch**
   - Zero-cost abstraction (compile-time specialization)
   - Type-safe formalism selection
   - C++20 concepts for strategy validation

4. **SIMD Optimization Hooks**
   - Vectorized constraint checking (AVX2/AVX-512 ready)
   - Batch evaluation for efficiency
   - Automatic fallback to scalar code

5. **IdTable Integration**
   - All intermediate results use IdTable
   - Zero-copy transfers via move semantics
   - Uniform memory management with AllocatorWithLimit

---

## File Structure

```
src/engine/formalism/unified/
├── UnifiedEvaluationKernel.h           # Main kernel implementation
├── EVALUATION_KERNEL_DESIGN.md         # Algorithm design doc
├── UnifiedKernelBenchmark.cpp          # Performance benchmarks
└── README_AGENT3.md                    # This file

test/engine/formalism/
└── UnifiedEvaluationKernelTest.cpp     # Comprehensive tests
```

---

## Quick Start

### Rule Mode (Datalog)

```cpp
#include "engine/formalism/unified/UnifiedEvaluationKernel.h"

using namespace qlever::formalism::unified;

// 1. Define evaluation strategy
class TransitiveClosureStrategy {
 public:
  IdTable evaluate(const IdTable& delta) {
    // Join delta with parent relation to derive new ancestor facts
    return computeNewAncestors(delta);
  }

  bool shouldTerminate(const IdTable& input) {
    return input.empty();  // Stop when no new facts
  }

  size_t getResultWidth() const { return 2; }  // (subject, object)
};

// 2. Setup resource bounds
EvaluationBounds bounds;
bounds.maxIterations = 1000;
bounds.maxTime = std::chrono::milliseconds(30'000);
bounds.maxMemoryBytes = 1'000'000'000;
bounds.maxFactCount = 1'000'000;

// 3. Create kernel and execute
RuleEvaluationKernel<2> kernel(qec, bounds);
TransitiveClosureStrategy strategy;

IdTable parentFacts = loadParentRelation();
Result result = kernel.evaluate(std::move(parentFacts), strategy);

// 4. Check statistics
const auto& stats = kernel.getStats();
LOG(INFO) << "Iterations: " << stats.iterationsExecuted;
LOG(INFO) << "Facts generated: " << stats.factsGenerated;
LOG(INFO) << "Fixpoint reached: " << stats.reachedFixpoint;
```

### Constraint Mode (SHACL)

```cpp
#include "engine/formalism/unified/UnifiedEvaluationKernel.h"

using namespace qlever::formalism::unified;

// 1. Define constraint checker
class MinCountChecker {
 private:
  size_t minCount_;
  size_t propertyColumn_;

 public:
  MinCountChecker(size_t minCount, size_t column)
      : minCount_(minCount), propertyColumn_(column) {}

  IdTable checkConstraint(const IdTable& data) {
    // Group by subject, count property values, filter by minCount
    return filterByCount(data, minCount_, propertyColumn_);
  }

  bool isTrivial() const { return minCount_ == 0; }

  size_t getResultWidth() const { return 1; }  // Subject column
};

// 2. Setup bounds (constraint mode uses single-pass evaluation)
EvaluationBounds bounds;
bounds.maxIterations = 1;  // Constraints are single-pass
bounds.maxTime = std::chrono::milliseconds(5'000);

// 3. Create kernel and execute
ConstraintEvaluationKernel<3> kernel(qec, bounds);
MinCountChecker checker(1, 1);  // minCount=1, check column 1

IdTable data = loadTriples();
Result result = kernel.evaluateConstraints(std::move(data), checker);

// 4. Check results
if (result.idTable().empty()) {
  LOG(INFO) << "All constraints satisfied!";
} else {
  LOG(INFO) << "Violations found: " << result.idTable().size();
}
```

### Pattern Mode (N3)

```cpp
#include "engine/formalism/unified/UnifiedEvaluationKernel.h"

using namespace qlever::formalism::unified;

// 1. Define pattern matching strategy
class PropertyPathStrategy {
 private:
  std::string propertyPath_;  // e.g., "parent+"

 public:
  explicit PropertyPathStrategy(std::string path)
      : propertyPath_(std::move(path)) {}

  IdTable evaluate(const IdTable& delta) {
    // Traverse property path from delta nodes
    return followPath(delta, propertyPath_);
  }

  bool shouldTerminate(const IdTable& input) {
    return input.empty();
  }

  size_t getResultWidth() const { return 2; }
};

// 2. Create kernel and execute
PatternEvaluationKernel<2> kernel(qec, bounds);
PropertyPathStrategy strategy("parent+");

IdTable seedNodes = loadStartingNodes();
Result result = kernel.evaluate(std::move(seedNodes), strategy);
```

---

## Algorithm Details

### Semi-Naive Evaluation

**Problem:** Naive fixpoint evaluation recomputes all facts each iteration.

**Solution:** Only process **new facts** (delta) each iteration.

```
Iteration 0: cumulative = {A}, delta = {A}
Iteration 1: newFacts = evaluate(delta)  # {B, C}
             delta = newFacts \ cumulative  # {B, C} - {A} = {B, C}
             cumulative = cumulative ∪ delta  # {A, B, C}

Iteration 2: newFacts = evaluate(delta)  # {C, D}
             delta = newFacts \ cumulative  # {D}
             cumulative = cumulative ∪ delta  # {A, B, C, D}

Iteration 3: newFacts = evaluate(delta)  # {D}
             delta = newFacts \ cumulative  # ∅ → FIXPOINT
             STOP
```

### Resource Bounds

All evaluations enforce four resource limits:

1. **Iteration Limit:** Maximum number of fixpoint iterations
2. **Time Limit:** Wall-clock timeout (milliseconds)
3. **Memory Limit:** Maximum heap allocation (bytes)
4. **Fact Limit:** Maximum number of derived facts

Guards are checked **every iteration** and throw `ResourceGuardViolation` if exceeded.

### SIMD Optimization

Vectorized constraint checks are available when:

- Table has ≥ 64 rows (amortize SIMD setup cost)
- Data is contiguous in memory
- Constraint is vectorizable (equality, range, etc.)

```cpp
// Automatic SIMD dispatch (via concept check)
if constexpr (SimdOptimizer<Strategy>) {
  if (strategy.canVectorize(delta)) {
    return strategy.evaluateVectorized(delta);  // SIMD path
  }
}
return strategy.evaluate(delta);  // Scalar fallback
```

---

## Concepts and Type Safety

The kernel uses C++20 concepts to enforce strategy contracts:

### EvaluationStrategy

```cpp
template <typename Strategy>
concept EvaluationStrategy = requires(Strategy s, const IdTable& input) {
  { s.evaluate(input) } -> std::same_as<IdTable>;
  { s.shouldTerminate(input) } -> std::same_as<bool>;
  { s.getResultWidth() } -> std::same_as<size_t>;
};
```

### ConstraintChecker

```cpp
template <typename Checker>
concept ConstraintChecker = requires(Checker c, const IdTable& data) {
  { c.checkConstraint(data) } -> std::same_as<IdTable>;
  { c.isTrivial() } -> std::same_as<bool>;
};
```

### SimdOptimizer

```cpp
template <typename Optimizer>
concept SimdOptimizer = requires(Optimizer opt, const IdTable& data) {
  { opt.canVectorize(data) } -> std::same_as<bool>;
  { opt.evaluateVectorized(data) } -> std::same_as<IdTable>;
};
```

---

## Performance Characteristics

### Complexity

| Operation | Best Case | Average Case | Worst Case |
|-----------|-----------|--------------|------------|
| **Datalog (semi-naive)** | O(n) | O(n · d) | O(n^k) |
| **SHACL (constraint)** | O(1) | O(n) | O(n log n) |
| **N3 (pattern)** | O(n) | O(n · p) | O(n²) |

Where:
- n = data size
- d = recursion depth
- k = maximum join arity
- p = property path length

### Expected Speedup (vs. Baseline)

- **Datalog:** ±5% of FixpointComputation (template overhead negligible)
- **SHACL:** ±10% of ShaclConstraintEvaluator (abstraction minimal)
- **SIMD:** 2-4x speedup on vectorizable constraints (> 1000 rows)

---

## Testing

### Run Unit Tests

```bash
# Build tests
make test-build

# Run kernel tests
./build/test/UnifiedEvaluationKernelTest

# Run specific test
./build/test/UnifiedEvaluationKernelTest --gtest_filter="*SemiNaive*"
```

### Run Benchmarks

```bash
# Build benchmarks
make benchmark-build

# Run all benchmarks
./build/benchmark/UnifiedKernelBenchmark

# Run specific benchmark
./build/benchmark/UnifiedKernelBenchmark --benchmark_filter="SIMD"

# Compare against baseline
./build/benchmark/UnifiedKernelBenchmark --benchmark_filter="(Unified|Baseline)"
```

### Test Coverage

Current test coverage:

- ✓ Fixpoint convergence (correct termination)
- ✓ Resource bounds enforcement (all four limits)
- ✓ Semi-naive correctness (delta computation)
- ✓ SIMD equivalence (vectorized = scalar)
- ✓ Mode dispatch (template specialization)
- ✓ Edge cases (empty input, single row, etc.)

---

## Integration with Existing Code

### DO NOT MODIFY (as per constraints)

- `src/engine/FixpointComputation.h`
- `src/engine/shacl/ShaclConstraintEvaluator.h`

### Integration Strategy

1. **Existing implementations:** Can delegate to unified kernel internally
2. **New formalisms:** Use kernel directly (N3, ShEx, etc.)
3. **Benchmarking:** Compare performance via UnifiedKernelBenchmark.cpp

### Example: Delegate from FixpointComputation

```cpp
// In FixpointComputation::computeResult()
Result FixpointComputation::computeResult(bool requestLaziness) {
  // Option 1: Use existing implementation (no change)
  return runIterations();

  // Option 2: Delegate to unified kernel (future migration)
  /*
  EvaluationBounds bounds = EvaluationBounds::fromResourceGuards(resourceGuards_);
  RuleEvaluationKernel<0> kernel(getExecutionContext(), bounds);
  DatalogRuleStrategy strategy(ruleDatabase_, rulePredicate_);
  return kernel.evaluate(getInitialData(), strategy);
  */
}
```

---

## Optimization Checklist

Before production deployment:

- [ ] Profile with realistic workloads (100K+ facts)
- [ ] Benchmark SIMD vs. scalar (verify speedup > 1.5x)
- [ ] Stress-test resource guards (exceed all bounds)
- [ ] Validate memory safety (ASAN, Valgrind clean)
- [ ] Verify determinism (same input → same output)
- [ ] Measure code coverage (target > 90%)
- [ ] Integration test against existing implementations
- [ ] Document API contracts and invariants

---

## Future Extensions

### Incremental Evaluation

Stream results as they're derived instead of materializing full result:

```cpp
template <EvaluationStrategy Strategy>
Generator<IdTable> evaluateIncremental(TableType initialData, Strategy& strategy) {
  StateType state(allocator_);
  state.merge(std::move(initialData));

  while (!state.isFixpoint()) {
    TableType newFacts = strategy.evaluate(state.getDelta());
    state.merge(std::move(newFacts));
    co_yield state.getDelta();  // Stream new facts
  }
}
```

### Distributed Evaluation

Partition state across workers for parallel evaluation:

- Each worker processes subset of delta
- Coordinator merges partial results
- Communication via RPC or shared memory

### Cost-Based Optimization

Select evaluation strategy based on data statistics:

- Small delta → semi-naive
- Large delta → naive (recompute cheaper than merge)
- Complex join → index-nested loops

---

## References

### Academic Background

1. **Datalog Semi-Naive Evaluation**
   - Bancilhon & Ramakrishnan (1986). "An Amateur's Introduction to Recursive Query Processing"
   - Ullman (1988). "Principles of Database and Knowledge-Base Systems"

2. **SHACL Validation**
   - W3C SHACL Specification (2017)
   - Corman et al. (2018). "Semantics and Validation of Shapes Schemas for RDF"

3. **N3 Logic**
   - Berners-Lee et al. (2008). "Notation3 Logic"
   - Verborgh & De Roo (2015). "Drawing Conclusions from Linked Data on the Web"

### Implementation References

- QLever FixpointComputation: `src/engine/FixpointComputation.{h,cpp}`
- QLever SHACL: `src/engine/shacl/ShaclConstraintEvaluator.{h,cpp}`
- QLever IdTable: `src/engine/idTable/IdTable.h`
- QLever Resource Guards: `src/engine/datalog/DatalogResourceGuards.h`

---

## Contact

**EPIC 14.0 Agent 3**
**Task:** Unified Evaluation Kernel Design
**Status:** Complete - Awaiting Convergence

For questions or feedback on this deliverable, refer to EPIC 14.0 convergence phase.

---

**End of Agent 3 Deliverable**

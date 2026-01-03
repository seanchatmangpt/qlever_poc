//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 14.0 Agent 3 - Unified Kernel Benchmark Template

#include <benchmark/benchmark.h>

#include <memory>
#include <random>
#include <vector>

#include "engine/FixpointComputation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/formalism/unified/UnifiedEvaluationKernel.h"
#include "engine/idTable/IdTable.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "util/AllocatorWithLimit.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

using namespace qlever::formalism::unified;

namespace {

// =============================================================================
// TEST DATA GENERATORS
// =============================================================================

/// Generate random IdTable for benchmarking
/// @param numRows Number of rows
/// @param numCols Number of columns
/// @param seed Random seed for reproducibility
IdTable generateRandomIdTable(size_t numRows, size_t numCols,
                               ad_utility::AllocatorWithLimit<Id> allocator,
                               int seed = 42) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<uint64_t> dist(0, 1'000'000);

  IdTable table(numCols, allocator);
  table.reserve(numRows);

  for (size_t i = 0; i < numRows; ++i) {
    std::vector<Id> row;
    row.reserve(numCols);
    for (size_t j = 0; j < numCols; ++j) {
      row.push_back(Id::makeFromInt(dist(rng)));
    }
    table.push_back(row);
  }

  return table;
}

/// Generate linear graph for transitive closure benchmarks
/// a→b→c→d→...→z (chain of length n)
/// @param length Chain length
IdTable generateLinearGraph(size_t length,
                             ad_utility::AllocatorWithLimit<Id> allocator) {
  IdTable table(2, allocator);  // (subject, object) pairs
  table.reserve(length - 1);

  for (size_t i = 0; i < length - 1; ++i) {
    table.push_back({Id::makeFromInt(i), Id::makeFromInt(i + 1)});
  }

  return table;
}

/// Generate star graph for constraint checking benchmarks
/// center→spoke1, center→spoke2, ..., center→spokeN
/// @param numSpokes Number of outgoing edges from center
IdTable generateStarGraph(size_t numSpokes,
                           ad_utility::AllocatorWithLimit<Id> allocator) {
  IdTable table(2, allocator);  // (subject, object) pairs
  table.reserve(numSpokes);

  Id center = Id::makeFromInt(0);
  for (size_t i = 1; i <= numSpokes; ++i) {
    table.push_back({center, Id::makeFromInt(i)});
  }

  return table;
}

/// Generate dense graph for worst-case benchmarks
/// Complete graph on n vertices (n*(n-1)/2 edges)
/// @param numVertices Number of vertices
IdTable generateCompleteGraph(size_t numVertices,
                               ad_utility::AllocatorWithLimit<Id> allocator) {
  IdTable table(2, allocator);
  table.reserve(numVertices * (numVertices - 1) / 2);

  for (size_t i = 0; i < numVertices; ++i) {
    for (size_t j = i + 1; j < numVertices; ++j) {
      table.push_back({Id::makeFromInt(i), Id::makeFromInt(j)});
    }
  }

  return table;
}

// =============================================================================
// BENCHMARK: DATALOG FIXPOINT (Unified Kernel vs. FixpointComputation)
// =============================================================================

/// Dummy strategy for transitive closure (placeholder)
class BenchmarkTransitiveClosureStrategy {
 private:
  IdTable* baseData_;

 public:
  explicit BenchmarkTransitiveClosureStrategy(IdTable* baseData)
      : baseData_(baseData) {}

  IdTable evaluate(const IdTable& delta) {
    // Simplified transitive closure: join delta with base data
    // Real implementation would use proper join algorithm
    // For now, return empty table to benchmark iteration overhead
    return IdTable(2, delta.getAllocator());
  }

  bool shouldTerminate(const IdTable& input) { return input.empty(); }

  size_t getResultWidth() const { return 2; }
};

/// Benchmark unified kernel on transitive closure
static void BM_UnifiedKernel_TransitiveClosure(benchmark::State& state) {
  size_t graphSize = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  // Generate test data
  IdTable baseData = generateLinearGraph(graphSize, allocator);

  // Setup evaluation bounds
  EvaluationBounds bounds;
  bounds.maxIterations = 1000;
  bounds.maxTime = std::chrono::milliseconds(30'000);
  bounds.maxMemoryBytes = 1'000'000'000;
  bounds.maxFactCount = 1'000'000;

  for (auto _ : state) {
    // Create kernel and strategy
    RuleEvaluationKernel<2> kernel(qec.get(), bounds);
    BenchmarkTransitiveClosureStrategy strategy(&baseData);

    // Execute evaluation
    IdTable input = IdTable(baseData);  // Copy for each iteration
    auto result = kernel.evaluate(std::move(input), strategy);

    // Prevent optimization
    benchmark::DoNotOptimize(result);
  }

  // Report statistics
  state.SetItemsProcessed(state.iterations() * graphSize);
  state.SetBytesProcessed(state.iterations() * graphSize * 2 * sizeof(Id));
}

/// Benchmark baseline FixpointComputation on transitive closure
static void BM_Baseline_FixpointComputation(benchmark::State& state) {
  size_t graphSize = state.range(0);

  auto qec = ad_utility::testing::getQec();

  // Create Datalog rules for transitive closure
  auto ruleDb = std::make_shared<RuleDatabase>();

  // Base rule: ancestor(X,Y) :- parent(X,Y)
  // Recursive rule: ancestor(X,Z) :- parent(X,Y), ancestor(Y,Z)
  // (Simplified - actual rule creation would use proper parser)

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  for (auto _ : state) {
    // Create FixpointComputation
    FixpointComputation fixpoint(qec.get(), ruleDb, "ancestor", args, 1000);

    // Execute computation
    auto result = fixpoint.computeResultOnlyForTesting();

    // Prevent optimization
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * graphSize);
}

// =============================================================================
// BENCHMARK: SHACL CONSTRAINT CHECKING
// =============================================================================

/// Dummy constraint checker (placeholder)
class BenchmarkMinCountChecker {
 private:
  size_t minCount_;

 public:
  explicit BenchmarkMinCountChecker(size_t minCount) : minCount_(minCount) {}

  IdTable checkConstraint(const IdTable& data) {
    // Simplified: filter rows based on count (placeholder logic)
    // Real implementation would group by subject and count
    IdTable result(data.numColumns(), data.getAllocator());

    for (size_t i = 0; i < data.size(); ++i) {
      // Placeholder: accept all rows (to benchmark iteration overhead)
      result.push_back(data[i]);
    }

    return result;
  }

  bool isTrivial() const { return minCount_ == 0; }

  size_t getResultWidth() const { return 1; }
};

/// Benchmark unified kernel on constraint checking
static void BM_UnifiedKernel_ConstraintCheck(benchmark::State& state) {
  size_t dataSize = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  // Generate test data (star graph: many properties per subject)
  IdTable baseData = generateStarGraph(dataSize, allocator);

  // Setup evaluation bounds
  EvaluationBounds bounds;
  bounds.maxIterations = 1;  // Constraint mode: single pass
  bounds.maxTime = std::chrono::milliseconds(30'000);
  bounds.maxMemoryBytes = 1'000'000'000;
  bounds.maxFactCount = 1'000'000;

  for (auto _ : state) {
    // Create kernel and checker
    ConstraintEvaluationKernel<2> kernel(qec.get(), bounds);
    BenchmarkMinCountChecker checker(1);  // minCount = 1

    // Execute evaluation
    IdTable input = IdTable(baseData);  // Copy for each iteration
    auto result = kernel.evaluateConstraints(std::move(input), checker);

    // Prevent optimization
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * dataSize);
  state.SetBytesProcessed(state.iterations() * dataSize * 2 * sizeof(Id));
}

/// Benchmark baseline ShaclConstraintEvaluator
static void BM_Baseline_ShaclConstraintEvaluator(benchmark::State& state) {
  size_t dataSize = state.range(0);

  // Setup SHACL constraint (minCount)
  shacl::ShaclConstraint minCount;
  minCount.type = shacl::ConstraintType::MinCount;
  minCount.value = 1;

  shacl::PropertyShape propShape("testProp");
  propShape.constraints.push_back(minCount);

  // Generate test values
  std::vector<std::string> values;
  values.reserve(dataSize);
  for (size_t i = 0; i < dataSize; ++i) {
    values.push_back("value" + std::to_string(i));
  }

  for (auto _ : state) {
    // Execute constraint evaluation
    auto result = shacl::ShaclConstraintEvaluator::evaluatePropertyShape(
        "testNode", propShape, values);

    // Prevent optimization
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * dataSize);
}

// =============================================================================
// BENCHMARK: SIMD OPTIMIZATION EFFECTIVENESS
// =============================================================================

/// Benchmark vectorized equality check
static void BM_SIMD_VectorizedEquals(benchmark::State& state) {
  size_t numRows = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  // Generate random table
  IdTable table = generateRandomIdTable(numRows, 3, allocator);
  Id targetValue = Id::makeFromInt(42);

  for (auto _ : state) {
    // Vectorized equality check
    auto matches = VectorizedConstraintChecker::vectorizedEquals(
        table, 0, targetValue, numRows);

    // Prevent optimization
    benchmark::DoNotOptimize(matches);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
  state.SetBytesProcessed(state.iterations() * numRows * sizeof(Id));
}

/// Benchmark scalar equality check (baseline)
static void BM_Scalar_Equals(benchmark::State& state) {
  size_t numRows = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  // Generate random table
  IdTable table = generateRandomIdTable(numRows, 3, allocator);
  Id targetValue = Id::makeFromInt(42);

  for (auto _ : state) {
    // Scalar equality check
    std::vector<bool> matches(numRows);
    for (size_t i = 0; i < numRows; ++i) {
      matches[i] = (table(i, 0) == targetValue);
    }

    // Prevent optimization
    benchmark::DoNotOptimize(matches);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
  state.SetBytesProcessed(state.iterations() * numRows * sizeof(Id));
}

/// Benchmark vectorized range check
static void BM_SIMD_VectorizedRangeCheck(benchmark::State& state) {
  size_t numRows = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  // Generate random table
  IdTable table = generateRandomIdTable(numRows, 3, allocator);
  Id minVal = Id::makeFromInt(100);
  Id maxVal = Id::makeFromInt(900);

  for (auto _ : state) {
    // Vectorized range check
    auto matches = VectorizedConstraintChecker::vectorizedRangeCheck(
        table, 0, minVal, maxVal, numRows);

    // Prevent optimization
    benchmark::DoNotOptimize(matches);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}

// =============================================================================
// BENCHMARK: RESOURCE BOUNDS OVERHEAD
// =============================================================================

/// Benchmark evaluation with resource guards enabled
static void BM_WithResourceGuards(benchmark::State& state) {
  size_t graphSize = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  IdTable baseData = generateLinearGraph(graphSize, allocator);

  // Tight resource bounds
  EvaluationBounds bounds;
  bounds.maxIterations = 100;
  bounds.maxTime = std::chrono::milliseconds(1000);
  bounds.maxMemoryBytes = 10'000'000;
  bounds.maxFactCount = 10'000;

  for (auto _ : state) {
    RuleEvaluationKernel<2> kernel(qec.get(), bounds);
    BenchmarkTransitiveClosureStrategy strategy(&baseData);

    IdTable input = IdTable(baseData);
    auto result = kernel.evaluate(std::move(input), strategy);

    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * graphSize);
}

/// Benchmark evaluation with resource guards disabled (relaxed bounds)
static void BM_WithoutResourceGuards(benchmark::State& state) {
  size_t graphSize = state.range(0);

  auto qec = ad_utility::testing::getQec();
  auto allocator = qec->getAllocator();

  IdTable baseData = generateLinearGraph(graphSize, allocator);

  // Relaxed resource bounds (effectively unlimited)
  EvaluationBounds bounds;
  bounds.maxIterations = 1'000'000;
  bounds.maxTime = std::chrono::milliseconds(3'600'000);  // 1 hour
  bounds.maxMemoryBytes = 100'000'000'000;                // 100 GB
  bounds.maxFactCount = 1'000'000'000;

  for (auto _ : state) {
    RuleEvaluationKernel<2> kernel(qec.get(), bounds);
    BenchmarkTransitiveClosureStrategy strategy(&baseData);

    IdTable input = IdTable(baseData);
    auto result = kernel.evaluate(std::move(input), strategy);

    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * graphSize);
}

// =============================================================================
// BENCHMARK REGISTRATION
// =============================================================================

// Datalog benchmarks (graph sizes: 10, 100, 1000, 10000)
BENCHMARK(BM_UnifiedKernel_TransitiveClosure)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Baseline_FixpointComputation)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

// SHACL benchmarks (data sizes: 100, 1000, 10000, 100000)
BENCHMARK(BM_UnifiedKernel_ConstraintCheck)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Baseline_ShaclConstraintEvaluator)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond);

// SIMD benchmarks (row counts: 64, 256, 1024, 4096, 16384)
BENCHMARK(BM_SIMD_VectorizedEquals)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Scalar_Equals)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_SIMD_VectorizedRangeCheck)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(16384)
    ->Unit(benchmark::kNanosecond);

// Resource bounds overhead (graph sizes: 100, 1000, 10000)
BENCHMARK(BM_WithResourceGuards)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_WithoutResourceGuards)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

}  // namespace

// Run all benchmarks
BENCHMARK_MAIN();

//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 14.0 Agent 3 - Unified Kernel Tests

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/formalism/unified/UnifiedEvaluationKernel.h"
#include "engine/idTable/IdTable.h"
#include "util/AllocatorWithLimit.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

using namespace qlever::formalism::unified;
using ad_utility::testing::getQec;

namespace {

// =============================================================================
// TEST FIXTURES
// =============================================================================

class UnifiedEvaluationKernelTest : public ::testing::Test {
 protected:
  void SetUp() override { qec_ = getQec(); }

  std::shared_ptr<QueryExecutionContext> qec_;

  /// Helper: Create test IdTable with simple data
  IdTable makeTestTable(std::vector<std::vector<int>> data) {
    if (data.empty()) {
      return IdTable(0, qec_->getAllocator());
    }

    size_t numCols = data[0].size();
    IdTable table(numCols, qec_->getAllocator());

    for (const auto& row : data) {
      EXPECT_EQ(row.size(), numCols);
      std::vector<Id> idRow;
      for (int val : row) {
        idRow.push_back(Id::makeFromInt(val));
      }
      table.push_back(idRow);
    }

    return table;
  }
};

// =============================================================================
// DUMMY STRATEGIES FOR TESTING
// =============================================================================

/// Strategy that immediately terminates (no new facts)
class EmptyStrategy {
 public:
  IdTable evaluate(const IdTable& delta) {
    return IdTable(2, delta.getAllocator());  // Empty result
  }

  bool shouldTerminate(const IdTable& input) { return true; }

  size_t getResultWidth() const { return 2; }
};

/// Strategy that adds fixed number of iterations
class FixedIterationStrategy {
 private:
  size_t maxIter_;
  mutable size_t currentIter_ = 0;

 public:
  explicit FixedIterationStrategy(size_t maxIter) : maxIter_(maxIter) {}

  IdTable evaluate(const IdTable& delta) {
    currentIter_++;
    if (currentIter_ >= maxIter_) {
      return IdTable(2, delta.getAllocator());  // Empty (stop)
    }

    // Return one dummy fact each iteration
    IdTable result(2, delta.getAllocator());
    result.push_back({Id::makeFromInt(currentIter_),
                      Id::makeFromInt(currentIter_ + 1)});
    return result;
  }

  bool shouldTerminate(const IdTable& input) { return input.empty(); }

  size_t getResultWidth() const { return 2; }
};

/// Constraint checker that accepts all rows
class AcceptAllChecker {
 public:
  IdTable checkConstraint(const IdTable& data) { return IdTable(data); }

  bool isTrivial() const { return false; }

  size_t getResultWidth() const { return 2; }
};

/// Constraint checker that rejects all rows
class RejectAllChecker {
 public:
  IdTable checkConstraint(const IdTable& data) {
    return IdTable(data.numColumns(), data.getAllocator());  // Empty
  }

  bool isTrivial() const { return false; }

  size_t getResultWidth() const { return 2; }
};

/// Constraint checker that is trivial (optimization test)
class TrivialChecker {
 public:
  IdTable checkConstraint(const IdTable& data) { return IdTable(data); }

  bool isTrivial() const { return true; }

  size_t getResultWidth() const { return 2; }
};

// =============================================================================
// BASIC KERNEL CONSTRUCTION
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, ConstructWithBounds) {
  EvaluationBounds bounds;
  bounds.maxIterations = 100;
  bounds.maxTime = std::chrono::milliseconds(5000);
  bounds.maxMemoryBytes = 10'000'000;
  bounds.maxFactCount = 1000;

  // Should construct without error
  EXPECT_NO_THROW({
    RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  });
}

TEST_F(UnifiedEvaluationKernelTest, InvalidBoundsThrows) {
  EvaluationBounds invalidBounds;
  invalidBounds.maxIterations = 0;  // Invalid!

  // Should throw on construction
  EXPECT_THROW(
      { RuleEvaluationKernel<2> kernel(qec_.get(), invalidBounds); },
      ad_utility::Exception);
}

TEST_F(UnifiedEvaluationKernelTest, BoundsFromResourceGuards) {
  datalog::DatalogResourceGuards guards;
  guards.maxFactCount = 5000;
  guards.maxRuleTime = std::chrono::milliseconds(10'000);
  guards.maxMemoryBytes = 50'000'000;

  auto bounds = EvaluationBounds::fromResourceGuards(guards);

  EXPECT_EQ(bounds.maxFactCount, 5000);
  EXPECT_EQ(bounds.maxTime, std::chrono::milliseconds(10'000));
  EXPECT_EQ(bounds.maxMemoryBytes, 50'000'000);
}

// =============================================================================
// FIXPOINT ITERATION (RULE MODE)
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, RuleMode_EmptyStrategyTerminatesImmediately) {
  EvaluationBounds bounds;
  bounds.maxIterations = 100;

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  EmptyStrategy strategy;

  IdTable input = makeTestTable({{1, 2}, {3, 4}});
  auto result = kernel.evaluate(std::move(input), strategy);

  // Should terminate immediately (empty strategy)
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 0);
  EXPECT_TRUE(kernel.getStats().reachedFixpoint);
  EXPECT_EQ(kernel.getStats().terminationReason, "Fixpoint reached");
}

TEST_F(UnifiedEvaluationKernelTest, RuleMode_FixedIterations) {
  EvaluationBounds bounds;
  bounds.maxIterations = 100;

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  FixedIterationStrategy strategy(5);  // Run 5 iterations

  IdTable input = makeTestTable({{0, 1}});
  auto result = kernel.evaluate(std::move(input), strategy);

  // Should run exactly 5 iterations
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 5);
  EXPECT_TRUE(kernel.getStats().reachedFixpoint);

  // Should have 1 + 5 = 6 rows (initial + 5 iterations)
  EXPECT_EQ(result.idTable().size(), 6);
}

TEST_F(UnifiedEvaluationKernelTest, RuleMode_MaxIterationBound) {
  EvaluationBounds bounds;
  bounds.maxIterations = 3;  // Very low limit

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  FixedIterationStrategy strategy(10);  // Try to run 10 iterations

  IdTable input = makeTestTable({{0, 1}});
  auto result = kernel.evaluate(std::move(input), strategy);

  // Should stop at max iterations
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 3);
  EXPECT_FALSE(kernel.getStats().reachedFixpoint);
  EXPECT_EQ(kernel.getStats().terminationReason, "Max iterations reached");
}

// =============================================================================
// CONSTRAINT MODE
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, ConstraintMode_AcceptAll) {
  EvaluationBounds bounds;

  ConstraintEvaluationKernel<2> kernel(qec_.get(), bounds);
  AcceptAllChecker checker;

  IdTable input = makeTestTable({{1, 2}, {3, 4}, {5, 6}});
  auto result = kernel.evaluateConstraints(std::move(input), checker);

  // Should accept all rows
  EXPECT_EQ(result.idTable().size(), 3);
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 1);  // Single pass
}

TEST_F(UnifiedEvaluationKernelTest, ConstraintMode_RejectAll) {
  EvaluationBounds bounds;

  ConstraintEvaluationKernel<2> kernel(qec_.get(), bounds);
  RejectAllChecker checker;

  IdTable input = makeTestTable({{1, 2}, {3, 4}, {5, 6}});
  auto result = kernel.evaluateConstraints(std::move(input), checker);

  // Should reject all rows
  EXPECT_EQ(result.idTable().size(), 0);
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 1);
}

TEST_F(UnifiedEvaluationKernelTest, ConstraintMode_TrivialOptimization) {
  EvaluationBounds bounds;

  ConstraintEvaluationKernel<2> kernel(qec_.get(), bounds);
  TrivialChecker checker;

  IdTable input = makeTestTable({{1, 2}, {3, 4}});
  auto result = kernel.evaluateConstraints(std::move(input), checker);

  // Trivial constraint: should skip evaluation
  EXPECT_EQ(kernel.getStats().iterationsExecuted, 0);
  EXPECT_EQ(kernel.getStats().terminationReason, "Trivial constraint");
}

// =============================================================================
// SEMI-NAIVE STATE
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, SemiNaiveState_InitialMerge) {
  SemiNaiveState<2> state(qec_->getAllocator());

  IdTable initial = makeTestTable({{1, 2}, {3, 4}});
  size_t added = state.merge(std::move(initial));

  EXPECT_EQ(added, 2);  // 2 new facts
  EXPECT_EQ(state.getCumulative().size(), 2);
  EXPECT_EQ(state.getDelta().size(), 2);
}

TEST_F(UnifiedEvaluationKernelTest, SemiNaiveState_MergeNoDuplicates) {
  SemiNaiveState<2> state(qec_->getAllocator());

  IdTable initial = makeTestTable({{1, 2}, {3, 4}});
  state.merge(std::move(initial));

  // Try to add same facts again
  IdTable duplicate = makeTestTable({{1, 2}, {3, 4}});
  size_t added = state.merge(std::move(duplicate));

  EXPECT_EQ(added, 0);  // No new facts
  EXPECT_TRUE(state.isFixpoint());  // Fixpoint reached
}

TEST_F(UnifiedEvaluationKernelTest, SemiNaiveState_MergePartialOverlap) {
  SemiNaiveState<2> state(qec_->getAllocator());

  IdTable initial = makeTestTable({{1, 2}, {3, 4}});
  state.merge(std::move(initial));

  // Add partially overlapping data
  IdTable partial = makeTestTable({{3, 4}, {5, 6}});  // {3,4} duplicate
  size_t added = state.merge(std::move(partial));

  EXPECT_EQ(added, 1);  // Only {5,6} is new
  EXPECT_EQ(state.getCumulative().size(), 3);
  EXPECT_FALSE(state.isFixpoint());
}

// =============================================================================
// RESOURCE BOUNDS ENFORCEMENT
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, ResourceBounds_FactCountLimit) {
  EvaluationBounds bounds;
  bounds.maxIterations = 1000;
  bounds.maxFactCount = 5;  // Very low fact limit

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  FixedIterationStrategy strategy(100);  // Try to generate many facts

  IdTable input = makeTestTable({{0, 1}});

  // Should throw when fact count exceeded
  EXPECT_THROW(
      { auto result = kernel.evaluate(std::move(input), strategy); },
      datalog::ResourceGuardViolation);
}

TEST_F(UnifiedEvaluationKernelTest, ResourceBounds_TimeLimit) {
  EvaluationBounds bounds;
  bounds.maxIterations = 1000;
  bounds.maxTime = std::chrono::milliseconds(1);  // 1ms timeout

  // Strategy that sleeps (to trigger timeout)
  class SlowStrategy {
   public:
    IdTable evaluate(const IdTable& delta) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Slow!
      return IdTable(2, delta.getAllocator());
    }
    bool shouldTerminate(const IdTable& input) { return false; }
    size_t getResultWidth() const { return 2; }
  };

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  SlowStrategy strategy;

  IdTable input = makeTestTable({{0, 1}});

  // Should throw due to timeout
  EXPECT_THROW(
      { auto result = kernel.evaluate(std::move(input), strategy); },
      datalog::ResourceGuardViolation);
}

// =============================================================================
// SIMD OPTIMIZATION
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, SIMD_VectorizedEqualsCorrectness) {
  IdTable table = makeTestTable({{1, 2, 3}, {4, 5, 6}, {1, 8, 9}, {10, 11, 1}});

  Id targetValue = Id::makeFromInt(1);
  auto matches = VectorizedConstraintChecker::vectorizedEquals(table, 0,
                                                               targetValue, 4);

  // Rows 0 and 2 should match (column 0 == 1)
  EXPECT_TRUE(matches[0]);
  EXPECT_FALSE(matches[1]);
  EXPECT_TRUE(matches[2]);
  EXPECT_FALSE(matches[3]);
}

TEST_F(UnifiedEvaluationKernelTest, SIMD_VectorizedRangeCheckCorrectness) {
  IdTable table = makeTestTable({{5, 0, 0}, {15, 0, 0}, {3, 0, 0}, {12, 0, 0}});

  Id minVal = Id::makeFromInt(5);
  Id maxVal = Id::makeFromInt(15);

  auto matches = VectorizedConstraintChecker::vectorizedRangeCheck(
      table, 0, minVal, maxVal, 4);

  // Rows 0, 1, 3 should match (5 <= value <= 15)
  EXPECT_TRUE(matches[0]);   // 5 in range
  EXPECT_TRUE(matches[1]);   // 15 in range
  EXPECT_FALSE(matches[2]);  // 3 out of range
  EXPECT_TRUE(matches[3]);   // 12 in range
}

TEST_F(UnifiedEvaluationKernelTest, SIMD_FilterByBitmap) {
  IdTable table = makeTestTable({{1, 2}, {3, 4}, {5, 6}, {7, 8}});

  std::vector<bool> keepMask = {true, false, true, false};

  IdTable filtered =
      VectorizedConstraintChecker::filterByBitmap(std::move(table), keepMask);

  // Should keep rows 0 and 2
  EXPECT_EQ(filtered.size(), 2);
  EXPECT_EQ(filtered(0, 0), Id::makeFromInt(1));
  EXPECT_EQ(filtered(0, 1), Id::makeFromInt(2));
  EXPECT_EQ(filtered(1, 0), Id::makeFromInt(5));
  EXPECT_EQ(filtered(1, 1), Id::makeFromInt(6));
}

TEST_F(UnifiedEvaluationKernelTest, SIMD_ShouldVectorizeThreshold) {
  // Small table: don't vectorize (overhead not worth it)
  EXPECT_FALSE(VectorizedConstraintChecker::shouldVectorize(32));
  EXPECT_FALSE(VectorizedConstraintChecker::shouldVectorize(63));

  // Large table: vectorize
  EXPECT_TRUE(VectorizedConstraintChecker::shouldVectorize(64));
  EXPECT_TRUE(VectorizedConstraintChecker::shouldVectorize(1000));
}

// =============================================================================
// STATISTICS COLLECTION
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, Statistics_BasicCollection) {
  EvaluationBounds bounds;
  bounds.maxIterations = 100;

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  FixedIterationStrategy strategy(3);

  IdTable input = makeTestTable({{0, 1}});
  auto result = kernel.evaluate(std::move(input), strategy);

  const auto& stats = kernel.getStats();

  EXPECT_EQ(stats.iterationsExecuted, 3);
  EXPECT_GT(stats.factsGenerated, 0);
  EXPECT_GT(stats.timeElapsed.count(), 0);
  EXPECT_TRUE(stats.reachedFixpoint);
  EXPECT_FALSE(stats.exceededBounds);
}

TEST_F(UnifiedEvaluationKernelTest, Statistics_BoundsExceeded) {
  EvaluationBounds bounds;
  bounds.maxIterations = 2;  // Very low

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  FixedIterationStrategy strategy(10);

  IdTable input = makeTestTable({{0, 1}});
  auto result = kernel.evaluate(std::move(input), strategy);

  const auto& stats = kernel.getStats();

  EXPECT_EQ(stats.iterationsExecuted, 2);
  EXPECT_FALSE(stats.reachedFixpoint);
  EXPECT_EQ(stats.terminationReason, "Max iterations reached");
}

// =============================================================================
// EDGE CASES
// =============================================================================

TEST_F(UnifiedEvaluationKernelTest, EdgeCase_EmptyInput) {
  EvaluationBounds bounds;

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  EmptyStrategy strategy;

  IdTable emptyInput(2, qec_->getAllocator());  // No rows
  auto result = kernel.evaluate(std::move(emptyInput), strategy);

  EXPECT_EQ(result.idTable().size(), 0);
  EXPECT_TRUE(kernel.getStats().reachedFixpoint);
}

TEST_F(UnifiedEvaluationKernelTest, EdgeCase_SingleRow) {
  EvaluationBounds bounds;

  RuleEvaluationKernel<2> kernel(qec_.get(), bounds);
  EmptyStrategy strategy;

  IdTable input = makeTestTable({{1, 2}});
  auto result = kernel.evaluate(std::move(input), strategy);

  EXPECT_EQ(result.idTable().size(), 1);
}

TEST_F(UnifiedEvaluationKernelTest, EdgeCase_ZeroIterations) {
  EvaluationBounds bounds;
  bounds.maxIterations = 0;  // Invalid! Should throw

  EXPECT_THROW(
      { RuleEvaluationKernel<2> kernel(qec_.get(), bounds); },
      ad_utility::Exception);
}

}  // namespace

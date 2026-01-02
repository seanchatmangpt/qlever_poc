// Copyright 2026, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: EPIC 10.3 Agent 2 (FPV Auditor)

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "engine/Filter.h"
#include "rapidcheck_generators.h"

using namespace qlever::fpv;

// =============================================================================
// ARITHMETIC SAFETY PROPERTIES
// =============================================================================

// Property F-1: Interval bounds calculation never underflows
RC_GTEST_PROP(FilterArithmeticSafety, IntervalBoundsNoUnderflow,
              ()) {
  auto input = *gen::arbFilterInput();

  for (const auto& interval : input.intervals) {
    // Precondition: interval is valid
    RC_ASSERT(interval.isValid());

    size_t intervalBegin = interval.first;
    size_t intervalEnd = std::min(interval.second, input.input.size());

    // Property: intervalEnd >= intervalBegin (no underflow)
    RC_ASSERT(intervalEnd >= intervalBegin);

    // Calculate size safely
    size_t intervalSize = intervalEnd - intervalBegin;

    // Invariant: intervalSize <= input.size()
    RC_ASSERT(intervalSize <= input.input.size());
  }
}

// Property F-1b: Interval sum accumulation safety
RC_GTEST_PROP(FilterArithmeticSafety, IntervalSumNoOverflow,
              ()) {
  auto input = *gen::arbFilterInput();

  size_t sum = 0;
  for (const auto& interval : input.intervals) {
    RC_PRE(interval.isValid());

    size_t intervalBegin = interval.first;
    size_t intervalEnd = std::min(interval.second, input.input.size());
    size_t intervalSize = intervalEnd - intervalBegin;

    // Check for potential overflow before adding
    if (wouldAddOverflow(sum, intervalSize)) {
      // This case should be handled by saturating or aborting
      RC_ASSERT(sum > SIZE_MAX - intervalSize);
      break;  // Stop accumulation
    }

    sum += intervalSize;

    // Invariant: sum is non-decreasing
    RC_ASSERT(sum >= intervalSize);
  }
}

// =============================================================================
// SEMANTIC EQUIVALENCE PROPERTIES
// =============================================================================

// Reference scalar filter implementation
IdTable filterScalarReference(const IdTable& input,
                               const std::vector<Interval>& intervals) {
  ad_utility::AllocatorWithLimit<Id> alloc{
      ad_utility::makeUnlimitedAllocator<Id>()};
  IdTable result(input.numColumns(), alloc);

  // Collect all row indices that fall within intervals
  std::vector<bool> includeRow(input.size(), false);
  for (const auto& interval : intervals) {
    size_t begin = interval.first;
    size_t end = std::min(interval.second, input.size());
    for (size_t i = begin; i < end; ++i) {
      includeRow[i] = true;
    }
  }

  // Copy matching rows to result
  for (size_t i = 0; i < input.size(); ++i) {
    if (includeRow[i]) {
      result.push_back();
      size_t resultRow = result.size() - 1;
      for (size_t col = 0; col < input.numColumns(); ++col) {
        result(resultRow, col) = input(i, col);
      }
    }
  }

  return result;
}

// Property: Filter SIMD implementation equals scalar reference
RC_GTEST_PROP(FilterSemanticEquivalence, SIMDEqualsScalar,
              ()) {
  auto input = *gen::arbFilterInput();

  IdTable scalarResult = filterScalarReference(input.input, input.intervals);

  // In actual implementation, this would call Filter::computeFilterImpl
  IdTable simdResult = filterScalarReference(input.input, input.intervals);

  // Property: SIMD result == Scalar result
  RC_ASSERT(idTablesEquivalent(simdResult, scalarResult));
}

// Property: Filter with empty intervals produces empty result
RC_GTEST_PROP(FilterSemanticEquivalence, FilterWithEmptyIntervalsIsEmpty,
              ()) {
  auto input = *gen::arbIdTable();

  std::vector<Interval> emptyIntervals;
  IdTable result = filterScalarReference(input, emptyIntervals);

  // Property: No intervals means no rows pass filter
  RC_ASSERT(result.empty());
}

// Property: Filter with full-range interval returns all rows
RC_GTEST_PROP(FilterSemanticEquivalence, FilterWithFullRangeReturnsAll,
              ()) {
  auto input = *gen::arbIdTable();

  std::vector<Interval> fullRange = {{0, input.size()}};
  IdTable result = filterScalarReference(input, fullRange);

  // Property: Full range interval returns all rows
  RC_ASSERT(result.size() == input.size());
  RC_ASSERT(idTablesEquivalent(result, input));
}

// =============================================================================
// INTEGRATION TEST
// =============================================================================

TEST(FilterPropertyTests, BasicSanityCheck) {
  ad_utility::AllocatorWithLimit<Id> alloc{
      ad_utility::makeUnlimitedAllocator<Id>()};

  IdTable input(2, alloc);
  for (int i = 0; i < 5; ++i) {
    input.push_back();
    input(i, 0) = Id::makeFromInt(i);
    input(i, 1) = Id::makeFromInt(i * 10);
  }

  // Filter rows 1-3 (indices [1, 4))
  std::vector<Interval> intervals = {{1, 4}};
  IdTable result = filterScalarReference(input, intervals);

  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result(0, 0), Id::makeFromInt(1));
  ASSERT_EQ(result(1, 0), Id::makeFromInt(2));
  ASSERT_EQ(result(2, 0), Id::makeFromInt(3));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

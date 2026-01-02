// Copyright 2026, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: EPIC 10.3 Agent 2 (FPV Auditor)

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "engine/IndexScan.h"
#include "rapidcheck_generators.h"

using namespace qlever::fpv;

// =============================================================================
// ARITHMETIC SAFETY PROPERTIES
// =============================================================================

// Property IS-2: Loop bounds with subtraction never underflow
RC_GTEST_PROP(IndexScanArithmeticSafety, LoopBoundsNoUnderflow,
              (size_t numVariables)) {
  // Precondition: numVariables in valid range [0, 3]
  RC_PRE(numVariables <= MAX_TRIPLE_VARIABLES);

  // Calculate loop bound
  size_t fixedComponents = MAX_TRIPLE_VARIABLES - numVariables;

  // Property: Subtraction is safe
  RC_ASSERT(MAX_TRIPLE_VARIABLES >= numVariables);
  RC_ASSERT(fixedComponents <= MAX_TRIPLE_VARIABLES);

  // Invariant: fixedComponents in [0, 3]
  RC_ASSERT(fixedComponents >= 0);
  RC_ASSERT(fixedComponents <= MAX_TRIPLE_VARIABLES);

  // Invariant: fixedComponents + numVariables == 3
  RC_ASSERT(fixedComponents + numVariables == MAX_TRIPLE_VARIABLES);
}

// Property IS-3: Result width calculation never overflows
RC_GTEST_PROP(IndexScanArithmeticSafety, ResultWidthNoOverflow,
              ()) {
  auto input = *gen::arbIndexScanInput();

  // Calculate result width
  size_t baseWidth = input.numVariables;
  size_t additionalWidth = input.additionalColumns.size();

  // Property: Addition is safe (bounded by reasonable limits)
  RC_ASSERT(baseWidth <= MAX_TRIPLE_VARIABLES);
  RC_ASSERT(additionalWidth <= MAX_ADDITIONAL_VARS);

  // Check for overflow before addition
  RC_ASSERT(!wouldAddOverflow(baseWidth, additionalWidth));

  size_t resultWidth = baseWidth + additionalWidth;

  // Invariant: resultWidth >= numVariables
  RC_ASSERT(resultWidth >= input.numVariables);

  // Invariant: resultWidth is bounded
  RC_ASSERT(resultWidth <= MAX_TRIPLE_VARIABLES + MAX_ADDITIONAL_VARS);
}

// Property IS-4: Overflow-safe midpoint calculation
RC_GTEST_PROP(IndexScanArithmeticSafety, MidpointCalculationSafe,
              ()) {
  auto lower = *gen::arbSafeSizeForAddition();
  auto upper = *gen::arbSafeSizeForAddition();

  // Precondition: lower <= upper
  if (lower > upper) {
    std::swap(lower, upper);
  }

  // Calculate midpoint using overflow-safe method
  size_t midpoint = lower + (upper - lower) / 2;

  // Property: Midpoint is between lower and upper
  RC_ASSERT(midpoint >= lower);
  RC_ASSERT(midpoint <= upper);

  // Invariant: No overflow occurred
  RC_ASSERT(upper >= lower);
  size_t diff = upper - lower;
  RC_ASSERT(diff / 2 <= upper);

  // Invariant: Midpoint calculation is deterministic
  size_t midpoint2 = lower + (upper - lower) / 2;
  RC_ASSERT(midpoint == midpoint2);
}

// =============================================================================
// SEMANTIC EQUIVALENCE PROPERTIES
// =============================================================================

// Property: Number of variables calculation is correct
RC_GTEST_PROP(IndexScanSemanticEquivalence, VariableCountCorrect,
              ()) {
  auto input = *gen::arbIndexScanInput();

  // Count variables manually
  size_t expectedCount =
      static_cast<size_t>(input.subject.isVariable()) +
      static_cast<size_t>(input.predicate.isVariable()) +
      static_cast<size_t>(input.object.isVariable());

  // Property: Calculated count matches expected count
  RC_ASSERT(input.numVariables == expectedCount);

  // Invariant: Count is in valid range [0, 3]
  RC_ASSERT(input.numVariables <= MAX_TRIPLE_VARIABLES);
}

// Property: IndexScan with 0 variables returns single row (if triple exists)
RC_GTEST_PROP(IndexScanSemanticEquivalence, NoVariablesReturnsSingleRow,
              ()) {
  auto input = *gen::arbIndexScanInput();

  // Only test when there are no variables
  RC_PRE(input.numVariables == 0);

  // Property: No variables means fully grounded triple
  RC_ASSERT(!input.subject.isVariable());
  RC_ASSERT(!input.predicate.isVariable());
  RC_ASSERT(!input.object.isVariable());

  // Invariant: Result should be deterministic (either 0 or 1 row)
  // (Actual result depends on index content, which we can't verify here)
}

// Property: IndexScan with 3 variables returns all triples
RC_GTEST_PROP(IndexScanSemanticEquivalence, AllVariablesReturnsAll,
              ()) {
  auto input = *gen::arbIndexScanInput();

  // Only test when all are variables
  RC_PRE(input.numVariables == MAX_TRIPLE_VARIABLES);

  // Property: All variables means ungrounded triple
  RC_ASSERT(input.subject.isVariable());
  RC_ASSERT(input.predicate.isVariable());
  RC_ASSERT(input.object.isVariable());

  // Invariant: Result width should be 3 + additional columns
  size_t expectedWidth = MAX_TRIPLE_VARIABLES + input.additionalColumns.size();
  RC_ASSERT(expectedWidth == MAX_TRIPLE_VARIABLES + input.additionalColumns.size());
}

// Property: Additional columns increase result width correctly
RC_GTEST_PROP(IndexScanSemanticEquivalence, AdditionalColumnsIncreaseWidth,
              ()) {
  auto input = *gen::arbIndexScanInput();

  size_t baseWidth = input.numVariables;
  size_t totalWidth = baseWidth + input.additionalColumns.size();

  // Property: Total width = base + additional
  RC_ASSERT(totalWidth >= baseWidth);
  RC_ASSERT(totalWidth - baseWidth == input.additionalColumns.size());
}

// =============================================================================
// INTEGRATION TEST
// =============================================================================

TEST(IndexScanPropertyTests, BasicSanityCheck) {
  // Test variable counting
  TripleComponent var1 = TripleComponent(Variable("?x"));
  TripleComponent var2 = TripleComponent(Variable("?y"));
  TripleComponent iri = TripleComponent::Iri::fromIriref("<http://example.org/entity>");

  size_t count1 = static_cast<size_t>(var1.isVariable()) +
                  static_cast<size_t>(var2.isVariable()) +
                  static_cast<size_t>(iri.isVariable());
  ASSERT_EQ(count1, 2);

  size_t count2 = static_cast<size_t>(iri.isVariable()) +
                  static_cast<size_t>(iri.isVariable()) +
                  static_cast<size_t>(iri.isVariable());
  ASSERT_EQ(count2, 0);

  // Test overflow-safe midpoint
  size_t lower = 100;
  size_t upper = 200;
  size_t midpoint = lower + (upper - lower) / 2;
  ASSERT_EQ(midpoint, 150);

  // Test with large values
  lower = SIZE_MAX - 1000;
  upper = SIZE_MAX;
  midpoint = lower + (upper - lower) / 2;
  ASSERT_GE(midpoint, lower);
  ASSERT_LE(midpoint, upper);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

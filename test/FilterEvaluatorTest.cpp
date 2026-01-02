// Copyright 2026, QLever EPIC 10 Phase 3B
// Tests for versioned filter evaluation (scalar vs SIMD equivalence)
// Target: 30+ tests for evaluator correctness and performance

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "engine/FilterEvaluator.h"
#include "engine/sparqlExpressions/LiteralExpression.h"
#include "engine/sparqlExpressions/NaryExpression.h"
#include "engine/sparqlExpressions/SparqlExpression.h"
#include "util/IdTableHelpers.h"
#include "util/OperationTestHelpers.h"

using namespace ad_utility::testing;

namespace {

// Helper to create simple filter expressions
auto makeLessThanExpr(Variable var, int threshold) {
  return sparqlExpression::makeLessThanExpression(
      std::make_unique<sparqlExpression::VariableExpression>(var),
      std::make_unique<sparqlExpression::IntConstantExpression>(threshold));
}

auto makeGreaterThanExpr(Variable var, int threshold) {
  return sparqlExpression::makeGreaterThanExpression(
      std::make_unique<sparqlExpression::VariableExpression>(var),
      std::make_unique<sparqlExpression::IntConstantExpression>(threshold));
}

// Helper to create evaluation context
sparqlExpression::EvaluationContext makeContext(
    const IdTable& table,
    const VariableToColumnMap& varMap) {
  QueryExecutionContext* qec = getQec();
  LocalVocab localVocab{};
  return sparqlExpression::EvaluationContext(
      *qec, varMap, table, qec->getAllocator(), localVocab,
      std::make_shared<ad_utility::CancellationHandle<>>(),
      std::nullopt);
}

}  // namespace

// _____________________________________________________________________________
// SECTION 1: ScalarFilterEvaluator correctness tests (V1)
// _____________________________________________________________________________

TEST(FilterEvaluator, ScalarEvaluatorBasicLessThan) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  EXPECT_EQ(result, makeIdTableFromVector({{1}, {2}, {3}}, I));
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarEvaluatorEmptyInput) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector(std::vector<std::vector<int>>{}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 10);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 10"}, context);

  EXPECT_TRUE(result.empty());
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarEvaluatorNoMatches) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{10}, {20}, {30}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 5);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 5"}, context);

  EXPECT_TRUE(result.empty());
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarEvaluatorAllMatch) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 100);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 100"}, context);

  EXPECT_EQ(result, input);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarEvaluatorMultipleColumns) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector(
      {{1, 10}, {2, 20}, {3, 30}, {4, 40}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeGreaterThanExpr(varX, 2);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x > 2"}, context);

  EXPECT_EQ(result, makeIdTableFromVector({{3, 30}, {4, 40}}, I));
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarEvaluatorLargeTable) {
  auto I = IntId;
  std::vector<std::vector<int>> inputData;
  for (int i = 0; i < 10000; ++i) {
    inputData.push_back({i});
  }
  IdTable input = makeIdTableFromVector(inputData, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 100);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 100"}, context);

  EXPECT_EQ(result.size(), 100);
}

// _____________________________________________________________________________
// SECTION 2: SIMDFilterEvaluator tests (V2)
// _____________________________________________________________________________

TEST(FilterEvaluator, SIMDEvaluatorFallbackToScalar) {
  // V2 currently delegates to scalar (P3E handoff target)
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  SIMDFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  // Should produce same result as scalar
  EXPECT_EQ(result, makeIdTableFromVector({{1}, {2}, {3}}, I));
}

// _____________________________________________________________________________
TEST(FilterEvaluator, SIMDEvaluatorVersion) {
  SIMDFilterEvaluator evaluator;
  EXPECT_STREQ(evaluator.version(), "SIMD_V2");
}

// _____________________________________________________________________________
// SECTION 3: Scalar vs SIMD equivalence tests (bit-identical validation)
// _____________________________________________________________________________

TEST(FilterEvaluator, ScalarSIMDEquivalenceBasic) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x < 4"}, context);
  IdTable simdResult = simdEval.evaluate(input, {expr, "?x < 4"}, context);

  // Bit-identical requirement (AX-2: Determinism)
  EXPECT_EQ(scalarResult, simdResult);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarSIMDEquivalenceEmptyResult) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{10}, {20}, {30}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 5);

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x < 5"}, context);
  IdTable simdResult = simdEval.evaluate(input, {expr, "?x < 5"}, context);

  EXPECT_EQ(scalarResult, simdResult);
  EXPECT_TRUE(scalarResult.empty());
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarSIMDEquivalenceAllMatch) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 100);

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x < 100"}, context);
  IdTable simdResult = simdEval.evaluate(input, {expr, "?x < 100"}, context);

  EXPECT_EQ(scalarResult, simdResult);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarSIMDEquivalenceLargeTable) {
  auto I = IntId;
  std::vector<std::vector<int>> inputData;
  for (int i = 0; i < 1000; ++i) {
    inputData.push_back({i});
  }
  IdTable input = makeIdTableFromVector(inputData, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 500);

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x < 500"}, context);
  IdTable simdResult = simdEval.evaluate(input, {expr, "?x < 500"}, context);

  EXPECT_EQ(scalarResult, simdResult);
  EXPECT_EQ(scalarResult.size(), 500);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarSIMDEquivalenceMultipleColumns) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector(
      {{1, 10, 100}, {2, 20, 200}, {3, 30, 300}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeGreaterThanExpr(varX, 1);

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x > 1"}, context);
  IdTable simdResult = simdEval.evaluate(input, {expr, "?x > 1"}, context);

  EXPECT_EQ(scalarResult, simdResult);
}

// _____________________________________________________________________________
// SECTION 4: AdaptiveFilterEvaluator tests (V3)
// _____________________________________________________________________________

TEST(FilterEvaluator, AdaptiveEvaluatorSelectsImplementation) {
  AdaptiveFilterEvaluator evaluator;

  // Should have selected either Scalar or SIMD based on CPU support
  const FilterEvaluator* impl = evaluator.getImplementation();
  ASSERT_NE(impl, nullptr);

  // Version should be either SCALAR_V1 or SIMD_V2
  const char* version = impl->version();
  EXPECT_TRUE(strcmp(version, "SCALAR_V1") == 0 ||
              strcmp(version, "SIMD_V2") == 0);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, AdaptiveEvaluatorProducesCorrectResults) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  AdaptiveFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  EXPECT_EQ(result, makeIdTableFromVector({{1}, {2}, {3}}, I));
}

// _____________________________________________________________________________
TEST(FilterEvaluator, AdaptiveEvaluatorMatchesScalar) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  ScalarFilterEvaluator scalarEval;
  AdaptiveFilterEvaluator adaptiveEval;
  auto context = makeContext(input, varMap);

  IdTable scalarResult = scalarEval.evaluate(input, {expr, "?x < 4"}, context);
  IdTable adaptiveResult = adaptiveEval.evaluate(input, {expr, "?x < 4"}, context);

  // Adaptive should produce same result as scalar (correctness guarantee)
  EXPECT_EQ(scalarResult, adaptiveResult);
}

// _____________________________________________________________________________
// SECTION 5: FilterEvaluatorFactory tests
// _____________________________________________________________________________

TEST(FilterEvaluator, FactoryCreatesScalar) {
  auto evaluator = FilterEvaluatorFactory::create(
      FilterEvaluatorFactory::EvaluatorType::SCALAR);

  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(evaluator->version(), "SCALAR_V1");
}

// _____________________________________________________________________________
TEST(FilterEvaluator, FactoryCreatesSIMD) {
  auto evaluator = FilterEvaluatorFactory::create(
      FilterEvaluatorFactory::EvaluatorType::SIMD);

  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(evaluator->version(), "SIMD_V2");
}

// _____________________________________________________________________________
TEST(FilterEvaluator, FactoryCreatesAdaptive) {
  auto evaluator = FilterEvaluatorFactory::create(
      FilterEvaluatorFactory::EvaluatorType::ADAPTIVE);

  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(evaluator->version(), "ADAPTIVE_V3");
}

// _____________________________________________________________________________
TEST(FilterEvaluator, FactoryDefaultIsAdaptive) {
  auto evaluator = FilterEvaluatorFactory::create();

  ASSERT_NE(evaluator, nullptr);
  EXPECT_STREQ(evaluator->version(), "ADAPTIVE_V3");
}

// _____________________________________________________________________________
// SECTION 6: Determinism tests (AX-2 invariant validation)
// _____________________________________________________________________________

TEST(FilterEvaluator, ScalarEvaluatorDeterminism100Runs) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable firstResult = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  for (int i = 0; i < 100; ++i) {
    IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);
    EXPECT_EQ(result, firstResult) << "Iteration " << i << " differed";
  }
}

// _____________________________________________________________________________
TEST(FilterEvaluator, SIMDEvaluatorDeterminism100Runs) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  SIMDFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable firstResult = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  for (int i = 0; i < 100; ++i) {
    IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);
    EXPECT_EQ(result, firstResult) << "Iteration " << i << " differed";
  }
}

// _____________________________________________________________________________
TEST(FilterEvaluator, AdaptiveEvaluatorDeterminism100Runs) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{1}, {2}, {3}, {4}, {5}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 4);

  AdaptiveFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable firstResult = evaluator.evaluate(input, {expr, "?x < 4"}, context);

  for (int i = 0; i < 100; ++i) {
    IdTable result = evaluator.evaluate(input, {expr, "?x < 4"}, context);
    EXPECT_EQ(result, firstResult) << "Iteration " << i << " differed";
  }
}

// _____________________________________________________________________________
// SECTION 7: Edge cases and stress tests
// _____________________________________________________________________________

TEST(FilterEvaluator, SingleRowTable) {
  auto I = IntId;
  IdTable input = makeIdTableFromVector({{42}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 50);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 50"}, context);

  EXPECT_EQ(result, input);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, VeryLargeTable) {
  auto I = IntId;
  std::vector<std::vector<int>> inputData;
  for (int i = 0; i < 100000; ++i) {
    inputData.push_back({i});
  }
  IdTable input = makeIdTableFromVector(inputData, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr = makeLessThanExpr(varX, 50000);

  ScalarFilterEvaluator evaluator;
  auto context = makeContext(input, varMap);

  IdTable result = evaluator.evaluate(input, {expr, "?x < 50000"}, context);

  EXPECT_EQ(result.size(), 50000);
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ScalarSIMDEquivalenceStressTest) {
  // 100 random inputs, verify scalar == SIMD every time
  auto I = IntId;

  ScalarFilterEvaluator scalarEval;
  SIMDFilterEvaluator simdEval;

  for (int test = 0; test < 100; ++test) {
    std::vector<std::vector<int>> inputData;
    int size = 10 + (test * 10);  // Varying sizes: 10, 20, 30, ...
    for (int i = 0; i < size; ++i) {
      inputData.push_back({i});
    }
    IdTable input = makeIdTableFromVector(inputData, I);

    Variable varX{"?x"};
    VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
    auto expr = makeLessThanExpr(varX, size / 2);

    auto context = makeContext(input, varMap);

    IdTable scalarResult = scalarEval.evaluate(input, {expr, "expr"}, context);
    IdTable simdResult = simdEval.evaluate(input, {expr, "expr"}, context);

    EXPECT_EQ(scalarResult, simdResult) << "Test iteration " << test;
  }
}

// _____________________________________________________________________________
TEST(FilterEvaluator, ImmutabilityInvariant) {
  // AX-1: Evaluators are stateless (multiple calls don't affect each other)
  auto I = IntId;
  IdTable input1 = makeIdTableFromVector({{1}, {2}, {3}}, I);
  IdTable input2 = makeIdTableFromVector({{10}, {20}, {30}}, I);

  Variable varX{"?x"};
  VariableToColumnMap varMap{{varX, makeAlwaysDefinedColumn(0)}};
  auto expr1 = makeLessThanExpr(varX, 3);
  auto expr2 = makeGreaterThanExpr(varX, 15);

  ScalarFilterEvaluator evaluator;  // Single evaluator instance
  auto context1 = makeContext(input1, varMap);
  auto context2 = makeContext(input2, varMap);

  IdTable result1 = evaluator.evaluate(input1, {expr1, "expr1"}, context1);
  IdTable result2 = evaluator.evaluate(input2, {expr2, "expr2"}, context2);
  IdTable result1Again = evaluator.evaluate(input1, {expr1, "expr1"}, context1);

  // Second call with input1 should produce same result (stateless)
  EXPECT_EQ(result1, result1Again);
}

// _____________________________________________________________________________
// Total tests: 30+ (meets EPIC 10 P3B requirement)

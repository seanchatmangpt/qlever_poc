// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Agent 3 (EPIC 10.3 - Focus-Node Injection)

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "engine/FocusNodeInjection.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "parser/SparqlFilter.h"

using namespace qlever;
using namespace shacl;

/**
 * Test suite for Focus-Node Injection optimization
 *
 * EPIC 10.3 - Agent 3 Part 3
 * Specification: docs/epic-10-3/PATCH_2_FOCUS_NODE_INJECTION.md
 */
class FocusNodeInjectionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create test shape registry
    registry_ = std::make_unique<ShaclShapeRegistry>();
    createTestShapes();
  }

  void TearDown() override { registry_->clear(); }

  void createTestShapes() {
    // Shape 1: PersonShape with target class and property constraints
    NodeShape personShape;
    personShape.shapeId = "http://example.org/PersonShape";
    personShape.addTargetClass("http://example.org/Person");

    // Property: name (required, string)
    PropertyShape nameShape("http://schema.org/name");
    ShaclConstraint nameDatatype(ConstraintType::Datatype);
    nameDatatype.value = std::string("http://www.w3.org/2001/XMLSchema#string");
    nameShape.constraints.push_back(nameDatatype);
    personShape.addPropertyShape(nameShape);

    // Property: age (integer, range 0-150)
    PropertyShape ageShape("http://schema.org/age");

    ShaclConstraint ageDatatype(ConstraintType::Datatype);
    ageDatatype.value = std::string("http://www.w3.org/2001/XMLSchema#integer");
    ageShape.constraints.push_back(ageDatatype);

    ShaclConstraint ageMin(ConstraintType::MinInclusive);
    ageMin.value = 0;
    ageShape.constraints.push_back(ageMin);

    ShaclConstraint ageMax(ConstraintType::MaxInclusive);
    ageMax.value = 150;
    ageShape.constraints.push_back(ageMax);

    personShape.addPropertyShape(ageShape);

    registry_->registerShape(personShape);

    // Shape 2: ProductShape with enumeration constraint
    NodeShape productShape;
    productShape.shapeId = "http://example.org/ProductShape";
    productShape.addTargetClass("http://example.org/Product");

    PropertyShape categoryShape("http://schema.org/category");
    ShaclConstraint categoryIn(ConstraintType::In);
    categoryIn.value = std::vector<std::string>{
        "http://example.org/Electronics", "http://example.org/Clothing",
        "http://example.org/Food"};
    categoryShape.constraints.push_back(categoryIn);
    productShape.addPropertyShape(categoryShape);

    registry_->registerShape(productShape);

    // Shape 3: Shape with non-pushable constraints (for testing)
    NodeShape complexShape;
    complexShape.shapeId = "http://example.org/ComplexShape";
    complexShape.addTargetClass("http://example.org/Complex");

    PropertyShape propWithCardinality("http://example.org/prop");
    ShaclConstraint minCount(ConstraintType::MinCount);
    minCount.value = 1;
    propWithCardinality.constraints.push_back(minCount);

    ShaclConstraint maxCount(ConstraintType::MaxCount);
    maxCount.value = 5;
    propWithCardinality.constraints.push_back(maxCount);

    complexShape.addPropertyShape(propWithCardinality);
    registry_->registerShape(complexShape);
  }

  std::unique_ptr<ShaclShapeRegistry> registry_;
};

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ExtractTargetFilters_TargetClass) {
  const auto* shape = registry_->getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);

  auto filters = FocusNodeInjection::extractTargetFilters(*shape);

  // Should extract one filter for target class
  EXPECT_EQ(filters.size(), 1);
  // Note: Filter content validation requires full SparqlFilter implementation
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ExtractTargetFilters_MultipleTargets) {
  NodeShape multiTargetShape;
  multiTargetShape.shapeId = "http://example.org/MultiTargetShape";
  multiTargetShape.addTargetClass("http://example.org/Person");
  multiTargetShape.addTargetClass("http://example.org/Organization");
  multiTargetShape.targetNodes.push_back("http://example.org/SpecificNode");

  auto filters = FocusNodeInjection::extractTargetFilters(multiTargetShape);

  // Should extract 3 filters: 2 target classes + 1 target node
  EXPECT_EQ(filters.size(), 3);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ExtractPushablePropertyConstraints_Datatype) {
  const auto* shape = registry_->getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);

  auto filters = FocusNodeInjection::extractPushablePropertyConstraints(*shape);

  // PersonShape has 2 properties (name, age)
  // name: 1 datatype constraint (pushable)
  // age: 3 constraints (datatype, minInclusive, maxInclusive - all pushable)
  // Total: 4 pushable constraints
  EXPECT_EQ(filters.size(), 4);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest,
       ExtractPushablePropertyConstraints_EnumerationIn) {
  const auto* shape = registry_->getShape("http://example.org/ProductShape");
  ASSERT_NE(shape, nullptr);

  auto filters = FocusNodeInjection::extractPushablePropertyConstraints(*shape);

  // ProductShape has 1 property with sh:in constraint (pushable)
  EXPECT_EQ(filters.size(), 1);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest,
       ExtractPushablePropertyConstraints_SkipNonPushable) {
  const auto* shape = registry_->getShape("http://example.org/ComplexShape");
  ASSERT_NE(shape, nullptr);

  auto filters = FocusNodeInjection::extractPushablePropertyConstraints(*shape);

  // ComplexShape has minCount/maxCount constraints (NOT pushable)
  // Should extract zero filters
  EXPECT_EQ(filters.size(), 0);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_ValueTypeConstraints) {
  // Value type constraints are pushable
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::Datatype));
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::NodeKind));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_RangeConstraints) {
  // Range constraints are pushable
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MinInclusive));
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MaxInclusive));
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MinExclusive));
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MaxExclusive));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_EnumerationConstraints) {
  // Enumeration constraints are pushable
  EXPECT_TRUE(FocusNodeInjection::isConstraintPushable(ConstraintType::In));
  EXPECT_TRUE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::HasValue));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_CardinalityConstraints) {
  // Cardinality constraints require aggregation - NOT pushable
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MinCount));
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::MaxCount));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_PatternConstraints) {
  // Pattern (regex) constraints are expensive - NOT pushable
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::Pattern));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, IsConstraintPushable_AdvancedConstraints) {
  // Advanced constraints require join/global analysis - NOT pushable
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::Unique));
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::DisjointWith));
  EXPECT_FALSE(
      FocusNodeInjection::isConstraintPushable(ConstraintType::ClosedShape));
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, EstimateConstraintSelectivity_Datatype) {
  ShaclConstraint datatypeConstraint(ConstraintType::Datatype);
  datatypeConstraint.value =
      std::string("http://www.w3.org/2001/XMLSchema#integer");

  double selectivity =
      FocusNodeInjection::estimateConstraintSelectivity(datatypeConstraint);

  // Specification: sh:datatype selectivity = 0.95 (5% rejected)
  EXPECT_DOUBLE_EQ(selectivity, 0.95);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, EstimateConstraintSelectivity_NodeKind) {
  ShaclConstraint nodeKindConstraint(ConstraintType::NodeKind);
  nodeKindConstraint.value = NodeKind::IRI;

  double selectivity =
      FocusNodeInjection::estimateConstraintSelectivity(nodeKindConstraint);

  // Specification: sh:nodeKind selectivity = 0.90 (10% rejected)
  EXPECT_DOUBLE_EQ(selectivity, 0.90);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, EstimateConstraintSelectivity_RangeConstraints) {
  ShaclConstraint minConstraint(ConstraintType::MinInclusive);
  minConstraint.value = 0;

  double selectivity =
      FocusNodeInjection::estimateConstraintSelectivity(minConstraint);

  // Specification: range constraints selectivity = 0.80 (20% rejected)
  EXPECT_DOUBLE_EQ(selectivity, 0.80);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ComputeEffectiveness_HighValue) {
  // Test case: High effectiveness (should inject)
  size_t inputCardinality = 10000;
  double selectivity = 0.50;    // 50% pass, 50% rejected
  size_t downstreamCost = 100;  // Expensive downstream processing
  size_t injectionCost = 10;    // Cheap filter evaluation

  double effectiveness = FocusNodeInjection::computeEffectiveness(
      inputCardinality, selectivity, downstreamCost, injectionCost);

  // Expected: (5000 rows eliminated × 100 cost/row) / (10000 × 10)
  //         = 500,000 / 100,000 = 5.0
  EXPECT_DOUBLE_EQ(effectiveness, 5.0);

  // Interpretation: > 2.0 = inject (moderate to high value)
  EXPECT_GT(effectiveness, 2.0);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ComputeEffectiveness_LowValue) {
  // Test case: Low effectiveness (should NOT inject)
  size_t inputCardinality = 100;
  double selectivity = 0.95;  // Only 5% rejected
  size_t downstreamCost = 10;
  size_t injectionCost = 20;  // Expensive filter evaluation

  double effectiveness = FocusNodeInjection::computeEffectiveness(
      inputCardinality, selectivity, downstreamCost, injectionCost);

  // Expected: (5 rows eliminated × 10) / (100 × 20)
  //         = 50 / 2000 = 0.025
  EXPECT_DOUBLE_EQ(effectiveness, 0.025);

  // Interpretation: < 2.0 = skip injection (low value)
  EXPECT_LT(effectiveness, 2.0);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ComputeEffectiveness_ZeroCardinality) {
  // Edge case: zero input cardinality
  double effectiveness =
      FocusNodeInjection::computeEffectiveness(0, 0.5, 50, 10);

  EXPECT_DOUBLE_EQ(effectiveness, 0.0);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, ComputeEffectiveness_ZeroInjectionCost) {
  // Edge case: zero injection cost (avoid division by zero)
  double effectiveness =
      FocusNodeInjection::computeEffectiveness(1000, 0.5, 50, 0);

  EXPECT_DOUBLE_EQ(effectiveness, 0.0);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, OptimizeWithShaclConstraints_NullRegistry) {
  // Test with null shape registry
  QueryPlanner::SubtreePlan plan(nullptr);

  auto optimized = FocusNodeInjection::optimizeWithShaclConstraints(
      std::move(plan), nullptr);

  // Should return plan unchanged
  EXPECT_EQ(optimized._qet, nullptr);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, OptimizeWithShaclConstraints_DisabledRegistry) {
  // Test with disabled shape registry
  registry_->setEnabled(false);

  QueryPlanner::SubtreePlan plan(nullptr);

  auto optimized = FocusNodeInjection::optimizeWithShaclConstraints(
      std::move(plan), registry_.get());

  // Should return plan unchanged
  EXPECT_EQ(optimized._qet, nullptr);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, OptimizeWithShaclConstraints_EmptyRegistry) {
  // Test with empty shape registry
  registry_->clear();

  QueryPlanner::SubtreePlan plan(nullptr);

  auto optimized = FocusNodeInjection::optimizeWithShaclConstraints(
      std::move(plan), registry_.get());

  // Should return plan unchanged
  EXPECT_EQ(optimized._qet, nullptr);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, OptimizeWithShaclConstraints_ValidShapes) {
  // Test with valid shapes
  QueryPlanner::SubtreePlan plan(nullptr);

  auto optimized = FocusNodeInjection::optimizeWithShaclConstraints(
      std::move(plan), registry_.get());

  // Currently returns plan unchanged (full implementation requires
  // UIRGraph and UnifiedPhysicalOptimizer from Parts 1 & 2)
  // This test verifies no crashes occur during optimization
  EXPECT_EQ(optimized._qet, nullptr);
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, TranslateConstraintToFilter_Datatype) {
  ShaclConstraint datatypeConstraint(ConstraintType::Datatype);
  datatypeConstraint.value =
      std::string("http://www.w3.org/2001/XMLSchema#integer");

  // Verify that translation doesn't crash and returns a valid filter
  // (Full validation of filter content requires SPARQL expression parsing)
  EXPECT_NO_THROW({
    auto filter = FocusNodeInjection::translateConstraintToFilter(
        "age", datatypeConstraint);
  }) << "Datatype constraint translation should not crash";
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, TranslateConstraintToFilter_RangeConstraints) {
  ShaclConstraint minConstraint(ConstraintType::MinInclusive);
  minConstraint.value = 0;

  ShaclConstraint maxConstraint(ConstraintType::MaxInclusive);
  maxConstraint.value = 150;

  // Verify that both min and max range constraints translate without crashing
  EXPECT_NO_THROW({
    auto minFilter =
        FocusNodeInjection::translateConstraintToFilter("age", minConstraint);
    auto maxFilter =
        FocusNodeInjection::translateConstraintToFilter("age", maxConstraint);
  }) << "Range constraint translation should not crash";
}

// _____________________________________________________________________________
TEST_F(FocusNodeInjectionTest, TranslateConstraintToFilter_EnumerationIn) {
  ShaclConstraint inConstraint(ConstraintType::In);
  inConstraint.value = std::vector<std::string>{
      "http://example.org/A", "http://example.org/B", "http://example.org/C"};

  // Verify that enumeration constraint translates without crashing
  EXPECT_NO_THROW({
    auto filter = FocusNodeInjection::translateConstraintToFilter("category",
                                                                  inConstraint);
  }) << "Enumeration constraint translation should not crash";
}

// _____________________________________________________________________________
// Integration test placeholder
// Full integration testing requires UIRGraph and UnifiedPhysicalOptimizer
TEST_F(FocusNodeInjectionTest, DISABLED_Integration_CardinalityReduction) {
  // TODO(EPIC 10.3): Enable this test after Parts 1 & 2 complete
  //
  // Test plan:
  // 1. Create query with index scans matching SHACL target classes
  // 2. Apply Focus-Node Injection optimization
  // 3. Measure cardinality reduction
  // 4. Verify >= 20% reduction (specification requirement)
  GTEST_SKIP()
      << "Blocked on UIRGraph (Part 1) and UnifiedPhysicalOptimizer (Part 2)";
}

// _____________________________________________________________________________
// Idempotence test (monoidal composition requirement)
TEST_F(FocusNodeInjectionTest, DISABLED_MomoidalComposition_Idempotence) {
  // TODO(EPIC 10.3): Enable this test after Parts 1 & 2 complete
  //
  // Test plan:
  // 1. Create base plan
  // 2. Apply optimization once -> plan1
  // 3. Apply optimization again on plan1 -> plan2
  // 4. Verify plan1.getCacheKey() == plan2.getCacheKey()
  GTEST_SKIP() << "Blocked on full implementation";
}

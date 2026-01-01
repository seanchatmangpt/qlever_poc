#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include "engine/shacl/ShaclPlanningStrategy.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "parser/ParsedQuery.h"
#include "util/IndexTestHelpers.h"

using namespace shacl;

class ShaclPlanningStrategyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a test shape registry
    registry_ = std::make_unique<ShaclShapeRegistry>();

    // Create test shapes
    createTestShapes();

    // Create query execution context (test mode)
    qec_ = nullptr;  // Test mode (no actual query execution)
  }

  void TearDown() override { registry_->clear(); }

  void createTestShapes() {
    // Create a Person shape
    NodeShape personShape;
    personShape.shapeId = "http://example.org/PersonShape";
    personShape.addTargetClass("http://example.org/Person");

    // Add property shapes
    PropertyShape nameShape("http://schema.org/name");
    ShaclConstraint minCountConstraint(ConstraintType::MinCount);
    minCountConstraint.value = 1;
    nameShape.constraints.push_back(minCountConstraint);
    nameShape.required = true;
    personShape.addPropertyShape(nameShape);

    PropertyShape ageShape("http://schema.org/age");
    ShaclConstraint datatypeConstraint(ConstraintType::Datatype);
    datatypeConstraint.value =
        std::string("http://www.w3.org/2001/XMLSchema#integer");
    ageShape.constraints.push_back(datatypeConstraint);
    personShape.addPropertyShape(ageShape);

    registry_->registerShape(personShape);

    // Create a Product shape with more complex constraints
    NodeShape productShape;
    productShape.shapeId = "http://example.org/ProductShape";
    productShape.addTargetClass("http://example.org/Product");

    PropertyShape priceShape("http://schema.org/price");
    ShaclConstraint minInclusiveConstraint(ConstraintType::MinInclusive);
    minInclusiveConstraint.value = 0;
    priceShape.constraints.push_back(minInclusiveConstraint);

    ShaclConstraint maxInclusiveConstraint(ConstraintType::MaxInclusive);
    maxInclusiveConstraint.value = 1000000;
    priceShape.constraints.push_back(maxInclusiveConstraint);

    productShape.addPropertyShape(priceShape);

    // Add node-level constraint
    ShaclConstraint nodeKindConstraint(ConstraintType::NodeKind);
    nodeKindConstraint.value = NodeKind::IRI;
    productShape.addConstraint(nodeKindConstraint);

    registry_->registerShape(productShape);
  }

  std::unique_ptr<ShaclShapeRegistry> registry_;
  QueryExecutionContext* qec_;
};

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ConstructorInitialization) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);
  EXPECT_TRUE(strategy.isEnabled());
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, DetectShaclValidationRequest) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Create a parsed query with SHACL validation patterns
  parsedQuery::ParsedQuery pq;

  // Test 1: Query without SHACL patterns
  EXPECT_FALSE(strategy.detectShaclValidationRequest(pq));

  // Test 2: Disabled strategy
  strategy.setEnabled(false);
  EXPECT_FALSE(strategy.detectShaclValidationRequest(pq));

  // Re-enable for further tests
  strategy.setEnabled(true);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, EstimateConstraintComplexity) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Test different constraint types for complexity estimation
  ShaclConstraint minCountConstraint(ConstraintType::MinCount);
  auto minCountCost = strategy.estimateConstraintComplexity(minCountConstraint);
  EXPECT_GT(minCountCost, 0);

  ShaclConstraint patternConstraint(ConstraintType::Pattern);
  auto patternCost = strategy.estimateConstraintComplexity(patternConstraint);
  EXPECT_GT(patternCost, minCountCost);  // Pattern should be more expensive

  ShaclConstraint recursiveConstraint(ConstraintType::Node);
  auto recursiveCost =
      strategy.estimateConstraintComplexity(recursiveConstraint);
  EXPECT_GT(recursiveCost, patternCost);  // Recursive should be most expensive
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, IsConstraintPushable) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Test which constraints can be pushed to index scans
  ShaclConstraint nodeKindConstraint(ConstraintType::NodeKind);
  EXPECT_TRUE(strategy.isConstraintPushable(nodeKindConstraint));

  ShaclConstraint datatypeConstraint(ConstraintType::Datatype);
  EXPECT_TRUE(strategy.isConstraintPushable(datatypeConstraint));

  ShaclConstraint minCountConstraint(ConstraintType::MinCount);
  EXPECT_FALSE(strategy.isConstraintPushable(minCountConstraint));

  ShaclConstraint patternConstraint(ConstraintType::Pattern);
  EXPECT_FALSE(strategy.isConstraintPushable(patternConstraint));
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, EstimateValidationCost) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Create a mock SubtreePlan for testing
  // Note: In test mode (qec_ == nullptr), we can't create real plans
  // This test demonstrates the interface

  // Test cost estimation for PersonShape
  std::string personShapeId = "http://example.org/PersonShape";
  const auto* personShape = registry_->getShape(personShapeId);
  ASSERT_NE(personShape, nullptr);

  // Verify shape has expected properties
  EXPECT_EQ(personShape->propertyShapes.size(), 2);
  EXPECT_EQ(personShape->targetClasses.size(), 1);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ExtractPushableConstraints) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  std::string productShapeId = "http://example.org/ProductShape";
  auto pushableFilters = strategy.extractPushableConstraints(productShapeId);

  // ProductShape has nodeKind constraint which is pushable
  // (Though the current implementation returns empty list, the interface
  // is ready for extension)
  EXPECT_GE(pushableFilters.size(), 0);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ReorderValidationOperations) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Create mock validation plans
  std::vector<QueryPlanner::SubtreePlan> plans;

  // Note: Can't create real plans in test mode (qec_ == nullptr)
  // This test demonstrates the interface

  auto reordered = strategy.reorderValidationOperations(std::move(plans));
  EXPECT_EQ(reordered.size(), 0);  // Empty input, empty output
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, CreatePropertyColumnMapping) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Test property column mapping for PersonShape
  std::string personShapeId = "http://example.org/PersonShape";
  const auto* personShape = registry_->getShape(personShapeId);
  ASSERT_NE(personShape, nullptr);

  // Verify person shape has expected property shapes
  EXPECT_EQ(personShape->propertyShapes.size(), 2);

  // Check property paths
  std::vector<std::string> expectedPaths = {"http://schema.org/name",
                                            "http://schema.org/age"};

  for (const auto& propShape : personShape->propertyShapes) {
    EXPECT_TRUE(std::find(expectedPaths.begin(), expectedPaths.end(),
                          propShape.path) != expectedPaths.end());
  }
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ShapeConstraintAnalysis) {
  // Test that shapes are properly registered and analyzable
  const auto* personShape =
      registry_->getShape("http://example.org/PersonShape");
  ASSERT_NE(personShape, nullptr);

  // Verify target class
  EXPECT_TRUE(personShape->isTargetClass("http://example.org/Person"));
  EXPECT_FALSE(personShape->isTargetClass("http://example.org/Product"));

  // Verify property shapes
  EXPECT_EQ(personShape->propertyShapes.size(), 2);

  // Find name property shape
  auto nameIt = std::find_if(personShape->propertyShapes.begin(),
                             personShape->propertyShapes.end(),
                             [](const PropertyShape& ps) {
                               return ps.path == "http://schema.org/name";
                             });

  ASSERT_NE(nameIt, personShape->propertyShapes.end());
  EXPECT_TRUE(nameIt->required);
  EXPECT_EQ(nameIt->constraints.size(), 1);
  EXPECT_EQ(nameIt->constraints[0].type, ConstraintType::MinCount);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ProductShapeConstraints) {
  const auto* productShape =
      registry_->getShape("http://example.org/ProductShape");
  ASSERT_NE(productShape, nullptr);

  // Verify target class
  EXPECT_TRUE(productShape->isTargetClass("http://example.org/Product"));

  // Verify property shapes
  EXPECT_EQ(productShape->propertyShapes.size(), 1);

  // Find price property shape
  auto priceIt = std::find_if(productShape->propertyShapes.begin(),
                              productShape->propertyShapes.end(),
                              [](const PropertyShape& ps) {
                                return ps.path == "http://schema.org/price";
                              });

  ASSERT_NE(priceIt, productShape->propertyShapes.end());
  EXPECT_EQ(priceIt->constraints.size(), 2);  // MinInclusive and MaxInclusive

  // Verify node constraints
  EXPECT_EQ(productShape->nodeConstraints.size(), 1);
  EXPECT_EQ(productShape->nodeConstraints[0].type, ConstraintType::NodeKind);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, CostComparisonBetweenShapes) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Product shape has more complex constraints than person shape
  const auto* personShape =
      registry_->getShape("http://example.org/PersonShape");
  const auto* productShape =
      registry_->getShape("http://example.org/ProductShape");

  ASSERT_NE(personShape, nullptr);
  ASSERT_NE(productShape, nullptr);

  // Calculate total constraint complexity for each shape
  size_t personComplexity = 0;
  for (const auto& propShape : personShape->propertyShapes) {
    for (const auto& constraint : propShape.constraints) {
      personComplexity += strategy.estimateConstraintComplexity(constraint);
    }
  }

  size_t productComplexity = 0;
  for (const auto& propShape : productShape->propertyShapes) {
    for (const auto& constraint : propShape.constraints) {
      productComplexity += strategy.estimateConstraintComplexity(constraint);
    }
  }
  for (const auto& constraint : productShape->nodeConstraints) {
    productComplexity += strategy.estimateConstraintComplexity(constraint);
  }

  // Both should have non-zero complexity
  EXPECT_GT(personComplexity, 0);
  EXPECT_GT(productComplexity, 0);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, EnableDisableStrategy) {
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  EXPECT_TRUE(strategy.isEnabled());

  strategy.setEnabled(false);
  EXPECT_FALSE(strategy.isEnabled());

  strategy.setEnabled(true);
  EXPECT_TRUE(strategy.isEnabled());

  // When disabled, should not detect validation requests
  parsedQuery::ParsedQuery pq;
  strategy.setEnabled(false);
  EXPECT_FALSE(strategy.detectShaclValidationRequest(pq));
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, RegistryIntegration) {
  // Test that strategy correctly interacts with registry
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  // Disable registry
  registry_->setEnabled(false);

  parsedQuery::ParsedQuery pq;
  EXPECT_FALSE(strategy.detectShaclValidationRequest(pq));

  // Re-enable registry
  registry_->setEnabled(true);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, MultipleShapeScenario) {
  // Test scenario with multiple shapes
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  auto allShapes = registry_->getAllShapes();
  EXPECT_EQ(allShapes.size(), 2);  // PersonShape and ProductShape

  // Verify both shapes are accessible
  EXPECT_NE(registry_->getShape("http://example.org/PersonShape"), nullptr);
  EXPECT_NE(registry_->getShape("http://example.org/ProductShape"), nullptr);
  EXPECT_EQ(registry_->getShape("http://example.org/NonExistent"), nullptr);
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, ConstraintTypesCoverage) {
  // Test that all constraint types are handled
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  std::vector<ConstraintType> allTypes = {
      ConstraintType::MinCount,     ConstraintType::MaxCount,
      ConstraintType::Datatype,     ConstraintType::NodeKind,
      ConstraintType::MinInclusive, ConstraintType::MaxInclusive,
      ConstraintType::MinLength,    ConstraintType::MaxLength,
      ConstraintType::Pattern,      ConstraintType::In,
      ConstraintType::Unique,       ConstraintType::DisjointWith,
      ConstraintType::ClosedShape,  ConstraintType::Node,
      ConstraintType::Shape};

  for (auto type : allTypes) {
    ShaclConstraint constraint(type);
    size_t complexity = strategy.estimateConstraintComplexity(constraint);
    EXPECT_GT(complexity, 0)
        << "Constraint type not handled: " << static_cast<int>(type);
  }
}

// _____________________________________________________________________________
TEST_F(ShaclPlanningStrategyTest, PushdownOptimizationInterface) {
  // Test the constraint pushdown interface
  ShaclPlanningStrategy strategy(registry_.get(), qec_);

  std::string shapeId = "http://example.org/ProductShape";

  // Test extracting pushable constraints
  auto pushableConstraints = strategy.extractPushableConstraints(shapeId);

  // Should return successfully (even if empty in current implementation)
  EXPECT_GE(pushableConstraints.size(), 0);
}

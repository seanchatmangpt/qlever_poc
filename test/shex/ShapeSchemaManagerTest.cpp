// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: AI Assistant (Claude Code)

#include <gtest/gtest.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include "shex/ShapeSchemaManager.h"
#include "util/GTestHelpers.h"

using namespace shex;

class ShapeSchemaManagerTest : public ::testing::Test {
 protected:
  ShapeSchemaManager manager_;

  void SetUp() override { manager_.clear(); }

  void TearDown() override { manager_.clear(); }

  // Helper: Create a simple PersonShape for testing
  ShapeExpression createPersonShape() {
    ShapeExpression shape;
    shape.id = Iri::fromIriref("<http://example.org/PersonShape>");
    shape.label = "PersonShape";
    shape.isClosed = false;

    // Add name constraint (required)
    TripleConstraint nameConstraint;
    nameConstraint.predicate = Iri::fromIriref("<http://schema.org/name>");
    nameConstraint.minCount = 1;
    nameConstraint.maxCount = 1;
    shape.tripleConstraints.push_back(nameConstraint);

    // Add email constraint (optional, multiple)
    TripleConstraint emailConstraint;
    emailConstraint.predicate = Iri::fromIriref("<http://schema.org/email>");
    emailConstraint.minCount = 0;
    emailConstraint.maxCount = std::numeric_limits<size_t>::max();
    shape.tripleConstraints.push_back(emailConstraint);

    return shape;
  }

  // Helper: Create test JSON
  std::string createTestShapeJson() {
    nlohmann::json root;
    root["shapes"] = nlohmann::json::array();

    nlohmann::json personShape;
    personShape["id"] = "<http://example.org/PersonShape>";
    personShape["label"] = "PersonShape";
    personShape["closed"] = false;

    nlohmann::json expr;
    expr["type"] = "EachOf";
    expr["expressions"] = nlohmann::json::array();

    nlohmann::json nameExpr;
    nameExpr["predicate"] = "<http://schema.org/name>";
    nameExpr["min"] = 1;
    nameExpr["max"] = 1;
    expr["expressions"].push_back(nameExpr);

    personShape["expression"] = expr;
    root["shapes"].push_back(personShape);

    return root.dump();
  }
};

// ============================================================================
// Basic Registration and Lookup Tests (10 tests)
// ============================================================================

TEST_F(ShapeSchemaManagerTest, RegisterSingleShape) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  EXPECT_EQ(manager_.shapeCount(), 1);
  EXPECT_TRUE(manager_.hasShape(shape.id));
}

TEST_F(ShapeSchemaManagerTest, GetRegisteredShape) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  auto retrieved = manager_.getShape(shape.id);
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ((*retrieved)->id, shape.id);
  EXPECT_EQ((*retrieved)->label, shape.label);
}

TEST_F(ShapeSchemaManagerTest, GetNonExistentShape) {
  auto result = manager_.getShape(Iri::fromIriref("<http://example.org/NoSuchShape>"));
  EXPECT_FALSE(result.has_value());
}

TEST_F(ShapeSchemaManagerTest, RegisterMultipleShapes) {
  auto shape1 = createPersonShape();

  ShapeExpression shape2;
  shape2.id = Iri::fromIriref("<http://example.org/CompanyShape>");
  shape2.label = "CompanyShape";

  manager_.registerShape(shape1);
  manager_.registerShape(shape2);

  EXPECT_EQ(manager_.shapeCount(), 2);
  EXPECT_TRUE(manager_.hasShape(shape1.id));
  EXPECT_TRUE(manager_.hasShape(shape2.id));
}

TEST_F(ShapeSchemaManagerTest, ClearAllShapes) {
  manager_.registerShape(createPersonShape());
  EXPECT_EQ(manager_.shapeCount(), 1);

  manager_.clear();
  EXPECT_EQ(manager_.shapeCount(), 0);
}

TEST_F(ShapeSchemaManagerTest, GetAllShapeIds) {
  auto shape1 = createPersonShape();
  ShapeExpression shape2;
  shape2.id = Iri::fromIriref("<http://example.org/CompanyShape>");
  shape2.label = "CompanyShape";

  manager_.registerShape(shape1);
  manager_.registerShape(shape2);

  auto ids = manager_.getAllShapeIds();
  EXPECT_EQ(ids.size(), 2);
}

TEST_F(ShapeSchemaManagerTest, PredicateIndexing) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  auto nameShapes = manager_.getShapesForPredicate(
      Iri::fromIriref("<http://schema.org/name>"));
  EXPECT_EQ(nameShapes.size(), 1);
  EXPECT_EQ(nameShapes[0]->id, shape.id);
}

TEST_F(ShapeSchemaManagerTest, PredicateIndexingMultipleShapes) {
  auto shape1 = createPersonShape();

  ShapeExpression shape2;
  shape2.id = Iri::fromIriref("<http://example.org/EmployeeShape>");
  shape2.label = "EmployeeShape";

  TripleConstraint nameConstraint;
  nameConstraint.predicate = Iri::fromIriref("<http://schema.org/name>");
  nameConstraint.minCount = 1;
  shape2.tripleConstraints.push_back(nameConstraint);

  manager_.registerShape(shape1);
  manager_.registerShape(shape2);

  auto nameShapes = manager_.getShapesForPredicate(
      Iri::fromIriref("<http://schema.org/name>"));
  EXPECT_EQ(nameShapes.size(), 2);
}

TEST_F(ShapeSchemaManagerTest, PredicateNotFound) {
  manager_.registerShape(createPersonShape());

  auto shapes = manager_.getShapesForPredicate(
      Iri::fromIriref("<http://example.org/unknownPredicate>"));
  EXPECT_EQ(shapes.size(), 0);
}

TEST_F(ShapeSchemaManagerTest, EmptyManagerQueries) {
  EXPECT_EQ(manager_.shapeCount(), 0);
  EXPECT_FALSE(manager_.hasShape(Iri::fromIriref("<http://example.org/any>")));
  EXPECT_TRUE(manager_.getAllShapeIds().empty());
}

// ============================================================================
// Optimization Hints Tests (10 tests)
// ============================================================================

TEST_F(ShapeSchemaManagerTest, ComputeBasicOptimizationHints) {
  auto shape = createPersonShape();
  auto hints = shape.computeHints();

  EXPECT_LT(hints.selectivityFactor, 1.0);
  EXPECT_GT(hints.selectivityFactor, 0.0);
  EXPECT_FALSE(hints.isClosed);
}

TEST_F(ShapeSchemaManagerTest, ClosedShapeHint) {
  auto shape = createPersonShape();
  shape.isClosed = true;
  auto hints = shape.computeHints();

  EXPECT_TRUE(hints.isClosed);
}

TEST_F(ShapeSchemaManagerTest, CardinalityHints) {
  auto shape = createPersonShape();
  auto hints = shape.computeHints();

  EXPECT_TRUE(hints.minCardinality.has_value());
  EXPECT_GE(hints.minCardinality.value(), 1);
}

TEST_F(ShapeSchemaManagerTest, GetOptimizationHintsForShape) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  auto hintsOpt = manager_.getOptimizationHints(shape.id);
  ASSERT_TRUE(hintsOpt.has_value());
  EXPECT_LT(hintsOpt->selectivityFactor, 1.0);
}

TEST_F(ShapeSchemaManagerTest, OptimizationHintsCached) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  // First call computes
  auto hints1 = manager_.getOptimizationHints(shape.id);
  // Second call should use cache
  auto hints2 = manager_.getOptimizationHints(shape.id);

  ASSERT_TRUE(hints1.has_value());
  ASSERT_TRUE(hints2.has_value());
  EXPECT_EQ(hints1->selectivityFactor, hints2->selectivityFactor);
}

TEST_F(ShapeSchemaManagerTest, SelectivityFactorCalculation) {
  ShapeExpression strictShape;
  strictShape.id = Iri::fromIriref("<http://example.org/StrictShape>");

  TripleConstraint tc1;
  tc1.predicate = Iri::fromIriref("<http://ex.org/p1>");
  tc1.minCount = 1;
  tc1.maxCount = 1;
  strictShape.tripleConstraints.push_back(tc1);

  TripleConstraint tc2;
  tc2.predicate = Iri::fromIriref("<http://ex.org/p2>");
  tc2.minCount = 2;
  tc2.maxCount = 5;
  strictShape.tripleConstraints.push_back(tc2);

  auto hints = strictShape.computeHints();
  // Should be quite selective due to multiple constraints
  EXPECT_LT(hints.selectivityFactor, 0.5);
}

TEST_F(ShapeSchemaManagerTest, RequiredPredicatesExtraction) {
  auto shape = createPersonShape();
  auto hints = shape.computeHints();

  EXPECT_GE(hints.requiredPredicates.size(), 2);
}

TEST_F(ShapeSchemaManagerTest, HintsForNonExistentShape) {
  auto hintsOpt = manager_.getOptimizationHints(
      Iri::fromIriref("<http://example.org/NoSuchShape>"));
  EXPECT_FALSE(hintsOpt.has_value());
}

TEST_F(ShapeSchemaManagerTest, MinMaxCardinalityExtraction) {
  auto shape = createPersonShape();
  auto hints = shape.computeHints();

  // Name is required (min=1, max=1)
  // Email is optional (min=0, max=*)
  EXPECT_TRUE(hints.minCardinality.has_value());
  EXPECT_EQ(hints.minCardinality.value(), 1);
}

TEST_F(ShapeSchemaManagerTest, EmptyShapeHints) {
  ShapeExpression emptyShape;
  emptyShape.id = Iri::fromIriref("<http://example.org/EmptyShape>");

  auto hints = emptyShape.computeHints();
  EXPECT_DOUBLE_EQ(hints.selectivityFactor, 1.0);
}

// ============================================================================
// Metadata Persistence Tests (10 tests)
// ============================================================================

TEST_F(ShapeSchemaManagerTest, SaveMetadataToFile) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  std::string filename = "/tmp/test_shape_metadata.json";
  EXPECT_NO_THROW(manager_.saveMetadata(filename));

  // Verify file exists
  std::ifstream file(filename);
  EXPECT_TRUE(file.good());
  file.close();

  // Cleanup
  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, LoadMetadataFromFile) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  std::string filename = "/tmp/test_shape_metadata_load.json";
  manager_.saveMetadata(filename);

  // Create new manager and load
  ShapeSchemaManager newManager;
  EXPECT_NO_THROW(newManager.loadMetadata(filename));

  EXPECT_EQ(newManager.shapeCount(), 1);
  EXPECT_TRUE(newManager.hasShape(shape.id));

  // Cleanup
  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, MetadataRoundTrip) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  std::string filename = "/tmp/test_shape_roundtrip.json";
  manager_.saveMetadata(filename);

  ShapeSchemaManager newManager;
  newManager.loadMetadata(filename);

  auto retrieved = newManager.getShape(shape.id);
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ((*retrieved)->label, shape.label);
  EXPECT_EQ((*retrieved)->tripleConstraints.size(),
            shape.tripleConstraints.size());

  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, SaveEmptyManager) {
  std::string filename = "/tmp/test_empty_metadata.json";
  EXPECT_NO_THROW(manager_.saveMetadata(filename));

  ShapeSchemaManager newManager;
  newManager.loadMetadata(filename);
  EXPECT_EQ(newManager.shapeCount(), 0);

  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, LoadFromJson) {
  std::string jsonContent = createTestShapeJson();
  EXPECT_NO_THROW(manager_.loadFromJson(jsonContent));

  EXPECT_GE(manager_.shapeCount(), 1);
}

TEST_F(ShapeSchemaManagerTest, LoadInvalidJson) {
  std::string invalidJson = "{invalid json}";
  EXPECT_ANY_THROW(manager_.loadFromJson(invalidJson));
}

TEST_F(ShapeSchemaManagerTest, MetadataPreservesConstraints) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  std::string filename = "/tmp/test_constraints.json";
  manager_.saveMetadata(filename);

  ShapeSchemaManager newManager;
  newManager.loadMetadata(filename);

  auto retrieved = newManager.getShape(shape.id);
  ASSERT_TRUE(retrieved.has_value());

  EXPECT_EQ((*retrieved)->tripleConstraints[0].minCount, 1);
  EXPECT_EQ((*retrieved)->tripleConstraints[0].maxCount, 1);

  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, LoadNonExistentFile) {
  EXPECT_ANY_THROW(manager_.loadFromFile("/nonexistent/file.json"));
}

TEST_F(ShapeSchemaManagerTest, MultipleShapesMetadata) {
  auto shape1 = createPersonShape();

  ShapeExpression shape2;
  shape2.id = Iri::fromIriref("<http://example.org/CompanyShape>");
  shape2.label = "CompanyShape";

  manager_.registerShape(shape1);
  manager_.registerShape(shape2);

  std::string filename = "/tmp/test_multiple_shapes.json";
  manager_.saveMetadata(filename);

  ShapeSchemaManager newManager;
  newManager.loadMetadata(filename);

  EXPECT_EQ(newManager.shapeCount(), 2);

  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, MetadataVersionCheck) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  std::string filename = "/tmp/test_version.json";
  manager_.saveMetadata(filename);

  // Read and verify version
  std::ifstream file(filename);
  nlohmann::json root;
  file >> root;
  file.close();

  EXPECT_TRUE(root.contains("version"));
  EXPECT_EQ(root["version"], "1.0");

  std::remove(filename.c_str());
}

// ============================================================================
// Statistics Tests (10 tests)
// ============================================================================

TEST_F(ShapeSchemaManagerTest, StatisticsInitiallyZero) {
  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 0);
  EXPECT_EQ(stats.successfulValidations, 0);
  EXPECT_EQ(stats.failedValidations, 0);
  EXPECT_EQ(stats.optimizationHintsUsed, 0);
}

TEST_F(ShapeSchemaManagerTest, RecordSuccessfulValidation) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(true, 0.8);

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 1);
  EXPECT_EQ(stats.successfulValidations, 1);
  EXPECT_EQ(stats.failedValidations, 0);
}

TEST_F(ShapeSchemaManagerTest, RecordFailedValidation) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(false);

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 1);
  EXPECT_EQ(stats.successfulValidations, 0);
  EXPECT_EQ(stats.failedValidations, 1);
}

TEST_F(ShapeSchemaManagerTest, RecordOptimizationHint) {
  manager_.setCollectStatistics(true);
  manager_.recordOptimizationHint();

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.optimizationHintsUsed, 1);
}

TEST_F(ShapeSchemaManagerTest, StatisticsDisabled) {
  manager_.setCollectStatistics(false);
  manager_.recordValidation(true);

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 0);
}

TEST_F(ShapeSchemaManagerTest, ResetStatistics) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(true);
  manager_.recordValidation(false);

  manager_.resetStatistics();

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 0);
  EXPECT_EQ(stats.successfulValidations, 0);
  EXPECT_EQ(stats.failedValidations, 0);
}

TEST_F(ShapeSchemaManagerTest, CardinalityReductionTracking) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(true, 0.7);
  manager_.recordValidation(true, 0.9);

  const auto& stats = manager_.getStatistics();
  EXPECT_GT(stats.avgCardinalityReduction, 0.0);
  EXPECT_DOUBLE_EQ(stats.avgCardinalityReduction, 0.8);
}

TEST_F(ShapeSchemaManagerTest, MultipleValidationsTracking) {
  manager_.setCollectStatistics(true);

  for (int i = 0; i < 10; ++i) {
    manager_.recordValidation(i % 2 == 0);
  }

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 10);
  EXPECT_EQ(stats.successfulValidations, 5);
  EXPECT_EQ(stats.failedValidations, 5);
}

TEST_F(ShapeSchemaManagerTest, StatisticsPersistence) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(true);
  manager_.recordOptimizationHint();

  std::string filename = "/tmp/test_stats_persist.json";
  manager_.saveMetadata(filename);

  ShapeSchemaManager newManager;
  newManager.loadMetadata(filename);

  const auto& stats = newManager.getStatistics();
  EXPECT_EQ(stats.totalValidations, 1);
  EXPECT_EQ(stats.optimizationHintsUsed, 1);

  std::remove(filename.c_str());
}

TEST_F(ShapeSchemaManagerTest, DisableStatisticsResetsCounters) {
  manager_.setCollectStatistics(true);
  manager_.recordValidation(true);

  manager_.setCollectStatistics(false);

  const auto& stats = manager_.getStatistics();
  EXPECT_EQ(stats.totalValidations, 0);
}

// ============================================================================
// Error Handling Tests (5 tests)
// ============================================================================

TEST_F(ShapeSchemaManagerTest, InvalidJsonHandling) {
  std::string invalidJson = "not valid json at all";
  EXPECT_ANY_THROW(manager_.loadFromJson(invalidJson));
}

TEST_F(ShapeSchemaManagerTest, MissingShapesFieldInJson) {
  std::string incompleteJson = R"({"version": "1.0"})";
  EXPECT_ANY_THROW(manager_.loadFromJson(incompleteJson));
}

TEST_F(ShapeSchemaManagerTest, ShapeWithoutId) {
  std::string json = R"({
    "shapes": [
      {"label": "NoIdShape"}
    ]
  })";

  // Should skip shape without ID
  EXPECT_NO_THROW(manager_.loadFromJson(json));
  EXPECT_EQ(manager_.shapeCount(), 0);
}

TEST_F(ShapeSchemaManagerTest, SaveToInvalidPath) {
  auto shape = createPersonShape();
  manager_.registerShape(shape);

  EXPECT_ANY_THROW(manager_.saveMetadata("/invalid/path/that/does/not/exist.json"));
}

TEST_F(ShapeSchemaManagerTest, LoadFromInvalidPath) {
  EXPECT_ANY_THROW(manager_.loadMetadata("/invalid/path/that/does/not/exist.json"));
}

#include <gtest/gtest.h>
#include "engine/shacl/RecursiveShapeValidator.h"
#include "engine/shacl/ShaclShapeRegistry.h"

namespace shacl {

class RecursiveShapeValidatorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<ShaclShapeRegistry>();
    validator_ = std::make_unique<RecursiveShapeValidator>(registry_.get());
  }

  void TearDown() override {
    validator_.reset();
    registry_.reset();
  }

  std::unique_ptr<ShaclShapeRegistry> registry_;
  std::unique_ptr<RecursiveShapeValidator> validator_;
};

// Test basic recursive validation with sh:node constraint
TEST_F(RecursiveShapeValidatorTest, BasicNodeConstraint) {
  // Create a base shape for addresses
  NodeShape addressShape;
  addressShape.shapeId = "http://example.org/AddressShape";

  ShaclConstraint nodeKindIRI;
  nodeKindIRI.type = ConstraintType::NodeKind;
  nodeKindIRI.value = NodeKind::IRI;
  addressShape.nodeConstraints.push_back(nodeKindIRI);

  // Create a person shape that references address shape
  NodeShape personShape;
  personShape.shapeId = "http://example.org/PersonShape";

  PropertyShape addressProp("http://example.org/hasAddress");
  ShaclConstraint nodeConstraint;
  nodeConstraint.type = ConstraintType::Node;
  nodeConstraint.value = std::string("http://example.org/AddressShape");
  addressProp.constraints.push_back(nodeConstraint);
  personShape.propertyShapes.push_back(addressProp);

  // Register shapes
  registry_->registerShape(addressShape);
  registry_->registerShape(personShape);

  // Validate
  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape(
      "http://example.org/person1", "http://example.org/PersonShape", context);

  // Should succeed (no actual property values to validate in this test)
  EXPECT_TRUE(result.conforms);
}

// Test circular reference detection
TEST_F(RecursiveShapeValidatorTest, CircularReferenceDetection) {
  // Create shape A that references shape B
  NodeShape shapeA;
  shapeA.shapeId = "http://example.org/ShapeA";

  ShaclConstraint nodeConstraintB;
  nodeConstraintB.type = ConstraintType::Node;
  nodeConstraintB.value = std::string("http://example.org/ShapeB");
  shapeA.nodeConstraints.push_back(nodeConstraintB);

  // Create shape B that references shape A (circular!)
  NodeShape shapeB;
  shapeB.shapeId = "http://example.org/ShapeB";

  ShaclConstraint nodeConstraintA;
  nodeConstraintA.type = ConstraintType::Node;
  nodeConstraintA.value = std::string("http://example.org/ShapeA");
  shapeB.nodeConstraints.push_back(nodeConstraintA);

  registry_->registerShape(shapeA);
  registry_->registerShape(shapeB);

  // Validate - should detect circular reference
  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape("http://example.org/node1",
                                                   "http://example.org/ShapeA",
                                                   context);

  // Should fail due to circular reference
  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);

  // Check that violation message mentions circular reference
  bool hasCircularMessage = false;
  for (const auto& violation : result.violations) {
    if (violation.find("Circular") != std::string::npos ||
        violation.find("circular") != std::string::npos) {
      hasCircularMessage = true;
      break;
    }
  }
  EXPECT_TRUE(hasCircularMessage);
}

// Test memoization - same node/shape combination should use cache
TEST_F(RecursiveShapeValidatorTest, MemoizationCache) {
  NodeShape simpleShape;
  simpleShape.shapeId = "http://example.org/SimpleShape";

  ShaclConstraint nodeKind;
  nodeKind.type = ConstraintType::NodeKind;
  nodeKind.value = NodeKind::IRI;
  simpleShape.nodeConstraints.push_back(nodeKind);

  registry_->registerShape(simpleShape);

  auto context = validator_->createContext();

  // First validation
  auto result1 = validator_->validateNodeWithShape(
      "http://example.org/node1", "http://example.org/SimpleShape", context);

  // Second validation of same node with same shape
  auto result2 = validator_->validateNodeWithShape(
      "http://example.org/node1", "http://example.org/SimpleShape", context);

  // Both should succeed
  EXPECT_TRUE(result1.conforms);
  EXPECT_TRUE(result2.conforms);

  // Cache should have been used for second validation
  EXPECT_TRUE(context.hasMemoized("http://example.org/node1",
                                  "http://example.org/SimpleShape"));

  // Should have at least one cache hit
  EXPECT_GT(validator_->getCacheHits(), 0);
}

// Test validation context stack management
TEST_F(RecursiveShapeValidatorTest, ValidationStackManagement) {
  auto context = validator_->createContext();

  EXPECT_FALSE(context.isInValidationStack("node1", "shape1"));

  context.pushValidation("node1", "shape1");
  EXPECT_TRUE(context.isInValidationStack("node1", "shape1"));
  EXPECT_FALSE(context.isInValidationStack("node2", "shape1"));
  EXPECT_FALSE(context.isInValidationStack("node1", "shape2"));

  context.pushValidation("node2", "shape2");
  EXPECT_TRUE(context.isInValidationStack("node1", "shape1"));
  EXPECT_TRUE(context.isInValidationStack("node2", "shape2"));

  context.popValidation();
  EXPECT_TRUE(context.isInValidationStack("node1", "shape1"));
  EXPECT_FALSE(context.isInValidationStack("node2", "shape2"));

  context.popValidation();
  EXPECT_FALSE(context.isInValidationStack("node1", "shape1"));
}

// Test RAII validation guard
TEST_F(RecursiveShapeValidatorTest, ValidationGuardRAII) {
  auto context = validator_->createContext();

  {
    RecursiveShapeValidator::ValidationGuard guard(context, "node1", "shape1");
    EXPECT_TRUE(context.isInValidationStack("node1", "shape1"));
  }

  // Guard should have cleaned up on destruction
  EXPECT_FALSE(context.isInValidationStack("node1", "shape1"));
}

// Test nested recursive validation (3 levels deep)
TEST_F(RecursiveShapeValidatorTest, NestedRecursiveValidation) {
  // Create shape hierarchy: ShapeA -> ShapeB -> ShapeC
  NodeShape shapeC;
  shapeC.shapeId = "http://example.org/ShapeC";
  ShaclConstraint nodeKindC;
  nodeKindC.type = ConstraintType::NodeKind;
  nodeKindC.value = NodeKind::IRI;
  shapeC.nodeConstraints.push_back(nodeKindC);

  NodeShape shapeB;
  shapeB.shapeId = "http://example.org/ShapeB";
  ShaclConstraint nodeConstraintC;
  nodeConstraintC.type = ConstraintType::Node;
  nodeConstraintC.value = std::string("http://example.org/ShapeC");
  shapeB.nodeConstraints.push_back(nodeConstraintC);

  NodeShape shapeA;
  shapeA.shapeId = "http://example.org/ShapeA";
  ShaclConstraint nodeConstraintB;
  nodeConstraintB.type = ConstraintType::Node;
  nodeConstraintB.value = std::string("http://example.org/ShapeB");
  shapeA.nodeConstraints.push_back(nodeConstraintB);

  registry_->registerShape(shapeA);
  registry_->registerShape(shapeB);
  registry_->registerShape(shapeC);

  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape("http://example.org/node1",
                                                   "http://example.org/ShapeA",
                                                   context);

  // Should succeed (all shapes validate node kind IRI)
  EXPECT_TRUE(result.conforms);
  EXPECT_EQ(result.violations.size(), 0);
}

// Test maximum recursion depth limit
TEST_F(RecursiveShapeValidatorTest, MaxRecursionDepthLimit) {
  // Create a chain of shapes that reference each other linearly
  // This will test the depth limit without circular references
  const int chainLength = 150;  // Exceeds default maxDepth of 100

  for (int i = 0; i < chainLength; ++i) {
    NodeShape shape;
    shape.shapeId = "http://example.org/Shape" + std::to_string(i);

    if (i < chainLength - 1) {
      ShaclConstraint nodeConstraint;
      nodeConstraint.type = ConstraintType::Node;
      nodeConstraint.value =
          std::string("http://example.org/Shape" + std::to_string(i + 1));
      shape.nodeConstraints.push_back(nodeConstraint);
    }

    registry_->registerShape(shape);
  }

  auto context = validator_->createContext(100);  // Set max depth to 100
  auto result = validator_->validateNodeWithShape(
      "http://example.org/node1", "http://example.org/Shape0", context);

  // Should fail due to depth limit
  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);

  // Check for depth limit message
  bool hasDepthMessage = false;
  for (const auto& violation : result.violations) {
    if (violation.find("depth") != std::string::npos ||
        violation.find("recursion") != std::string::npos) {
      hasDepthMessage = true;
      break;
    }
  }
  EXPECT_TRUE(hasDepthMessage);
}

// Test sh:shape constraint in property shapes
TEST_F(RecursiveShapeValidatorTest, ShapeReferenceInPropertyShape) {
  // Create a shape for validating email addresses
  NodeShape emailShape;
  emailShape.shapeId = "http://example.org/EmailShape";

  ShaclConstraint nodeKindLiteral;
  nodeKindLiteral.type = ConstraintType::NodeKind;
  nodeKindLiteral.value = NodeKind::Literal;
  emailShape.nodeConstraints.push_back(nodeKindLiteral);

  // Create a person shape with email property using sh:shape
  NodeShape personShape;
  personShape.shapeId = "http://example.org/PersonShape";

  PropertyShape emailProp("http://example.org/email");
  ShaclConstraint shapeConstraint;
  shapeConstraint.type = ConstraintType::Shape;
  shapeConstraint.value = std::string("http://example.org/EmailShape");
  emailProp.constraints.push_back(shapeConstraint);
  personShape.propertyShapes.push_back(emailProp);

  registry_->registerShape(emailShape);
  registry_->registerShape(personShape);

  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape(
      "http://example.org/person1", "http://example.org/PersonShape", context);

  // Should succeed (no property values in this simplified test)
  EXPECT_TRUE(result.conforms);
}

// Test validation of multiple nodes
TEST_F(RecursiveShapeValidatorTest, ValidateMultipleNodes) {
  NodeShape simpleShape;
  simpleShape.shapeId = "http://example.org/SimpleShape";

  ShaclConstraint nodeKind;
  nodeKind.type = ConstraintType::NodeKind;
  nodeKind.value = NodeKind::IRI;
  simpleShape.nodeConstraints.push_back(nodeKind);

  registry_->registerShape(simpleShape);

  std::vector<std::string> nodes = {"http://example.org/node1",
                                    "http://example.org/node2",
                                    "http://example.org/node3"};

  auto results = validator_->validateNodesWithShape(
      nodes, "http://example.org/SimpleShape");

  EXPECT_EQ(results.size(), 3);

  for (const auto& result : results) {
    EXPECT_TRUE(result.conforms);
  }
}

// Test shape not found error
TEST_F(RecursiveShapeValidatorTest, ShapeNotFoundError) {
  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape(
      "http://example.org/node1", "http://example.org/NonExistentShape",
      context);

  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);

  // Check for "not found" message
  bool hasNotFoundMessage = false;
  for (const auto& violation : result.violations) {
    if (violation.find("not found") != std::string::npos ||
        violation.find("Not found") != std::string::npos) {
      hasNotFoundMessage = true;
      break;
    }
  }
  EXPECT_TRUE(hasNotFoundMessage);
}

// Test memoization key generation
TEST_F(RecursiveShapeValidatorTest, MemoizationKeyGeneration) {
  auto key1 =
      RecursiveShapeValidator::ValidationContext::getMemoKey("node1", "shape1");
  auto key2 =
      RecursiveShapeValidator::ValidationContext::getMemoKey("node1", "shape1");
  auto key3 =
      RecursiveShapeValidator::ValidationContext::getMemoKey("node2", "shape1");
  auto key4 =
      RecursiveShapeValidator::ValidationContext::getMemoKey("node1", "shape2");

  // Same inputs should produce same key
  EXPECT_EQ(key1, key2);

  // Different inputs should produce different keys
  EXPECT_NE(key1, key3);
  EXPECT_NE(key1, key4);
  EXPECT_NE(key3, key4);
}

// Test clearCache functionality
TEST_F(RecursiveShapeValidatorTest, ClearCache) {
  NodeShape simpleShape;
  simpleShape.shapeId = "http://example.org/SimpleShape";
  registry_->registerShape(simpleShape);

  auto context = validator_->createContext();
  validator_->validateNodeWithShape("http://example.org/node1",
                                    "http://example.org/SimpleShape", context);

  EXPECT_GT(validator_->getCacheSize(), 0);

  validator_->clearCache();

  EXPECT_EQ(validator_->getCacheSize(), 0);
  EXPECT_EQ(validator_->getCacheHits(), 0);
  EXPECT_EQ(validator_->getValidationCount(), 0);
}

// Test complex recursive scenario: person has friends who are also persons
TEST_F(RecursiveShapeValidatorTest, RecursivePersonFriendship) {
  NodeShape personShape;
  personShape.shapeId = "http://example.org/PersonShape";

  // Person has friends that must also be persons (recursive!)
  PropertyShape friendProp("http://example.org/hasFriend");
  ShaclConstraint shapeConstraint;
  shapeConstraint.type = ConstraintType::Shape;
  shapeConstraint.value =
      std::string("http://example.org/PersonShape");  // Self-reference!
  friendProp.constraints.push_back(shapeConstraint);

  personShape.propertyShapes.push_back(friendProp);

  registry_->registerShape(personShape);

  auto context = validator_->createContext();
  auto result = validator_->validateNodeWithShape(
      "http://example.org/person1", "http://example.org/PersonShape", context);

  // Should succeed (self-referential shapes are allowed, just not circular
  // validation)
  EXPECT_TRUE(result.conforms);
}

// Test validation statistics tracking
TEST_F(RecursiveShapeValidatorTest, ValidationStatistics) {
  NodeShape simpleShape;
  simpleShape.shapeId = "http://example.org/SimpleShape";
  registry_->registerShape(simpleShape);

  validator_->clearCache();
  EXPECT_EQ(validator_->getValidationCount(), 0);
  EXPECT_EQ(validator_->getCacheHits(), 0);

  auto context = validator_->createContext();

  // First validation
  validator_->validateNodeWithShape("http://example.org/node1",
                                    "http://example.org/SimpleShape", context);
  EXPECT_EQ(validator_->getValidationCount(), 1);
  EXPECT_EQ(validator_->getCacheHits(), 0);

  // Second validation (should hit cache)
  validator_->validateNodeWithShape("http://example.org/node1",
                                    "http://example.org/SimpleShape", context);
  EXPECT_EQ(validator_->getValidationCount(), 2);
  EXPECT_EQ(validator_->getCacheHits(), 1);

  // Different node (no cache hit)
  validator_->validateNodeWithShape("http://example.org/node2",
                                    "http://example.org/SimpleShape", context);
  EXPECT_EQ(validator_->getValidationCount(), 3);
  EXPECT_EQ(validator_->getCacheHits(), 1);
}

// Test multiple concurrent validation contexts
TEST_F(RecursiveShapeValidatorTest, MultipleConcurrentContexts) {
  NodeShape simpleShape;
  simpleShape.shapeId = "http://example.org/SimpleShape";
  registry_->registerShape(simpleShape);

  auto context1 = validator_->createContext();
  auto context2 = validator_->createContext();

  // Validate with context1
  validator_->validateNodeWithShape("http://example.org/node1",
                                    "http://example.org/SimpleShape", context1);

  // Context2 should not have context1's memoization
  EXPECT_TRUE(context1.hasMemoized("http://example.org/node1",
                                   "http://example.org/SimpleShape"));
  EXPECT_FALSE(context2.hasMemoized("http://example.org/node1",
                                    "http://example.org/SimpleShape"));

  // Validate with context2
  validator_->validateNodeWithShape("http://example.org/node1",
                                    "http://example.org/SimpleShape", context2);

  // Now both should have it
  EXPECT_TRUE(context2.hasMemoized("http://example.org/node1",
                                   "http://example.org/SimpleShape"));
}

}  // namespace shacl

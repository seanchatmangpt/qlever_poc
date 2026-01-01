/**
 * SHACL Shapes Graph Validation Test
 *
 * This test suite validates the structure and semantics of SHACL shapes graphs
 * according to the W3C SHACL specification Section 2.1.2 and 2.1.3.
 *
 * A shapes graph is an RDF graph that contains SHACL shape definitions.
 * This test ensures:
 * - Proper shape identification and structure
 * - Valid shape composition and referencing
 * - Correct handling of shape hierarchies
 * - Shape metadata and annotations
 * - Shape closure and scoping
 *
 * Reference: https://www.w3.org/TR/shacl/#shapes-graph
 */

#include <gtest/gtest.h>
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include <memory>
#include <string>

namespace shacl {

class ShapesGraphValidationTest : public ::testing::Test {
 protected:
  ShaclShapeRegistry shapesGraph;

  void SetUp() override {
    shapesGraph = ShaclShapeRegistry();
  }

  // Helper to create a complete node shape
  NodeShape createCompleteNodeShape(
      const std::string& id,
      const std::vector<std::string>& targetClasses = {},
      const std::vector<std::string>& targetNodes = {}) {
    NodeShape shape;
    shape.shapeId = id;
    for (const auto& cls : targetClasses) {
      shape.addTargetClass(cls);
    }
    for (const auto& node : targetNodes) {
      shape.targetNodes.push_back(node);
    }
    return shape;
  }
};

// =============================================================================
// SECTION 1: Shape Identification and Structure
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeIdentification_NodeShape) {
  // Test that node shapes are properly identified
  // W3C SHACL 2.1.3.1 - Node Shapes

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  EXPECT_EQ(shape.shapeId, "http://example.org/shapes/PersonShape");
  EXPECT_TRUE(shape.hasTargets());
  EXPECT_EQ(shape.targetClasses.size(), 1);
}

TEST_F(ShapesGraphValidationTest, ShapeIdentification_PropertyShape) {
  // Test that property shapes are properly structured
  // W3C SHACL 2.1.3.2 - Property Shapes

  PropertyShape propShape("http://xmlns.com/foaf/0.1/name");

  ShaclConstraint datatypeConstraint;
  datatypeConstraint.type = ConstraintType::Datatype;
  datatypeConstraint.value = std::string("http://www.w3.org/2001/XMLSchema#string");
  propShape.constraints.push_back(datatypeConstraint);

  EXPECT_EQ(propShape.path, "http://xmlns.com/foaf/0.1/name");
  EXPECT_EQ(propShape.constraints.size(), 1);
}

TEST_F(ShapesGraphValidationTest, ShapeIdentification_MultipleShapes) {
  // Test shapes graph with multiple shapes
  // Validates that multiple shapes can coexist in the same graph

  NodeShape personShape = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  NodeShape organizationShape = createCompleteNodeShape(
      "http://example.org/shapes/OrganizationShape",
      {"http://xmlns.com/foaf/0.1/Organization"});

  shapesGraph.registerShape(personShape);
  shapesGraph.registerShape(organizationShape);

  auto allShapes = shapesGraph.getAllShapes();
  EXPECT_GE(allShapes.size(), 2);

  const NodeShape* retrievedPerson = shapesGraph.getShape(
      "http://example.org/shapes/PersonShape");
  const NodeShape* retrievedOrg = shapesGraph.getShape(
      "http://example.org/shapes/OrganizationShape");

  EXPECT_NE(retrievedPerson, nullptr);
  EXPECT_NE(retrievedOrg, nullptr);
  EXPECT_NE(retrievedPerson, retrievedOrg);
}

// =============================================================================
// SECTION 2: Shape Targets and Scoping
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeTargets_TargetClass) {
  // Test sh:targetClass functionality
  // W3C SHACL 2.1.3.1 - Target Class

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape",
      {"http://xmlns.com/foaf/0.1/Person",
       "http://example.org/Employee"});

  EXPECT_EQ(shape.targetClasses.size(), 2);
  EXPECT_TRUE(shape.isTargetClass("http://xmlns.com/foaf/0.1/Person"));
  EXPECT_TRUE(shape.isTargetClass("http://example.org/Employee"));
  EXPECT_FALSE(shape.isTargetClass("http://example.org/Customer"));
}

TEST_F(ShapesGraphValidationTest, ShapeTargets_TargetNode) {
  // Test sh:targetNode functionality
  // W3C SHACL 2.1.3.1 - Target Node

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/SpecificNodesShape",
      {},
      {"http://example.org/alice",
       "http://example.org/bob",
       "http://example.org/charlie"});

  EXPECT_EQ(shape.targetNodes.size(), 3);
  EXPECT_TRUE(shape.isTargetNode("http://example.org/alice"));
  EXPECT_TRUE(shape.isTargetNode("http://example.org/bob"));
  EXPECT_TRUE(shape.isTargetNode("http://example.org/charlie"));
  EXPECT_FALSE(shape.isTargetNode("http://example.org/dave"));
}

TEST_F(ShapesGraphValidationTest, ShapeTargets_CombinedTargets) {
  // Test shape with multiple target types
  // A shape can have both targetClass and targetNode

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/MixedTargetsShape",
      {"http://xmlns.com/foaf/0.1/Person"},
      {"http://example.org/specialCase"});

  EXPECT_TRUE(shape.hasTargets());
  EXPECT_EQ(shape.targetClasses.size(), 1);
  EXPECT_EQ(shape.targetNodes.size(), 1);
}

TEST_F(ShapesGraphValidationTest, ShapeTargets_NoTargets) {
  // Test shape without explicit targets
  // Some shapes may not have targets (e.g., referenced by other shapes)

  NodeShape shape;
  shape.shapeId = "http://example.org/shapes/AddressShape";

  EXPECT_FALSE(shape.hasTargets());
  EXPECT_EQ(shape.targetClasses.size(), 0);
  EXPECT_EQ(shape.targetNodes.size(), 0);
}

// =============================================================================
// SECTION 3: Shape Composition and Structure
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeComposition_PropertyShapes) {
  // Test node shape with multiple property shapes
  // W3C SHACL 2.1.3.2 - Property Shapes

  NodeShape personShape = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  // Name property
  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint nameMinCount;
  nameMinCount.type = ConstraintType::MinCount;
  nameMinCount.value = 1;
  nameShape.constraints.push_back(nameMinCount);
  personShape.addPropertyShape(nameShape);

  // Email property
  PropertyShape emailShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint emailMaxCount;
  emailMaxCount.type = ConstraintType::MaxCount;
  emailMaxCount.value = 1;
  emailShape.constraints.push_back(emailMaxCount);
  personShape.addPropertyShape(emailShape);

  // Age property
  PropertyShape ageShape("http://example.org/age");
  ShaclConstraint ageDatatype;
  ageDatatype.type = ConstraintType::Datatype;
  ageDatatype.value = std::string("http://www.w3.org/2001/XMLSchema#integer");
  ageShape.constraints.push_back(ageDatatype);
  personShape.addPropertyShape(ageShape);

  EXPECT_EQ(personShape.propertyShapes.size(), 3);
  EXPECT_EQ(personShape.propertyShapes[0].path, "http://xmlns.com/foaf/0.1/name");
  EXPECT_EQ(personShape.propertyShapes[1].path, "http://xmlns.com/foaf/0.1/mbox");
  EXPECT_EQ(personShape.propertyShapes[2].path, "http://example.org/age");
}

TEST_F(ShapesGraphValidationTest, ShapeComposition_NodeConstraints) {
  // Test node-level constraints (not property-specific)
  // W3C SHACL 2.1.3.1 - Node Shapes with constraints

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/IRIShape",
      {"http://example.org/Resource"});

  ShaclConstraint nodeKindConstraint;
  nodeKindConstraint.type = ConstraintType::NodeKind;
  nodeKindConstraint.value = NodeKind::IRI;
  shape.addConstraint(nodeKindConstraint);

  EXPECT_EQ(shape.nodeConstraints.size(), 1);
  EXPECT_EQ(shape.nodeConstraints[0].type, ConstraintType::NodeKind);
}

TEST_F(ShapesGraphValidationTest, ShapeComposition_ClosedShape) {
  // Test closed shapes
  // W3C SHACL 4.2 - sh:closed

  NodeShape closedShape = createCompleteNodeShape(
      "http://example.org/shapes/ClosedPersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  closedShape.closed = true;

  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");
  closedShape.addPropertyShape(nameShape);

  PropertyShape emailShape("http://xmlns.com/foaf/0.1/mbox");
  closedShape.addPropertyShape(emailShape);

  EXPECT_TRUE(closedShape.closed);
  EXPECT_EQ(closedShape.propertyShapes.size(), 2);
}

// =============================================================================
// SECTION 4: Shape Registry and Discovery
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeRegistry_RegisterShape) {
  // Test shape registration in shapes graph

  NodeShape shape = createCompleteNodeShape(
      "http://example.org/shapes/TestShape",
      {"http://example.org/TestClass"});

  shapesGraph.registerShape(shape);

  const NodeShape* retrieved = shapesGraph.getShape(
      "http://example.org/shapes/TestShape");

  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->shapeId, "http://example.org/shapes/TestShape");
}

TEST_F(ShapesGraphValidationTest, ShapeRegistry_GetShapesByClass) {
  // Test discovery of shapes by target class

  NodeShape shape1 = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape1",
      {"http://xmlns.com/foaf/0.1/Person"});

  NodeShape shape2 = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape2",
      {"http://xmlns.com/foaf/0.1/Person"});

  NodeShape shape3 = createCompleteNodeShape(
      "http://example.org/shapes/OrgShape",
      {"http://xmlns.com/foaf/0.1/Organization"});

  shapesGraph.registerShape(shape1);
  shapesGraph.registerShape(shape2);
  shapesGraph.registerShape(shape3);

  auto personShapes = shapesGraph.getShapesForClass(
      "http://xmlns.com/foaf/0.1/Person");

  EXPECT_EQ(personShapes.size(), 2);
}

TEST_F(ShapesGraphValidationTest, ShapeRegistry_GetShapesByNode) {
  // Test discovery of shapes by target node

  NodeShape shape1 = createCompleteNodeShape(
      "http://example.org/shapes/AliceShape",
      {},
      {"http://example.org/alice"});

  NodeShape shape2 = createCompleteNodeShape(
      "http://example.org/shapes/AliceAndBobShape",
      {},
      {"http://example.org/alice", "http://example.org/bob"});

  shapesGraph.registerShape(shape1);
  shapesGraph.registerShape(shape2);

  auto aliceShapes = shapesGraph.getShapesForNode("http://example.org/alice");

  EXPECT_EQ(aliceShapes.size(), 2);
}

TEST_F(ShapesGraphValidationTest, ShapeRegistry_GetAllShapes) {
  // Test retrieval of all shapes

  NodeShape shape1 = createCompleteNodeShape(
      "http://example.org/shapes/Shape1",
      {"http://example.org/Class1"});

  NodeShape shape2 = createCompleteNodeShape(
      "http://example.org/shapes/Shape2",
      {"http://example.org/Class2"});

  NodeShape shape3 = createCompleteNodeShape(
      "http://example.org/shapes/Shape3",
      {"http://example.org/Class3"});

  shapesGraph.registerShape(shape1);
  shapesGraph.registerShape(shape2);
  shapesGraph.registerShape(shape3);

  auto allShapes = shapesGraph.getAllShapes();

  EXPECT_GE(allShapes.size(), 3);
}

// =============================================================================
// SECTION 5: Shape Semantics and Validation
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeSemantics_RequiredProperties) {
  // Test semantics of required properties (minCount >= 1)

  PropertyShape requiredProp("http://example.org/requiredProperty");
  requiredProp.required = true;

  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 1;
  requiredProp.constraints.push_back(minCount);

  EXPECT_TRUE(requiredProp.required);
  EXPECT_EQ(requiredProp.constraints.size(), 1);
}

TEST_F(ShapesGraphValidationTest, ShapeSemantics_SingleValuedProperties) {
  // Test semantics of single-valued properties (maxCount = 1)

  PropertyShape singleValuedProp("http://example.org/singleProperty");

  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 1;
  singleValuedProp.constraints.push_back(maxCount);

  // Validate single value
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", singleValuedProp, {"\"value\""});
  EXPECT_TRUE(validResult.conforms);

  // Validate multiple values (should fail)
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", singleValuedProp,
      {"\"value1\"", "\"value2\""});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(ShapesGraphValidationTest, ShapeSemantics_OptionalProperties) {
  // Test semantics of optional properties (no minCount or minCount = 0)

  PropertyShape optionalProp("http://example.org/optionalProperty");

  // No minCount constraint means optional

  // Validate with no values (should pass)
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", optionalProp, {});
  EXPECT_TRUE(result.conforms);
}

// =============================================================================
// SECTION 6: Shape Metadata and Annotations
// =============================================================================

TEST_F(ShapesGraphValidationTest, ShapeMetadata_Messages) {
  // Test constraint messages (sh:message)

  ShaclConstraint constraintWithMessage;
  constraintWithMessage.type = ConstraintType::MinCount;
  constraintWithMessage.value = 1;
  constraintWithMessage.message = "Property is required";

  EXPECT_EQ(constraintWithMessage.message, "Property is required");
}

TEST_F(ShapesGraphValidationTest, ShapeMetadata_Severity) {
  // Test severity levels (sh:severity)

  ShaclConstraint violation;
  violation.type = ConstraintType::MinCount;
  violation.value = 1;
  violation.severity = SeverityLevel::Violation;

  ShaclConstraint warning;
  warning.type = ConstraintType::MaxLength;
  warning.value = 100;
  warning.severity = SeverityLevel::Warning;

  ShaclConstraint info;
  info.type = ConstraintType::Pattern;
  info.value = std::string(".*");
  info.severity = SeverityLevel::Info;

  EXPECT_EQ(violation.severity, SeverityLevel::Violation);
  EXPECT_EQ(warning.severity, SeverityLevel::Warning);
  EXPECT_EQ(info.severity, SeverityLevel::Info);
}

// =============================================================================
// SECTION 7: Complex Shapes Graph Scenarios
// =============================================================================

TEST_F(ShapesGraphValidationTest, ComplexScenario_ComprehensivePersonShape) {
  // Test a comprehensive person shape with multiple constraints

  NodeShape personShape = createCompleteNodeShape(
      "http://example.org/shapes/PersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  // Required name (1..1)
  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint nameMinCount;
  nameMinCount.type = ConstraintType::MinCount;
  nameMinCount.value = 1;
  nameMinCount.message = "Person must have a name";
  nameShape.constraints.push_back(nameMinCount);

  ShaclConstraint nameMaxCount;
  nameMaxCount.type = ConstraintType::MaxCount;
  nameMaxCount.value = 1;
  nameShape.constraints.push_back(nameMaxCount);

  ShaclConstraint nameMinLength;
  nameMinLength.type = ConstraintType::MinLength;
  nameMinLength.value = 1;
  nameShape.constraints.push_back(nameMinLength);

  nameShape.required = true;
  personShape.addPropertyShape(nameShape);

  // Optional email with pattern (0..1)
  PropertyShape emailShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint emailMaxCount;
  emailMaxCount.type = ConstraintType::MaxCount;
  emailMaxCount.value = 1;
  emailShape.constraints.push_back(emailMaxCount);

  ShaclConstraint emailPattern;
  emailPattern.type = ConstraintType::Pattern;
  emailPattern.value = std::string("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
  emailPattern.severity = SeverityLevel::Warning;
  emailPattern.message = "Email format is invalid";
  emailShape.constraints.push_back(emailPattern);
  personShape.addPropertyShape(emailShape);

  // Optional age with datatype and range (0..1)
  PropertyShape ageShape("http://example.org/age");
  ShaclConstraint ageDatatype;
  ageDatatype.type = ConstraintType::Datatype;
  ageDatatype.value = std::string("http://www.w3.org/2001/XMLSchema#integer");
  ageShape.constraints.push_back(ageDatatype);

  ShaclConstraint ageMin;
  ageMin.type = ConstraintType::MinInclusive;
  ageMin.value = std::string("0");
  ageShape.constraints.push_back(ageMin);

  ShaclConstraint ageMax;
  ageMax.type = ConstraintType::MaxInclusive;
  ageMax.value = std::string("150");
  ageShape.constraints.push_back(ageMax);
  personShape.addPropertyShape(ageShape);

  // Register shape
  shapesGraph.registerShape(personShape);

  // Verify shape structure
  const NodeShape* retrieved = shapesGraph.getShape(
      "http://example.org/shapes/PersonShape");

  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->propertyShapes.size(), 3);
  EXPECT_TRUE(retrieved->hasTargets());
  EXPECT_EQ(retrieved->targetClasses.size(), 1);

  // Verify property shapes
  EXPECT_EQ(retrieved->propertyShapes[0].path, "http://xmlns.com/foaf/0.1/name");
  EXPECT_TRUE(retrieved->propertyShapes[0].required);
  EXPECT_EQ(retrieved->propertyShapes[0].constraints.size(), 3);

  EXPECT_EQ(retrieved->propertyShapes[1].path, "http://xmlns.com/foaf/0.1/mbox");
  EXPECT_EQ(retrieved->propertyShapes[1].constraints.size(), 2);

  EXPECT_EQ(retrieved->propertyShapes[2].path, "http://example.org/age");
  EXPECT_EQ(retrieved->propertyShapes[2].constraints.size(), 3);
}

TEST_F(ShapesGraphValidationTest, ComplexScenario_MultipleShapesWithSharedTargets) {
  // Test multiple shapes targeting the same class
  // Each shape may validate different aspects

  NodeShape basicPersonShape = createCompleteNodeShape(
      "http://example.org/shapes/BasicPersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint nameMinCount;
  nameMinCount.type = ConstraintType::MinCount;
  nameMinCount.value = 1;
  nameShape.constraints.push_back(nameMinCount);
  basicPersonShape.addPropertyShape(nameShape);

  NodeShape advancedPersonShape = createCompleteNodeShape(
      "http://example.org/shapes/AdvancedPersonShape",
      {"http://xmlns.com/foaf/0.1/Person"});

  PropertyShape emailShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint emailPattern;
  emailPattern.type = ConstraintType::Pattern;
  emailPattern.value = std::string("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
  emailShape.constraints.push_back(emailPattern);
  advancedPersonShape.addPropertyShape(emailShape);

  shapesGraph.registerShape(basicPersonShape);
  shapesGraph.registerShape(advancedPersonShape);

  auto personShapes = shapesGraph.getShapesForClass("http://xmlns.com/foaf/0.1/Person");
  EXPECT_EQ(personShapes.size(), 2);
}

}  // namespace shacl

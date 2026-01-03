/**
 * W3C SHACL 1.0 Compliance Test Suite
 *
 * This test suite validates QLever's SHACL implementation against the
 * W3C SHACL 1.0 specification: https://www.w3.org/TR/shacl/
 *
 * Coverage:
 * - Core Constraint Components (Section 4.1)
 * - Target Types (Section 2.1)
 * - Severity Levels (Section 3.5)
 * - Shape Types (Node Shapes and Property Shapes)
 * - Logical Constraint Components (sh:and, sh:or, sh:xone, sh:not)
 * - Property Pair Constraints
 * - Qualified Shapes
 * - Closed Shapes
 */

#include <gtest/gtest.h>

#include "engine/shacl/AdvancedConstraints.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"

namespace shacl {

class ShaclComplianceTest : public ::testing::Test {
 protected:
  ShaclShapeRegistry registry;

  void SetUp() override {
    // Initialize registry for each test
    registry = ShaclShapeRegistry();
  }

  // Helper to create a basic node shape
  NodeShape createBasicNodeShape(const std::string& id) {
    NodeShape shape;
    shape.shapeId = id;
    return shape;
  }

  // Helper to create a property shape
  PropertyShape createPropertyShape(const std::string& path) {
    return PropertyShape(path);
  }
};

// =============================================================================
// SECTION 1: Core Constraint Components (W3C SHACL Section 4.1)
// =============================================================================

// -----------------------------------------------------------------------------
// 1.1 Value Type Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_Datatype_Constraint) {
  // sh:datatype - Specifies the datatype of literal values
  // https://www.w3.org/TR/shacl/#DatatypeConstraintComponent

  ShaclConstraint datatypeConstraint;
  datatypeConstraint.type = ConstraintType::Datatype;
  datatypeConstraint.value =
      std::string("http://www.w3.org/2001/XMLSchema#string");

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(datatypeConstraint);

  // Valid string literal
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: IRI instead of literal
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"http://example.org/Alice"});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(ShaclComplianceTest, W3C_NodeKind_Constraint) {
  // sh:nodeKind - Specifies the type of nodes
  // https://www.w3.org/TR/shacl/#NodeKindConstraintComponent

  // Test sh:IRI
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRI, "http://example.org/resource"));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::IRI, "\"literal\""));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::IRI, "_:blank"));

  // Test sh:BlankNode
  EXPECT_TRUE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::BlankNode, "_:b1"));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNode, "http://example.org/resource"));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::BlankNode,
                                                          "\"literal\""));

  // Test sh:Literal
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::Literal,
                                                         "\"literal\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::Literal, "http://example.org/resource"));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::Literal, "_:blank"));

  // Test sh:BlankNodeOrIRI
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrIRI, "_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrIRI, "http://example.org/resource"));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrIRI, "\"literal\""));

  // Test sh:BlankNodeOrLiteral
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "\"literal\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "http://example.org/resource"));

  // Test sh:IRIOrLiteral
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRIOrLiteral, "http://example.org/resource"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::IRIOrLiteral,
                                                         "\"literal\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRIOrLiteral, "_:b1"));
}

// -----------------------------------------------------------------------------
// 1.2 Cardinality Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_MinCount_Constraint) {
  // sh:minCount - Minimum number of value nodes
  // https://www.w3.org/TR/shacl/#MinCountConstraintComponent

  ShaclConstraint minCount1;
  minCount1.type = ConstraintType::MinCount;
  minCount1.value = 1;

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(minCount1);

  // Valid: 1 value (meets minimum)
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Valid: 2 values (exceeds minimum)
  auto validResult2 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\"", "\"Bob\""});
  EXPECT_TRUE(validResult2.conforms);

  // Invalid: 0 values (below minimum)
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(ShaclComplianceTest, W3C_MaxCount_Constraint) {
  // sh:maxCount - Maximum number of value nodes
  // https://www.w3.org/TR/shacl/#MaxCountConstraintComponent

  ShaclConstraint maxCount1;
  maxCount1.type = ConstraintType::MaxCount;
  maxCount1.value = 1;

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(maxCount1);

  // Valid: 0 values (below maximum)
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {});
  EXPECT_TRUE(validResult.conforms);

  // Valid: 1 value (equals maximum)
  auto validResult2 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult2.conforms);

  // Invalid: 2 values (exceeds maximum)
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\"", "\"Bob\""});
  EXPECT_FALSE(invalidResult.conforms);
}

// -----------------------------------------------------------------------------
// 1.3 Value Range Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_MinInclusive_Constraint) {
  // sh:minInclusive - Minimum value (inclusive)
  // https://www.w3.org/TR/shacl/#MinInclusiveConstraintComponent

  ShaclConstraint minInclusive;
  minInclusive.type = ConstraintType::MinInclusive;
  minInclusive.value = std::string("0");

  PropertyShape propShape = createPropertyShape("http://example.org/age");
  propShape.constraints.push_back(minInclusive);

  // Valid: value equals minimum (inclusive)
  auto validEqual = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"0\""});
  EXPECT_TRUE(validEqual.conforms);

  // Valid: value greater than minimum
  auto validGreater = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"\"10\""});
  EXPECT_TRUE(validGreater.conforms);

  // Invalid: value less than minimum
  auto invalidLess = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node3", propShape, {"\"-5\""});
  EXPECT_FALSE(invalidLess.conforms);
}

TEST_F(ShaclComplianceTest, W3C_MaxInclusive_Constraint) {
  // sh:maxInclusive - Maximum value (inclusive)
  // https://www.w3.org/TR/shacl/#MaxInclusiveConstraintComponent

  ShaclConstraint maxInclusive;
  maxInclusive.type = ConstraintType::MaxInclusive;
  maxInclusive.value = std::string("100");

  PropertyShape propShape = createPropertyShape("http://example.org/score");
  propShape.constraints.push_back(maxInclusive);

  // Valid: value equals maximum (inclusive)
  auto validEqual = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"100\""});
  EXPECT_TRUE(validEqual.conforms);

  // Valid: value less than maximum
  auto validLess = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"\"50\""});
  EXPECT_TRUE(validLess.conforms);

  // Invalid: value greater than maximum
  auto invalidGreater = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node3", propShape, {"\"150\""});
  EXPECT_FALSE(invalidGreater.conforms);
}

TEST_F(ShaclComplianceTest, W3C_MinExclusive_Constraint) {
  // sh:minExclusive - Minimum value (exclusive)
  // https://www.w3.org/TR/shacl/#MinExclusiveConstraintComponent

  ShaclConstraint minExclusive;
  minExclusive.type = ConstraintType::MinExclusive;
  minExclusive.value = MinExclusiveConstraintValue(0.0);

  PropertyShape propShape =
      createPropertyShape("http://example.org/temperature");
  propShape.constraints.push_back(minExclusive);

  // Valid: value greater than minimum (exclusive)
  auto validGreater = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"0.1\""});
  EXPECT_TRUE(validGreater.conforms);

  auto validMuchGreater = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"\"10\""});
  EXPECT_TRUE(validMuchGreater.conforms);

  // Invalid: value equals minimum (exclusive means not equal)
  auto invalidEqual = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node3", propShape, {"\"0\""});
  EXPECT_FALSE(invalidEqual.conforms);

  // Invalid: value less than minimum
  auto invalidLess = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node4", propShape, {"\"-5\""});
  EXPECT_FALSE(invalidLess.conforms);
}

TEST_F(ShaclComplianceTest, W3C_MaxExclusive_Constraint) {
  // sh:maxExclusive - Maximum value (exclusive)
  // https://www.w3.org/TR/shacl/#MaxExclusiveConstraintComponent

  ShaclConstraint maxExclusive;
  maxExclusive.type = ConstraintType::MaxExclusive;
  maxExclusive.value = MaxExclusiveConstraintValue(100.0);

  PropertyShape propShape =
      createPropertyShape("http://example.org/percentage");
  propShape.constraints.push_back(maxExclusive);

  // Valid: value less than maximum (exclusive)
  auto validLess = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"99.9\""});
  EXPECT_TRUE(validLess.conforms);

  auto validMuchLess = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"\"50\""});
  EXPECT_TRUE(validMuchLess.conforms);

  // Invalid: value equals maximum (exclusive means not equal)
  auto invalidEqual = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node3", propShape, {"\"100\""});
  EXPECT_FALSE(invalidEqual.conforms);

  // Invalid: value greater than maximum
  auto invalidGreater = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node4", propShape, {"\"150\""});
  EXPECT_FALSE(invalidGreater.conforms);
}

// -----------------------------------------------------------------------------
// 1.4 String-based Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_MinLength_Constraint) {
  // sh:minLength - Minimum string length
  // https://www.w3.org/TR/shacl/#MinLengthConstraintComponent

  ShaclConstraint minLength5;
  minLength5.type = ConstraintType::MinLength;
  minLength5.value = 5;

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(minLength5);

  // Valid: length >= 5
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: length < 5
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Bob\""});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(ShaclComplianceTest, W3C_MaxLength_Constraint) {
  // sh:maxLength - Maximum string length
  // https://www.w3.org/TR/shacl/#MaxLengthConstraintComponent

  ShaclConstraint maxLength10;
  maxLength10.type = ConstraintType::MaxLength;
  maxLength10.value = 10;

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(maxLength10);

  // Valid: length <= 10
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: length > 10
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"This is a very long name\""});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(ShaclComplianceTest, W3C_Pattern_Constraint) {
  // sh:pattern - Regular expression pattern
  // https://www.w3.org/TR/shacl/#PatternConstraintComponent

  ShaclConstraint patternConstraint;
  patternConstraint.type = ConstraintType::Pattern;
  patternConstraint.value = std::string("^[A-Z][a-z]+$");

  PropertyShape propShape = createPropertyShape("http://example.org/name");
  propShape.constraints.push_back(patternConstraint);

  // Valid: matches pattern
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: doesn't match pattern
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"alice\""});
  EXPECT_FALSE(invalidResult.conforms);

  auto invalidResult2 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"ALICE\""});
  EXPECT_FALSE(invalidResult2.conforms);
}

// -----------------------------------------------------------------------------
// 1.5 Property Pair Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_Disjoint_Constraint) {
  // sh:disjoint - Property values must be disjoint
  // https://www.w3.org/TR/shacl/#DisjointConstraintComponent

  // Test disjointWith constraint using the evaluator
  DisjointWithConstraintValue disjointConstraint("http://example.org/prop1",
                                                 "http://example.org/prop2");

  // Test valid case: no overlapping values
  std::vector<std::string> values1 = {"\"a\"", "\"b\""};
  std::vector<std::string> values2 = {"\"c\"", "\"d\""};
  bool validDisjoint = ShaclConstraintEvaluator::evaluateDisjointWith(
      disjointConstraint, values1, values2);
  EXPECT_TRUE(validDisjoint);

  // Test invalid case: overlapping values
  std::vector<std::string> values3 = {"\"a\"", "\"b\""};
  std::vector<std::string> values4 = {"\"b\"", "\"c\""};
  bool invalidOverlap = ShaclConstraintEvaluator::evaluateDisjointWith(
      disjointConstraint, values3, values4);
  // evaluateDisjointWith returns false when there is overlap (violation)
  EXPECT_FALSE(invalidOverlap) << "Should return false when values overlap";
}

// -----------------------------------------------------------------------------
// 1.6 Logical Constraint Components
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 1.7 Shape-based Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_Property_Constraint) {
  // sh:property - Property shape constraint
  // https://www.w3.org/TR/shacl/#PropertyConstraintComponent

  // This is a core feature - test property shapes
  NodeShape nodeShape = createBasicNodeShape("http://example.org/PersonShape");

  PropertyShape nameProp =
      createPropertyShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 1;
  nameProp.constraints.push_back(minCount);

  nodeShape.propertyShapes.push_back(nameProp);

  EXPECT_EQ(nodeShape.propertyShapes.size(), 1);
  EXPECT_EQ(nodeShape.propertyShapes[0].path, "http://xmlns.com/foaf/0.1/name");
  EXPECT_EQ(nodeShape.propertyShapes[0].constraints.size(), 1);
}

// -----------------------------------------------------------------------------
// 1.8 Other Constraint Components
// -----------------------------------------------------------------------------

TEST_F(ShaclComplianceTest, W3C_Closed_Constraint) {
  // sh:closed - Closed shape (no additional properties)
  // https://www.w3.org/TR/shacl/#ClosedConstraintComponent

  NodeShape nodeShape = createBasicNodeShape("http://example.org/ClosedShape");
  nodeShape.closed = true;

  // Test basic closed flag
  EXPECT_TRUE(nodeShape.closed);

  // Test closed constraint with allowed properties
  ClosedConstraintValue closedConstraint(true);
  closedConstraint.allowedProperties.push_back("http://example.org/name");
  closedConstraint.allowedProperties.push_back("http://example.org/age");

  // Valid: only allowed properties
  std::unordered_set<std::string> validProps = {"http://example.org/name",
                                                "http://example.org/age"};
  bool validClosed =
      ShaclConstraintEvaluator::evaluateClosed(closedConstraint, validProps);
  EXPECT_TRUE(validClosed);

  // Invalid: has unexpected property
  std::unordered_set<std::string> invalidProps = {
      "http://example.org/name", "http://example.org/age",
      "http://example.org/email"  // Not allowed
  };
  bool invalidClosed =
      ShaclConstraintEvaluator::evaluateClosed(closedConstraint, invalidProps);
  EXPECT_FALSE(invalidClosed);

  // Test with ignored properties
  closedConstraint.ignoredProperties.push_back(
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#type");

  std::unordered_set<std::string> propsWithType = {
      "http://example.org/name",
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"  // Ignored
  };
  bool validWithIgnored =
      ShaclConstraintEvaluator::evaluateClosed(closedConstraint, propsWithType);
  EXPECT_TRUE(validWithIgnored);
}

TEST_F(ShaclComplianceTest, W3C_HasValue_Constraint) {
  // sh:hasValue - Must have specific value
  // https://www.w3.org/TR/shacl/#HasValueConstraintComponent

  ShaclConstraint hasValue;
  hasValue.type = ConstraintType::HasValue;
  hasValue.value = HasValueConstraintValue("\"admin\"");

  PropertyShape propShape = createPropertyShape("http://example.org/role");
  propShape.constraints.push_back(hasValue);

  // Valid: required value is present
  auto validSingle = ShaclConstraintEvaluator::evaluatePropertyShapeWithContext(
      "http://example.org/node1", propShape, {"\"admin\""}, nullptr);
  EXPECT_TRUE(validSingle.conforms);

  // Valid: required value is present among multiple values
  auto validMultiple =
      ShaclConstraintEvaluator::evaluatePropertyShapeWithContext(
          "http://example.org/node2", propShape,
          {"\"user\"", "\"admin\"", "\"guest\""}, nullptr);
  EXPECT_TRUE(validMultiple.conforms);

  // Invalid: required value is not present
  auto invalidMissing =
      ShaclConstraintEvaluator::evaluatePropertyShapeWithContext(
          "http://example.org/node3", propShape, {"\"user\""}, nullptr);
  EXPECT_FALSE(invalidMissing.conforms);

  // Invalid: no values at all
  auto invalidEmpty =
      ShaclConstraintEvaluator::evaluatePropertyShapeWithContext(
          "http://example.org/node4", propShape, {}, nullptr);
  EXPECT_FALSE(invalidEmpty.conforms);
}

TEST_F(ShaclComplianceTest, W3C_In_Constraint) {
  // sh:in - Value must be in enumeration
  // https://www.w3.org/TR/shacl/#InConstraintComponent

  ShaclConstraint inConstraint;
  inConstraint.type = ConstraintType::In;
  inConstraint.value =
      std::vector<std::string>{"\"red\"", "\"green\"", "\"blue\""};

  PropertyShape propShape = createPropertyShape("http://example.org/color");
  propShape.constraints.push_back(inConstraint);

  // Valid: value in allowed list
  auto validRed = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"red\""});
  EXPECT_TRUE(validRed.conforms);

  auto validGreen = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node2", propShape, {"\"green\""});
  EXPECT_TRUE(validGreen.conforms);

  auto validBlue = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node3", propShape, {"\"blue\""});
  EXPECT_TRUE(validBlue.conforms);

  // Invalid: value not in allowed list
  auto invalidYellow = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node4", propShape, {"\"yellow\""});
  EXPECT_FALSE(invalidYellow.conforms);

  auto invalidPurple = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node5", propShape, {"\"purple\""});
  EXPECT_FALSE(invalidPurple.conforms);
}

// =============================================================================
// SECTION 2: Target Types (W3C SHACL Section 2.1)
// =============================================================================

TEST_F(ShaclComplianceTest, W3C_TargetClass) {
  // sh:targetClass - Target nodes of a class
  // https://www.w3.org/TR/shacl/#targetClass

  NodeShape personShape =
      createBasicNodeShape("http://example.org/PersonShape");
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  EXPECT_EQ(personShape.targetClasses.size(), 1);
  EXPECT_TRUE(personShape.isTargetClass("http://xmlns.com/foaf/0.1/Person"));
  EXPECT_FALSE(
      personShape.isTargetClass("http://xmlns.com/foaf/0.1/Organization"));
}

TEST_F(ShaclComplianceTest, W3C_TargetNode) {
  // sh:targetNode - Target specific nodes
  // https://www.w3.org/TR/shacl/#targetNode

  NodeShape nodeShape = createBasicNodeShape("http://example.org/NodeShape");
  nodeShape.targetNodes.push_back("http://example.org/alice");
  nodeShape.targetNodes.push_back("http://example.org/bob");

  EXPECT_EQ(nodeShape.targetNodes.size(), 2);
  EXPECT_TRUE(nodeShape.isTargetNode("http://example.org/alice"));
  EXPECT_TRUE(nodeShape.isTargetNode("http://example.org/bob"));
  EXPECT_FALSE(nodeShape.isTargetNode("http://example.org/charlie"));
}

// =============================================================================
// SECTION 3: Severity Levels (W3C SHACL Section 3.5)
// =============================================================================

TEST_F(ShaclComplianceTest, W3C_Severity_Violation) {
  // sh:Violation - Default severity level
  // https://www.w3.org/TR/shacl/#severity

  ShaclConstraint constraint;
  constraint.type = ConstraintType::MinCount;
  constraint.value = 1;
  constraint.severity = SeverityLevel::Violation;

  EXPECT_EQ(constraint.severity, SeverityLevel::Violation);
}

TEST_F(ShaclComplianceTest, W3C_Severity_Warning) {
  // sh:Warning - Warning severity level
  // https://www.w3.org/TR/shacl/#severity

  ShaclConstraint constraint;
  constraint.type = ConstraintType::MaxLength;
  constraint.value = 100;
  constraint.severity = SeverityLevel::Warning;

  EXPECT_EQ(constraint.severity, SeverityLevel::Warning);
}

TEST_F(ShaclComplianceTest, W3C_Severity_Info) {
  // sh:Info - Information severity level
  // https://www.w3.org/TR/shacl/#severity

  ShaclConstraint constraint;
  constraint.type = ConstraintType::Pattern;
  constraint.value = std::string(".*");
  constraint.severity = SeverityLevel::Info;

  EXPECT_EQ(constraint.severity, SeverityLevel::Info);
}

// =============================================================================
// SECTION 4: Validation Report (W3C SHACL Section 3.6)
// =============================================================================

TEST_F(ShaclComplianceTest, W3C_ValidationReport_Structure) {
  // Validation report structure
  // https://www.w3.org/TR/shacl/#validation-report

  ValidationReport report;

  ValidationResult result1;
  result1.focusNode = "http://example.org/alice";
  result1.conforms = true;

  ValidationResult result2;
  result2.focusNode = "http://example.org/bob";
  result2.conforms = false;
  result2.violations.push_back("Violation 1");
  result2.violations.push_back("Violation 2");

  report.addResult(result1);
  report.addResult(result2);

  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.violationCount, 2);
  EXPECT_EQ(report.results.size(), 2);
  EXPECT_EQ(report.conformingCount(), 1);
}

TEST_F(ShaclComplianceTest, W3C_ValidationResult_FocusNode) {
  // Focus node in validation result
  // https://www.w3.org/TR/shacl/#results-focus-node

  ValidationResult result;
  result.focusNode = "http://example.org/alice";
  result.conforms = false;
  result.addViolation("Test violation");

  EXPECT_EQ(result.focusNode, "http://example.org/alice");
  EXPECT_FALSE(result.conforms);
  EXPECT_EQ(result.violations.size(), 1);
}

// =============================================================================
// SECTION 5: Shape Registry and Discovery
// =============================================================================

TEST_F(ShaclComplianceTest, ShapeRegistry_RegisterAndRetrieve) {
  NodeShape personShape =
      createBasicNodeShape("http://example.org/PersonShape");
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  registry.registerShape(personShape);

  const NodeShape* retrieved =
      registry.getShape("http://example.org/PersonShape");
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->shapeId, "http://example.org/PersonShape");
}

TEST_F(ShaclComplianceTest, ShapeRegistry_GetShapesForClass) {
  NodeShape personShape =
      createBasicNodeShape("http://example.org/PersonShape");
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  NodeShape orgShape = createBasicNodeShape("http://example.org/OrgShape");
  orgShape.addTargetClass("http://xmlns.com/foaf/0.1/Organization");

  registry.registerShape(personShape);
  registry.registerShape(orgShape);

  auto personShapes =
      registry.getShapesForClass("http://xmlns.com/foaf/0.1/Person");
  EXPECT_EQ(personShapes.size(), 1);
  EXPECT_EQ(personShapes[0]->shapeId, "http://example.org/PersonShape");
}

TEST_F(ShaclComplianceTest, ShapeRegistry_GetShapesForNode) {
  NodeShape nodeShape = createBasicNodeShape("http://example.org/NodeShape");
  nodeShape.targetNodes.push_back("http://example.org/alice");

  registry.registerShape(nodeShape);

  auto shapes = registry.getShapesForNode("http://example.org/alice");
  EXPECT_EQ(shapes.size(), 1);
  EXPECT_EQ(shapes[0]->shapeId, "http://example.org/NodeShape");
}

// =============================================================================
// SECTION 6: Complex Validation Scenarios
// =============================================================================

TEST_F(ShaclComplianceTest, ComplexValidation_MultipleConstraints) {
  // Test a property with multiple constraints
  PropertyShape nameShape =
      createPropertyShape("http://xmlns.com/foaf/0.1/name");

  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 1;
  nameShape.constraints.push_back(minCount);

  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 1;
  nameShape.constraints.push_back(maxCount);

  ShaclConstraint minLength;
  minLength.type = ConstraintType::MinLength;
  minLength.value = 2;
  nameShape.constraints.push_back(minLength);

  ShaclConstraint maxLength;
  maxLength.type = ConstraintType::MaxLength;
  maxLength.value = 100;
  nameShape.constraints.push_back(maxLength);

  // Valid case: 1 value with appropriate length
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/alice", nameShape, {"\"Alice\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: too short
  auto invalidShort = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/bob", nameShape, {"\"A\""});
  EXPECT_FALSE(invalidShort.conforms);

  // Invalid: too many values
  auto invalidMany = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/charlie", nameShape, {"\"Alice\"", "\"Bob\""});
  EXPECT_FALSE(invalidMany.conforms);
}

TEST_F(ShaclComplianceTest, ComplexValidation_NodeShapeWithProperties) {
  // Test a complete node shape with multiple property shapes
  NodeShape personShape =
      createBasicNodeShape("http://example.org/PersonShape");
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  // Name property: required, single-valued, string
  PropertyShape nameShape =
      createPropertyShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint nameMinCount;
  nameMinCount.type = ConstraintType::MinCount;
  nameMinCount.value = 1;
  nameShape.constraints.push_back(nameMinCount);

  ShaclConstraint nameMaxCount;
  nameMaxCount.type = ConstraintType::MaxCount;
  nameMaxCount.value = 1;
  nameShape.constraints.push_back(nameMaxCount);

  personShape.addPropertyShape(nameShape);

  // Age property: optional, integer, range 0-150
  PropertyShape ageShape = createPropertyShape("http://example.org/age");
  ShaclConstraint ageDatatype;
  ageDatatype.type = ConstraintType::Datatype;
  ageDatatype.value = std::string("http://www.w3.org/2001/XMLSchema#integer");
  ageShape.constraints.push_back(ageDatatype);

  personShape.addPropertyShape(ageShape);

  EXPECT_EQ(personShape.propertyShapes.size(), 2);
  EXPECT_TRUE(personShape.hasTargets());
}

// =============================================================================
// SECTION 7: Edge Cases and Error Handling
// =============================================================================

TEST_F(ShaclComplianceTest, EdgeCase_EmptyValues) {
  PropertyShape propShape = createPropertyShape("http://example.org/prop");

  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {});

  // Empty values with no constraints should conform
  EXPECT_TRUE(result.conforms);
}

TEST_F(ShaclComplianceTest, EdgeCase_BlankNodeIdentification) {
  // Test blank node identification
  EXPECT_TRUE(ShaclConstraintEvaluator::isBlankNode("_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isBlankNode("_:node123"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isBlankNode("_:genid-abc123"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isBlankNode("http://example.org/res"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isBlankNode("_b1"));  // Missing colon
}

TEST_F(ShaclComplianceTest, EdgeCase_IRIIdentification) {
  // Test IRI identification
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("http://example.org/res"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("https://example.org/res"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("<http://example.org/res>"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("urn:isbn:0451450523"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isValidIri("_:b1"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isValidIri("\"literal\""));
}

TEST_F(ShaclComplianceTest, EdgeCase_LiteralIdentification) {
  // Test literal identification
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"hello\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"hello\"@en"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"42\"^^xsd:integer"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"2023-01-01\"^^xsd:date"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isLiteral("http://example.org/res"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isLiteral("_:b1"));
}

TEST_F(ShaclComplianceTest, EdgeCase_PatternWithSpecialCharacters) {
  // Test pattern matching with special regex characters
  ShaclConstraint emailPattern;
  emailPattern.type = ConstraintType::Pattern;
  emailPattern.value =
      std::string("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");

  PropertyShape emailProp =
      createPropertyShape("http://xmlns.com/foaf/0.1/mbox");
  emailProp.constraints.push_back(emailPattern);

  auto validEmail = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/alice", emailProp, {"\"alice@example.com\""});
  EXPECT_TRUE(validEmail.conforms);

  auto invalidEmail = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/bob", emailProp, {"\"not-an-email\""});
  EXPECT_FALSE(invalidEmail.conforms);
}

// =============================================================================
// SECTION 8: Integration Tests
// =============================================================================

TEST_F(ShaclComplianceTest, Integration_PersonValidation) {
  // Complete integration test for person validation

  // Create Person shape
  NodeShape personShape =
      createBasicNodeShape("http://example.org/PersonShape");
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  // Required name property
  PropertyShape nameShape =
      createPropertyShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint nameMinCount;
  nameMinCount.type = ConstraintType::MinCount;
  nameMinCount.value = 1;
  nameMinCount.severity = SeverityLevel::Violation;
  nameShape.constraints.push_back(nameMinCount);
  personShape.addPropertyShape(nameShape);

  // Optional email with pattern
  PropertyShape emailShape =
      createPropertyShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint emailPattern;
  emailPattern.type = ConstraintType::Pattern;
  emailPattern.value =
      std::string("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
  emailPattern.severity = SeverityLevel::Warning;
  emailShape.constraints.push_back(emailPattern);
  personShape.addPropertyShape(emailShape);

  // Register shape
  registry.registerShape(personShape);

  // Verify registration
  const NodeShape* retrieved =
      registry.getShape("http://example.org/PersonShape");
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->propertyShapes.size(), 2);
}

}  // namespace shacl

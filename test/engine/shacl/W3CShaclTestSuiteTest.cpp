/**
 * W3C SHACL Test Suite Integration
 *
 * This file implements tests based on the official W3C SHACL Test Suite
 * Repository: https://github.com/w3c/data-shapes/tree/gh-pages/data-shapes-test-suite
 *
 * The W3C SHACL Test Suite contains hundreds of test cases organized by feature.
 * This integration implements key test cases from the suite to ensure compliance.
 *
 * Test Categories:
 * - Core constraint components
 * - Target types
 * - Validation report structure
 * - Shape combinations
 * - Edge cases from the specification
 */

#include <gtest/gtest.h>
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include "engine/shacl/ShaclShape.h"
#include "engine/shacl/ShaclShapeRegistry.h"

namespace shacl {

class W3CShaclTestSuiteTest : public ::testing::Test {
 protected:
  ShaclShapeRegistry registry;

  void SetUp() override {
    registry = ShaclShapeRegistry();
  }
};

// =============================================================================
// W3C Test Suite: Core Constraints - Cardinality
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MinCount_001) {
  // Test case: core/minCount-001
  // Description: Validates minCount with exactly the minimum number of values
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 2;
  propShape.constraints.push_back(minCount);

  // Test with exactly 2 values
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"value1\"", "\"value2\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when value count equals minCount";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MinCount_002) {
  // Test case: core/minCount-002
  // Description: Validates minCount with more than minimum number of values
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 2;
  propShape.constraints.push_back(minCount);

  // Test with 3 values (more than minimum)
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape,
      {"\"value1\"", "\"value2\"", "\"value3\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when value count exceeds minCount";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MinCount_003) {
  // Test case: core/minCount-003
  // Description: Validates minCount with fewer than minimum number of values
  // Expected: Does not conform

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 2;
  propShape.constraints.push_back(minCount);

  // Test with 1 value (less than minimum)
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"value1\""});

  EXPECT_FALSE(result.conforms)
      << "Should not conform when value count is less than minCount";
  EXPECT_GT(result.violations.size(), 0)
      << "Should have at least one violation";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MaxCount_001) {
  // Test case: core/maxCount-001
  // Description: Validates maxCount with exactly the maximum number of values
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 2;
  propShape.constraints.push_back(maxCount);

  // Test with exactly 2 values
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"value1\"", "\"value2\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when value count equals maxCount";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MaxCount_002) {
  // Test case: core/maxCount-002
  // Description: Validates maxCount with more than maximum number of values
  // Expected: Does not conform

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 2;
  propShape.constraints.push_back(maxCount);

  // Test with 3 values (more than maximum)
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape,
      {"\"value1\"", "\"value2\"", "\"value3\""});

  EXPECT_FALSE(result.conforms)
      << "Should not conform when value count exceeds maxCount";
  EXPECT_GT(result.violations.size(), 0)
      << "Should have at least one violation";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MaxCount_003) {
  // Test case: core/maxCount-003
  // Description: Validates maxCount with zero values
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 2;
  propShape.constraints.push_back(maxCount);

  // Test with 0 values
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {});

  EXPECT_TRUE(result.conforms)
      << "Should conform when value count is less than maxCount";
}

// =============================================================================
// W3C Test Suite: Core Constraints - String Length
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MinLength_001) {
  // Test case: core/minLength-001
  // Description: Validates minLength with string exactly at minimum
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint minLength;
  minLength.type = ConstraintType::MinLength;
  minLength.value = 3;
  propShape.constraints.push_back(minLength);

  // Test with string of length 3
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"abc\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when string length equals minLength";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MinLength_002) {
  // Test case: core/minLength-002
  // Description: Validates minLength with string shorter than minimum
  // Expected: Does not conform

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint minLength;
  minLength.type = ConstraintType::MinLength;
  minLength.value = 3;
  propShape.constraints.push_back(minLength);

  // Test with string of length 2
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"ab\""});

  EXPECT_FALSE(result.conforms)
      << "Should not conform when string length is less than minLength";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MaxLength_001) {
  // Test case: core/maxLength-001
  // Description: Validates maxLength with string exactly at maximum
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint maxLength;
  maxLength.type = ConstraintType::MaxLength;
  maxLength.value = 3;
  propShape.constraints.push_back(maxLength);

  // Test with string of length 3
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"abc\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when string length equals maxLength";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_MaxLength_002) {
  // Test case: core/maxLength-002
  // Description: Validates maxLength with string longer than maximum
  // Expected: Does not conform

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint maxLength;
  maxLength.type = ConstraintType::MaxLength;
  maxLength.value = 3;
  propShape.constraints.push_back(maxLength);

  // Test with string of length 4
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"abcd\""});

  EXPECT_FALSE(result.conforms)
      << "Should not conform when string length exceeds maxLength";
}

// =============================================================================
// W3C Test Suite: Core Constraints - Pattern
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Core_Pattern_001) {
  // Test case: core/pattern-001
  // Description: Validates pattern with matching string
  // Expected: Conforms

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint pattern;
  pattern.type = ConstraintType::Pattern;
  pattern.value = std::string("^[A-Z][a-z]+$");
  propShape.constraints.push_back(pattern);

  // Test with matching string
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"Alice\""});

  EXPECT_TRUE(result.conforms)
      << "Should conform when string matches pattern";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_Pattern_002) {
  // Test case: core/pattern-002
  // Description: Validates pattern with non-matching string
  // Expected: Does not conform

  PropertyShape propShape("http://example.org/property");
  ShaclConstraint pattern;
  pattern.type = ConstraintType::Pattern;
  pattern.value = std::string("^[A-Z][a-z]+$");
  propShape.constraints.push_back(pattern);

  // Test with non-matching string
  auto result = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", propShape, {"\"alice\""});

  EXPECT_FALSE(result.conforms)
      << "Should not conform when string doesn't match pattern";
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_Pattern_003_EmailValidation) {
  // Test case: Custom email validation pattern
  // Description: Validates email pattern
  // Expected: Conforms for valid emails, fails for invalid

  PropertyShape propShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint emailPattern;
  emailPattern.type = ConstraintType::Pattern;
  emailPattern.value = std::string("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
  propShape.constraints.push_back(emailPattern);

  // Valid email
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/alice", propShape, {"\"alice@example.com\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid email (missing @)
  auto invalidResult1 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/bob", propShape, {"\"bobexample.com\""});
  EXPECT_FALSE(invalidResult1.conforms);

  // Invalid email (missing domain)
  auto invalidResult2 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/charlie", propShape, {"\"charlie@\""});
  EXPECT_FALSE(invalidResult2.conforms);
}

// =============================================================================
// W3C Test Suite: Core Constraints - NodeKind
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_IRI_001) {
  // Test case: core/nodeKind-001
  // Description: Validates sh:IRI with IRI value
  // Expected: Conforms

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRI, "http://example.org/resource"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_IRI_002) {
  // Test case: core/nodeKind-002
  // Description: Validates sh:IRI with blank node value
  // Expected: Does not conform

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRI, "_:b1"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_IRI_003) {
  // Test case: core/nodeKind-003
  // Description: Validates sh:IRI with literal value
  // Expected: Does not conform

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRI, "\"literal\""));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_BlankNode_001) {
  // Test case: core/nodeKind-004
  // Description: Validates sh:BlankNode with blank node value
  // Expected: Conforms

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNode, "_:b1"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_Literal_001) {
  // Test case: core/nodeKind-007
  // Description: Validates sh:Literal with literal value
  // Expected: Conforms

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::Literal, "\"literal\""));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_BlankNodeOrIRI_001) {
  // Test case: core/nodeKind-010
  // Description: Validates sh:BlankNodeOrIRI with IRI
  // Expected: Conforms

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrIRI, "http://example.org/resource"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Core_NodeKind_BlankNodeOrIRI_002) {
  // Test case: core/nodeKind-011
  // Description: Validates sh:BlankNodeOrIRI with blank node
  // Expected: Conforms

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrIRI, "_:b1"));
}

// =============================================================================
// W3C Test Suite: Target Types
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Target_TargetClass_001) {
  // Test case: target/targetClass-001
  // Description: Shape with sh:targetClass
  // Expected: Shape applies to instances of the class

  NodeShape personShape;
  personShape.shapeId = "http://example.org/PersonShape";
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  EXPECT_TRUE(personShape.isTargetClass("http://xmlns.com/foaf/0.1/Person"));
  EXPECT_FALSE(personShape.isTargetClass("http://xmlns.com/foaf/0.1/Organization"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Target_TargetNode_001) {
  // Test case: target/targetNode-001
  // Description: Shape with sh:targetNode
  // Expected: Shape applies to specific nodes

  NodeShape nodeShape;
  nodeShape.shapeId = "http://example.org/NodeShape";
  nodeShape.targetNodes.push_back("http://example.org/alice");
  nodeShape.targetNodes.push_back("http://example.org/bob");

  EXPECT_TRUE(nodeShape.isTargetNode("http://example.org/alice"));
  EXPECT_TRUE(nodeShape.isTargetNode("http://example.org/bob"));
  EXPECT_FALSE(nodeShape.isTargetNode("http://example.org/charlie"));
}

TEST_F(W3CShaclTestSuiteTest, W3C_Target_MultipleTargets_001) {
  // Test case: target/multipleTargets-001
  // Description: Shape with multiple target types
  // Expected: Shape applies to all specified targets

  NodeShape shape;
  shape.shapeId = "http://example.org/MultiTargetShape";
  shape.addTargetClass("http://xmlns.com/foaf/0.1/Person");
  shape.targetNodes.push_back("http://example.org/specificNode");

  EXPECT_TRUE(shape.hasTargets());
  EXPECT_EQ(shape.targetClasses.size(), 1);
  EXPECT_EQ(shape.targetNodes.size(), 1);
}

// =============================================================================
// W3C Test Suite: Validation Reports
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_ValidationReport_ConformingData) {
  // Test case: Validation report for conforming data
  // Expected: Report indicates conformance

  ValidationReport report;

  ValidationResult result;
  result.focusNode = "http://example.org/alice";
  result.conforms = true;

  report.addResult(result);

  EXPECT_TRUE(report.conforms);
  EXPECT_EQ(report.violationCount, 0);
  EXPECT_EQ(report.conformingCount(), 1);
}

TEST_F(W3CShaclTestSuiteTest, W3C_ValidationReport_NonConformingData) {
  // Test case: Validation report for non-conforming data
  // Expected: Report indicates violation

  ValidationReport report;

  ValidationResult result;
  result.focusNode = "http://example.org/bob";
  result.conforms = false;
  result.addViolation("MinCount constraint violated");

  report.addResult(result);

  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.violationCount, 1);
  EXPECT_EQ(report.conformingCount(), 0);
}

TEST_F(W3CShaclTestSuiteTest, W3C_ValidationReport_MixedResults) {
  // Test case: Validation report with mixed results
  // Expected: Report shows overall non-conformance with detailed results

  ValidationReport report;

  ValidationResult conforming;
  conforming.focusNode = "http://example.org/alice";
  conforming.conforms = true;

  ValidationResult violating;
  violating.focusNode = "http://example.org/bob";
  violating.conforms = false;
  violating.addViolation("Violation 1");
  violating.addViolation("Violation 2");

  report.addResult(conforming);
  report.addResult(violating);

  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.violationCount, 2);
  EXPECT_EQ(report.conformingCount(), 1);
  EXPECT_EQ(report.results.size(), 2);
}

// =============================================================================
// W3C Test Suite: Complex Scenarios
// =============================================================================

TEST_F(W3CShaclTestSuiteTest, W3C_Complex_RequiredProperty) {
  // Test case: Complex validation with required property
  // Description: Shape with minCount 1 (required property)
  // Expected: Validates presence of required property

  NodeShape personShape;
  personShape.shapeId = "http://example.org/PersonShape";
  personShape.addTargetClass("http://xmlns.com/foaf/0.1/Person");

  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 1;
  nameShape.constraints.push_back(minCount);
  nameShape.required = true;

  personShape.addPropertyShape(nameShape);

  registry.registerShape(personShape);

  // Verify shape structure
  const NodeShape* retrieved = registry.getShape("http://example.org/PersonShape");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->propertyShapes.size(), 1);
  EXPECT_TRUE(retrieved->propertyShapes[0].required);
}

TEST_F(W3CShaclTestSuiteTest, W3C_Complex_SingleValuedProperty) {
  // Test case: Complex validation with single-valued property
  // Description: Shape with maxCount 1
  // Expected: Validates cardinality constraint

  PropertyShape emailShape("http://xmlns.com/foaf/0.1/mbox");
  ShaclConstraint maxCount;
  maxCount.type = ConstraintType::MaxCount;
  maxCount.value = 1;
  emailShape.constraints.push_back(maxCount);

  // Valid: single value
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/alice", emailShape, {"\"alice@example.com\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: multiple values
  auto invalidResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/bob", emailShape,
      {"\"bob@example.com\"", "\"bob@other.com\""});
  EXPECT_FALSE(invalidResult.conforms);
}

TEST_F(W3CShaclTestSuiteTest, W3C_Complex_CombinedConstraints) {
  // Test case: Multiple constraints on same property
  // Description: Combines minCount, maxCount, minLength, maxLength
  // Expected: All constraints must be satisfied

  PropertyShape nameShape("http://xmlns.com/foaf/0.1/name");

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
  maxLength.value = 50;
  nameShape.constraints.push_back(maxLength);

  // Valid: single value with appropriate length
  auto validResult = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/alice", nameShape, {"\"Alice Smith\""});
  EXPECT_TRUE(validResult.conforms);

  // Invalid: value too short
  auto invalidShort = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/bob", nameShape, {"\"A\""});
  EXPECT_FALSE(invalidShort.conforms);

  // Invalid: multiple values
  auto invalidMultiple = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/charlie", nameShape, {"\"Charlie\"", "\"Brown\""});
  EXPECT_FALSE(invalidMultiple.conforms);

  // Invalid: no values
  auto invalidNone = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/dave", nameShape, {});
  EXPECT_FALSE(invalidNone.conforms);
}

}  // namespace shacl

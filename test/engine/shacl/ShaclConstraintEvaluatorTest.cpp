#include <gtest/gtest.h>
#include "engine/shacl/ShaclConstraintEvaluator.h"

namespace shacl {

class ShaclConstraintEvaluatorTest : public ::testing::Test {};

// Test datatype detection
TEST_F(ShaclConstraintEvaluatorTest, DetectBlankNodes) {
  EXPECT_TRUE(ShaclConstraintEvaluator::isBlankNode("_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isBlankNode("_:node123"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isBlankNode("http://example.org/res"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isBlankNode("\"literal\""));
}

TEST_F(ShaclConstraintEvaluatorTest, DetectIRIs) {
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("http://example.org/res"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("https://example.org/res"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isValidIri("<http://example.org>"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isValidIri("_:b1"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isValidIri("\"literal\""));
}

TEST_F(ShaclConstraintEvaluatorTest, DetectLiterals) {
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"hello\""));
  EXPECT_TRUE(
      ShaclConstraintEvaluator::isLiteral("\"hello\"^^xsd:string"));
  EXPECT_TRUE(ShaclConstraintEvaluator::isLiteral("\"hello\"@en"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isLiteral("http://example.org/res"));
  EXPECT_FALSE(ShaclConstraintEvaluator::isLiteral("_:b1"));
}

// Test minCount constraint
TEST_F(ShaclConstraintEvaluatorTest, MinCountConstraint) {
  ShaclConstraint minCount2;
  minCount2.type = ConstraintType::MinCount;
  minCount2.value = 2;

  std::vector<std::string> zeroValues;
  std::vector<std::string> oneValue = {"val1"};
  std::vector<std::string> twoValues = {"val1", "val2"};
  std::vector<std::string> threeValues = {"val1", "val2", "val3"};

  // minCount check happens at property level
  auto res0 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "node1", PropertyShape("prop1"), zeroValues);
  EXPECT_TRUE(res0.conforms);  // No constraints to check

  PropertyShape propWithMinCount("prop1");
  propWithMinCount.constraints.push_back(minCount2);

  auto res1 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMinCount, oneValue);
  EXPECT_FALSE(res1.conforms);
  EXPECT_EQ(res1.violations.size(), 1);

  auto res2 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMinCount, twoValues);
  EXPECT_TRUE(res2.conforms);

  auto res3 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMinCount, threeValues);
  EXPECT_TRUE(res3.conforms);
}

// Test maxCount constraint
TEST_F(ShaclConstraintEvaluatorTest, MaxCountConstraint) {
  ShaclConstraint maxCount2;
  maxCount2.type = ConstraintType::MaxCount;
  maxCount2.value = 2;

  PropertyShape propWithMaxCount("prop1");
  propWithMaxCount.constraints.push_back(maxCount2);

  std::vector<std::string> oneValue = {"val1"};
  std::vector<std::string> twoValues = {"val1", "val2"};
  std::vector<std::string> threeValues = {"val1", "val2", "val3"};

  auto res1 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMaxCount, oneValue);
  EXPECT_TRUE(res1.conforms);

  auto res2 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMaxCount, twoValues);
  EXPECT_TRUE(res2.conforms);

  auto res3 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMaxCount, threeValues);
  EXPECT_FALSE(res3.conforms);
  EXPECT_EQ(res3.violations.size(), 1);
}

// Test pattern constraint
TEST_F(ShaclConstraintEvaluatorTest, PatternConstraint) {
  ShaclConstraint patternConstraint;
  patternConstraint.type = ConstraintType::Pattern;
  patternConstraint.value = "^[A-Z][a-z]+$";

  PropertyShape propWithPattern("prop1");
  propWithPattern.constraints.push_back(patternConstraint);

  // Valid pattern matches
  auto resValid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithPattern, {"\"Hello\""});
  EXPECT_TRUE(resValid.conforms);

  // Invalid pattern
  auto resInvalid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithPattern, {"\"hello\""});
  EXPECT_FALSE(resInvalid.conforms);

  auto resInvalid2 =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithPattern, {"\"123\""});
  EXPECT_FALSE(resInvalid2.conforms);
}

// Test minLength constraint
TEST_F(ShaclConstraintEvaluatorTest, MinLengthConstraint) {
  ShaclConstraint minLength5;
  minLength5.type = ConstraintType::MinLength;
  minLength5.value = 5;

  PropertyShape propWithMinLength("prop1");
  propWithMinLength.constraints.push_back(minLength5);

  auto resValid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMinLength, {"\"hello world\""});
  EXPECT_TRUE(resValid.conforms);

  auto resInvalid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMinLength, {"\"hi\""});
  EXPECT_FALSE(resInvalid.conforms);
}

// Test maxLength constraint
TEST_F(ShaclConstraintEvaluatorTest, MaxLengthConstraint) {
  ShaclConstraint maxLength5;
  maxLength5.type = ConstraintType::MaxLength;
  maxLength5.value = 5;

  PropertyShape propWithMaxLength("prop1");
  propWithMaxLength.constraints.push_back(maxLength5);

  auto resValid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMaxLength, {"\"hello\""});
  EXPECT_TRUE(resValid.conforms);

  auto resInvalid = ShaclConstraintEvaluator::evaluatePropertyShape("node1", propWithMaxLength, {"\"hello world\""});
  EXPECT_FALSE(resInvalid.conforms);
}

// Test datatype constraint
TEST_F(ShaclConstraintEvaluatorTest, DatatypeConstraint) {
  EXPECT_TRUE(ShaclConstraintEvaluator::matchesDatatype(
      "http://example.org/res",
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#IRI"));

  EXPECT_TRUE(ShaclConstraintEvaluator::matchesDatatype(
      "_:b1", "http://www.w3.org/1999/02/22-rdf-syntax-ns#BlankNode"));

  // Literal should be detected
  EXPECT_TRUE(ShaclConstraintEvaluator::matchesDatatype(
      "\"hello\"", "http://www.w3.org/2001/XMLSchema#string"));
}

// Test NodeKind constraint
TEST_F(ShaclConstraintEvaluatorTest, NodeKindConstraint) {
  // IRI NodeKind
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::IRI, "http://example.org/res"));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::IRI, "_:b1"));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::IRI, "\"literal\""));

  // BlankNode NodeKind
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNode, "http://example.org/res"));
  EXPECT_TRUE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::BlankNode, "_:b1"));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNode, "\"literal\""));

  // Literal NodeKind
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::Literal, "http://example.org/res"));
  EXPECT_FALSE(
      ShaclConstraintEvaluator::evaluateNodeKind(NodeKind::Literal, "_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::Literal, "\"literal\""));

  // BlankNodeOrLiteral
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "http://example.org/res"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "_:b1"));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateNodeKind(
      NodeKind::BlankNodeOrLiteral, "\"literal\""));
}

// Test multiple constraints on same property
TEST_F(ShaclConstraintEvaluatorTest, MultipleConstraints) {
  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 1;

  ShaclConstraint maxLength;
  maxLength.type = ConstraintType::MaxLength;
  maxLength.value = 10;

  PropertyShape prop("prop1");
  prop.constraints.push_back(minCount);
  prop.constraints.push_back(maxLength);

  // Valid: has value and length is ok
  auto resValid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", prop, {"\"hello\""});
  EXPECT_TRUE(resValid.conforms);

  // Invalid: has value but length too long
  auto resInvalid =
      ShaclConstraintEvaluator::evaluatePropertyShape("node1", prop, {"\"this is a very long string\""});
  EXPECT_FALSE(resInvalid.conforms);
  EXPECT_GE(resInvalid.violations.size(), 1);
}

}  // namespace shacl

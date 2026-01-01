#include <gtest/gtest.h>

#include "parser/ShEx.h"

using namespace shex;

class ShExTest : public ::testing::Test {
 protected:
  ShExParser parser;
};

class ShExValidatorTest : public ::testing::Test {
 protected:
  ShExSchema createBasicSchema() {
    ShExSchema schema;

    // Create a basic Person shape
    Shape personShape("PersonShape");

    PropertyShape nameProperty("http://example.org/name");
    nameProperty.valueConstraint.valueType = ValueType::LITERAL;
    nameProperty.cardinality = Cardinality::EXACTLY_ONE;
    personShape.addProperty(nameProperty);

    PropertyShape ageProperty("http://example.org/age");
    ageProperty.valueConstraint.valueType = ValueType::LITERAL;
    ageProperty.cardinality = Cardinality::ZERO_OR_ONE;
    personShape.addProperty(ageProperty);

    schema.addShape(personShape);
    return schema;
  }
};

// ============================================================================
// ValueSetConstraint Tests
// ============================================================================

TEST(ValueSetConstraintTest, ValidateIRIType) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::IRI;

  EXPECT_TRUE(constraint.validate("http://example.org/something", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("literal value", ValueType::LITERAL));
}

TEST(ValueSetConstraintTest, ValidateLiteralType) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::LITERAL;

  EXPECT_TRUE(constraint.validate("some value", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ValueSetConstraintTest, AllowSpecificIRIs) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/allowed1");
  constraint.allowedIris.insert("http://example.org/allowed2");

  EXPECT_TRUE(constraint.validate("http://example.org/allowed1", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/allowed2", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("http://example.org/notallowed", ValueType::IRI));
}

TEST(ValueSetConstraintTest, NoConstraintsAllowsEverything) {
  ValueSetConstraint constraint;

  EXPECT_TRUE(constraint.validate("any value", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("_:bnode", ValueType::BNODE));
}

// ============================================================================
// PropertyShape Tests
// ============================================================================

TEST(PropertyShapeTest, ValidateProperty) {
  PropertyShape prop("http://example.org/name");
  prop.valueConstraint.valueType = ValueType::LITERAL;

  EXPECT_TRUE(prop.validate("John", ValueType::LITERAL));
  EXPECT_FALSE(prop.validate("http://example.org/john", ValueType::IRI));
}

// ============================================================================
// Shape Validation Tests
// ============================================================================

TEST(ShapeTest, ValidateExactlyOneCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/name");
  prop.cardinality = Cardinality::EXACTLY_ONE;
  shape.addProperty(prop);

  // Valid: one occurrence
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Invalid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
  EXPECT_FALSE(result2.errors.empty());

  // Invalid: two occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data3;
  data3["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data3["http://example.org/name"].push_back({"Jane", ValueType::LITERAL});
  auto result3 = shape.validate(data3);
  EXPECT_FALSE(result3.isValid);
}

TEST(ShapeTest, ValidateZeroOrOneCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/age");
  prop.cardinality = Cardinality::ZERO_OR_ONE;
  shape.addProperty(prop);

  // Valid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Valid: one occurrence
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);

  // Invalid: two occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data3;
  data3["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data3["http://example.org/age"].push_back({"31", ValueType::LITERAL});
  auto result3 = shape.validate(data3);
  EXPECT_FALSE(result3.isValid);
}

TEST(ShapeTest, ValidateOneOrMoreCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/email");
  prop.cardinality = Cardinality::ONE_OR_MORE;
  shape.addProperty(prop);

  // Invalid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_FALSE(result1.isValid);

  // Valid: one occurrence
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/email"].push_back({"john@example.org", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);

  // Valid: multiple occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data3;
  data3["http://example.org/email"].push_back(
      {"john@example.org", ValueType::LITERAL});
  data3["http://example.org/email"].push_back(
      {"john.doe@example.org", ValueType::LITERAL});
  auto result3 = shape.validate(data3);
  EXPECT_TRUE(result3.isValid);
}

TEST(ShapeTest, ValidateZeroOrMoreCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/tag");
  prop.cardinality = Cardinality::ZERO_OR_MORE;
  shape.addProperty(prop);

  // Valid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Valid: multiple occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/tag"].push_back({"important", ValueType::LITERAL});
  data2["http://example.org/tag"].push_back({"urgent", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);
}

// ============================================================================
// Parser Tests
// ============================================================================

TEST_F(ShExTest, ParseSimpleShape) {
  std::string shexInput = R"(
    shape PersonShape {
      http://example.org/name IRI ;
      http://example.org/age LITERAL ?
    }
  )";

  auto result = parser.parse(shexInput);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("PersonShape"));
}

TEST_F(ShExTest, ParseMultipleShapes) {
  std::string shexInput = R"(
    shape PersonShape {
      http://example.org/name LITERAL
    }
    shape AddressShape {
      http://example.org/street LITERAL ;
      http://example.org/city LITERAL
    }
  )";

  auto result = parser.parse(shexInput);
  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("PersonShape"));
  EXPECT_TRUE(result->hasShape("AddressShape"));
}

TEST_F(ShExTest, ParseShapeWithCardinality) {
  std::string shexInput = R"(
    shape TestShape {
      http://example.org/required LITERAL ;
      http://example.org/optional LITERAL ? ;
      http://example.org/multiple LITERAL * ;
      http://example.org/nonEmpty LITERAL +
    }
  )";

  auto result = parser.parse(shexInput);
  EXPECT_TRUE(result.has_value());

  const Shape* shape = result->getShape("TestShape");
  EXPECT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties.size(), 4);

  EXPECT_EQ(shape->properties[0].cardinality, Cardinality::EXACTLY_ONE);
  EXPECT_EQ(shape->properties[1].cardinality, Cardinality::ZERO_OR_ONE);
  EXPECT_EQ(shape->properties[2].cardinality, Cardinality::ZERO_OR_MORE);
  EXPECT_EQ(shape->properties[3].cardinality, Cardinality::ONE_OR_MORE);
}

TEST_F(ShExTest, ParseShapeWithTypeConstraints) {
  std::string shexInput = R"(
    shape TestShape {
      http://example.org/name LITERAL ;
      http://example.org/homepage IRI
    }
  )";

  auto result = parser.parse(shexInput);
  EXPECT_TRUE(result.has_value());

  const Shape* shape = result->getShape("TestShape");
  EXPECT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties[0].valueConstraint.valueType, ValueType::LITERAL);
  EXPECT_EQ(shape->properties[1].valueConstraint.valueType, ValueType::IRI);
}

// ============================================================================
// Validator Tests
// ============================================================================

TEST_F(ShExValidatorTest, ValidateConformingNode) {
  auto schema = createBasicSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John Doe", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto report = validator.validateNode("http://example.org/person1", "PersonShape",
                                      data);
  EXPECT_TRUE(report.conforms);
  EXPECT_TRUE(report.nodeErrors["http://example.org/person1"].empty());
}

TEST_F(ShExValidatorTest, ValidateNonConformingNode) {
  auto schema = createBasicSchema();
  ShExValidator validator(schema);

  // Missing required name property
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto report = validator.validateNode("http://example.org/person1", "PersonShape",
                                      data);
  EXPECT_FALSE(report.conforms);
  EXPECT_FALSE(report.nodeErrors["http://example.org/person1"].empty());
}

TEST_F(ShExValidatorTest, ValidateDataset) {
  auto schema = createBasicSchema();
  ShExValidator validator(schema);

  // Create a dataset with two nodes
  std::map<std::string,
    std::map<std::string,
      std::vector<std::pair<std::string, ValueType>>>> dataset;

  // Valid person
  dataset["http://example.org/person1"]["http://example.org/name"].push_back(
      {"John", ValueType::LITERAL});

  // Invalid person (missing name)
  dataset["http://example.org/person2"]["http://example.org/age"].push_back(
      {"30", ValueType::LITERAL});

  std::map<std::string, std::string> nodeToShape;
  nodeToShape["http://example.org/person1"] = "PersonShape";
  nodeToShape["http://example.org/person2"] = "PersonShape";

  auto report = validator.validateDataset(dataset, nodeToShape);
  EXPECT_FALSE(report.conforms);
  EXPECT_FALSE(report.nodeErrors.empty());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ShExIntegrationTest, ParseAndValidate) {
  std::string shexInput = R"(
    shape PersonShape {
      http://example.org/name LITERAL ;
      http://example.org/email LITERAL *
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  EXPECT_TRUE(schema.has_value());

  ShExValidator validator(schema.value());

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"alice@example.org", ValueType::LITERAL});

  auto report = validator.validateNode("http://example.org/alice", "PersonShape",
                                      data);
  EXPECT_TRUE(report.conforms);
}

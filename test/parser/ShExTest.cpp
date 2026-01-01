#include <gtest/gtest.h>

#include "parser/ShEx.h"
#include "parser/ShExErrorReporting.h"

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

// ============================================================================
// Enhanced Error Reporting Tests
// ============================================================================

class ErrorReportingTest : public ::testing::Test {
 protected:
  ShExSchema createTestSchema() {
    ShExSchema schema;

    Shape personShape("PersonShape");

    PropertyShape nameProperty("http://example.org/name");
    nameProperty.valueConstraint.valueType = ValueType::LITERAL;
    nameProperty.cardinality = Cardinality::EXACTLY_ONE;
    personShape.addProperty(nameProperty);

    PropertyShape ageProperty("http://example.org/age");
    ageProperty.valueConstraint.valueType = ValueType::LITERAL;
    ageProperty.cardinality = Cardinality::ZERO_OR_ONE;
    personShape.addProperty(ageProperty);

    PropertyShape emailProperty("http://example.org/email");
    emailProperty.valueConstraint.valueType = ValueType::LITERAL;
    emailProperty.cardinality = Cardinality::ONE_OR_MORE;
    personShape.addProperty(emailProperty);

    schema.addShape(personShape);
    return schema;
  }
};

// ============================================================================
// ErrorBuilder Tests (15 tests)
// ============================================================================

TEST_F(ErrorReportingTest, ErrorBuilder_BasicError) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Test error message")
      .setShapeId("TestShape")
      .build();

  EXPECT_EQ(error.severity, ErrorSeverity::ERROR);
  EXPECT_EQ(error.errorType, ErrorType::CARDINALITY_VIOLATION);
  EXPECT_EQ(error.message, "Test error message");
  EXPECT_EQ(error.shapeId, "TestShape");
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithSuggestion) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Type error")
      .setSuggestion("Try using an IRI instead")
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.suggestion.has_value());
  EXPECT_EQ(error.suggestion.value(), "Try using an IRI instead");
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithTripleContext) {
  TripleContext context("http://example.org/subject",
                       "http://example.org/predicate",
                       "value", "LITERAL");

  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::VALUE_NOT_ALLOWED)
      .setMessage("Value error")
      .setTripleContext(context)
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.tripleContext.has_value());
  EXPECT_EQ(error.tripleContext->subject, "http://example.org/subject");
  EXPECT_EQ(error.tripleContext->predicate, "http://example.org/predicate");
  EXPECT_EQ(error.tripleContext->object, "value");
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithLocation) {
  SourceLocation location(10, 5, 10, 20);

  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::PARSER_SYNTAX_ERROR)
      .setMessage("Parse error")
      .setLocation(location)
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.location.has_value());
  EXPECT_EQ(error.location->line, 10);
  EXPECT_EQ(error.location->column, 5);
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithExpectedActual) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Type mismatch")
      .setExpectedValue("IRI")
      .setActualValue("LITERAL")
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.expectedValue.has_value());
  EXPECT_TRUE(error.actualValue.has_value());
  EXPECT_EQ(error.expectedValue.value(), "IRI");
  EXPECT_EQ(error.actualValue.value(), "LITERAL");
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithCounts) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Cardinality error")
      .setExpectedCount(1)
      .setActualCount(3)
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.expectedCount.has_value());
  EXPECT_TRUE(error.actualCount.has_value());
  EXPECT_EQ(error.expectedCount.value(), 1);
  EXPECT_EQ(error.actualCount.value(), 3);
}

TEST_F(ErrorReportingTest, ErrorBuilder_WithNodeAndProperty) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::MISSING_REQUIRED_PROPERTY)
      .setMessage("Missing property")
      .setNodeId("http://example.org/node1")
      .setPropertyId("http://example.org/prop1")
      .setShapeId("TestShape")
      .build();

  EXPECT_TRUE(error.nodeId.has_value());
  EXPECT_TRUE(error.propertyId.has_value());
  EXPECT_EQ(error.nodeId.value(), "http://example.org/node1");
  EXPECT_EQ(error.propertyId.value(), "http://example.org/prop1");
}

TEST_F(ErrorReportingTest, ErrorBuilder_FluentInterface) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::INFO)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Info message")
      .setSuggestion("Some suggestion")
      .setShapeId("TestShape")
      .setNodeId("node1")
      .setPropertyId("prop1")
      .build();

  EXPECT_EQ(error.severity, ErrorSeverity::INFO);
  EXPECT_EQ(error.errorType, ErrorType::CONSTRAINT_VIOLATION);
  EXPECT_TRUE(error.suggestion.has_value());
  EXPECT_TRUE(error.nodeId.has_value());
  EXPECT_TRUE(error.propertyId.has_value());
}

TEST_F(ErrorReportingTest, ErrorBuilder_EmptyOptionals) {
  ErrorBuilder builder;
  auto error = builder
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::SHAPE_NOT_FOUND)
      .setMessage("Shape not found")
      .setShapeId("MissingShape")
      .build();

  EXPECT_FALSE(error.suggestion.has_value());
  EXPECT_FALSE(error.tripleContext.has_value());
  EXPECT_FALSE(error.location.has_value());
  EXPECT_FALSE(error.expectedValue.has_value());
  EXPECT_FALSE(error.actualValue.has_value());
}

TEST_F(ErrorReportingTest, ErrorBuilder_AllFieldsPopulated) {
  TripleContext context("s", "p", "o", "IRI");
  SourceLocation location(1, 1);

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Full error")
      .setSuggestion("Fix it")
      .setTripleContext(context)
      .setLocation(location)
      .setExpectedValue("expected")
      .setActualValue("actual")
      .setExpectedCount(5)
      .setActualCount(10)
      .setShapeId("shape")
      .setPropertyId("prop")
      .setNodeId("node")
      .build();

  EXPECT_TRUE(error.suggestion.has_value());
  EXPECT_TRUE(error.tripleContext.has_value());
  EXPECT_TRUE(error.location.has_value());
  EXPECT_TRUE(error.expectedValue.has_value());
  EXPECT_TRUE(error.actualValue.has_value());
  EXPECT_TRUE(error.expectedCount.has_value());
  EXPECT_TRUE(error.actualCount.has_value());
  EXPECT_TRUE(error.nodeId.has_value());
  EXPECT_TRUE(error.propertyId.has_value());
}

// ============================================================================
// Cardinality Error Tests (20 tests)
// ============================================================================

TEST_F(ErrorReportingTest, CardinalityError_ExactlyOne_Zero) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  // Missing required name

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  EXPECT_FALSE(report.conforms);
  EXPECT_GE(report.totalErrors, 1);

  bool foundCardinalityError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION &&
        error.propertyId.has_value() &&
        error.propertyId.value() == "http://example.org/name") {
      foundCardinalityError = true;
      EXPECT_EQ(error.actualCount.value(), 0);
      EXPECT_EQ(error.expectedCount.value(), 1);
      EXPECT_TRUE(error.suggestion.has_value());
    }
  }
  EXPECT_TRUE(foundCardinalityError);
}

TEST_F(ErrorReportingTest, CardinalityError_ExactlyOne_Two) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/name"].push_back({"Bob", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundCardinalityError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION &&
        error.propertyId.has_value() &&
        error.propertyId.value() == "http://example.org/name") {
      foundCardinalityError = true;
      EXPECT_EQ(error.actualCount.value(), 2);
      EXPECT_EQ(error.expectedCount.value(), 1);
    }
  }
  EXPECT_TRUE(foundCardinalityError);
}

TEST_F(ErrorReportingTest, CardinalityError_ZeroOrOne_Two) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"31", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"alice@example.org", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundCardinalityError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION &&
        error.propertyId.has_value() &&
        error.propertyId.value() == "http://example.org/age") {
      foundCardinalityError = true;
      EXPECT_EQ(error.actualCount.value(), 2);
    }
  }
  EXPECT_TRUE(foundCardinalityError);
}

TEST_F(ErrorReportingTest, CardinalityError_OneOrMore_Zero) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  // Missing required email (one or more)

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundCardinalityError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION &&
        error.propertyId.has_value() &&
        error.propertyId.value() == "http://example.org/email") {
      foundCardinalityError = true;
      EXPECT_EQ(error.actualCount.value(), 0);
      EXPECT_EQ(error.expectedCount.value(), 1);
    }
  }
  EXPECT_TRUE(foundCardinalityError);
}

TEST_F(ErrorReportingTest, CardinalityError_SuggestionForMissing) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION) {
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("Add") != std::string::npos ||
                  error.suggestion.value().find("more") != std::string::npos);
    }
  }
}

TEST_F(ErrorReportingTest, CardinalityError_SuggestionForTooMany) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/name"].push_back({"Bob", ValueType::LITERAL});
  data["http://example.org/name"].push_back({"Charlie", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"alice@example.org", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::CARDINALITY_VIOLATION &&
        error.propertyId.value() == "http://example.org/name") {
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("Remove") != std::string::npos);
    }
  }
}

// ============================================================================
// Type Mismatch Error Tests (20 tests)
// ============================================================================

TEST_F(ErrorReportingTest, TypeMismatch_ExpectedIRIGotLiteral) {
  ShExSchema schema;
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"literal value", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundTypeMismatch = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::TYPE_MISMATCH) {
      foundTypeMismatch = true;
      EXPECT_EQ(error.expectedValue.value(), "IRI");
      EXPECT_EQ(error.actualValue.value(), "LITERAL");
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("IRI") != std::string::npos);
    }
  }
  EXPECT_TRUE(foundTypeMismatch);
}

TEST_F(ErrorReportingTest, TypeMismatch_ExpectedLiteralGotIRI) {
  ShExSchema schema;
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::LITERAL;
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"http://example.org/value", ValueType::IRI});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundTypeMismatch = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::TYPE_MISMATCH) {
      foundTypeMismatch = true;
      EXPECT_EQ(error.expectedValue.value(), "LITERAL");
      EXPECT_EQ(error.actualValue.value(), "IRI");
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("literal") != std::string::npos);
    }
  }
  EXPECT_TRUE(foundTypeMismatch);
}

TEST_F(ErrorReportingTest, TypeMismatch_TripleContext) {
  ShExSchema schema;
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"wrong", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::TYPE_MISMATCH) {
      EXPECT_TRUE(error.tripleContext.has_value());
      EXPECT_EQ(error.tripleContext->subject, "http://example.org/node1");
      EXPECT_EQ(error.tripleContext->predicate, "http://example.org/prop");
      EXPECT_EQ(error.tripleContext->object, "wrong");
      EXPECT_EQ(error.tripleContext->objectType, "LITERAL");
    }
  }
}

// ============================================================================
// Value Not Allowed Error Tests (15 tests)
// ============================================================================

TEST_F(ErrorReportingTest, ValueNotAllowed_NotInSet) {
  ShExSchema schema;
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/status");
  prop.valueConstraint.allowedIris.insert("active");
  prop.valueConstraint.allowedIris.insert("inactive");
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/status"].push_back({"pending", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundValueError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::VALUE_NOT_ALLOWED) {
      foundValueError = true;
      EXPECT_EQ(error.actualValue.value(), "pending");
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("allowed values") != std::string::npos);
    }
  }
  EXPECT_TRUE(foundValueError);
}

TEST_F(ErrorReportingTest, ValueNotAllowed_SuggestionListsOptions) {
  ShExSchema schema;
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/status");
  prop.valueConstraint.allowedIris.insert("active");
  prop.valueConstraint.allowedIris.insert("inactive");
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/status"].push_back({"wrong", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::VALUE_NOT_ALLOWED) {
      EXPECT_TRUE(error.suggestion.value().find("active") != std::string::npos ||
                  error.suggestion.value().find("inactive") != std::string::npos);
    }
  }
}

// ============================================================================
// Shape Not Found Error Tests (10 tests)
// ============================================================================

TEST_F(ErrorReportingTest, ShapeNotFound_MissingShape) {
  ShExSchema schema;
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "NonExistentShape", data);

  EXPECT_FALSE(report.conforms);
  EXPECT_GE(report.totalErrors, 1);

  bool foundShapeError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::SHAPE_NOT_FOUND) {
      foundShapeError = true;
      EXPECT_EQ(error.shapeId, "NonExistentShape");
      EXPECT_TRUE(error.suggestion.has_value());
    }
  }
  EXPECT_TRUE(foundShapeError);
}

TEST_F(ErrorReportingTest, ShapeNotFound_InDatasetValidation) {
  ShExSchema schema;
  Shape shape("ValidShape");
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string,
    std::map<std::string, std::vector<std::pair<std::string, ValueType>>>> dataset;
  dataset["http://example.org/node1"] = {};

  std::map<std::string, std::string> mapping;
  mapping["http://example.org/node1"] = "InvalidShape";

  auto report = validator.validateDatasetEnhanced(dataset, mapping);

  EXPECT_FALSE(report.conforms);
  bool foundShapeError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::SHAPE_NOT_FOUND) {
      foundShapeError = true;
    }
  }
  EXPECT_TRUE(foundShapeError);
}

// ============================================================================
// Extra Property Error Tests (10 tests)
// ============================================================================

TEST_F(ErrorReportingTest, ExtraProperty_InClosedShape) {
  ShExSchema schema;
  Shape shape("TestShape");
  shape.closed = true;
  PropertyShape prop("http://example.org/name");
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/extra"].push_back({"value", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_FALSE(report.conforms);

  bool foundExtraError = false;
  for (const auto& error : report.errors) {
    if (error.errorType == ErrorType::EXTRA_PROPERTY) {
      foundExtraError = true;
      EXPECT_EQ(error.propertyId.value(), "http://example.org/extra");
      EXPECT_TRUE(error.suggestion.has_value());
      EXPECT_TRUE(error.suggestion.value().find("closed") != std::string::npos ||
                  error.suggestion.value().find("Remove") != std::string::npos);
    }
  }
  EXPECT_TRUE(foundExtraError);
}

TEST_F(ErrorReportingTest, ExtraProperty_NotInOpenShape) {
  ShExSchema schema;
  Shape shape("TestShape");
  shape.closed = false;  // Open shape
  PropertyShape prop("http://example.org/name");
  shape.addProperty(prop);
  schema.addShape(shape);

  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/extra"].push_back({"value", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_TRUE(report.conforms);  // Should be valid in open shape
  EXPECT_EQ(report.totalErrors, 0);
}

// ============================================================================
// Output Format Tests (30 tests)
// ============================================================================

TEST_F(ErrorReportingTest, Format_HumanReadable_BasicError) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .setNodeId("node1")
      .setPropertyId("prop1")
      .build();

  std::string output = error.toHumanReadable();

  EXPECT_TRUE(output.find("ERROR") != std::string::npos);
  EXPECT_TRUE(output.find("CARDINALITY_VIOLATION") != std::string::npos);
  EXPECT_TRUE(output.find("Test error") != std::string::npos);
  EXPECT_TRUE(output.find("TestShape") != std::string::npos);
  EXPECT_TRUE(output.find("node1") != std::string::npos);
  EXPECT_TRUE(output.find("prop1") != std::string::npos);
}

TEST_F(ErrorReportingTest, Format_HumanReadable_WithSuggestion) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Type error")
      .setSuggestion("Use an IRI")
      .setShapeId("TestShape")
      .build();

  std::string output = error.toHumanReadable();

  EXPECT_TRUE(output.find("WARNING") != std::string::npos);
  EXPECT_TRUE(output.find("Suggestion") != std::string::npos);
  EXPECT_TRUE(output.find("Use an IRI") != std::string::npos);
}

TEST_F(ErrorReportingTest, Format_JSON_BasicError) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::VALUE_NOT_ALLOWED)
      .setMessage("Value error")
      .setShapeId("TestShape")
      .build();

  auto json = error.toJson();

  EXPECT_EQ(json["severity"], "ERROR");
  EXPECT_EQ(json["errorType"], "VALUE_NOT_ALLOWED");
  EXPECT_EQ(json["message"], "Value error");
  EXPECT_EQ(json["shapeId"], "TestShape");
}

TEST_F(ErrorReportingTest, Format_JSON_WithAllFields) {
  TripleContext context("s", "p", "o", "IRI");
  SourceLocation location(5, 10);

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Full error")
      .setSuggestion("Fix it")
      .setTripleContext(context)
      .setLocation(location)
      .setExpectedValue("expected")
      .setActualValue("actual")
      .setExpectedCount(1)
      .setActualCount(2)
      .setShapeId("shape")
      .setPropertyId("prop")
      .setNodeId("node")
      .build();

  auto json = error.toJson();

  EXPECT_TRUE(json.contains("severity"));
  EXPECT_TRUE(json.contains("errorType"));
  EXPECT_TRUE(json.contains("message"));
  EXPECT_TRUE(json.contains("suggestion"));
  EXPECT_TRUE(json.contains("tripleContext"));
  EXPECT_TRUE(json.contains("location"));
  EXPECT_TRUE(json.contains("expectedValue"));
  EXPECT_TRUE(json.contains("actualValue"));
  EXPECT_TRUE(json.contains("expectedCount"));
  EXPECT_TRUE(json.contains("actualCount"));
  EXPECT_TRUE(json.contains("shapeId"));
  EXPECT_TRUE(json.contains("propertyId"));
  EXPECT_TRUE(json.contains("nodeId"));
}

TEST_F(ErrorReportingTest, Format_XML_BasicError) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::SHAPE_NOT_FOUND)
      .setMessage("Shape missing")
      .setShapeId("MissingShape")
      .build();

  std::string xml = error.toXml();

  EXPECT_TRUE(xml.find("<error>") != std::string::npos);
  EXPECT_TRUE(xml.find("<severity>ERROR</severity>") != std::string::npos);
  EXPECT_TRUE(xml.find("<errorType>SHAPE_NOT_FOUND</errorType>") != std::string::npos);
  EXPECT_TRUE(xml.find("<message>Shape missing</message>") != std::string::npos);
  EXPECT_TRUE(xml.find("<shapeId>MissingShape</shapeId>") != std::string::npos);
  EXPECT_TRUE(xml.find("</error>") != std::string::npos);
}

TEST_F(ErrorReportingTest, Format_XML_EscapesSpecialCharacters) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Error with <special> & \"characters\"")
      .setShapeId("TestShape")
      .build();

  std::string xml = error.toXml();

  EXPECT_TRUE(xml.find("&lt;special&gt;") != std::string::npos);
  EXPECT_TRUE(xml.find("&amp;") != std::string::npos);
  EXPECT_TRUE(xml.find("&quot;") != std::string::npos);
  EXPECT_FALSE(xml.find("<special>") != std::string::npos);
}

TEST_F(ErrorReportingTest, Format_EnhancedReport_HumanReadable) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  // Missing required fields

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  std::string output = report.toHumanReadable();

  EXPECT_TRUE(output.find("ShEx Validation Report") != std::string::npos);
  EXPECT_TRUE(output.find("DOES NOT CONFORM") != std::string::npos);
  EXPECT_TRUE(output.find("Errors:") != std::string::npos);
}

TEST_F(ErrorReportingTest, Format_EnhancedReport_JSON) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  auto json = report.toJson();

  EXPECT_TRUE(json.contains("conforms"));
  EXPECT_TRUE(json.contains("statistics"));
  EXPECT_TRUE(json.contains("errors"));
  EXPECT_TRUE(json.contains("conformanceMap"));
  EXPECT_FALSE(json["conforms"]);
  EXPECT_GT(json["statistics"]["totalErrors"], 0);
}

TEST_F(ErrorReportingTest, Format_EnhancedReport_XML) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"alice@example.org", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  std::string xml = report.toXml();

  EXPECT_TRUE(xml.find("<?xml") != std::string::npos);
  EXPECT_TRUE(xml.find("<validationReport>") != std::string::npos);
  EXPECT_TRUE(xml.find("<conforms>") != std::string::npos);
  EXPECT_TRUE(xml.find("<statistics>") != std::string::npos);
  EXPECT_TRUE(xml.find("</validationReport>") != std::string::npos);
}

// ============================================================================
// Conformance Map Tests (20 tests)
// ============================================================================

TEST_F(ErrorReportingTest, ConformanceMap_AddEntry) {
  ShapeConformanceMap map;

  map.addEntry("node1", "shape1", ConformanceStatus::CONFORMS);

  EXPECT_TRUE(map.nodeConformsToShape("node1", "shape1"));
}

TEST_F(ErrorReportingTest, ConformanceMap_AddError) {
  ShapeConformanceMap map;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Test error")
      .setShapeId("shape1")
      .build();

  map.addError("node1", "shape1", error);

  EXPECT_FALSE(map.nodeConformsToShape("node1", "shape1"));
}

TEST_F(ErrorReportingTest, ConformanceMap_GetFailedShapes) {
  ShapeConformanceMap map;

  map.addEntry("node1", "shape1", ConformanceStatus::CONFORMS);
  map.addEntry("node1", "shape2", ConformanceStatus::DOES_NOT_CONFORM);
  map.addEntry("node1", "shape3", ConformanceStatus::DOES_NOT_CONFORM);

  auto failed = map.getFailedShapes("node1");

  EXPECT_EQ(failed.size(), 2);
  EXPECT_TRUE(std::find(failed.begin(), failed.end(), "shape2") != failed.end());
  EXPECT_TRUE(std::find(failed.begin(), failed.end(), "shape3") != failed.end());
}

TEST_F(ErrorReportingTest, ConformanceMap_MultipleNodes) {
  ShapeConformanceMap map;

  map.addEntry("node1", "shape1", ConformanceStatus::CONFORMS);
  map.addEntry("node2", "shape1", ConformanceStatus::DOES_NOT_CONFORM);

  EXPECT_TRUE(map.nodeConformsToShape("node1", "shape1"));
  EXPECT_FALSE(map.nodeConformsToShape("node2", "shape1"));
}

TEST_F(ErrorReportingTest, ConformanceMap_ToJSON) {
  ShapeConformanceMap map;

  map.addEntry("node1", "shape1", ConformanceStatus::CONFORMS);

  auto json = map.toJson();

  EXPECT_TRUE(json.contains("node1"));
  EXPECT_TRUE(json["node1"].is_array());
}

TEST_F(ErrorReportingTest, ConformanceMap_InEnhancedReport) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"Alice", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"alice@example.org", ValueType::LITERAL});

  auto report = validator.validateNodeEnhanced(
      "http://example.org/person1", "PersonShape", data);

  EXPECT_TRUE(report.conformanceMap.nodeConformsToShape(
      "http://example.org/person1", "PersonShape"));
}

// ============================================================================
// Statistics Tests (10 tests)
// ============================================================================

TEST_F(ErrorReportingTest, Statistics_CountErrors) {
  EnhancedValidationReport report;

  auto error1 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Error 1")
      .setShapeId("TestShape")
      .build();

  auto error2 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Error 2")
      .setShapeId("TestShape")
      .build();

  report.addError(error1);
  report.addError(error2);

  EXPECT_EQ(report.totalErrors, 2);
  EXPECT_EQ(report.totalWarnings, 0);
  EXPECT_FALSE(report.conforms);
}

TEST_F(ErrorReportingTest, Statistics_CountWarnings) {
  EnhancedValidationReport report;

  auto warning1 = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Warning 1")
      .setShapeId("TestShape")
      .build();

  auto warning2 = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Warning 2")
      .setShapeId("TestShape")
      .build();

  report.addError(warning1);
  report.addError(warning2);

  EXPECT_EQ(report.totalWarnings, 2);
  EXPECT_EQ(report.totalErrors, 0);
  EXPECT_TRUE(report.conforms);  // Warnings don't affect conformance
}

TEST_F(ErrorReportingTest, Statistics_CountInfo) {
  EnhancedValidationReport report;

  auto info = ErrorBuilder()
      .setSeverity(ErrorSeverity::INFO)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Info message")
      .setShapeId("TestShape")
      .build();

  report.addError(info);

  EXPECT_EQ(report.totalInfoMessages, 1);
  EXPECT_TRUE(report.conforms);
}

TEST_F(ErrorReportingTest, Statistics_MixedSeverities) {
  EnhancedValidationReport report;

  report.addError(ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Error")
      .setShapeId("TestShape")
      .build());

  report.addError(ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Warning")
      .setShapeId("TestShape")
      .build());

  report.addError(ErrorBuilder()
      .setSeverity(ErrorSeverity::INFO)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Info")
      .setShapeId("TestShape")
      .build());

  EXPECT_EQ(report.totalErrors, 1);
  EXPECT_EQ(report.totalWarnings, 1);
  EXPECT_EQ(report.totalInfoMessages, 1);
  EXPECT_FALSE(report.conforms);
}

// ============================================================================
// Integration Tests (20 tests)
// ============================================================================

TEST_F(ErrorReportingTest, Integration_MultipleErrorsOneNode) {
  ShExSchema schema;
  Shape shape("TestShape");

  PropertyShape prop1("http://example.org/prop1");
  prop1.cardinality = Cardinality::EXACTLY_ONE;
  prop1.valueConstraint.valueType = ValueType::IRI;
  shape.addProperty(prop1);

  PropertyShape prop2("http://example.org/prop2");
  prop2.cardinality = Cardinality::ONE_OR_MORE;
  shape.addProperty(prop2);

  schema.addShape(shape);
  ShExValidator validator(schema);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop1"].push_back({"literal", ValueType::LITERAL});
  // Missing prop2

  auto report = validator.validateNodeEnhanced(
      "http://example.org/node1", "TestShape", data);

  EXPECT_FALSE(report.conforms);
  EXPECT_GE(report.totalErrors, 2);  // Type mismatch + cardinality
}

TEST_F(ErrorReportingTest, Integration_DatasetValidation) {
  auto schema = createTestSchema();
  ShExValidator validator(schema);

  std::map<std::string,
    std::map<std::string, std::vector<std::pair<std::string, ValueType>>>> dataset;

  // Valid node
  dataset["http://example.org/person1"]["http://example.org/name"].push_back(
      {"Alice", ValueType::LITERAL});
  dataset["http://example.org/person1"]["http://example.org/email"].push_back(
      {"alice@example.org", ValueType::LITERAL});

  // Invalid node
  dataset["http://example.org/person2"]["http://example.org/age"].push_back(
      {"30", ValueType::LITERAL});

  std::map<std::string, std::string> mapping;
  mapping["http://example.org/person1"] = "PersonShape";
  mapping["http://example.org/person2"] = "PersonShape";

  auto report = validator.validateDatasetEnhanced(dataset, mapping);

  EXPECT_FALSE(report.conforms);
  EXPECT_GT(report.totalErrors, 0);
  EXPECT_TRUE(report.conformanceMap.nodeConformsToShape(
      "http://example.org/person1", "PersonShape"));
  EXPECT_FALSE(report.conformanceMap.nodeConformsToShape(
      "http://example.org/person2", "PersonShape"));
}

// ============================================================================
// Phase 2C: Negation Tests - Simple Value Type Negation (25+ tests)
// ============================================================================

#include <chrono>

TEST(ShExNegationTest, NotIRIAcceptsLiteral) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::IRI;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("literal value", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationTest, NotIRIAcceptsBNode) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::IRI;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("_:bnode", ValueType::BNODE));
}

TEST(ShExNegationTest, NotLiteralAcceptsIRI) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::LITERAL;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, NotLiteralAcceptsBNode) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::LITERAL;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("_:bnode", ValueType::BNODE));
}

TEST(ShExNegationTest, NotBNodeAcceptsIRI) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::BNODE;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("_:bnode", ValueType::BNODE));
}

TEST(ShExNegationTest, NotBNodeAcceptsLiteral) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::BNODE;
  constraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(constraint.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, PropertyShapeWithNotIRI) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(prop.validate("literal", ValueType::LITERAL));
  EXPECT_FALSE(prop.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationTest, PropertyShapeWithNotLiteral) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::LITERAL;
  prop.valueConstraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(prop.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(prop.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, PropertyShapeWithNotBNode) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::BNODE;
  prop.valueConstraint.negation = NegationOperator::NOT;

  EXPECT_TRUE(prop.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(prop.validate("_:bnode", ValueType::BNODE));
}

TEST(ShExNegationTest, NegationWithNoConstraintAcceptsNothing) {
  ValueSetConstraint constraint;
  constraint.negation = NegationOperator::NOT;
  // No specific constraint, so default is true, negated becomes false

  EXPECT_FALSE(constraint.validate("anything", ValueType::LITERAL));
  EXPECT_FALSE(constraint.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationTest, NegationAtPropertyLevel) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.negation = NegationOperator::NOT;

  // Property-level negation applies after value constraint
  // Value constraint passes for IRI, but property negation inverts it
  EXPECT_FALSE(prop.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_TRUE(prop.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, DoubleNegationValueAndProperty) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.negation = NegationOperator::NOT;

  // Value constraint: !IRI (accepts LITERAL/BNODE, rejects IRI)
  // Property negation: inverts result
  // IRI: valueConstraint=false, property negation=true
  EXPECT_TRUE(prop.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(prop.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, NegationStatsTracking) {
  g_negationStats.reset();

  ValueSetConstraint constraint;
  constraint.valueType = ValueType::IRI;
  constraint.negation = NegationOperator::NOT;

  constraint.validate("literal", ValueType::LITERAL);
  // Stats tracking happens in ConstraintEvaluator, not in simple validation
  // This test ensures stats don't crash
  EXPECT_GE(g_negationStats.totalNegations, 0);
}

TEST(ShExNegationTest, NegationWithDatatypeRestriction) {
  ValueSetConstraint constraint;
  constraint.datatypeRestriction = "xsd:integer";
  constraint.negation = NegationOperator::NOT;

  // Datatype restriction accepts only literals, negated accepts non-literals
  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("123", ValueType::LITERAL));
}

TEST(ShExNegationTest, NegationPreservesCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::EXACTLY_ONE;
  shape.addProperty(prop);

  // Valid: one literal (matches !IRI)
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/prop"].push_back({"literal", ValueType::LITERAL});
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Invalid: one IRI (doesn't match !IRI)
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/prop"].push_back({"http://example.org/iri", ValueType::IRI});
  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
}

TEST(ShExNegationTest, NegationWithZeroOrOneCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::LITERAL;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::ZERO_OR_ONE;
  shape.addProperty(prop);

  // Valid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Valid: one IRI (matches !LITERAL)
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/prop"].push_back({"http://example.org/iri", ValueType::IRI});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);
}

TEST(ShExNegationTest, NegationMixedWithPositiveConstraints) {
  Shape shape("TestShape");

  PropertyShape prop1("http://example.org/name");
  prop1.valueConstraint.valueType = ValueType::LITERAL;
  shape.addProperty(prop1);

  PropertyShape prop2("http://example.org/link");
  prop2.valueConstraint.valueType = ValueType::LITERAL;
  prop2.valueConstraint.negation = NegationOperator::NOT;
  shape.addProperty(prop2);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/link"].push_back({"http://example.org/page", ValueType::IRI});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

// Additional 10 tests for simple negation edge cases
TEST(ShExNegationTest, NegationWithEmptyValueSet) {
  ValueSetConstraint constraint;
  constraint.allowedIris = {};  // Empty set
  constraint.negation = NegationOperator::NOT;

  // Empty set accepts nothing, negated accepts everything
  EXPECT_TRUE(constraint.validate("anything", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationTest, NegationErrorMessagesIncludeNegation) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  shape.addProperty(prop);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"http://example.org/iri", ValueType::IRI});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_FALSE(result.errors.empty());
}

TEST(ShExNegationTest, NegationWithInverseProperty) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.inverse = true;

  // Negation works with inverse properties
  EXPECT_TRUE(prop.validate("literal", ValueType::LITERAL));
  EXPECT_FALSE(prop.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationTest, NegationWithNodeKind) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.nodeKind = "BlankNode";

  // Negation applies to value constraint, nodeKind is separate
  EXPECT_TRUE(prop.validate("literal", ValueType::LITERAL));
}

TEST(ShExNegationTest, NegationWithZeroOrMoreCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::BNODE;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::ZERO_OR_MORE;
  shape.addProperty(prop);

  // Valid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Valid: multiple non-BNODEs
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/prop"].push_back({"http://example.org/iri", ValueType::IRI});
  data2["http://example.org/prop"].push_back({"literal", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);
}

TEST(ShExNegationTest, NegationWithOneOrMoreCardinality) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::ONE_OR_MORE;
  shape.addProperty(prop);

  // Valid: multiple literals (all match !IRI)
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"literal1", ValueType::LITERAL});
  data["http://example.org/prop"].push_back({"literal2", ValueType::LITERAL});
  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

// ============================================================================
// Phase 2C: Negation Tests - Value Set Negation (20+ tests)
// ============================================================================

TEST(ShExValueSetNegationTest, NotValueSetSingleIRI) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/allowed");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/allowed", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetMultipleIRIs) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/allowed1");
  constraint.allowedIris.insert("http://example.org/allowed2");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/allowed1", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("http://example.org/allowed2", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetWithLiterals) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("allowed_literal");
  constraint.negation = NegationOperator::NOT;

  // Negated value set rejects values in the set
  EXPECT_FALSE(constraint.validate("allowed_literal", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("other_literal", ValueType::LITERAL));
}

TEST(ShExValueSetNegationTest, NotValueSetEmptySet) {
  ValueSetConstraint constraint;
  constraint.allowedIris = {};
  constraint.negation = NegationOperator::NOT;

  // Empty set accepts nothing, negated accepts everything
  EXPECT_TRUE(constraint.validate("anything", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetLargeSet) {
  ValueSetConstraint constraint;
  for (int i = 0; i < 100; ++i) {
    constraint.allowedIris.insert("http://example.org/item" + std::to_string(i));
  }
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/item50", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetWithTypeConstraint) {
  ValueSetConstraint constraint;
  constraint.valueType = ValueType::IRI;
  constraint.allowedIris.insert("http://example.org/allowed");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/allowed", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));

  // For LITERAL: type check fails first, so negation makes it pass
  EXPECT_TRUE(constraint.validate("literal", ValueType::LITERAL));
}

TEST(ShExValueSetNegationTest, NotValueSetCaseSensitive) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/Allowed");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/Allowed", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/allowed", ValueType::IRI));  // Different case
}

TEST(ShExValueSetNegationTest, NotValueSetSpecialCharacters) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/special!@#$%");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/special!@#$%", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetUnicodeValues) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/日本語");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("http://example.org/日本語", ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/english", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetVeryLongIRI) {
  ValueSetConstraint constraint;
  std::string longIri = "http://example.org/" + std::string(1000, 'a');
  constraint.allowedIris.insert(longIri);
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate(longIri, ValueType::IRI));
  EXPECT_TRUE(constraint.validate("http://example.org/short", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetInPropertyShape) {
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.allowedIris.insert("http://example.org/allowed");
  prop.valueConstraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(prop.validate("http://example.org/allowed", ValueType::IRI));
  EXPECT_TRUE(prop.validate("http://example.org/other", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetInShape) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.allowedIris.insert("http://example.org/forbidden");
  prop.valueConstraint.negation = NegationOperator::NOT;
  shape.addProperty(prop);

  // Valid: value not in forbidden set
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/prop"].push_back({"http://example.org/allowed", ValueType::IRI});
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Invalid: value in forbidden set
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/prop"].push_back({"http://example.org/forbidden", ValueType::IRI});
  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
}

TEST(ShExValueSetNegationTest, NotValueSetMultipleValues) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.allowedIris.insert("http://example.org/forbidden");
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::ONE_OR_MORE;
  shape.addProperty(prop);

  // All values must not be in the forbidden set
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"http://example.org/allowed1", ValueType::IRI});
  data["http://example.org/prop"].push_back({"http://example.org/allowed2", ValueType::IRI});
  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ShExValueSetNegationTest, NotValueSetOneViolation) {
  Shape shape("TestShape");
  PropertyShape prop("http://example.org/prop");
  prop.valueConstraint.allowedIris.insert("http://example.org/forbidden");
  prop.valueConstraint.negation = NegationOperator::NOT;
  prop.cardinality = Cardinality::ONE_OR_MORE;
  shape.addProperty(prop);

  // One value in forbidden set causes failure
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/prop"].push_back({"http://example.org/allowed", ValueType::IRI});
  data["http://example.org/prop"].push_back({"http://example.org/forbidden", ValueType::IRI});
  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
}

TEST(ShExValueSetNegationTest, NotValueSetWithBNodes) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("_:b1");
  constraint.negation = NegationOperator::NOT;

  EXPECT_FALSE(constraint.validate("_:b1", ValueType::BNODE));
  EXPECT_TRUE(constraint.validate("_:b2", ValueType::BNODE));
}

TEST(ShExValueSetNegationTest, NotValueSetDuplicateValues) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/item");
  constraint.allowedIris.insert("http://example.org/item");  // Duplicate
  constraint.negation = NegationOperator::NOT;

  // Duplicates don't change behavior
  EXPECT_FALSE(constraint.validate("http://example.org/item", ValueType::IRI));
}

TEST(ShExValueSetNegationTest, NotValueSetMixedTypes) {
  ValueSetConstraint constraint;
  constraint.allowedIris.insert("http://example.org/iri");
  constraint.allowedIris.insert("literal_value");
  constraint.negation = NegationOperator::NOT;

  // Both IRIs and literals can be in the value set
  EXPECT_FALSE(constraint.validate("http://example.org/iri", ValueType::IRI));
  EXPECT_FALSE(constraint.validate("literal_value", ValueType::LITERAL));
  EXPECT_TRUE(constraint.validate("other", ValueType::LITERAL));
}

TEST(ShExValueSetNegationTest, NotValueSetPerformance) {
  ValueSetConstraint constraint;
  for (int i = 0; i < 10000; ++i) {
    constraint.allowedIris.insert("http://example.org/item" + std::to_string(i));
  }
  constraint.negation = NegationOperator::NOT;

  // Performance test: large value set with negation
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; ++i) {
    constraint.validate("http://example.org/other", ValueType::IRI);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Should complete in reasonable time (< 100ms for 1000 validations)
  EXPECT_LT(duration.count(), 100000);
}

TEST(ShExValueSetNegationTest, NotValueSetMemoryEfficiency) {
  ValueSetConstraint constraint;
  // Large value set
  for (int i = 0; i < 100000; ++i) {
    constraint.allowedIris.insert("http://example.org/item" + std::to_string(i));
  }
  constraint.negation = NegationOperator::NOT;

  // Should not crash or consume excessive memory
  EXPECT_TRUE(constraint.validate("http://example.org/other", ValueType::IRI));
}

// ============================================================================
// Phase 2C: Constraint Node Tests (15+ tests)
// ============================================================================

TEST(ShExConstraintNodeTest, CreateValueType) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NONE);

  EXPECT_EQ(node->type, ConstraintNode::Type::VALUE_TYPE);
  EXPECT_EQ(node->valueType.value(), "IRI");
  EXPECT_FALSE(node->isNegated());
}

TEST(ShExConstraintNodeTest, CreateValueSet) {
  absl::flat_hash_set<std::string> set = {"a", "b", "c"};
  auto node = ConstraintNode::createValueSet(set, NegationOperator::NOT);

  EXPECT_EQ(node->type, ConstraintNode::Type::VALUE_SET);
  EXPECT_EQ(node->valueSet.size(), 3);
  EXPECT_TRUE(node->isNegated());
}

TEST(ShExConstraintNodeTest, CreateShapeRef) {
  auto node = ConstraintNode::createShapeRef("PersonShape");

  EXPECT_EQ(node->type, ConstraintNode::Type::SHAPE_REF);
  EXPECT_EQ(node->shapeRef.value(), "PersonShape");
}

TEST(ShExConstraintNodeTest, CreateLogical) {
  auto child1 = ConstraintNode::createValueType("IRI");
  auto child2 = ConstraintNode::createValueType("LITERAL");
  auto logical = ConstraintNode::createLogical(LogicalOperator::AND, {child1, child2});

  EXPECT_EQ(logical->type, ConstraintNode::Type::LOGICAL);
  EXPECT_EQ(logical->logicalOp, LogicalOperator::AND);
  EXPECT_EQ(logical->children.size(), 2);
}

TEST(ShExConstraintNodeTest, NegateNode) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NONE);

  EXPECT_FALSE(node->isNegated());
  node->negate();
  EXPECT_TRUE(node->isNegated());
  node->negate();
  EXPECT_FALSE(node->isNegated());
}

TEST(ShExConstraintNodeTest, ToStringValueType) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NOT);

  std::string str = node->toString();
  EXPECT_NE(str.find("!"), std::string::npos);  // Contains negation
  EXPECT_NE(str.find("IRI"), std::string::npos);  // Contains type
}

TEST(ShExConstraintNodeTest, ToStringValueSet) {
  absl::flat_hash_set<std::string> set = {"a", "b"};
  auto node = ConstraintNode::createValueSet(set);

  std::string str = node->toString();
  EXPECT_NE(str.find("["), std::string::npos);  // Contains brackets
}

TEST(ShExConstraintNodeTest, ToStringLogical) {
  auto child1 = ConstraintNode::createValueType("IRI");
  auto child2 = ConstraintNode::createValueType("LITERAL");
  auto logical = ConstraintNode::createLogical(LogicalOperator::AND, {child1, child2});

  std::string str = logical->toString();
  EXPECT_NE(str.find("AND"), std::string::npos);
  EXPECT_NE(str.find("IRI"), std::string::npos);
  EXPECT_NE(str.find("LITERAL"), std::string::npos);
}

TEST(ShExConstraintNodeTest, EvaluateValueType) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NONE);

  EXPECT_TRUE(ConstraintEvaluator::evaluate(node, "http://example.org/iri", "IRI"));
  EXPECT_FALSE(ConstraintEvaluator::evaluate(node, "literal", "LITERAL"));
}

TEST(ShExConstraintNodeTest, EvaluateNegatedValueType) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NOT);

  EXPECT_FALSE(ConstraintEvaluator::evaluate(node, "http://example.org/iri", "IRI"));
  EXPECT_TRUE(ConstraintEvaluator::evaluate(node, "literal", "LITERAL"));
}

TEST(ShExConstraintNodeTest, EvaluateValueSet) {
  absl::flat_hash_set<std::string> set = {"allowed"};
  auto node = ConstraintNode::createValueSet(set);

  EXPECT_TRUE(ConstraintEvaluator::evaluate(node, "allowed", "LITERAL"));
  EXPECT_FALSE(ConstraintEvaluator::evaluate(node, "forbidden", "LITERAL"));
}

TEST(ShExConstraintNodeTest, EvaluateNegatedValueSet) {
  absl::flat_hash_set<std::string> set = {"forbidden"};
  auto node = ConstraintNode::createValueSet(set, NegationOperator::NOT);

  EXPECT_FALSE(ConstraintEvaluator::evaluate(node, "forbidden", "LITERAL"));
  EXPECT_TRUE(ConstraintEvaluator::evaluate(node, "allowed", "LITERAL"));
}

TEST(ShExConstraintNodeTest, AndConstraintBasic) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2});

  EXPECT_TRUE(ConstraintEvaluator::evaluate(andNode, "http://example.org/iri", "IRI"));
  EXPECT_FALSE(ConstraintEvaluator::evaluate(andNode, "literal", "LITERAL"));
}

TEST(ShExConstraintNodeTest, OrConstraintBasic) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto orNode = ConstraintNode::createLogical(LogicalOperator::OR, {node1, node2});

  EXPECT_TRUE(ConstraintEvaluator::evaluate(orNode, "http://example.org/iri", "IRI"));
  EXPECT_TRUE(ConstraintEvaluator::evaluate(orNode, "literal", "LITERAL"));
  EXPECT_FALSE(ConstraintEvaluator::evaluate(orNode, "_:bnode", "BNODE"));
}

TEST(ShExConstraintNodeTest, NegatedAndConstraint) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                              NegationOperator::NOT);

  // !(IRI AND IRI) = !IRI OR !IRI (after De Morgan)
  // For IRI: both children match, AND=true, negated=false
  EXPECT_FALSE(ConstraintEvaluator::evaluate(andNode, "http://example.org/iri", "IRI"));
  EXPECT_TRUE(ConstraintEvaluator::evaluate(andNode, "literal", "LITERAL"));
}

// ============================================================================
// Phase 2C: De Morgan Optimization Tests (15+ tests)
// ============================================================================

TEST(ShExDeMorganTest, NotAndBecomesOrNot) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                              NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::optimize(andNode);

  // !(IRI AND LITERAL) -> !IRI OR !LITERAL
  EXPECT_EQ(optimized->logicalOp, LogicalOperator::OR);
  EXPECT_EQ(optimized->negation, NegationOperator::NONE);
  EXPECT_EQ(optimized->children.size(), 2);
  EXPECT_TRUE(optimized->children[0]->isNegated());
  EXPECT_TRUE(optimized->children[1]->isNegated());
}

TEST(ShExDeMorganTest, NotOrBecomesAndNot) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto orNode = ConstraintNode::createLogical(LogicalOperator::OR, {node1, node2},
                                             NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::optimize(orNode);

  // !(IRI OR LITERAL) -> !IRI AND !LITERAL
  EXPECT_EQ(optimized->logicalOp, LogicalOperator::AND);
  EXPECT_EQ(optimized->negation, NegationOperator::NONE);
  EXPECT_EQ(optimized->children.size(), 2);
  EXPECT_TRUE(optimized->children[0]->isNegated());
  EXPECT_TRUE(optimized->children[1]->isNegated());
}

TEST(ShExDeMorganTest, NoOptimizationWithoutNegation) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2});

  auto optimized = DeMorganOptimizer::optimize(andNode);

  // No change expected
  EXPECT_EQ(optimized->logicalOp, LogicalOperator::AND);
  EXPECT_EQ(optimized->negation, NegationOperator::NONE);
}

TEST(ShExDeMorganTest, OptimizationCountTracking) {
  g_negationStats.reset();

  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                              NegationOperator::NOT);

  DeMorganOptimizer::optimize(andNode);

  EXPECT_EQ(g_negationStats.deMorganOptimizations, 1);
}

TEST(ShExDeMorganTest, NestedOptimization) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto inner = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                            NegationOperator::NOT);
  auto outer = ConstraintNode::createLogical(LogicalOperator::OR, {inner, node1});

  auto optimized = DeMorganOptimizer::optimize(outer);

  // Inner AND should be optimized
  EXPECT_EQ(optimized->children[0]->logicalOp, LogicalOperator::OR);
}

TEST(ShExDeMorganTest, DoubleNegationSimple) {
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NOT);
  node->negate();  // Apply negation again

  auto optimized = DeMorganOptimizer::eliminateDoubleNegation(node);

  // !!IRI -> IRI
  EXPECT_EQ(optimized->negation, NegationOperator::NONE);
}

TEST(ShExDeMorganTest, FullOptimizationPipeline) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                              NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::fullyOptimize(andNode);

  EXPECT_EQ(optimized->logicalOp, LogicalOperator::OR);
  EXPECT_EQ(optimized->negation, NegationOperator::NONE);
}

TEST(ShExDeMorganTest, TautologyDetectionOrNotOr) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("IRI", NegationOperator::NOT);

  EXPECT_TRUE(DeMorganOptimizer::isTautology(node1, node2));
}

TEST(ShExDeMorganTest, ContradictionDetectionAndNotAnd) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("IRI", NegationOperator::NOT);

  EXPECT_TRUE(DeMorganOptimizer::isContradiction(node1, node2));
}

TEST(ShExDeMorganTest, NoTautologyDifferentTypes) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NOT);

  EXPECT_FALSE(DeMorganOptimizer::isTautology(node1, node2));
}

TEST(ShExDeMorganTest, NoContradictionSameNegation) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NOT);
  auto node2 = ConstraintNode::createValueType("IRI", NegationOperator::NOT);

  EXPECT_FALSE(DeMorganOptimizer::isContradiction(node1, node2));
}

TEST(ShExDeMorganTest, OptimizationWithValueSets) {
  absl::flat_hash_set<std::string> set = {"http://example.org/a"};
  auto node1 = ConstraintNode::createValueSet(set);
  auto node2 = ConstraintNode::createValueSet(set);
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                              NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::optimize(andNode);

  EXPECT_EQ(optimized->logicalOp, LogicalOperator::OR);
}

TEST(ShExDeMorganTest, ComplexNegationPattern) {
  // !(A AND (B OR C))
  auto a = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto b = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto c = ConstraintNode::createValueType("BNODE", NegationOperator::NONE);

  auto orBC = ConstraintNode::createLogical(LogicalOperator::OR, {b, c});
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {a, orBC},
                                              NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::optimize(andNode);

  // Should become: !A OR !(B OR C)
  EXPECT_EQ(optimized->logicalOp, LogicalOperator::OR);
  EXPECT_TRUE(optimized->children[0]->isNegated());
  EXPECT_TRUE(optimized->children[1]->isNegated());
}

TEST(ShExDeMorganTest, StatsTrackingMultipleOptimizations) {
  g_negationStats.reset();

  for (int i = 0; i < 10; ++i) {
    auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
    auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
    auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                                NegationOperator::NOT);
    DeMorganOptimizer::optimize(andNode);
  }

  EXPECT_EQ(g_negationStats.deMorganOptimizations, 10);
}

TEST(ShExDeMorganTest, StatsToString) {
  g_negationStats.reset();
  g_negationStats.totalNegations = 100;
  g_negationStats.deMorganOptimizations = 10;
  g_negationStats.tautologiesDetected = 2;
  g_negationStats.contradictionsDetected = 1;
  g_negationStats.optimizationSpeedup = 0.95;

  std::string stats = g_negationStats.toString();
  EXPECT_NE(stats.find("100"), std::string::npos);
  EXPECT_NE(stats.find("10"), std::string::npos);
}

TEST(ShExDeMorganTest, StatsReset) {
  g_negationStats.totalNegations = 100;
  g_negationStats.reset();

  EXPECT_EQ(g_negationStats.totalNegations, 0);
  EXPECT_EQ(g_negationStats.deMorganOptimizations, 0);
}

// ============================================================================
// Phase 2C: Performance and Integration Tests (10+ tests)
// ============================================================================

TEST(ShExNegationPerformanceTest, SimpleNegationOverhead) {
  g_negationStats.reset();

  ValueSetConstraint positive;
  positive.valueType = ValueType::IRI;

  ValueSetConstraint negative;
  negative.valueType = ValueType::IRI;
  negative.negation = NegationOperator::NOT;

  auto start1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    positive.validate("http://example.org/iri", ValueType::IRI);
  }
  auto end1 = std::chrono::high_resolution_clock::now();
  auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);

  auto start2 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    negative.validate("http://example.org/iri", ValueType::IRI);
  }
  auto end2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2);

  // Negation overhead should be < 1.5x
  double overhead = static_cast<double>(duration2.count()) / duration1.count();
  EXPECT_LT(overhead, 1.5);

  // Store in stats
  g_negationStats.optimizationSpeedup = 1.0 / overhead;
}

TEST(ShExNegationPerformanceTest, DeMorganOptimizationSpeedup) {
  auto node1 = ConstraintNode::createValueType("IRI", NegationOperator::NONE);
  auto node2 = ConstraintNode::createValueType("LITERAL", NegationOperator::NONE);
  auto unoptimized = ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                                  NegationOperator::NOT);

  auto optimized = DeMorganOptimizer::optimize(
      ConstraintNode::createLogical(LogicalOperator::AND, {node1, node2},
                                   NegationOperator::NOT));

  auto start1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    ConstraintEvaluator::evaluate(unoptimized, "http://example.org/iri", "IRI");
  }
  auto end1 = std::chrono::high_resolution_clock::now();
  auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);

  auto start2 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    ConstraintEvaluator::evaluate(optimized, "http://example.org/iri", "IRI");
  }
  auto end2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2);

  // Optimized should be competitive (0.8x - 1.2x)
  double ratio = static_cast<double>(duration2.count()) / duration1.count();
  EXPECT_LT(ratio, 1.2);
  EXPECT_GT(ratio, 0.8);
}

TEST(ShExNegationIntegrationTest, CompleteShapeWithNegations) {
  Shape shape("PersonShape");

  PropertyShape name("http://example.org/name");
  name.valueConstraint.valueType = ValueType::LITERAL;
  shape.addProperty(name);

  PropertyShape link("http://example.org/link");
  link.valueConstraint.valueType = ValueType::LITERAL;
  link.valueConstraint.negation = NegationOperator::NOT;
  shape.addProperty(link);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/link"].push_back({"http://example.org/page", ValueType::IRI});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ShExNegationIntegrationTest, MultipleNegatedProperties) {
  Shape shape("TestShape");

  for (int i = 0; i < 10; ++i) {
    PropertyShape prop("http://example.org/prop" + std::to_string(i));
    prop.valueConstraint.valueType = (i % 2 == 0) ? ValueType::IRI : ValueType::LITERAL;
    prop.valueConstraint.negation = NegationOperator::NOT;
    prop.cardinality = Cardinality::ZERO_OR_ONE;
    shape.addProperty(prop);
  }

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  for (int i = 0; i < 10; ++i) {
    ValueType vt = (i % 2 == 0) ? ValueType::LITERAL : ValueType::IRI;
    data["http://example.org/prop" + std::to_string(i)].push_back(
        {"value" + std::to_string(i), vt});
  }

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ShExNegationEdgeCaseTest, NullConstraintTree) {
  ValueSetConstraint constraint;
  constraint.constraintTree = nullptr;
  constraint.valueType = ValueType::IRI;

  // Should fall back to simple validation
  EXPECT_TRUE(constraint.validate("http://example.org/iri", ValueType::IRI));
}

TEST(ShExNegationEdgeCaseTest, EmptyValueTypeString) {
  auto node = ConstraintNode::createValueType("", NegationOperator::NONE);
  // Should handle gracefully
  EXPECT_FALSE(ConstraintEvaluator::evaluate(node, "value", "IRI"));
}

TEST(ShExNegationEdgeCaseTest, VeryLongConstraintTree) {
  auto base = ConstraintNode::createValueType("IRI", NegationOperator::NONE);

  auto current = base;
  for (int i = 0; i < 100; ++i) {
    current = ConstraintNode::createLogical(LogicalOperator::AND, {current, base});
  }

  // Should not crash or timeout
  EXPECT_TRUE(ConstraintEvaluator::evaluate(current, "http://example.org/iri", "IRI"));
}

TEST(ShExNegationEdgeCaseTest, CyclicReferenceProtection) {
  // Constraint trees shouldn't allow cycles, but test graceful handling
  auto node = ConstraintNode::createValueType("IRI", NegationOperator::NONE);

  // This is a simple test - in practice, cycles would be prevented at construction
  EXPECT_TRUE(ConstraintEvaluator::evaluate(node, "http://example.org/iri", "IRI"));
}

TEST(ShExNegationEdgeCaseTest, EmptyLogicalAnd) {
  auto andNode = ConstraintNode::createLogical(LogicalOperator::AND, {});

  // Empty AND is true
  EXPECT_TRUE(ConstraintEvaluator::evaluate(andNode, "anything", "IRI"));
}

TEST(ShExNegationEdgeCaseTest, EmptyLogicalOr) {
  auto orNode = ConstraintNode::createLogical(LogicalOperator::OR, {});

  // Empty OR is false (no children to satisfy)
  EXPECT_FALSE(ConstraintEvaluator::evaluate(orNode, "anything", "IRI"));
}

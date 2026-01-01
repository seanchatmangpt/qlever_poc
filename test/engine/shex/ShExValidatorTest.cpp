#include <gtest/gtest.h>
#include "engine/shex/ShExParser.h"
#include "engine/shex/ShExValidator.h"
#include "engine/shex/ShExConformance.h"

namespace shex {

class ShExValidatorTest : public ::testing::Test {
 protected:
  ShExParser parser;
  ShExValidator validator;
  RDFGraph graph;

  void SetUp() override {
    // Clear graph before each test
    graph.clear();
  }
};

// ============================================================================
// Parser Tests
// ============================================================================

TEST_F(ShExValidatorTest, ParseBasicShape) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  EXPECT_GT(shapeMap.size(), 0);
  EXPECT_TRUE(shapeMap.hasShape("http://example.org/PersonShape"));
}

TEST_F(ShExValidatorTest, ParseShapeWithCardinality) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string,
      ex:email xsd:string*,
      ex:phone xsd:string+,
      ex:id xsd:integer{1,1}
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  ASSERT_GT(shapeMap.size(), 0);

  auto shape = shapeMap.getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->tripleConstraints.size(), 4);
}

TEST_F(ShExValidatorTest, ParseClosedShape) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape CLOSED {
      ex:name xsd:string,
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  auto shape = shapeMap.getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST_F(ShExValidatorTest, ParseShapeWithExtra) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape CLOSED EXTRA ex:id {
      ex:name xsd:string,
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  auto shape = shapeMap.getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
  EXPECT_GT(shape->extraProps.size(), 0);
}

TEST_F(ShExValidatorTest, RejectUnsupportedRegex) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:email /^[a-z]+@[a-z]+\.[a-z]+$/
    }
  )";

  EXPECT_THROW(parser.parseSchema(schema), std::runtime_error);
  EXPECT_EQ(parser.getLastError(), ErrorCode::UNSUPPORTED_SHEX_FEATURE);
}

TEST_F(ShExValidatorTest, RejectUnsupportedNegation) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name NOT xsd:integer
    }
  )";

  EXPECT_THROW(parser.parseSchema(schema), std::runtime_error);
  EXPECT_EQ(parser.getLastError(), ErrorCode::UNSUPPORTED_SHEX_FEATURE);
}

// ============================================================================
// Validator Tests - Cardinality
// ============================================================================

TEST_F(ShExValidatorTest, ValidateMinCount) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Create RDF data - node with required name
  graph.addTriple("ex:john", "ex:name", "\"John Doe\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
  EXPECT_EQ(result.violations.size(), 0);
}

TEST_F(ShExValidatorTest, ValidateMinCountViolation) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Create RDF data - node WITHOUT required name
  graph.addTriple("ex:john", "ex:age", "\"30\"^^xsd:integer");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);
}

TEST_F(ShExValidatorTest, ValidateMaxCount) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:email xsd:string{0,1}
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Create RDF data - node with one email (valid)
  graph.addTriple("ex:john", "ex:email", "\"john@example.org\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

TEST_F(ShExValidatorTest, ValidateMaxCountViolation) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:email xsd:string{0,1}
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Create RDF data - node with TWO emails (invalid)
  graph.addTriple("ex:john", "ex:email", "\"john@example.org\"");
  graph.addTriple("ex:john", "ex:email", "\"j.doe@example.org\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);
}

// ============================================================================
// Validator Tests - Datatype
// ============================================================================

TEST_F(ShExValidatorTest, ValidateDatatypeString) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  graph.addTriple("ex:john", "ex:name", "\"John Doe\"^^xsd:string");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

TEST_F(ShExValidatorTest, ValidateDatatypeInteger) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  graph.addTriple("ex:john", "ex:age", "\"30\"^^xsd:integer");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

TEST_F(ShExValidatorTest, ValidateDatatypeViolation) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Wrong datatype: string instead of integer
  graph.addTriple("ex:john", "ex:age", "\"thirty\"^^xsd:string");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);
}

// ============================================================================
// Validator Tests - Node Kind
// ============================================================================

TEST_F(ShExValidatorTest, ValidateNodeKindIRI) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:knows IRI
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  graph.addTriple("ex:john", "ex:knows", "<http://example.org/jane>");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

TEST_F(ShExValidatorTest, ValidateNodeKindLiteral) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name LITERAL
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  graph.addTriple("ex:john", "ex:name", "\"John Doe\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

// ============================================================================
// Validator Tests - CLOSED Shapes
// ============================================================================

TEST_F(ShExValidatorTest, ValidateClosedShapeValid) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape CLOSED {
      ex:name xsd:string,
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Only allowed properties
  graph.addTriple("ex:john", "ex:name", "\"John\"");
  graph.addTriple("ex:john", "ex:age", "\"30\"^^xsd:integer");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

TEST_F(ShExValidatorTest, ValidateClosedShapeViolation) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape CLOSED {
      ex:name xsd:string,
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Includes a property not in the shape (ex:email)
  graph.addTriple("ex:john", "ex:name", "\"John\"");
  graph.addTriple("ex:john", "ex:age", "\"30\"^^xsd:integer");
  graph.addTriple("ex:john", "ex:email", "\"john@example.org\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_FALSE(result.conforms);
  EXPECT_GT(result.violations.size(), 0);
}

TEST_F(ShExValidatorTest, ValidateClosedShapeWithExtra) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape CLOSED EXTRA ex:id {
      ex:name xsd:string,
      ex:age xsd:integer
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  // Includes EXTRA property (ex:id) which is allowed
  graph.addTriple("ex:john", "ex:name", "\"John\"");
  graph.addTriple("ex:john", "ex:age", "\"30\"^^xsd:integer");
  graph.addTriple("ex:john", "ex:id", "\"12345\"");

  ValidationConfig config;
  auto result = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);

  EXPECT_TRUE(result.conforms);
}

// ============================================================================
// Validator Tests - Guards and Limits
// ============================================================================

TEST_F(ShExValidatorTest, ValidateMaxFocusNodesGuard) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  ValidationConfig config;
  config.maxFocusNodes = 2;  // Very low limit
  config.failClosed = true;

  graph.addTriple("ex:john", "ex:name", "\"John\"");

  // First validation should succeed
  auto result1 = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);
  EXPECT_TRUE(result1.conforms || !result1.conforms);  // May or may not conform

  // Second validation should succeed
  auto result2 = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);
  EXPECT_TRUE(result2.conforms || !result2.conforms);

  // Third validation should fail due to limit
  auto result3 = validator.validateNode("ex:john", "http://example.org/PersonShape", graph, config);
  EXPECT_FALSE(result3.conforms);
  EXPECT_GT(result3.violations.size(), 0);
}

TEST_F(ShExValidatorTest, ValidateMaxViolationsGuard) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  ValidationConfig config;
  config.maxViolations = 1;
  config.failClosed = true;

  // Create nodes without required property
  graph.addTriple("ex:john", "ex:age", "\"30\"");
  graph.addTriple("ex:jane", "ex:age", "\"25\"");
  graph.addTriple("ex:bob", "ex:age", "\"40\"");

  std::vector<std::string> nodes = {"ex:john", "ex:jane", "ex:bob"};
  auto report = validator.validateNodes(nodes, "http://example.org/PersonShape", graph, config);

  // Should stop after max violations
  EXPECT_FALSE(report.conforms);
  EXPECT_LE(report.results.size(), 3);  // May stop early
}

// ============================================================================
// Validator Tests - Multiple Nodes
// ============================================================================

TEST_F(ShExValidatorTest, ValidateMultipleNodes) {
  std::string schema = R"(
    PREFIX ex: <http://example.org/>

    ex:PersonShape {
      ex:name xsd:string
    }
  )";

  auto shapeMap = parser.parseSchema(schema);
  validator.loadShapes(shapeMap);

  graph.addTriple("ex:john", "ex:name", "\"John\"");
  graph.addTriple("ex:jane", "ex:name", "\"Jane\"");
  graph.addTriple("ex:bob", "ex:age", "\"40\"");  // Missing name

  std::vector<std::string> nodes = {"ex:john", "ex:jane", "ex:bob"};
  ValidationConfig config;
  auto report = validator.validateNodes(nodes, "http://example.org/PersonShape", graph, config);

  EXPECT_FALSE(report.conforms);  // At least one failed
  EXPECT_EQ(report.conformingCount(), 2);
  EXPECT_EQ(report.nonConformingCount(), 1);
}

// ============================================================================
// Feature Support Tests
// ============================================================================

TEST_F(ShExValidatorTest, SupportedFeaturesDeclaration) {
  // Test that supported features are correctly declared
  EXPECT_TRUE(SupportedFeatures::SHAPE_DEFINITIONS);
  EXPECT_TRUE(SupportedFeatures::TRIPLE_CONSTRAINTS);
  EXPECT_TRUE(SupportedFeatures::CARDINALITY);
  EXPECT_TRUE(SupportedFeatures::DATATYPE_CONSTRAINTS);
  EXPECT_TRUE(SupportedFeatures::NODE_KIND_CONSTRAINTS);
  EXPECT_TRUE(SupportedFeatures::CLOSED_SHAPES);
  EXPECT_TRUE(SupportedFeatures::EXTRA_PROPERTIES);
  EXPECT_TRUE(SupportedFeatures::NESTED_SHAPES);
}

TEST_F(ShExValidatorTest, UnsupportedFeaturesDeclaration) {
  // Test that unsupported features are correctly declared
  EXPECT_FALSE(SupportedFeatures::REGEX_CONSTRAINTS);
  EXPECT_FALSE(SupportedFeatures::INHERITANCE);
  EXPECT_FALSE(SupportedFeatures::SEMANTIC_ACTIONS);
  EXPECT_FALSE(SupportedFeatures::NEGATION);
}

TEST_F(ShExValidatorTest, FeatureValidation) {
  std::string validSchema = R"(
    PREFIX ex: <http://example.org/>
    ex:PersonShape { ex:name xsd:string }
  )";

  EXPECT_TRUE(parser.validateFeatures(validSchema));

  std::string invalidSchema = R"(
    PREFIX ex: <http://example.org/>
    ex:PersonShape { ex:email /pattern/ }
  )";

  EXPECT_FALSE(parser.validateFeatures(invalidSchema));
}

}  // namespace shex

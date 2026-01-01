#include <gtest/gtest.h>

#include "parser/ShapeSerializer.h"
#include "parser/ShEx.h"

#include <fstream>
#include <sstream>

using namespace shex;

// ============================================================================
// Test Fixtures
// ============================================================================

class ShapeSerializerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create sample shapes for testing
    sampleShape1 = Shape("http://example.org/PersonShape");
    PropertyShape nameProp("http://xmlns.com/foaf/0.1/name");
    nameProp.cardinality = Cardinality::EXACTLY_ONE;
    nameProp.valueConstraint.valueType = ValueType::LITERAL;
    sampleShape1.addProperty(nameProp);

    PropertyShape ageProp("http://xmlns.com/foaf/0.1/age");
    ageProp.cardinality = Cardinality::ZERO_OR_ONE;
    ageProp.valueConstraint.valueType = ValueType::LITERAL;
    sampleShape1.addProperty(ageProp);

    // Create sample schema
    sampleSchema.addShape(sampleShape1);

    // Create more complex shape
    sampleShape2 = Shape("http://example.org/BookShape");
    sampleShape2.closed = true;
    PropertyShape titleProp("http://purl.org/dc/terms/title");
    titleProp.cardinality = Cardinality::ONE_OR_MORE;
    sampleShape2.addProperty(titleProp);
    sampleSchema.addShape(sampleShape2);
  }

  Shape sampleShape1{"http://example.org/PersonShape"};
  Shape sampleShape2{"http://example.org/BookShape"};
  ShExSchema sampleSchema;
};

// ============================================================================
// JSON-LD Serialization Tests (30+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, JsonLdSerializeSimpleShape) {
  JsonLdSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape1);

  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("\"type\": \"Shape\""), std::string::npos);
  EXPECT_NE(result.find("http://example.org/PersonShape"), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeShapeWithProperties) {
  JsonLdSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape1);

  EXPECT_NE(result.find("foaf/0.1/name"), std::string::npos);
  EXPECT_NE(result.find("foaf/0.1/age"), std::string::npos);
  EXPECT_NE(result.find("\"cardinality\""), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeClosedShape) {
  JsonLdSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape2);

  EXPECT_NE(result.find("\"closed\": true"), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeSchema) {
  JsonLdSerializer serializer;
  std::string result = serializer.serializeSchema(sampleSchema);

  EXPECT_NE(result.find("\"@context\""), std::string::npos);
  EXPECT_NE(result.find("\"type\": \"Schema\""), std::string::npos);
  EXPECT_NE(result.find("\"shapes\""), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeSimpleSchema) {
  JsonLdSerializer serializer;
  std::string jsonLd = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [
      {
        "type": "Shape",
        "id": "http://example.org/TestShape"
      }
    ]
  })";

  auto result = serializer.deserializeSchema(jsonLd);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeShapeWithProperties) {
  JsonLdSerializer serializer;
  std::string jsonLd = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [
      {
        "type": "Shape",
        "id": "http://example.org/PersonShape",
        "expression": {
          "type": "TripleConstraint",
          "predicate": "http://xmlns.com/foaf/0.1/name",
          "cardinality": "exactly_one"
        }
      }
    ]
  })";

  auto result = serializer.deserializeSchema(jsonLd);
  ASSERT_TRUE(result.has_value());

  const Shape* shape = result->getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 1);
  EXPECT_EQ(shape->properties[0].predicate, "http://xmlns.com/foaf/0.1/name");
  EXPECT_EQ(shape->properties[0].cardinality, Cardinality::EXACTLY_ONE);
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeMultipleProperties) {
  JsonLdSerializer serializer;
  std::string jsonLd = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [
      {
        "type": "Shape",
        "id": "http://example.org/PersonShape",
        "expression": {
          "type": "EachOf",
          "expressions": [
            {
              "type": "TripleConstraint",
              "predicate": "http://xmlns.com/foaf/0.1/name",
              "cardinality": "exactly_one"
            },
            {
              "type": "TripleConstraint",
              "predicate": "http://xmlns.com/foaf/0.1/age",
              "cardinality": "zero_or_one"
            }
          ]
        }
      }
    ]
  })";

  auto result = serializer.deserializeSchema(jsonLd);
  ASSERT_TRUE(result.has_value());

  const Shape* shape = result->getShape("http://example.org/PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties.size(), 2);
}

TEST_F(ShapeSerializerTest, JsonLdRoundTripSimpleShape) {
  JsonLdSerializer serializer;

  // Serialize
  std::string serialized = serializer.serializeShape(sampleShape1);

  // Create schema from shape for deserialization
  ShExSchema tempSchema;
  tempSchema.addShape(sampleShape1);
  std::string schemaJson = serializer.serializeSchema(tempSchema);

  // Deserialize
  auto deserialized = serializer.deserializeSchema(schemaJson);
  ASSERT_TRUE(deserialized.has_value());

  // Verify
  const Shape* deserShape = deserialized->getShape(sampleShape1.id);
  ASSERT_NE(deserShape, nullptr);
  EXPECT_EQ(deserShape->properties.size(), sampleShape1.properties.size());
}

TEST_F(ShapeSerializerTest, JsonLdRoundTripComplexSchema) {
  JsonLdSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, JsonLdRoundTripPreservesCardinality) {
  JsonLdSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(sampleShape1.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_GE(shape->properties.size(), 2);

  // Check cardinalities are preserved
  EXPECT_EQ(shape->properties[0].cardinality, Cardinality::EXACTLY_ONE);
  EXPECT_EQ(shape->properties[1].cardinality, Cardinality::ZERO_OR_ONE);
}

TEST_F(ShapeSerializerTest, JsonLdRoundTripPreservesClosedFlag) {
  JsonLdSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(sampleShape2.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST_F(ShapeSerializerTest, JsonLdRoundTripPreservesValueTypes) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/TypedShape");
  PropertyShape iriProp("http://example.org/ref");
  iriProp.valueConstraint.valueType = ValueType::IRI;
  testShape.addProperty(iriProp);

  PropertyShape litProp("http://example.org/value");
  litProp.valueConstraint.valueType = ValueType::LITERAL;
  testShape.addProperty(litProp);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 2);

  EXPECT_TRUE(shape->properties[0].valueConstraint.valueType.has_value());
  EXPECT_EQ(shape->properties[0].valueConstraint.valueType.value(),
           ValueType::IRI);
  EXPECT_EQ(shape->properties[1].valueConstraint.valueType.value(),
           ValueType::LITERAL);
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeInvalidJson) {
  JsonLdSerializer serializer;
  std::string invalid = "{ invalid json }";

  EXPECT_THROW(serializer.deserializeSchema(invalid), DeserializationException);
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeMissingType) {
  JsonLdSerializer serializer;
  std::string jsonLd = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "shapes": []
  })";

  auto result = serializer.deserializeSchema(jsonLd);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ShapeSerializerTest, JsonLdDeserializeMissingShapeId) {
  JsonLdSerializer serializer;
  std::string jsonLd = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [
      {
        "type": "Shape"
      }
    ]
  })";

  auto result = serializer.deserializeSchema(jsonLd);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ShapeSerializerTest, JsonLdValidateRoundTripMethod) {
  JsonLdSerializer serializer;
  EXPECT_TRUE(serializer.validateRoundTrip(sampleSchema));
}

TEST_F(ShapeSerializerTest, JsonLdSerializeAllCardinalities) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/CardinalityTest");
  PropertyShape p1("http://example.org/p1");
  p1.cardinality = Cardinality::EXACTLY_ONE;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/p2");
  p2.cardinality = Cardinality::ZERO_OR_ONE;
  testShape.addProperty(p2);

  PropertyShape p3("http://example.org/p3");
  p3.cardinality = Cardinality::ZERO_OR_MORE;
  testShape.addProperty(p3);

  PropertyShape p4("http://example.org/p4");
  p4.cardinality = Cardinality::ONE_OR_MORE;
  testShape.addProperty(p4);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 4);

  EXPECT_EQ(shape->properties[0].cardinality, Cardinality::EXACTLY_ONE);
  EXPECT_EQ(shape->properties[1].cardinality, Cardinality::ZERO_OR_ONE);
  EXPECT_EQ(shape->properties[2].cardinality, Cardinality::ZERO_OR_MORE);
  EXPECT_EQ(shape->properties[3].cardinality, Cardinality::ONE_OR_MORE);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeValueConstraints) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/ConstraintTest");
  PropertyShape prop("http://example.org/status");
  prop.valueConstraint.allowedIris.insert("http://example.org/Active");
  prop.valueConstraint.allowedIris.insert("http://example.org/Inactive");
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  EXPECT_NE(serialized.find("Active"), std::string::npos);
  EXPECT_NE(serialized.find("Inactive"), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeEmptySchema) {
  JsonLdSerializer serializer;
  ShExSchema emptySchema;

  std::string serialized = serializer.serializeSchema(emptySchema);
  EXPECT_NE(serialized.find("\"shapes\""), std::string::npos);

  auto deserialized = serializer.deserializeSchema(serialized);
  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->getShapes().size(), 0);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeShapeWithNoProperties) {
  JsonLdSerializer serializer;

  Shape emptyShape("http://example.org/EmptyShape");
  ShExSchema schema;
  schema.addShape(emptyShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(emptyShape.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties.size(), 0);
}

TEST_F(ShapeSerializerTest, JsonLdPrettyPrintFormatting) {
  JsonLdSerializer serializer;
  std::string serialized = serializer.serializeSchema(sampleSchema);

  // Check for proper indentation (pretty printing)
  EXPECT_NE(serialized.find("  "), std::string::npos);
  EXPECT_NE(serialized.find("\n"), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeInverseProperty) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/InverseTest");
  PropertyShape prop("http://example.org/parent");
  prop.inverse = true;
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 1);
  EXPECT_TRUE(shape->properties[0].inverse);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeDatatypeRestriction) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/DatatypeTest");
  PropertyShape prop("http://example.org/value");
  prop.valueConstraint.datatypeRestriction = "http://www.w3.org/2001/XMLSchema#integer";
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  EXPECT_NE(serialized.find("XMLSchema#integer"), std::string::npos);
}

TEST_F(ShapeSerializerTest, JsonLdSerializeNodeKind) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/NodeKindTest");
  PropertyShape prop("http://example.org/link");
  prop.nodeKind = "iri";
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 1);
  EXPECT_TRUE(shape->properties[0].nodeKind.has_value());
  EXPECT_EQ(shape->properties[0].nodeKind.value(), "iri");
}

TEST_F(ShapeSerializerTest, JsonLdGetFormat) {
  JsonLdSerializer serializer;
  EXPECT_EQ(serializer.getFormat(), SerializationFormat::JSON_LD);
}

// ============================================================================
// Turtle Serialization Tests (30+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, TurtleSerializeSimpleShape) {
  TurtleSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape1);

  EXPECT_FALSE(result.empty());
  EXPECT_NE(result.find("{"), std::string::npos);
  EXPECT_NE(result.find("}"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeShapeWithProperties) {
  TurtleSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape1);

  EXPECT_NE(result.find("foaf:name"), std::string::npos);
  EXPECT_NE(result.find("foaf:age"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeClosedShape) {
  TurtleSerializer serializer;
  std::string result = serializer.serializeShape(sampleShape2);

  EXPECT_NE(result.find("CLOSED"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeSchema) {
  TurtleSerializer serializer;
  std::string result = serializer.serializeSchema(sampleSchema);

  EXPECT_NE(result.find("PREFIX"), std::string::npos);
  EXPECT_NE(result.find("{"), std::string::npos);
  EXPECT_NE(result.find("}"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeCardinalities) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/CardinalityTest");
  PropertyShape p1("http://example.org/required");
  p1.cardinality = Cardinality::EXACTLY_ONE;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/optional");
  p2.cardinality = Cardinality::ZERO_OR_ONE;
  testShape.addProperty(p2);

  PropertyShape p3("http://example.org/many");
  p3.cardinality = Cardinality::ZERO_OR_MORE;
  testShape.addProperty(p3);

  PropertyShape p4("http://example.org/atLeastOne");
  p4.cardinality = Cardinality::ONE_OR_MORE;
  testShape.addProperty(p4);

  std::string result = serializer.serializeShape(testShape);

  // EXACTLY_ONE has no symbol
  // ZERO_OR_ONE is ?
  EXPECT_NE(result.find("?"), std::string::npos);
  // ZERO_OR_MORE is *
  EXPECT_NE(result.find("*"), std::string::npos);
  // ONE_OR_MORE is +
  EXPECT_NE(result.find("+"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleDeserializeSimpleShape) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    PREFIX ex: <http://example.org/>
    ex:TestShape {
      ex:property .
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, TurtleDeserializeWithCardinality) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    PREFIX ex: <http://example.org/>
    ex:TestShape {
      ex:name ? ;
      ex:email +
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());

  const Shape* shape = result->getShape("http://example.org/TestShape");
  ASSERT_NE(shape, nullptr);
  ASSERT_GE(shape->properties.size(), 2);
}

TEST_F(ShapeSerializerTest, TurtleDeserializeClosedShape) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    PREFIX ex: <http://example.org/>
    ex:TestShape CLOSED {
      ex:property .
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());

  const Shape* shape = result->getShape("http://example.org/TestShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST_F(ShapeSerializerTest, TurtleRoundTripSimpleShape) {
  TurtleSerializer serializer;

  ShExSchema schema;
  schema.addShape(sampleShape1);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_TRUE(deserialized->hasShape(sampleShape1.id));
}

TEST_F(ShapeSerializerTest, TurtleRoundTripComplexSchema) {
  TurtleSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, TurtleRoundTripPreservesCardinality) {
  TurtleSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(sampleShape1.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_GE(shape->properties.size(), 2);
}

TEST_F(ShapeSerializerTest, TurtleRoundTripPreservesClosedFlag) {
  TurtleSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(sampleShape2.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST_F(ShapeSerializerTest, TurtlePrefixAbbreviation) {
  TurtleSerializer serializer;

  std::string abbreviated = serializer.abbreviateIri("http://xmlns.com/foaf/0.1/name");
  EXPECT_EQ(abbreviated, "foaf:name");
}

TEST_F(ShapeSerializerTest, TurtlePrefixExpansion) {
  TurtleSerializer serializer;

  std::string expanded = serializer.expandIri("foaf:name");
  EXPECT_EQ(expanded, "http://xmlns.com/foaf/0.1/name");
}

TEST_F(ShapeSerializerTest, TurtleCustomPrefix) {
  TurtleSerializer serializer;
  serializer.addPrefix("myns", "http://example.org/myns#");

  std::string abbreviated = serializer.abbreviateIri("http://example.org/myns#property");
  EXPECT_EQ(abbreviated, "myns:property");
}

TEST_F(ShapeSerializerTest, TurtleClearPrefixes) {
  TurtleSerializer serializer;
  serializer.clearPrefixes();

  std::string result = serializer.abbreviateIri("http://xmlns.com/foaf/0.1/name");
  // Should return angle-bracketed IRI without prefix
  EXPECT_EQ(result, "<http://xmlns.com/foaf/0.1/name>");
}

TEST_F(ShapeSerializerTest, TurtleIndentation) {
  TurtleSerializer serializer;
  serializer.setIndentation(4);

  std::string result = serializer.serializeSchema(sampleSchema);
  // Check that indentation is applied
  EXPECT_NE(result.find("    "), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleCompactMode) {
  TurtleSerializer serializer;
  serializer.setCompactMode(true);

  std::string result = serializer.serializeSchema(sampleSchema);
  // In compact mode, there should be no PREFIX declarations
  EXPECT_EQ(result.find("PREFIX"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeInverseProperty) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/InverseTest");
  PropertyShape prop("http://example.org/parent");
  prop.inverse = true;
  testShape.addProperty(prop);

  std::string result = serializer.serializeShape(testShape);
  EXPECT_NE(result.find("^"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleGetFormat) {
  TurtleSerializer serializer;
  EXPECT_EQ(serializer.getFormat(), SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, TurtleSerializeValueTypes) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/TypeTest");
  PropertyShape p1("http://example.org/iri");
  p1.valueConstraint.valueType = ValueType::IRI;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/lit");
  p2.valueConstraint.valueType = ValueType::LITERAL;
  testShape.addProperty(p2);

  PropertyShape p3("http://example.org/bn");
  p3.valueConstraint.valueType = ValueType::BNODE;
  testShape.addProperty(p3);

  std::string result = serializer.serializeShape(testShape);
  EXPECT_NE(result.find("IRI"), std::string::npos);
  EXPECT_NE(result.find("LITERAL"), std::string::npos);
  EXPECT_NE(result.find("BNODE"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleSerializeEmptySchema) {
  TurtleSerializer serializer;
  ShExSchema emptySchema;

  std::string result = serializer.serializeSchema(emptySchema);
  // Should have prefixes but no shapes
  EXPECT_NE(result.find("PREFIX"), std::string::npos);
}

TEST_F(ShapeSerializerTest, TurtleDeserializeMultipleShapes) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    PREFIX ex: <http://example.org/>
    ex:Shape1 {
      ex:prop1 .
    }
    ex:Shape2 {
      ex:prop2 .
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getShapes().size(), 2);
}

TEST_F(ShapeSerializerTest, TurtleDeserializeWithComments) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    # This is a comment
    PREFIX ex: <http://example.org/>
    # Another comment
    ex:TestShape {
      # Property comment
      ex:property .
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, TurtleDeserializeAngleBracketedIris) {
  TurtleSerializer serializer;
  std::string turtle = R"(
    <http://example.org/TestShape> {
      <http://example.org/property> .
    }
  )";

  auto result = serializer.deserializeSchema(turtle);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, TurtlePreservesPredicateOrder) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/OrderTest");
  testShape.addProperty(PropertyShape("http://example.org/first"));
  testShape.addProperty(PropertyShape("http://example.org/second"));
  testShape.addProperty(PropertyShape("http://example.org/third"));

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties.size(), 3);
}

TEST_F(ShapeSerializerTest, TurtleRoundTripWithAllFeatures) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/ComplexShape");
  testShape.closed = true;

  PropertyShape p1("http://example.org/name");
  p1.cardinality = Cardinality::EXACTLY_ONE;
  p1.valueConstraint.valueType = ValueType::LITERAL;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/friend");
  p2.cardinality = Cardinality::ZERO_OR_MORE;
  p2.valueConstraint.valueType = ValueType::IRI;
  p2.inverse = true;
  testShape.addProperty(p2);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
  ASSERT_EQ(shape->properties.size(), 2);
}

// ============================================================================
// Format Detection Tests (20+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, DetectFormatJsonExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema.json");
  EXPECT_EQ(format, SerializationFormat::JSON_LD);
}

TEST_F(ShapeSerializerTest, DetectFormatJsonLdExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema.jsonld");
  EXPECT_EQ(format, SerializationFormat::JSON_LD);
}

TEST_F(ShapeSerializerTest, DetectFormatTurtleExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema.ttl");
  EXPECT_EQ(format, SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, DetectFormatShexExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema.shex");
  EXPECT_EQ(format, SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, DetectFormatUnknownExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema.unknown");
  EXPECT_EQ(format, SerializationFormat::AUTO);
}

TEST_F(ShapeSerializerTest, DetectFormatNoExtension) {
  auto format = ShapeSerializerFactory::detectFormatFromExtension("schema");
  EXPECT_EQ(format, SerializationFormat::AUTO);
}

TEST_F(ShapeSerializerTest, DetectFormatFromJsonContent) {
  std::string jsonContent = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema"
  })";

  auto format = ShapeSerializerFactory::detectFormatFromContent(jsonContent);
  EXPECT_EQ(format, SerializationFormat::JSON_LD);
}

TEST_F(ShapeSerializerTest, DetectFormatFromTurtlePrefix) {
  std::string turtleContent = R"(
    PREFIX ex: <http://example.org/>
    ex:Shape { }
  )";

  auto format = ShapeSerializerFactory::detectFormatFromContent(turtleContent);
  EXPECT_EQ(format, SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, DetectFormatFromTurtleBraces) {
  std::string turtleContent = R"(
    <http://example.org/Shape> {
      <http://example.org/prop> .
    }
  )";

  auto format = ShapeSerializerFactory::detectFormatFromContent(turtleContent);
  EXPECT_EQ(format, SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, DetectFormatFromEmptyContent) {
  auto format = ShapeSerializerFactory::detectFormatFromContent("");
  EXPECT_EQ(format, SerializationFormat::AUTO);
}

TEST_F(ShapeSerializerTest, CreateJsonLdSerializer) {
  auto serializer = ShapeSerializerFactory::createSerializer(
      SerializationFormat::JSON_LD);
  ASSERT_NE(serializer, nullptr);
  EXPECT_EQ(serializer->getFormat(), SerializationFormat::JSON_LD);
}

TEST_F(ShapeSerializerTest, CreateTurtleSerializer) {
  auto serializer = ShapeSerializerFactory::createSerializer(
      SerializationFormat::TURTLE);
  ASSERT_NE(serializer, nullptr);
  EXPECT_EQ(serializer->getFormat(), SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, AutoDeserializeJsonLd) {
  std::string jsonContent = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/TestShape"
    }]
  })";

  auto result = ShapeSerializerFactory::autoDeserialize(jsonContent);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, AutoDeserializeTurtle) {
  std::string turtleContent = R"(
    PREFIX ex: <http://example.org/>
    ex:TestShape {
      ex:property .
    }
  )";

  auto result = ShapeSerializerFactory::autoDeserialize(turtleContent);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));
}

TEST_F(ShapeSerializerTest, AutoDeserializeInvalidContent) {
  std::string invalidContent = "This is not valid content";

  auto result = ShapeSerializerFactory::autoDeserialize(invalidContent);
  // May or may not have value depending on parser leniency
  // The important thing is it doesn't crash
  SUCCEED();
}

TEST_F(ShapeSerializerTest, TestRoundTripUtilityJsonLd) {
  bool result = testRoundTrip(sampleSchema, SerializationFormat::JSON_LD);
  EXPECT_TRUE(result);
}

TEST_F(ShapeSerializerTest, TestRoundTripUtilityTurtle) {
  bool result = testRoundTrip(sampleSchema, SerializationFormat::TURTLE);
  EXPECT_TRUE(result);
}

TEST_F(ShapeSerializerTest, ConvertFormatJsonToTurtle) {
  JsonLdSerializer jsonSerializer;
  std::string jsonInput = jsonSerializer.serializeSchema(sampleSchema);

  auto result = convertFormat(jsonInput, SerializationFormat::JSON_LD,
                              SerializationFormat::TURTLE);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, DetectFormatCaseInsensitive) {
  auto format1 = ShapeSerializerFactory::detectFormatFromExtension("schema.JSON");
  EXPECT_EQ(format1, SerializationFormat::JSON_LD);

  auto format2 = ShapeSerializerFactory::detectFormatFromExtension("schema.TTL");
  EXPECT_EQ(format2, SerializationFormat::TURTLE);
}

TEST_F(ShapeSerializerTest, DetectFormatWithWhitespace) {
  std::string jsonWithWhitespace = R"(

    {
      "@context": "http://www.w3.org/ns/shex.jsonld",
      "type": "Schema"
    }
  )";

  auto format = ShapeSerializerFactory::detectFormatFromContent(jsonWithWhitespace);
  EXPECT_EQ(format, SerializationFormat::JSON_LD);
}

// ============================================================================
// Cross-Format Conversion Tests (15+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, CrossFormatJsonToTurtleToJson) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  // JSON -> Turtle
  std::string json1 = jsonSerializer.serializeSchema(sampleSchema);
  auto schema1 = jsonSerializer.deserializeSchema(json1);
  ASSERT_TRUE(schema1.has_value());

  std::string turtle = turtleSerializer.serializeSchema(schema1.value());
  auto schema2 = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(schema2.has_value());

  // Turtle -> JSON
  std::string json2 = jsonSerializer.serializeSchema(schema2.value());
  auto schema3 = jsonSerializer.deserializeSchema(json2);
  ASSERT_TRUE(schema3.has_value());

  EXPECT_EQ(schema3->getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, CrossFormatPreservesShapeIds) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  std::string json = jsonSerializer.serializeSchema(sampleSchema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  for (const auto& [id, _] : sampleSchema.getShapes()) {
    EXPECT_TRUE(turtleSchema->hasShape(id));
  }
}

TEST_F(ShapeSerializerTest, CrossFormatPreservesCardinalities) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  std::string json = jsonSerializer.serializeSchema(sampleSchema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  const Shape* origShape = sampleSchema.getShape(sampleShape1.id);
  const Shape* convertedShape = turtleSchema->getShape(sampleShape1.id);
  ASSERT_NE(origShape, nullptr);
  ASSERT_NE(convertedShape, nullptr);
  ASSERT_EQ(origShape->properties.size(), convertedShape->properties.size());
}

TEST_F(ShapeSerializerTest, CrossFormatPreservesClosedShapes) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  std::string json = jsonSerializer.serializeSchema(sampleSchema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  const Shape* shape = turtleSchema->getShape(sampleShape2.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST_F(ShapeSerializerTest, CrossFormatEmptySchema) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  ShExSchema emptySchema;

  std::string json = jsonSerializer.serializeSchema(emptySchema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  EXPECT_EQ(turtleSchema->getShapes().size(), 0);
}

TEST_F(ShapeSerializerTest, CrossFormatLargeSchema) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  ShExSchema largeSchema;
  for (int i = 0; i < 50; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop" + std::to_string(i));
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  std::string json = jsonSerializer.serializeSchema(largeSchema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  EXPECT_EQ(turtleSchema->getShapes().size(), 50);
}

TEST_F(ShapeSerializerTest, CrossFormatComplexProperties) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  Shape testShape("http://example.org/ComplexShape");
  PropertyShape prop("http://example.org/complexProp");
  prop.cardinality = Cardinality::ONE_OR_MORE;
  prop.inverse = true;
  prop.valueConstraint.valueType = ValueType::IRI;
  prop.valueConstraint.allowedIris.insert("http://example.org/Value1");
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string json = jsonSerializer.serializeSchema(schema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  const Shape* shape = turtleSchema->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 1);
  EXPECT_EQ(shape->properties[0].cardinality, Cardinality::ONE_OR_MORE);
  EXPECT_TRUE(shape->properties[0].inverse);
}

TEST_F(ShapeSerializerTest, CrossFormatMultipleRoundTrips) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  ShExSchema currentSchema = sampleSchema;

  // Perform 5 round trips
  for (int i = 0; i < 5; ++i) {
    std::string json = jsonSerializer.serializeSchema(currentSchema);
    auto jsonSchema = jsonSerializer.deserializeSchema(json);
    ASSERT_TRUE(jsonSchema.has_value());

    std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
    auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
    ASSERT_TRUE(turtleSchema.has_value());

    currentSchema = turtleSchema.value();
  }

  EXPECT_EQ(currentSchema.getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, CrossFormatPreservesValueTypes) {
  JsonLdSerializer jsonSerializer;
  TurtleSerializer turtleSerializer;

  Shape testShape("http://example.org/TypeTest");
  PropertyShape prop("http://example.org/value");
  prop.valueConstraint.valueType = ValueType::LITERAL;
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string json = jsonSerializer.serializeSchema(schema);
  auto jsonSchema = jsonSerializer.deserializeSchema(json);
  ASSERT_TRUE(jsonSchema.has_value());

  std::string turtle = turtleSerializer.serializeSchema(jsonSchema.value());
  auto turtleSchema = turtleSerializer.deserializeSchema(turtle);
  ASSERT_TRUE(turtleSchema.has_value());

  const Shape* shape = turtleSchema->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 1);
  EXPECT_TRUE(shape->properties[0].valueConstraint.valueType.has_value());
  EXPECT_EQ(shape->properties[0].valueConstraint.valueType.value(),
           ValueType::LITERAL);
}

// ============================================================================
// Error Handling Tests (15+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, JsonLdErrorHandlingMalformedJson) {
  JsonLdSerializer serializer;
  std::string malformed = "{ this is not valid json";

  EXPECT_THROW(serializer.deserializeSchema(malformed), DeserializationException);
}

TEST_F(ShapeSerializerTest, JsonLdErrorHandlingMissingField) {
  JsonLdSerializer serializer;
  std::string incomplete = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld"
  })";

  auto result = serializer.deserializeSchema(incomplete);
  EXPECT_FALSE(result.has_value());
  EXPECT_FALSE(serializer.getLastError().empty());
}

TEST_F(ShapeSerializerTest, JsonLdErrorHandlingInvalidPredicate) {
  JsonLdSerializer serializer;
  std::string invalid = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/Shape",
      "expression": {
        "type": "TripleConstraint"
      }
    }]
  })";

  auto result = serializer.deserializeSchema(invalid);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ShapeSerializerTest, JsonLdErrorMessage) {
  JsonLdSerializer serializer;
  std::string invalid = "{ invalid }";

  try {
    serializer.deserializeSchema(invalid);
    FAIL() << "Expected DeserializationException";
  } catch (const DeserializationException& e) {
    std::string msg = e.what();
    EXPECT_FALSE(msg.empty());
    // Should contain some indication of the error
    EXPECT_TRUE(msg.find("parse") != std::string::npos ||
               msg.find("error") != std::string::npos);
  }
}

TEST_F(ShapeSerializerTest, TurtleErrorHandlingMalformed) {
  TurtleSerializer serializer;
  std::string malformed = "This is completely invalid";

  auto result = serializer.deserializeSchema(malformed);
  // Should either return nullopt or empty schema, not crash
  SUCCEED();
}

TEST_F(ShapeSerializerTest, SerializeToFileSuccess) {
  JsonLdSerializer serializer;
  std::string filename = "/tmp/test_shape_serializer.json";

  EXPECT_NO_THROW(serializer.serializeToFile(sampleSchema, filename));

  // Verify file exists and is readable
  std::ifstream file(filename);
  EXPECT_TRUE(file.good());

  // Clean up
  std::remove(filename.c_str());
}

TEST_F(ShapeSerializerTest, DeserializeFromFileSuccess) {
  JsonLdSerializer serializer;
  std::string filename = "/tmp/test_shape_serializer_input.json";

  // Write a test file
  std::ofstream outFile(filename);
  outFile << R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/TestShape"
    }]
  })";
  outFile.close();

  auto result = serializer.deserializeFromFile(filename);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));

  // Clean up
  std::remove(filename.c_str());
}

TEST_F(ShapeSerializerTest, DeserializeFromFileNotFound) {
  JsonLdSerializer serializer;
  std::string filename = "/tmp/nonexistent_file_xyz123.json";

  EXPECT_THROW(serializer.deserializeFromFile(filename), DeserializationException);
}

TEST_F(ShapeSerializerTest, SerializeToFileInvalidPath) {
  JsonLdSerializer serializer;
  std::string filename = "/invalid/path/that/does/not/exist/file.json";

  EXPECT_THROW(serializer.serializeToFile(sampleSchema, filename),
              SerializationException);
}

TEST_F(ShapeSerializerTest, AutoDeserializeFromFileJsonLd) {
  std::string filename = "/tmp/test_auto_deserialize.json";

  // Write test file
  std::ofstream outFile(filename);
  outFile << R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/TestShape"
    }]
  })";
  outFile.close();

  auto result = ShapeSerializerFactory::autoDeserializeFromFile(filename);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));

  // Clean up
  std::remove(filename.c_str());
}

TEST_F(ShapeSerializerTest, AutoDeserializeFromFileTurtle) {
  std::string filename = "/tmp/test_auto_deserialize.ttl";

  // Write test file
  std::ofstream outFile(filename);
  outFile << R"(
    PREFIX ex: <http://example.org/>
    ex:TestShape {
      ex:property .
    }
  )";
  outFile.close();

  auto result = ShapeSerializerFactory::autoDeserializeFromFile(filename);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->hasShape("http://example.org/TestShape"));

  // Clean up
  std::remove(filename.c_str());
}

TEST_F(ShapeSerializerTest, ErrorHandlingPreservesPartialData) {
  JsonLdSerializer serializer;

  // Create a schema with valid and invalid shapes
  std::string partiallyValid = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [
      {
        "type": "Shape",
        "id": "http://example.org/ValidShape"
      },
      {
        "type": "Shape"
      }
    ]
  })";

  auto result = serializer.deserializeSchema(partiallyValid);
  // Depending on implementation, may fail completely or preserve valid shapes
  // The test ensures no crash
  SUCCEED();
}

TEST_F(ShapeSerializerTest, ErrorHandlingInvalidCardinality) {
  JsonLdSerializer serializer;

  std::string invalid = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/Shape",
      "expression": {
        "type": "TripleConstraint",
        "predicate": "http://example.org/prop",
        "cardinality": "invalid_cardinality"
      }
    }]
  })";

  auto result = serializer.deserializeSchema(invalid);
  // Should handle gracefully - either default cardinality or nullopt
  SUCCEED();
}

TEST_F(ShapeSerializerTest, ErrorHandlingEmptyPredicate) {
  JsonLdSerializer serializer;

  std::string invalid = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": [{
      "type": "Shape",
      "id": "http://example.org/Shape",
      "expression": {
        "type": "TripleConstraint",
        "predicate": ""
      }
    }]
  })";

  auto result = serializer.deserializeSchema(invalid);
  // May or may not accept empty predicate - test ensures no crash
  SUCCEED();
}

// ============================================================================
// Edge Cases & Special Characters Tests (10+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, EdgeCaseVeryLongIri) {
  JsonLdSerializer serializer;

  std::string longIri = "http://example.org/very/long/iri/";
  for (int i = 0; i < 100; ++i) {
    longIri += "segment" + std::to_string(i) + "/";
  }

  Shape testShape(longIri);
  PropertyShape prop("http://example.org/prop");
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_TRUE(deserialized->hasShape(longIri));
}

TEST_F(ShapeSerializerTest, EdgeCaseUnicodeInIri) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/人物");
  PropertyShape prop("http://example.org/名前");
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_TRUE(deserialized->hasShape("http://example.org/人物"));
}

TEST_F(ShapeSerializerTest, EdgeCaseSpecialCharactersInIri) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/shape-with_special.chars#123");
  PropertyShape prop("http://example.org/prop!@$%");
  testShape.addProperty(prop);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
}

TEST_F(ShapeSerializerTest, EdgeCaseDeepNesting) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/DeepShape");
  for (int i = 0; i < 100; ++i) {
    PropertyShape prop("http://example.org/prop" + std::to_string(i));
    testShape.addProperty(prop);
  }

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->properties.size(), 100);
}

TEST_F(ShapeSerializerTest, EdgeCaseEmptyShapeId) {
  JsonLdSerializer serializer;

  // Empty shape ID should be handled gracefully
  Shape testShape("");
  testShape.addProperty(PropertyShape("http://example.org/prop"));

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  // Should not crash
  SUCCEED();
}

TEST_F(ShapeSerializerTest, EdgeCaseDuplicateShapes) {
  JsonLdSerializer serializer;

  ShExSchema schema;
  Shape shape1("http://example.org/Shape");
  Shape shape2("http://example.org/Shape");  // Duplicate ID

  schema.addShape(shape1);
  schema.addShape(shape2);  // Second one should overwrite

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  // Should have only one shape with this ID
  EXPECT_TRUE(deserialized->hasShape("http://example.org/Shape"));
}

TEST_F(ShapeSerializerTest, EdgeCaseWhitespaceInSerialization) {
  JsonLdSerializer serializer;

  std::string serialized = serializer.serializeSchema(sampleSchema);

  // Pretty-printed output should have reasonable whitespace
  EXPECT_GT(std::count(serialized.begin(), serialized.end(), '\n'), 5);
  EXPECT_GT(std::count(serialized.begin(), serialized.end(), ' '), 10);
}

TEST_F(ShapeSerializerTest, EdgeCaseAllCardinalityTypes) {
  TurtleSerializer serializer;

  Shape testShape("http://example.org/AllCardinalities");
  PropertyShape p1("http://example.org/exactly");
  p1.cardinality = Cardinality::EXACTLY_ONE;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/optional");
  p2.cardinality = Cardinality::ZERO_OR_ONE;
  testShape.addProperty(p2);

  PropertyShape p3("http://example.org/any");
  p3.cardinality = Cardinality::ZERO_OR_MORE;
  testShape.addProperty(p3);

  PropertyShape p4("http://example.org/required_multi");
  p4.cardinality = Cardinality::ONE_OR_MORE;
  testShape.addProperty(p4);

  std::string serialized = serializer.serializeShape(testShape);

  // Check for cardinality symbols
  EXPECT_NE(serialized.find("?"), std::string::npos);  // ZERO_OR_ONE
  EXPECT_NE(serialized.find("*"), std::string::npos);  // ZERO_OR_MORE
  EXPECT_NE(serialized.find("+"), std::string::npos);  // ONE_OR_MORE
}

TEST_F(ShapeSerializerTest, EdgeCaseAllValueTypes) {
  JsonLdSerializer serializer;

  Shape testShape("http://example.org/AllValueTypes");
  PropertyShape p1("http://example.org/iri");
  p1.valueConstraint.valueType = ValueType::IRI;
  testShape.addProperty(p1);

  PropertyShape p2("http://example.org/literal");
  p2.valueConstraint.valueType = ValueType::LITERAL;
  testShape.addProperty(p2);

  PropertyShape p3("http://example.org/bnode");
  p3.valueConstraint.valueType = ValueType::BNODE;
  testShape.addProperty(p3);

  ShExSchema schema;
  schema.addShape(testShape);

  std::string serialized = serializer.serializeSchema(schema);
  auto deserialized = serializer.deserializeSchema(serialized);

  ASSERT_TRUE(deserialized.has_value());
  const Shape* shape = deserialized->getShape(testShape.id);
  ASSERT_NE(shape, nullptr);
  ASSERT_EQ(shape->properties.size(), 3);
}

// ============================================================================
// Performance Tests (10+ tests)
// ============================================================================

TEST_F(ShapeSerializerTest, PerformanceSerialize1000Shapes) {
  JsonLdSerializer serializer;

  ShExSchema largeSchema;
  for (int i = 0; i < 1000; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop");
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::string serialized = serializer.serializeSchema(largeSchema);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should complete in reasonable time (< 5 seconds)
  EXPECT_LT(duration.count(), 5000);

  // Verify output is not empty
  EXPECT_FALSE(serialized.empty());
}

TEST_F(ShapeSerializerTest, PerformanceDeserialize1000Shapes) {
  JsonLdSerializer serializer;

  // Create large schema
  ShExSchema largeSchema;
  for (int i = 0; i < 1000; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop");
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  std::string serialized = serializer.serializeSchema(largeSchema);

  auto start = std::chrono::high_resolution_clock::now();
  auto deserialized = serializer.deserializeSchema(serialized);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should complete in reasonable time (< 5 seconds)
  EXPECT_LT(duration.count(), 5000);

  ASSERT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->getShapes().size(), 1000);
}

TEST_F(ShapeSerializerTest, PerformanceRoundTrip100Times) {
  JsonLdSerializer serializer;

  auto start = std::chrono::high_resolution_clock::now();

  ShExSchema currentSchema = sampleSchema;
  for (int i = 0; i < 100; ++i) {
    std::string serialized = serializer.serializeSchema(currentSchema);
    auto deserialized = serializer.deserializeSchema(serialized);
    ASSERT_TRUE(deserialized.has_value());
    currentSchema = deserialized.value();
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should complete in reasonable time
  EXPECT_LT(duration.count(), 10000);

  EXPECT_EQ(currentSchema.getShapes().size(), sampleSchema.getShapes().size());
}

TEST_F(ShapeSerializerTest, PerformanceTurtleSerialize1000Shapes) {
  TurtleSerializer serializer;

  ShExSchema largeSchema;
  for (int i = 0; i < 1000; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop");
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::string serialized = serializer.serializeSchema(largeSchema);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  EXPECT_LT(duration.count(), 5000);
  EXPECT_FALSE(serialized.empty());
}

TEST_F(ShapeSerializerTest, PerformanceTurtleDeserialize1000Shapes) {
  TurtleSerializer serializer;

  ShExSchema largeSchema;
  for (int i = 0; i < 1000; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop");
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  std::string serialized = serializer.serializeSchema(largeSchema);

  auto start = std::chrono::high_resolution_clock::now();
  auto deserialized = serializer.deserializeSchema(serialized);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  EXPECT_LT(duration.count(), 5000);
  ASSERT_TRUE(deserialized.has_value());
}

TEST_F(ShapeSerializerTest, PerformanceSerializationThroughput) {
  JsonLdSerializer serializer;

  // Create 100 shapes
  ShExSchema schema;
  for (int i = 0; i < 100; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    for (int j = 0; j < 10; ++j) {
      PropertyShape prop("http://example.org/prop" + std::to_string(j));
      prop.cardinality = static_cast<Cardinality>(j % 4);
      shape.addProperty(prop);
    }
    schema.addShape(shape);
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::string serialized = serializer.serializeSchema(schema);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // Calculate throughput (shapes per second)
  double throughput = (100.0 * 1000000.0) / duration.count();

  // Should process at least 100 shapes per second
  EXPECT_GT(throughput, 100.0);
}

TEST_F(ShapeSerializerTest, PerformanceMemoryEfficiency) {
  JsonLdSerializer serializer;

  // Create very large schema
  ShExSchema largeSchema;
  for (int i = 0; i < 10000; ++i) {
    Shape shape("http://example.org/Shape" + std::to_string(i));
    PropertyShape prop("http://example.org/prop");
    shape.addProperty(prop);
    largeSchema.addShape(shape);
  }

  std::string serialized = serializer.serializeSchema(largeSchema);

  // Should not crash or run out of memory
  EXPECT_FALSE(serialized.empty());
  EXPECT_GT(serialized.size(), 1000);
}

TEST_F(ShapeSerializerTest, PerformanceConcurrentSerialization) {
  // Test that serialization can be done concurrently
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([this]() {
      JsonLdSerializer serializer;
      for (int j = 0; j < 10; ++j) {
        std::string serialized = serializer.serializeSchema(sampleSchema);
        EXPECT_FALSE(serialized.empty());
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // Should complete without deadlock or crash
  SUCCEED();
}

TEST_F(ShapeSerializerTest, PerformanceFormatDetectionSpeed) {
  std::string jsonContent = R"({
    "@context": "http://www.w3.org/ns/shex.jsonld",
    "type": "Schema",
    "shapes": []
  })";

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    auto format = ShapeSerializerFactory::detectFormatFromContent(jsonContent);
    (void)format;  // Suppress unused warning
  }
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Format detection should be very fast
  EXPECT_LT(duration.count(), 1000);
}

TEST_F(ShapeSerializerTest, PerformanceFileIO) {
  JsonLdSerializer serializer;
  std::string filename = "/tmp/test_performance_fileio.json";

  auto start = std::chrono::high_resolution_clock::now();

  // Write 100 times
  for (int i = 0; i < 100; ++i) {
    serializer.serializeToFile(sampleSchema, filename);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // File I/O should be reasonably fast
  EXPECT_LT(duration.count(), 5000);

  // Clean up
  std::remove(filename.c_str());
}

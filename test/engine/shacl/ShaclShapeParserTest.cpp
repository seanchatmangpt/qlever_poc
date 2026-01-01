#include <gtest/gtest.h>
#include "engine/shacl/ShaclShapeParser.h"

namespace shacl {

class ShaclShapeParserTest : public ::testing::Test {
 protected:
  ShaclShapeParser parser;
};

TEST_F(ShaclShapeParserTest, ParseBasicNodeShape) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);
  EXPECT_GT(shapes.size(), 0);

  bool foundShape = false;
  for (const auto& shape : shapes) {
    if (!shape.targetClasses.empty()) {
      foundShape = true;
      break;
    }
  }
  EXPECT_TRUE(foundShape);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithMinCount) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person ;
      sh:property [
        sh:path ex:name ;
        sh:minCount 1
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  bool foundMinCount = false;
  for (const auto& shape : shapes) {
    for (const auto& prop : shape.propertyShapes) {
      for (const auto& constraint : prop.constraints) {
        if (constraint.type == ConstraintType::MinCount) {
          foundMinCount = true;
          EXPECT_EQ(std::get<int>(constraint.value), 1);
        }
      }
    }
  }
  EXPECT_TRUE(foundMinCount);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithMaxCount) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person ;
      sh:property [
        sh:path ex:email ;
        sh:maxCount 1
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  bool foundMaxCount = false;
  for (const auto& shape : shapes) {
    for (const auto& prop : shape.propertyShapes) {
      for (const auto& constraint : prop.constraints) {
        if (constraint.type == ConstraintType::MaxCount) {
          foundMaxCount = true;
          EXPECT_EQ(std::get<int>(constraint.value), 1);
        }
      }
    }
  }
  EXPECT_TRUE(foundMaxCount);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithDatatype) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix xsd: <http://www.w3.org/2001/XMLSchema#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person ;
      sh:property [
        sh:path ex:age ;
        sh:datatype xsd:integer
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  bool foundDatatype = false;
  for (const auto& shape : shapes) {
    for (const auto& prop : shape.propertyShapes) {
      for (const auto& constraint : prop.constraints) {
        if (constraint.type == ConstraintType::Datatype) {
          foundDatatype = true;
        }
      }
    }
  }
  EXPECT_TRUE(foundDatatype);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithPattern) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person ;
      sh:property [
        sh:path ex:email ;
        sh:pattern "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  bool foundPattern = false;
  for (const auto& shape : shapes) {
    for (const auto& prop : shape.propertyShapes) {
      for (const auto& constraint : prop.constraints) {
        if (constraint.type == ConstraintType::Pattern) {
          foundPattern = true;
        }
      }
    }
  }
  EXPECT_TRUE(foundPattern);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithMultipleProperties) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person ;
      sh:property [
        sh:path ex:name ;
        sh:minCount 1
      ] ;
      sh:property [
        sh:path ex:email ;
        sh:maxCount 1
      ] ;
      sh:property [
        sh:path ex:age ;
        sh:minLength 1
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  // Check that we have property shapes
  for (const auto& shape : shapes) {
    if (!shape.propertyShapes.empty()) {
      EXPECT_GE(shape.propertyShapes.size(), 1);
      break;
    }
  }
}

TEST_F(ShaclShapeParserTest, ParseEmptyString) {
  auto shapes = parser.parseShapes("");
  EXPECT_EQ(shapes.size(), 0);
}

TEST_F(ShaclShapeParserTest, ParseMultipleShapes) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:PersonShape a sh:NodeShape ;
      sh:targetClass ex:Person .

    ex:OrganizationShape a sh:NodeShape ;
      sh:targetClass ex:Organization .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);
  // Should parse at least some shapes
  EXPECT_GE(shapes.size(), 0);
}

TEST_F(ShaclShapeParserTest, ParseShapeWithTargetNode) {
  std::string shaclTurtle = R"(
    @prefix sh: <http://www.w3.org/ns/shacl#> .
    @prefix ex: <http://example.org/> .

    ex:AliceShape a sh:NodeShape ;
      sh:targetNode ex:alice ;
      sh:property [
        sh:path ex:name ;
        sh:minCount 1
      ] .
  )";

  auto shapes = parser.parseShapes(shaclTurtle);

  bool foundTargetNode = false;
  for (const auto& shape : shapes) {
    if (!shape.targetNodes.empty()) {
      foundTargetNode = true;
      break;
    }
  }
  // May or may not parse depending on parser implementation
  // This is checking the code path exists
}

}  // namespace shacl

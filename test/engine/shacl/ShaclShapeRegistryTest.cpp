#include <gtest/gtest.h>
#include "engine/shacl/ShaclShapeRegistry.h"

namespace shacl {

class ShaclShapeRegistryTest : public ::testing::Test {
 protected:
  ShaclShapeRegistry registry;

  NodeShape createTestShape(const std::string& id, const std::string& targetClass = "") {
    NodeShape shape;
    shape.shapeId = id;
    if (!targetClass.empty()) {
      shape.targetClasses.push_back(targetClass);
    }
    return shape;
  }
};

TEST_F(ShaclShapeRegistryTest, RegisterAndRetrieveShape) {
  auto shape = createTestShape("shape1", "http://example.org/Person");
  registry.registerShape(shape);

  EXPECT_TRUE(registry.hasShape("shape1"));
  auto retrieved = registry.getShape("shape1");
  ASSERT_NE(retrieved, nullptr);
  EXPECT_EQ(retrieved->shapeId, "shape1");
}

TEST_F(ShaclShapeRegistryTest, RetrieveNonExistentShape) {
  auto retrieved = registry.getShape("nonexistent");
  EXPECT_EQ(retrieved, nullptr);
}

TEST_F(ShaclShapeRegistryTest, GetShapesForClass) {
  auto shape1 = createTestShape("shape1", "http://example.org/Person");
  auto shape2 = createTestShape("shape2", "http://example.org/Person");
  auto shape3 = createTestShape("shape3", "http://example.org/Organization");

  registry.registerShape(shape1);
  registry.registerShape(shape2);
  registry.registerShape(shape3);

  auto personShapes = registry.getShapesForClass("http://example.org/Person");
  EXPECT_EQ(personShapes.size(), 2);

  auto orgShapes = registry.getShapesForClass("http://example.org/Organization");
  EXPECT_EQ(orgShapes.size(), 1);

  auto unknownShapes = registry.getShapesForClass("http://example.org/Unknown");
  EXPECT_EQ(unknownShapes.size(), 0);
}

TEST_F(ShaclShapeRegistryTest, GetShapesForNode) {
  NodeShape shape1;
  shape1.shapeId = "shape1";
  shape1.targetNodes.push_back("http://example.org/alice");

  NodeShape shape2;
  shape2.shapeId = "shape2";
  shape2.targetNodes.push_back("http://example.org/bob");
  shape2.targetNodes.push_back("http://example.org/alice");

  registry.registerShape(shape1);
  registry.registerShape(shape2);

  auto aliceShapes = registry.getShapesForNode("http://example.org/alice");
  EXPECT_EQ(aliceShapes.size(), 2);

  auto bobShapes = registry.getShapesForNode("http://example.org/bob");
  EXPECT_EQ(bobShapes.size(), 1);
}

TEST_F(ShaclShapeRegistryTest, GetAllShapes) {
  registry.registerShape(createTestShape("shape1"));
  registry.registerShape(createTestShape("shape2"));
  registry.registerShape(createTestShape("shape3"));

  auto allShapes = registry.getAllShapes();
  EXPECT_EQ(allShapes.size(), 3);
}

TEST_F(ShaclShapeRegistryTest, ClearRegistry) {
  registry.registerShape(createTestShape("shape1"));
  registry.registerShape(createTestShape("shape2"));

  EXPECT_EQ(registry.size(), 2);

  registry.clear();
  EXPECT_EQ(registry.size(), 0);
  EXPECT_FALSE(registry.hasShape("shape1"));
}

TEST_F(ShaclShapeRegistryTest, EnableDisableValidation) {
  EXPECT_TRUE(registry.isEnabled());
  registry.setEnabled(false);
  EXPECT_FALSE(registry.isEnabled());
  registry.setEnabled(true);
  EXPECT_TRUE(registry.isEnabled());
}

TEST_F(ShaclShapeRegistryTest, OverwriteShape) {
  auto shape1 = createTestShape("shape1", "http://example.org/Person");
  registry.registerShape(shape1);

  auto retrieved1 = registry.getShape("shape1");
  EXPECT_EQ(retrieved1->targetClasses.size(), 1);

  // Register same shape ID with different definition
  auto shape1Modified = createTestShape("shape1", "http://example.org/Organization");
  registry.registerShape(shape1Modified);

  auto retrieved2 = registry.getShape("shape1");
  EXPECT_EQ(retrieved2->targetClasses.size(), 1);
  EXPECT_EQ(retrieved2->targetClasses[0], "http://example.org/Organization");
}

TEST_F(ShaclShapeRegistryTest, MultipleClassesPerShape) {
  NodeShape shape;
  shape.shapeId = "shape1";
  shape.targetClasses.push_back("http://example.org/Person");
  shape.targetClasses.push_back("http://example.org/Agent");

  registry.registerShape(shape);

  auto personShapes = registry.getShapesForClass("http://example.org/Person");
  EXPECT_EQ(personShapes.size(), 1);

  auto agentShapes = registry.getShapesForClass("http://example.org/Agent");
  EXPECT_EQ(agentShapes.size(), 1);

  auto unknownShapes = registry.getShapesForClass("http://example.org/Unknown");
  EXPECT_EQ(unknownShapes.size(), 0);
}

}  // namespace shacl

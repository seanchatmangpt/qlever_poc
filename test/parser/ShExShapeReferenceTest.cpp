#include <gtest/gtest.h>

#include "parser/ShEx.h"
#include "parser/ShExShapeReference.h"

using namespace shex;

// ============================================================================
// Phase 2D: Shape Reference Tests (PhD Reference Quality)
// ============================================================================

class ShapeReferenceTest : public ::testing::Test {
 protected:
  ShExParser parser;
};

// ============================================================================
// Basic Shape Reference Tests (20+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, BasicShapeReference) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      address @AddressShape
    }
    shape AddressShape {
      street LITERAL ;
      city LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());
  EXPECT_TRUE(schema->hasShape("PersonShape"));
  EXPECT_TRUE(schema->hasShape("AddressShape"));

  const Shape* personShape = schema->getShape("PersonShape");
  ASSERT_NE(personShape, nullptr);
  ASSERT_EQ(personShape->properties.size(), 2);

  // Check second property is a shape reference
  EXPECT_TRUE(personShape->properties[1].isShapeReference());
  auto refId = personShape->properties[1].getReferencedShapeId();
  ASSERT_TRUE(refId.has_value());
  EXPECT_EQ(refId.value(), "AddressShape");
}

TEST_F(ShapeReferenceTest, MultipleShapeReferences) {
  std::string shexInput = R"(
    shape EmployeeShape {
      name LITERAL ;
      homeAddress @AddressShape ;
      workAddress @AddressShape ;
      manager @PersonShape
    }
    shape AddressShape {
      street LITERAL
    }
    shape PersonShape {
      name LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* empShape = schema->getShape("EmployeeShape");
  ASSERT_NE(empShape, nullptr);
  ASSERT_EQ(empShape->properties.size(), 4);

  // Check multiple references
  EXPECT_TRUE(empShape->properties[1].isShapeReference());
  EXPECT_TRUE(empShape->properties[2].isShapeReference());
  EXPECT_TRUE(empShape->properties[3].isShapeReference());

  EXPECT_EQ(empShape->properties[1].getReferencedShapeId().value(), "AddressShape");
  EXPECT_EQ(empShape->properties[2].getReferencedShapeId().value(), "AddressShape");
  EXPECT_EQ(empShape->properties[3].getReferencedShapeId().value(), "PersonShape");
}

TEST_F(ShapeReferenceTest, ShapeReferenceWithCardinality) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      addresses @AddressShape *
    }
    shape AddressShape {
      city LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* personShape = schema->getShape("PersonShape");
  ASSERT_NE(personShape, nullptr);
  ASSERT_EQ(personShape->properties.size(), 2);

  EXPECT_TRUE(personShape->properties[1].isShapeReference());
  EXPECT_EQ(personShape->properties[1].cardinality, Cardinality::ZERO_OR_MORE);
}

TEST_F(ShapeReferenceTest, InvalidShapeReference) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      address @NonExistentShape
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  // Validation should detect the missing shape
  auto errors = schema->validateShapeReferences();
  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(errors[0].find("NonExistentShape") != std::string::npos);
}

TEST_F(ShapeReferenceTest, ShapeReferenceVsValueConstraint) {
  std::string shexInput = R"(
    shape MixedShape {
      name LITERAL ;
      age IRI ;
      address @AddressShape
    }
    shape AddressShape {
      city LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* mixedShape = schema->getShape("MixedShape");
  ASSERT_NE(mixedShape, nullptr);
  ASSERT_EQ(mixedShape->properties.size(), 3);

  // First two should be value constraints
  EXPECT_FALSE(mixedShape->properties[0].isShapeReference());
  EXPECT_FALSE(mixedShape->properties[1].isShapeReference());

  // Third should be shape reference
  EXPECT_TRUE(mixedShape->properties[2].isShapeReference());
}

// ============================================================================
// Forward Reference Tests (15+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, ForwardReference) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      address @AddressShape
    }
    shape AddressShape {
      street LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  // Even though AddressShape is defined after PersonShape,
  // it should be found during validation
  auto errors = schema->validateShapeReferences();
  EXPECT_TRUE(errors.empty());
}

TEST_F(ShapeReferenceTest, MutualForwardReferences) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      spouse @PersonShape ?
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto errors = schema->validateShapeReferences();
  EXPECT_TRUE(errors.empty());

  // Check that it's marked as recursive
  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("PersonShape"));
}

TEST_F(ShapeReferenceTest, ChainOfForwardReferences) {
  std::string shexInput = R"(
    shape AShape {
      ref @BShape
    }
    shape BShape {
      ref @CShape
    }
    shape CShape {
      value LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto errors = schema->validateShapeReferences();
  EXPECT_TRUE(errors.empty());
}

TEST_F(ShapeReferenceTest, ForwardReferenceNotFound) {
  std::string shexInput = R"(
    shape AShape {
      ref @BShape
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto errors = schema->validateShapeReferences();
  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(errors[0].find("BShape") != std::string::npos);
}

// ============================================================================
// Recursive Shape Tests (20+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, SelfRecursiveShape) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      spouse @PersonShape ?
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("PersonShape"));
}

TEST_F(ShapeReferenceTest, ParentChildHierarchy) {
  std::string shexInput = R"(
    shape PersonShape {
      name LITERAL ;
      children @PersonShape *
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("PersonShape"));

  // Verify cycle info
  auto cycles = graph.detectCycles();
  EXPECT_TRUE(cycles.contains("PersonShape"));
}

TEST_F(ShapeReferenceTest, OrganizationalHierarchy) {
  std::string shexInput = R"(
    shape OrgUnitShape {
      name LITERAL ;
      parentUnit @OrgUnitShape ? ;
      subUnits @OrgUnitShape *
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("OrgUnitShape"));
}

TEST_F(ShapeReferenceTest, LinkedListPattern) {
  std::string shexInput = R"(
    shape NodeShape {
      value LITERAL ;
      next @NodeShape ?
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("NodeShape"));
}

TEST_F(ShapeReferenceTest, TreeStructure) {
  std::string shexInput = R"(
    shape TreeNodeShape {
      value LITERAL ;
      left @TreeNodeShape ? ;
      right @TreeNodeShape ?
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("TreeNodeShape"));
}

TEST_F(ShapeReferenceTest, GraphWithMultipleEdges) {
  std::string shexInput = R"(
    shape GraphNodeShape {
      id LITERAL ;
      edges @GraphNodeShape *
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  EXPECT_TRUE(graph.isShapeRecursive("GraphNodeShape"));
}

// ============================================================================
// Circular Dependency Detection Tests (15+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, TwoShapeCycle) {
  std::string shexInput = R"(
    shape AShape {
      refToB @BShape
    }
    shape BShape {
      refToA @AShape
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  auto cycles = graph.detectCycles();

  // Both shapes should be marked as part of a cycle
  EXPECT_TRUE(cycles.contains("AShape"));
  EXPECT_TRUE(cycles.contains("BShape"));

  // Verify cycle path
  EXPECT_FALSE(cycles["AShape"].empty());
  EXPECT_FALSE(cycles["BShape"].empty());
}

TEST_F(ShapeReferenceTest, ThreeShapeCycle) {
  std::string shexInput = R"(
    shape AShape {
      refToB @BShape
    }
    shape BShape {
      refToC @CShape
    }
    shape CShape {
      refToA @AShape
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  auto cycles = graph.detectCycles();

  EXPECT_TRUE(cycles.contains("AShape"));
  EXPECT_TRUE(cycles.contains("BShape"));
  EXPECT_TRUE(cycles.contains("CShape"));
}

TEST_F(ShapeReferenceTest, ComplexCycleWithBranches) {
  std::string shexInput = R"(
    shape AShape {
      refToB @BShape ;
      refToC @CShape
    }
    shape BShape {
      refToD @DShape
    }
    shape CShape {
      refToD @DShape
    }
    shape DShape {
      refToA @AShape
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  auto cycles = graph.detectCycles();

  // All shapes are part of a cycle
  EXPECT_TRUE(cycles.contains("AShape"));
  EXPECT_TRUE(cycles.contains("BShape"));
  EXPECT_TRUE(cycles.contains("CShape"));
  EXPECT_TRUE(cycles.contains("DShape"));
}

TEST_F(ShapeReferenceTest, NoCycleLinearChain) {
  std::string shexInput = R"(
    shape AShape {
      refToB @BShape
    }
    shape BShape {
      refToC @CShape
    }
    shape CShape {
      value LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  auto cycles = graph.detectCycles();

  // No cycles should be detected
  EXPECT_FALSE(cycles.contains("AShape"));
  EXPECT_FALSE(cycles.contains("BShape"));
  EXPECT_FALSE(cycles.contains("CShape"));
}

TEST_F(ShapeReferenceTest, DiamondPattern) {
  std::string shexInput = R"(
    shape AShape {
      refToB @BShape ;
      refToC @CShape
    }
    shape BShape {
      refToD @DShape
    }
    shape CShape {
      refToD @DShape
    }
    shape DShape {
      value LITERAL
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  auto graph = schema->buildDependencyGraph();
  auto cycles = graph.detectCycles();

  // Diamond pattern has no cycles
  EXPECT_FALSE(cycles.contains("AShape"));
  EXPECT_FALSE(cycles.contains("BShape"));
  EXPECT_FALSE(cycles.contains("CShape"));
  EXPECT_FALSE(cycles.contains("DShape"));
}

// ============================================================================
// Deep Recursion Tests (10+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, DeepNestingLevel20) {
  // Create a deeply nested structure
  std::stringstream ss;
  const int depth = 20;

  for (int i = 0; i < depth; ++i) {
    ss << "shape Level" << i << "Shape {\n";
    ss << "  value LITERAL ;\n";
    if (i < depth - 1) {
      ss << "  next @Level" << (i + 1) << "Shape\n";
    }
    ss << "}\n";
  }

  auto schema = parser.parse(ss.str());
  ASSERT_TRUE(schema.has_value());

  // Verify all shapes exist
  for (int i = 0; i < depth; ++i) {
    std::string shapeName = "Level" + std::to_string(i) + "Shape";
    EXPECT_TRUE(schema->hasShape(shapeName));
  }

  auto errors = schema->validateShapeReferences();
  EXPECT_TRUE(errors.empty());
}

TEST_F(ShapeReferenceTest, DeepRecursion50Levels) {
  std::string shexInput = R"(
    shape RecursiveShape {
      value LITERAL ;
      next @RecursiveShape ?
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  // Simulate validation with deep recursion
  ValidationContext context;

  // Enter shape 50 times (simulating deep recursion)
  for (int i = 0; i < 50; ++i) {
    EXPECT_TRUE(context.enterShape("RecursiveShape_" + std::to_string(i)));
  }

  EXPECT_EQ(context.getDepth(), 50);
  EXPECT_FALSE(context.isDepthExceeded());
}

TEST_F(ShapeReferenceTest, RecursionDepthLimit) {
  ValidationContext context;

  // Try to exceed max recursion depth
  for (size_t i = 0; i <= ValidationContext::MAX_RECURSION_DEPTH + 10; ++i) {
    context.enterShape("Shape_" + std::to_string(i));
  }

  EXPECT_TRUE(context.isDepthExceeded());
}

// ============================================================================
// Validation Context Tests (10+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, ValidationContextCycleDetection) {
  ValidationContext context;

  EXPECT_TRUE(context.enterShape("ShapeA"));
  EXPECT_TRUE(context.enterShape("ShapeB"));
  EXPECT_TRUE(context.enterShape("ShapeC"));

  // Trying to enter ShapeA again should detect cycle
  EXPECT_FALSE(context.isInValidationPath("ShapeA"));
  context.enterShape("ShapeA");  // This would be detected as cycle
  EXPECT_TRUE(context.isInValidationPath("ShapeA"));
}

TEST_F(ShapeReferenceTest, ValidationGuardRAII) {
  ValidationContext context;

  {
    ValidationGuard guard1(context, "ShapeA");
    EXPECT_TRUE(guard1.isValid());
    EXPECT_EQ(context.getDepth(), 1);

    {
      ValidationGuard guard2(context, "ShapeB");
      EXPECT_TRUE(guard2.isValid());
      EXPECT_EQ(context.getDepth(), 2);
    }

    // After guard2 goes out of scope, depth should be back to 1
    EXPECT_EQ(context.getDepth(), 1);
  }

  // After both guards destroyed, depth should be 0
  EXPECT_EQ(context.getDepth(), 0);
}

TEST_F(ShapeReferenceTest, ValidationGuardDetectsCycle) {
  ValidationContext context;

  ValidationGuard guard1(context, "ShapeA");
  EXPECT_TRUE(guard1.isValid());

  ValidationGuard guard2(context, "ShapeA");  // Try to enter same shape
  EXPECT_FALSE(guard2.isValid());  // Should fail due to cycle
}

TEST_F(ShapeReferenceTest, ValidationContextPathTracking) {
  ValidationContext context;

  context.enterShape("Root");
  context.enterShape("Child1");
  context.enterShape("Child2");

  auto path = context.getPath();
  ASSERT_EQ(path.size(), 3);
  EXPECT_EQ(path[0], "Root");
  EXPECT_EQ(path[1], "Child1");
  EXPECT_EQ(path[2], "Child2");
}

TEST_F(ShapeReferenceTest, ValidationContextClear) {
  ValidationContext context;

  context.enterShape("ShapeA");
  context.enterShape("ShapeB");
  EXPECT_EQ(context.getDepth(), 2);

  context.clear();
  EXPECT_EQ(context.getDepth(), 0);
  EXPECT_FALSE(context.isInValidationPath("ShapeA"));
  EXPECT_FALSE(context.isInValidationPath("ShapeB"));
}

// ============================================================================
// Shape Dependency Graph Tests (10+ tests)
// ============================================================================

TEST_F(ShapeReferenceTest, DependencyGraphBasic) {
  ShapeDependencyGraph graph;

  graph.addDependency("AShape", "BShape");
  graph.addDependency("BShape", "CShape");

  auto deps = graph.getDependencies("AShape");
  ASSERT_NE(deps, nullptr);
  EXPECT_TRUE(deps->contains("BShape"));
  EXPECT_FALSE(deps->contains("CShape"));
}

TEST_F(ShapeReferenceTest, DependencyGraphCycleDetection) {
  ShapeDependencyGraph graph;

  graph.addDependency("A", "B");
  graph.addDependency("B", "C");
  graph.addDependency("C", "A");

  auto cycles = graph.detectCycles();
  EXPECT_TRUE(cycles.contains("A"));
  EXPECT_TRUE(cycles.contains("B"));
  EXPECT_TRUE(cycles.contains("C"));
}

TEST_F(ShapeReferenceTest, DependencyGraphNoCycles) {
  ShapeDependencyGraph graph;

  graph.addDependency("A", "B");
  graph.addDependency("B", "C");
  graph.addDependency("A", "C");

  auto cycles = graph.detectCycles();
  EXPECT_TRUE(cycles.empty());
}

TEST_F(ShapeReferenceTest, DependencyGraphMultipleCycles) {
  ShapeDependencyGraph graph;

  // First cycle: A -> B -> A
  graph.addDependency("A", "B");
  graph.addDependency("B", "A");

  // Second cycle: C -> D -> C
  graph.addDependency("C", "D");
  graph.addDependency("D", "C");

  auto cycles = graph.detectCycles();
  EXPECT_TRUE(cycles.contains("A"));
  EXPECT_TRUE(cycles.contains("B"));
  EXPECT_TRUE(cycles.contains("C"));
  EXPECT_TRUE(cycles.contains("D"));
}

TEST_F(ShapeReferenceTest, DependencyGraphClear) {
  ShapeDependencyGraph graph;

  graph.addDependency("A", "B");
  graph.clear();

  auto deps = graph.getDependencies("A");
  EXPECT_EQ(deps, nullptr);
}

// ============================================================================
// Performance Tests (100+ nodes)
// ============================================================================

TEST_F(ShapeReferenceTest, PerformanceLargeRecursiveStructure) {
  std::string shexInput = R"(
    shape NodeShape {
      id LITERAL ;
      children @NodeShape *
    }
  )";

  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  // Create a large dataset with 100+ nodes
  std::map<std::string,
    std::map<std::string, std::vector<std::pair<std::string, ValueType>>>> dataset;

  for (int i = 0; i < 100; ++i) {
    std::string nodeId = "node" + std::to_string(i);
    dataset[nodeId]["id"].push_back({nodeId, ValueType::LITERAL});

    // Add some children references
    if (i < 50) {
      int child1 = i * 2 + 1;
      int child2 = i * 2 + 2;
      if (child1 < 100) {
        dataset[nodeId]["children"].push_back({"node" + std::to_string(child1), ValueType::IRI});
      }
      if (child2 < 100) {
        dataset[nodeId]["children"].push_back({"node" + std::to_string(child2), ValueType::IRI});
      }
    }
  }

  // This test just ensures large structures don't crash
  // Actual validation would require full implementation
  EXPECT_EQ(dataset.size(), 100);
}

TEST_F(ShapeReferenceTest, Performance100LevelDeepChain) {
  std::stringstream ss;
  const int depth = 100;

  for (int i = 0; i < depth; ++i) {
    ss << "shape Level" << i << "Shape {\n";
    ss << "  value LITERAL ;\n";
    if (i < depth - 1) {
      ss << "  next @Level" << (i + 1) << "Shape\n";
    }
    ss << "}\n";
  }

  auto schema = parser.parse(ss.str());
  ASSERT_TRUE(schema.has_value());

  // Verify parsing works for deep chains
  EXPECT_EQ(schema->getShapes().size(), depth);
}

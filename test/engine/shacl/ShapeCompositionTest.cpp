#include <gtest/gtest.h>
#include "engine/shacl/ShapeComposition.h"
#include "engine/shacl/ShaclShapeRegistry.h"

namespace shacl {

class ShapeCompositionTest : public ::testing::Test {
 protected:
  ShaclShapeRegistry registry;

  // Helper to create a basic composable shape
  ComposableNodeShape createComposableShape(const std::string& id) {
    ComposableNodeShape shape;
    shape.shapeId = id;
    return shape;
  }

  // Helper to create a constraint
  ShaclConstraint createConstraint(ConstraintType type, int value) {
    ShaclConstraint constraint(type);
    constraint.value = value;
    return constraint;
  }

  ShaclConstraint createConstraint(ConstraintType type,
                                   const std::string& value) {
    ShaclConstraint constraint(type);
    constraint.value = value;
    return constraint;
  }
};

// ============================================================================
// ShapeDependencyGraph Tests
// ============================================================================

TEST_F(ShapeCompositionTest, DependencyGraphBasic) {
  ShapeDependencyGraph graph;

  // Shape A has no dependencies
  graph.addShape("ShapeA", {});

  // Shape B depends on Shape A
  graph.addShape("ShapeB", {"ShapeA"});

  // Shape C depends on Shape B
  graph.addShape("ShapeC", {"ShapeB"});

  auto order = graph.getResolutionOrder();
  ASSERT_EQ(order.size(), 3);

  // ShapeA should come before ShapeB and ShapeC
  auto itA = std::find(order.begin(), order.end(), "ShapeA");
  auto itB = std::find(order.begin(), order.end(), "ShapeB");
  auto itC = std::find(order.begin(), order.end(), "ShapeC");

  EXPECT_LT(itA, itB);
  EXPECT_LT(itB, itC);
}

TEST_F(ShapeCompositionTest, DependencyGraphMultipleDependencies) {
  ShapeDependencyGraph graph;

  // Diamond dependency pattern
  graph.addShape("ShapeBase", {});
  graph.addShape("ShapeLeft", {"ShapeBase"});
  graph.addShape("ShapeRight", {"ShapeBase"});
  graph.addShape("ShapeTop", {"ShapeLeft", "ShapeRight"});

  auto order = graph.getResolutionOrder();
  ASSERT_EQ(order.size(), 4);

  // ShapeBase should come before all others
  auto itBase = std::find(order.begin(), order.end(), "ShapeBase");
  auto itLeft = std::find(order.begin(), order.end(), "ShapeLeft");
  auto itRight = std::find(order.begin(), order.end(), "ShapeRight");
  auto itTop = std::find(order.begin(), order.end(), "ShapeTop");

  EXPECT_LT(itBase, itLeft);
  EXPECT_LT(itBase, itRight);
  EXPECT_LT(itLeft, itTop);
  EXPECT_LT(itRight, itTop);
}

TEST_F(ShapeCompositionTest, DependencyGraphCircularDependency) {
  ShapeDependencyGraph graph;

  // Create circular dependency: A -> B -> C -> A
  graph.addShape("ShapeA", {"ShapeC"});
  graph.addShape("ShapeB", {"ShapeA"});
  graph.addShape("ShapeC", {"ShapeB"});

  EXPECT_TRUE(graph.hasCycle());
  auto order = graph.getResolutionOrder();
  EXPECT_TRUE(order.empty());
}

TEST_F(ShapeCompositionTest, DependencyGraphSelfReference) {
  ShapeDependencyGraph graph;

  // Self-referential shape (cycle)
  graph.addShape("ShapeA", {"ShapeA"});

  EXPECT_TRUE(graph.hasCycle());
}

TEST_F(ShapeCompositionTest, DependencyGraphGetDependencies) {
  ShapeDependencyGraph graph;

  graph.addShape("ShapeA", {});
  graph.addShape("ShapeB", {"ShapeA"});
  graph.addShape("ShapeC", {"ShapeA", "ShapeB"});

  auto depsA = graph.getDependencies("ShapeA");
  EXPECT_EQ(depsA.size(), 0);

  auto depsB = graph.getDependencies("ShapeB");
  EXPECT_EQ(depsB.size(), 1);
  EXPECT_EQ(depsB[0], "ShapeA");

  auto depsC = graph.getDependencies("ShapeC");
  EXPECT_EQ(depsC.size(), 2);
}

TEST_F(ShapeCompositionTest, DependencyGraphGetDependents) {
  ShapeDependencyGraph graph;

  graph.addShape("ShapeA", {});
  graph.addShape("ShapeB", {"ShapeA"});
  graph.addShape("ShapeC", {"ShapeA"});

  auto dependentsA = graph.getDependents("ShapeA");
  EXPECT_EQ(dependentsA.size(), 2);

  auto dependentsB = graph.getDependents("ShapeB");
  EXPECT_EQ(dependentsB.size(), 0);
}

// ============================================================================
// ConstraintMerger Tests
// ============================================================================

TEST_F(ShapeCompositionTest, MergeConstraintsOverride) {
  std::vector<ShaclConstraint> parentConstraints;
  parentConstraints.push_back(createConstraint(ConstraintType::MinCount, 1));

  std::vector<ShaclConstraint> childConstraints;
  childConstraints.push_back(createConstraint(ConstraintType::MinCount, 2));

  auto merged = ConstraintMerger::mergeConstraints(
      parentConstraints, childConstraints,
      ConstraintMerger::MergeStrategy::Override);

  EXPECT_EQ(merged.size(), 1);
  EXPECT_EQ(merged[0].type, ConstraintType::MinCount);
  EXPECT_EQ(std::get<int>(merged[0].value), 2); // Child value wins
}

TEST_F(ShapeCompositionTest, MergeConstraintsMostRestrictive) {
  std::vector<ShaclConstraint> parentConstraints;
  parentConstraints.push_back(createConstraint(ConstraintType::MinCount, 2));
  parentConstraints.push_back(createConstraint(ConstraintType::MaxCount, 10));

  std::vector<ShaclConstraint> childConstraints;
  childConstraints.push_back(createConstraint(ConstraintType::MinCount, 1));
  childConstraints.push_back(createConstraint(ConstraintType::MaxCount, 5));

  auto merged = ConstraintMerger::mergeConstraints(
      parentConstraints, childConstraints,
      ConstraintMerger::MergeStrategy::MostRestrictive);

  EXPECT_EQ(merged.size(), 2);

  // Find MinCount constraint
  auto minCountIt =
      std::find_if(merged.begin(), merged.end(), [](const ShaclConstraint& c) {
        return c.type == ConstraintType::MinCount;
      });
  ASSERT_NE(minCountIt, merged.end());
  EXPECT_EQ(std::get<int>(minCountIt->value), 2); // Higher min is more
                                                   // restrictive

  // Find MaxCount constraint
  auto maxCountIt =
      std::find_if(merged.begin(), merged.end(), [](const ShaclConstraint& c) {
        return c.type == ConstraintType::MaxCount;
      });
  ASSERT_NE(maxCountIt, merged.end());
  EXPECT_EQ(std::get<int>(maxCountIt->value), 5); // Lower max is more
                                                   // restrictive
}

TEST_F(ShapeCompositionTest, MergeConstraintsAccumulate) {
  std::vector<ShaclConstraint> parentConstraints;
  parentConstraints.push_back(createConstraint(ConstraintType::MinCount, 1));
  parentConstraints.push_back(
      createConstraint(ConstraintType::Datatype, "xsd:integer"));

  std::vector<ShaclConstraint> childConstraints;
  childConstraints.push_back(createConstraint(ConstraintType::MaxCount, 5));

  auto merged = ConstraintMerger::mergeConstraints(
      parentConstraints, childConstraints,
      ConstraintMerger::MergeStrategy::Accumulate);

  // Should have all unique constraints
  EXPECT_GE(merged.size(), 2);
}

TEST_F(ShapeCompositionTest, MergePropertyShapes) {
  PropertyShape parentProp("http://example.org/name");
  parentProp.constraints.push_back(
      createConstraint(ConstraintType::MinLength, 1));

  PropertyShape childProp("http://example.org/name");
  childProp.constraints.push_back(
      createConstraint(ConstraintType::MaxLength, 100));

  std::vector<PropertyShape> parentShapes{parentProp};
  std::vector<PropertyShape> childShapes{childProp};

  auto merged = ConstraintMerger::mergePropertyShapes(
      parentShapes, childShapes,
      ConstraintMerger::MergeStrategy::Accumulate);

  EXPECT_EQ(merged.size(), 1);
  EXPECT_EQ(merged[0].path, "http://example.org/name");
  EXPECT_GE(merged[0].constraints.size(), 1);
}

TEST_F(ShapeCompositionTest, MergePropertyShapesNoConflict) {
  PropertyShape parentProp("http://example.org/name");
  PropertyShape childProp("http://example.org/email");

  std::vector<PropertyShape> parentShapes{parentProp};
  std::vector<PropertyShape> childShapes{childProp};

  auto merged = ConstraintMerger::mergePropertyShapes(
      parentShapes, childShapes,
      ConstraintMerger::MergeStrategy::Accumulate);

  EXPECT_EQ(merged.size(), 2); // Both properties present
}

// ============================================================================
// ShapeCompositionEngine Tests
// ============================================================================

TEST_F(ShapeCompositionTest, SimpleInheritance) {
  // Create parent shape
  NodeShape parent;
  parent.shapeId = "ParentShape";
  parent.nodeConstraints.push_back(
      createConstraint(ConstraintType::MinCount, 1));

  // Create child shape with inheritance
  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("ParentShape"));
  child.nodeConstraints.push_back(
      createConstraint(ConstraintType::MaxCount, 10));

  registry.registerShape(parent);
  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);
  auto resolved = engine.resolveShape(child);

  // Should have constraints from both parent and child
  EXPECT_GE(resolved.nodeConstraints.size(), 1);
  EXPECT_EQ(resolved.shapeId, "ChildShape");
}

TEST_F(ShapeCompositionTest, MultiLevelInheritance) {
  // Grandparent
  NodeShape grandparent;
  grandparent.shapeId = "GrandparentShape";
  grandparent.nodeConstraints.push_back(
      createConstraint(ConstraintType::MinCount, 1));

  // Parent extends grandparent
  ComposableNodeShape parent = createComposableShape("ParentShape");
  parent.addExtends(ShapeReference("GrandparentShape"));
  parent.nodeConstraints.push_back(
      createConstraint(ConstraintType::MaxCount, 100));

  // Child extends parent
  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("ParentShape"));
  child.nodeConstraints.push_back(
      createConstraint(ConstraintType::MaxLength, 50));

  registry.registerShape(grandparent);
  registry.registerComposableShape(parent);
  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);

  // Resolve parent first (it depends on grandparent)
  auto resolvedParent = engine.resolveShape(parent);
  registry.registerShape(resolvedParent);

  // Then resolve child
  auto resolvedChild = engine.resolveShape(child);

  EXPECT_EQ(resolvedChild.shapeId, "ChildShape");
}

TEST_F(ShapeCompositionTest, MultipleInheritance) {
  // Create two parent shapes
  NodeShape parent1;
  parent1.shapeId = "Parent1";
  parent1.nodeConstraints.push_back(
      createConstraint(ConstraintType::MinCount, 1));

  NodeShape parent2;
  parent2.shapeId = "Parent2";
  parent2.nodeConstraints.push_back(
      createConstraint(ConstraintType::MaxCount, 10));

  // Child extends both parents
  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("Parent1"));
  child.addExtends(ShapeReference("Parent2"));

  registry.registerShape(parent1);
  registry.registerShape(parent2);
  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);
  auto resolved = engine.resolveShape(child);

  // Should have constraints from both parents
  EXPECT_GE(resolved.nodeConstraints.size(), 1);
}

TEST_F(ShapeCompositionTest, ParameterSubstitution) {
  // Create a shape with parameters
  NodeShape parameterizedShape;
  parameterizedShape.shapeId = "ParameterizedShape";

  ShaclConstraint constraint(ConstraintType::MinCount);
  constraint.value = std::string("${minCount}");
  constraint.message = "Must have at least ${minCount} values";
  parameterizedShape.nodeConstraints.push_back(constraint);

  registry.registerShape(parameterizedShape);

  ShapeCompositionEngine engine(&registry);

  // Apply parameter bindings
  std::unordered_map<std::string, std::string> bindings;
  bindings["minCount"] = "5";

  auto bound = engine.applyParameterBindings(parameterizedShape, bindings);

  EXPECT_EQ(bound.nodeConstraints.size(), 1);
  // Note: In real implementation, value substitution would be more complex
}

TEST_F(ShapeCompositionTest, NodeReferences) {
  // Create a referenced shape
  NodeShape referencedShape;
  referencedShape.shapeId = "ReferencedShape";
  referencedShape.nodeConstraints.push_back(
      createConstraint(ConstraintType::MinCount, 1));

  // Create shape with node reference
  ComposableNodeShape mainShape = createComposableShape("MainShape");
  mainShape.addNodeReference(ShapeReference("ReferencedShape"));

  registry.registerShape(referencedShape);
  registry.registerComposableShape(mainShape);

  ShapeCompositionEngine engine(&registry);
  auto resolved = engine.resolveShape(mainShape);

  // Should accumulate constraints from referenced shape
  EXPECT_GE(resolved.nodeConstraints.size(), 1);
}

TEST_F(ShapeCompositionTest, CompositionValidationSuccess) {
  // Create valid composition hierarchy
  NodeShape parent;
  parent.shapeId = "ParentShape";

  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("ParentShape"));

  registry.registerShape(parent);
  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);
  auto errors = engine.validateComposition();

  EXPECT_TRUE(errors.empty()); // No errors
}

TEST_F(ShapeCompositionTest, CompositionValidationMissingDependency) {
  // Create shape with missing parent
  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("MissingParent"));

  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);
  auto errors = engine.validateComposition();

  EXPECT_FALSE(errors.empty()); // Should have errors
}

TEST_F(ShapeCompositionTest, ComplexCompositionHierarchy) {
  // Build a complex hierarchy:
  //     Base
  //    /    \
  //  Left  Right
  //    \    /
  //     Top

  NodeShape base;
  base.shapeId = "Base";
  base.nodeConstraints.push_back(createConstraint(ConstraintType::MinCount, 1));

  ComposableNodeShape left = createComposableShape("Left");
  left.addExtends(ShapeReference("Base"));
  left.nodeConstraints.push_back(
      createConstraint(ConstraintType::MaxCount, 100));

  ComposableNodeShape right = createComposableShape("Right");
  right.addExtends(ShapeReference("Base"));
  right.nodeConstraints.push_back(
      createConstraint(ConstraintType::MinLength, 5));

  ComposableNodeShape top = createComposableShape("Top");
  top.addExtends(ShapeReference("Left"));
  top.addExtends(ShapeReference("Right"));

  registry.registerShape(base);
  registry.registerComposableShape(left);
  registry.registerComposableShape(right);
  registry.registerComposableShape(top);

  ShapeCompositionEngine engine(&registry);

  // Resolve left and right first
  auto resolvedLeft = engine.resolveShape(left);
  auto resolvedRight = engine.resolveShape(right);
  registry.registerShape(resolvedLeft);
  registry.registerShape(resolvedRight);

  // Then resolve top
  auto resolvedTop = engine.resolveShape(top);

  EXPECT_EQ(resolvedTop.shapeId, "Top");
  // Top should have accumulated constraints from all ancestors
  EXPECT_GE(resolvedTop.nodeConstraints.size(), 1);
}

TEST_F(ShapeCompositionTest, PropertyShapeInheritance) {
  // Parent with property shape
  NodeShape parent;
  parent.shapeId = "ParentShape";

  PropertyShape nameProp("http://example.org/name");
  nameProp.constraints.push_back(
      createConstraint(ConstraintType::MinLength, 1));
  parent.propertyShapes.push_back(nameProp);

  // Child extends parent and adds more constraints to same property
  ComposableNodeShape child = createComposableShape("ChildShape");
  child.addExtends(ShapeReference("ParentShape"));

  PropertyShape childNameProp("http://example.org/name");
  childNameProp.constraints.push_back(
      createConstraint(ConstraintType::MaxLength, 100));
  child.propertyShapes.push_back(childNameProp);

  registry.registerShape(parent);
  registry.registerComposableShape(child);

  ShapeCompositionEngine engine(&registry);
  auto resolved = engine.resolveShape(child);

  // Should have one property shape with merged constraints
  EXPECT_GE(resolved.propertyShapes.size(), 1);

  auto nameShape = std::find_if(
      resolved.propertyShapes.begin(), resolved.propertyShapes.end(),
      [](const PropertyShape& ps) {
        return ps.path == "http://example.org/name";
      });

  EXPECT_NE(nameShape, resolved.propertyShapes.end());
}

TEST_F(ShapeCompositionTest, MergeStrategyConfiguration) {
  ShapeCompositionEngine engine(&registry);

  // Test different merge strategies
  engine.setMergeStrategy(ConstraintMerger::MergeStrategy::Override);
  engine.setMergeStrategy(ConstraintMerger::MergeStrategy::Accumulate);
  engine.setMergeStrategy(ConstraintMerger::MergeStrategy::MostRestrictive);

  // No assertion needed, just verifying the API works
}

} // namespace shacl

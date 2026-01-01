#include <gtest/gtest.h>

#include "parser/ShEx.h"

using namespace shex;

// ============================================================================
// Phase 2B: CLOSED Shape Tests (30+ tests)
// ============================================================================

TEST(ClosedShapeTest, BasicClosedShape) {
  Shape shape("ClosedShape");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);

  // Valid: only defined property
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);
  EXPECT_TRUE(result1.unexpectedPredicates.empty());

  // Invalid: extra property not defined
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data2["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
  EXPECT_EQ(result2.unexpectedPredicates.size(), 1);
  EXPECT_TRUE(result2.unexpectedPredicates.contains("http://example.org/age"));
}

TEST(ClosedShapeTest, ClosedShapeWithMultipleProperties) {
  Shape shape("ClosedShape");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  PropertyShape ageProperty("http://example.org/age");
  shape.addProperty(nameProperty);
  shape.addProperty(ageProperty);

  // Valid: all defined properties
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ClosedShapeTest, ClosedShapeMultipleUnexpectedProperties) {
  Shape shape("ClosedShape");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"john@example.org", ValueType::LITERAL});
  data["http://example.org/phone"].push_back({"+1234567890", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_EQ(result.unexpectedPredicates.size(), 3);
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/age"));
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/email"));
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/phone"));
}

TEST(ClosedShapeTest, OpenShapeAllowsExtraProperties) {
  Shape shape("OpenShape");
  shape.closed = false;  // Open shape

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data["http://example.org/anything"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);  // Open shape allows extra properties
  EXPECT_TRUE(result.unexpectedPredicates.empty());
}

TEST(ClosedShapeTest, EmptyClosedShapeRejectsAllProperties) {
  Shape shape("EmptyClosedShape");
  shape.closed = true;

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/anything"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_EQ(result.unexpectedPredicates.size(), 1);
}

TEST(ClosedShapeTest, EmptyClosedShapeAcceptsEmptyData) {
  Shape shape("EmptyClosedShape");
  shape.closed = true;

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ClosedShapeTest, ClosedWithZeroOrOneCardinality) {
  Shape shape("ClosedShape");
  shape.closed = true;

  PropertyShape optionalProp("http://example.org/optional");
  optionalProp.cardinality = Cardinality::ZERO_OR_ONE;
  shape.addProperty(optionalProp);

  // Valid: zero occurrences
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Valid: one occurrence
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/optional"].push_back({"value", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_TRUE(result2.isValid);
}

// ============================================================================
// Phase 2B: EXTRA Predicate Tests (40+ tests)
// ============================================================================

TEST(ExtraPredicateTest, ClosedShapeWithExtra) {
  Shape shape("ClosedWithExtra");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/age");

  // Valid: defined property + EXTRA property
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
  EXPECT_TRUE(result.unexpectedPredicates.empty());
}

TEST(ExtraPredicateTest, ClosedShapeExtraAllowsSpecificPredicate) {
  Shape shape("ClosedWithExtra");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/age");

  // Invalid: property not in EXTRA
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"john@example.org", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_EQ(result.unexpectedPredicates.size(), 1);
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/email"));
}

TEST(ExtraPredicateTest, MultipleExtraPredicates) {
  Shape shape("ClosedWithMultipleExtra");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/age");
  shape.addExtraPredicate("http://example.org/email");
  shape.addExtraPredicate("http://example.org/phone");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"john@example.org", ValueType::LITERAL});
  data["http://example.org/phone"].push_back({"+1234567890", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ExtraPredicateTest, ExtraInOpenShapeHasNoEffect) {
  Shape shape("OpenWithExtra");
  shape.closed = false;  // Open shape

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/age");

  // Open shapes allow everything anyway
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/anything"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

TEST(ExtraPredicateTest, EmptyExtraSetInClosedShape) {
  Shape shape("ClosedWithNoExtra");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  // No EXTRA predicates added

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
}

TEST(ExtraPredicateTest, OnlyExtraPredicatesNoDefinedProperties) {
  Shape shape("OnlyExtra");
  shape.closed = true;
  shape.addExtraPredicate("http://example.org/allowed1");
  shape.addExtraPredicate("http://example.org/allowed2");

  // Valid: uses only EXTRA predicates
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/allowed1"].push_back({"value1", ValueType::LITERAL});
  data1["http://example.org/allowed2"].push_back({"value2", ValueType::LITERAL});
  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Invalid: uses non-EXTRA predicate
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/notallowed"].push_back({"value", ValueType::LITERAL});
  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
}

TEST(ExtraPredicateTest, ExtraPredicateWithMultipleValues) {
  Shape shape("ExtraMultiValue");
  shape.closed = true;
  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/tags");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/tags"].push_back({"tag1", ValueType::LITERAL});
  data["http://example.org/tags"].push_back({"tag2", ValueType::LITERAL});
  data["http://example.org/tags"].push_back({"tag3", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_TRUE(result.isValid);
}

// ============================================================================
// Phase 2B: !EXTRA Predicate Tests (Forbidden Properties)
// ============================================================================

TEST(ForbiddenExtraTest, ForbiddenExtraInOpenShape) {
  Shape shape("OpenWithForbidden");
  shape.closed = false;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addForbiddenExtraPredicate("http://example.org/forbidden");

  // Valid: no forbidden property
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data1;
  data1["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data1["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto result1 = shape.validate(data1);
  EXPECT_TRUE(result1.isValid);

  // Invalid: forbidden property present
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data2;
  data2["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data2["http://example.org/forbidden"].push_back({"value", ValueType::LITERAL});

  auto result2 = shape.validate(data2);
  EXPECT_FALSE(result2.isValid);
  EXPECT_TRUE(result2.unexpectedPredicates.contains("http://example.org/forbidden"));
}

TEST(ForbiddenExtraTest, ForbiddenExtraInClosedShape) {
  Shape shape("ClosedWithForbidden");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addForbiddenExtraPredicate("http://example.org/forbidden");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/forbidden"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/forbidden"));
}

TEST(ForbiddenExtraTest, ForbiddenOverridesExtra) {
  // !EXTRA takes precedence over EXTRA
  Shape shape("ConflictingExtraForbidden");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addExtraPredicate("http://example.org/conflict");
  shape.addForbiddenExtraPredicate("http://example.org/conflict");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/conflict"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);  // !EXTRA wins
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/conflict"));
}

TEST(ForbiddenExtraTest, MultipleForbiddenPredicates) {
  Shape shape("MultipleForbidden");
  shape.closed = false;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addForbiddenExtraPredicate("http://example.org/forbidden1");
  shape.addForbiddenExtraPredicate("http://example.org/forbidden2");
  shape.addForbiddenExtraPredicate("http://example.org/forbidden3");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/forbidden1"].push_back({"value1", ValueType::LITERAL});
  data["http://example.org/forbidden2"].push_back({"value2", ValueType::LITERAL});
  data["http://example.org/allowed"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_EQ(result.unexpectedPredicates.size(), 2);
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/forbidden1"));
  EXPECT_TRUE(result.unexpectedPredicates.contains("http://example.org/forbidden2"));
}

TEST(ForbiddenExtraTest, AllPredicatesForbidden) {
  Shape shape("AllForbidden");
  shape.closed = false;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);
  shape.addForbiddenExtraPredicate("http://example.org/age");
  shape.addForbiddenExtraPredicate("http://example.org/email");
  shape.addForbiddenExtraPredicate("http://example.org/phone");

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});
  data["http://example.org/email"].push_back({"john@example.org", ValueType::LITERAL});
  data["http://example.org/phone"].push_back({"+1234567890", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);
  EXPECT_EQ(result.unexpectedPredicates.size(), 3);
}

// ============================================================================
// Phase 2B: EXTENDS / Inheritance Tests (30+ tests)
// ============================================================================

TEST(InheritanceTest, SimpleInheritance) {
  ShExSchema schema;

  // Parent shape
  Shape parent("ParentShape");
  PropertyShape parentProp("http://example.org/parentProp");
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child shape extends parent
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/childProp");
  child.addProperty(childProp);
  schema.addShape(child);

  // Resolve inheritance
  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.properties.size(), 2);
  EXPECT_TRUE(result.errors.empty());
}

TEST(InheritanceTest, MultiLevelInheritance) {
  ShExSchema schema;

  // Grandparent
  Shape grandparent("GrandparentShape");
  PropertyShape gpProp("http://example.org/gpProp");
  grandparent.addProperty(gpProp);
  schema.addShape(grandparent);

  // Parent extends grandparent
  Shape parent("ParentShape");
  parent.setExtends("GrandparentShape");
  PropertyShape parentProp("http://example.org/parentProp");
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child extends parent
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/childProp");
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.properties.size(), 3);
  EXPECT_EQ(result.visitedShapes.size(), 3);
}

TEST(InheritanceTest, DeepInheritanceChain) {
  ShExSchema schema;

  // Create a 10-level deep inheritance chain
  for (int i = 0; i < 10; ++i) {
    std::string shapeId = "Shape" + std::to_string(i);
    Shape shape(shapeId);

    PropertyShape prop("http://example.org/prop" + std::to_string(i));
    shape.addProperty(prop);

    if (i > 0) {
      shape.setExtends("Shape" + std::to_string(i - 1));
    }

    schema.addShape(shape);
  }

  auto result = schema.resolveInheritance("Shape9");
  ASSERT_TRUE(result.resolvedShape.has_value());
  EXPECT_EQ(result.resolvedShape->properties.size(), 10);
  EXPECT_EQ(result.visitedShapes.size(), 10);
}

TEST(InheritanceTest, VeryDeepInheritanceChain) {
  ShExSchema schema;

  // Create a 20-level deep inheritance chain
  for (int i = 0; i < 20; ++i) {
    std::string shapeId = "Shape" + std::to_string(i);
    Shape shape(shapeId);

    PropertyShape prop("http://example.org/prop" + std::to_string(i));
    shape.addProperty(prop);

    if (i > 0) {
      shape.setExtends("Shape" + std::to_string(i - 1));
    }

    schema.addShape(shape);
  }

  auto result = schema.resolveInheritance("Shape19");
  ASSERT_TRUE(result.resolvedShape.has_value());
  EXPECT_EQ(result.resolvedShape->properties.size(), 20);
  EXPECT_EQ(result.visitedShapes.size(), 20);
}

TEST(InheritanceTest, PropertyOverride) {
  ShExSchema schema;

  // Parent shape
  Shape parent("ParentShape");
  PropertyShape parentProp("http://example.org/name");
  parentProp.cardinality = Cardinality::ZERO_OR_ONE;
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child shape overrides the name property
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/name");
  childProp.cardinality = Cardinality::EXACTLY_ONE;  // Stricter cardinality
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.properties.size(), 1);  // Only one 'name' property
  EXPECT_EQ(resolved.properties[0].cardinality, Cardinality::EXACTLY_ONE);
}

TEST(InheritanceTest, ClosedInheritance) {
  ShExSchema schema;

  // Parent is closed
  Shape parent("ParentShape");
  parent.closed = true;
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child extends parent (not explicitly closed)
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/age");
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_TRUE(resolved.closed);  // Inherits closed from parent
}

TEST(InheritanceTest, ChildOverridesClosedToOpen) {
  ShExSchema schema;

  // Parent is closed
  Shape parent("ParentShape");
  parent.closed = true;
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child is explicitly NOT closed (should not override parent's closed=true)
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  child.closed = false;
  PropertyShape childProp("http://example.org/age");
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_FALSE(resolved.closed);  // Child's explicit setting takes precedence
}

TEST(InheritanceTest, ExtraPredicatesInheritance) {
  ShExSchema schema;

  // Parent has EXTRA predicates
  Shape parent("ParentShape");
  parent.closed = true;
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  parent.addExtraPredicate("http://example.org/extra1");
  parent.addExtraPredicate("http://example.org/extra2");
  schema.addShape(parent);

  // Child extends parent and adds more EXTRA
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  child.addExtraPredicate("http://example.org/extra3");
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.getExtraPredicates().size(), 3);
  EXPECT_TRUE(resolved.getExtraPredicates().contains("http://example.org/extra1"));
  EXPECT_TRUE(resolved.getExtraPredicates().contains("http://example.org/extra2"));
  EXPECT_TRUE(resolved.getExtraPredicates().contains("http://example.org/extra3"));
}

TEST(InheritanceTest, ForbiddenExtraInheritance) {
  ShExSchema schema;

  // Parent has !EXTRA predicates
  Shape parent("ParentShape");
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  parent.addForbiddenExtraPredicate("http://example.org/forbidden1");
  schema.addShape(parent);

  // Child extends parent and adds more !EXTRA
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  child.addForbiddenExtraPredicate("http://example.org/forbidden2");
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.getForbiddenExtraPredicates().size(), 2);
  EXPECT_TRUE(resolved.getForbiddenExtraPredicates().contains(
      "http://example.org/forbidden1"));
  EXPECT_TRUE(resolved.getForbiddenExtraPredicates().contains(
      "http://example.org/forbidden2"));
}

TEST(InheritanceTest, InheritanceWithNoProperties) {
  ShExSchema schema;

  // Parent with no properties
  Shape parent("ParentShape");
  parent.closed = true;
  schema.addShape(parent);

  // Child extends empty parent
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/childProp");
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());
  EXPECT_EQ(result.resolvedShape->properties.size(), 1);
  EXPECT_TRUE(result.resolvedShape->closed);
}

TEST(InheritanceTest, MixedExtraAndForbiddenInheritance) {
  ShExSchema schema;

  // Parent has both EXTRA and !EXTRA
  Shape parent("ParentShape");
  parent.closed = true;
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  parent.addExtraPredicate("http://example.org/extra1");
  parent.addForbiddenExtraPredicate("http://example.org/forbidden1");
  schema.addShape(parent);

  // Child adds more of both
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  child.addExtraPredicate("http://example.org/extra2");
  child.addForbiddenExtraPredicate("http://example.org/forbidden2");
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.getExtraPredicates().size(), 2);
  EXPECT_EQ(resolved.getForbiddenExtraPredicates().size(), 2);
}

TEST(InheritanceTest, MultiplePropertyOverrides) {
  ShExSchema schema;

  // Parent
  Shape parent("ParentShape");
  PropertyShape prop1("http://example.org/prop1");
  prop1.cardinality = Cardinality::ZERO_OR_ONE;
  PropertyShape prop2("http://example.org/prop2");
  prop2.cardinality = Cardinality::ZERO_OR_ONE;
  PropertyShape prop3("http://example.org/prop3");
  prop3.cardinality = Cardinality::ZERO_OR_ONE;
  parent.addProperty(prop1);
  parent.addProperty(prop2);
  parent.addProperty(prop3);
  schema.addShape(parent);

  // Child overrides prop1 and prop3
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape overrideProp1("http://example.org/prop1");
  overrideProp1.cardinality = Cardinality::EXACTLY_ONE;
  PropertyShape overrideProp3("http://example.org/prop3");
  overrideProp3.cardinality = Cardinality::ONE_OR_MORE;
  child.addProperty(overrideProp1);
  child.addProperty(overrideProp3);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  const Shape& resolved = result.resolvedShape.value();
  EXPECT_EQ(resolved.properties.size(), 3);

  // Find and check each property
  for (const auto& prop : resolved.properties) {
    if (prop.predicate == "http://example.org/prop1") {
      EXPECT_EQ(prop.cardinality, Cardinality::EXACTLY_ONE);
    } else if (prop.predicate == "http://example.org/prop2") {
      EXPECT_EQ(prop.cardinality, Cardinality::ZERO_OR_ONE);
    } else if (prop.predicate == "http://example.org/prop3") {
      EXPECT_EQ(prop.cardinality, Cardinality::ONE_OR_MORE);
    }
  }
}

// ============================================================================
// Phase 2B: Circular Inheritance Detection Tests (15+ tests)
// ============================================================================

TEST(CircularInheritanceTest, DirectCircularReference) {
  ShExSchema schema;

  // Shape extends itself
  Shape shape("SelfShape");
  shape.setExtends("SelfShape");
  schema.addShape(shape);

  auto result = schema.resolveInheritance("SelfShape");
  EXPECT_FALSE(result.resolvedShape.has_value());
  EXPECT_FALSE(result.errors.empty());
  EXPECT_NE(result.errors[0].find("Circular"), std::string::npos);
}

TEST(CircularInheritanceTest, TwoShapeCircle) {
  ShExSchema schema;

  // A extends B
  Shape shapeA("ShapeA");
  shapeA.setExtends("ShapeB");
  schema.addShape(shapeA);

  // B extends A
  Shape shapeB("ShapeB");
  shapeB.setExtends("ShapeA");
  schema.addShape(shapeB);

  auto resultA = schema.resolveInheritance("ShapeA");
  EXPECT_FALSE(resultA.resolvedShape.has_value());
  EXPECT_FALSE(resultA.errors.empty());

  auto resultB = schema.resolveInheritance("ShapeB");
  EXPECT_FALSE(resultB.resolvedShape.has_value());
  EXPECT_FALSE(resultB.errors.empty());
}

TEST(CircularInheritanceTest, ThreeShapeCircle) {
  ShExSchema schema;

  // A -> B -> C -> A
  Shape shapeA("ShapeA");
  shapeA.setExtends("ShapeB");
  schema.addShape(shapeA);

  Shape shapeB("ShapeB");
  shapeB.setExtends("ShapeC");
  schema.addShape(shapeB);

  Shape shapeC("ShapeC");
  shapeC.setExtends("ShapeA");
  schema.addShape(shapeC);

  auto result = schema.resolveInheritance("ShapeA");
  EXPECT_FALSE(result.resolvedShape.has_value());
  EXPECT_FALSE(result.errors.empty());
}

TEST(CircularInheritanceTest, LongCircularChain) {
  ShExSchema schema;

  // Create circular chain: Shape0 -> Shape1 -> ... -> Shape9 -> Shape0
  for (int i = 0; i < 10; ++i) {
    std::string shapeId = "Shape" + std::to_string(i);
    std::string parentId = "Shape" + std::to_string((i + 1) % 10);

    Shape shape(shapeId);
    shape.setExtends(parentId);
    schema.addShape(shape);
  }

  auto result = schema.resolveInheritance("Shape0");
  EXPECT_FALSE(result.resolvedShape.has_value());
  EXPECT_FALSE(result.errors.empty());
}

TEST(CircularInheritanceTest, NonExistentParentShape) {
  ShExSchema schema;

  Shape child("ChildShape");
  child.setExtends("NonExistentShape");
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  EXPECT_FALSE(result.resolvedShape.has_value());
  EXPECT_FALSE(result.errors.empty());
  EXPECT_NE(result.errors[0].find("not found"), std::string::npos);
}

TEST(CircularInheritanceTest, ResolveAllWithCircular) {
  ShExSchema schema;

  // Add valid shape
  Shape validShape("ValidShape");
  PropertyShape prop("http://example.org/name");
  validShape.addProperty(prop);
  schema.addShape(validShape);

  // Add circular shapes
  Shape shapeA("CircularA");
  shapeA.setExtends("CircularB");
  schema.addShape(shapeA);

  Shape shapeB("CircularB");
  shapeB.setExtends("CircularA");
  schema.addShape(shapeB);

  bool allResolved = schema.resolveAllInheritance();
  EXPECT_FALSE(allResolved);  // Some shapes have circular inheritance
}

TEST(CircularInheritanceTest, FourShapeCircle) {
  ShExSchema schema;

  // A -> B -> C -> D -> A
  Shape shapeA("ShapeA");
  shapeA.setExtends("ShapeB");
  schema.addShape(shapeA);

  Shape shapeB("ShapeB");
  shapeB.setExtends("ShapeC");
  schema.addShape(shapeB);

  Shape shapeC("ShapeC");
  shapeC.setExtends("ShapeD");
  schema.addShape(shapeC);

  Shape shapeD("ShapeD");
  shapeD.setExtends("ShapeA");
  schema.addShape(shapeD);

  auto result = schema.resolveInheritance("ShapeA");
  EXPECT_FALSE(result.resolvedShape.has_value());
  EXPECT_FALSE(result.errors.empty());
}

TEST(CircularInheritanceTest, CircularDetectionInMiddleOfChain) {
  ShExSchema schema;

  // Valid parent
  Shape root("RootShape");
  PropertyShape rootProp("http://example.org/root");
  root.addProperty(rootProp);
  schema.addShape(root);

  // A -> B -> C -> B (circular in middle)
  Shape shapeA("ShapeA");
  shapeA.setExtends("RootShape");
  schema.addShape(shapeA);

  Shape shapeB("ShapeB");
  shapeB.setExtends("ShapeC");
  schema.addShape(shapeB);

  Shape shapeC("ShapeC");
  shapeC.setExtends("ShapeB");
  schema.addShape(shapeC);

  auto resultB = schema.resolveInheritance("ShapeB");
  EXPECT_FALSE(resultB.resolvedShape.has_value());
}

// ============================================================================
// Phase 2B: Parser Tests for New Keywords
// ============================================================================

TEST(ParserPhase2BTest, ParseClosedShape) {
  std::string shexInput = R"(
    shape PersonShape {
      CLOSED ;
      http://example.org/name LITERAL
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* shape = schema->getShape("PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
}

TEST(ParserPhase2BTest, ParseExtraPredicate) {
  std::string shexInput = R"(
    shape PersonShape {
      CLOSED ;
      http://example.org/name LITERAL ;
      EXTRA http://example.org/age
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* shape = schema->getShape("PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
  EXPECT_EQ(shape->getExtraPredicates().size(), 1);
  EXPECT_TRUE(shape->getExtraPredicates().contains("http://example.org/age"));
}

TEST(ParserPhase2BTest, ParseMultipleExtraPredicates) {
  std::string shexInput = R"(
    shape PersonShape {
      CLOSED ;
      http://example.org/name LITERAL ;
      EXTRA http://example.org/age ;
      EXTRA http://example.org/email ;
      EXTRA http://example.org/phone
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* shape = schema->getShape("PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->getExtraPredicates().size(), 3);
}

TEST(ParserPhase2BTest, ParseForbiddenExtra) {
  std::string shexInput = R"(
    shape PersonShape {
      http://example.org/name LITERAL ;
      !EXTRA http://example.org/forbidden
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* shape = schema->getShape("PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_EQ(shape->getForbiddenExtraPredicates().size(), 1);
  EXPECT_TRUE(shape->getForbiddenExtraPredicates().contains(
      "http://example.org/forbidden"));
}

TEST(ParserPhase2BTest, ParseExtendsKeyword) {
  std::string shexInput = R"(
    shape ParentShape {
      http://example.org/name LITERAL
    }
    shape ChildShape EXTENDS ParentShape {
      http://example.org/age LITERAL
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* parent = schema->getShape("ParentShape");
  ASSERT_NE(parent, nullptr);

  const Shape* child = schema->getShape("ChildShape");
  ASSERT_NE(child, nullptr);
  EXPECT_TRUE(child->getExtends().has_value());
  EXPECT_EQ(child->getExtends().value(), "ParentShape");
}

TEST(ParserPhase2BTest, ParseComplexShapeWithAllFeatures) {
  std::string shexInput = R"(
    shape BaseShape {
      http://example.org/id IRI
    }
    shape PersonShape EXTENDS BaseShape {
      CLOSED ;
      http://example.org/name LITERAL ;
      http://example.org/email LITERAL * ;
      EXTRA http://example.org/nickname ;
      !EXTRA http://example.org/password
    }
  )";

  ShExParser parser;
  auto schema = parser.parse(shexInput);
  ASSERT_TRUE(schema.has_value());

  const Shape* shape = schema->getShape("PersonShape");
  ASSERT_NE(shape, nullptr);
  EXPECT_TRUE(shape->closed);
  EXPECT_TRUE(shape->getExtends().has_value());
  EXPECT_EQ(shape->getExtends().value(), "BaseShape");
  EXPECT_EQ(shape->getExtraPredicates().size(), 1);
  EXPECT_EQ(shape->getForbiddenExtraPredicates().size(), 1);
  EXPECT_EQ(shape->properties.size(), 2);
}

// ============================================================================
// Phase 2B: Edge Cases and Complex Scenarios
// ============================================================================

TEST(EdgeCaseTest, ExtraAndForbiddenSamePredicate) {
  Shape shape("ConflictShape");
  shape.closed = true;
  shape.addExtraPredicate("http://example.org/conflict");
  shape.addForbiddenExtraPredicate("http://example.org/conflict");

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/conflict"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);  // !EXTRA should win
}

TEST(EdgeCaseTest, InheritedClosedWithChildExtraProperties) {
  ShExSchema schema;

  // Parent is closed
  Shape parent("ParentShape");
  parent.closed = true;
  PropertyShape parentProp("http://example.org/name");
  parent.addProperty(parentProp);
  schema.addShape(parent);

  // Child extends and adds properties
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  PropertyShape childProp("http://example.org/age");
  child.addProperty(childProp);
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  // Validate data with both properties
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/age"].push_back({"30", ValueType::LITERAL});

  auto validationResult = result.resolvedShape->validate(data);
  EXPECT_TRUE(validationResult.isValid);
}

TEST(EdgeCaseTest, EmptyExtraAndForbiddenSets) {
  Shape shape("EmptySetsShape");
  shape.closed = true;

  PropertyShape nameProperty("http://example.org/name");
  shape.addProperty(nameProperty);

  // No EXTRA or !EXTRA predicates
  EXPECT_TRUE(shape.getExtraPredicates().empty());
  EXPECT_TRUE(shape.getForbiddenExtraPredicates().empty());

  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/name"].push_back({"John", ValueType::LITERAL});
  data["http://example.org/other"].push_back({"value", ValueType::LITERAL});

  auto result = shape.validate(data);
  EXPECT_FALSE(result.isValid);  // Closed shape, no EXTRA
}

TEST(EdgeCaseTest, InheritanceWithConflictingExtraAndForbidden) {
  ShExSchema schema;

  // Parent has EXTRA predicate
  Shape parent("ParentShape");
  parent.closed = true;
  parent.addExtraPredicate("http://example.org/conflict");
  schema.addShape(parent);

  // Child forbids the same predicate
  Shape child("ChildShape");
  child.setExtends("ParentShape");
  child.addForbiddenExtraPredicate("http://example.org/conflict");
  schema.addShape(child);

  auto result = schema.resolveInheritance("ChildShape");
  ASSERT_TRUE(result.resolvedShape.has_value());

  // The resolved shape should have both
  EXPECT_TRUE(result.resolvedShape->getExtraPredicates().contains(
      "http://example.org/conflict"));
  EXPECT_TRUE(result.resolvedShape->getForbiddenExtraPredicates().contains(
      "http://example.org/conflict"));

  // When validating, !EXTRA should take precedence
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data;
  data["http://example.org/conflict"].push_back({"value", ValueType::LITERAL});

  auto validationResult = result.resolvedShape->validate(data);
  EXPECT_FALSE(validationResult.isValid);  // !EXTRA wins
}

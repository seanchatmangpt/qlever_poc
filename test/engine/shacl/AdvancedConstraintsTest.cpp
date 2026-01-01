#include <gtest/gtest.h>

#include "engine/shacl/AdvancedConstraints.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"
#include "engine/shacl/ShaclShape.h"

using namespace shacl;

// Test fixture for advanced constraints
class AdvancedConstraintsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tracker = std::make_unique<ResourceStateTracker>();
    context.stateTracker = tracker.get();
  }

  void TearDown() override { tracker->clear(); }

  std::unique_ptr<ResourceStateTracker> tracker;
  AdvancedConstraintContext context;
};

// ============================================================================
// ResourceStateTracker Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, TrackUniqueValueSuccessful) {
  std::string propertyPath = "http://example.org/prop1";
  std::string value1 = "value1";
  std::string value2 = "value2";

  // First value should be tracked successfully
  EXPECT_TRUE(tracker->trackUniqueValue(propertyPath, value1));

  // Second different value should be tracked successfully
  EXPECT_TRUE(tracker->trackUniqueValue(propertyPath, value2));

  EXPECT_EQ(tracker->valueCount(propertyPath), 2);
}

TEST_F(AdvancedConstraintsTest, TrackUniqueValueDetectsDuplicate) {
  std::string propertyPath = "http://example.org/prop1";
  std::string value = "duplicate";

  // First occurrence
  EXPECT_TRUE(tracker->trackUniqueValue(propertyPath, value));

  // Duplicate detection
  EXPECT_FALSE(tracker->trackUniqueValue(propertyPath, value));
}

TEST_F(AdvancedConstraintsTest, HasValueCheck) {
  std::string propertyPath = "http://example.org/prop1";
  std::string value = "testValue";

  EXPECT_FALSE(tracker->hasValue(propertyPath, value));

  tracker->trackUniqueValue(propertyPath, value);

  EXPECT_TRUE(tracker->hasValue(propertyPath, value));
}

TEST_F(AdvancedConstraintsTest, GetValuesReturnsTrackedValues) {
  std::string propertyPath = "http://example.org/prop1";

  tracker->trackUniqueValue(propertyPath, "value1");
  tracker->trackUniqueValue(propertyPath, "value2");
  tracker->trackUniqueValue(propertyPath, "value3");

  auto values = tracker->getValues(propertyPath);
  EXPECT_EQ(values.size(), 3);
  EXPECT_TRUE(std::find(values.begin(), values.end(), "value1") !=
              values.end());
  EXPECT_TRUE(std::find(values.begin(), values.end(), "value2") !=
              values.end());
  EXPECT_TRUE(std::find(values.begin(), values.end(), "value3") !=
              values.end());
}

TEST_F(AdvancedConstraintsTest, DisjointViolationDetection) {
  std::vector<std::string> values1 = {"a", "b", "c"};
  std::vector<std::string> values2 = {"d", "e", "f"};
  std::vector<std::string> values3 = {"c", "d", "e"};

  // No overlap - no violation
  EXPECT_FALSE(tracker->hasDisjointViolation("prop1", "prop2", values1, values2));

  // Overlap exists - violation
  EXPECT_TRUE(tracker->hasDisjointViolation("prop1", "prop2", values1, values3));
}

TEST_F(AdvancedConstraintsTest, ClearStateTracker) {
  tracker->trackUniqueValue("prop1", "value1");
  tracker->trackUniqueValue("prop2", "value2");

  EXPECT_EQ(tracker->propertyCount(), 2);

  tracker->clear();

  EXPECT_EQ(tracker->propertyCount(), 0);
  EXPECT_FALSE(tracker->hasValue("prop1", "value1"));
}

// ============================================================================
// sh:unique Constraint Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, UniqueConstraintNoViolation) {
  UniqueConstraintValue constraint("http://example.org/email");

  std::vector<std::string> resource1Values = {"alice@example.org"};
  std::vector<std::string> resource2Values = {"bob@example.org"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                        resource1Values, &context));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                        resource2Values, &context));
}

TEST_F(AdvancedConstraintsTest, UniqueConstraintViolation) {
  UniqueConstraintValue constraint("http://example.org/email");

  std::vector<std::string> resource1Values = {"alice@example.org"};
  std::vector<std::string> resource2Values = {"alice@example.org"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                        resource1Values, &context));

  // Second resource with same email should violate uniqueness
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                         resource2Values, &context));
}

TEST_F(AdvancedConstraintsTest, UniqueConstraintMultipleValues) {
  UniqueConstraintValue constraint("http://example.org/tag");

  std::vector<std::string> resource1Values = {"tag1", "tag2", "tag3"};
  std::vector<std::string> resource2Values = {"tag4", "tag5", "tag2"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                        resource1Values, &context));

  // "tag2" is duplicated
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateUnique(constraint,
                                                         resource2Values, &context));
}

TEST_F(AdvancedConstraintsTest, UniqueConstraintNoContext) {
  UniqueConstraintValue constraint("http://example.org/email");
  std::vector<std::string> values = {"test@example.org"};

  // Without context, should return true (cannot validate)
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint, values, nullptr));
}

// ============================================================================
// sh:disjointWith Constraint Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, DisjointWithNoViolation) {
  DisjointWithConstraintValue constraint("http://example.org/prop1",
                                          "http://example.org/prop2");

  std::vector<std::string> values1 = {"value1", "value2"};
  std::vector<std::string> values2 = {"value3", "value4"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint,
                                                              values1, values2));
}

TEST_F(AdvancedConstraintsTest, DisjointWithViolation) {
  DisjointWithConstraintValue constraint("http://example.org/prop1",
                                          "http://example.org/prop2");

  std::vector<std::string> values1 = {"value1", "value2", "shared"};
  std::vector<std::string> values2 = {"value3", "shared", "value4"};

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint,
                                                               values1, values2));
}

TEST_F(AdvancedConstraintsTest, DisjointWithEmptyValues) {
  DisjointWithConstraintValue constraint("http://example.org/prop1",
                                          "http://example.org/prop2");

  std::vector<std::string> values1 = {"value1"};
  std::vector<std::string> values2 = {};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint,
                                                              values1, values2));
}

// ============================================================================
// sh:closed with sh:ignoredProperties Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, ClosedShapeAllAllowed) {
  ClosedConstraintValue constraint(true);
  constraint.allowedProperties = {"http://example.org/prop1",
                                   "http://example.org/prop2"};

  std::unordered_set<std::string> properties = {"http://example.org/prop1",
                                                 "http://example.org/prop2"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateClosed(constraint, properties));
}

TEST_F(AdvancedConstraintsTest, ClosedShapeUnexpectedProperty) {
  ClosedConstraintValue constraint(true);
  constraint.allowedProperties = {"http://example.org/prop1"};

  std::unordered_set<std::string> properties = {
      "http://example.org/prop1", "http://example.org/unexpected"};

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateClosed(constraint, properties));
}

TEST_F(AdvancedConstraintsTest, ClosedShapeWithIgnoredProperties) {
  ClosedConstraintValue constraint(true);
  constraint.allowedProperties = {"http://example.org/prop1"};
  constraint.ignoredProperties = {"http://www.w3.org/1999/02/22-rdf-syntax-ns#type"};

  std::unordered_set<std::string> properties = {
      "http://example.org/prop1",
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateClosed(constraint, properties));
}

TEST_F(AdvancedConstraintsTest, ClosedShapeNotClosed) {
  ClosedConstraintValue constraint(false);
  constraint.allowedProperties = {"http://example.org/prop1"};

  std::unordered_set<std::string> properties = {"http://example.org/prop1",
                                                 "http://example.org/prop2",
                                                 "http://example.org/prop3"};

  // Not closed, so any properties allowed
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateClosed(constraint, properties));
}

// ============================================================================
// sh:hasValue Constraint Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, HasValueFound) {
  HasValueConstraintValue constraint("requiredValue");

  std::vector<std::string> values = {"otherValue", "requiredValue",
                                     "anotherValue"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateHasValue(constraint, values));
}

TEST_F(AdvancedConstraintsTest, HasValueNotFound) {
  HasValueConstraintValue constraint("requiredValue");

  std::vector<std::string> values = {"otherValue", "anotherValue"};

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateHasValue(constraint, values));
}

TEST_F(AdvancedConstraintsTest, HasValueEmptyList) {
  HasValueConstraintValue constraint("requiredValue");

  std::vector<std::string> values = {};

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateHasValue(constraint, values));
}

TEST_F(AdvancedConstraintsTest, HasValueExactMatch) {
  HasValueConstraintValue constraint("\"42\"^^<http://www.w3.org/2001/XMLSchema#integer>");

  std::vector<std::string> values = {
      "\"41\"^^<http://www.w3.org/2001/XMLSchema#integer>",
      "\"42\"^^<http://www.w3.org/2001/XMLSchema#integer>"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateHasValue(constraint, values));
}

// ============================================================================
// sh:minExclusive Constraint Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, MinExclusiveValid) {
  MinExclusiveConstraintValue constraint(10.0);

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"15.5\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"11\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"100\""));
}

TEST_F(AdvancedConstraintsTest, MinExclusiveInvalid) {
  MinExclusiveConstraintValue constraint(10.0);

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"10\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"9.9\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"5\""));
}

TEST_F(AdvancedConstraintsTest, MinExclusiveBoundaryCase) {
  MinExclusiveConstraintValue constraint(10.0);

  // Exactly at boundary - should fail (exclusive)
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"10.0\""));

  // Just above boundary - should pass
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(constraint, "\"10.01\""));
}

// ============================================================================
// sh:maxExclusive Constraint Tests
// ============================================================================

TEST_F(AdvancedConstraintsTest, MaxExclusiveValid) {
  MaxExclusiveConstraintValue constraint(100.0);

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"50\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"99.9\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"0\""));
}

TEST_F(AdvancedConstraintsTest, MaxExclusiveInvalid) {
  MaxExclusiveConstraintValue constraint(100.0);

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"100\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"100.1\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"200\""));
}

TEST_F(AdvancedConstraintsTest, MaxExclusiveBoundaryCase) {
  MaxExclusiveConstraintValue constraint(100.0);

  // Exactly at boundary - should fail (exclusive)
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"100.0\""));

  // Just below boundary - should pass
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(constraint, "\"99.99\""));
}

// ============================================================================
// Integration Tests: Complex Scenarios
// ============================================================================

TEST_F(AdvancedConstraintsTest, IntegrationUniqueAcrossMultipleResources) {
  UniqueConstraintValue emailConstraint("http://example.org/email");

  // Validate 5 resources with unique emails
  for (int i = 1; i <= 5; ++i) {
    std::vector<std::string> values = {
        "user" + std::to_string(i) + "@example.org"};
    EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(emailConstraint,
                                                          values, &context));
  }

  // 6th resource with duplicate email
  std::vector<std::string> duplicateValues = {"user3@example.org"};
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateUnique(emailConstraint,
                                                         duplicateValues, &context));
}

TEST_F(AdvancedConstraintsTest, IntegrationClosedShapeComplex) {
  ClosedConstraintValue constraint(true);
  constraint.allowedProperties = {"http://schema.org/name",
                                   "http://schema.org/email"};
  constraint.ignoredProperties = {
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
      "http://www.w3.org/2000/01/rdf-schema#label"};

  // Valid: only allowed and ignored properties
  std::unordered_set<std::string> validProperties = {
      "http://schema.org/name", "http://schema.org/email",
      "http://www.w3.org/1999/02/22-rdf-syntax-ns#type"};
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateClosed(constraint, validProperties));

  // Invalid: contains unexpected property
  std::unordered_set<std::string> invalidProperties = {
      "http://schema.org/name", "http://schema.org/email",
      "http://schema.org/telephone"};
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateClosed(constraint, invalidProperties));
}

TEST_F(AdvancedConstraintsTest, IntegrationNumericRangeExclusive) {
  MinExclusiveConstraintValue minConstraint(0.0);
  MaxExclusiveConstraintValue maxConstraint(100.0);

  // Valid range (0 < value < 100)
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint, "\"0.1\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint, "\"99.9\""));

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint, "\"50\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint, "\"50\""));

  // Invalid: at boundaries
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint, "\"0\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint, "\"100\""));

  // Invalid: outside range
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint, "\"-5\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint, "\"150\""));
}

TEST_F(AdvancedConstraintsTest, IntegrationDisjointMultipleProperties) {
  DisjointWithConstraintValue constraint1("prop1", "prop2");
  DisjointWithConstraintValue constraint2("prop1", "prop3");

  std::vector<std::string> prop1Values = {"a", "b", "c"};
  std::vector<std::string> prop2Values = {"d", "e", "f"};
  std::vector<std::string> prop3Values = {"g", "h", "i"};
  std::vector<std::string> prop4Values = {"a", "x", "y"};

  // prop1 and prop2 are disjoint
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint1,
                                                              prop1Values, prop2Values));

  // prop1 and prop3 are disjoint
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint2,
                                                              prop1Values, prop3Values));

  // prop1 and prop4 have overlap (value "a")
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateDisjointWith(constraint1,
                                                               prop1Values, prop4Values));
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(AdvancedConstraintsTest, EdgeCaseEmptyPropertyPath) {
  UniqueConstraintValue constraint("");
  std::vector<std::string> values = {"value1"};

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateUnique(constraint, values, &context));
}

TEST_F(AdvancedConstraintsTest, EdgeCaseNonNumericValueForExclusive) {
  MinExclusiveConstraintValue minConstraint(10.0);
  MaxExclusiveConstraintValue maxConstraint(100.0);

  // Non-numeric values should fail gracefully
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint,
                                                               "\"not a number\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint,
                                                               "\"not a number\""));
}

TEST_F(AdvancedConstraintsTest, EdgeCaseVeryLargeNumbers) {
  MinExclusiveConstraintValue minConstraint(1e100);
  MaxExclusiveConstraintValue maxConstraint(1e-100);

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint,
                                                              "\"1e101\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint,
                                                              "\"1e-101\""));
}

TEST_F(AdvancedConstraintsTest, EdgeCaseNegativeNumbers) {
  MinExclusiveConstraintValue minConstraint(-100.0);
  MaxExclusiveConstraintValue maxConstraint(-10.0);

  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint,
                                                              "\"-50\""));
  EXPECT_TRUE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint,
                                                              "\"-50\""));

  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMinExclusive(minConstraint,
                                                               "\"-100\""));
  EXPECT_FALSE(ShaclConstraintEvaluator::evaluateMaxExclusive(maxConstraint,
                                                               "\"-10\""));
}

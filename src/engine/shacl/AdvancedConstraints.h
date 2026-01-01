#ifndef QLEVER_ENGINE_SHACL_ADVANCEDCONSTRAINTS_H
#define QLEVER_ENGINE_SHACL_ADVANCEDCONSTRAINTS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shacl {

// Advanced constraint value types

// sh:unique - Ensures no duplicate values across resources for a property
struct UniqueConstraintValue {
  std::string propertyPath;
  std::string message = "Duplicate value found for unique property";

  UniqueConstraintValue() = default;
  explicit UniqueConstraintValue(const std::string& path) : propertyPath(path) {}
};

// sh:disjointWith - Two properties cannot both have values on the same node
struct DisjointWithConstraintValue {
  std::string property1;
  std::string property2;
  std::string message = "Properties have overlapping values";

  DisjointWithConstraintValue() = default;
  DisjointWithConstraintValue(const std::string& p1, const std::string& p2)
      : property1(p1), property2(p2) {}
};

// sh:closed with sh:ignoredProperties
struct ClosedConstraintValue {
  bool closed = true;
  std::vector<std::string> ignoredProperties;
  std::vector<std::string> allowedProperties;
  std::string message = "Unexpected property found on closed shape";

  ClosedConstraintValue() = default;
  explicit ClosedConstraintValue(bool c) : closed(c) {}
};

// sh:hasValue - Property must have a specific literal value
struct HasValueConstraintValue {
  std::string requiredValue;
  std::string message = "Required value not found";

  HasValueConstraintValue() = default;
  explicit HasValueConstraintValue(const std::string& value)
      : requiredValue(value) {}
};

// sh:minExclusive - Value must be strictly greater than minimum
struct MinExclusiveConstraintValue {
  double minValue;
  std::string message = "Value is not greater than minimum";

  MinExclusiveConstraintValue() : minValue(0.0) {}
  explicit MinExclusiveConstraintValue(double min) : minValue(min) {}
};

// sh:maxExclusive - Value must be strictly less than maximum
struct MaxExclusiveConstraintValue {
  double maxValue;
  std::string message = "Value is not less than maximum";

  MaxExclusiveConstraintValue() : maxValue(0.0) {}
  explicit MaxExclusiveConstraintValue(double max) : maxValue(max) {}
};

// Resource-level state tracker for uniqueness checking
class ResourceStateTracker {
 public:
  // Track a value for a property on a resource
  // Returns false if the value already exists (duplicate)
  bool trackUniqueValue(const std::string& propertyPath,
                        const std::string& value);

  // Check if a value is already tracked for a property
  bool hasValue(const std::string& propertyPath, const std::string& value) const;

  // Get all values for a property
  std::vector<std::string> getValues(const std::string& propertyPath) const;

  // Check if two properties have any overlapping values
  bool hasDisjointViolation(const std::string& property1,
                            const std::string& property2,
                            const std::vector<std::string>& values1,
                            const std::vector<std::string>& values2) const;

  // Clear all tracked state
  void clear();

  // Get number of tracked properties
  size_t propertyCount() const { return propertyValues_.size(); }

  // Get number of unique values for a property
  size_t valueCount(const std::string& propertyPath) const;

 private:
  // Map of property path -> set of values across all resources
  std::unordered_map<std::string, std::unordered_set<std::string>>
      propertyValues_;
};

// Evaluation context for advanced constraints
struct AdvancedConstraintContext {
  // Shared state tracker across all validations
  ResourceStateTracker* stateTracker = nullptr;

  // Current resource being validated
  std::string currentResource;

  // All properties present on the current resource
  std::unordered_set<std::string> currentResourceProperties;

  AdvancedConstraintContext() = default;
  explicit AdvancedConstraintContext(ResourceStateTracker* tracker)
      : stateTracker(tracker) {}
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_ADVANCEDCONSTRAINTS_H

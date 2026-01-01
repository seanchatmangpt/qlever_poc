#ifndef QLEVER_ENGINE_SHACL_SHACLSHAPE_H
#define QLEVER_ENGINE_SHACL_SHACLSHAPE_H

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

#include "engine/shacl/ComplexPropertyPaths.h"

namespace shacl {

// Forward declarations for advanced constraints
struct UniqueConstraintValue;
struct DisjointWithConstraintValue;
struct ClosedConstraintValue;
struct HasValueConstraintValue;
struct MinExclusiveConstraintValue;
struct MaxExclusiveConstraintValue;

// Forward declarations for logical shapes
class LogicalShapeConstraint;
class AndConstraint;
class OrConstraint;
class NotConstraint;
class XoneConstraint;

// Core SHACL constraint types (80/20: most common constraints)
enum class ConstraintType {
  // Cardinality constraints
  MinCount,
  MaxCount,

  // Value type constraints
  Datatype,
  NodeKind,

  // Value constraints
  MinInclusive,
  MaxInclusive,
  MinLength,
  MaxLength,
  Pattern,
  In,

  // Advanced (not in 80/20)
  Unique,
  DisjointWith,
  ClosedShape,
  HasValue,
  MinExclusive,
  MaxExclusive,

  // Recursive shape constraints
  Node,       // sh:node - validates a node with another shape
  Shape,      // sh:shape - recursive reference to another shape

  // SPARQL-based constraint
  Sparql      // sh:sparql - custom SPARQL constraint query
};

// Constraint severity levels
enum class SeverityLevel {
  Violation,
  Warning,
  Info
};

// SHACL NodeKind values
enum class NodeKind {
  BlankNode,
  IRI,
  Literal,
  BlankNodeOrIRI,
  BlankNodeOrLiteral,
  IRIOrLiteral
};

// Single SHACL constraint
struct ShaclConstraint {
  ConstraintType type;
  SeverityLevel severity = SeverityLevel::Violation;
  std::string message;

  // Constraint values (variant for different constraint types)
  std::variant<
      int,                           // MinCount, MaxCount, MinLength, MaxLength
      double,                        // MinExclusive, MaxExclusive (numeric)
      std::string,                   // Datatype, Pattern, HasValue
      NodeKind,                      // NodeKind
      std::vector<std::string>,      // In (allowed values)
      UniqueConstraintValue,         // sh:unique
      DisjointWithConstraintValue,   // sh:disjointWith
      ClosedConstraintValue,         // sh:closed
      HasValueConstraintValue,       // sh:hasValue
      MinExclusiveConstraintValue,   // sh:minExclusive
      MaxExclusiveConstraintValue    // sh:maxExclusive
  > value;

  ShaclConstraint() = default;
  ShaclConstraint(ConstraintType t) : type(t) {}
};

// Represents a property shape (constraints on a property)
struct PropertyShape {
  // Property path - can be simple IRI or complex path expression
  // For backward compatibility, we store both representations
  std::string path;  // Simple property IRI (deprecated, use propertyPath)
  PropertyPath propertyPath;  // Complex property path (sh:path)

  std::vector<ShaclConstraint> constraints;
  bool required = false;  // sh:minCount >= 1

  PropertyShape() = default;

  // Constructor for simple path (backward compatibility)
  explicit PropertyShape(const std::string& p)
      : path(p), propertyPath(PropertyPath::simple(p)) {}

  // Constructor for complex path
  explicit PropertyShape(PropertyPath p)
      : propertyPath(std::move(p)) {
    // Set legacy path field if it's a simple path
    if (propertyPath.isSimple()) {
      path = propertyPath.getSimpleIri();
    }
  }

  // Check if this property shape uses a complex path
  bool hasComplexPath() const { return !propertyPath.isSimple(); }

  // Get the property path (prefer this over direct path access)
  const PropertyPath& getPropertyPath() const { return propertyPath; }
};

// Represents a node shape (constraints on a node/resource)
class NodeShape {
 public:
  std::string shapeId;                              // Shape IRI
  std::vector<std::string> targetClasses;           // sh:targetClass
  std::vector<std::string> targetNodes;             // sh:targetNode
  std::vector<PropertyShape> propertyShapes;        // sh:property
  std::vector<ShaclConstraint> nodeConstraints;     // Node-level constraints
  bool closed = false;                              // sh:closed

  // Logical shape constraints (sh:and, sh:or, sh:not, sh:xone)
  std::shared_ptr<AndConstraint> andConstraint;
  std::shared_ptr<OrConstraint> orConstraint;
  std::shared_ptr<NotConstraint> notConstraint;
  std::shared_ptr<XoneConstraint> xoneConstraint;

  // Getters
  bool hasTargets() const {
    return !targetClasses.empty() || !targetNodes.empty();
  }

  bool isTargetNode(const std::string& nodeIri) const {
    return std::find(targetNodes.begin(), targetNodes.end(), nodeIri)
           != targetNodes.end();
  }

  bool isTargetClass(const std::string& classIri) const {
    return std::find(targetClasses.begin(), targetClasses.end(), classIri)
           != targetClasses.end();
  }

  // Builders for fluent API
  NodeShape& addTargetClass(const std::string& classIri) {
    targetClasses.push_back(classIri);
    return *this;
  }

  NodeShape& addPropertyShape(const PropertyShape& prop) {
    propertyShapes.push_back(prop);
    return *this;
  }

  NodeShape& addConstraint(const ShaclConstraint& constraint) {
    nodeConstraints.push_back(constraint);
    return *this;
  }

  // Logical constraint setters
  NodeShape& setAndConstraint(std::shared_ptr<AndConstraint> constraint) {
    andConstraint = std::move(constraint);
    return *this;
  }

  NodeShape& setOrConstraint(std::shared_ptr<OrConstraint> constraint) {
    orConstraint = std::move(constraint);
    return *this;
  }

  NodeShape& setNotConstraint(std::shared_ptr<NotConstraint> constraint) {
    notConstraint = std::move(constraint);
    return *this;
  }

  NodeShape& setXoneConstraint(std::shared_ptr<XoneConstraint> constraint) {
    xoneConstraint = std::move(constraint);
    return *this;
  }

  // Check if shape has any logical constraints
  bool hasLogicalConstraints() const {
    return andConstraint || orConstraint || notConstraint || xoneConstraint;
  }
};

// Validation result for a single resource
struct ValidationResult {
  std::string focusNode;  // The node being validated
  bool conforms = true;
  std::vector<std::string> violations;  // List of violation messages

  void addViolation(const std::string& message) {
    conforms = false;
    violations.push_back(message);
  }
};

// Overall validation report
struct ValidationReport {
  bool conforms = true;
  size_t violationCount = 0;
  std::unordered_map<std::string, ValidationResult> results;
                                          // focusNode -> ValidationResult

  void addResult(const ValidationResult& result) {
    results[result.focusNode] = result;
    if (!result.conforms) {
      conforms = false;
      violationCount += result.violations.size();
    }
  }

  // Summary statistics
  size_t conformingCount() const {
    size_t count = 0;
    for (const auto& [_, result] : results) {
      if (result.conforms) count++;
    }
    return count;
  }
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLSHAPE_H

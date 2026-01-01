#ifndef QLEVER_ENGINE_SHACL_SHACLSHAPE_H
#define QLEVER_ENGINE_SHACL_SHACLSHAPE_H

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

namespace shacl {

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
  ClosedShape
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
      int,                    // MinCount, MaxCount, MinLength, MaxLength
      std::string,            // Datatype, Pattern
      NodeKind,               // NodeKind
      std::vector<std::string> // In (allowed values)
  > value;

  ShaclConstraint() = default;
  ShaclConstraint(ConstraintType t) : type(t) {}
};

// Represents a property shape (constraints on a property)
struct PropertyShape {
  std::string path;  // Property IRI (sh:path)
  std::vector<ShaclConstraint> constraints;
  bool required = false;  // sh:minCount >= 1

  PropertyShape() = default;
  explicit PropertyShape(const std::string& p) : path(p) {}
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

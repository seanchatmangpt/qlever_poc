#ifndef QLEVER_ENGINE_SHEX_SHEXTYPES_H
#define QLEVER_ENGINE_SHEX_SHEXTYPES_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace shex {

// Forward declarations
class ShapeExpr;
class TripleConstraint;

// ShEx node kind enumeration
enum class ShExNodeKind {
  IRI,
  BlankNode,
  Literal,
  NonLiteral  // IRI or BlankNode
};

// ShEx value type (for datatype constraints)
struct ValueType {
  std::string datatype;  // XSD datatype IRI

  ValueType() = default;
  explicit ValueType(std::string dt) : datatype(std::move(dt)) {}

  bool operator==(const ValueType& other) const {
    return datatype == other.datatype;
  }
};

// Triple constraint - constraints on a property
class TripleConstraint {
 public:
  std::string predicate;  // Property IRI
  std::optional<std::shared_ptr<ShapeExpr>>
      valueExpr;                       // Optional nested shape for value
  std::optional<int> minCount;         // Minimum cardinality (default: 1)
  std::optional<int> maxCount;         // Maximum cardinality (default: 1)
  std::optional<ValueType> valueType;  // Datatype constraint
  std::optional<ShExNodeKind> nodeKind;  // Node kind constraint

  TripleConstraint() = default;
  explicit TripleConstraint(std::string pred) : predicate(std::move(pred)) {}

  // Get effective min count (default 1 if not specified)
  int getMinCount() const { return minCount.value_or(1); }

  // Get effective max count (default 1 if not specified, -1 for unbounded)
  int getMaxCount() const { return maxCount.value_or(1); }

  // Check if this constraint has a nested shape
  bool hasNestedShape() const { return valueExpr.has_value(); }
};

// Shape expression - represents a ShEx shape
class ShapeExpr {
 public:
  std::string shapeId;  // Shape IRI
  std::vector<TripleConstraint> tripleConstraints;
  bool closed = false;                    // CLOSED constraint
  std::vector<std::string> extraProps;    // EXTRA properties (allowed when CLOSED)

  ShapeExpr() = default;
  explicit ShapeExpr(std::string id) : shapeId(std::move(id)) {}

  // Builder methods for fluent API
  ShapeExpr& addTripleConstraint(const TripleConstraint& tc) {
    tripleConstraints.push_back(tc);
    return *this;
  }

  ShapeExpr& setClosed(bool isClosed) {
    closed = isClosed;
    return *this;
  }

  ShapeExpr& addExtraProperty(const std::string& prop) {
    extraProps.push_back(prop);
    return *this;
  }

  // Check if a property is allowed (for CLOSED shapes)
  bool isPropertyAllowed(const std::string& prop) const {
    // If not closed, all properties are allowed
    if (!closed) {
      return true;
    }

    // Check if property is in EXTRA list
    for (const auto& extra : extraProps) {
      if (extra == prop) {
        return true;
      }
    }

    // Check if property is declared in triple constraints
    for (const auto& tc : tripleConstraints) {
      if (tc.predicate == prop) {
        return true;
      }
    }

    return false;
  }
};

// Shape map - maps shape IDs to shapes
class ShapeMap {
 public:
  std::unordered_map<std::string, std::shared_ptr<ShapeExpr>> shapes;

  // Add a shape to the map
  void addShape(std::shared_ptr<ShapeExpr> shape) {
    if (shape && !shape->shapeId.empty()) {
      shapes[shape->shapeId] = shape;
    }
  }

  // Get a shape by ID
  std::shared_ptr<ShapeExpr> getShape(const std::string& shapeId) const {
    auto it = shapes.find(shapeId);
    return (it != shapes.end()) ? it->second : nullptr;
  }

  // Check if a shape exists
  bool hasShape(const std::string& shapeId) const {
    return shapes.find(shapeId) != shapes.end();
  }

  // Get all shape IDs
  std::vector<std::string> getShapeIds() const {
    std::vector<std::string> ids;
    ids.reserve(shapes.size());
    for (const auto& [id, _] : shapes) {
      ids.push_back(id);
    }
    return ids;
  }

  // Clear all shapes
  void clear() { shapes.clear(); }

  // Get number of shapes
  size_t size() const { return shapes.size(); }

  // Check if empty
  bool empty() const { return shapes.empty(); }
};

// Validation result for a single node
struct ValidationResult {
  std::string focusNode;  // The node being validated
  std::string shapeId;    // The shape it was validated against
  bool conforms = true;   // Whether validation passed
  std::vector<std::string> violations;  // List of violation messages

  ValidationResult() = default;
  ValidationResult(std::string node, std::string shape)
      : focusNode(std::move(node)), shapeId(std::move(shape)) {}

  // Add a violation
  void addViolation(const std::string& message) {
    conforms = false;
    violations.push_back(message);
  }

  // Check if valid
  bool isValid() const { return conforms; }
};

// Overall validation report
struct ValidationReport {
  bool conforms = true;
  size_t violationCount = 0;
  std::vector<ValidationResult> results;

  // Add a validation result
  void addResult(const ValidationResult& result) {
    results.push_back(result);
    if (!result.conforms) {
      conforms = false;
      violationCount += result.violations.size();
    }
  }

  // Get summary statistics
  size_t conformingCount() const {
    size_t count = 0;
    for (const auto& result : results) {
      if (result.conforms) {
        count++;
      }
    }
    return count;
  }

  size_t nonConformingCount() const {
    return results.size() - conformingCount();
  }

  // Clear all results
  void clear() {
    conforms = true;
    violationCount = 0;
    results.clear();
  }
};

}  // namespace shex

#endif  // QLEVER_ENGINE_SHEX_SHEXTYPES_H

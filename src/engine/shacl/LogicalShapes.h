#ifndef QLEVER_ENGINE_SHACL_LOGICALSHAPES_H
#define QLEVER_ENGINE_SHACL_LOGICALSHAPES_H

#include <memory>
#include <string>
#include <vector>
#include "ShaclShape.h"

namespace shacl {

// Forward declaration
class NodeShape;
class ShaclShapeRegistry;

// Logical constraint types for shape composition
enum class LogicalConstraintType { And, Or, Not, Xone };

// Represents a reference to a shape (either by ID or inline shape)
struct ShapeReference {
  std::string shapeId;                   // Referenced shape ID (if not inline)
  std::shared_ptr<NodeShape> inlineShape;  // Inline shape definition (if any)

  // Constructor for shape ID reference
  explicit ShapeReference(const std::string& id) : shapeId(id) {}

  // Constructor for inline shape
  explicit ShapeReference(std::shared_ptr<NodeShape> shape)
      : inlineShape(std::move(shape)) {}

  // Check if this is an inline shape
  bool isInline() const { return inlineShape != nullptr; }

  // Get the shape (requires registry for ID-based references)
  const NodeShape* getShape(const ShaclShapeRegistry* registry) const;
};

// Base class for logical shape constraints
class LogicalShapeConstraint {
 public:
  LogicalConstraintType type;
  std::vector<ShapeReference> shapes;  // Referenced shapes

  explicit LogicalShapeConstraint(LogicalConstraintType t) : type(t) {}
  virtual ~LogicalShapeConstraint() = default;

  // Add a shape reference by ID
  void addShapeReference(const std::string& shapeId) {
    shapes.emplace_back(shapeId);
  }

  // Add an inline shape
  void addInlineShape(std::shared_ptr<NodeShape> shape) {
    shapes.emplace_back(std::move(shape));
  }

  // Evaluate the logical constraint against a focus node
  // Returns true if the logical constraint is satisfied
  virtual ValidationResult evaluate(
      const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const = 0;

 protected:
  // Helper to evaluate a single shape reference
  ValidationResult evaluateShape(
      const ShapeReference& shapeRef, const std::string& focusNode,
      const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const;
};

// sh:and - All shapes must conform (conjunction)
class AndConstraint : public LogicalShapeConstraint {
 public:
  AndConstraint() : LogicalShapeConstraint(LogicalConstraintType::And) {}

  ValidationResult evaluate(
      const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const override;
};

// sh:or - At least one shape must conform (disjunction)
class OrConstraint : public LogicalShapeConstraint {
 public:
  OrConstraint() : LogicalShapeConstraint(LogicalConstraintType::Or) {}

  ValidationResult evaluate(
      const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const override;
};

// sh:not - Shape must not conform (negation)
class NotConstraint : public LogicalShapeConstraint {
 public:
  NotConstraint() : LogicalShapeConstraint(LogicalConstraintType::Not) {}

  ValidationResult evaluate(
      const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const override;
};

// sh:xone - Exactly one shape must conform (exclusive or)
class XoneConstraint : public LogicalShapeConstraint {
 public:
  XoneConstraint() : LogicalShapeConstraint(LogicalConstraintType::Xone) {}

  ValidationResult evaluate(
      const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
      const ShaclShapeRegistry* registry,
      const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
      const override;
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_LOGICALSHAPES_H

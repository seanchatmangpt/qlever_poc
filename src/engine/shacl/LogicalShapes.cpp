#include "LogicalShapes.h"
#include "ShaclConstraintEvaluator.h"
#include "ShaclShapeRegistry.h"

namespace shacl {

// Get the shape from a reference (resolve ID if needed)
const NodeShape* ShapeReference::getShape(
    const ShaclShapeRegistry* registry) const {
  if (isInline()) {
    return inlineShape.get();
  }
  if (registry && !shapeId.empty()) {
    return registry->getShape(shapeId);
  }
  return nullptr;
}

// Helper to evaluate a single shape reference
ValidationResult LogicalShapeConstraint::evaluateShape(
    const ShapeReference& shapeRef, const std::string& focusNode,
    const IdTable& inputTable, size_t rowIndex,
    const ShaclShapeRegistry* registry,
    const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
    const {
  ValidationResult result;
  result.focusNode = focusNode;

  // Get the referenced shape
  const NodeShape* shape = shapeRef.getShape(registry);
  if (!shape) {
    result.addViolation("Referenced shape not found: " + shapeRef.shapeId);
    return result;
  }

  // Evaluate property shapes
  for (const auto& propShape : shape->propertyShapes) {
    std::vector<std::string> values;

    // Get values for this property from the input table
    auto it = propertyColumns.find(propShape.path);
    if (it != propertyColumns.end()) {
      auto colIdx = it->second;
      if (colIdx < inputTable.getWidth()) {
        auto val = inputTable.getEntry(rowIndex, colIdx);
        if (val != 0) {  // 0 is typically NULL/undefined
          values.push_back(std::to_string(val));
        }
      }
    }

    // Validate property constraints
    auto propResult = ShaclConstraintEvaluator::evaluatePropertyShape(
        focusNode, propShape, values);
    if (!propResult.conforms) {
      result.conforms = false;
      for (const auto& violation : propResult.violations) {
        result.violations.push_back(violation);
      }
    }
  }

  // Evaluate node-level constraints
  for (const auto& constraint : shape->nodeConstraints) {
    // Get the value for node-level validation (the focus node itself)
    if (!ShaclConstraintEvaluator::evaluateConstraint(constraint, focusNode)) {
      result.addViolation("Node constraint violated: " + constraint.message);
    }
  }

  return result;
}

// sh:and - All shapes must conform (conjunction)
// Short-circuit: stop on first failure
ValidationResult AndConstraint::evaluate(
    const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
    const ShaclShapeRegistry* registry,
    const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
    const {
  ValidationResult result;
  result.focusNode = focusNode;

  // All shapes must conform - short-circuit on first failure
  for (const auto& shapeRef : shapes) {
    auto shapeResult = evaluateShape(shapeRef, focusNode, inputTable, rowIndex,
                                     registry, propertyColumns);

    if (!shapeResult.conforms) {
      // Short-circuit: first failure means AND fails
      result.conforms = false;
      result.addViolation("sh:and constraint violated - shape '" +
                          shapeRef.shapeId + "' did not conform");
      for (const auto& violation : shapeResult.violations) {
        result.violations.push_back("  " + violation);
      }
      return result;  // Early exit
    }
  }

  // All shapes conformed
  return result;
}

// sh:or - At least one shape must conform (disjunction)
// Short-circuit: stop on first success
ValidationResult OrConstraint::evaluate(
    const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
    const ShaclShapeRegistry* registry,
    const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
    const {
  ValidationResult result;
  result.focusNode = focusNode;

  std::vector<std::string> allViolations;

  // At least one shape must conform - short-circuit on first success
  for (const auto& shapeRef : shapes) {
    auto shapeResult = evaluateShape(shapeRef, focusNode, inputTable, rowIndex,
                                     registry, propertyColumns);

    if (shapeResult.conforms) {
      // Short-circuit: first success means OR succeeds
      return result;  // Early exit with conforming result
    }

    // Collect violations for error message
    for (const auto& violation : shapeResult.violations) {
      allViolations.push_back(violation);
    }
  }

  // No shapes conformed
  result.conforms = false;
  result.addViolation("sh:or constraint violated - none of the shapes "
                      "conformed");
  for (const auto& violation : allViolations) {
    result.violations.push_back("  " + violation);
  }

  return result;
}

// sh:not - Shape must not conform (negation)
ValidationResult NotConstraint::evaluate(
    const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
    const ShaclShapeRegistry* registry,
    const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
    const {
  ValidationResult result;
  result.focusNode = focusNode;

  if (shapes.empty()) {
    result.addViolation("sh:not constraint requires exactly one shape");
    return result;
  }

  if (shapes.size() > 1) {
    result.addViolation(
        "sh:not constraint can only have one shape (found " +
        std::to_string(shapes.size()) + ")");
    return result;
  }

  // Evaluate the negated shape
  auto shapeResult = evaluateShape(shapes[0], focusNode, inputTable, rowIndex,
                                   registry, propertyColumns);

  // Negate the result: if shape conforms, NOT fails
  if (shapeResult.conforms) {
    result.conforms = false;
    result.addViolation("sh:not constraint violated - shape '" +
                        shapes[0].shapeId + "' should not have conformed");
  }
  // If shape doesn't conform, NOT succeeds (result.conforms is already true)

  return result;
}

// sh:xone - Exactly one shape must conform (exclusive or)
ValidationResult XoneConstraint::evaluate(
    const std::string& focusNode, const IdTable& inputTable, size_t rowIndex,
    const ShaclShapeRegistry* registry,
    const std::unordered_map<std::string, ColumnIndex>& propertyColumns)
    const {
  ValidationResult result;
  result.focusNode = focusNode;

  int conformingCount = 0;
  std::vector<std::string> conformingShapes;

  // Count how many shapes conform (no short-circuit for xone)
  for (const auto& shapeRef : shapes) {
    auto shapeResult = evaluateShape(shapeRef, focusNode, inputTable, rowIndex,
                                     registry, propertyColumns);

    if (shapeResult.conforms) {
      conformingCount++;
      conformingShapes.push_back(shapeRef.shapeId);
    }
  }

  // Exactly one shape must conform
  if (conformingCount != 1) {
    result.conforms = false;
    if (conformingCount == 0) {
      result.addViolation(
          "sh:xone constraint violated - none of the shapes conformed "
          "(expected exactly 1)");
    } else {
      result.addViolation("sh:xone constraint violated - " +
                          std::to_string(conformingCount) +
                          " shapes conformed (expected exactly 1)");
      for (const auto& shapeName : conformingShapes) {
        result.violations.push_back("  Conforming shape: " + shapeName);
      }
    }
  }

  return result;
}

}  // namespace shacl

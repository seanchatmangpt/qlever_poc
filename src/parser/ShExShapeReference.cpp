#include "ShExShapeReference.h"
#include "ShEx.h"

#include <algorithm>
#include <sstream>

namespace shex {

// ============================================================================
// ShapeDependencyGraph Implementation
// ============================================================================

// Implementation is in the header (inline)

// ============================================================================
// Helper Functions for ShEx Schema
// ============================================================================

ShapeDependencyGraph ShExSchema::buildDependencyGraph() const {
  ShapeDependencyGraph graph;

  // Iterate through all shapes and their properties
  for (const auto& [shapeId, shape] : shapes_) {
    for (const auto& prop : shape.properties) {
      if (prop.isShapeReference()) {
        auto refId = prop.getReferencedShapeId();
        if (refId.has_value()) {
          graph.addDependency(shapeId, refId.value());
        }
      }
    }

    // Also check EXTENDS relationship (from Phase 2B)
    if (shape.extendsShapeId_.has_value()) {
      graph.addDependency(shapeId, shape.extendsShapeId_.value());
    }
  }

  return graph;
}

std::vector<std::string> ShExSchema::validateShapeReferences() {
  std::vector<std::string> errors;

  // Check all shape references point to existing shapes
  for (const auto& [shapeId, shape] : shapes_) {
    for (const auto& prop : shape.properties) {
      if (prop.isShapeReference()) {
        auto refId = prop.getReferencedShapeId();
        if (refId.has_value()) {
          if (!hasShape(refId.value())) {
            std::ostringstream oss;
            oss << "Shape '" << shapeId << "' references non-existent shape '"
                << refId.value() << "' in property '" << prop.predicate << "'";
            errors.push_back(oss.str());
          }
        }
      }
    }

    // Check EXTENDS references
    if (shape.extendsShapeId_.has_value()) {
      if (!hasShape(shape.extendsShapeId_.value())) {
        std::ostringstream oss;
        oss << "Shape '" << shapeId << "' extends non-existent shape '"
            << shape.extendsShapeId_.value() << "'";
        errors.push_back(oss.str());
      }
    }
  }

  // Detect cycles and mark recursive shapes
  auto graph = buildDependencyGraph();
  auto cycles = graph.detectCycles();

  for (const auto& [shapeId, cyclePath] : cycles) {
    // Mark all shapes in the cycle as recursive
    auto it = shapes_.find(shapeId);
    if (it != shapes_.end()) {
      Shape& shape = it->second;
      for (auto& prop : shape.properties) {
        if (prop.isShapeReference()) {
          auto& shapeRef = const_cast<ShapeReference&>(prop.getShapeReference());
          // Check if the referenced shape is in the cycle
          if (std::find(cyclePath.begin(), cyclePath.end(),
                       shapeRef.targetShapeId) != cyclePath.end()) {
            shapeRef.isRecursive = true;
          }
        }
      }
    }
  }

  return errors;
}

// ============================================================================
// Shape Validation with ValidationContext
// ============================================================================

Shape::ValidationResult Shape::validate(
    const std::map<std::string,
                  std::vector<std::pair<std::string, ValueType>>>& nodeData,
    ValidationContext& context,
    const ShExSchema& schema) const {

  ValidationResult result{true, {}, {}, {}};

  // Enter this shape's validation context
  ValidationGuard guard(context, id);
  if (!guard.isValid()) {
    // Cycle detected - for recursive shapes, assume optimistic validation
    // (this is standard ShEx behavior for recursive references)
    result.errors.push_back("Recursive validation cycle detected for shape: " + id);
    // Note: This is not necessarily an error - recursive shapes are valid
    // We just note it and continue with optimistic validation
    return result;
  }

  // Check recursion depth limit
  if (context.isDepthExceeded()) {
    result.isValid = false;
    result.errors.push_back("Maximum recursion depth exceeded for shape: " + id);
    return result;
  }

  // Check each property in the shape
  for (const auto& prop : properties) {
    auto it = nodeData.find(prop.predicate);

    // Check cardinality
    size_t count = (it != nodeData.end()) ? it->second.size() : 0;

    switch (prop.cardinality) {
      case Cardinality::EXACTLY_ONE:
        if (count != 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear exactly once (found " +
                                std::to_string(count) + ")");
          result.failedPredicates.insert(prop.predicate);
        }
        break;
      case Cardinality::ZERO_OR_ONE:
        if (count > 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at most once (found " +
                                std::to_string(count) + ")");
          result.failedPredicates.insert(prop.predicate);
        }
        break;
      case Cardinality::ZERO_OR_MORE:
        // Always valid
        break;
      case Cardinality::ONE_OR_MORE:
        if (count < 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at least once");
          result.failedPredicates.insert(prop.predicate);
        }
        break;
    }

    // Validate each value against constraints
    if (it != nodeData.end()) {
      for (const auto& [value, type] : it->second) {
        if (prop.isShapeReference()) {
          // Recursive shape validation
          auto refShapeId = prop.getReferencedShapeId();
          if (refShapeId.has_value()) {
            const Shape* refShape = schema.getShape(refShapeId.value());
            if (refShape) {
              // For shape references, the value should be a node IRI
              // We would need node data for that IRI to validate
              // For now, we just check that the shape exists
              // Full validation would require access to the full dataset
            } else {
              result.isValid = false;
              result.errors.push_back("Referenced shape not found: " +
                                    refShapeId.value());
            }
          }
        } else {
          // Value constraint validation
          if (!prop.validate(value, type)) {
            result.isValid = false;
            result.errors.push_back("Property " + prop.predicate +
                                  " value '" + value + "' does not match constraints");
            result.failedPredicates.insert(prop.predicate);
          }
        }
      }
    }
  }

  // Check for closed shape violations
  if (closed) {
    absl::flat_hash_set<std::string> declaredPredicates;
    for (const auto& prop : properties) {
      declaredPredicates.insert(prop.predicate);
    }

    for (const auto& [predicate, _] : nodeData) {
      bool isAllowed = declaredPredicates.contains(predicate) ||
                      extraPredicates_.contains(predicate);
      bool isForbidden = forbiddenExtraPredicates_.contains(predicate);

      if (!isAllowed || isForbidden) {
        result.isValid = false;
        result.errors.push_back("Unexpected predicate in closed shape: " +
                              predicate);
        result.unexpectedPredicates.insert(predicate);
      }
    }
  }

  return result;
}

// ============================================================================
// ShExValidator with ValidationContext
// ============================================================================

ShExValidator::ValidationReport ShExValidator::validateNode(
    const std::string& nodeIri,
    const std::string& targetShapeId,
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>&
        data,
    ValidationContext& context) {

  ValidationReport report{true, {}, {}};

  const Shape* shape = schema_.getShape(targetShapeId);
  if (!shape) {
    report.schemaErrors.push_back("Shape '" + targetShapeId + "' not found");
    report.conforms = false;
    return report;
  }

  auto result = shape->validate(data, context, schema_);
  report.conforms = result.isValid;
  report.nodeErrors[nodeIri] = result.errors;

  return report;
}

}  // namespace shex

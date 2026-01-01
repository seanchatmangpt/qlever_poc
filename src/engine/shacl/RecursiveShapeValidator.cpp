#include "RecursiveShapeValidator.h"
#include "ShaclConstraintEvaluator.h"
#include <algorithm>
#include <sstream>

namespace shacl {

// ============================================================================
// ValidationContext implementation
// ============================================================================

bool RecursiveShapeValidator::ValidationContext::isInValidationStack(
    const std::string& nodeId, const std::string& shapeId) const {
  for (const auto& [stackNodeId, stackShapeId] : validationStack) {
    if (stackNodeId == nodeId && stackShapeId == shapeId) {
      return true;
    }
  }
  return false;
}

void RecursiveShapeValidator::ValidationContext::pushValidation(
    const std::string& nodeId, const std::string& shapeId) {
  validationStack.emplace_back(nodeId, shapeId);
}

void RecursiveShapeValidator::ValidationContext::popValidation() {
  if (!validationStack.empty()) {
    validationStack.pop_back();
  }
}

std::string RecursiveShapeValidator::ValidationContext::getMemoKey(
    const std::string& nodeId, const std::string& shapeId) {
  return nodeId + "|" + shapeId;
}

bool RecursiveShapeValidator::ValidationContext::hasMemoized(
    const std::string& nodeId, const std::string& shapeId) const {
  auto key = getMemoKey(nodeId, shapeId);
  return memoCache.find(key) != memoCache.end();
}

const ValidationResult& RecursiveShapeValidator::ValidationContext::getMemoized(
    const std::string& nodeId, const std::string& shapeId) const {
  auto key = getMemoKey(nodeId, shapeId);
  return memoCache.at(key);
}

void RecursiveShapeValidator::ValidationContext::memoize(
    const std::string& nodeId, const std::string& shapeId,
    const ValidationResult& result) {
  auto key = getMemoKey(nodeId, shapeId);
  memoCache[key] = result;
}

// ============================================================================
// RecursiveShapeValidator implementation
// ============================================================================

RecursiveShapeValidator::RecursiveShapeValidator(
    const ShaclShapeRegistry* registry)
    : registry_(registry) {}

RecursiveShapeValidator::ValidationContext
RecursiveShapeValidator::createContext(size_t maxDepth) {
  ValidationContext ctx;
  ctx.maxDepth = maxDepth;
  return ctx;
}

void RecursiveShapeValidator::clearCache() {
  globalCacheSize_ = 0;
  cacheHits_ = 0;
  validationCount_ = 0;
}

bool RecursiveShapeValidator::wouldCreateCircularReference(
    const std::string& nodeId, const std::string& shapeId,
    const ValidationContext& context) const {
  return context.isInValidationStack(nodeId, shapeId);
}

ValidationResult RecursiveShapeValidator::validateNodeWithShape(
    const std::string& nodeId, const std::string& shapeId,
    ValidationContext& context) {
  validationCount_++;

  // Check memoization cache first
  if (context.hasMemoized(nodeId, shapeId)) {
    cacheHits_++;
    return context.getMemoized(nodeId, shapeId);
  }

  // Check for circular reference
  if (wouldCreateCircularReference(nodeId, shapeId, context)) {
    ValidationResult circularResult;
    circularResult.focusNode = nodeId;
    circularResult.addViolation("Circular reference detected: validating " +
                                nodeId + " with shape " + shapeId);
    return circularResult;
  }

  // Check recursion depth
  if (context.validationStack.size() >= context.maxDepth) {
    ValidationResult depthResult;
    depthResult.focusNode = nodeId;
    depthResult.addViolation("Maximum recursion depth exceeded (" +
                             std::to_string(context.maxDepth) + ")");
    return depthResult;
  }

  // Get the shape from registry
  const NodeShape* shape = registry_->getShape(shapeId);
  if (!shape) {
    ValidationResult notFoundResult;
    notFoundResult.focusNode = nodeId;
    notFoundResult.addViolation("Shape not found: " + shapeId);
    return notFoundResult;
  }

  // Use RAII guard to manage validation stack
  ValidationGuard guard(context, nodeId, shapeId);

  // Perform actual validation
  ValidationResult result = validateNodeWithShapeImpl(nodeId, *shape, context);

  // Memoize the result
  context.memoize(nodeId, shapeId, result);
  globalCacheSize_++;

  return result;
}

ValidationResult RecursiveShapeValidator::validateNodeConstraint(
    const std::string& nodeId, const std::string& targetShapeId,
    ValidationContext& context) {
  // sh:node constraint: the node itself must conform to the target shape
  return validateNodeWithShape(nodeId, targetShapeId, context);
}

ValidationResult RecursiveShapeValidator::validateShapeReference(
    const std::string& valueNodeId, const std::string& shapeId,
    ValidationContext& context) {
  // sh:shape constraint: validate a property value against a shape
  return validateNodeWithShape(valueNodeId, shapeId, context);
}

std::vector<ValidationResult> RecursiveShapeValidator::validateNodesWithShape(
    const std::vector<std::string>& nodeIds, const std::string& shapeId) {
  std::vector<ValidationResult> results;
  results.reserve(nodeIds.size());

  // Create a shared validation context for all nodes
  ValidationContext context = createContext();

  for (const auto& nodeId : nodeIds) {
    results.push_back(validateNodeWithShape(nodeId, shapeId, context));
  }

  return results;
}

ValidationResult RecursiveShapeValidator::validateNodeWithShapeImpl(
    const std::string& nodeId, const NodeShape& shape,
    ValidationContext& context) {
  ValidationResult result;
  result.focusNode = nodeId;

  // Validate node-level constraints (non-recursive)
  validateNodeConstraints(nodeId, shape, result);

  // Validate property shapes (may trigger recursive validation)
  validatePropertyShapes(nodeId, shape, context, result);

  return result;
}

void RecursiveShapeValidator::validateNodeConstraints(
    const std::string& nodeId, const NodeShape& shape,
    ValidationResult& result) {
  // Validate each node-level constraint
  for (const auto& constraint : shape.nodeConstraints) {
    // Handle recursive constraints
    if (constraint.type == ConstraintType::Node) {
      // sh:node constraint - will be handled by caller with context
      // This is a placeholder; actual implementation would need access to
      // ValidationContext
      continue;
    }

    // For non-recursive constraints, delegate to ShaclConstraintEvaluator
    // Note: This is a simplified version. Real implementation would need
    // access to the actual node data
    bool satisfied = true;

    switch (constraint.type) {
      case ConstraintType::NodeKind: {
        auto kind = std::get<NodeKind>(constraint.value);
        satisfied = ShaclConstraintEvaluator::evaluateNodeKind(kind, nodeId);
        break;
      }
      default:
        // Other node-level constraints would be handled here
        break;
    }

    if (!satisfied) {
      result.addViolation(constraint.message.empty()
                              ? "Node constraint violation"
                              : constraint.message);
    }
  }
}

void RecursiveShapeValidator::validatePropertyShapes(
    const std::string& nodeId, const NodeShape& shape,
    ValidationContext& context, ValidationResult& result) {
  // Validate each property shape
  for (const auto& propShape : shape.propertyShapes) {
    // Get property values (placeholder - would integrate with index)
    std::vector<std::string> values = getPropertyValues(nodeId, propShape.path);

    // Validate the property shape with recursive support
    validatePropertyShape(nodeId, propShape, values, context, result);
  }
}

void RecursiveShapeValidator::validatePropertyShape(
    const std::string& nodeId, const PropertyShape& propShape,
    const std::vector<std::string>& values, ValidationContext& context,
    ValidationResult& result) {
  // First, check cardinality constraints
  for (const auto& constraint : propShape.constraints) {
    if (constraint.type == ConstraintType::MinCount) {
      auto minCount = std::get<int>(constraint.value);
      if (static_cast<int>(values.size()) < minCount) {
        result.addViolation(constraint.message.empty()
                                ? "MinCount violation for " + propShape.path
                                : constraint.message);
      }
    } else if (constraint.type == ConstraintType::MaxCount) {
      auto maxCount = std::get<int>(constraint.value);
      if (static_cast<int>(values.size()) > maxCount) {
        result.addViolation(constraint.message.empty()
                                ? "MaxCount violation for " + propShape.path
                                : constraint.message);
      }
    }
  }

  // Check if required property is present
  if (propShape.required && values.empty()) {
    result.addViolation("Required property " + propShape.path + " is missing");
    return;
  }

  // Validate each value
  for (const auto& value : values) {
    for (const auto& constraint : propShape.constraints) {
      // Handle recursive shape constraints
      if (constraint.type == ConstraintType::Shape) {
        auto targetShapeId = std::get<std::string>(constraint.value);
        auto valueResult =
            validateShapeReference(value, targetShapeId, context);

        // Merge violations from recursive validation
        for (const auto& violation : valueResult.violations) {
          result.addViolation("Property " + propShape.path + " value " +
                              value + ": " + violation);
        }
      } else if (constraint.type == ConstraintType::Node) {
        // sh:node for property values
        auto targetShapeId = std::get<std::string>(constraint.value);
        auto valueResult =
            validateNodeConstraint(value, targetShapeId, context);

        // Merge violations
        for (const auto& violation : valueResult.violations) {
          result.addViolation("Property " + propShape.path + " node " + value +
                              ": " + violation);
        }
      } else if (constraint.type != ConstraintType::MinCount &&
                 constraint.type != ConstraintType::MaxCount) {
        // Regular constraint validation
        if (!ShaclConstraintEvaluator::evaluateConstraint(constraint, value)) {
          result.addViolation(constraint.message.empty()
                                  ? "Constraint violation for " +
                                        propShape.path + " value " + value
                                  : constraint.message);
        }
      }
    }
  }
}

std::vector<std::string> RecursiveShapeValidator::getPropertyValues(
    const std::string& nodeId, const std::string& propertyPath) {
  // Placeholder implementation
  // In a real implementation, this would query the RDF index to get
  // all values for the given property on the given node
  //
  // For now, return empty vector to allow compilation
  // Integration with QLever's index would happen here
  return {};
}

}  // namespace shacl

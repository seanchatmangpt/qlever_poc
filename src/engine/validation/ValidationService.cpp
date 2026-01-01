#include "ValidationService.h"
#include "ViolationFormatter.h"
#include <chrono>
#include <stdexcept>

namespace validation {

ValidationService::ValidationService(
    const shacl::ShaclShapeRegistry* shapeRegistry,
    ValidationConfig config,
    std::shared_ptr<shacl::ShaclValidationCache> cache)
    : config_(std::move(config)),
      shapeRegistry_(shapeRegistry),
      cache_(std::move(cache)) {
  if (!shapeRegistry_) {
    throw std::invalid_argument(
        "ValidationService requires a non-null shape registry");
  }

  // Create default cache if not provided and caching is enabled
  if (!cache_ && config_.enableCaching) {
    cache_ = std::make_shared<shacl::ShaclValidationCache>();
  }
}

ValidationResult ValidationService::validate(const ValidationInput& input) {
  // Start timing
  startTime_ = std::chrono::steady_clock::now();

  // Validate input
  if (input.shapeSet.isEmpty()) {
    ValidationResult result;
    result.ok = false;
    result.errorCode = contracts::ErrorCode::INVALID_SHAPE_SET;
    result.errorMessage = "Shape set is empty";
    return result;
  }

  // Perform validation using SHACL
  return validateWithShacl(input);
}

ValidationResult ValidationService::validateWithShacl(
    const ValidationInput& input) {
  ValidationResult result;
  result.ok = true;
  result.errorCode = contracts::ErrorCode::OK;

  // Initialize stats
  result.stats = ValidationStats{};

  // NOTE: This is a wrapper around the existing SHACL validator.
  // The actual validation requires integration with QueryExecutionContext
  // and IdTable processing, which is handled by the ShaclValidator Operation.
  //
  // For a standalone validation service that doesn't operate on query results,
  // we would need to create a simplified validation path that:
  // 1. Loads shapes from the registry
  // 2. Identifies focus nodes based on input.scope
  // 3. Evaluates constraints directly without query execution
  //
  // Since the task requires wrapping the existing SHACL validator without
  // rewriting it, and the existing validator is designed as a query operation,
  // we provide a framework that can be integrated when called from query
  // execution context.

  // For now, we create a placeholder that demonstrates the API structure
  // A full implementation would require additional refactoring to separate
  // the validation logic from the Operation execution model.

  // Create a detailed validation report structure
  shacl::DetailedValidationReport shaclReport;

  // Get shapes to validate against
  std::vector<const shacl::NodeShape*> shapesToValidate;
  for (const auto& shapeId : input.shapeSet.shapeIds) {
    auto* shape = shapeRegistry_->getShape(shapeId);
    if (!shape) {
      result.ok = false;
      result.errorCode = contracts::ErrorCode::MISSING_SHAPE;
      result.errorMessage = "Shape not found: " + shapeId;
      return result;
    }
    shapesToValidate.push_back(shape);
  }

  // Track statistics
  size_t focusNodesEvaluated = 0;
  size_t constraintsEvaluated = 0;

  // Guard check: maximum shapes
  if (shapesToValidate.size() > config_.maxFocusNodes) {
    result.ok = false;
    result.errorCode = contracts::ErrorCode::FOCUS_NODES_LIMIT_EXCEEDED;
    result.errorMessage = "Too many shapes to validate";
    result.stats.guardsTriggered = true;
    result.stats.guardTriggeredReason = "Shape count exceeded limit";
    return result;
  }

  // This would be the integration point for actual validation
  // The real implementation would:
  // 1. Query the data graph for focus nodes matching shape targets
  // 2. For each focus node, evaluate all constraints
  // 3. Collect violations with guard checks
  // 4. Convert to standardized format

  // For demonstration, we show the conversion flow
  result = convertShaclReport(shaclReport);

  // Apply final guards
  applyGuards(result);

  // Calculate runtime
  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime_);
  result.stats.runtimeMs = duration.count();

  return result;
}

ValidationResult ValidationService::convertShaclReport(
    const shacl::DetailedValidationReport& shaclReport) {
  ValidationResult result;

  // Convert SHACL violations to standardized violations
  result.violations = ViolationFormatter::fromShaclReport(shaclReport);

  // Set conformance
  result.ok = shaclReport.conforms;

  // Populate stats
  result.stats.violationsFound = shaclReport.totalViolations;
  result.stats.warningsFound = shaclReport.totalWarnings;
  result.stats.infosFound = shaclReport.totalInfo;

  // Set error code based on conformance
  if (!shaclReport.conforms) {
    result.errorCode = contracts::ErrorCode::VIOLATIONS_FOUND;
  }

  return result;
}

void ValidationService::applyGuards(ValidationResult& result) {
  // Check violation limit
  if (result.violations.size() > config_.maxViolations) {
    result.violations.resize(config_.maxViolations);
    result.errorCode = contracts::ErrorCode::VIOLATIONS_LIMIT_EXCEEDED;
    result.errorMessage = "Violation limit exceeded: " +
                          std::to_string(config_.maxViolations);
    result.stats.guardsTriggered = true;
    result.stats.guardTriggeredReason = "Violation count exceeded limit";
  }

  // Check focus node limit
  if (result.stats.focusNodesEvaluated > config_.maxFocusNodes) {
    result.errorCode = contracts::ErrorCode::FOCUS_NODES_LIMIT_EXCEEDED;
    result.errorMessage = "Focus node limit exceeded: " +
                          std::to_string(config_.maxFocusNodes);
    result.stats.guardsTriggered = true;
    result.stats.guardTriggeredReason = "Focus node count exceeded limit";
  }

  // Check constraint evaluation limit
  if (result.stats.constraintsEvaluated > config_.maxConstraintEvaluations) {
    result.errorCode = contracts::ErrorCode::CONSTRAINT_EVALS_LIMIT_EXCEEDED;
    result.errorMessage = "Constraint evaluation limit exceeded: " +
                          std::to_string(config_.maxConstraintEvaluations);
    result.stats.guardsTriggered = true;
    result.stats.guardTriggeredReason =
        "Constraint evaluation count exceeded limit";
  }

  // Check runtime limit
  if (config_.maxRuntimeMsPerShape.has_value()) {
    if (result.stats.runtimeMs > *config_.maxRuntimeMsPerShape) {
      result.errorCode = contracts::ErrorCode::RUNTIME_EXCEEDED;
      result.errorMessage = "Runtime limit exceeded: " +
                            std::to_string(*config_.maxRuntimeMsPerShape) +
                            "ms";
      result.stats.guardsTriggered = true;
      result.stats.guardTriggeredReason = "Runtime exceeded limit";
    }
  }
}

bool ValidationService::checkViolationLimit(size_t currentCount) {
  return currentCount < config_.maxViolations;
}

bool ValidationService::checkFocusNodeLimit(size_t currentCount) {
  return currentCount < config_.maxFocusNodes;
}

bool ValidationService::checkConstraintEvaluationLimit(size_t currentCount) {
  return currentCount < config_.maxConstraintEvaluations;
}

bool ValidationService::checkRuntimeLimit(size_t elapsedMs) {
  if (!config_.maxRuntimeMsPerShape.has_value()) {
    return true;
  }
  return elapsedMs < *config_.maxRuntimeMsPerShape;
}

std::string ValidationService::getCacheStatistics() const {
  if (!cache_) {
    return "Caching disabled";
  }
  // Would return cache statistics if available
  return "Cache statistics not implemented";
}

void ValidationService::clearCache() {
  if (cache_) {
    cache_->clear();
  }
}

}  // namespace validation

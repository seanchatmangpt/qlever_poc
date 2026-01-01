#ifndef QLEVER_ENGINE_VALIDATION_VALIDATIONSERVICE_H
#define QLEVER_ENGINE_VALIDATION_VALIDATIONSERVICE_H

#include "engine/contracts/Violation.h"
#include "engine/contracts/ErrorCode.h"
#include "engine/contracts/CommonTypes.h"
#include "engine/validation/ValidationConfig.h"
#include "engine/validation/ValidationStats.h"
#include "engine/shacl/ShaclValidator.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace validation {

// Input specification for validation request
struct ValidationInput {
  // Optional epoch manifest SHA256 hash for reproducibility
  std::optional<std::string> epochManifestSha256;

  // Dataset scope to validate
  contracts::DatasetScope scope;

  // Shape set to use for validation
  contracts::ShapeSet shapeSet;

  // Optional: specific focus nodes to validate (if scope == FOCUS_NODES)
  std::vector<std::string> focusNodes;

  // Optional: named graph IRI (if scope == NAMED_GRAPH)
  std::optional<std::string> namedGraphIri;

  ValidationInput()
      : scope(contracts::DatasetScope::FULL_DATASET) {}

  ValidationInput(contracts::DatasetScope s, contracts::ShapeSet shapes)
      : scope(s), shapeSet(std::move(shapes)) {}
};

// Result of validation operation
struct ValidationResult {
  // Validation outcome
  bool ok = true;  // true if conforms or no violations found

  // Standardized violations
  std::vector<contracts::Violation> violations;

  // Validation statistics
  ValidationStats stats;

  // Error code (if validation failed or guard triggered)
  contracts::ErrorCode errorCode = contracts::ErrorCode::OK;

  // Optional error message (for guard violations or errors)
  std::optional<std::string> errorMessage;

  // Check if validation passed (no violations, no errors)
  bool conforms() const {
    return ok && violations.empty() &&
           errorCode == contracts::ErrorCode::OK;
  }

  // Check if validation failed due to guard
  bool guardTriggered() const {
    return contracts::isGuardTriggered(errorCode);
  }

  // Get total violation count
  size_t violationCount() const {
    return violations.size();
  }
};

// Main validation orchestrator
// Wraps existing SHACL validator and outputs standardized violation records
// Thread-safe if underlying SHACL validator is thread-safe
class ValidationService {
 private:
  // Configuration
  ValidationConfig config_;

  // SHACL shape registry (non-owning)
  const shacl::ShaclShapeRegistry* shapeRegistry_;

  // Validation cache (optional)
  std::shared_ptr<shacl::ShaclValidationCache> cache_;

  // Start time for runtime tracking
  std::chrono::steady_clock::time_point startTime_;

 public:
  // Constructor
  explicit ValidationService(
      const shacl::ShaclShapeRegistry* shapeRegistry,
      ValidationConfig config = ValidationConfig::defaultConfig(),
      std::shared_ptr<shacl::ShaclValidationCache> cache = nullptr);

  // Main validation method
  // Pure in-process API, no endpoint integration
  ValidationResult validate(const ValidationInput& input);

  // Get current configuration
  const ValidationConfig& getConfig() const { return config_; }

  // Update configuration
  void setConfig(const ValidationConfig& config) { config_ = config; }

  // Get cache statistics (if caching enabled)
  std::string getCacheStatistics() const;

  // Clear validation cache
  void clearCache();

 private:
  // Validate using SHACL validator
  ValidationResult validateWithShacl(const ValidationInput& input);

  // Check guards and enforce limits
  bool checkViolationLimit(size_t currentCount);
  bool checkFocusNodeLimit(size_t currentCount);
  bool checkConstraintEvaluationLimit(size_t currentCount);
  bool checkRuntimeLimit(size_t elapsedMs);

  // Convert SHACL report to standardized result
  ValidationResult convertShaclReport(
      const shacl::DetailedValidationReport& shaclReport);

  // Apply guards to result
  void applyGuards(ValidationResult& result);
};

}  // namespace validation

#endif  // QLEVER_ENGINE_VALIDATION_VALIDATIONSERVICE_H

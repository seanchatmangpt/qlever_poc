#ifndef QLEVER_ENGINE_VALIDATION_VALIDATIONCONFIG_H
#define QLEVER_ENGINE_VALIDATION_VALIDATIONCONFIG_H

#include <optional>
#include <cstddef>

namespace validation {

// SHACL profile selection
enum class ShaclProfile {
  CORE,  // SHACL Core profile (basic constraints only)
  FULL   // SHACL Full profile (includes SPARQL constraints)
};

// Validation configuration with resource guards
// Fail-closed: if any guard is exceeded, validation terminates with error
struct ValidationConfig {
  // Maximum number of violations to collect before terminating
  // Default: 10000 (fail closed if exceeded)
  size_t maxViolations = 10000;

  // Maximum number of focus nodes to validate
  // Default: 100000
  size_t maxFocusNodes = 100000;

  // Maximum number of constraint evaluations
  // Default: 1000000 (1 million)
  size_t maxConstraintEvaluations = 1000000;

  // Maximum runtime in milliseconds per shape
  // Default: 5000ms (5 seconds), optional
  std::optional<size_t> maxRuntimeMsPerShape = 5000;

  // Maximum SPARQL results for SPARQL-based constraints
  // Default: 100000
  size_t maxSparqlResults = 100000;

  // SHACL profile selection
  // Default: FULL
  ShaclProfile profile = ShaclProfile::FULL;

  // Enable parallel validation (if supported by validator)
  // Default: true
  bool enableParallelValidation = true;

  // Number of threads for parallel validation (0 = auto-detect)
  // Default: 0
  size_t parallelThreads = 0;

  // Enable validation caching
  // Default: true
  bool enableCaching = true;

  // Default configuration (conservative limits)
  static ValidationConfig defaultConfig() {
    return ValidationConfig{};
  }

  // Permissive configuration (higher limits for large datasets)
  static ValidationConfig permissiveConfig() {
    ValidationConfig config;
    config.maxViolations = 100000;
    config.maxFocusNodes = 1000000;
    config.maxConstraintEvaluations = 10000000;
    config.maxRuntimeMsPerShape = 30000;  // 30 seconds
    config.maxSparqlResults = 1000000;
    return config;
  }

  // Strict configuration (lower limits for fast feedback)
  static ValidationConfig strictConfig() {
    ValidationConfig config;
    config.maxViolations = 1000;
    config.maxFocusNodes = 10000;
    config.maxConstraintEvaluations = 100000;
    config.maxRuntimeMsPerShape = 1000;  // 1 second
    config.maxSparqlResults = 10000;
    return config;
  }

  // Testing configuration (minimal limits for unit tests)
  static ValidationConfig testConfig() {
    ValidationConfig config;
    config.maxViolations = 100;
    config.maxFocusNodes = 1000;
    config.maxConstraintEvaluations = 10000;
    config.maxRuntimeMsPerShape = 500;  // 500ms
    config.maxSparqlResults = 1000;
    config.enableParallelValidation = false;  // Deterministic for tests
    return config;
  }
};

}  // namespace validation

#endif  // QLEVER_ENGINE_VALIDATION_VALIDATIONCONFIG_H

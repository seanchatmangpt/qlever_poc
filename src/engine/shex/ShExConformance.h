#ifndef QLEVER_ENGINE_SHEX_SHEXCONFORMANCE_H
#define QLEVER_ENGINE_SHEX_SHEXCONFORMANCE_H

#include <string>
#include <unordered_set>

namespace shex {

// Standardized error codes for ShEx validation
// These map to EPIC 5 contracts module error codes when integrated
enum class ErrorCode {
  // Success
  SUCCESS = 0,

  // Cardinality violations
  MIN_COUNT_VIOLATION,  // Fewer values than required
  MAX_COUNT_VIOLATION,  // More values than allowed

  // Type violations
  DATATYPE_VIOLATION,  // Value doesn't match required datatype
  NODE_KIND_VIOLATION,  // Node kind doesn't match constraint

  // CLOSED shape violations
  CLOSED_SHAPE_VIOLATION,  // Property not allowed in CLOSED shape

  // Nested shape violations
  NESTED_SHAPE_VIOLATION,  // Value doesn't conform to nested shape

  // Parser errors
  PARSE_ERROR,  // Failed to parse ShEx schema
  UNSUPPORTED_SHEX_FEATURE,  // Feature not supported in minimal implementation

  // Validation errors
  SHAPE_NOT_FOUND,  // Referenced shape doesn't exist
  VALIDATION_TIMEOUT,  // Validation exceeded time limit
  VALIDATION_ERROR,  // General validation error

  // Configuration errors
  INVALID_CONFIGURATION,  // Invalid validation configuration
  MAX_VIOLATIONS_EXCEEDED  // Too many violations (fail-closed guard)
};

// ShEx features supported in this minimal implementation
struct SupportedFeatures {
  // Supported (✅)
  static constexpr bool SHAPE_DEFINITIONS = true;
  static constexpr bool TRIPLE_CONSTRAINTS = true;
  static constexpr bool CARDINALITY = true;
  static constexpr bool DATATYPE_CONSTRAINTS = true;
  static constexpr bool NODE_KIND_CONSTRAINTS = true;
  static constexpr bool CLOSED_SHAPES = true;
  static constexpr bool EXTRA_PROPERTIES = true;
  static constexpr bool NESTED_SHAPES = true;

  // Not supported (❌) - reject with UNSUPPORTED_SHEX_FEATURE
  static constexpr bool REGEX_CONSTRAINTS = false;
  static constexpr bool INHERITANCE = false;
  static constexpr bool SEMANTIC_ACTIONS = false;
  static constexpr bool NEGATION = false;
  static constexpr bool LANGUAGE_CONSTRAINTS = false;
  static constexpr bool LITERAL_LENGTH_CONSTRAINTS = false;
  static constexpr bool NUMERIC_RANGE_CONSTRAINTS = false;

  // Get list of supported feature names
  static std::unordered_set<std::string> getSupportedFeatureNames() {
    return {
        "shape_definitions",
        "triple_constraints",
        "cardinality",
        "datatype_constraints",
        "node_kind_constraints",
        "closed_shapes",
        "extra_properties",
        "nested_shapes"
    };
  }

  // Get list of unsupported feature names
  static std::unordered_set<std::string> getUnsupportedFeatureNames() {
    return {
        "regex_constraints",
        "inheritance",
        "semantic_actions",
        "negation",
        "language_constraints",
        "literal_length_constraints",
        "numeric_range_constraints"
    };
  }

  // Check if a feature is supported by name
  static bool isFeatureSupported(const std::string& featureName) {
    return getSupportedFeatureNames().count(featureName) > 0;
  }
};

// Violation severity levels
enum class SeverityLevel {
  ERROR,    // Critical violation
  WARNING,  // Non-critical violation
  INFO      // Informational message
};

// Convert error code to string
inline std::string errorCodeToString(ErrorCode code) {
  switch (code) {
    case ErrorCode::SUCCESS:
      return "SUCCESS";
    case ErrorCode::MIN_COUNT_VIOLATION:
      return "MIN_COUNT_VIOLATION";
    case ErrorCode::MAX_COUNT_VIOLATION:
      return "MAX_COUNT_VIOLATION";
    case ErrorCode::DATATYPE_VIOLATION:
      return "DATATYPE_VIOLATION";
    case ErrorCode::NODE_KIND_VIOLATION:
      return "NODE_KIND_VIOLATION";
    case ErrorCode::CLOSED_SHAPE_VIOLATION:
      return "CLOSED_SHAPE_VIOLATION";
    case ErrorCode::NESTED_SHAPE_VIOLATION:
      return "NESTED_SHAPE_VIOLATION";
    case ErrorCode::PARSE_ERROR:
      return "PARSE_ERROR";
    case ErrorCode::UNSUPPORTED_SHEX_FEATURE:
      return "UNSUPPORTED_SHEX_FEATURE";
    case ErrorCode::SHAPE_NOT_FOUND:
      return "SHAPE_NOT_FOUND";
    case ErrorCode::VALIDATION_TIMEOUT:
      return "VALIDATION_TIMEOUT";
    case ErrorCode::VALIDATION_ERROR:
      return "VALIDATION_ERROR";
    case ErrorCode::INVALID_CONFIGURATION:
      return "INVALID_CONFIGURATION";
    case ErrorCode::MAX_VIOLATIONS_EXCEEDED:
      return "MAX_VIOLATIONS_EXCEEDED";
    default:
      return "UNKNOWN_ERROR";
  }
}

// Convert severity level to string
inline std::string severityToString(SeverityLevel severity) {
  switch (severity) {
    case SeverityLevel::ERROR:
      return "ERROR";
    case SeverityLevel::WARNING:
      return "WARNING";
    case SeverityLevel::INFO:
      return "INFO";
    default:
      return "UNKNOWN";
  }
}

// Violation structure for standardized error reporting
struct Violation {
  ErrorCode errorCode;
  SeverityLevel severity;
  std::string focusNode;
  std::string propertyPath;
  std::string shapeId;
  std::string message;

  Violation(ErrorCode code, std::string node, std::string path,
            std::string shape, std::string msg,
            SeverityLevel sev = SeverityLevel::ERROR)
      : errorCode(code),
        severity(sev),
        focusNode(std::move(node)),
        propertyPath(std::move(path)),
        shapeId(std::move(shape)),
        message(std::move(msg)) {}

  // Get formatted violation message
  std::string format() const {
    std::string result = "[" + severityToString(severity) + "] ";
    result += errorCodeToString(errorCode) + ": ";
    result += message;
    result += " (node: " + focusNode;
    if (!propertyPath.empty()) {
      result += ", property: " + propertyPath;
    }
    result += ", shape: " + shapeId + ")";
    return result;
  }
};

// Validation configuration
struct ValidationConfig {
  size_t maxFocusNodes = 10000;  // Maximum number of nodes to validate
  size_t maxConstraintEvals = 1000000;  // Maximum constraint evaluations
  size_t maxViolations = 10000;  // Maximum violations before fail-closed
  bool failClosed = true;  // Fail if limits exceeded
  bool collectAllViolations = false;  // Collect all violations or stop at first

  ValidationConfig() = default;

  // Validate configuration
  bool isValid() const {
    return maxFocusNodes > 0 && maxConstraintEvals > 0 && maxViolations > 0;
  }
};

}  // namespace shex

#endif  // QLEVER_ENGINE_SHEX_SHEXCONFORMANCE_H

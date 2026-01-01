#ifndef PARSER_SHEX_ERROR_REPORTING_H
#define PARSER_SHEX_ERROR_REPORTING_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <sstream>

#include "absl/container/flat_hash_map.h"
#include "../util/json.h"

namespace shex {

// ============================================================================
// Error Severity Levels
// ============================================================================

enum class ErrorSeverity {
  ERROR,    // Validation failure, shape does not conform
  WARNING,  // Potential issue but not a hard failure
  INFO      // Informational message about validation
};

inline std::string severityToString(ErrorSeverity severity) {
  switch (severity) {
    case ErrorSeverity::ERROR:   return "ERROR";
    case ErrorSeverity::WARNING: return "WARNING";
    case ErrorSeverity::INFO:    return "INFO";
  }
  return "UNKNOWN";
}

// ============================================================================
// Error Type Classification
// ============================================================================

enum class ErrorType {
  CARDINALITY_VIOLATION,      // Wrong number of property occurrences
  TYPE_MISMATCH,               // Value type doesn't match constraint
  VALUE_NOT_ALLOWED,           // Value not in allowed set
  DATATYPE_MISMATCH,           // Literal datatype doesn't match
  SHAPE_NOT_FOUND,             // Referenced shape doesn't exist
  MISSING_REQUIRED_PROPERTY,   // Required property absent
  EXTRA_PROPERTY,              // Property not allowed in closed shape
  PARSER_SYNTAX_ERROR,         // Parsing failed
  PARSER_SEMANTIC_ERROR,       // Parsing succeeded but semantically invalid
  CONSTRAINT_VIOLATION         // Generic constraint violation
};

inline std::string errorTypeToString(ErrorType type) {
  switch (type) {
    case ErrorType::CARDINALITY_VIOLATION:     return "CARDINALITY_VIOLATION";
    case ErrorType::TYPE_MISMATCH:              return "TYPE_MISMATCH";
    case ErrorType::VALUE_NOT_ALLOWED:          return "VALUE_NOT_ALLOWED";
    case ErrorType::DATATYPE_MISMATCH:          return "DATATYPE_MISMATCH";
    case ErrorType::SHAPE_NOT_FOUND:            return "SHAPE_NOT_FOUND";
    case ErrorType::MISSING_REQUIRED_PROPERTY:  return "MISSING_REQUIRED_PROPERTY";
    case ErrorType::EXTRA_PROPERTY:             return "EXTRA_PROPERTY";
    case ErrorType::PARSER_SYNTAX_ERROR:        return "PARSER_SYNTAX_ERROR";
    case ErrorType::PARSER_SEMANTIC_ERROR:      return "PARSER_SEMANTIC_ERROR";
    case ErrorType::CONSTRAINT_VIOLATION:       return "CONSTRAINT_VIOLATION";
  }
  return "UNKNOWN";
}

// ============================================================================
// Triple Context (Subject, Predicate, Object)
// ============================================================================

struct TripleContext {
  std::string subject;
  std::string predicate;
  std::string object;
  std::string objectType;  // "IRI", "LITERAL", "BNODE"

  TripleContext() = default;
  TripleContext(const std::string& s, const std::string& p,
                const std::string& o, const std::string& ot = "")
      : subject(s), predicate(p), object(o), objectType(ot) {}

  std::string toString() const {
    std::ostringstream oss;
    oss << "<" << subject << "> <" << predicate << "> ";
    if (objectType == "IRI") {
      oss << "<" << object << ">";
    } else if (objectType == "LITERAL") {
      oss << "\"" << object << "\"";
    } else {
      oss << object;
    }
    return oss.str();
  }

  nlohmann::json toJson() const {
    return {
        {"subject", subject},
        {"predicate", predicate},
        {"object", object},
        {"objectType", objectType}
    };
  }
};

// ============================================================================
// Source Location (for parser errors)
// ============================================================================

struct SourceLocation {
  int line;
  int column;
  std::optional<int> endLine;
  std::optional<int> endColumn;

  SourceLocation() : line(0), column(0) {}
  SourceLocation(int l, int c) : line(l), column(c) {}
  SourceLocation(int l, int c, int el, int ec)
      : line(l), column(c), endLine(el), endColumn(ec) {}

  std::string toString() const {
    std::ostringstream oss;
    oss << "line " << line << ", column " << column;
    if (endLine.has_value() && endColumn.has_value()) {
      oss << " to line " << endLine.value() << ", column " << endColumn.value();
    }
    return oss.str();
  }

  nlohmann::json toJson() const {
    nlohmann::json j = {{"line", line}, {"column", column}};
    if (endLine.has_value()) j["endLine"] = endLine.value();
    if (endColumn.has_value()) j["endColumn"] = endColumn.value();
    return j;
  }
};

// ============================================================================
// Detailed Validation Error
// ============================================================================

struct DetailedValidationError {
  ErrorSeverity severity;
  ErrorType errorType;
  std::string message;
  std::optional<std::string> suggestion;

  // Context information
  std::optional<TripleContext> tripleContext;
  std::optional<SourceLocation> location;

  // Expected vs Actual
  std::optional<std::string> expectedValue;
  std::optional<std::string> actualValue;
  std::optional<int> expectedCount;
  std::optional<int> actualCount;

  // Shape context
  std::string shapeId;
  std::optional<std::string> propertyId;
  std::optional<std::string> nodeId;

  DetailedValidationError() = default;

  // Human-readable format
  std::string toHumanReadable() const {
    std::ostringstream oss;
    oss << "[" << severityToString(severity) << "] "
        << errorTypeToString(errorType) << ": " << message << "\n";

    if (nodeId.has_value()) {
      oss << "  Node: " << nodeId.value() << "\n";
    }
    if (!shapeId.empty()) {
      oss << "  Shape: " << shapeId << "\n";
    }
    if (propertyId.has_value()) {
      oss << "  Property: " << propertyId.value() << "\n";
    }

    if (tripleContext.has_value()) {
      oss << "  Triple: " << tripleContext->toString() << "\n";
    }

    if (expectedValue.has_value() && actualValue.has_value()) {
      oss << "  Expected: " << expectedValue.value() << "\n";
      oss << "  Actual: " << actualValue.value() << "\n";
    }

    if (expectedCount.has_value() && actualCount.has_value()) {
      oss << "  Expected count: " << expectedCount.value() << "\n";
      oss << "  Actual count: " << actualCount.value() << "\n";
    }

    if (location.has_value()) {
      oss << "  Location: " << location->toString() << "\n";
    }

    if (suggestion.has_value()) {
      oss << "  Suggestion: " << suggestion.value() << "\n";
    }

    return oss.str();
  }

  // JSON format
  nlohmann::json toJson() const {
    nlohmann::json j = {
        {"severity", severityToString(severity)},
        {"errorType", errorTypeToString(errorType)},
        {"message", message},
        {"shapeId", shapeId}
    };

    if (suggestion.has_value()) j["suggestion"] = suggestion.value();
    if (nodeId.has_value()) j["nodeId"] = nodeId.value();
    if (propertyId.has_value()) j["propertyId"] = propertyId.value();
    if (tripleContext.has_value()) j["tripleContext"] = tripleContext->toJson();
    if (location.has_value()) j["location"] = location->toJson();
    if (expectedValue.has_value()) j["expectedValue"] = expectedValue.value();
    if (actualValue.has_value()) j["actualValue"] = actualValue.value();
    if (expectedCount.has_value()) j["expectedCount"] = expectedCount.value();
    if (actualCount.has_value()) j["actualCount"] = actualCount.value();

    return j;
  }

  // XML format
  std::string toXml() const {
    std::ostringstream oss;
    oss << "  <error>\n";
    oss << "    <severity>" << severityToString(severity) << "</severity>\n";
    oss << "    <errorType>" << errorTypeToString(errorType) << "</errorType>\n";
    oss << "    <message>" << xmlEscape(message) << "</message>\n";
    oss << "    <shapeId>" << xmlEscape(shapeId) << "</shapeId>\n";

    if (nodeId.has_value()) {
      oss << "    <nodeId>" << xmlEscape(nodeId.value()) << "</nodeId>\n";
    }
    if (propertyId.has_value()) {
      oss << "    <propertyId>" << xmlEscape(propertyId.value()) << "</propertyId>\n";
    }
    if (expectedValue.has_value()) {
      oss << "    <expectedValue>" << xmlEscape(expectedValue.value())
          << "</expectedValue>\n";
    }
    if (actualValue.has_value()) {
      oss << "    <actualValue>" << xmlEscape(actualValue.value())
          << "</actualValue>\n";
    }
    if (suggestion.has_value()) {
      oss << "    <suggestion>" << xmlEscape(suggestion.value())
          << "</suggestion>\n";
    }

    oss << "  </error>\n";
    return oss.str();
  }

 private:
  static std::string xmlEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
      switch (c) {
        case '<':  result += "&lt;"; break;
        case '>':  result += "&gt;"; break;
        case '&':  result += "&amp;"; break;
        case '"':  result += "&quot;"; break;
        case '\'': result += "&apos;"; break;
        default:   result += c; break;
      }
    }
    return result;
  }
};

// ============================================================================
// Shape Conformance Status
// ============================================================================

enum class ConformanceStatus {
  CONFORMS,        // Node fully conforms to shape
  DOES_NOT_CONFORM, // Node does not conform to shape
  UNKNOWN          // Conformance status unknown
};

inline std::string conformanceStatusToString(ConformanceStatus status) {
  switch (status) {
    case ConformanceStatus::CONFORMS:        return "CONFORMS";
    case ConformanceStatus::DOES_NOT_CONFORM: return "DOES_NOT_CONFORM";
    case ConformanceStatus::UNKNOWN:         return "UNKNOWN";
  }
  return "UNKNOWN";
}

// ============================================================================
// Shape Conformance Map
// ============================================================================

struct ShapeConformanceEntry {
  std::string shapeId;
  ConformanceStatus status;
  std::vector<DetailedValidationError> errors;
  std::vector<std::string> failedConstraints;

  nlohmann::json toJson() const {
    nlohmann::json errorArray = nlohmann::json::array();
    for (const auto& error : errors) {
      errorArray.push_back(error.toJson());
    }

    return {
        {"shapeId", shapeId},
        {"status", conformanceStatusToString(status)},
        {"failedConstraints", failedConstraints},
        {"errors", errorArray}
    };
  }
};

class ShapeConformanceMap {
 public:
  // Map: NodeIRI -> ShapeID -> ConformanceEntry
  using MapType = absl::flat_hash_map<
      std::string,
      absl::flat_hash_map<std::string, ShapeConformanceEntry>>;

  void addEntry(const std::string& nodeId, const std::string& shapeId,
                ConformanceStatus status) {
    conformanceMap_[nodeId][shapeId].shapeId = shapeId;
    conformanceMap_[nodeId][shapeId].status = status;
  }

  void addError(const std::string& nodeId, const std::string& shapeId,
                const DetailedValidationError& error) {
    conformanceMap_[nodeId][shapeId].errors.push_back(error);
    conformanceMap_[nodeId][shapeId].status = ConformanceStatus::DOES_NOT_CONFORM;
  }

  void addFailedConstraint(const std::string& nodeId, const std::string& shapeId,
                           const std::string& constraint) {
    conformanceMap_[nodeId][shapeId].failedConstraints.push_back(constraint);
  }

  const MapType& getMap() const { return conformanceMap_; }

  bool nodeConformsToShape(const std::string& nodeId,
                           const std::string& shapeId) const {
    auto nodeIt = conformanceMap_.find(nodeId);
    if (nodeIt == conformanceMap_.end()) return false;

    auto shapeIt = nodeIt->second.find(shapeId);
    if (shapeIt == nodeIt->second.end()) return false;

    return shapeIt->second.status == ConformanceStatus::CONFORMS;
  }

  std::vector<std::string> getFailedShapes(const std::string& nodeId) const {
    std::vector<std::string> failedShapes;
    auto nodeIt = conformanceMap_.find(nodeId);
    if (nodeIt == conformanceMap_.end()) return failedShapes;

    for (const auto& [shapeId, entry] : nodeIt->second) {
      if (entry.status == ConformanceStatus::DOES_NOT_CONFORM) {
        failedShapes.push_back(shapeId);
      }
    }
    return failedShapes;
  }

  nlohmann::json toJson() const {
    nlohmann::json result = nlohmann::json::object();
    for (const auto& [nodeId, shapeMap] : conformanceMap_) {
      nlohmann::json shapeArray = nlohmann::json::array();
      for (const auto& [shapeId, entry] : shapeMap) {
        shapeArray.push_back(entry.toJson());
      }
      result[nodeId] = shapeArray;
    }
    return result;
  }

 private:
  MapType conformanceMap_;
};

// ============================================================================
// Enhanced Validation Report
// ============================================================================

struct EnhancedValidationReport {
  bool conforms;
  std::vector<DetailedValidationError> errors;
  ShapeConformanceMap conformanceMap;

  // Statistics
  int totalErrors = 0;
  int totalWarnings = 0;
  int totalInfoMessages = 0;

  EnhancedValidationReport() : conforms(true) {}

  void addError(const DetailedValidationError& error) {
    errors.push_back(error);

    switch (error.severity) {
      case ErrorSeverity::ERROR:
        totalErrors++;
        conforms = false;
        break;
      case ErrorSeverity::WARNING:
        totalWarnings++;
        break;
      case ErrorSeverity::INFO:
        totalInfoMessages++;
        break;
    }
  }

  void computeStatistics() {
    totalErrors = 0;
    totalWarnings = 0;
    totalInfoMessages = 0;

    for (const auto& error : errors) {
      switch (error.severity) {
        case ErrorSeverity::ERROR:   totalErrors++; break;
        case ErrorSeverity::WARNING: totalWarnings++; break;
        case ErrorSeverity::INFO:    totalInfoMessages++; break;
      }
    }

    conforms = (totalErrors == 0);
  }

  // Human-readable format
  std::string toHumanReadable() const {
    std::ostringstream oss;
    oss << "=== ShEx Validation Report ===\n";
    oss << "Overall Status: " << (conforms ? "CONFORMS" : "DOES NOT CONFORM") << "\n";
    oss << "Errors: " << totalErrors << ", Warnings: " << totalWarnings
        << ", Info: " << totalInfoMessages << "\n\n";

    if (!errors.empty()) {
      oss << "Detailed Errors:\n";
      oss << "================\n\n";
      for (const auto& error : errors) {
        oss << error.toHumanReadable() << "\n";
      }
    }

    return oss.str();
  }

  // JSON format
  nlohmann::json toJson() const {
    nlohmann::json errorArray = nlohmann::json::array();
    for (const auto& error : errors) {
      errorArray.push_back(error.toJson());
    }

    return {
        {"conforms", conforms},
        {"statistics", {
            {"totalErrors", totalErrors},
            {"totalWarnings", totalWarnings},
            {"totalInfoMessages", totalInfoMessages}
        }},
        {"errors", errorArray},
        {"conformanceMap", conformanceMap.toJson()}
    };
  }

  // XML format
  std::string toXml() const {
    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    oss << "<validationReport>\n";
    oss << "  <conforms>" << (conforms ? "true" : "false") << "</conforms>\n";
    oss << "  <statistics>\n";
    oss << "    <totalErrors>" << totalErrors << "</totalErrors>\n";
    oss << "    <totalWarnings>" << totalWarnings << "</totalWarnings>\n";
    oss << "    <totalInfoMessages>" << totalInfoMessages << "</totalInfoMessages>\n";
    oss << "  </statistics>\n";
    oss << "  <errors>\n";

    for (const auto& error : errors) {
      oss << error.toXml();
    }

    oss << "  </errors>\n";
    oss << "</validationReport>\n";
    return oss.str();
  }
};

// ============================================================================
// Error Builder Helper
// ============================================================================

class ErrorBuilder {
 public:
  ErrorBuilder& setSeverity(ErrorSeverity severity) {
    error_.severity = severity;
    return *this;
  }

  ErrorBuilder& setErrorType(ErrorType type) {
    error_.errorType = type;
    return *this;
  }

  ErrorBuilder& setMessage(const std::string& message) {
    error_.message = message;
    return *this;
  }

  ErrorBuilder& setSuggestion(const std::string& suggestion) {
    error_.suggestion = suggestion;
    return *this;
  }

  ErrorBuilder& setTripleContext(const TripleContext& context) {
    error_.tripleContext = context;
    return *this;
  }

  ErrorBuilder& setLocation(const SourceLocation& location) {
    error_.location = location;
    return *this;
  }

  ErrorBuilder& setExpectedValue(const std::string& value) {
    error_.expectedValue = value;
    return *this;
  }

  ErrorBuilder& setActualValue(const std::string& value) {
    error_.actualValue = value;
    return *this;
  }

  ErrorBuilder& setExpectedCount(int count) {
    error_.expectedCount = count;
    return *this;
  }

  ErrorBuilder& setActualCount(int count) {
    error_.actualCount = count;
    return *this;
  }

  ErrorBuilder& setShapeId(const std::string& shapeId) {
    error_.shapeId = shapeId;
    return *this;
  }

  ErrorBuilder& setPropertyId(const std::string& propertyId) {
    error_.propertyId = propertyId;
    return *this;
  }

  ErrorBuilder& setNodeId(const std::string& nodeId) {
    error_.nodeId = nodeId;
    return *this;
  }

  DetailedValidationError build() const {
    return error_;
  }

 private:
  DetailedValidationError error_;
};

}  // namespace shex

#endif  // PARSER_SHEX_ERROR_REPORTING_H

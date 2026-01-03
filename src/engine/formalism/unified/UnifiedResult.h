//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 5 - EPIC 14.0 Formalism Convergence
//
//  Unified Result/Violation Schema for All Formalisms
//  Based on W3C SHACL Validation Report (https://www.w3.org/TR/shacl/#validation-report)
//  Extended to support SHACL, N3, Datalog, and ShEx result types

#ifndef QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDRESULT_H
#define QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDRESULT_H

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>

namespace formalism {

// Formalism type identifier for result provenance
enum class FormalismType {
  SHACL,    // Shapes Constraint Language
  N3,       // Notation3
  Datalog,  // Datalog rules
  ShEx      // Shape Expressions
};

// Severity levels (aligned with W3C SHACL)
// See: https://www.w3.org/TR/shacl/#severity
enum class SeverityLevel {
  Violation,  // sh:Violation - constraint violation (error)
  Warning,    // sh:Warning - potential issue
  Info        // sh:Info - informational message
};

// Result type classification
enum class ResultType {
  ValidationViolation,  // Constraint/shape validation failure (SHACL, ShEx)
  ComplianceIssue,      // Feature compatibility issue (N3)
  RuleDerivation,       // Derived tuple from rule evaluation (Datalog)
  ParseError,           // Syntax or parsing error (all formalisms)
  SemanticWarning       // Semantic consistency warning (all formalisms)
};

// Forward declarations
class UnifiedViolation;
class UnifiedValidationReport;

// ============================================================================
// UnifiedViolation: Core violation/result representation
// ============================================================================
//
// Based on W3C SHACL ValidationResult shape:
//   sh:ValidationResult
//     a rdfs:Class ;
//     sh:property [
//       sh:path sh:focusNode ;      # The RDF term that caused the violation
//       sh:minCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:resultPath ;     # The property path where violation occurred
//       sh:maxCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:resultSeverity ; # Severity: Violation, Warning, or Info
//       sh:maxCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:sourceConstraintComponent ; # Constraint that failed
//       sh:maxCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:sourceShape ;    # Shape that was validated
//       sh:maxCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:value ;          # Actual value(s) that caused violation
//     ] ;
//     sh:property [
//       sh:path sh:resultMessage ;  # Human-readable message
//     ] .
//
// Extended with formalism-specific metadata to support all four formalisms.
class UnifiedViolation {
 public:
  // ========== Core W3C SHACL ValidationResult Properties ==========

  // The formalism that produced this result
  FormalismType formalism;

  // Classification of result type
  ResultType resultType = ResultType::ValidationViolation;

  // Severity level (aligned with sh:resultSeverity)
  SeverityLevel severity = SeverityLevel::Violation;

  // Focus node: The RDF term or resource that was validated
  // - SHACL: focusNode (e.g., "ex:Alice")
  // - N3: filename or IRI being validated
  // - Datalog: rule predicate name (e.g., "ancestor")
  // - ShEx: target node
  std::string focusNode;

  // Result path: Property or path where violation occurred
  // - SHACL: resultPath (e.g., "ex:name" or "ex:friend/ex:name")
  // - N3: feature name (e.g., "formulae", "implication")
  // - Datalog: variable binding context
  // - ShEx: triple expression reference
  std::optional<std::string> resultPath;

  // Source shape: The shape/rule/constraint that was evaluated
  // - SHACL: sourceShape IRI (e.g., "ex:PersonShape")
  // - N3: "N3ComplianceVerifier"
  // - Datalog: rule name (e.g., "ancestor(?x, ?y) :- ...")
  // - ShEx: shape IRI
  std::optional<std::string> sourceShape;

  // Source constraint component: Specific constraint that failed
  // - SHACL: sh:minCount, sh:datatype, sh:pattern, etc.
  // - N3: "hasFormulae", "hasImplication", etc.
  // - Datalog: filter constraint or safety check
  // - ShEx: cardinality, datatype, value constraint
  std::optional<std::string> sourceConstraintComponent;

  // Value(s): Actual values that caused the violation
  // - SHACL: literal values or IRIs
  // - N3: line content
  // - Datalog: variable bindings (tuple columns)
  // - ShEx: actual node values
  std::vector<std::string> values;

  // Expected value: What was expected by the constraint
  // - SHACL: expected datatype, min/max value, pattern
  // - N3: "supported" or "unsupported"
  // - Datalog: expected tuple schema
  // - ShEx: expected node kind or value
  std::optional<std::string> expectedValue;

  // Result message: Human-readable description (sh:resultMessage)
  std::string message;

  // ========== Formalism-Specific Extensions ==========

  // Additional metadata (key-value pairs)
  // Examples:
  //   SHACL: {"constraintType": "minCount", "parameterValue": "1"}
  //   N3: {"lineNumber": "42", "lineContent": "@forAll :x :y { ... }"}
  //   Datalog: {"ruleArity": "2", "isRecursive": "true"}
  //   ShEx: {"shapeLabel": "PersonShape", "tripleExpr": "te1"}
  std::unordered_map<std::string, std::string> metadata;

  // Source location information
  struct SourceLocation {
    std::optional<std::string> filename;
    std::optional<size_t> lineNumber;
    std::optional<size_t> columnNumber;
    std::optional<std::string> lineContent;
  };
  std::optional<SourceLocation> location;

  // ========== Constructors ==========

  UnifiedViolation() = default;

  UnifiedViolation(FormalismType fmt, std::string focus,
                   SeverityLevel sev = SeverityLevel::Violation)
      : formalism(fmt), focusNode(std::move(focus)), severity(sev) {}

  // ========== Builder Pattern Methods ==========

  UnifiedViolation& withResultType(ResultType type) {
    resultType = type;
    return *this;
  }

  UnifiedViolation& withSeverity(SeverityLevel sev) {
    severity = sev;
    return *this;
  }

  UnifiedViolation& withResultPath(const std::string& path) {
    resultPath = path;
    return *this;
  }

  UnifiedViolation& withSourceShape(const std::string& shape) {
    sourceShape = shape;
    return *this;
  }

  UnifiedViolation& withSourceConstraintComponent(const std::string& component) {
    sourceConstraintComponent = component;
    return *this;
  }

  UnifiedViolation& withValue(const std::string& value) {
    values.push_back(value);
    return *this;
  }

  UnifiedViolation& withValues(const std::vector<std::string>& vals) {
    values = vals;
    return *this;
  }

  UnifiedViolation& withExpectedValue(const std::string& expected) {
    expectedValue = expected;
    return *this;
  }

  UnifiedViolation& withMessage(const std::string& msg) {
    message = msg;
    return *this;
  }

  UnifiedViolation& withMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
    return *this;
  }

  UnifiedViolation& withLocation(const std::string& filename, size_t lineNum) {
    if (!location) {
      location = SourceLocation{};
    }
    location->filename = filename;
    location->lineNumber = lineNum;
    return *this;
  }

  UnifiedViolation& withLineContent(const std::string& content) {
    if (!location) {
      location = SourceLocation{};
    }
    location->lineContent = content;
    return *this;
  }

  // ========== Query Methods ==========

  bool isError() const { return severity == SeverityLevel::Violation; }
  bool isWarning() const { return severity == SeverityLevel::Warning; }
  bool isInfo() const { return severity == SeverityLevel::Info; }

  std::string getFormalismName() const;
  std::string getSeverityString() const;
  std::string getResultTypeString() const;

  // ========== Serialization Methods ==========

  // Serialize to JSON format
  std::string toJSON(bool prettyPrint = false) const;

  // Serialize to RDF Turtle format (W3C SHACL ValidationResult)
  std::string toRDF(const std::string& blankNodeId = "_:result") const;

  // Serialize to SPARQL result binding (for Datalog-style output)
  std::string toSPARQLBinding() const;

  // Generate human-readable summary
  std::string getSummary() const;

  // Get detailed description with all context
  std::string getDetailedDescription() const;
};

// ============================================================================
// UnifiedValidationReport: Complete validation report
// ============================================================================
//
// Based on W3C SHACL ValidationReport shape:
//   sh:ValidationReport
//     a rdfs:Class ;
//     sh:property [
//       sh:path sh:conforms ;  # Boolean: true if valid, false if violations
//       sh:datatype xsd:boolean ;
//       sh:minCount 1 ;
//       sh:maxCount 1 ;
//     ] ;
//     sh:property [
//       sh:path sh:result ;    # List of ValidationResult instances
//     ] .
class UnifiedValidationReport {
 public:
  // Overall conformance (sh:conforms)
  // - true: No violations (may have warnings/info)
  // - false: At least one violation exists
  bool conforms = true;

  // Formalism being validated
  FormalismType formalism;

  // Source identifier (filename, URI, or description)
  std::string source;

  // Timestamp (ISO 8601 format)
  std::optional<std::string> timestamp;

  // Total counts by severity
  size_t violationCount = 0;
  size_t warningCount = 0;
  size_t infoCount = 0;

  // All results (in order encountered)
  std::vector<UnifiedViolation> results;

  // Results grouped by focus node
  std::unordered_map<std::string, std::vector<UnifiedViolation>>
      resultsByFocusNode;

  // Results grouped by severity
  std::unordered_map<SeverityLevel, std::vector<UnifiedViolation>>
      resultsBySeverity;

  // Results grouped by result type
  std::unordered_map<ResultType, std::vector<UnifiedViolation>>
      resultsByType;

  // Additional report metadata
  std::unordered_map<std::string, std::string> metadata;

  // ========== Constructors ==========

  UnifiedValidationReport() = default;

  explicit UnifiedValidationReport(FormalismType fmt, std::string src = "")
      : formalism(fmt), source(std::move(src)) {}

  // ========== Modification Methods ==========

  void addResult(const UnifiedViolation& result);
  void addResult(UnifiedViolation&& result);

  void clear() {
    conforms = true;
    violationCount = 0;
    warningCount = 0;
    infoCount = 0;
    results.clear();
    resultsByFocusNode.clear();
    resultsBySeverity.clear();
    resultsByType.clear();
  }

  // ========== Query Methods ==========

  size_t getTotalIssues() const {
    return violationCount + warningCount + infoCount;
  }

  bool hasViolations() const { return violationCount > 0; }
  bool hasWarnings() const { return warningCount > 0; }
  bool hasInfo() const { return infoCount > 0; }

  const std::vector<UnifiedViolation>& getResultsForFocusNode(
      const std::string& focusNode) const;

  std::vector<UnifiedViolation> getResultsBySeverity(
      SeverityLevel severity) const;

  std::vector<UnifiedViolation> getResultsByType(ResultType type) const;

  // ========== Serialization Methods ==========

  // Serialize to JSON format
  std::string toJSON(bool prettyPrint = false) const;

  // Serialize to RDF Turtle format (W3C SHACL ValidationReport)
  std::string toRDF() const;

  // Generate human-readable summary
  std::string getSummary() const;

  // Generate detailed report with all results
  std::string getDetailedReport() const;

  // ========== Statistics ==========

  struct Statistics {
    size_t totalResults = 0;
    size_t violations = 0;
    size_t warnings = 0;
    size_t infos = 0;
    size_t uniqueFocusNodes = 0;
    std::unordered_map<std::string, size_t> resultsByConstraint;
    std::unordered_map<std::string, size_t> resultsByShape;
    std::unordered_map<ResultType, size_t> resultsByType;
  };

  Statistics getStatistics() const;

 private:
  void updateIndices(const UnifiedViolation& result);
};

// ============================================================================
// Conversion Functions: From Formalism-Specific to Unified
// ============================================================================

// Forward declarations for formalism-specific types
namespace shacl {
  class ShaclViolation;
  class DetailedValidationReport;
}

namespace ad_utility {
  struct N3ComplianceIssue;
  class N3ComplianceVerifier;
}

class DatalogRule;

// Convert SHACL violation to unified format
UnifiedViolation fromShaclViolation(const shacl::ShaclViolation& violation);

// Convert SHACL report to unified format
UnifiedValidationReport fromShaclReport(
    const shacl::DetailedValidationReport& report);

// Convert N3 compliance issue to unified format
UnifiedViolation fromN3ComplianceIssue(
    const ad_utility::N3ComplianceIssue& issue,
    const std::string& filename = "");

// Convert N3 compliance verifier results to unified format
UnifiedValidationReport fromN3ComplianceVerifier(
    const ad_utility::N3ComplianceVerifier& verifier);

// Convert Datalog rule evaluation result to unified format
// (Datalog produces tuples, so this creates info-level results)
UnifiedViolation fromDatalogTuple(
    const DatalogRule& rule,
    const std::vector<std::string>& tuple,
    const std::vector<std::string>& variableNames);

// Create a Datalog rule derivation report
UnifiedValidationReport createDatalogDerivationReport(
    const std::string& ruleName,
    const std::vector<std::vector<std::string>>& derivedTuples,
    const std::vector<std::string>& variableNames);

// ============================================================================
// ViolationFormatter: Human-Readable Output
// ============================================================================

// Output format options
enum class OutputFormat {
  Text,       // Plain text, human-readable
  JSON,       // JSON format
  RDF,        // RDF Turtle (W3C SHACL ValidationReport)
  Markdown,   // Markdown format (like N3 compliance reports)
  SPARQL      // SPARQL result set format
};

class ViolationFormatter {
 public:
  // Format a single violation
  static std::string format(const UnifiedViolation& violation,
                           OutputFormat format = OutputFormat::Text);

  // Format a complete validation report
  static std::string format(const UnifiedValidationReport& report,
                           OutputFormat format = OutputFormat::Text);

  // Format to an output stream
  static void format(const UnifiedValidationReport& report,
                    std::ostream& out,
                    OutputFormat format = OutputFormat::Text);

  // Format summary statistics only
  static std::string formatSummary(const UnifiedValidationReport& report);

  // Format results grouped by focus node
  static std::string formatByFocusNode(const UnifiedValidationReport& report,
                                       OutputFormat format = OutputFormat::Text);

  // Format results grouped by severity
  static std::string formatBySeverity(const UnifiedValidationReport& report,
                                      OutputFormat format = OutputFormat::Text);

  // Format results grouped by constraint type
  static std::string formatByConstraint(const UnifiedValidationReport& report,
                                        OutputFormat format = OutputFormat::Text);

 private:
  // Format-specific implementations
  static std::string formatText(const UnifiedViolation& violation);
  static std::string formatJSON(const UnifiedViolation& violation);
  static std::string formatRDF(const UnifiedViolation& violation,
                               const std::string& blankNodeId);
  static std::string formatMarkdown(const UnifiedViolation& violation);

  static std::string formatTextReport(const UnifiedValidationReport& report);
  static std::string formatJSONReport(const UnifiedValidationReport& report);
  static std::string formatRDFReport(const UnifiedValidationReport& report);
  static std::string formatMarkdownReport(const UnifiedValidationReport& report);

  // Helper methods
  static std::string escapeJSON(const std::string& str);
  static std::string escapeTurtle(const std::string& str);
  static std::string escapeMarkdown(const std::string& str);
  static std::string severityToString(SeverityLevel severity);
  static std::string severityToIRI(SeverityLevel severity);
  static std::string formalismToString(FormalismType formalism);
  static std::string resultTypeToString(ResultType type);

  // Generate unique blank node IDs for RDF output
  static std::string generateBlankNodeId(size_t index);
};

}  // namespace formalism

#endif  // QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDRESULT_H

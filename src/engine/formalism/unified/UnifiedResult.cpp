//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 5 - EPIC 14.0 Formalism Convergence

#include "UnifiedResult.h"
#include "engine/shacl/ShaclViolation.h"
#include "util/N3ComplianceVerifier.h"
#include "parser/DatalogRule.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

namespace formalism {

// ============================================================================
// UnifiedViolation Implementation
// ============================================================================

std::string UnifiedViolation::getFormalismName() const {
  switch (formalism) {
    case FormalismType::SHACL: return "SHACL";
    case FormalismType::N3: return "N3";
    case FormalismType::Datalog: return "Datalog";
    case FormalismType::ShEx: return "ShEx";
  }
  return "Unknown";
}

std::string UnifiedViolation::getSeverityString() const {
  switch (severity) {
    case SeverityLevel::Violation: return "Violation";
    case SeverityLevel::Warning: return "Warning";
    case SeverityLevel::Info: return "Info";
  }
  return "Unknown";
}

std::string UnifiedViolation::getResultTypeString() const {
  switch (resultType) {
    case ResultType::ValidationViolation: return "ValidationViolation";
    case ResultType::ComplianceIssue: return "ComplianceIssue";
    case ResultType::RuleDerivation: return "RuleDerivation";
    case ResultType::ParseError: return "ParseError";
    case ResultType::SemanticWarning: return "SemanticWarning";
  }
  return "Unknown";
}

std::string UnifiedViolation::toJSON(bool prettyPrint) const {
  std::ostringstream oss;
  const std::string indent = prettyPrint ? "  " : "";
  const std::string nl = prettyPrint ? "\n" : "";

  oss << "{" << nl;
  oss << indent << "\"formalism\": \"" << ViolationFormatter::escapeJSON(getFormalismName()) << "\"," << nl;
  oss << indent << "\"resultType\": \"" << ViolationFormatter::escapeJSON(getResultTypeString()) << "\"," << nl;
  oss << indent << "\"severity\": \"" << ViolationFormatter::escapeJSON(getSeverityString()) << "\"," << nl;
  oss << indent << "\"focusNode\": \"" << ViolationFormatter::escapeJSON(focusNode) << "\"";

  if (resultPath) {
    oss << "," << nl << indent << "\"resultPath\": \"" << ViolationFormatter::escapeJSON(*resultPath) << "\"";
  }
  if (sourceShape) {
    oss << "," << nl << indent << "\"sourceShape\": \"" << ViolationFormatter::escapeJSON(*sourceShape) << "\"";
  }
  if (sourceConstraintComponent) {
    oss << "," << nl << indent << "\"sourceConstraintComponent\": \"" << ViolationFormatter::escapeJSON(*sourceConstraintComponent) << "\"";
  }

  if (!values.empty()) {
    oss << "," << nl << indent << "\"values\": [";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << "\"" << ViolationFormatter::escapeJSON(values[i]) << "\"";
    }
    oss << "]";
  }

  if (expectedValue) {
    oss << "," << nl << indent << "\"expectedValue\": \"" << ViolationFormatter::escapeJSON(*expectedValue) << "\"";
  }

  if (!message.empty()) {
    oss << "," << nl << indent << "\"message\": \"" << ViolationFormatter::escapeJSON(message) << "\"";
  }

  if (!metadata.empty()) {
    oss << "," << nl << indent << "\"metadata\": {" << nl;
    bool first = true;
    for (const auto& [key, value] : metadata) {
      if (!first) oss << "," << nl;
      oss << indent << indent << "\"" << ViolationFormatter::escapeJSON(key) << "\": \""
          << ViolationFormatter::escapeJSON(value) << "\"";
      first = false;
    }
    oss << nl << indent << "}";
  }

  if (location) {
    oss << "," << nl << indent << "\"location\": {";
    bool first = true;
    if (location->filename) {
      oss << nl << indent << indent << "\"filename\": \"" << ViolationFormatter::escapeJSON(*location->filename) << "\"";
      first = false;
    }
    if (location->lineNumber) {
      if (!first) oss << ",";
      oss << nl << indent << indent << "\"lineNumber\": " << *location->lineNumber;
      first = false;
    }
    if (location->columnNumber) {
      if (!first) oss << ",";
      oss << nl << indent << indent << "\"columnNumber\": " << *location->columnNumber;
      first = false;
    }
    if (location->lineContent) {
      if (!first) oss << ",";
      oss << nl << indent << indent << "\"lineContent\": \"" << ViolationFormatter::escapeJSON(*location->lineContent) << "\"";
    }
    oss << nl << indent << "}";
  }

  oss << nl << "}";
  return oss.str();
}

std::string UnifiedViolation::toRDF(const std::string& blankNodeId) const {
  std::ostringstream oss;

  oss << blankNodeId << " a sh:ValidationResult ;\n";
  oss << "  sh:focusNode <" << ViolationFormatter::escapeTurtle(focusNode) << "> ;\n";
  oss << "  sh:resultSeverity " << ViolationFormatter::severityToIRI(severity) << " ;\n";
  oss << "  qlever:formalism \"" << ViolationFormatter::escapeTurtle(getFormalismName()) << "\" ;\n";
  oss << "  qlever:resultType \"" << ViolationFormatter::escapeTurtle(getResultTypeString()) << "\" ";

  if (resultPath) {
    oss << ";\n  sh:resultPath <" << ViolationFormatter::escapeTurtle(*resultPath) << "> ";
  }
  if (sourceShape) {
    oss << ";\n  sh:sourceShape <" << ViolationFormatter::escapeTurtle(*sourceShape) << "> ";
  }
  if (sourceConstraintComponent) {
    oss << ";\n  sh:sourceConstraintComponent <" << ViolationFormatter::escapeTurtle(*sourceConstraintComponent) << "> ";
  }

  for (const auto& value : values) {
    oss << ";\n  sh:value \"" << ViolationFormatter::escapeTurtle(value) << "\" ";
  }

  if (!message.empty()) {
    oss << ";\n  sh:resultMessage \"" << ViolationFormatter::escapeTurtle(message) << "\" ";
  }

  oss << ".\n";
  return oss.str();
}

std::string UnifiedViolation::toSPARQLBinding() const {
  std::ostringstream oss;

  oss << "?focusNode=\"" << focusNode << "\" ";
  if (resultPath) {
    oss << "?resultPath=\"" << *resultPath << "\" ";
  }
  if (!values.empty()) {
    for (size_t i = 0; i < values.size(); ++i) {
      oss << "?value" << i << "=\"" << values[i] << "\" ";
    }
  }
  oss << "?severity=\"" << getSeverityString() << "\"";

  return oss.str();
}

std::string UnifiedViolation::getSummary() const {
  std::ostringstream oss;
  oss << "[" << getSeverityString() << "] " << focusNode;
  if (resultPath) {
    oss << " / " << *resultPath;
  }
  return oss.str();
}

std::string UnifiedViolation::getDetailedDescription() const {
  std::ostringstream oss;

  oss << getSummary() << "\n";
  if (sourceConstraintComponent) {
    oss << "  Constraint: " << *sourceConstraintComponent << "\n";
  }
  if (sourceShape) {
    oss << "  Shape: " << *sourceShape << "\n";
  }
  if (!values.empty()) {
    oss << "  Values: ";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << "\"" << values[i] << "\"";
    }
    oss << "\n";
  }
  if (expectedValue) {
    oss << "  Expected: " << *expectedValue << "\n";
  }
  if (!message.empty()) {
    oss << "  Message: " << message << "\n";
  }
  if (location && location->lineNumber) {
    oss << "  Location: line " << *location->lineNumber;
    if (location->filename) {
      oss << " in " << *location->filename;
    }
    oss << "\n";
  }

  return oss.str();
}

// ============================================================================
// UnifiedValidationReport Implementation
// ============================================================================

void UnifiedValidationReport::addResult(const UnifiedViolation& result) {
  results.push_back(result);
  updateIndices(result);
}

void UnifiedValidationReport::addResult(UnifiedViolation&& result) {
  updateIndices(result);
  results.push_back(std::move(result));
}

void UnifiedValidationReport::updateIndices(const UnifiedViolation& result) {
  // Update counts
  if (result.isError()) {
    violationCount++;
    conforms = false;
  } else if (result.isWarning()) {
    warningCount++;
  } else if (result.isInfo()) {
    infoCount++;
  }

  // Update indices
  resultsByFocusNode[result.focusNode].push_back(result);
  resultsBySeverity[result.severity].push_back(result);
  resultsByType[result.resultType].push_back(result);
}

const std::vector<UnifiedViolation>& UnifiedValidationReport::getResultsForFocusNode(
    const std::string& focusNode) const {
  static const std::vector<UnifiedViolation> empty;
  auto it = resultsByFocusNode.find(focusNode);
  return it != resultsByFocusNode.end() ? it->second : empty;
}

std::vector<UnifiedViolation> UnifiedValidationReport::getResultsBySeverity(
    SeverityLevel severity) const {
  auto it = resultsBySeverity.find(severity);
  return it != resultsBySeverity.end() ? it->second : std::vector<UnifiedViolation>();
}

std::vector<UnifiedViolation> UnifiedValidationReport::getResultsByType(
    ResultType type) const {
  auto it = resultsByType.find(type);
  return it != resultsByType.end() ? it->second : std::vector<UnifiedViolation>();
}

UnifiedValidationReport::Statistics UnifiedValidationReport::getStatistics() const {
  Statistics stats;
  stats.totalResults = results.size();
  stats.violations = violationCount;
  stats.warnings = warningCount;
  stats.infos = infoCount;
  stats.uniqueFocusNodes = resultsByFocusNode.size();

  for (const auto& result : results) {
    if (result.sourceConstraintComponent) {
      stats.resultsByConstraint[*result.sourceConstraintComponent]++;
    }
    if (result.sourceShape) {
      stats.resultsByShape[*result.sourceShape]++;
    }
    stats.resultsByType[result.resultType]++;
  }

  return stats;
}

std::string UnifiedValidationReport::toJSON(bool prettyPrint) const {
  std::ostringstream oss;
  const std::string indent = prettyPrint ? "  " : "";
  const std::string nl = prettyPrint ? "\n" : "";

  oss << "{" << nl;
  oss << indent << "\"conforms\": " << (conforms ? "true" : "false") << "," << nl;
  oss << indent << "\"formalism\": \"" << ViolationFormatter::formalismToString(formalism) << "\"," << nl;
  oss << indent << "\"source\": \"" << ViolationFormatter::escapeJSON(source) << "\"," << nl;

  if (timestamp) {
    oss << indent << "\"timestamp\": \"" << ViolationFormatter::escapeJSON(*timestamp) << "\"," << nl;
  }

  oss << indent << "\"violationCount\": " << violationCount << "," << nl;
  oss << indent << "\"warningCount\": " << warningCount << "," << nl;
  oss << indent << "\"infoCount\": " << infoCount << "," << nl;
  oss << indent << "\"results\": [" << nl;

  for (size_t i = 0; i < results.size(); ++i) {
    if (i > 0) oss << "," << nl;
    std::string resultJSON = results[i].toJSON(prettyPrint);
    if (prettyPrint) {
      // Indent each line
      std::istringstream iss(resultJSON);
      std::string line;
      bool first = true;
      while (std::getline(iss, line)) {
        if (!first) oss << nl;
        oss << indent << indent << line;
        first = false;
      }
    } else {
      oss << resultJSON;
    }
  }

  oss << nl << indent << "]";

  if (!metadata.empty()) {
    oss << "," << nl << indent << "\"metadata\": {" << nl;
    bool first = true;
    for (const auto& [key, value] : metadata) {
      if (!first) oss << "," << nl;
      oss << indent << indent << "\"" << ViolationFormatter::escapeJSON(key) << "\": \""
          << ViolationFormatter::escapeJSON(value) << "\"";
      first = false;
    }
    oss << nl << indent << "}";
  }

  oss << nl << "}";
  return oss.str();
}

std::string UnifiedValidationReport::toRDF() const {
  std::ostringstream oss;

  oss << "@prefix sh: <http://www.w3.org/ns/shacl#> .\n";
  oss << "@prefix qlever: <http://qlever.cs.uni-freiburg.de/formalism#> .\n";
  oss << "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n\n";

  oss << "_:report a sh:ValidationReport ;\n";
  oss << "  sh:conforms " << (conforms ? "true" : "false") << " ;\n";
  oss << "  qlever:formalism \"" << ViolationFormatter::formalismToString(formalism) << "\" ;\n";
  oss << "  qlever:source \"" << ViolationFormatter::escapeTurtle(source) << "\" ";

  if (timestamp) {
    oss << ";\n  qlever:timestamp \"" << ViolationFormatter::escapeTurtle(*timestamp) << "\"^^xsd:dateTime ";
  }

  for (size_t i = 0; i < results.size(); ++i) {
    std::string blankNodeId = "_:result" + std::to_string(i);
    oss << ";\n  sh:result " << blankNodeId << " ";
  }

  oss << ".\n\n";

  // Emit each result
  for (size_t i = 0; i < results.size(); ++i) {
    std::string blankNodeId = "_:result" + std::to_string(i);
    oss << results[i].toRDF(blankNodeId) << "\n";
  }

  return oss.str();
}

std::string UnifiedValidationReport::getSummary() const {
  std::ostringstream oss;
  oss << "Validation Report: " << source << "\n";
  oss << "Conforms: " << (conforms ? "Yes" : "No") << "\n";
  oss << "Violations: " << violationCount << " | ";
  oss << "Warnings: " << warningCount << " | ";
  oss << "Info: " << infoCount << "\n";
  return oss.str();
}

std::string UnifiedValidationReport::getDetailedReport() const {
  return ViolationFormatter::format(*this, OutputFormat::Text);
}

// ============================================================================
// Conversion Functions
// ============================================================================

UnifiedViolation fromShaclViolation(const shacl::ShaclViolation& violation) {
  UnifiedViolation unified(FormalismType::SHACL, violation.focusNode);

  unified.resultType = ResultType::ValidationViolation;
  unified.severity = violation.severity;

  if (!violation.resultPath.empty()) {
    unified.resultPath = violation.resultPath;
  }
  if (!violation.sourceShape.empty()) {
    unified.sourceShape = violation.sourceShape;
  }
  if (!violation.sourceConstraintComponent.empty()) {
    unified.sourceConstraintComponent = violation.sourceConstraintComponent;
  }

  unified.values = violation.values;
  unified.expectedValue = violation.expectedValue;
  unified.message = violation.message;

  // Convert details to metadata
  for (const auto& [key, value] : violation.details) {
    unified.metadata[key] = value;
  }

  return unified;
}

UnifiedValidationReport fromShaclReport(const shacl::DetailedValidationReport& report) {
  UnifiedValidationReport unified(FormalismType::SHACL);
  unified.conforms = report.conforms;

  for (const auto& violation : report.violations) {
    unified.addResult(fromShaclViolation(violation));
  }

  return unified;
}

UnifiedViolation fromN3ComplianceIssue(const ad_utility::N3ComplianceIssue& issue,
                                       const std::string& filename) {
  std::string focusNode = filename.empty() ? "N3Document" : filename;
  UnifiedViolation unified(FormalismType::N3, focusNode);

  unified.resultType = ResultType::ComplianceIssue;
  unified.severity = issue.isSupported ? SeverityLevel::Info : SeverityLevel::Violation;

  unified.resultPath = issue.feature;
  unified.sourceShape = "N3ComplianceVerifier";
  unified.sourceConstraintComponent = "has" + issue.feature;

  unified.values.push_back(issue.lineContent);
  unified.expectedValue = issue.isSupported ? "supported" : "unsupported";
  unified.message = issue.description;

  // Add location info
  unified.withLocation(filename, issue.lineNumber);
  unified.withLineContent(issue.lineContent);

  unified.metadata["feature"] = issue.feature;
  unified.metadata["isSupported"] = issue.isSupported ? "true" : "false";

  return unified;
}

UnifiedValidationReport fromN3ComplianceVerifier(const ad_utility::N3ComplianceVerifier& verifier) {
  UnifiedValidationReport unified(FormalismType::N3);
  unified.conforms = verifier.isCompatible();

  for (const auto& issue : verifier.getIssues()) {
    std::string filename = ""; // Verifier should provide this
    unified.addResult(fromN3ComplianceIssue(issue, filename));
  }

  // Add summary metadata
  unified.metadata["totalLines"] = std::to_string(verifier.getTotalLines());
  unified.metadata["unsupportedFeatureCount"] = std::to_string(verifier.getUnsupportedFeatureCount());
  unified.metadata["supportedFeatureCount"] = std::to_string(verifier.getSupportedFeatureCount());

  return unified;
}

UnifiedViolation fromDatalogTuple(const DatalogRule& rule,
                                  const std::vector<std::string>& tuple,
                                  const std::vector<std::string>& variableNames) {
  UnifiedViolation unified(FormalismType::Datalog, rule.getHeadPredicate());

  unified.resultType = ResultType::RuleDerivation;
  unified.severity = SeverityLevel::Info; // Derivations are informational

  // Build variable binding context
  std::ostringstream pathOss;
  for (size_t i = 0; i < variableNames.size() && i < tuple.size(); ++i) {
    if (i > 0) pathOss << ", ";
    pathOss << variableNames[i] << "=" << tuple[i];
  }
  unified.resultPath = pathOss.str();

  unified.sourceShape = rule.toString();
  unified.values = tuple;

  std::ostringstream msgOss;
  msgOss << "Derived tuple for " << rule.getHeadPredicate() << "(";
  for (size_t i = 0; i < variableNames.size(); ++i) {
    if (i > 0) msgOss << ", ";
    msgOss << variableNames[i];
  }
  msgOss << ")";
  unified.message = msgOss.str();

  unified.metadata["arity"] = std::to_string(rule.getArity());
  unified.metadata["isRecursive"] = rule.isRecursive() ? "true" : "false";

  return unified;
}

UnifiedValidationReport createDatalogDerivationReport(
    const std::string& ruleName,
    const std::vector<std::vector<std::string>>& derivedTuples,
    const std::vector<std::string>& variableNames) {

  UnifiedValidationReport unified(FormalismType::Datalog, ruleName);
  unified.conforms = true; // Derivations don't violate

  // Create a minimal DatalogRule for metadata
  DatalogRule dummyRule;
  // Note: In actual implementation, we'd pass the real rule

  for (const auto& tuple : derivedTuples) {
    unified.addResult(fromDatalogTuple(dummyRule, tuple, variableNames));
  }

  unified.metadata["totalDerivations"] = std::to_string(derivedTuples.size());

  return unified;
}

// ============================================================================
// ViolationFormatter Implementation
// ============================================================================

std::string ViolationFormatter::escapeJSON(const std::string& str) {
  std::ostringstream oss;
  for (char c : str) {
    switch (c) {
      case '"': oss << "\\\""; break;
      case '\\': oss << "\\\\"; break;
      case '\n': oss << "\\n"; break;
      case '\r': oss << "\\r"; break;
      case '\t': oss << "\\t"; break;
      default: oss << c;
    }
  }
  return oss.str();
}

std::string ViolationFormatter::escapeTurtle(const std::string& str) {
  std::ostringstream oss;
  for (char c : str) {
    switch (c) {
      case '\\': oss << "\\\\"; break;
      case '"': oss << "\\\""; break;
      case '\n': oss << "\\n"; break;
      case '\r': oss << "\\r"; break;
      case '\t': oss << "\\t"; break;
      default: oss << c;
    }
  }
  return oss.str();
}

std::string ViolationFormatter::escapeMarkdown(const std::string& str) {
  std::ostringstream oss;
  for (char c : str) {
    // Escape special markdown characters
    if (c == '*' || c == '_' || c == '[' || c == ']' || c == '`') {
      oss << '\\';
    }
    oss << c;
  }
  return oss.str();
}

std::string ViolationFormatter::severityToString(SeverityLevel severity) {
  switch (severity) {
    case SeverityLevel::Violation: return "Violation";
    case SeverityLevel::Warning: return "Warning";
    case SeverityLevel::Info: return "Info";
  }
  return "Unknown";
}

std::string ViolationFormatter::severityToIRI(SeverityLevel severity) {
  switch (severity) {
    case SeverityLevel::Violation: return "<http://www.w3.org/ns/shacl#Violation>";
    case SeverityLevel::Warning: return "<http://www.w3.org/ns/shacl#Warning>";
    case SeverityLevel::Info: return "<http://www.w3.org/ns/shacl#Info>";
  }
  return "<http://www.w3.org/ns/shacl#Violation>";
}

std::string ViolationFormatter::formalismToString(FormalismType formalism) {
  switch (formalism) {
    case FormalismType::SHACL: return "SHACL";
    case FormalismType::N3: return "N3";
    case FormalismType::Datalog: return "Datalog";
    case FormalismType::ShEx: return "ShEx";
  }
  return "Unknown";
}

std::string ViolationFormatter::resultTypeToString(ResultType type) {
  switch (type) {
    case ResultType::ValidationViolation: return "ValidationViolation";
    case ResultType::ComplianceIssue: return "ComplianceIssue";
    case ResultType::RuleDerivation: return "RuleDerivation";
    case ResultType::ParseError: return "ParseError";
    case ResultType::SemanticWarning: return "SemanticWarning";
  }
  return "Unknown";
}

std::string ViolationFormatter::generateBlankNodeId(size_t index) {
  return "_:result" + std::to_string(index);
}

std::string ViolationFormatter::format(const UnifiedViolation& violation,
                                       OutputFormat format) {
  switch (format) {
    case OutputFormat::Text: return formatText(violation);
    case OutputFormat::JSON: return formatJSON(violation);
    case OutputFormat::RDF: return formatRDF(violation, "_:result");
    case OutputFormat::Markdown: return formatMarkdown(violation);
    case OutputFormat::SPARQL: return violation.toSPARQLBinding();
  }
  return "";
}

std::string ViolationFormatter::formatText(const UnifiedViolation& violation) {
  return violation.getDetailedDescription();
}

std::string ViolationFormatter::formatJSON(const UnifiedViolation& violation) {
  return violation.toJSON(true);
}

std::string ViolationFormatter::formatRDF(const UnifiedViolation& violation,
                                          const std::string& blankNodeId) {
  return violation.toRDF(blankNodeId);
}

std::string ViolationFormatter::formatMarkdown(const UnifiedViolation& violation) {
  std::ostringstream oss;

  oss << "### [" << violation.getSeverityString() << "] " << violation.focusNode;
  if (violation.resultPath) {
    oss << " / " << *violation.resultPath;
  }
  oss << "\n\n";

  if (violation.sourceConstraintComponent) {
    oss << "- **Constraint**: " << escapeMarkdown(*violation.sourceConstraintComponent) << "\n";
  }
  if (violation.sourceShape) {
    oss << "- **Shape**: " << escapeMarkdown(*violation.sourceShape) << "\n";
  }
  if (!violation.values.empty()) {
    oss << "- **Values**: ";
    for (size_t i = 0; i < violation.values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << "`" << escapeMarkdown(violation.values[i]) << "`";
    }
    oss << "\n";
  }
  if (violation.expectedValue) {
    oss << "- **Expected**: " << escapeMarkdown(*violation.expectedValue) << "\n";
  }
  if (!violation.message.empty()) {
    oss << "- **Message**: " << escapeMarkdown(violation.message) << "\n";
  }
  if (violation.location && violation.location->lineNumber) {
    oss << "- **Location**: Line " << *violation.location->lineNumber;
    if (violation.location->filename) {
      oss << " in " << escapeMarkdown(*violation.location->filename);
    }
    oss << "\n";
  }

  oss << "\n";
  return oss.str();
}

std::string ViolationFormatter::format(const UnifiedValidationReport& report,
                                       OutputFormat format) {
  switch (format) {
    case OutputFormat::Text: return formatTextReport(report);
    case OutputFormat::JSON: return formatJSONReport(report);
    case OutputFormat::RDF: return formatRDFReport(report);
    case OutputFormat::Markdown: return formatMarkdownReport(report);
    case OutputFormat::SPARQL: return formatTextReport(report); // Fallback
  }
  return "";
}

std::string ViolationFormatter::formatTextReport(const UnifiedValidationReport& report) {
  std::ostringstream oss;

  oss << "=== Validation Report ===\n";
  oss << "Source: " << report.source << "\n";
  oss << "Formalism: " << formalismToString(report.formalism) << "\n";
  oss << "Conforms: " << (report.conforms ? "Yes" : "No") << "\n";
  oss << "Violations: " << report.violationCount << " | ";
  oss << "Warnings: " << report.warningCount << " | ";
  oss << "Info: " << report.infoCount << "\n";
  oss << "========================\n\n";

  for (const auto& result : report.results) {
    oss << result.getDetailedDescription() << "\n";
  }

  return oss.str();
}

std::string ViolationFormatter::formatJSONReport(const UnifiedValidationReport& report) {
  return report.toJSON(true);
}

std::string ViolationFormatter::formatRDFReport(const UnifiedValidationReport& report) {
  return report.toRDF();
}

std::string ViolationFormatter::formatMarkdownReport(const UnifiedValidationReport& report) {
  std::ostringstream oss;

  oss << "# Validation Report: " << escapeMarkdown(report.source) << "\n\n";
  oss << "**Formalism**: " << formalismToString(report.formalism) << "\n";
  oss << "**Conforms**: " << (report.conforms ? "✅ Yes" : "❌ No") << "\n";
  oss << "**Violations**: " << report.violationCount << " | ";
  oss << "**Warnings**: " << report.warningCount << " | ";
  oss << "**Info**: " << report.infoCount << "\n\n";
  oss << "---\n\n";

  if (report.violationCount > 0) {
    oss << "## Violations\n\n";
    for (const auto& result : report.results) {
      if (result.isError()) {
        oss << formatMarkdown(result);
      }
    }
  }

  if (report.warningCount > 0) {
    oss << "## Warnings\n\n";
    for (const auto& result : report.results) {
      if (result.isWarning()) {
        oss << formatMarkdown(result);
      }
    }
  }

  if (report.infoCount > 0) {
    oss << "## Info\n\n";
    for (const auto& result : report.results) {
      if (result.isInfo()) {
        oss << formatMarkdown(result);
      }
    }
  }

  oss << "---\n\n";
  oss << "## Summary\n\n";
  oss << "- **Total Issues**: " << report.getTotalIssues() << "\n";
  oss << "- **Unique Focus Nodes**: " << report.resultsByFocusNode.size() << "\n";

  return oss.str();
}

std::string ViolationFormatter::formatSummary(const UnifiedValidationReport& report) {
  return report.getSummary();
}

void ViolationFormatter::format(const UnifiedValidationReport& report,
                                std::ostream& out,
                                OutputFormat format) {
  out << ViolationFormatter::format(report, format);
}

std::string ViolationFormatter::formatByFocusNode(const UnifiedValidationReport& report,
                                                  OutputFormat format) {
  std::ostringstream oss;

  oss << "=== Results Grouped by Focus Node ===\n\n";
  for (const auto& [focusNode, results] : report.resultsByFocusNode) {
    oss << "Focus Node: " << focusNode << " (" << results.size() << " results)\n";
    for (const auto& result : results) {
      oss << "  " << result.getSummary() << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

std::string ViolationFormatter::formatBySeverity(const UnifiedValidationReport& report,
                                                 OutputFormat format) {
  std::ostringstream oss;

  oss << "=== Results Grouped by Severity ===\n\n";

  auto violations = report.getResultsBySeverity(SeverityLevel::Violation);
  if (!violations.empty()) {
    oss << "Violations (" << violations.size() << "):\n";
    for (const auto& result : violations) {
      oss << "  " << result.getSummary() << "\n";
    }
    oss << "\n";
  }

  auto warnings = report.getResultsBySeverity(SeverityLevel::Warning);
  if (!warnings.empty()) {
    oss << "Warnings (" << warnings.size() << "):\n";
    for (const auto& result : warnings) {
      oss << "  " << result.getSummary() << "\n";
    }
    oss << "\n";
  }

  auto infos = report.getResultsBySeverity(SeverityLevel::Info);
  if (!infos.empty()) {
    oss << "Info (" << infos.size() << "):\n";
    for (const auto& result : infos) {
      oss << "  " << result.getSummary() << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

std::string ViolationFormatter::formatByConstraint(const UnifiedValidationReport& report,
                                                   OutputFormat format) {
  std::ostringstream oss;

  oss << "=== Results Grouped by Constraint ===\n\n";

  std::unordered_map<std::string, std::vector<UnifiedViolation>> byConstraint;
  for (const auto& result : report.results) {
    if (result.sourceConstraintComponent) {
      byConstraint[*result.sourceConstraintComponent].push_back(result);
    }
  }

  for (const auto& [constraint, results] : byConstraint) {
    oss << "Constraint: " << constraint << " (" << results.size() << " results)\n";
    for (const auto& result : results) {
      oss << "  " << result.getSummary() << "\n";
    }
    oss << "\n";
  }

  return oss.str();
}

}  // namespace formalism

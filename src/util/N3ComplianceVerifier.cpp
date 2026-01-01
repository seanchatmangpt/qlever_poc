// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include "util/N3ComplianceVerifier.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>

#include "util/Exception.h"
#include "util/Log.h"

namespace ad_utility {

// ____________________________________________________________________________
void N3ComplianceVerifier::analyzeFile(const std::string& filename) {
  filename_ = filename;
  issues_.clear();
  featureUsage_.clear();
  totalLines_ = 0;
  unsupportedFeatureCount_ = 0;
  supportedFeatureCount_ = 0;
  fileExists_ = false;
  parseSuccess_ = false;

  std::ifstream file(filename);
  if (!file.is_open()) {
    AD_LOG_ERROR << "Failed to open file: " << filename << std::endl;
    return;
  }

  fileExists_ = true;
  std::string line;
  size_t lineNumber = 0;

  while (std::getline(file, line)) {
    lineNumber++;
    totalLines_++;
    analyzeLineForFeatures(line, lineNumber);
  }

  parseSuccess_ = true;
  AD_LOG_INFO << "Analysis complete: " << filename << " (" << totalLines_
              << " lines)" << std::endl;
}

// ____________________________________________________________________________
void N3ComplianceVerifier::analyzeLineForFeatures(const std::string& line,
                                                   size_t lineNumber) {
  // Skip empty lines and comments
  std::string trimmed = line;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
  if (trimmed.empty() || trimmed[0] == '#') {
    return;
  }

  // Detect advanced (unsupported) N3 features first
  detectAdvancedN3Features(line, lineNumber);

  // Detect supported features
  detectSupportedFeatures(line, lineNumber);
}

// ____________________________________________________________________________
void N3ComplianceVerifier::detectAdvancedN3Features(const std::string& line,
                                                     size_t lineNumber) {
  if (hasFormulae(line)) {
    addIssue("Formulae/Quoted Graphs",
             "N3 formulae (quoted graphs) using { } are not supported", lineNumber,
             line, false);
    recordFeatureUsage("formulae");
  }

  if (hasImplication(line)) {
    addIssue("Implication Rules",
             "N3 implications (=>, <=) are not supported", lineNumber, line,
             false);
    recordFeatureUsage("implication");
  }

  if (hasQuantifier(line)) {
    addIssue("Quantifiers",
             "N3 quantifiers (@forAll, @forSome) are not supported", lineNumber,
             line, false);
    recordFeatureUsage("quantifier");
  }

  if (hasVariable(line)) {
    addIssue("Variables", "N3 variables (?var) are not supported", lineNumber,
             line, false);
    recordFeatureUsage("variable");
  }

  if (hasBuiltinFunction(line)) {
    addIssue("Built-in Functions",
             "N3 built-in functions (log:, math:, etc.) are not supported",
             lineNumber, line, false);
    recordFeatureUsage("builtin_function");
  }

  if (hasN3Path(line)) {
    addIssue("N3 Paths", "N3 path expressions (!,:^) may not be fully supported",
             lineNumber, line, false);
    recordFeatureUsage("n3_path");
  }
}

// ____________________________________________________________________________
void N3ComplianceVerifier::detectSupportedFeatures(const std::string& line,
                                                    size_t lineNumber) {
  // Check for prefixes
  std::regex prefixPattern(R"(^\s*@prefix\s+\w*:\s*<[^>]+>\s*\.)");
  std::regex sparqlPrefixPattern(R"(^\s*PREFIX\s+\w*:\s*<[^>]+>)");
  if (std::regex_search(line, prefixPattern) ||
      std::regex_search(line, sparqlPrefixPattern)) {
    recordFeatureUsage("prefix");
  }

  // Check for base
  std::regex basePattern(R"(^\s*@base\s+<[^>]+>\s*\.)");
  std::regex sparqlBasePattern(R"(^\s*BASE\s+<[^>]+>)");
  if (std::regex_search(line, basePattern) ||
      std::regex_search(line, sparqlBasePattern)) {
    recordFeatureUsage("base");
  }

  // Check for typed literals
  std::regex typedLiteralPattern(R"(\"\^\\")");
  if (std::regex_search(line, typedLiteralPattern)) {
    recordFeatureUsage("typed_literal");
  }

  // Check for language tags
  std::regex langTagPattern(R"(\"@[a-z]{2}(-[A-Z]{2})?)");
  if (std::regex_search(line, langTagPattern)) {
    recordFeatureUsage("language_tag");
  }

  // Check for blank nodes
  std::regex blankNodePattern(R"(\[\s*\]|_:[a-zA-Z0-9]+)");
  if (std::regex_search(line, blankNodePattern)) {
    recordFeatureUsage("blank_node");
  }

  // Check for collections
  std::regex collectionPattern(R"(\(\s*[^)]+\s*\))");
  if (std::regex_search(line, collectionPattern)) {
    recordFeatureUsage("collection");
  }

  // Check for numeric literals
  std::regex numericPattern(R"(\s+\d+(\.\d+)?\s*[;.,])");
  if (std::regex_search(line, numericPattern)) {
    recordFeatureUsage("numeric_literal");
  }

  // Check for boolean literals
  if (line.find("true") != std::string::npos ||
      line.find("false") != std::string::npos) {
    recordFeatureUsage("boolean_literal");
  }
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasFormulae(const std::string& line) const {
  // Simple heuristic: look for curly braces used as graph delimiters
  // Need to avoid false positives from IRIs containing { }
  size_t openBrace = line.find('{');
  size_t closeBrace = line.find('}');

  if (openBrace == std::string::npos && closeBrace == std::string::npos) {
    return false;
  }

  // Check if braces are likely part of a formula (not inside IRI)
  if (openBrace != std::string::npos) {
    size_t iriStart = line.rfind('<', openBrace);
    size_t iriEnd = line.find('>', openBrace);
    if (iriStart != std::string::npos && iriEnd != std::string::npos &&
        iriStart < openBrace && openBrace < iriEnd) {
      return false;  // Brace is inside an IRI
    }
    return true;
  }

  return closeBrace != std::string::npos;
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasImplication(const std::string& line) const {
  // Check for => (implies) or <= (is implied by)
  return line.find("=>") != std::string::npos ||
         line.find("<=") != std::string::npos ||
         line.find("log:implies") != std::string::npos;
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasQuantifier(const std::string& line) const {
  return line.find("@forAll") != std::string::npos ||
         line.find("@forSome") != std::string::npos;
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasVariable(const std::string& line) const {
  // N3 variables start with ? or $
  // Need to avoid matching ? in IRIs or strings
  std::regex variablePattern(R"(\s+\?[a-zA-Z][a-zA-Z0-9]*\s+)");
  return std::regex_search(line, variablePattern);
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasBuiltinFunction(const std::string& line) const {
  // Common N3 built-in namespaces
  return line.find("log:") != std::string::npos ||
         line.find("math:") != std::string::npos ||
         line.find("string:") != std::string::npos ||
         line.find("list:") != std::string::npos ||
         line.find("time:") != std::string::npos ||
         line.find("crypto:") != std::string::npos;
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::hasN3Path(const std::string& line) const {
  // N3 path operators: ! (forward path), ^ (backward path)
  // Be careful not to match these inside strings or IRIs
  std::regex pathPattern(R"(\s+[!^]\s+)");
  return std::regex_search(line, pathPattern);
}

// ____________________________________________________________________________
void N3ComplianceVerifier::addIssue(const std::string& feature,
                                    const std::string& description,
                                    size_t lineNumber,
                                    const std::string& lineContent,
                                    bool isSupported) {
  issues_.push_back({feature, description, lineNumber, lineContent, isSupported});
  if (isSupported) {
    supportedFeatureCount_++;
  } else {
    unsupportedFeatureCount_++;
  }
}

// ____________________________________________________________________________
void N3ComplianceVerifier::recordFeatureUsage(const std::string& feature) {
  featureUsage_[feature]++;
}

// ____________________________________________________________________________
bool N3ComplianceVerifier::isCompatible() const {
  // File is compatible if parsing succeeded and no unsupported features found
  return parseSuccess_ && unsupportedFeatureCount_ == 0;
}

// ____________________________________________________________________________
std::map<std::string, bool> N3ComplianceVerifier::featureMatrix() const {
  std::map<std::string, bool> matrix;

  // Supported features
  matrix["@prefix / PREFIX"] = featureUsage_.count("prefix") > 0;
  matrix["@base / BASE"] = featureUsage_.count("base") > 0;
  matrix["Typed Literals"] = featureUsage_.count("typed_literal") > 0;
  matrix["Language Tags"] = featureUsage_.count("language_tag") > 0;
  matrix["Blank Nodes"] = featureUsage_.count("blank_node") > 0;
  matrix["Collections"] = featureUsage_.count("collection") > 0;
  matrix["Numeric Literals"] = featureUsage_.count("numeric_literal") > 0;
  matrix["Boolean Literals"] = featureUsage_.count("boolean_literal") > 0;

  // Unsupported features (should be false for compatibility)
  matrix["Formulae (UNSUPPORTED)"] = featureUsage_.count("formulae") > 0;
  matrix["Implications (UNSUPPORTED)"] = featureUsage_.count("implication") > 0;
  matrix["Quantifiers (UNSUPPORTED)"] = featureUsage_.count("quantifier") > 0;
  matrix["Variables (UNSUPPORTED)"] = featureUsage_.count("variable") > 0;
  matrix["Built-in Functions (UNSUPPORTED)"] =
      featureUsage_.count("builtin_function") > 0;
  matrix["N3 Paths (UNSUPPORTED)"] = featureUsage_.count("n3_path") > 0;

  return matrix;
}

// ____________________________________________________________________________
std::string N3ComplianceVerifier::generateReport() const {
  std::ostringstream report;

  report << "# N3 Format Compliance Report\n\n";
  report << generateSummarySection();
  report << generateFeatureMatrixSection();
  report << generateIssuesSection();
  report << generateRecommendationsSection();

  return report.str();
}

// ____________________________________________________________________________
std::string N3ComplianceVerifier::generateSummarySection() const {
  std::ostringstream section;

  section << "## Summary\n\n";
  section << "**File:** `" << filename_ << "`\n\n";
  section << "**Total Lines:** " << totalLines_ << "\n\n";
  section << "**Compatibility Status:** ";

  if (!fileExists_) {
    section << "ERROR - File not found\n\n";
  } else if (!parseSuccess_) {
    section << "ERROR - Analysis failed\n\n";
  } else if (isCompatible()) {
    section << "COMPATIBLE\n\n";
    section << "This file uses only supported Turtle/N3 features and can be "
               "loaded by QLever.\n\n";
  } else {
    section << "INCOMPATIBLE\n\n";
    section << "This file uses " << unsupportedFeatureCount_
            << " unsupported N3 feature(s) that will cause parsing errors in "
               "QLever.\n\n";
  }

  return section.str();
}

// ____________________________________________________________________________
std::string N3ComplianceVerifier::generateFeatureMatrixSection() const {
  std::ostringstream section;

  section << "## Feature Matrix\n\n";
  section << "| Feature | Used in File | Supported by QLever |\n";
  section << "|---------|--------------|---------------------|\n";

  auto matrix = featureMatrix();

  // First show supported features
  std::vector<std::string> supportedFeatures = {
      "@prefix / PREFIX", "@ / BASE",          "Typed Literals",
      "Language Tags",    "Blank Nodes",       "Collections",
      "Numeric Literals", "Boolean Literals"};

  for (const auto& feature : supportedFeatures) {
    bool used = matrix.at(feature);
    section << "| " << feature << " | " << (used ? "Yes" : "No") << " | Yes |\n";
  }

  // Then show unsupported features
  std::vector<std::string> unsupportedFeatures = {
      "Formulae (UNSUPPORTED)",       "Implications (UNSUPPORTED)",
      "Quantifiers (UNSUPPORTED)",    "Variables (UNSUPPORTED)",
      "Built-in Functions (UNSUPPORTED)", "N3 Paths (UNSUPPORTED)"};

  for (const auto& feature : unsupportedFeatures) {
    bool used = matrix.at(feature);
    section << "| " << feature << " | " << (used ? "**Yes**" : "No")
            << " | No |\n";
  }

  section << "\n";
  return section.str();
}

// ____________________________________________________________________________
std::string N3ComplianceVerifier::generateIssuesSection() const {
  std::ostringstream section;

  if (issues_.empty()) {
    section << "## Issues\n\n";
    section << "No issues detected. File appears to use only basic Turtle/N3 "
               "syntax.\n\n";
    return section.str();
  }

  section << "## Detected Issues\n\n";
  section << "The following unsupported features were detected:\n\n";

  for (const auto& issue : issues_) {
    section << "### Line " << issue.lineNumber << ": " << issue.feature << "\n\n";
    section << "**Description:** " << issue.description << "\n\n";
    section << "**Line content:**\n```\n"
            << issue.lineContent << "\n```\n\n";
  }

  return section.str();
}

// ____________________________________________________________________________
std::string N3ComplianceVerifier::generateRecommendationsSection() const {
  std::ostringstream section;

  section << "## Recommendations\n\n";

  if (isCompatible()) {
    section << "- This file is ready to be loaded into QLever\n";
    section << "- Use IndexBuilderMain with the N3 parser to index this "
               "data\n\n";
    return section.str();
  }

  section << "To make this file compatible with QLever:\n\n";

  if (featureUsage_.count("formulae") > 0) {
    section << "- **Formulae:** Convert quoted graphs to named graphs or "
               "reification\n";
  }

  if (featureUsage_.count("implication") > 0) {
    section << "- **Implications:** Materialize rule results or use external "
               "reasoner\n";
  }

  if (featureUsage_.count("quantifier") > 0) {
    section << "- **Quantifiers:** Remove or convert to concrete triples\n";
  }

  if (featureUsage_.count("variable") > 0) {
    section << "- **Variables:** Replace with concrete values or remove\n";
  }

  if (featureUsage_.count("builtin_function") > 0) {
    section << "- **Built-in Functions:** Pre-compute results and store as "
               "triples\n";
  }

  if (featureUsage_.count("n3_path") > 0) {
    section << "- **N3 Paths:** Convert to standard SPARQL property paths "
               "after loading\n";
  }

  section << "\nAlternatively, use a full N3 reasoner (like EYE or cwm) to "
             "process this file\n";
  section << "and convert the results to Turtle format before loading into "
             "QLever.\n\n";

  return section.str();
}

}  // namespace ad_utility

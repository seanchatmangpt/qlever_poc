#ifndef QLEVER_ENGINE_SHACL_VIOLATIONFORMATTER_H
#define QLEVER_ENGINE_SHACL_VIOLATIONFORMATTER_H

#include "ShaclViolation.h"
#include <string>
#include <ostream>

namespace shacl {

// Output format for violation reports
enum class ViolationFormat {
  Text,     // Human-readable text format
  JSON,     // JSON format
  Turtle,   // RDF Turtle format (SHACL validation report)
  Summary   // Condensed summary format
};

// Formatter for SHACL violation reports
// Supports multiple output formats for different use cases
class ViolationFormatter {
 public:
  // Format a single violation
  static std::string format(const ShaclViolation& violation,
                            ViolationFormat format = ViolationFormat::Text);

  // Format a complete validation report
  static std::string format(const DetailedValidationReport& report,
                            ViolationFormat format = ViolationFormat::Text);

  // Format to an output stream
  static void format(const DetailedValidationReport& report,
                     std::ostream& out,
                     ViolationFormat format = ViolationFormat::Text);

  // Format just the summary statistics
  static std::string formatSummary(const DetailedValidationReport& report);

  // Format violations grouped by focus node
  static std::string formatByFocusNode(const DetailedValidationReport& report,
                                       ViolationFormat format = ViolationFormat::Text);

  // Format violations grouped by shape
  static std::string formatByShape(const DetailedValidationReport& report,
                                   ViolationFormat format = ViolationFormat::Text);

  // Format violations grouped by constraint type
  static std::string formatByConstraint(
      const DetailedValidationReport& report,
      ViolationFormat format = ViolationFormat::Text);

 private:
  // Format-specific implementations for single violation
  static std::string formatText(const ShaclViolation& violation);
  static std::string formatJSON(const ShaclViolation& violation);
  static std::string formatTurtle(const ShaclViolation& violation);

  // Format-specific implementations for report
  static std::string formatTextReport(const DetailedValidationReport& report);
  static std::string formatJSONReport(const DetailedValidationReport& report);
  static std::string formatTurtleReport(const DetailedValidationReport& report);

  // Helper methods
  static std::string escapeJSON(const std::string& str);
  static std::string escapeTurtle(const std::string& str);
  static std::string severityToString(SeverityLevel severity);
  static std::string severityToIRI(SeverityLevel severity);

  // Generate unique blank node IDs for RDF output
  static std::string generateBlankNodeId(size_t index);
};

// Utility class for pretty-printing violation reports
class ViolationPrinter {
 public:
  explicit ViolationPrinter(std::ostream& out) : out_(out) {}

  // Print a single violation with optional indentation
  void printViolation(const ShaclViolation& violation, int indent = 0);

  // Print the full report with sections
  void printReport(const DetailedValidationReport& report);

  // Print summary statistics
  void printSummary(const DetailedValidationReport::Summary& summary);

  // Print violations grouped by focus node
  void printByFocusNode(const DetailedValidationReport& report);

  // Print violations grouped by shape
  void printByShape(const DetailedValidationReport& report);

  // Print violations grouped by constraint
  void printByConstraint(const DetailedValidationReport& report);

  // Configure output options
  void setShowDetails(bool show) { showDetails_ = show; }
  void setColorOutput(bool color) { useColor_ = color; }
  void setMaxViolationsPerGroup(size_t max) { maxPerGroup_ = max; }

 private:
  std::ostream& out_;
  bool showDetails_ = true;
  bool useColor_ = false;
  size_t maxPerGroup_ = 100;

  void printIndent(int level);
  void printSeparator(char c = '-', int length = 80);
  std::string colorize(const std::string& text, const std::string& color);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_VIOLATIONFORMATTER_H

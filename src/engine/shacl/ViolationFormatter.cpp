#include "ViolationFormatter.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace shacl {

// Helper function implementations
std::string ViolationFormatter::escapeJSON(const std::string& str) {
  std::ostringstream oss;
  for (char c : str) {
    switch (c) {
      case '"':
        oss << "\\\"";
        break;
      case '\\':
        oss << "\\\\";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << c;
    }
  }
  return oss.str();
}

std::string ViolationFormatter::escapeTurtle(const std::string& str) {
  std::ostringstream oss;
  for (char c : str) {
    switch (c) {
      case '\\':
        oss << "\\\\";
        break;
      case '"':
        oss << "\\\"";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << c;
    }
  }
  return oss.str();
}

std::string ViolationFormatter::severityToString(SeverityLevel severity) {
  switch (severity) {
    case SeverityLevel::Violation:
      return "Violation";
    case SeverityLevel::Warning:
      return "Warning";
    case SeverityLevel::Info:
      return "Info";
  }
  return "Unknown";
}

std::string ViolationFormatter::severityToIRI(SeverityLevel severity) {
  switch (severity) {
    case SeverityLevel::Violation:
      return "http://www.w3.org/ns/shacl#Violation";
    case SeverityLevel::Warning:
      return "http://www.w3.org/ns/shacl#Warning";
    case SeverityLevel::Info:
      return "http://www.w3.org/ns/shacl#Info";
  }
  return "http://www.w3.org/ns/shacl#Violation";
}

std::string ViolationFormatter::generateBlankNodeId(size_t index) {
  return "_:violation" + std::to_string(index);
}

// Single violation formatting
std::string ViolationFormatter::formatText(const ShaclViolation& violation) {
  std::ostringstream oss;

  oss << "[" << severityToString(violation.severity) << "] ";
  oss << violation.focusNode;

  if (!violation.resultPath.empty()) {
    oss << " / " << violation.resultPath;
  }

  oss << "\n  Constraint: " << violation.sourceConstraintComponent;
  oss << "\n  Shape: " << violation.sourceShape;

  if (!violation.message.empty()) {
    oss << "\n  Message: " << violation.message;
  } else {
    oss << "\n  Message: " << violation.getOrGenerateMessage();
  }

  if (violation.expectedValue) {
    oss << "\n  Expected: " << *violation.expectedValue;
  }

  if (!violation.values.empty()) {
    oss << "\n  Actual Value(s): ";
    for (size_t i = 0; i < violation.values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << violation.values[i];
    }
  }

  if (!violation.details.empty()) {
    oss << "\n  Details:";
    for (const auto& [key, value] : violation.details) {
      oss << "\n    " << key << ": " << value;
    }
  }

  return oss.str();
}

std::string ViolationFormatter::formatJSON(const ShaclViolation& violation) {
  std::ostringstream oss;

  oss << "{\n";
  oss << "  \"focusNode\": \"" << escapeJSON(violation.focusNode) << "\",\n";

  if (!violation.resultPath.empty()) {
    oss << "  \"resultPath\": \"" << escapeJSON(violation.resultPath)
        << "\",\n";
  }

  oss << "  \"sourceShape\": \"" << escapeJSON(violation.sourceShape)
      << "\",\n";
  oss << "  \"sourceConstraintComponent\": \""
      << escapeJSON(violation.sourceConstraintComponent) << "\",\n";
  oss << "  \"severity\": \"" << severityToString(violation.severity)
      << "\",\n";

  if (!violation.message.empty()) {
    oss << "  \"message\": \"" << escapeJSON(violation.message) << "\",\n";
  } else {
    oss << "  \"message\": \"" << escapeJSON(violation.getOrGenerateMessage())
        << "\",\n";
  }

  if (violation.expectedValue) {
    oss << "  \"expectedValue\": \"" << escapeJSON(*violation.expectedValue)
        << "\",\n";
  }

  if (!violation.values.empty()) {
    oss << "  \"values\": [";
    for (size_t i = 0; i < violation.values.size(); ++i) {
      if (i > 0) oss << ", ";
      oss << "\"" << escapeJSON(violation.values[i]) << "\"";
    }
    oss << "],\n";
  }

  if (!violation.details.empty()) {
    oss << "  \"details\": {\n";
    size_t count = 0;
    for (const auto& [key, value] : violation.details) {
      if (count > 0) oss << ",\n";
      oss << "    \"" << escapeJSON(key) << "\": \"" << escapeJSON(value)
          << "\"";
      count++;
    }
    oss << "\n  }\n";
  } else {
    // Remove trailing comma from last field
    std::string result = oss.str();
    size_t lastComma = result.rfind(",\n");
    if (lastComma != std::string::npos) {
      result = result.substr(0, lastComma) + "\n";
    }
    oss.str("");
    oss << result;
  }

  oss << "}";
  return oss.str();
}

std::string ViolationFormatter::formatTurtle(const ShaclViolation& violation) {
  static size_t violationCounter = 0;
  std::string blankNodeId = generateBlankNodeId(violationCounter++);

  std::ostringstream oss;

  oss << blankNodeId << " a sh:ValidationResult ;\n";
  oss << "  sh:focusNode <" << violation.focusNode << "> ;\n";

  if (!violation.resultPath.empty()) {
    oss << "  sh:resultPath <" << violation.resultPath << "> ;\n";
  }

  oss << "  sh:sourceShape <" << violation.sourceShape << "> ;\n";
  oss << "  sh:sourceConstraintComponent <"
      << violation.sourceConstraintComponent << "> ;\n";
  oss << "  sh:resultSeverity <" << severityToIRI(violation.severity)
      << "> ;\n";

  std::string message =
      !violation.message.empty() ? violation.message
                                 : violation.getOrGenerateMessage();
  oss << "  sh:resultMessage \"" << escapeTurtle(message) << "\" ;\n";

  if (!violation.values.empty()) {
    for (const auto& value : violation.values) {
      oss << "  sh:value \"" << escapeTurtle(value) << "\" ;\n";
    }
  }

  // Remove trailing semicolon and newline, add period
  std::string result = oss.str();
  if (result.length() >= 3 && result.substr(result.length() - 2) == ";\n") {
    result = result.substr(0, result.length() - 2) + " .\n";
  }

  return result;
}

std::string ViolationFormatter::format(const ShaclViolation& violation,
                                       ViolationFormat format) {
  switch (format) {
    case ViolationFormat::Text:
    case ViolationFormat::Summary:
      return formatText(violation);
    case ViolationFormat::JSON:
      return formatJSON(violation);
    case ViolationFormat::Turtle:
      return formatTurtle(violation);
  }
  return formatText(violation);
}

// Full report formatting
std::string ViolationFormatter::formatTextReport(
    const DetailedValidationReport& report) {
  std::ostringstream oss;

  oss << "SHACL Validation Report\n";
  oss << "======================\n\n";

  oss << "Conforms: " << (report.conforms ? "Yes" : "No") << "\n";
  oss << "Total Issues: " << report.getTotalIssues() << "\n";
  oss << "  Violations: " << report.totalViolations << "\n";
  oss << "  Warnings: " << report.totalWarnings << "\n";
  oss << "  Info: " << report.totalInfo << "\n";
  oss << "Affected Resources: " << report.violationsByFocusNode.size()
      << "\n\n";

  if (!report.violations.empty()) {
    oss << "Violations:\n";
    oss << "-----------\n\n";

    for (size_t i = 0; i < report.violations.size(); ++i) {
      oss << (i + 1) << ". " << formatText(report.violations[i]) << "\n\n";
    }
  }

  return oss.str();
}

std::string ViolationFormatter::formatJSONReport(
    const DetailedValidationReport& report) {
  std::ostringstream oss;

  oss << "{\n";
  oss << "  \"conforms\": " << (report.conforms ? "true" : "false") << ",\n";
  oss << "  \"totalIssues\": " << report.getTotalIssues() << ",\n";
  oss << "  \"statistics\": {\n";
  oss << "    \"violations\": " << report.totalViolations << ",\n";
  oss << "    \"warnings\": " << report.totalWarnings << ",\n";
  oss << "    \"infos\": " << report.totalInfo << ",\n";
  oss << "    \"affectedResources\": " << report.violationsByFocusNode.size()
      << "\n";
  oss << "  },\n";

  oss << "  \"results\": [\n";
  for (size_t i = 0; i < report.violations.size(); ++i) {
    if (i > 0) oss << ",\n";
    oss << "    " << formatJSON(report.violations[i]);
  }
  oss << "\n  ]\n";
  oss << "}\n";

  return oss.str();
}

std::string ViolationFormatter::formatTurtleReport(
    const DetailedValidationReport& report) {
  std::ostringstream oss;

  // Prefixes
  oss << "@prefix sh: <http://www.w3.org/ns/shacl#> .\n";
  oss << "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n";
  oss << "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n\n";

  // Validation report
  oss << "_:report a sh:ValidationReport ;\n";
  oss << "  sh:conforms " << (report.conforms ? "true" : "false") << " ;\n";

  if (!report.violations.empty()) {
    for (size_t i = 0; i < report.violations.size(); ++i) {
      oss << "  sh:result " << generateBlankNodeId(i);
      if (i < report.violations.size() - 1) {
        oss << " ;";
      } else {
        oss << " .";
      }
      oss << "\n";
    }
    oss << "\n";

    // Individual violations
    for (size_t i = 0; i < report.violations.size(); ++i) {
      oss << formatTurtle(report.violations[i]) << "\n";
    }
  } else {
    oss << "  .\n";
  }

  return oss.str();
}

std::string ViolationFormatter::format(const DetailedValidationReport& report,
                                       ViolationFormat format) {
  switch (format) {
    case ViolationFormat::Text:
      return formatTextReport(report);
    case ViolationFormat::JSON:
      return formatJSONReport(report);
    case ViolationFormat::Turtle:
      return formatTurtleReport(report);
    case ViolationFormat::Summary:
      return formatSummary(report);
  }
  return formatTextReport(report);
}

void ViolationFormatter::format(const DetailedValidationReport& report,
                                std::ostream& out, ViolationFormat format) {
  out << ViolationFormatter::format(report, format);
}

std::string ViolationFormatter::formatSummary(
    const DetailedValidationReport& report) {
  auto summary = report.getSummary();
  std::ostringstream oss;

  oss << "SHACL Validation Summary\n";
  oss << "========================\n\n";

  oss << "Resources validated: " << summary.totalResources << "\n";
  oss << "  Conforming: " << summary.conformingResources << "\n";
  oss << "  Non-conforming: " << summary.nonConformingResources << "\n\n";

  oss << "Issues found: " << (summary.violations + summary.warnings + summary.infos) << "\n";
  oss << "  Violations: " << summary.violations << "\n";
  oss << "  Warnings: " << summary.warnings << "\n";
  oss << "  Infos: " << summary.infos << "\n\n";

  if (!summary.violationsByShape.empty()) {
    oss << "Violations by Shape:\n";
    for (const auto& [shape, count] : summary.violationsByShape) {
      oss << "  " << shape << ": " << count << "\n";
    }
    oss << "\n";
  }

  if (!summary.violationsByConstraint.empty()) {
    oss << "Violations by Constraint:\n";
    for (const auto& [constraint, count] : summary.violationsByConstraint) {
      oss << "  " << constraint << ": " << count << "\n";
    }
  }

  return oss.str();
}

std::string ViolationFormatter::formatByFocusNode(
    const DetailedValidationReport& report, ViolationFormat format) {
  std::ostringstream oss;

  oss << "Violations Grouped by Focus Node\n";
  oss << "=================================\n\n";

  for (const auto& [focusNode, violations] : report.violationsByFocusNode) {
    oss << "Focus Node: " << focusNode << " (" << violations.size()
        << " violations)\n";
    oss << "-------------------------------------------\n";

    for (const auto& violation : violations) {
      oss << ViolationFormatter::format(violation, format) << "\n\n";
    }
  }

  return oss.str();
}

std::string ViolationFormatter::formatByShape(
    const DetailedValidationReport& report, ViolationFormat format) {
  std::ostringstream oss;

  oss << "Violations Grouped by Shape\n";
  oss << "============================\n\n";

  for (const auto& [shape, violations] : report.violationsByShape) {
    oss << "Shape: " << shape << " (" << violations.size()
        << " violations)\n";
    oss << "-------------------------------------------\n";

    for (const auto& violation : violations) {
      oss << ViolationFormatter::format(violation, format) << "\n\n";
    }
  }

  return oss.str();
}

std::string ViolationFormatter::formatByConstraint(
    const DetailedValidationReport& report, ViolationFormat format) {
  std::ostringstream oss;

  oss << "Violations Grouped by Constraint\n";
  oss << "=================================\n\n";

  for (const auto& [constraint, violations] : report.violationsByConstraint) {
    oss << "Constraint: " << constraint << " (" << violations.size()
        << " violations)\n";
    oss << "-------------------------------------------\n";

    for (const auto& violation : violations) {
      oss << ViolationFormatter::format(violation, format) << "\n\n";
    }
  }

  return oss.str();
}

// ViolationPrinter implementations
void ViolationPrinter::printIndent(int level) {
  for (int i = 0; i < level; ++i) {
    out_ << "  ";
  }
}

void ViolationPrinter::printSeparator(char c, int length) {
  for (int i = 0; i < length; ++i) {
    out_ << c;
  }
  out_ << "\n";
}

std::string ViolationPrinter::colorize(const std::string& text,
                                       const std::string& color) {
  if (!useColor_) return text;

  static const std::unordered_map<std::string, std::string> colors = {
      {"red", "\033[31m"},   {"green", "\033[32m"}, {"yellow", "\033[33m"},
      {"blue", "\033[34m"},  {"reset", "\033[0m"},  {"bold", "\033[1m"}};

  auto it = colors.find(color);
  if (it != colors.end()) {
    return it->second + text + colors.at("reset");
  }
  return text;
}

void ViolationPrinter::printViolation(const ShaclViolation& violation,
                                      int indent) {
  printIndent(indent);

  std::string prefix;
  if (violation.isError()) {
    prefix = colorize("[ERROR] ", "red");
  } else if (violation.isWarning()) {
    prefix = colorize("[WARNING] ", "yellow");
  } else {
    prefix = colorize("[INFO] ", "blue");
  }

  out_ << prefix << violation.focusNode;
  if (!violation.resultPath.empty()) {
    out_ << " / " << violation.resultPath;
  }
  out_ << "\n";

  if (showDetails_) {
    printIndent(indent + 1);
    out_ << "Constraint: " << violation.sourceConstraintComponent << "\n";

    if (!violation.message.empty()) {
      printIndent(indent + 1);
      out_ << "Message: " << violation.message << "\n";
    }
  }
}

void ViolationPrinter::printSummary(
    const DetailedValidationReport::Summary& summary) {
  out_ << colorize("SHACL Validation Summary", "bold") << "\n";
  printSeparator('=');

  out_ << "Resources: " << summary.totalResources << " ("
       << colorize(std::to_string(summary.conformingResources), "green")
       << " conforming, "
       << colorize(std::to_string(summary.nonConformingResources), "red")
       << " non-conforming)\n";

  out_ << "Issues: " << (summary.violations + summary.warnings + summary.infos)
       << " (";
  out_ << colorize(std::to_string(summary.violations), "red") << " violations, ";
  out_ << colorize(std::to_string(summary.warnings), "yellow") << " warnings, ";
  out_ << colorize(std::to_string(summary.infos), "blue") << " infos)\n";
}

void ViolationPrinter::printReport(const DetailedValidationReport& report) {
  printSummary(report.getSummary());
  out_ << "\n";

  if (!report.violations.empty()) {
    out_ << colorize("Violations:", "bold") << "\n";
    printSeparator('-');

    size_t count = 0;
    for (const auto& violation : report.violations) {
      if (count >= maxPerGroup_) {
        out_ << "... and " << (report.violations.size() - count)
             << " more violations\n";
        break;
      }
      printViolation(violation);
      count++;
    }
  }
}

void ViolationPrinter::printByFocusNode(
    const DetailedValidationReport& report) {
  out_ << colorize("Violations by Focus Node:", "bold") << "\n";
  printSeparator('=');

  for (const auto& [node, violations] : report.violationsByFocusNode) {
    out_ << "\n" << colorize(node, "bold") << " (" << violations.size()
         << " violations)\n";

    size_t count = 0;
    for (const auto& violation : violations) {
      if (count >= maxPerGroup_) break;
      printViolation(violation, 1);
      count++;
    }
  }
}

void ViolationPrinter::printByShape(const DetailedValidationReport& report) {
  out_ << colorize("Violations by Shape:", "bold") << "\n";
  printSeparator('=');

  for (const auto& [shape, violations] : report.violationsByShape) {
    out_ << "\n" << colorize(shape, "bold") << " (" << violations.size()
         << " violations)\n";

    size_t count = 0;
    for (const auto& violation : violations) {
      if (count >= maxPerGroup_) break;
      printViolation(violation, 1);
      count++;
    }
  }
}

void ViolationPrinter::printByConstraint(
    const DetailedValidationReport& report) {
  out_ << colorize("Violations by Constraint:", "bold") << "\n";
  printSeparator('=');

  for (const auto& [constraint, violations] : report.violationsByConstraint) {
    out_ << "\n" << colorize(constraint, "bold") << " (" << violations.size()
         << " violations)\n";

    size_t count = 0;
    for (const auto& violation : violations) {
      if (count >= maxPerGroup_) break;
      printViolation(violation, 1);
      count++;
    }
  }
}

}  // namespace shacl

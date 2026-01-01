#include "ShExErrorReporting.h"
#include <iostream>

namespace shex {

// ============================================================================
// Example Error Scenarios
// ============================================================================

class ErrorReportingExamples {
 public:
  // Scenario 1: Cardinality Violation
  static DetailedValidationError cardinalityViolationExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::ERROR)
        .setErrorType(ErrorType::CARDINALITY_VIOLATION)
        .setMessage("Property occurs 3 times but shape requires exactly 1 occurrence")
        .setShapeId("ex:PersonShape")
        .setPropertyId("ex:email")
        .setNodeId("ex:person1")
        .setExpectedCount(1)
        .setActualCount(3)
        .setTripleContext(TripleContext(
            "ex:person1",
            "ex:email",
            "alice@example.org",
            "LITERAL"
        ))
        .setSuggestion("Remove 2 occurrences of ex:email property, keeping only one")
        .build();
  }

  // Scenario 2: Type Mismatch
  static DetailedValidationError typeMismatchExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::ERROR)
        .setErrorType(ErrorType::TYPE_MISMATCH)
        .setMessage("Property value type is IRI but shape requires LITERAL")
        .setShapeId("ex:PersonShape")
        .setPropertyId("ex:name")
        .setNodeId("ex:person2")
        .setExpectedValue("LITERAL")
        .setActualValue("IRI")
        .setTripleContext(TripleContext(
            "ex:person2",
            "ex:name",
            "http://example.org/names/John",
            "IRI"
        ))
        .setSuggestion("Change the value to a literal string (e.g., \"John\") instead of an IRI")
        .build();
  }

  // Scenario 3: Value Not in Allowed Set
  static DetailedValidationError valueNotAllowedExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::ERROR)
        .setErrorType(ErrorType::VALUE_NOT_ALLOWED)
        .setMessage("Property value is not in the allowed set of IRIs")
        .setShapeId("ex:EmployeeShape")
        .setPropertyId("ex:department")
        .setNodeId("ex:employee42")
        .setExpectedValue("One of: [ex:Sales, ex:Engineering, ex:Marketing]")
        .setActualValue("ex:HumanResources")
        .setTripleContext(TripleContext(
            "ex:employee42",
            "ex:department",
            "http://example.org/HumanResources",
            "IRI"
        ))
        .setSuggestion("Use one of the allowed department IRIs: ex:Sales, ex:Engineering, or ex:Marketing")
        .build();
  }

  // Scenario 4: Missing Required Property
  static DetailedValidationError missingRequiredPropertyExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::ERROR)
        .setErrorType(ErrorType::MISSING_REQUIRED_PROPERTY)
        .setMessage("Required property is missing (cardinality requires at least 1 occurrence)")
        .setShapeId("ex:PersonShape")
        .setPropertyId("ex:name")
        .setNodeId("ex:person3")
        .setExpectedCount(1)
        .setActualCount(0)
        .setSuggestion("Add a triple with ex:name property: ex:person3 ex:name \"Name\"^^xsd:string")
        .build();
  }

  // Scenario 5: Parser Syntax Error
  static DetailedValidationError parserSyntaxErrorExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::ERROR)
        .setErrorType(ErrorType::PARSER_SYNTAX_ERROR)
        .setMessage("Expected '}' to close shape definition but found ';'")
        .setShapeId("ex:PersonShape")
        .setLocation(SourceLocation(15, 42, 15, 43))
        .setSuggestion("Add a closing brace '}' at the end of the shape definition")
        .build();
  }

  // Scenario 6: Datatype Mismatch with Warning
  static DetailedValidationError datatypeMismatchWarningExample() {
    return ErrorBuilder()
        .setSeverity(ErrorSeverity::WARNING)
        .setErrorType(ErrorType::DATATYPE_MISMATCH)
        .setMessage("Literal value has datatype xsd:string but xsd:integer was expected")
        .setShapeId("ex:PersonShape")
        .setPropertyId("ex:age")
        .setNodeId("ex:person4")
        .setExpectedValue("xsd:integer")
        .setActualValue("xsd:string")
        .setTripleContext(TripleContext(
            "ex:person4",
            "ex:age",
            "\"thirty\"^^xsd:string",
            "LITERAL"
        ))
        .setSuggestion("Change the value to a numeric literal: \"30\"^^xsd:integer")
        .build();
  }

  // Generate complete report with all scenarios
  static EnhancedValidationReport generateCompleteReport() {
    EnhancedValidationReport report;

    // Add all example errors
    auto error1 = cardinalityViolationExample();
    auto error2 = typeMismatchExample();
    auto error3 = valueNotAllowedExample();
    auto error4 = missingRequiredPropertyExample();
    auto error5 = parserSyntaxErrorExample();
    auto error6 = datatypeMismatchWarningExample();

    report.addError(error1);
    report.addError(error2);
    report.addError(error3);
    report.addError(error4);
    report.addError(error5);
    report.addError(error6);

    // Update conformance map
    report.conformanceMap.addEntry("ex:person1", "ex:PersonShape",
                                   ConformanceStatus::DOES_NOT_CONFORM);
    report.conformanceMap.addError("ex:person1", "ex:PersonShape", error1);

    report.conformanceMap.addEntry("ex:person2", "ex:PersonShape",
                                   ConformanceStatus::DOES_NOT_CONFORM);
    report.conformanceMap.addError("ex:person2", "ex:PersonShape", error2);

    report.conformanceMap.addEntry("ex:employee42", "ex:EmployeeShape",
                                   ConformanceStatus::DOES_NOT_CONFORM);
    report.conformanceMap.addError("ex:employee42", "ex:EmployeeShape", error3);
    report.conformanceMap.addFailedConstraint("ex:employee42", "ex:EmployeeShape",
                                              "department value constraint");

    report.conformanceMap.addEntry("ex:person3", "ex:PersonShape",
                                   ConformanceStatus::DOES_NOT_CONFORM);
    report.conformanceMap.addError("ex:person3", "ex:PersonShape", error4);

    report.conformanceMap.addEntry("ex:person4", "ex:PersonShape",
                                   ConformanceStatus::CONFORMS);
    report.conformanceMap.addError("ex:person4", "ex:PersonShape", error6);

    report.computeStatistics();

    return report;
  }

  // Generate JSON output with all scenarios
  static nlohmann::json generateJsonOutput() {
    auto report = generateCompleteReport();

    nlohmann::json output = {
        {"detailed_error", report.errors[0].toJson()},
        {"conformance_map", report.conformanceMap.toJson()},
        {"error_messages", nlohmann::json::array()}
    };

    // Add all error messages
    for (const auto& error : report.errors) {
      output["error_messages"].push_back({
          {"severity", severityToString(error.severity)},
          {"type", errorTypeToString(error.errorType)},
          {"message", error.message},
          {"suggestion", error.suggestion.value_or("")},
          {"human_readable", error.toHumanReadable()}
      });
    }

    // Add complete report
    output["complete_report"] = report.toJson();

    return output;
  }

  // Print all examples
  static void printAllExamples() {
    auto report = generateCompleteReport();

    std::cout << "=================================================\n";
    std::cout << "W3C ShEx Comprehensive Error Reporting Examples\n";
    std::cout << "=================================================\n\n";

    // Print human-readable format
    std::cout << report.toHumanReadable() << "\n";

    std::cout << "\n=================================================\n";
    std::cout << "JSON Format\n";
    std::cout << "=================================================\n";
    std::cout << report.toJson().dump(2) << "\n";

    std::cout << "\n=================================================\n";
    std::cout << "XML Format\n";
    std::cout << "=================================================\n";
    std::cout << report.toXml() << "\n";

    std::cout << "\n=================================================\n";
    std::cout << "Complete JSON Output (as requested)\n";
    std::cout << "=================================================\n";
    std::cout << generateJsonOutput().dump(2) << "\n";
  }

  // Individual scenario demonstrations
  static void demonstrateScenario1() {
    std::cout << "=== Scenario 1: Cardinality Violation ===\n";
    auto error = cardinalityViolationExample();
    std::cout << error.toHumanReadable() << "\n";
  }

  static void demonstrateScenario2() {
    std::cout << "=== Scenario 2: Type Mismatch ===\n";
    auto error = typeMismatchExample();
    std::cout << error.toHumanReadable() << "\n";
  }

  static void demonstrateScenario3() {
    std::cout << "=== Scenario 3: Value Not in Allowed Set ===\n";
    auto error = valueNotAllowedExample();
    std::cout << error.toHumanReadable() << "\n";
  }

  static void demonstrateScenario4() {
    std::cout << "=== Scenario 4: Missing Required Property ===\n";
    auto error = missingRequiredPropertyExample();
    std::cout << error.toHumanReadable() << "\n";
  }

  static void demonstrateScenario5() {
    std::cout << "=== Scenario 5: Parser Syntax Error ===\n";
    auto error = parserSyntaxErrorExample();
    std::cout << error.toHumanReadable() << "\n";
  }

  static void demonstrateScenario6() {
    std::cout << "=== Scenario 6: Datatype Mismatch Warning ===\n";
    auto error = datatypeMismatchWarningExample();
    std::cout << error.toHumanReadable() << "\n";
  }
};

}  // namespace shex

// ============================================================================
// Main function for demonstration
// ============================================================================

int main() {
  shex::ErrorReportingExamples::printAllExamples();
  return 0;
}

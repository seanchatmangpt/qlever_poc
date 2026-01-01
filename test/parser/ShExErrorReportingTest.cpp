#include <gtest/gtest.h>

#include "parser/ShExErrorReporting.h"

using namespace shex;

// ============================================================================
// ErrorBuilder Tests
// ============================================================================

TEST(ErrorBuilderTest, BuildBasicError) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error message")
      .setShapeId("TestShape")
      .build();

  EXPECT_EQ(error.severity, ErrorSeverity::ERROR);
  EXPECT_EQ(error.errorType, ErrorType::TYPE_MISMATCH);
  EXPECT_EQ(error.message, "Test error message");
  EXPECT_EQ(error.shapeId, "TestShape");
}

TEST(ErrorBuilderTest, BuildErrorWithTripleContext) {
  TripleContext context("ex:subject", "ex:predicate", "ex:object", "IRI");

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::VALUE_NOT_ALLOWED)
      .setMessage("Value not allowed")
      .setShapeId("TestShape")
      .setTripleContext(context)
      .build();

  ASSERT_TRUE(error.tripleContext.has_value());
  EXPECT_EQ(error.tripleContext->subject, "ex:subject");
  EXPECT_EQ(error.tripleContext->predicate, "ex:predicate");
  EXPECT_EQ(error.tripleContext->object, "ex:object");
  EXPECT_EQ(error.tripleContext->objectType, "IRI");
}

TEST(ErrorBuilderTest, BuildErrorWithLocation) {
  SourceLocation loc(10, 5, 10, 15);

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::PARSER_SYNTAX_ERROR)
      .setMessage("Syntax error")
      .setShapeId("TestShape")
      .setLocation(loc)
      .build();

  ASSERT_TRUE(error.location.has_value());
  EXPECT_EQ(error.location->line, 10);
  EXPECT_EQ(error.location->column, 5);
  EXPECT_EQ(error.location->endLine.value(), 10);
  EXPECT_EQ(error.location->endColumn.value(), 15);
}

TEST(ErrorBuilderTest, BuildErrorWithExpectedActual) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Wrong cardinality")
      .setShapeId("TestShape")
      .setExpectedCount(1)
      .setActualCount(3)
      .build();

  ASSERT_TRUE(error.expectedCount.has_value());
  ASSERT_TRUE(error.actualCount.has_value());
  EXPECT_EQ(error.expectedCount.value(), 1);
  EXPECT_EQ(error.actualCount.value(), 3);
}

TEST(ErrorBuilderTest, BuildErrorWithSuggestion) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Datatype mismatch")
      .setShapeId("TestShape")
      .setSuggestion("Use xsd:integer instead")
      .build();

  ASSERT_TRUE(error.suggestion.has_value());
  EXPECT_EQ(error.suggestion.value(), "Use xsd:integer instead");
}

// ============================================================================
// TripleContext Tests
// ============================================================================

TEST(TripleContextTest, ToStringIRI) {
  TripleContext context("ex:subject", "ex:predicate", "ex:object", "IRI");
  std::string str = context.toString();

  EXPECT_NE(str.find("ex:subject"), std::string::npos);
  EXPECT_NE(str.find("ex:predicate"), std::string::npos);
  EXPECT_NE(str.find("ex:object"), std::string::npos);
}

TEST(TripleContextTest, ToStringLiteral) {
  TripleContext context("ex:subject", "ex:predicate", "value", "LITERAL");
  std::string str = context.toString();

  EXPECT_NE(str.find("\"value\""), std::string::npos);
}

TEST(TripleContextTest, ToJson) {
  TripleContext context("ex:subject", "ex:predicate", "ex:object", "IRI");
  auto json = context.toJson();

  EXPECT_EQ(json["subject"], "ex:subject");
  EXPECT_EQ(json["predicate"], "ex:predicate");
  EXPECT_EQ(json["object"], "ex:object");
  EXPECT_EQ(json["objectType"], "IRI");
}

// ============================================================================
// SourceLocation Tests
// ============================================================================

TEST(SourceLocationTest, BasicLocation) {
  SourceLocation loc(10, 5);

  EXPECT_EQ(loc.line, 10);
  EXPECT_EQ(loc.column, 5);
  EXPECT_FALSE(loc.endLine.has_value());
  EXPECT_FALSE(loc.endColumn.has_value());
}

TEST(SourceLocationTest, RangeLocation) {
  SourceLocation loc(10, 5, 15, 20);

  EXPECT_EQ(loc.line, 10);
  EXPECT_EQ(loc.column, 5);
  EXPECT_EQ(loc.endLine.value(), 15);
  EXPECT_EQ(loc.endColumn.value(), 20);
}

TEST(SourceLocationTest, ToString) {
  SourceLocation loc(10, 5, 10, 15);
  std::string str = loc.toString();

  EXPECT_NE(str.find("line 10"), std::string::npos);
  EXPECT_NE(str.find("column 5"), std::string::npos);
}

TEST(SourceLocationTest, ToJson) {
  SourceLocation loc(10, 5, 10, 15);
  auto json = loc.toJson();

  EXPECT_EQ(json["line"], 10);
  EXPECT_EQ(json["column"], 5);
  EXPECT_EQ(json["endLine"], 10);
  EXPECT_EQ(json["endColumn"], 15);
}

// ============================================================================
// DetailedValidationError Tests
// ============================================================================

TEST(DetailedValidationErrorTest, ToHumanReadable) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .setNodeId("ex:node1")
      .setPropertyId("ex:property1")
      .build();

  std::string output = error.toHumanReadable();

  EXPECT_NE(output.find("[ERROR]"), std::string::npos);
  EXPECT_NE(output.find("CARDINALITY_VIOLATION"), std::string::npos);
  EXPECT_NE(output.find("Test error"), std::string::npos);
  EXPECT_NE(output.find("TestShape"), std::string::npos);
  EXPECT_NE(output.find("ex:node1"), std::string::npos);
}

TEST(DetailedValidationErrorTest, ToJson) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Type mismatch")
      .setShapeId("TestShape")
      .build();

  auto json = error.toJson();

  EXPECT_EQ(json["severity"], "ERROR");
  EXPECT_EQ(json["errorType"], "TYPE_MISMATCH");
  EXPECT_EQ(json["message"], "Type mismatch");
  EXPECT_EQ(json["shapeId"], "TestShape");
}

TEST(DetailedValidationErrorTest, ToXml) {
  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Warning message")
      .setShapeId("TestShape")
      .build();

  std::string xml = error.toXml();

  EXPECT_NE(xml.find("<severity>WARNING</severity>"), std::string::npos);
  EXPECT_NE(xml.find("<errorType>DATATYPE_MISMATCH</errorType>"), std::string::npos);
  EXPECT_NE(xml.find("<message>Warning message</message>"), std::string::npos);
}

// ============================================================================
// ShapeConformanceMap Tests
// ============================================================================

TEST(ShapeConformanceMapTest, AddEntry) {
  ShapeConformanceMap map;

  map.addEntry("ex:node1", "ex:Shape1", ConformanceStatus::CONFORMS);
  map.addEntry("ex:node1", "ex:Shape2", ConformanceStatus::DOES_NOT_CONFORM);

  EXPECT_TRUE(map.nodeConformsToShape("ex:node1", "ex:Shape1"));
  EXPECT_FALSE(map.nodeConformsToShape("ex:node1", "ex:Shape2"));
}

TEST(ShapeConformanceMapTest, AddError) {
  ShapeConformanceMap map;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error")
      .setShapeId("ex:Shape1")
      .build();

  map.addError("ex:node1", "ex:Shape1", error);

  EXPECT_FALSE(map.nodeConformsToShape("ex:node1", "ex:Shape1"));
}

TEST(ShapeConformanceMapTest, GetFailedShapes) {
  ShapeConformanceMap map;

  map.addEntry("ex:node1", "ex:Shape1", ConformanceStatus::CONFORMS);
  map.addEntry("ex:node1", "ex:Shape2", ConformanceStatus::DOES_NOT_CONFORM);
  map.addEntry("ex:node1", "ex:Shape3", ConformanceStatus::DOES_NOT_CONFORM);

  auto failedShapes = map.getFailedShapes("ex:node1");

  EXPECT_EQ(failedShapes.size(), 2);
  EXPECT_NE(std::find(failedShapes.begin(), failedShapes.end(), "ex:Shape2"),
            failedShapes.end());
  EXPECT_NE(std::find(failedShapes.begin(), failedShapes.end(), "ex:Shape3"),
            failedShapes.end());
}

TEST(ShapeConformanceMapTest, ToJson) {
  ShapeConformanceMap map;

  map.addEntry("ex:node1", "ex:Shape1", ConformanceStatus::CONFORMS);

  auto json = map.toJson();

  EXPECT_TRUE(json.contains("ex:node1"));
  EXPECT_TRUE(json["ex:node1"].is_array());
}

// ============================================================================
// EnhancedValidationReport Tests
// ============================================================================

TEST(EnhancedValidationReportTest, InitialState) {
  EnhancedValidationReport report;

  EXPECT_TRUE(report.conforms);
  EXPECT_EQ(report.totalErrors, 0);
  EXPECT_EQ(report.totalWarnings, 0);
  EXPECT_EQ(report.totalInfoMessages, 0);
}

TEST(EnhancedValidationReportTest, AddError) {
  EnhancedValidationReport report;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .build();

  report.addError(error);

  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.totalErrors, 1);
  EXPECT_EQ(report.errors.size(), 1);
}

TEST(EnhancedValidationReportTest, AddWarning) {
  EnhancedValidationReport report;

  auto warning = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Test warning")
      .setShapeId("TestShape")
      .build();

  report.addError(warning);

  EXPECT_TRUE(report.conforms);  // Warnings don't affect conformance
  EXPECT_EQ(report.totalWarnings, 1);
}

TEST(EnhancedValidationReportTest, AddMultipleErrors) {
  EnhancedValidationReport report;

  auto error1 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Error 1")
      .setShapeId("TestShape")
      .build();

  auto error2 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Error 2")
      .setShapeId("TestShape")
      .build();

  auto warning = ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Warning 1")
      .setShapeId("TestShape")
      .build();

  report.addError(error1);
  report.addError(error2);
  report.addError(warning);

  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.totalErrors, 2);
  EXPECT_EQ(report.totalWarnings, 1);
  EXPECT_EQ(report.errors.size(), 3);
}

TEST(EnhancedValidationReportTest, ComputeStatistics) {
  EnhancedValidationReport report;

  report.errors.push_back(ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Error")
      .setShapeId("TestShape")
      .build());

  report.errors.push_back(ErrorBuilder()
      .setSeverity(ErrorSeverity::WARNING)
      .setErrorType(ErrorType::DATATYPE_MISMATCH)
      .setMessage("Warning")
      .setShapeId("TestShape")
      .build());

  report.errors.push_back(ErrorBuilder()
      .setSeverity(ErrorSeverity::INFO)
      .setErrorType(ErrorType::CONSTRAINT_VIOLATION)
      .setMessage("Info")
      .setShapeId("TestShape")
      .build());

  report.computeStatistics();

  EXPECT_EQ(report.totalErrors, 1);
  EXPECT_EQ(report.totalWarnings, 1);
  EXPECT_EQ(report.totalInfoMessages, 1);
  EXPECT_FALSE(report.conforms);
}

TEST(EnhancedValidationReportTest, ToHumanReadable) {
  EnhancedValidationReport report;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .build();

  report.addError(error);

  std::string output = report.toHumanReadable();

  EXPECT_NE(output.find("ShEx Validation Report"), std::string::npos);
  EXPECT_NE(output.find("DOES NOT CONFORM"), std::string::npos);
  EXPECT_NE(output.find("Errors: 1"), std::string::npos);
}

TEST(EnhancedValidationReportTest, ToJson) {
  EnhancedValidationReport report;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .build();

  report.addError(error);

  auto json = report.toJson();

  EXPECT_FALSE(json["conforms"].get<bool>());
  EXPECT_EQ(json["statistics"]["totalErrors"], 1);
  EXPECT_TRUE(json["errors"].is_array());
  EXPECT_EQ(json["errors"].size(), 1);
}

TEST(EnhancedValidationReportTest, ToXml) {
  EnhancedValidationReport report;

  auto error = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Test error")
      .setShapeId("TestShape")
      .build();

  report.addError(error);

  std::string xml = report.toXml();

  EXPECT_NE(xml.find("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), std::string::npos);
  EXPECT_NE(xml.find("<validationReport>"), std::string::npos);
  EXPECT_NE(xml.find("<conforms>false</conforms>"), std::string::npos);
  EXPECT_NE(xml.find("<totalErrors>1</totalErrors>"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IntegrationTest, CompleteValidationWorkflow) {
  EnhancedValidationReport report;

  // Cardinality error
  auto error1 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::CARDINALITY_VIOLATION)
      .setMessage("Property occurs 3 times but shape requires exactly 1 occurrence")
      .setShapeId("ex:PersonShape")
      .setNodeId("ex:person1")
      .setPropertyId("ex:email")
      .setExpectedCount(1)
      .setActualCount(3)
      .setTripleContext(TripleContext("ex:person1", "ex:email",
                                      "alice@example.org", "LITERAL"))
      .setSuggestion("Remove 2 occurrences of ex:email property")
      .build();

  // Type mismatch error
  auto error2 = ErrorBuilder()
      .setSeverity(ErrorSeverity::ERROR)
      .setErrorType(ErrorType::TYPE_MISMATCH)
      .setMessage("Property value type is IRI but shape requires LITERAL")
      .setShapeId("ex:PersonShape")
      .setNodeId("ex:person2")
      .setPropertyId("ex:name")
      .setExpectedValue("LITERAL")
      .setActualValue("IRI")
      .setTripleContext(TripleContext("ex:person2", "ex:name",
                                      "http://example.org/names/John", "IRI"))
      .setSuggestion("Change to a literal string")
      .build();

  // Add errors
  report.addError(error1);
  report.addError(error2);

  // Update conformance map
  report.conformanceMap.addEntry("ex:person1", "ex:PersonShape",
                                 ConformanceStatus::DOES_NOT_CONFORM);
  report.conformanceMap.addError("ex:person1", "ex:PersonShape", error1);

  report.conformanceMap.addEntry("ex:person2", "ex:PersonShape",
                                 ConformanceStatus::DOES_NOT_CONFORM);
  report.conformanceMap.addError("ex:person2", "ex:PersonShape", error2);

  // Verify report
  EXPECT_FALSE(report.conforms);
  EXPECT_EQ(report.totalErrors, 2);
  EXPECT_EQ(report.errors.size(), 2);

  // Verify conformance map
  EXPECT_FALSE(report.conformanceMap.nodeConformsToShape("ex:person1", "ex:PersonShape"));
  EXPECT_FALSE(report.conformanceMap.nodeConformsToShape("ex:person2", "ex:PersonShape"));

  auto failedShapes1 = report.conformanceMap.getFailedShapes("ex:person1");
  EXPECT_EQ(failedShapes1.size(), 1);
  EXPECT_EQ(failedShapes1[0], "ex:PersonShape");

  // Test output formats
  std::string humanReadable = report.toHumanReadable();
  EXPECT_FALSE(humanReadable.empty());

  auto json = report.toJson();
  EXPECT_FALSE(json.empty());

  std::string xml = report.toXml();
  EXPECT_FALSE(xml.empty());
}

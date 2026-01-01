// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "engine/contracts/CommonTypes.h"
#include "engine/contracts/ErrorCode.h"
#include "engine/contracts/ResultDigest.h"
#include "engine/contracts/Violation.h"

using namespace qlever::contracts;

// ============================================================================
// ErrorCode Tests
// ============================================================================

TEST(ErrorCodeTest, ErrorCodeDescriptions) {
  // Test generic errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::UNKNOWN_ERROR), "Unknown error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::INTERNAL_ERROR), "Internal error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::NOT_IMPLEMENTED),
            "Feature not implemented");

  // Test parsing errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::PARSE_ERROR), "Parse error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::SYNTAX_ERROR), "Syntax error");

  // Test SHACL errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::SHACL_VALIDATION_ERROR),
            "SHACL validation error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::SHACL_MIN_COUNT_VIOLATION),
            "SHACL minimum count violation");

  // Test ShEx errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::SHEX_VALIDATION_ERROR),
            "ShEx validation error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::SHEX_SHAPE_MISMATCH),
            "ShEx shape mismatch");

  // Test rules errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::RULE_EXECUTION_ERROR),
            "Rule execution error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::RULE_DEPENDENCY_CYCLE),
            "Rule dependency cycle detected");

  // Test N3 errors
  EXPECT_EQ(errorCodeDescription(ErrorCode::N3_REASONING_ERROR),
            "N3 reasoning error");
  EXPECT_EQ(errorCodeDescription(ErrorCode::N3_CONTRADICTION),
            "N3 contradiction detected");

  // Test guard violations
  EXPECT_EQ(errorCodeDescription(ErrorCode::GUARD_VIOLATION),
            "Guard condition violation");
  EXPECT_EQ(errorCodeDescription(ErrorCode::GUARD_MEMORY_LIMIT_EXCEEDED),
            "Guard memory limit exceeded");
}

// ============================================================================
// CommonTypes Tests
// ============================================================================

TEST(CommonTypesTest, OutcomeEnum) {
  EXPECT_EQ(outcomeToString(Outcome::OK), "OK");
  EXPECT_EQ(outcomeToString(Outcome::GUARDED), "GUARDED");
  EXPECT_EQ(outcomeToString(Outcome::ERROR), "ERROR");
}

TEST(CommonTypesTest, SeverityEnum) {
  EXPECT_EQ(severityToString(Severity::INFO), "INFO");
  EXPECT_EQ(severityToString(Severity::WARNING), "WARNING");
  EXPECT_EQ(severityToString(Severity::ERROR), "ERROR");
  EXPECT_EQ(severityToString(Severity::CRITICAL), "CRITICAL");
}

TEST(CommonTypesTest, BoundedStringBasic) {
  ShortMessage msg("Hello, World!");
  EXPECT_EQ(msg.value(), "Hello, World!");
  EXPECT_EQ(msg.size(), 13);
  EXPECT_FALSE(msg.empty());
}

TEST(CommonTypesTest, BoundedStringTruncation) {
  // Create a string longer than max size (256 for ShortMessage)
  std::string longString(300, 'a');
  ShortMessage msg(longString);

  // Should be truncated to 256 characters (253 + "...")
  EXPECT_EQ(msg.size(), 256);
  EXPECT_TRUE(msg.value().ends_with("..."));
}

TEST(CommonTypesTest, BoundedStringEmpty) {
  MediumMessage msg("");
  EXPECT_TRUE(msg.empty());
  EXPECT_EQ(msg.size(), 0);
}

TEST(CommonTypesTest, TimestampGeneration) {
  uint64_t ts1 = currentTimestampMs();
  // Sleep briefly to ensure time progresses
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  uint64_t ts2 = currentTimestampMs();

  EXPECT_GT(ts2, ts1);
}

// ============================================================================
// Violation Tests
// ============================================================================

TEST(ViolationTest, BasicConstruction) {
  Violation v(ErrorCode::SHACL_MIN_COUNT_VIOLATION, Severity::ERROR,
              "Minimum count constraint violated");

  EXPECT_EQ(v.violationType, ErrorCode::SHACL_MIN_COUNT_VIOLATION);
  EXPECT_EQ(v.severity, Severity::ERROR);
  EXPECT_EQ(v.message, "Minimum count constraint violated");
  EXPECT_TRUE(v.affectedNodes.empty());
  EXPECT_FALSE(v.shapeReference.has_value());
  EXPECT_GT(v.timestampMs, 0);
}

TEST(ViolationTest, FullConstruction) {
  std::vector<Id> nodes = {Id::makeFromInt(1), Id::makeFromInt(2)};
  Violation v(ErrorCode::SHACL_CLASS_CONSTRAINT_VIOLATION, Severity::WARNING,
              "Class constraint violated", nodes, "ex:PersonShape");

  EXPECT_EQ(v.affectedNodes.size(), 2);
  EXPECT_EQ(v.affectedNodes[0].getBits(), 1);
  EXPECT_EQ(v.affectedNodes[1].getBits(), 2);
  EXPECT_TRUE(v.shapeReference.has_value());
  EXPECT_EQ(v.shapeReference.value(), "ex:PersonShape");
}

TEST(ViolationTest, GetViolationTypeString) {
  Violation v(ErrorCode::SHEX_CARDINALITY_VIOLATION, Severity::ERROR,
              "Cardinality violated");
  EXPECT_EQ(v.getViolationTypeString(), "ShEx cardinality violation");
}

TEST(ViolationTest, GetSeverityString) {
  Violation v(ErrorCode::RULE_UNBOUND_VARIABLE, Severity::CRITICAL,
              "Unbound variable");
  EXPECT_EQ(v.getSeverityString(), "CRITICAL");
}

TEST(ViolationTest, IsCritical) {
  Violation v1(ErrorCode::INTERNAL_ERROR, Severity::CRITICAL, "Critical error");
  Violation v2(ErrorCode::INTERNAL_ERROR, Severity::ERROR, "Normal error");

  EXPECT_TRUE(v1.isCritical());
  EXPECT_FALSE(v2.isCritical());
}

TEST(ViolationTest, IsError) {
  Violation v1(ErrorCode::INTERNAL_ERROR, Severity::CRITICAL, "Critical error");
  Violation v2(ErrorCode::INTERNAL_ERROR, Severity::ERROR, "Normal error");
  Violation v3(ErrorCode::INTERNAL_ERROR, Severity::WARNING, "Warning");

  EXPECT_TRUE(v1.isError());
  EXPECT_TRUE(v2.isError());
  EXPECT_FALSE(v3.isError());
}

TEST(ViolationTest, JsonSerialization) {
  std::vector<Id> nodes = {Id::makeFromInt(42)};
  Violation original(ErrorCode::GUARD_PRECONDITION_FAILED, Severity::ERROR,
                     "Precondition failed", nodes, "guard1");

  // Convert to JSON and back
  nlohmann::json j = original.toJson();
  Violation deserialized = Violation::fromJson(j);

  EXPECT_EQ(deserialized.violationType, original.violationType);
  EXPECT_EQ(deserialized.severity, original.severity);
  EXPECT_EQ(deserialized.message, original.message);
  EXPECT_EQ(deserialized.affectedNodes.size(), 1);
  EXPECT_EQ(deserialized.affectedNodes[0].getBits(), 42);
  EXPECT_TRUE(deserialized.shapeReference.has_value());
  EXPECT_EQ(deserialized.shapeReference.value(), "guard1");
}

// ============================================================================
// ViolationReport Tests
// ============================================================================

TEST(ViolationReportTest, EmptyReport) {
  ViolationReport report;
  EXPECT_TRUE(report.empty());
  EXPECT_EQ(report.count(), 0);
  EXPECT_FALSE(report.hasErrors());
  EXPECT_FALSE(report.getMaxSeverity().has_value());
}

TEST(ViolationReportTest, AddViolation) {
  ViolationReport report;
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::ERROR, "Parse failed"));

  EXPECT_FALSE(report.empty());
  EXPECT_EQ(report.count(), 1);
  EXPECT_TRUE(report.hasErrors());
}

TEST(ViolationReportTest, CountBySeverity) {
  ViolationReport report;
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::ERROR, "Error 1"));
  report.add(Violation(ErrorCode::SYNTAX_ERROR, Severity::ERROR, "Error 2"));
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::WARNING, "Warning 1"));
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::INFO, "Info 1"));

  EXPECT_EQ(report.countBySeverity(Severity::ERROR), 2);
  EXPECT_EQ(report.countBySeverity(Severity::WARNING), 1);
  EXPECT_EQ(report.countBySeverity(Severity::INFO), 1);
  EXPECT_EQ(report.countBySeverity(Severity::CRITICAL), 0);
}

TEST(ViolationReportTest, CountByType) {
  ViolationReport report;
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::ERROR, "Parse 1"));
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::WARNING, "Parse 2"));
  report.add(Violation(ErrorCode::SYNTAX_ERROR, Severity::ERROR, "Syntax 1"));

  EXPECT_EQ(report.countByType(ErrorCode::PARSE_ERROR), 2);
  EXPECT_EQ(report.countByType(ErrorCode::SYNTAX_ERROR), 1);
  EXPECT_EQ(report.countByType(ErrorCode::UNKNOWN_ERROR), 0);
}

TEST(ViolationReportTest, GetMaxSeverity) {
  ViolationReport report;
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::WARNING, "Warn"));
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::ERROR, "Error"));
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::INFO, "Info"));

  auto maxSev = report.getMaxSeverity();
  ASSERT_TRUE(maxSev.has_value());
  EXPECT_EQ(maxSev.value(), Severity::ERROR);
}

TEST(ViolationReportTest, JsonSerialization) {
  ViolationReport original;
  original.add(Violation(ErrorCode::SHACL_MIN_COUNT_VIOLATION, Severity::ERROR,
                         "Min count violated"));
  original.add(Violation(ErrorCode::SHACL_MAX_COUNT_VIOLATION, Severity::WARNING,
                         "Max count violated"));

  nlohmann::json j = original.toJson();
  ViolationReport deserialized = ViolationReport::fromJson(j);

  EXPECT_EQ(deserialized.count(), 2);
  EXPECT_EQ(deserialized.countBySeverity(Severity::ERROR), 1);
  EXPECT_EQ(deserialized.countBySeverity(Severity::WARNING), 1);
}

TEST(ViolationReportTest, Clear) {
  ViolationReport report;
  report.add(Violation(ErrorCode::PARSE_ERROR, Severity::ERROR, "Error"));
  EXPECT_FALSE(report.empty());

  report.clear();
  EXPECT_TRUE(report.empty());
  EXPECT_EQ(report.count(), 0);
}

// ============================================================================
// ResultDigest Tests
// ============================================================================

TEST(ResultDigestTest, BasicDigest) {
  std::vector<std::string> results = {"result1", "result2", "result3"};
  std::string digest = ResultDigest::computeDigest(results);

  // Should produce a 64-character hex string (SHA-256)
  EXPECT_EQ(digest.length(), 64);

  // Should be deterministic
  std::string digest2 = ResultDigest::computeDigest(results);
  EXPECT_EQ(digest, digest2);
}

TEST(ResultDigestTest, DigestOrderInvariance) {
  std::vector<std::string> results1 = {"a", "b", "c"};
  std::vector<std::string> results2 = {"c", "b", "a"};

  std::string digest1 = ResultDigest::computeDigest(results1);
  std::string digest2 = ResultDigest::computeDigest(results2);

  // Should be the same (sorted internally)
  EXPECT_EQ(digest1, digest2);
}

TEST(ResultDigestTest, DigestFromString) {
  std::string input = "test input string";
  std::string digest = ResultDigest::computeDigestFromString(input);

  EXPECT_EQ(digest.length(), 64);

  // Should be deterministic
  std::string digest2 = ResultDigest::computeDigestFromString(input);
  EXPECT_EQ(digest, digest2);
}

TEST(ResultDigestTest, VerifyDigest) {
  std::string input = "test";
  std::string digest = ResultDigest::computeDigestFromString(input);

  EXPECT_TRUE(ResultDigest::verifyDigest(digest, digest));
  EXPECT_FALSE(ResultDigest::verifyDigest(digest, "wrong_digest"));
}

TEST(ResultDigestTest, CombineDigests) {
  std::string digest1 = ResultDigest::computeDigestFromString("part1");
  std::string digest2 = ResultDigest::computeDigestFromString("part2");

  std::string combined = ResultDigest::combineDigests(digest1, digest2);

  EXPECT_EQ(combined.length(), 64);
  EXPECT_NE(combined, digest1);
  EXPECT_NE(combined, digest2);
}

TEST(ResultDigestTest, EmptyResults) {
  std::vector<std::string> empty;
  std::string digest = ResultDigest::computeDigest(empty);

  EXPECT_EQ(digest.length(), 64);
}

// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "engine/n3/N3ComplianceRunner.h"
#include "engine/n3/N3Config.h"
#include "engine/n3/N3ErrorCode.h"
#include "engine/n3/N3Service.h"
#include "engine/n3/N3VerifyResult.h"

using namespace ad_engine::n3;

// Test fixture for N3Service tests
class N3ServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create temporary test files
    test_dir_ = "/tmp/n3_test_" +
                std::to_string(std::chrono::system_clock::now()
                                   .time_since_epoch()
                                   .count());
    std::filesystem::create_directories(test_dir_);
  }

  void TearDown() override {
    // Clean up temporary files
    if (std::filesystem::exists(test_dir_)) {
      std::filesystem::remove_all(test_dir_);
    }
  }

  // Create a test file with given content
  std::string createTestFile(const std::string& content,
                             const std::string& filename = "test.n3") {
    std::string filepath = test_dir_ + "/" + filename;
    std::ofstream file(filepath);
    file << content;
    file.close();
    return filepath;
  }

  std::string test_dir_;
};

// Test N3Config
TEST_F(N3ServiceTest, ConfigDefaultValues) {
  N3Config config = N3Config::defaultConfig();
  EXPECT_EQ(config.max_input_size_bytes, 100 * 1024 * 1024);
  EXPECT_EQ(config.max_blank_nodes, 1'000'000);
  EXPECT_EQ(config.max_nesting_depth, 100);
  EXPECT_FALSE(config.strict_mode);
  EXPECT_TRUE(config.isValid());
}

TEST_F(N3ServiceTest, ConfigStrictMode) {
  N3Config config = N3Config::strictConfig();
  EXPECT_TRUE(config.strict_mode);
  EXPECT_TRUE(config.isValid());
}

TEST_F(N3ServiceTest, ConfigPermissive) {
  N3Config config = N3Config::permissiveConfig();
  EXPECT_EQ(config.max_input_size_bytes, 1024 * 1024 * 1024);
  EXPECT_FALSE(config.strict_mode);
  EXPECT_TRUE(config.isValid());
}

TEST_F(N3ServiceTest, ConfigMinimal) {
  N3Config config = N3Config::minimalConfig();
  EXPECT_EQ(config.max_input_size_bytes, 1024 * 1024);
  EXPECT_TRUE(config.isValid());
}

// Test N3ErrorCode
TEST_F(N3ServiceTest, ErrorCodeToString) {
  EXPECT_EQ(errorCodeToString(N3ErrorCode::PARSE_FAILED), "PARSE_FAILED");
  EXPECT_EQ(errorCodeToString(N3ErrorCode::FORMULAE_NOT_SUPPORTED),
            "FORMULAE_NOT_SUPPORTED");
  EXPECT_EQ(errorCodeToString(N3ErrorCode::INPUT_SIZE_EXCEEDED),
            "INPUT_SIZE_EXCEEDED");
}

TEST_F(N3ServiceTest, ErrorCodeDescription) {
  auto desc = errorCodeDescription(N3ErrorCode::FORMULAE_NOT_SUPPORTED);
  EXPECT_FALSE(std::string(desc).empty());
  EXPECT_NE(desc.find("formulae"), std::string::npos);
}

// Test N3Service with valid Turtle
TEST_F(N3ServiceTest, VerifyValidTurtle) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
)";

  std::string filepath = createTestFile(content);
  N3Service service;
  N3VerifyResult result = service.verifyFile(filepath);

  EXPECT_TRUE(result.ok);
  EXPECT_TRUE(result.errors.empty());
  EXPECT_FALSE(result.compliance_digest.empty());
  EXPECT_GT(result.stats.total_lines, 0);
}

// Test N3Service with unsupported feature (formulae)
TEST_F(N3ServiceTest, VerifyUnsupportedFormulae) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate { ex:inner ex:statement ex:value } .
)";

  std::string filepath = createTestFile(content);
  N3Service service;
  N3VerifyResult result = service.verifyFile(filepath);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  EXPECT_EQ(result.errors[0].code, N3ErrorCode::FORMULAE_NOT_SUPPORTED);
}

// Test N3Service with unsupported feature (implication)
TEST_F(N3ServiceTest, VerifyUnsupportedImplication) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
{ ex:a ex:b ex:c } => { ex:d ex:e ex:f } .
)";

  std::string filepath = createTestFile(content);
  N3Service service;
  N3VerifyResult result = service.verifyFile(filepath);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  // Should have both formulae and implication errors
  EXPECT_GE(result.errors.size(), 1);
}

// Test input size guard
TEST_F(N3ServiceTest, InputSizeGuard) {
  std::string content(2000, 'x');  // 2KB content
  N3Config config = N3Config::minimalConfig();
  config.max_input_size_bytes = 1024;  // 1KB limit

  N3Service service(config);
  N3VerifyResult result = service.verifyContent(content);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  EXPECT_EQ(result.errors[0].code, N3ErrorCode::INPUT_SIZE_EXCEEDED);
}

// Test blank node limit guard
TEST_F(N3ServiceTest, BlankNodeLimitGuard) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate _:blank1 .
ex:subject ex:predicate _:blank2 .
ex:subject ex:predicate _:blank3 .
)";

  N3Config config = N3Config::minimalConfig();
  config.max_blank_nodes = 2;  // Only allow 2 blank nodes

  std::string filepath = createTestFile(content);
  N3Service service(config);
  N3VerifyResult result = service.verifyFile(filepath);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  EXPECT_EQ(result.errors[0].code, N3ErrorCode::BLANK_NODE_LIMIT_EXCEEDED);
}

// Test nesting depth guard
TEST_F(N3ServiceTest, NestingDepthGuard) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ( ( ( ( 1 2 3 ) ) ) ) .
)";

  N3Config config = N3Config::minimalConfig();
  config.max_nesting_depth = 2;  // Low nesting depth

  std::string filepath = createTestFile(content);
  N3Service service(config);
  N3VerifyResult result = service.verifyFile(filepath);

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  EXPECT_EQ(result.errors[0].code, N3ErrorCode::NESTING_DEPTH_EXCEEDED);
}

// Test file not found
TEST_F(N3ServiceTest, FileNotFound) {
  N3Service service;
  N3VerifyResult result = service.verifyFile("/nonexistent/file.n3");

  EXPECT_FALSE(result.ok);
  EXPECT_FALSE(result.errors.empty());
  EXPECT_EQ(result.errors[0].code, N3ErrorCode::FILE_NOT_FOUND);
}

// Test JSON serialization
TEST_F(N3ServiceTest, JSONSerialization) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
)";

  std::string filepath = createTestFile(content);
  N3Service service;
  N3VerifyResult result = service.verifyFile(filepath);

  // Serialize to JSON
  nlohmann::json j = result.toJson();

  // Check JSON structure
  EXPECT_TRUE(j.contains("ok"));
  EXPECT_TRUE(j.contains("errors"));
  EXPECT_TRUE(j.contains("warnings"));
  EXPECT_TRUE(j.contains("compliance_digest"));
  EXPECT_TRUE(j.contains("stats"));

  // Deserialize from JSON
  N3VerifyResult result2 = N3VerifyResult::fromJson(j);
  EXPECT_EQ(result.ok, result2.ok);
  EXPECT_EQ(result.compliance_digest, result2.compliance_digest);
}

// Test digest determinism
TEST_F(N3ServiceTest, DigestDeterminism) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
)";

  N3Service service;
  N3VerifyResult result1 = service.verifyContent(content);
  N3VerifyResult result2 = service.verifyContent(content);

  EXPECT_EQ(result1.compliance_digest, result2.compliance_digest);
}

// Test N3ComplianceRunner basic functionality
TEST_F(N3ServiceTest, ComplianceRunnerBasic) {
  // Create test manifest
  nlohmann::json manifest;
  manifest["tests"] = nlohmann::json::array();

  // Create valid test case
  std::string valid_content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
)";
  std::string valid_file = createTestFile(valid_content, "valid.n3");

  nlohmann::json test1;
  test1["name"] = "valid_turtle";
  test1["input_file"] = valid_file;
  test1["should_pass"] = true;
  test1["description"] = "Valid Turtle document";
  manifest["tests"].push_back(test1);

  // Create invalid test case (with formulae)
  std::string invalid_content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate { ex:inner ex:statement ex:value } .
)";
  std::string invalid_file = createTestFile(invalid_content, "invalid.n3");

  nlohmann::json test2;
  test2["name"] = "invalid_formulae";
  test2["input_file"] = invalid_file;
  test2["should_pass"] = false;
  test2["description"] = "Document with unsupported formulae";
  manifest["tests"].push_back(test2);

  // Write manifest
  std::string manifest_file = test_dir_ + "/manifest.json";
  std::ofstream manifest_out(manifest_file);
  manifest_out << manifest.dump(2);
  manifest_out.close();

  // Run conformance tests
  N3ComplianceRunner runner;
  N3ConformanceResult result = runner.runConformanceTests(manifest_file);

  EXPECT_EQ(result.total_tests, 2);
  EXPECT_EQ(result.passed_tests, 2);
  EXPECT_EQ(result.failed_tests, 0);
}

// Test result summary
TEST_F(N3ServiceTest, ResultSummary) {
  std::string content = R"(
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
)";

  N3Service service;
  N3VerifyResult result = service.verifyContent(content);

  std::string summary = result.summary();
  EXPECT_FALSE(summary.empty());
  EXPECT_NE(summary.find("PASSED"), std::string::npos);
}

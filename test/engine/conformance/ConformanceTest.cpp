//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "engine/conformance/ConformanceDiffer.h"
#include "engine/conformance/ConformanceRunner.h"
#include "engine/conformance/ConformanceTestCase.h"
#include "engine/conformance/ConformanceUtils.h"
#include "util/GTestHelpers.h"

using namespace ad_engine::conformance;

// Test fixture for conformance tests
class ConformanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create temporary directory for test files
    testDir_ = std::filesystem::temp_directory_path() / "conformance_test";
    std::filesystem::create_directories(testDir_);
  }

  void TearDown() override {
    // Clean up temporary directory
    if (std::filesystem::exists(testDir_)) {
      std::filesystem::remove_all(testDir_);
    }
  }

  // Helper: Create a test file with given content
  std::string createTestFile(const std::string& filename,
                             const std::string& content) {
    auto filePath = testDir_ / filename;
    std::ofstream file(filePath);
    file << content;
    file.close();
    return filePath.string();
  }

  // Helper: Create a JSON test file
  std::string createJsonTestFile(const std::string& filename,
                                 const nlohmann::json& json) {
    return createTestFile(filename, json.dump(2));
  }

  std::filesystem::path testDir_;
};

// Test ConformanceTestCase serialization and deserialization
TEST_F(ConformanceTest, TestCaseJsonSerialization) {
  ConformanceTestCase testCase("test1", TestType::Validation, "/input.ttl",
                               "/expected.json", ExpectedOutcome::Violations);
  testCase.timeoutMs = 5000;
  testCase.tags = {"basic", "shacl"};
  testCase.metadata["author"] = "test_author";

  // Serialize to JSON
  nlohmann::json json = testCase.toJson();

  ASSERT_EQ(json["test_name"], "test1");
  ASSERT_EQ(json["test_type"], "Validation");
  ASSERT_EQ(json["input_path"], "/input.ttl");
  ASSERT_EQ(json["expected_output_path"], "/expected.json");
  ASSERT_EQ(json["expected_outcome"], "VIOLATIONS");
  ASSERT_EQ(json["timeout_ms"], 5000);
  ASSERT_TRUE(json["tags"].is_array());
  ASSERT_EQ(json["tags"].size(), 2);

  // Deserialize from JSON
  ConformanceTestCase deserialized = ConformanceTestCase::fromJson(json);

  ASSERT_EQ(deserialized.testName, "test1");
  ASSERT_EQ(deserialized.testType, TestType::Validation);
  ASSERT_EQ(deserialized.inputPath, "/input.ttl");
  ASSERT_EQ(deserialized.expectedOutputPath, "/expected.json");
  ASSERT_EQ(deserialized.expectedOutcome, ExpectedOutcome::Violations);
  ASSERT_EQ(deserialized.timeoutMs, 5000);
  ASSERT_EQ(deserialized.tags.size(), 2);
}

// Test ConformanceDiffer basic comparison
TEST_F(ConformanceTest, DifferBasicComparison) {
  nlohmann::json actual = {{"key1", "value1"}, {"key2", 42}};

  nlohmann::json expected = {{"key1", "value1"}, {"key2", 42}};

  DiffResult diff = ConformanceDiffer::compareJson(actual, expected);

  ASSERT_FALSE(diff.hasDifference);
  ASSERT_TRUE(diff.differences.empty());
}

// Test ConformanceDiffer detects differences
TEST_F(ConformanceTest, DifferDetectsDifferences) {
  nlohmann::json actual = {{"key1", "value1"}, {"key2", 42}};

  nlohmann::json expected = {{"key1", "value1"}, {"key2", 99}};

  DiffResult diff = ConformanceDiffer::compareJson(actual, expected);

  ASSERT_TRUE(diff.hasDifference);
  ASSERT_FALSE(diff.differences.empty());
  ASSERT_TRUE(diff.differences[0].find("key2") != std::string::npos);
}

// Test ConformanceDiffer array comparison
TEST_F(ConformanceTest, DifferArrayComparison) {
  nlohmann::json actual = {{"items", {1, 2, 3}}};

  nlohmann::json expected = {{"items", {1, 2, 3}}};

  DiffResult diff = ConformanceDiffer::compareJson(actual, expected);

  ASSERT_FALSE(diff.hasDifference);
}

// Test ConformanceDiffer array order independence
TEST_F(ConformanceTest, DifferArrayOrderIndependence) {
  nlohmann::json actual = {{"items", {3, 1, 2}}};

  nlohmann::json expected = {{"items", {1, 2, 3}}};

  // Without ignoring order - should have difference
  DiffResult diff1 = ConformanceDiffer::compareJson(actual, expected);
  ASSERT_TRUE(diff1.hasDifference);

  // With ignoring order - should be the same
  FuzzyMatchConfig config;
  config.ignoreArrayOrder = true;
  DiffResult diff2 = ConformanceDiffer::compareJson(actual, expected, config);
  ASSERT_FALSE(diff2.hasDifference);
}

// Test ConformanceDiffer fuzzy numeric matching
TEST_F(ConformanceTest, DifferFuzzyNumericMatching) {
  nlohmann::json actual = {{"value", 100.0}};

  nlohmann::json expected = {{"value", 105.0}};

  // Without tolerance - should differ
  DiffResult diff1 = ConformanceDiffer::compareJson(actual, expected);
  ASSERT_TRUE(diff1.hasDifference);

  // With 10% tolerance - should be approximately equal
  FuzzyMatchConfig config;
  config.numericTolerance = 0.10;  // 10%
  DiffResult diff2 = ConformanceDiffer::compareJson(actual, expected, config);
  ASSERT_FALSE(diff2.hasDifference);
}

// Test ConformanceUtils file operations
TEST_F(ConformanceTest, UtilsFileOperations) {
  nlohmann::json testData = {{"key", "value"}, {"number", 42}};

  std::string filePath = createJsonTestFile("test.json", testData);

  // Read back the JSON
  nlohmann::json readData = ConformanceUtils::readJsonFile(filePath);

  ASSERT_EQ(readData["key"], "value");
  ASSERT_EQ(readData["number"], 42);
}

// Test ConformanceUtils manifest loading
TEST_F(ConformanceTest, UtilsLoadManifest) {
  nlohmann::json manifest = nlohmann::json::array();

  nlohmann::json test1 = {{"test_name", "test1"},
                          {"test_type", "Validation"},
                          {"input_path", "/path/to/input1.ttl"},
                          {"expected_output_path", "/path/to/expected1.json"},
                          {"expected_outcome", "PASS"}};

  nlohmann::json test2 = {{"test_name", "test2"},
                          {"test_type", "Rules"},
                          {"input_path", "/path/to/input2.ttl"},
                          {"expected_output_path", "/path/to/expected2.json"},
                          {"expected_outcome", "VIOLATIONS"}};

  manifest.push_back(test1);
  manifest.push_back(test2);

  std::string manifestPath = createJsonTestFile("manifest.json", manifest);

  std::vector<ConformanceTestCase> testCases =
      ConformanceUtils::loadTestManifest(manifestPath);

  ASSERT_EQ(testCases.size(), 2);
  ASSERT_EQ(testCases[0].testName, "test1");
  ASSERT_EQ(testCases[0].testType, TestType::Validation);
  ASSERT_EQ(testCases[1].testName, "test2");
  ASSERT_EQ(testCases[1].testType, TestType::Rules);
}

// Test ConformanceRunner basic execution
TEST_F(ConformanceTest, RunnerBasicExecution) {
  // Create test input and expected output files
  nlohmann::json inputData = {{"input", "data"}};
  nlohmann::json expectedOutput = {{"violations", nlohmann::json::array()}};

  std::string inputPath = createJsonTestFile("input.json", inputData);
  std::string expectedPath = createJsonTestFile("expected.json", expectedOutput);

  // Create test case
  ConformanceTestCase testCase("test1", TestType::Validation, inputPath,
                               expectedPath, ExpectedOutcome::Pass);

  // Create runner with custom executor
  ConformanceRunner runner("TestRunner");

  runner.setTestExecutor([](const ConformanceTestCase& tc) -> nlohmann::json {
    // Mock executor that returns empty violations
    return {{"violations", nlohmann::json::array()}, {"status", "PASS"}};
  });

  // Run the test
  TestCaseResult result = runner.runTest(testCase);

  ASSERT_EQ(result.status, TestStatus::Pass);
  ASSERT_EQ(result.testName, "test1");
  ASSERT_FALSE(result.diff.hasDifference);
}

// Test ConformanceRunner handles errors
TEST_F(ConformanceTest, RunnerHandlesErrors) {
  // Create test input file
  nlohmann::json inputData = {{"input", "data"}};
  std::string inputPath = createJsonTestFile("input.json", inputData);

  // Create test case with non-existent expected output
  ConformanceTestCase testCase("test_error", TestType::Validation, inputPath,
                               "", ExpectedOutcome::Pass);

  // Create runner with executor that throws
  ConformanceRunner runner("TestRunner");

  runner.setTestExecutor([](const ConformanceTestCase& tc) -> nlohmann::json {
    throw std::runtime_error("Test error");
  });

  // Run the test
  TestCaseResult result = runner.runTest(testCase);

  ASSERT_EQ(result.status, TestStatus::Error);
  ASSERT_FALSE(result.errorMessage.empty());
  ASSERT_TRUE(result.errorMessage.find("Test error") != std::string::npos);
}

// Test ConformanceRunner summary statistics
TEST_F(ConformanceTest, RunnerSummaryStatistics) {
  // Create test files
  nlohmann::json inputData = {{"input", "data"}};
  nlohmann::json expectedPass = {{"status", "PASS"}};
  nlohmann::json expectedFail = {{"violations", {1, 2, 3}}};

  std::string input1 = createJsonTestFile("input1.json", inputData);
  std::string input2 = createJsonTestFile("input2.json", inputData);
  std::string expected1 = createJsonTestFile("expected1.json", expectedPass);
  std::string expected2 = createJsonTestFile("expected2.json", expectedFail);

  // Create test cases
  std::vector<ConformanceTestCase> testCases;
  testCases.push_back(ConformanceTestCase("test_pass", TestType::Validation,
                                          input1, expected1,
                                          ExpectedOutcome::Pass));
  testCases.push_back(ConformanceTestCase("test_fail", TestType::Validation,
                                          input2, expected2,
                                          ExpectedOutcome::Violations));

  // Create runner
  ConformanceRunner runner("TestRunner");

  runner.setTestExecutor([](const ConformanceTestCase& tc) -> nlohmann::json {
    if (tc.testName == "test_pass") {
      return {{"status", "PASS"}};
    } else {
      return {{"status", "PASS"}};  // Intentionally wrong to test failure
    }
  });

  // Run all tests
  ConformanceRunnerResult result = runner.runAllTests(testCases);

  ASSERT_EQ(result.summary.total, 2);
  ASSERT_EQ(result.runnerName, "TestRunner");
  ASSERT_FALSE(result.timestampIso8601.empty());
}

// Test TestCaseResult JSON serialization
TEST_F(ConformanceTest, TestCaseResultSerialization) {
  TestCaseResult result;
  result.testName = "test1";
  result.status = TestStatus::Pass;
  result.expectedOutcome = ExpectedOutcome::Pass;
  result.actualOutcome = "PASS";
  result.timingMs = 12.5;
  result.violationsExpected = 3;
  result.violationsActual = 3;

  nlohmann::json json = result.toJson();

  ASSERT_EQ(json["test_name"], "test1");
  ASSERT_EQ(json["status"], "PASS");
  ASSERT_EQ(json["expected_outcome"], "PASS");
  ASSERT_EQ(json["actual_outcome"], "PASS");
  ASSERT_EQ(json["timing_ms"], 12.5);
  ASSERT_EQ(json["violations_expected"], 3);
  ASSERT_EQ(json["violations_actual"], 3);
  ASSERT_EQ(json["passed"], true);
}

// Test ConformanceRunnerResult JSON serialization
TEST_F(ConformanceTest, RunnerResultSerialization) {
  ConformanceRunnerResult result;
  result.runnerName = "ShaclConformanceRunner";
  result.timestampIso8601 = "2026-01-01T00:00:00Z";

  TestCaseResult test1;
  test1.testName = "basic_shape_violation";
  test1.status = TestStatus::Pass;
  test1.expectedOutcome = ExpectedOutcome::Violations;
  test1.actualOutcome = "VIOLATIONS";
  test1.violationsExpected = 3;
  test1.violationsActual = 3;
  test1.timingMs = 12.0;

  result.testResults.push_back(test1);

  result.summary.total = 1;
  result.summary.passed = 1;
  result.summary.totalTimeMs = 12.0;

  nlohmann::json json = result.toJson();

  ASSERT_EQ(json["runner_name"], "ShaclConformanceRunner");
  ASSERT_EQ(json["timestamp_iso8601"], "2026-01-01T00:00:00Z");
  ASSERT_TRUE(json["tests"].is_array());
  ASSERT_EQ(json["tests"].size(), 1);
  ASSERT_EQ(json["tests"][0]["test_name"], "basic_shape_violation");
  ASSERT_EQ(json["summary"]["total"], 1);
  ASSERT_EQ(json["summary"]["passed"], 1);
}

// Test digest computation for reproducibility
TEST_F(ConformanceTest, DigestComputation) {
  nlohmann::json data1 = {{"key", "value"}, {"number", 42}};
  nlohmann::json data2 = {{"key", "value"}, {"number", 42}};
  nlohmann::json data3 = {{"key", "different"}, {"number", 42}};

  std::string digest1 = ConformanceUtils::computeDigest(data1);
  std::string digest2 = ConformanceUtils::computeDigest(data2);
  std::string digest3 = ConformanceUtils::computeDigest(data3);

  // Same data should produce same digest
  ASSERT_EQ(digest1, digest2);

  // Different data should produce different digest
  ASSERT_NE(digest1, digest3);
}

// Test validation result comparison
TEST_F(ConformanceTest, DifferValidationResults) {
  nlohmann::json actual = {{"violations", {1, 2, 3}}};
  nlohmann::json expected = {{"violations", {1, 2, 3}}};

  DiffResult diff =
      ConformanceDiffer::compareValidationResults(actual, expected);

  ASSERT_FALSE(diff.hasDifference);
}

// Test rule result comparison
TEST_F(ConformanceTest, DifferRuleResults) {
  nlohmann::json actual = {
      {"fact_count", 100}, {"digest", "abc123"}, {"rule_fires", {10, 20, 30}}};
  nlohmann::json expected = {
      {"fact_count", 100}, {"digest", "abc123"}, {"rule_fires", {10, 20, 30}}};

  DiffResult diff = ConformanceDiffer::compareRuleResults(actual, expected);

  ASSERT_FALSE(diff.hasDifference);
}

// Test N3 result comparison
TEST_F(ConformanceTest, DifferN3Results) {
  nlohmann::json actual = {
      {"error_code", "NONE"}, {"warnings", nlohmann::json::array()}};
  nlohmann::json expected = {
      {"error_code", "NONE"}, {"warnings", nlohmann::json::array()}};

  DiffResult diff = ConformanceDiffer::compareN3Results(actual, expected);

  ASSERT_FALSE(diff.hasDifference);
}

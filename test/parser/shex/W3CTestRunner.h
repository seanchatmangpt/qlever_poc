#ifndef TEST_PARSER_SHEX_W3CTESTRUNNER_H
#define TEST_PARSER_SHEX_W3CTESTRUNNER_H

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include "W3CManifestParser.h"
#include "parser/ShEx.h"

namespace shex::testing {

// ============================================================================
// Test Execution Result
// ============================================================================

enum class TestResult {
  PASS,                // Test passed as expected
  FAIL,                // Test failed unexpectedly
  XFAIL,               // Test failed as expected (expected failure)
  SKIP,                // Test was skipped
  ERROR                // Test encountered an error during execution
};

struct TestExecutionResult {
  std::string testId;
  TestResult result;
  std::string message;       // Error message or additional info
  std::chrono::milliseconds executionTime;
  std::optional<std::string> actualOutput;    // Actual output from test
  std::optional<std::string> expectedOutput;  // Expected output
  std::vector<std::string> errorDetails;      // Detailed error information

  TestExecutionResult() : result(TestResult::ERROR), executionTime(0) {}

  bool isSuccess() const {
    return result == TestResult::PASS || result == TestResult::XFAIL || result == TestResult::SKIP;
  }
};

// ============================================================================
// Skip and Expected Failure Management
// ============================================================================

class TestConfiguration {
 public:
  TestConfiguration() = default;

  // Load skip list from JSON file
  bool loadSkipList(const std::string& filePath);

  // Load expected failure list from JSON file
  bool loadExpectedFailures(const std::string& filePath);

  // Check if a test should be skipped
  bool shouldSkip(const std::string& testId) const;

  // Check if a test is expected to fail
  bool isExpectedFailure(const std::string& testId) const;

  // Get skip reason
  std::optional<std::string> getSkipReason(const std::string& testId) const;

  // Get expected failure reason
  std::optional<std::string> getExpectedFailureReason(const std::string& testId) const;

 private:
  absl::flat_hash_map<std::string, std::string> skipList_;        // testId -> reason
  absl::flat_hash_map<std::string, std::string> expectedFailures_; // testId -> reason
};

// ============================================================================
// W3C Test Runner
// ============================================================================

class W3CTestRunner {
 public:
  explicit W3CTestRunner(const TestConfiguration& config = TestConfiguration())
      : config_(config) {}

  // Execute a single test case
  TestExecutionResult executeTest(const W3CTestCase& testCase);

  // Execute multiple test cases
  std::vector<TestExecutionResult> executeTests(const std::vector<W3CTestCase>& testCases);

  // Execute validation test
  TestExecutionResult executeValidationTest(const W3CTestCase& testCase);

  // Execute negative syntax test
  TestExecutionResult executeNegativeSyntaxTest(const W3CTestCase& testCase);

  // Execute negative structure test
  TestExecutionResult executeNegativeStructureTest(const W3CTestCase& testCase);

  // Execute positive syntax test
  TestExecutionResult executePositiveSyntaxTest(const W3CTestCase& testCase);

  // Set verbose mode
  void setVerbose(bool verbose) { verbose_ = verbose; }

 private:
  TestConfiguration config_;
  bool verbose_ = false;

  // Helper methods
  bool fileExists(const std::string& path) const;
  std::string readFile(const std::string& path) const;
  void logVerbose(const std::string& message) const;

  // Parse schema from file
  std::optional<ShExSchema> parseSchemaFromFile(const std::string& schemaFile,
                                                 std::string& errorMsg);

  // Parse data from file (stub - would integrate with RDF parser)
  bool parseDataFromFile(const std::string& dataFile, std::string& errorMsg);

  // Compare actual vs expected results
  bool compareResults(const std::string& actual, const std::string& expected);
};

}  // namespace shex::testing

#endif  // TEST_PARSER_SHEX_W3CTESTRUNNER_H

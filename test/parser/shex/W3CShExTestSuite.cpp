#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "W3CManifestParser.h"
#include "W3CTestRunner.h"
#include "ComplianceTracker.h"

using namespace shex::testing;

// ============================================================================
// Test Fixture for W3C ShEx Tests
// ============================================================================

class W3CShExTestSuiteBase : public ::testing::Test {
 protected:
  void SetUp() override {
    // Determine test data directory
    testDataDir_ = std::filesystem::path(__FILE__).parent_path() / "test-data";

    // If test-data doesn't exist, try to find it in common locations
    if (!std::filesystem::exists(testDataDir_)) {
      // Try relative to build directory
      testDataDir_ = std::filesystem::current_path() / "test" / "parser" / "shex" / "test-data";
    }

    // Check if test data exists
    hasTestData_ = std::filesystem::exists(testDataDir_);

    if (hasTestData_) {
      // Load test configuration
      std::filesystem::path skipListPath = testDataDir_ / "skip-list.json";
      std::filesystem::path xfailListPath = testDataDir_ / "xfail-list.json";

      if (std::filesystem::exists(skipListPath)) {
        testConfig_.loadSkipList(skipListPath.string());
      }

      if (std::filesystem::exists(xfailListPath)) {
        testConfig_.loadExpectedFailures(xfailListPath.string());
      }

      // Initialize test runner
      testRunner_ = std::make_unique<W3CTestRunner>(testConfig_);
    }
  }

  void TearDown() override {
    // Save compliance reports if tests were run
    if (complianceTracker_ && complianceTracker_->getStatistics().totalTests > 0) {
      std::filesystem::path reportDir = testDataDir_ / "reports";
      std::filesystem::create_directories(reportDir);

      complianceTracker_->saveJSONReport((reportDir / "compliance-report.json").string());
      complianceTracker_->saveHTMLReport((reportDir / "compliance-report.html").string());
      complianceTracker_->saveFeatureCoverageCSV((reportDir / "feature-coverage.csv").string());
    }
  }

  std::filesystem::path testDataDir_;
  bool hasTestData_ = false;
  TestConfiguration testConfig_;
  std::unique_ptr<W3CTestRunner> testRunner_;
  std::unique_ptr<ComplianceTracker> complianceTracker_;
};

// ============================================================================
// Parameterized Test Fixture
// ============================================================================

class W3CShExParameterizedTest : public W3CShExTestSuiteBase,
                                  public ::testing::WithParamInterface<W3CTestCase> {
 protected:
  void SetUp() override {
    W3CShExTestSuiteBase::SetUp();
  }
};

// ============================================================================
// Test Case Loading
// ============================================================================

std::vector<W3CTestCase> loadW3CTestCases() {
  W3CShExTestSuiteBase base;
  base.SetUp();

  if (!base.hasTestData_) {
    std::cerr << "Warning: W3C test data not found at: " << base.testDataDir_ << std::endl;
    std::cerr << "Skipping W3C compliance tests. To run these tests, clone the W3C test repository:" << std::endl;
    std::cerr << "  git clone https://github.com/shexSpec/shexTest.git test/parser/shex/test-data" << std::endl;
    return {};
  }

  // Parse manifests
  W3CManifestParser parser;
  std::filesystem::path manifestDir = base.testDataDir_;

  // Try to find manifest files
  std::vector<W3CTestCase> testCases;

  if (std::filesystem::exists(manifestDir / "manifest.json")) {
    testCases = parser.parseManifest(manifestDir / "manifest.json");
  } else if (std::filesystem::exists(manifestDir / "manifest.jsonld")) {
    testCases = parser.parseManifest(manifestDir / "manifest.jsonld");
  } else {
    // Try to find all manifests recursively
    testCases = parser.parseManifestsInDirectory(manifestDir);
  }

  if (testCases.empty()) {
    std::cerr << "Warning: No test cases found in manifest" << std::endl;
    std::cerr << "Parser error: " << parser.getLastError() << std::endl;
  } else {
    auto stats = parser.getStatistics();
    std::cout << "Loaded " << stats.totalTests << " W3C test cases" << std::endl;
    std::cout << "  Validation: " << stats.validationTests << std::endl;
    std::cout << "  Negative Syntax: " << stats.negativeSyntaxTests << std::endl;
    std::cout << "  Negative Structure: " << stats.negativeStructureTests << std::endl;
    std::cout << "  Positive Syntax: " << stats.positiveSyntaxTests << std::endl;
  }

  return testCases;
}

// ============================================================================
// Parameterized Test
// ============================================================================

TEST_P(W3CShExParameterizedTest, ExecuteW3CTest) {
  if (!hasTestData_) {
    GTEST_SKIP() << "W3C test data not available";
  }

  auto testCase = GetParam();

  // Execute the test
  auto result = testRunner_->executeTest(testCase);

  // Track result for compliance reporting
  if (!complianceTracker_) {
    complianceTracker_ = std::make_unique<ComplianceTracker>();
  }
  complianceTracker_->trackResult(testCase, result);

  // Report result
  switch (result.result) {
    case TestResult::PASS:
      SUCCEED();
      break;

    case TestResult::FAIL:
      FAIL() << "Test failed: " << result.message << "\n"
             << "Test ID: " << testCase.id << "\n"
             << "Category: " << testCase.category;
      break;

    case TestResult::SKIP:
      GTEST_SKIP() << "Test skipped: " << result.message;
      break;

    case TestResult::XFAIL:
      // Expected failure - report as success
      SUCCEED() << "Expected failure: " << result.message;
      break;

    case TestResult::ERROR:
      FAIL() << "Test error: " << result.message << "\n"
             << "Test ID: " << testCase.id;
      break;
  }
}

// Instantiate parameterized tests
INSTANTIATE_TEST_SUITE_P(W3CCompliance,
                         W3CShExParameterizedTest,
                         ::testing::ValuesIn(loadW3CTestCases()),
                         [](const ::testing::TestParamInfo<W3CTestCase>& info) {
                           // Generate test name from test case ID
                           std::string name = info.param.id;
                           // Replace invalid characters for test names
                           std::replace(name.begin(), name.end(), ':', '_');
                           std::replace(name.begin(), name.end(), '/', '_');
                           std::replace(name.begin(), name.end(), '-', '_');
                           std::replace(name.begin(), name.end(), '.', '_');
                           return name;
                         });

// ============================================================================
// Compliance Summary Test
// ============================================================================

TEST_F(W3CShExTestSuiteBase, GenerateComplianceSummary) {
  if (!hasTestData_) {
    GTEST_SKIP() << "W3C test data not available";
  }

  // Load all test cases
  auto testCases = loadW3CTestCases();

  if (testCases.empty()) {
    GTEST_SKIP() << "No test cases loaded";
  }

  // Execute all tests
  ComplianceTracker tracker;
  for (const auto& testCase : testCases) {
    auto result = testRunner_->executeTest(testCase);
    tracker.trackResult(testCase, result);
  }

  // Generate and display summary
  auto stats = tracker.getStatistics();

  std::cout << "\n=== W3C ShEx Compliance Summary ===" << std::endl;
  std::cout << "Total Tests: " << stats.totalTests << std::endl;
  std::cout << "Passed: " << stats.passed << std::endl;
  std::cout << "Failed: " << stats.failed << std::endl;
  std::cout << "Skipped: " << stats.skipped << std::endl;
  std::cout << "Expected Failures: " << stats.expectedFailures << std::endl;
  std::cout << "Errors: " << stats.errors << std::endl;
  std::cout << "Compliance: " << std::fixed << std::setprecision(2)
            << stats.getCompliancePercentage() << "%" << std::endl;
  std::cout << "Pass Rate: " << std::fixed << std::setprecision(2)
            << stats.getPassRate() << "%" << std::endl;

  // Display feature coverage
  auto featureCoverage = tracker.getFeatureCoverage();
  std::cout << "\n=== Feature Coverage ===" << std::endl;
  for (const auto& fc : featureCoverage) {
    std::cout << fc.featureName << ": "
              << fc.passedTests << "/" << fc.totalTests << " ("
              << std::fixed << std::setprecision(1) << fc.getCoveragePercentage() << "%)"
              << (fc.isImplemented() ? " [Implemented]" : " [Not Implemented]")
              << std::endl;
  }

  // Save reports
  std::filesystem::path reportDir = testDataDir_ / "reports";
  std::filesystem::create_directories(reportDir);

  tracker.saveJSONReport((reportDir / "compliance-report.json").string());
  tracker.saveHTMLReport((reportDir / "compliance-report.html").string());
  tracker.saveFeatureCoverageCSV((reportDir / "feature-coverage.csv").string());

  std::cout << "\nReports saved to: " << reportDir << std::endl;

  // This test always succeeds - it's just for reporting
  SUCCEED();
}

// ============================================================================
// Manual Test for Specific Features
// ============================================================================

TEST_F(W3CShExTestSuiteBase, TestCardinalityFeature) {
  if (!hasTestData_) {
    GTEST_SKIP() << "W3C test data not available";
  }

  auto testCases = loadW3CTestCases();
  auto cardinalityTests = TestCaseFilter::filterByFeature(testCases, "cardinality");

  std::cout << "Testing cardinality feature: " << cardinalityTests.size() << " tests" << std::endl;

  ComplianceTracker tracker;
  for (const auto& testCase : cardinalityTests) {
    auto result = testRunner_->executeTest(testCase);
    tracker.trackResult(testCase, result);
  }

  auto stats = tracker.getStatistics();
  std::cout << "Cardinality tests: " << stats.passed << "/" << stats.totalTests << " passed" << std::endl;

  // Report as informational only
  SUCCEED();
}

TEST_F(W3CShExTestSuiteBase, TestNodeKindFeature) {
  if (!hasTestData_) {
    GTEST_SKIP() << "W3C test data not available";
  }

  auto testCases = loadW3CTestCases();
  auto nodeKindTests = TestCaseFilter::filterByFeature(testCases, "nodeKind");

  std::cout << "Testing nodeKind feature: " << nodeKindTests.size() << " tests" << std::endl;

  ComplianceTracker tracker;
  for (const auto& testCase : nodeKindTests) {
    auto result = testRunner_->executeTest(testCase);
    tracker.trackResult(testCase, result);
  }

  auto stats = tracker.getStatistics();
  std::cout << "NodeKind tests: " << stats.passed << "/" << stats.totalTests << " passed" << std::endl;

  // Report as informational only
  SUCCEED();
}

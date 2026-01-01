#ifndef TEST_PARSER_SHEX_COMPLIANCETRACKER_H
#define TEST_PARSER_SHEX_COMPLIANCETRACKER_H

#include <string>
#include <vector>
#include <chrono>
#include "W3CManifestParser.h"
#include "W3CTestRunner.h"
#include "nlohmann/json.hpp"
#include "absl/container/flat_hash_map.h"

namespace shex::testing {

// ============================================================================
// Compliance Statistics
// ============================================================================

struct ComplianceStatistics {
  size_t totalTests = 0;
  size_t passed = 0;
  size_t failed = 0;
  size_t skipped = 0;
  size_t expectedFailures = 0;
  size_t errors = 0;

  // Statistics by test type
  absl::flat_hash_map<std::string, size_t> passedByType;
  absl::flat_hash_map<std::string, size_t> failedByType;
  absl::flat_hash_map<std::string, size_t> totalByType;

  // Statistics by category
  absl::flat_hash_map<std::string, size_t> passedByCategory;
  absl::flat_hash_map<std::string, size_t> failedByCategory;
  absl::flat_hash_map<std::string, size_t> totalByCategory;

  // Statistics by feature
  absl::flat_hash_map<std::string, size_t> passedByFeature;
  absl::flat_hash_map<std::string, size_t> failedByFeature;
  absl::flat_hash_map<std::string, size_t> totalByFeature;

  // Timing statistics
  std::chrono::milliseconds totalExecutionTime{0};
  std::chrono::milliseconds averageExecutionTime{0};

  // Calculate compliance percentage
  double getCompliancePercentage() const {
    if (totalTests == 0) return 0.0;
    return (static_cast<double>(passed + expectedFailures) / totalTests) * 100.0;
  }

  // Calculate pass rate (excluding skipped tests)
  double getPassRate() const {
    size_t effective = totalTests - skipped;
    if (effective == 0) return 0.0;
    return (static_cast<double>(passed + expectedFailures) / effective) * 100.0;
  }
};

// ============================================================================
// Feature Coverage Matrix
// ============================================================================

struct FeatureCoverage {
  std::string featureName;
  size_t totalTests = 0;
  size_t passedTests = 0;
  size_t failedTests = 0;
  size_t skippedTests = 0;
  std::vector<std::string> failedTestIds;

  double getCoveragePercentage() const {
    if (totalTests == 0) return 0.0;
    return (static_cast<double>(passedTests) / totalTests) * 100.0;
  }

  bool isImplemented() const {
    return passedTests > 0 || (totalTests > 0 && skippedTests < totalTests);
  }
};

// ============================================================================
// Compliance Tracker
// ============================================================================

class ComplianceTracker {
 public:
  ComplianceTracker() = default;

  // Track a single test result
  void trackResult(const W3CTestCase& testCase, const TestExecutionResult& result);

  // Track multiple test results
  void trackResults(const std::vector<W3CTestCase>& testCases,
                   const std::vector<TestExecutionResult>& results);

  // Get overall statistics
  ComplianceStatistics getStatistics() const { return stats_; }

  // Get feature coverage
  std::vector<FeatureCoverage> getFeatureCoverage() const;

  // Get category statistics
  absl::flat_hash_map<std::string, ComplianceStatistics> getCategoryStatistics() const;

  // Generate JSON compliance report
  nlohmann::json generateJSONReport() const;

  // Generate HTML compliance report
  std::string generateHTMLReport() const;

  // Generate feature coverage matrix (CSV format)
  std::string generateFeatureCoverageCSV() const;

  // Save report to file
  bool saveJSONReport(const std::string& filePath) const;
  bool saveHTMLReport(const std::string& filePath) const;
  bool saveFeatureCoverageCSV(const std::string& filePath) const;

  // Get failed tests for a specific feature
  std::vector<std::string> getFailedTestsForFeature(const std::string& feature) const;

  // Get test results by category
  std::vector<TestExecutionResult> getResultsByCategory(const std::string& category) const;

 private:
  ComplianceStatistics stats_;
  std::vector<std::pair<W3CTestCase, TestExecutionResult>> results_;

  // Helper methods for HTML generation
  std::string generateHTMLHeader() const;
  std::string generateHTMLSummary() const;
  std::string generateHTMLFeatureTable() const;
  std::string generateHTMLCategoryTable() const;
  std::string generateHTMLFailedTests() const;
  std::string generateHTMLFooter() const;

  // Helper methods for statistics
  void updateStatistics(const W3CTestCase& testCase, const TestExecutionResult& result);
  std::string testTypeToString(TestType type) const;
};

// ============================================================================
// Historical Tracking (for regression detection)
// ============================================================================

class HistoricalTracker {
 public:
  HistoricalTracker() = default;

  // Load historical data from CSV file
  bool loadHistory(const std::string& filePath);

  // Add current run to history
  void addRun(const std::string& timestamp, const ComplianceStatistics& stats);

  // Save history to CSV file
  bool saveHistory(const std::string& filePath) const;

  // Detect regressions (compared to last run)
  struct RegressionReport {
    bool hasRegressions;
    std::vector<std::string> newFailures;
    std::vector<std::string> fixedTests;
    int passingDelta;
    int failingDelta;
  };

  RegressionReport detectRegressions() const;

 private:
  struct HistoricalRun {
    std::string timestamp;
    ComplianceStatistics stats;
  };

  std::vector<HistoricalRun> history_;
};

}  // namespace shex::testing

#endif  // TEST_PARSER_SHEX_COMPLIANCETRACKER_H

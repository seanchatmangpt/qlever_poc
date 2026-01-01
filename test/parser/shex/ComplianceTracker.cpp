#include "ComplianceTracker.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace shex::testing {

// ============================================================================
// ComplianceTracker Implementation
// ============================================================================

void ComplianceTracker::trackResult(const W3CTestCase& testCase,
                                    const TestExecutionResult& result) {
  results_.push_back({testCase, result});
  updateStatistics(testCase, result);
}

void ComplianceTracker::trackResults(const std::vector<W3CTestCase>& testCases,
                                     const std::vector<TestExecutionResult>& results) {
  if (testCases.size() != results.size()) {
    throw std::invalid_argument("testCases and results must have the same size");
  }

  for (size_t i = 0; i < testCases.size(); ++i) {
    trackResult(testCases[i], results[i]);
  }
}

void ComplianceTracker::updateStatistics(const W3CTestCase& testCase,
                                         const TestExecutionResult& result) {
  stats_.totalTests++;
  stats_.totalExecutionTime += result.executionTime;

  // Update overall counts
  switch (result.result) {
    case TestResult::PASS:
      stats_.passed++;
      break;
    case TestResult::FAIL:
      stats_.failed++;
      break;
    case TestResult::SKIP:
      stats_.skipped++;
      break;
    case TestResult::XFAIL:
      stats_.expectedFailures++;
      break;
    case TestResult::ERROR:
      stats_.errors++;
      break;
  }

  // Update by test type
  std::string typeStr = testTypeToString(testCase.type);
  stats_.totalByType[typeStr]++;
  if (result.result == TestResult::PASS || result.result == TestResult::XFAIL) {
    stats_.passedByType[typeStr]++;
  } else if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
    stats_.failedByType[typeStr]++;
  }

  // Update by category
  stats_.totalByCategory[testCase.category]++;
  if (result.result == TestResult::PASS || result.result == TestResult::XFAIL) {
    stats_.passedByCategory[testCase.category]++;
  } else if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
    stats_.failedByCategory[testCase.category]++;
  }

  // Update by feature
  for (const auto& feature : testCase.features) {
    stats_.totalByFeature[feature]++;
    if (result.result == TestResult::PASS || result.result == TestResult::XFAIL) {
      stats_.passedByFeature[feature]++;
    } else if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
      stats_.failedByFeature[feature]++;
    }
  }

  // Update average execution time
  if (stats_.totalTests > 0) {
    stats_.averageExecutionTime = stats_.totalExecutionTime / stats_.totalTests;
  }
}

std::vector<FeatureCoverage> ComplianceTracker::getFeatureCoverage() const {
  std::vector<FeatureCoverage> coverage;

  // Collect all features
  absl::flat_hash_set<std::string> allFeatures;
  for (const auto& [testCase, result] : results_) {
    for (const auto& feature : testCase.features) {
      allFeatures.insert(feature);
    }
  }

  // Build coverage for each feature
  for (const auto& feature : allFeatures) {
    FeatureCoverage fc;
    fc.featureName = feature;

    for (const auto& [testCase, result] : results_) {
      // Check if this test covers this feature
      bool coversFeature = false;
      for (const auto& f : testCase.features) {
        if (f == feature) {
          coversFeature = true;
          break;
        }
      }

      if (coversFeature) {
        fc.totalTests++;
        if (result.result == TestResult::PASS || result.result == TestResult::XFAIL) {
          fc.passedTests++;
        } else if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
          fc.failedTests++;
          fc.failedTestIds.push_back(testCase.id);
        } else if (result.result == TestResult::SKIP) {
          fc.skippedTests++;
        }
      }
    }

    coverage.push_back(fc);
  }

  // Sort by feature name
  std::sort(coverage.begin(), coverage.end(),
            [](const FeatureCoverage& a, const FeatureCoverage& b) {
              return a.featureName < b.featureName;
            });

  return coverage;
}

absl::flat_hash_map<std::string, ComplianceStatistics> ComplianceTracker::getCategoryStatistics() const {
  absl::flat_hash_map<std::string, ComplianceStatistics> categoryStats;

  for (const auto& [testCase, result] : results_) {
    auto& catStats = categoryStats[testCase.category];
    catStats.totalTests++;

    switch (result.result) {
      case TestResult::PASS:
        catStats.passed++;
        break;
      case TestResult::FAIL:
        catStats.failed++;
        break;
      case TestResult::SKIP:
        catStats.skipped++;
        break;
      case TestResult::XFAIL:
        catStats.expectedFailures++;
        break;
      case TestResult::ERROR:
        catStats.errors++;
        break;
    }
  }

  return categoryStats;
}

nlohmann::json ComplianceTracker::generateJSONReport() const {
  nlohmann::json report;

  // Overall statistics
  report["summary"]["total_tests"] = stats_.totalTests;
  report["summary"]["passed"] = stats_.passed;
  report["summary"]["failed"] = stats_.failed;
  report["summary"]["skipped"] = stats_.skipped;
  report["summary"]["expected_failures"] = stats_.expectedFailures;
  report["summary"]["errors"] = stats_.errors;
  report["summary"]["compliance_percentage"] = stats_.getCompliancePercentage();
  report["summary"]["pass_rate"] = stats_.getPassRate();
  report["summary"]["total_execution_time_ms"] = stats_.totalExecutionTime.count();
  report["summary"]["average_execution_time_ms"] = stats_.averageExecutionTime.count();

  // Feature coverage
  auto featureCoverage = getFeatureCoverage();
  for (const auto& fc : featureCoverage) {
    nlohmann::json featureJson;
    featureJson["name"] = fc.featureName;
    featureJson["total_tests"] = fc.totalTests;
    featureJson["passed_tests"] = fc.passedTests;
    featureJson["failed_tests"] = fc.failedTests;
    featureJson["skipped_tests"] = fc.skippedTests;
    featureJson["coverage_percentage"] = fc.getCoveragePercentage();
    featureJson["implemented"] = fc.isImplemented();
    featureJson["failed_test_ids"] = fc.failedTestIds;
    report["feature_coverage"].push_back(featureJson);
  }

  // Category statistics
  auto categoryStats = getCategoryStatistics();
  for (const auto& [category, stats] : categoryStats) {
    nlohmann::json catJson;
    catJson["name"] = category;
    catJson["total_tests"] = stats.totalTests;
    catJson["passed"] = stats.passed;
    catJson["failed"] = stats.failed;
    catJson["skipped"] = stats.skipped;
    catJson["compliance_percentage"] = stats.getCompliancePercentage();
    report["category_statistics"].push_back(catJson);
  }

  // Failed tests details
  for (const auto& [testCase, result] : results_) {
    if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
      nlohmann::json failedTest;
      failedTest["id"] = testCase.id;
      failedTest["name"] = testCase.name;
      failedTest["category"] = testCase.category;
      failedTest["type"] = testTypeToString(testCase.type);
      failedTest["message"] = result.message;
      failedTest["features"] = testCase.features;
      report["failed_tests"].push_back(failedTest);
    }
  }

  return report;
}

std::string ComplianceTracker::generateHTMLReport() const {
  std::stringstream html;

  html << generateHTMLHeader();
  html << generateHTMLSummary();
  html << generateHTMLFeatureTable();
  html << generateHTMLCategoryTable();
  html << generateHTMLFailedTests();
  html << generateHTMLFooter();

  return html.str();
}

std::string ComplianceTracker::generateHTMLHeader() const {
  return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ShEx W3C Compliance Report</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        h1, h2 {
            color: #333;
        }
        .summary {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        .metric {
            display: inline-block;
            margin: 10px 20px 10px 0;
        }
        .metric-label {
            font-weight: bold;
            color: #666;
        }
        .metric-value {
            font-size: 24px;
            color: #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background-color: white;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #4CAF50;
            color: white;
        }
        tr:hover {
            background-color: #f5f5f5;
        }
        .pass { color: #4CAF50; font-weight: bold; }
        .fail { color: #f44336; font-weight: bold; }
        .skip { color: #FFC107; font-weight: bold; }
        .progress-bar {
            width: 100%;
            height: 30px;
            background-color: #f0f0f0;
            border-radius: 4px;
            overflow: hidden;
        }
        .progress-fill {
            height: 100%;
            background-color: #4CAF50;
            text-align: center;
            color: white;
            line-height: 30px;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <h1>ShEx W3C Compliance Report</h1>
)";
}

std::string ComplianceTracker::generateHTMLSummary() const {
  std::stringstream html;

  html << "    <div class=\"summary\">\n";
  html << "        <h2>Summary</h2>\n";
  html << "        <div class=\"metric\">\n";
  html << "            <div class=\"metric-label\">Total Tests</div>\n";
  html << "            <div class=\"metric-value\">" << stats_.totalTests << "</div>\n";
  html << "        </div>\n";
  html << "        <div class=\"metric\">\n";
  html << "            <div class=\"metric-label\">Passed</div>\n";
  html << "            <div class=\"metric-value pass\">" << stats_.passed << "</div>\n";
  html << "        </div>\n";
  html << "        <div class=\"metric\">\n";
  html << "            <div class=\"metric-label\">Failed</div>\n";
  html << "            <div class=\"metric-value fail\">" << stats_.failed << "</div>\n";
  html << "        </div>\n";
  html << "        <div class=\"metric\">\n";
  html << "            <div class=\"metric-label\">Skipped</div>\n";
  html << "            <div class=\"metric-value skip\">" << stats_.skipped << "</div>\n";
  html << "        </div>\n";
  html << "        <div class=\"metric\">\n";
  html << "            <div class=\"metric-label\">Compliance</div>\n";
  html << "            <div class=\"metric-value\">" << std::fixed << std::setprecision(1)
       << stats_.getCompliancePercentage() << "%</div>\n";
  html << "        </div>\n";
  html << "        <div style=\"margin-top: 20px;\">\n";
  html << "            <div class=\"progress-bar\">\n";
  html << "                <div class=\"progress-fill\" style=\"width: "
       << stats_.getCompliancePercentage() << "%\">"
       << std::fixed << std::setprecision(1) << stats_.getCompliancePercentage() << "%</div>\n";
  html << "            </div>\n";
  html << "        </div>\n";
  html << "    </div>\n";

  return html.str();
}

std::string ComplianceTracker::generateHTMLFeatureTable() const {
  std::stringstream html;
  auto featureCoverage = getFeatureCoverage();

  html << "    <h2>Feature Coverage</h2>\n";
  html << "    <table>\n";
  html << "        <tr>\n";
  html << "            <th>Feature</th>\n";
  html << "            <th>Total Tests</th>\n";
  html << "            <th>Passed</th>\n";
  html << "            <th>Failed</th>\n";
  html << "            <th>Coverage %</th>\n";
  html << "            <th>Status</th>\n";
  html << "        </tr>\n";

  for (const auto& fc : featureCoverage) {
    html << "        <tr>\n";
    html << "            <td>" << fc.featureName << "</td>\n";
    html << "            <td>" << fc.totalTests << "</td>\n";
    html << "            <td class=\"pass\">" << fc.passedTests << "</td>\n";
    html << "            <td class=\"fail\">" << fc.failedTests << "</td>\n";
    html << "            <td>" << std::fixed << std::setprecision(1)
         << fc.getCoveragePercentage() << "%</td>\n";
    html << "            <td>" << (fc.isImplemented() ? "Implemented" : "Not Implemented") << "</td>\n";
    html << "        </tr>\n";
  }

  html << "    </table>\n";
  return html.str();
}

std::string ComplianceTracker::generateHTMLCategoryTable() const {
  std::stringstream html;
  auto categoryStats = getCategoryStatistics();

  html << "    <h2>Category Statistics</h2>\n";
  html << "    <table>\n";
  html << "        <tr>\n";
  html << "            <th>Category</th>\n";
  html << "            <th>Total Tests</th>\n";
  html << "            <th>Passed</th>\n";
  html << "            <th>Failed</th>\n";
  html << "            <th>Compliance %</th>\n";
  html << "        </tr>\n";

  for (const auto& [category, stats] : categoryStats) {
    html << "        <tr>\n";
    html << "            <td>" << category << "</td>\n";
    html << "            <td>" << stats.totalTests << "</td>\n";
    html << "            <td class=\"pass\">" << stats.passed << "</td>\n";
    html << "            <td class=\"fail\">" << stats.failed << "</td>\n";
    html << "            <td>" << std::fixed << std::setprecision(1)
         << stats.getCompliancePercentage() << "%</td>\n";
    html << "        </tr>\n";
  }

  html << "    </table>\n";
  return html.str();
}

std::string ComplianceTracker::generateHTMLFailedTests() const {
  std::stringstream html;

  html << "    <h2>Failed Tests</h2>\n";
  html << "    <table>\n";
  html << "        <tr>\n";
  html << "            <th>Test ID</th>\n";
  html << "            <th>Name</th>\n";
  html << "            <th>Category</th>\n";
  html << "            <th>Type</th>\n";
  html << "            <th>Message</th>\n";
  html << "        </tr>\n";

  for (const auto& [testCase, result] : results_) {
    if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
      html << "        <tr>\n";
      html << "            <td>" << testCase.id << "</td>\n";
      html << "            <td>" << testCase.name << "</td>\n";
      html << "            <td>" << testCase.category << "</td>\n";
      html << "            <td>" << testTypeToString(testCase.type) << "</td>\n";
      html << "            <td>" << result.message << "</td>\n";
      html << "        </tr>\n";
    }
  }

  html << "    </table>\n";
  return html.str();
}

std::string ComplianceTracker::generateHTMLFooter() const {
  return R"(</body>
</html>
)";
}

std::string ComplianceTracker::generateFeatureCoverageCSV() const {
  std::stringstream csv;
  auto featureCoverage = getFeatureCoverage();

  // Header
  csv << "Feature,Total Tests,Passed,Failed,Skipped,Coverage %,Implemented\n";

  // Data rows
  for (const auto& fc : featureCoverage) {
    csv << fc.featureName << ","
        << fc.totalTests << ","
        << fc.passedTests << ","
        << fc.failedTests << ","
        << fc.skippedTests << ","
        << std::fixed << std::setprecision(2) << fc.getCoveragePercentage() << ","
        << (fc.isImplemented() ? "Yes" : "No") << "\n";
  }

  return csv.str();
}

bool ComplianceTracker::saveJSONReport(const std::string& filePath) const {
  try {
    std::ofstream file(filePath);
    if (!file.is_open()) {
      return false;
    }

    auto report = generateJSONReport();
    file << report.dump(2);  // Pretty print with 2-space indentation
    return true;

  } catch (const std::exception& e) {
    return false;
  }
}

bool ComplianceTracker::saveHTMLReport(const std::string& filePath) const {
  try {
    std::ofstream file(filePath);
    if (!file.is_open()) {
      return false;
    }

    file << generateHTMLReport();
    return true;

  } catch (const std::exception& e) {
    return false;
  }
}

bool ComplianceTracker::saveFeatureCoverageCSV(const std::string& filePath) const {
  try {
    std::ofstream file(filePath);
    if (!file.is_open()) {
      return false;
    }

    file << generateFeatureCoverageCSV();
    return true;

  } catch (const std::exception& e) {
    return false;
  }
}

std::vector<std::string> ComplianceTracker::getFailedTestsForFeature(const std::string& feature) const {
  std::vector<std::string> failedTests;

  for (const auto& [testCase, result] : results_) {
    if (result.result == TestResult::FAIL || result.result == TestResult::ERROR) {
      for (const auto& f : testCase.features) {
        if (f == feature) {
          failedTests.push_back(testCase.id);
          break;
        }
      }
    }
  }

  return failedTests;
}

std::vector<TestExecutionResult> ComplianceTracker::getResultsByCategory(const std::string& category) const {
  std::vector<TestExecutionResult> categoryResults;

  for (const auto& [testCase, result] : results_) {
    if (testCase.category == category) {
      categoryResults.push_back(result);
    }
  }

  return categoryResults;
}

std::string ComplianceTracker::testTypeToString(TestType type) const {
  switch (type) {
    case TestType::VALIDATION:
      return "Validation";
    case TestType::NEGATIVE_SYNTAX:
      return "NegativeSyntax";
    case TestType::NEGATIVE_STRUCTURE:
      return "NegativeStructure";
    case TestType::POSITIVE_SYNTAX:
      return "PositiveSyntax";
    case TestType::REPRESENTATIVE_SYNTAX:
      return "RepresentativeSyntax";
    default:
      return "Unknown";
  }
}

}  // namespace shex::testing

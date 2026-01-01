//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "engine/conformance/ShaclConformanceRunner.h"
#include "util/Exception.h"
#include "util/Log.h"

namespace fs = std::filesystem;
using namespace ad_engine::conformance;

int main(int argc, char** argv) {
  try {
    // Parse command line arguments
    std::string conformanceDir = "conformance/shacl";
    std::string outputPath = "conformance/shacl/runner_output.json";

    if (argc > 1) {
      conformanceDir = argv[1];
    }
    if (argc > 2) {
      outputPath = argv[2];
    }

    LOG(INFO) << "=== SHACL Conformance Test Runner ===";
    LOG(INFO) << "Conformance directory: " << conformanceDir;
    LOG(INFO) << "Output path: " << outputPath;

    // Create runner
    ShaclConformanceRunner runner;

    // Load test cases
    LOG(INFO) << "Loading test cases...";
    auto testCases = runner.loadTestCases(conformanceDir);

    if (testCases.empty()) {
      LOG(ERROR) << "No test cases found in: " << conformanceDir;
      return 1;
    }

    LOG(INFO) << "Found " << testCases.size() << " test cases";

    // Run all tests
    LOG(INFO) << "Running tests...";
    ConformanceRunnerResult result = runner.runAllTests(testCases);

    // Print summary to console
    std::cout << "\n=== Test Run Summary ===\n";
    std::cout << "Runner: " << result.runnerName << "\n";
    std::cout << "Timestamp: " << result.timestampIso8601 << "\n";
    std::cout << "\nResults:\n";
    std::cout << "  Total:    " << result.summary.total << "\n";
    std::cout << "  Passed:   " << result.summary.passed << "\n";
    std::cout << "  Failed:   " << result.summary.failed << "\n";
    std::cout << "  Skipped:  " << result.summary.skipped << "\n";
    std::cout << "  Errors:   " << result.summary.errors << "\n";
    std::cout << "  Timeouts: " << result.summary.timeouts << "\n";
    std::cout << "  Pass rate: " << std::fixed << std::setprecision(2)
              << result.summary.passRate() << "%\n";
    std::cout << "  Total time: " << result.summary.totalTimeMs << " ms\n";

    // Print individual test results
    std::cout << "\n=== Individual Test Results ===\n";
    for (const auto& testResult : result.testResults) {
      std::string statusSymbol =
          testResult.passed() ? "✓" : "✗";
      std::cout << statusSymbol << " " << testResult.testName << " - "
                << testStatusToString(testResult.status) << " ("
                << testResult.timingMs << " ms)\n";

      if (!testResult.passed() && !testResult.errorMessage.empty()) {
        std::cout << "    Error: " << testResult.errorMessage << "\n";
      }

      if (testResult.diff.hasDifference) {
        std::cout << "    Differences found: "
                  << testResult.diff.differences.size() << "\n";
        for (const auto& diff : testResult.diff.differences) {
          std::cout << "      " << diff << "\n";
        }
      }
    }

    // Write output JSON
    LOG(INFO) << "Writing output to: " << outputPath;
    
    // Create output directory if it doesn't exist
    fs::path outputFilePath(outputPath);
    fs::create_directories(outputFilePath.parent_path());

    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
      LOG(ERROR) << "Failed to open output file: " << outputPath;
      return 1;
    }

    outputFile << std::setw(2) << result.toJson() << std::endl;
    outputFile.close();

    LOG(INFO) << "Output written successfully";

    // Return exit code based on results
    if (result.summary.passed == result.summary.total) {
      std::cout << "\n✓ All tests passed!\n";
      return 0;
    } else {
      std::cout << "\n✗ Some tests failed\n";
      return 1;
    }

  } catch (const std::exception& e) {
    LOG(ERROR) << "Fatal error: " << e.what();
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}

//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_UTILS_H
#define QLEVER_CONFORMANCE_UTILS_H

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "engine/conformance/ConformanceResultSchema.h"
#include "engine/conformance/ConformanceTestCase.h"
#include "util/Exception.h"
#include "util/File.h"
#include "util/HashSet.h"
#include "util/json.h"

namespace ad_engine::conformance {

// Utility class for conformance test operations
class ConformanceUtils {
 public:
  // Load test manifest from disk
  // Manifest format: JSON file with array of test cases
  static std::vector<ConformanceTestCase> loadTestManifest(
      const std::string& manifestPath) {
    try {
      nlohmann::json manifestJson = fileToJson<nlohmann::json>(manifestPath);

      std::vector<ConformanceTestCase> testCases;

      if (!manifestJson.is_array()) {
        throw std::runtime_error(
            "Test manifest must be a JSON array of test cases");
      }

      for (const auto& testJson : manifestJson) {
        testCases.push_back(ConformanceTestCase::fromJson(testJson));
      }

      return testCases;
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to load test manifest from " +
                               manifestPath + ": " + e.what());
    }
  }

  // Load test cases from directory using file convention
  // Convention: Each .test.json file is a test case
  static std::vector<ConformanceTestCase> loadTestsFromDirectory(
      const std::string& directoryPath) {
    std::vector<ConformanceTestCase> testCases;

    // Note: This is a simplified implementation. In production,
    // you would use std::filesystem or Boost.Filesystem to iterate
    // through directory entries. For now, we'll document the expected
    // convention but leave the implementation as a stub that requires
    // an explicit manifest file.

    throw std::runtime_error(
        "loadTestsFromDirectory not yet implemented. "
        "Please use loadTestManifest with a manifest.json file instead.");
  }

  // Save runner output to disk as JSON
  static void saveRunnerOutput(const ConformanceRunnerResult& result,
                               const std::string& outputPath) {
    try {
      nlohmann::ordered_json j = result.toJson();

      std::ofstream outputFile(outputPath);
      if (!outputFile.is_open()) {
        throw std::runtime_error("Failed to open output file: " + outputPath);
      }

      // Write with pretty formatting (indent=2) for human readability
      outputFile << j.dump(2) << std::endl;
      outputFile.close();
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to save runner output to " +
                               outputPath + ": " + e.what());
    }
  }

  // Compute digest for reproducibility checks (SHA-256 hash of JSON)
  // This ensures that results are deterministic and can be compared across runs
  static std::string computeDigest(const nlohmann::json& data) {
    // Convert JSON to stable string representation
    // Use compact representation for digest (no whitespace)
    std::string jsonStr = data.dump();

    // Compute simple hash using std::hash (for real implementation,
    // use a proper cryptographic hash like SHA-256)
    std::hash<std::string> hasher;
    size_t hashValue = hasher(jsonStr);

    // Convert to hex string
    std::stringstream ss;
    ss << std::hex << hashValue;
    return ss.str();
  }

  // Read file contents as string (with error recovery)
  static std::string readFileContents(const std::string& filePath) {
    try {
      std::ifstream file = ad_utility::makeIfstream(filePath);
      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to read file " + filePath + ": " +
                               e.what());
    }
  }

  // Read file as JSON
  static nlohmann::json readJsonFile(const std::string& filePath) {
    try {
      return fileToJson<nlohmann::json>(filePath);
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to read JSON file " + filePath + ": " +
                               e.what());
    }
  }

  // Write JSON to file with stable ordering
  static void writeJsonFile(const nlohmann::json& data,
                            const std::string& filePath) {
    try {
      std::ofstream outputFile(filePath);
      if (!outputFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " +
                                 filePath);
      }

      // Use ordered_json for stable output
      nlohmann::ordered_json orderedData;
      if (data.is_object()) {
        // Sort keys for deterministic output
        for (auto it = data.begin(); it != data.end(); ++it) {
          orderedData[it.key()] = it.value();
        }
      } else {
        orderedData = data;
      }

      outputFile << orderedData.dump(2) << std::endl;
      outputFile.close();
    } catch (const std::exception& e) {
      throw std::runtime_error("Failed to write JSON file " + filePath + ": " +
                               e.what());
    }
  }

  // Sort JSON object keys for deterministic comparison
  static nlohmann::ordered_json sortJsonKeys(const nlohmann::json& input) {
    if (!input.is_object()) {
      return input;
    }

    nlohmann::ordered_json result;
    std::vector<std::string> keys;

    for (auto it = input.begin(); it != input.end(); ++it) {
      keys.push_back(it.key());
    }

    std::sort(keys.begin(), keys.end());

    for (const auto& key : keys) {
      const auto& value = input[key];
      if (value.is_object()) {
        result[key] = sortJsonKeys(value);
      } else if (value.is_array()) {
        nlohmann::ordered_json arrayResult = nlohmann::ordered_json::array();
        for (const auto& item : value) {
          if (item.is_object()) {
            arrayResult.push_back(sortJsonKeys(item));
          } else {
            arrayResult.push_back(item);
          }
        }
        result[key] = arrayResult;
      } else {
        result[key] = value;
      }
    }

    return result;
  }

  // Check if file exists
  static bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
  }

  // Validate test case paths
  static bool validateTestCase(const ConformanceTestCase& testCase,
                               std::string& errorMessage) {
    if (testCase.testName.empty()) {
      errorMessage = "Test name cannot be empty";
      return false;
    }

    if (testCase.inputPath.empty()) {
      errorMessage = "Input path cannot be empty";
      return false;
    }

    if (!fileExists(testCase.inputPath)) {
      errorMessage = "Input file does not exist: " + testCase.inputPath;
      return false;
    }

    if (!testCase.expectedOutputPath.empty() &&
        !fileExists(testCase.expectedOutputPath)) {
      errorMessage =
          "Expected output file does not exist: " + testCase.expectedOutputPath;
      return false;
    }

    return true;
  }
};

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_UTILS_H

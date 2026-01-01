#ifndef TEST_PARSER_SHEX_W3CMANIFESTPARSER_H
#define TEST_PARSER_SHEX_W3CMANIFESTPARSER_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace shex::testing {

// ============================================================================
// W3C ShEx Test Case Types
// ============================================================================

enum class TestType {
  VALIDATION,           // Test validation of data against schema
  NEGATIVE_SYNTAX,      // Test that schema should not parse
  NEGATIVE_STRUCTURE,   // Test that schema is syntactically valid but structurally invalid
  POSITIVE_SYNTAX,      // Test that schema should parse successfully
  REPRESENTATIVE_SYNTAX // Representative syntax examples
};

struct W3CTestCase {
  std::string id;                    // Test case identifier
  std::string name;                  // Human-readable name
  TestType type;                     // Type of test
  std::string schemaFile;            // Path to ShEx schema file
  std::optional<std::string> dataFile;  // Path to RDF data file (for validation tests)
  std::optional<std::string> shapeMap;  // Shape map for validation
  bool shouldPass;                   // Expected result (true = should pass, false = should fail)
  std::vector<std::string> features; // Features tested by this case
  std::string category;              // Test category (e.g., "cardinality", "nodeKind", "datatype")
  std::optional<std::string> comment; // Test description/comment

  // Metadata
  std::string manifestPath;          // Path to the manifest file
  int lineNumber;                    // Line number in manifest (for debugging)

  W3CTestCase() : type(TestType::VALIDATION), shouldPass(true), lineNumber(0) {}
};

// ============================================================================
// W3C Manifest Parser
// ============================================================================

class W3CManifestParser {
 public:
  W3CManifestParser() = default;

  // Parse a single manifest file
  // Returns test cases extracted from the manifest
  std::vector<W3CTestCase> parseManifest(const std::filesystem::path& manifestPath);

  // Parse all manifests in a directory recursively
  std::vector<W3CTestCase> parseManifestsInDirectory(const std::filesystem::path& directory);

  // Get the last error message
  std::string getLastError() const { return lastError_; }

  // Statistics
  struct Statistics {
    size_t totalTests = 0;
    size_t validationTests = 0;
    size_t negativeSyntaxTests = 0;
    size_t negativeStructureTests = 0;
    size_t positiveSyntaxTests = 0;
    size_t representativeSyntaxTests = 0;

    absl::flat_hash_map<std::string, size_t> testsByCategory;
    absl::flat_hash_map<std::string, size_t> testsByFeature;
  };

  Statistics getStatistics() const { return stats_; }

 private:
  std::string lastError_;
  Statistics stats_;

  // Parse JSON-LD manifest format
  std::vector<W3CTestCase> parseJSONLDManifest(const nlohmann::json& manifest,
                                               const std::filesystem::path& manifestPath);

  // Parse a single test entry from JSON
  std::optional<W3CTestCase> parseTestEntry(const nlohmann::json& entry,
                                            const std::filesystem::path& manifestPath);

  // Resolve file paths relative to manifest
  std::string resolvePath(const std::string& relativePath,
                         const std::filesystem::path& manifestPath);

  // Determine test type from JSON entry
  TestType determineTestType(const nlohmann::json& entry);

  // Extract features from test entry
  std::vector<std::string> extractFeatures(const nlohmann::json& entry);

  // Extract category from test entry
  std::string extractCategory(const nlohmann::json& entry);

  // Update statistics
  void updateStatistics(const W3CTestCase& testCase);

  // Validate manifest structure
  bool validateManifestStructure(const nlohmann::json& manifest);
};

// ============================================================================
// Test Case Filtering
// ============================================================================

class TestCaseFilter {
 public:
  // Filter by test type
  static std::vector<W3CTestCase> filterByType(const std::vector<W3CTestCase>& tests,
                                                TestType type);

  // Filter by feature
  static std::vector<W3CTestCase> filterByFeature(const std::vector<W3CTestCase>& tests,
                                                   const std::string& feature);

  // Filter by category
  static std::vector<W3CTestCase> filterByCategory(const std::vector<W3CTestCase>& tests,
                                                    const std::string& category);

  // Filter by expected result
  static std::vector<W3CTestCase> filterByExpectedResult(const std::vector<W3CTestCase>& tests,
                                                          bool shouldPass);

  // Get unique features from test cases
  static absl::flat_hash_set<std::string> getUniqueFeatures(const std::vector<W3CTestCase>& tests);

  // Get unique categories from test cases
  static absl::flat_hash_set<std::string> getUniqueCategories(const std::vector<W3CTestCase>& tests);
};

}  // namespace shex::testing

#endif  // TEST_PARSER_SHEX_W3CMANIFESTPARSER_H

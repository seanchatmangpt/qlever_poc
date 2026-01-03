// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// EPIC 10.3 - Agent 3: UIR Semantic Equivalence Validator
// Part 4: Test Corpus Implementation

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryPlanner.h"
#include "engine/idTable/IdTable.h"
#include "util/CryptographicHashUtils.h"
#include "util/Exception.h"
#include "util/File.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"
#include "util/StringUtils.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

// Path to UIR test corpus directory
const fs::path UIR_CORPUS_DIR = fs::path(__FILE__).parent_path() / "../uir";

// Struct representing a UIR test query entry
struct UIRTestQuery {
  std::string id;
  std::string file;
  std::string description;
  std::string category;  // "golden" or "hybrid"
  std::string expectedDigest;
};

// =============================================================================
// Semantic Equivalence Validation (PATCH_5 Implementation)
// =============================================================================

// Validate that two query results are semantically equivalent
// (byte-exact equality via IdTable::operator==)
//
// Per PATCH_5 Section 5.2: This delegates to IdTable's built-in operator==
// which performs:
//   1. Column count check
//   2. Row count check
//   3. Element-wise comparison (column-major iteration)
bool areResultsEquivalent(const IdTable& oldResult,
                          const IdTable& newResult) noexcept {
  return (oldResult == newResult);
}

// Detailed equivalence report for diagnostics
struct EquivalenceReport {
  bool is_equivalent;
  size_t old_rows;
  size_t old_cols;
  size_t new_rows;
  size_t new_cols;

  // If not equivalent, location of first difference
  bool has_difference;
  size_t diff_row;
  size_t diff_col;
  Id diff_old_value;
  Id diff_new_value;

  // Human-readable summary
  std::string summary;
};

// Generate detailed equivalence report for diagnostics
EquivalenceReport generateEquivalenceReport(const IdTable& oldResult,
                                            const IdTable& newResult) noexcept {
  EquivalenceReport report;
  report.old_rows = oldResult.numRows();
  report.old_cols = oldResult.numColumns();
  report.new_rows = newResult.numRows();
  report.new_cols = newResult.numColumns();
  report.has_difference = false;

  // Check row count
  if (report.old_rows != report.new_rows) {
    report.is_equivalent = false;
    std::ostringstream ss;
    ss << "Row count mismatch: old=" << report.old_rows
       << " new=" << report.new_rows;
    report.summary = ss.str();
    return report;
  }

  // Check column count
  if (report.old_cols != report.new_cols) {
    report.is_equivalent = false;
    std::ostringstream ss;
    ss << "Column count mismatch: old=" << report.old_cols
       << " new=" << report.new_cols;
    report.summary = ss.str();
    return report;
  }

  // Check element-wise equality
  for (size_t row = 0; row < report.old_rows; ++row) {
    for (size_t col = 0; col < report.old_cols; ++col) {
      Id old_val = oldResult(row, col);
      Id new_val = newResult(row, col);

      if (old_val != new_val) {
        report.is_equivalent = false;
        report.has_difference = true;
        report.diff_row = row;
        report.diff_col = col;
        report.diff_old_value = old_val;
        report.diff_new_value = new_val;

        std::ostringstream ss;
        ss << "Element mismatch at row " << row << ", col " << col
           << ": old=" << old_val.getBits() << " new=" << new_val.getBits();
        report.summary = ss.str();
        return report;
      }
    }
  }

  // All checks passed
  report.is_equivalent = true;
  report.summary = "Results are semantically equivalent (bit-exact match)";
  return report;
}

// =============================================================================
// Canonical Result Serialization (PATCH_4 Section 4.1)
// =============================================================================

// Serialize IdTable to canonical TSV format
// Rules:
//   1. Column order: Deterministic (by variable name alphabetically)
//   2. Row order: Sorted lexicographically by concatenated row values
//   3. Value serialization: Consistent representation
//   4. Line endings: Unix LF (\n)
//   5. Encoding: UTF-8
std::string serializeToCanonicalTSV(
    const IdTable& result, const std::vector<std::string>& columnNames) {
  // TODO: Implement canonical TSV serialization
  // For now, return stub (will be implemented when QueryPlanner integration is
  // ready)
  std::ostringstream oss;

  // Header row (sorted column names)
  std::vector<std::string> sortedCols = columnNames;
  std::sort(sortedCols.begin(), sortedCols.end());

  for (size_t i = 0; i < sortedCols.size(); ++i) {
    if (i > 0) oss << "\t";
    oss << "?" << sortedCols[i];
  }
  oss << "\n";

  // Data rows (stub - need vocabulary access for proper serialization)
  // In production, this would:
  // - Sort rows lexicographically
  // - Serialize each Id to N-Triples format via vocabulary lookup
  // - Join with tabs

  oss << "STUB_CANONICAL_TSV_" << result.numRows() << "x"
      << result.numColumns();
  return oss.str();
}

// =============================================================================
// Digest Computation (PATCH_4 Section 4.2)
// =============================================================================

// Compute SHA256 digest of canonical TSV representation
// Per PATCH_4: Uses ad_utility::HashSha256 and hexadecimal formatting
std::string computeDigest(const std::string& canonicalTSV) {
  auto hashBytes = ad_utility::HashSha256{}(canonicalTSV);
  return absl::StrJoin(hashBytes, "", ad_utility::hexFormatter);
}

// =============================================================================
// Manifest Loading (PATCH_4 Section 3.1)
// =============================================================================

// Load manifest.json and parse UIR test query entries
std::vector<UIRTestQuery> loadManifest() {
  fs::path manifestPath = UIR_CORPUS_DIR / "uir_test_manifest.json";

  AD_CONTRACT_CHECK(fs::exists(manifestPath),
                    "Manifest file not found: " + manifestPath.string());

  std::ifstream manifestFile(manifestPath);
  AD_CONTRACT_CHECK(manifestFile.is_open(),
                    "Failed to open manifest file: " + manifestPath.string());

  json manifest;
  manifestFile >> manifest;

  std::vector<UIRTestQuery> queries;
  for (const auto& query : manifest["queries"]) {
    UIRTestQuery uq;
    uq.id = query["id"];
    uq.file = query["file"];
    uq.description = query["description"];
    uq.category = query["category"];
    uq.expectedDigest = query["digest"];
    queries.push_back(uq);
  }

  return queries;
}

// Load SPARQL query from file
std::string loadQueryFile(const std::string& filename) {
  fs::path queryPath = UIR_CORPUS_DIR / filename;

  AD_CONTRACT_CHECK(fs::exists(queryPath),
                    "Query file not found: " + queryPath.string());

  std::ifstream queryFile(queryPath);
  AD_CONTRACT_CHECK(queryFile.is_open(),
                    "Failed to open query file: " + queryPath.string());

  std::stringstream buffer;
  buffer << queryFile.rdbuf();
  return buffer.str();
}

// =============================================================================
// Query Execution Stubs (Will integrate with actual UIR when ready)
// =============================================================================

// Execute SPARQL query via old QueryPlanner and return result
// NOTE: Stub implementation - will integrate with actual QueryPlanner
IdTable executeViaOldPlanner(const std::string& sparqlQuery,
                             QueryExecutionContext* qec) {
  // TODO: Integrate with actual QueryPlanner execution
  // This requires:
  // - Parsing SPARQL query
  // - Creating QueryExecutionTree via QueryPlanner
  // - Executing and retrieving IdTable result

  // Stub: Return empty IdTable for now
  IdTable result(0, ad_utility::makeUnlimitedAllocator<Id>());
  return result;
}

// Execute SPARQL query via new UnifiedPhysicalOptimizer and return result
// NOTE: Stub implementation - will integrate with actual UIR when Part 2 is
// complete
IdTable executeViaUIR(const std::string& sparqlQuery,
                      QueryExecutionContext* qec) {
  // TODO: Integrate with UnifiedPhysicalOptimizer (Part 2 deliverable)
  // This requires:
  // - Parsing SPARQL query
  // - Creating UIR via UnifiedPhysicalOptimizer
  // - Executing UIR and retrieving IdTable result

  // Stub: Return empty IdTable for now
  IdTable result(0, ad_utility::makeUnlimitedAllocator<Id>());
  return result;
}

}  // namespace

// =============================================================================
// Test Fixture (PATCH_4 Section 6)
// =============================================================================

class UIRSemanticEquivalenceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Verify UIR corpus directory exists
    ASSERT_TRUE(fs::exists(UIR_CORPUS_DIR))
        << "UIR corpus directory not found: " << UIR_CORPUS_DIR;

    // Load manifest
    queries_ = loadManifest();
    ASSERT_FALSE(queries_.empty()) << "No queries found in manifest";

    // Initialize QueryExecutionContext with test dataset
    // NOTE: Using minimal test index for now (will use LUBM(1,0) when
    // available)
    qec_ = ad_utility::testing::getQec();
  }

  std::vector<UIRTestQuery> queries_;
  std::shared_ptr<QueryExecutionContext> qec_;
};

// =============================================================================
// Infrastructure Validation Tests
// =============================================================================

// Test that manifest.json loads correctly
TEST_F(UIRSemanticEquivalenceTest, ManifestLoads) {
  EXPECT_EQ(queries_.size(), 150) << "Expected 150 test queries";

  // Count golden vs hybrid
  int goldenCount = 0;
  int hybridCount = 0;

  for (const auto& query : queries_) {
    EXPECT_FALSE(query.id.empty()) << "Query missing ID";
    EXPECT_FALSE(query.file.empty()) << "Query " << query.id << " missing file";
    EXPECT_FALSE(query.description.empty())
        << "Query " << query.id << " missing description";
    EXPECT_FALSE(query.category.empty())
        << "Query " << query.id << " missing category";
    EXPECT_FALSE(query.expectedDigest.empty())
        << "Query " << query.id << " missing expected digest";

    if (query.category == "golden") {
      goldenCount++;
    } else if (query.category == "hybrid") {
      hybridCount++;
    } else {
      ADD_FAILURE() << "Invalid category: " << query.category << " for query "
                    << query.id;
    }
  }

  EXPECT_EQ(goldenCount, 100) << "Expected 100 golden queries";
  EXPECT_EQ(hybridCount, 50) << "Expected 50 hybrid queries";
}

// Test that all query files exist and are readable
TEST_F(UIRSemanticEquivalenceTest, QueryFilesExist) {
  for (const auto& query : queries_) {
    fs::path queryPath = UIR_CORPUS_DIR / query.file;
    EXPECT_TRUE(fs::exists(queryPath)) << "Query file not found: " << queryPath;

    if (fs::exists(queryPath)) {
      std::string content = loadQueryFile(query.file);
      EXPECT_FALSE(content.empty()) << "Query file is empty: " << query.file;
    }
  }
}

// =============================================================================
// Equivalence Function Unit Tests (PATCH_5 Examples)
// =============================================================================

TEST(UIRSemanticEquivalenceUnitTest, EquivalentResults) {
  // Example 1 from PATCH_5 Section 8.1: EQUIVALENT results
  IdTable result1(2, ad_utility::makeUnlimitedAllocator<Id>());
  result1.resize(3);
  result1(0, 0) = Id::makeFromInt(1);
  result1(0, 1) = Id::makeFromInt(10);
  result1(1, 0) = Id::makeFromInt(2);
  result1(1, 1) = Id::makeFromInt(20);
  result1(2, 0) = Id::makeFromInt(3);
  result1(2, 1) = Id::makeFromInt(30);

  IdTable result2(2, ad_utility::makeUnlimitedAllocator<Id>());
  result2.resize(3);
  result2(0, 0) = Id::makeFromInt(1);
  result2(0, 1) = Id::makeFromInt(10);
  result2(1, 0) = Id::makeFromInt(2);
  result2(1, 1) = Id::makeFromInt(20);
  result2(2, 0) = Id::makeFromInt(3);
  result2(2, 1) = Id::makeFromInt(30);

  EXPECT_TRUE(areResultsEquivalent(result1, result2));
}

TEST(UIRSemanticEquivalenceUnitTest, RowCountMismatch) {
  // Example 2 from PATCH_5 Section 8.2: NOT EQUIVALENT (row count)
  IdTable result1(2, ad_utility::makeUnlimitedAllocator<Id>());
  result1.resize(2);
  result1(0, 0) = Id::makeFromInt(1);
  result1(0, 1) = Id::makeFromInt(10);
  result1(1, 0) = Id::makeFromInt(2);
  result1(1, 1) = Id::makeFromInt(20);

  IdTable result2(2, ad_utility::makeUnlimitedAllocator<Id>());
  result2.resize(3);
  result2(0, 0) = Id::makeFromInt(1);
  result2(0, 1) = Id::makeFromInt(10);
  result2(1, 0) = Id::makeFromInt(2);
  result2(1, 1) = Id::makeFromInt(20);
  result2(2, 0) = Id::makeFromInt(3);
  result2(2, 1) = Id::makeFromInt(30);

  EXPECT_FALSE(areResultsEquivalent(result1, result2));
}

TEST(UIRSemanticEquivalenceUnitTest, ElementMismatch) {
  // Example 3 from PATCH_5 Section 8.3: NOT EQUIVALENT (element)
  IdTable result1(2, ad_utility::makeUnlimitedAllocator<Id>());
  result1.resize(2);
  result1(0, 0) = Id::makeFromInt(1);
  result1(0, 1) = Id::makeFromInt(10);
  result1(1, 0) = Id::makeFromInt(2);
  result1(1, 1) = Id::makeFromInt(20);

  IdTable result2(2, ad_utility::makeUnlimitedAllocator<Id>());
  result2.resize(2);
  result2(0, 0) = Id::makeFromInt(1);
  result2(0, 1) = Id::makeFromInt(10);
  result2(1, 0) = Id::makeFromInt(2);
  result2(1, 1) = Id::makeFromInt(21);  // Different value

  EXPECT_FALSE(areResultsEquivalent(result1, result2));
}

TEST(UIRSemanticEquivalenceUnitTest, OrderingMismatch) {
  // Example 4 from PATCH_5 Section 8.4: NOT EQUIVALENT (ordering)
  IdTable result1(2, ad_utility::makeUnlimitedAllocator<Id>());
  result1.resize(2);
  result1(0, 0) = Id::makeFromInt(1);
  result1(0, 1) = Id::makeFromInt(10);
  result1(1, 0) = Id::makeFromInt(2);
  result1(1, 1) = Id::makeFromInt(20);

  IdTable result2(2, ad_utility::makeUnlimitedAllocator<Id>());
  result2.resize(2);
  result2(0, 0) = Id::makeFromInt(2);  // Rows swapped
  result2(0, 1) = Id::makeFromInt(20);
  result2(1, 0) = Id::makeFromInt(1);
  result2(1, 1) = Id::makeFromInt(10);

  EXPECT_FALSE(areResultsEquivalent(result1, result2));
}

TEST(UIRSemanticEquivalenceUnitTest, EmptyResults) {
  // Example from PATCH_5 Section 9.1: Empty results
  IdTable result1(2, ad_utility::makeUnlimitedAllocator<Id>());
  IdTable result2(2, ad_utility::makeUnlimitedAllocator<Id>());

  EXPECT_TRUE(areResultsEquivalent(result1, result2));
}

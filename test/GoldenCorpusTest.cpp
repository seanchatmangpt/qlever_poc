// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Agent 8 - Golden Query Harness
// EPIC 10.2 - Reference Physics Suite

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "util/CryptographicHashUtils.h"
#include "util/Exception.h"
#include "util/File.h"
#include "util/StringUtils.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

// Path to golden corpus directory
const fs::path GOLDEN_CORPUS_DIR =
    fs::path(__FILE__).parent_path() / "golden_corpus";

// Struct representing a golden query entry
struct GoldenQuery {
  std::string id;
  std::string file;
  std::string description;
  std::string expectedDigest;
};

// Load manifest.json and parse golden query entries
std::vector<GoldenQuery> loadManifest() {
  fs::path manifestPath = GOLDEN_CORPUS_DIR / "manifest.json";

  AD_CONTRACT_CHECK(fs::exists(manifestPath),
                    "Manifest file not found: " + manifestPath.string());

  std::ifstream manifestFile(manifestPath);
  AD_CONTRACT_CHECK(manifestFile.is_open(),
                    "Failed to open manifest file: " + manifestPath.string());

  json manifest;
  manifestFile >> manifest;

  std::vector<GoldenQuery> queries;
  for (const auto& query : manifest["queries"]) {
    GoldenQuery gq;
    gq.id = query["id"];
    gq.file = query["file"];
    gq.description = query["description"];
    gq.expectedDigest = query["digest"];
    queries.push_back(gq);
  }

  return queries;
}

// Load SPARQL query from file
std::string loadQueryFile(const std::string& filename) {
  fs::path queryPath = GOLDEN_CORPUS_DIR / filename;

  AD_CONTRACT_CHECK(fs::exists(queryPath),
                    "Query file not found: " + queryPath.string());

  std::ifstream queryFile(queryPath);
  AD_CONTRACT_CHECK(queryFile.is_open(),
                    "Failed to open query file: " + queryPath.string());

  std::stringstream buffer;
  buffer << queryFile.rdbuf();
  return buffer.str();
}

// Compute SHA256 digest of a string (canonical result representation)
std::string computeDigest(const std::string& data) {
  auto hashBytes = ad_utility::HashSha256{}(data);
  return absl::StrJoin(hashBytes, "", ad_utility::hexFormatter);
}

// Execute SPARQL query and return canonical result representation
//
// ASPIRATIONAL STUB: This function is not yet implemented.
// Tests using this function are DISABLED to prevent false positives.
//
// NOTE: This is a STUB implementation for tests. Real implementation requires:
// 1. Loading a test dataset (e.g., LUBM(1,0)) into QLever index
// 2. Executing query via QueryPlanner/Engine/Server
// 3. Serializing results in canonical TSV format (sorted, deterministic)
//
// This stub is intentionally simple to keep tests focused on manifest loading
// and digest computation infrastructure, not query execution.
//
// WARNING: This function returns the query itself, NOT query results.
// All tests using this function (e.g., DISABLED_ValidateGoldenQueryResults)
// must remain DISABLED until real query execution is implemented.
std::string executeQueryAndGetCanonicalResult(const std::string& sparqlQuery) {
  // Stub: Return the query itself as a placeholder
  // This allows testing the hashing and validation infrastructure
  // without requiring full query execution
  return sparqlQuery;
}

}  // namespace

// Test fixture for golden corpus validation
class GoldenCorpusTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Verify golden corpus directory exists
    ASSERT_TRUE(fs::exists(GOLDEN_CORPUS_DIR))
        << "Golden corpus directory not found: " << GOLDEN_CORPUS_DIR;

    // Load manifest
    queries_ = loadManifest();
    ASSERT_FALSE(queries_.empty()) << "No queries found in manifest";
  }

  std::vector<GoldenQuery> queries_;
};

// Test that manifest.json loads correctly
TEST_F(GoldenCorpusTest, ManifestLoads) {
  EXPECT_GT(queries_.size(), 0);

  // Verify each query has required fields
  for (const auto& query : queries_) {
    EXPECT_FALSE(query.id.empty()) << "Query missing ID";
    EXPECT_FALSE(query.file.empty()) << "Query " << query.id << " missing file";
    EXPECT_FALSE(query.description.empty())
        << "Query " << query.id << " missing description";
    EXPECT_FALSE(query.expectedDigest.empty())
        << "Query " << query.id << " missing expected digest";
  }
}

// Test that all query files exist and are readable
TEST_F(GoldenCorpusTest, QueryFilesExist) {
  for (const auto& query : queries_) {
    fs::path queryPath = GOLDEN_CORPUS_DIR / query.file;
    EXPECT_TRUE(fs::exists(queryPath)) << "Query file not found: " << queryPath;

    if (fs::exists(queryPath)) {
      std::string content = loadQueryFile(query.file);
      EXPECT_FALSE(content.empty()) << "Query file is empty: " << query.file;
    }
  }
}

// Test golden query result validation
// NOTE: Currently disabled until baseline digests are computed
TEST_F(GoldenCorpusTest, DISABLED_ValidateGoldenQueryResults) {
  int passCount = 0;
  int failCount = 0;

  for (const auto& query : queries_) {
    // Skip queries with pending baseline computation
    if (query.expectedDigest == "PENDING_BASELINE_COMPUTATION") {
      GTEST_SKIP() << "Baseline digest not yet computed for: " << query.id;
      continue;
    }

    // Load query
    std::string sparqlQuery = loadQueryFile(query.file);

    // Execute query and get canonical result
    std::string canonicalResult =
        executeQueryAndGetCanonicalResult(sparqlQuery);

    // Compute digest
    std::string actualDigest = computeDigest(canonicalResult);

    // Compare with expected digest
    if (actualDigest == query.expectedDigest) {
      passCount++;
    } else {
      failCount++;
      ADD_FAILURE() << "Digest mismatch for query: " << query.id << "\n"
                    << "  Expected: " << query.expectedDigest << "\n"
                    << "  Actual:   " << actualDigest << "\n"
                    << "  Query: " << query.file;
    }
  }

  // Overall pass rate
  int totalQueries = passCount + failCount;
  double passRate = totalQueries > 0
                        ? static_cast<double>(passCount) / totalQueries * 100.0
                        : 0.0;

  std::cout << "\n=== Golden Corpus Validation Summary ===" << std::endl;
  std::cout << "Total queries: " << totalQueries << std::endl;
  std::cout << "Passed: " << passCount << std::endl;
  std::cout << "Failed: " << failCount << std::endl;
  std::cout << "Pass rate: " << passRate << "%" << std::endl;

  // Fail build if ANY query diverges
  ASSERT_EQ(failCount, 0) << "Golden corpus validation failed";
}

// Helper test to compute baseline digests
// Run this once with a reference QLever instance to generate manifest digests
TEST_F(GoldenCorpusTest, DISABLED_ComputeBaselineDigests) {
  std::cout << "\n=== Computing Baseline Digests ===" << std::endl;

  for (const auto& query : queries_) {
    std::string sparqlQuery = loadQueryFile(query.file);
    std::string canonicalResult =
        executeQueryAndGetCanonicalResult(sparqlQuery);
    std::string digest = computeDigest(canonicalResult);

    std::cout << "Query: " << query.id << std::endl;
    std::cout << "  File: " << query.file << std::endl;
    std::cout << "  Digest: " << digest << std::endl;
  }

  std::cout << "\nNOTE: Update manifest.json with these digests" << std::endl;
}

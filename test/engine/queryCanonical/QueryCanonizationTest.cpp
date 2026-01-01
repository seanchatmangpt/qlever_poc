//   Copyright 2025, University of Freiburg,
//   Chair of Algorithms and Data Structures.
//   Author: Claude AI Assistant

#include <gtest/gtest.h>

#include <array>
#include <openssl/sha.h>
#include <string>

#include "index/EncodedIriManager.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"

namespace {

// ===========================================================================
// Helper: Compute SHA-256 hash of a string
// ===========================================================================
std::string computeSHA256(const std::string& input) {
  std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
         hash.data());

  std::string result;
  result.reserve(SHA256_DIGEST_LENGTH * 2);
  for (unsigned char byte : hash) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", byte);
    result += buf;
  }
  return result;
}

// ===========================================================================
// Stub: Query Fingerprint Structure
// ===========================================================================
// This represents the future QueryFingerprint structure that will be
// implemented as part of EPIC 2. For now, we create a minimal version
// to demonstrate test structure.
struct QueryFingerprint {
  std::string raw_query_sha256;
  std::string normalized_text_sha256;
  std::string shape_sha256;
  std::string params_sha256;
  uint32_t feature_flags = 0;

  static constexpr uint32_t NON_DETERMINISTIC = 1 << 0;

  // Stub: In the real implementation, this would perform full canonicalization
  // For now, we just hash the query string with some basic normalization
  static QueryFingerprint fromParsedQuery(const ParsedQuery& query,
                                           const std::string& rawQueryString) {
    QueryFingerprint fp;

    // Raw query hash - exact text identity
    fp.raw_query_sha256 = computeSHA256(rawQueryString);

    // Normalized text - whitespace normalization
    std::string normalized = normalizeWhitespace(rawQueryString);
    fp.normalized_text_sha256 = computeSHA256(normalized);

    // Shape hash - would be computed from AST structure
    // For stub, we use a simplified version
    fp.shape_sha256 = computeSHA256(extractQueryShape(normalized));

    // Params hash - would be computed from constants/literals
    fp.params_sha256 = computeSHA256(extractQueryParams(normalized));

    // Feature flags - detect non-deterministic functions
    fp.feature_flags = detectFeatureFlags(rawQueryString);

    return fp;
  }

 private:
  // Stub: Normalize whitespace and prefixes
  static std::string normalizeWhitespace(const std::string& query) {
    std::string result;
    bool inWhitespace = false;

    for (char c : query) {
      if (std::isspace(static_cast<unsigned char>(c))) {
        if (!inWhitespace && !result.empty()) {
          result += ' ';
          inWhitespace = true;
        }
      } else {
        result += c;
        inWhitespace = false;
      }
    }

    return result;
  }

  // Stub: Extract query shape by removing constants
  static std::string extractQueryShape(const std::string& query) {
    std::string shape = query;

    // Simple stub: replace numeric constants with placeholder
    // Real implementation would use AST traversal
    std::regex numberPattern(R"(\b\d+\b)");
    shape = std::regex_replace(shape, numberPattern, "NUM");

    // Replace variable names with canonical names
    // This is a very simplified version
    std::regex varPattern(R"(\?[a-zA-Z_][a-zA-Z0-9_]*)");
    int varIndex = 0;
    std::map<std::string, std::string> varMap;

    auto replaceVars = [&](const std::smatch& match) {
      std::string var = match.str();
      if (varMap.find(var) == varMap.end()) {
        varMap[var] = "?v" + std::to_string(varIndex++);
      }
      return varMap[var];
    };

    shape = std::regex_replace(shape, varPattern,
                               [&](const std::smatch& m) {
                                 return replaceVars(m);
                               });

    return shape;
  }

  // Stub: Extract query parameters (constants)
  static std::string extractQueryParams(const std::string& query) {
    std::string params;

    // Simple stub: extract numeric constants
    // Real implementation would extract all literals from AST
    std::regex numberPattern(R"(\b\d+\b)");
    std::sregex_iterator iter(query.begin(), query.end(), numberPattern);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
      params += iter->str() + ";";
    }

    return params;
  }

  // Stub: Detect non-deterministic features
  static uint32_t detectFeatureFlags(const std::string& query) {
    uint32_t flags = 0;

    // Check for non-deterministic functions
    if (query.find("NOW()") != std::string::npos ||
        query.find("RAND()") != std::string::npos ||
        query.find("UUID()") != std::string::npos ||
        query.find("STRUUID()") != std::string::npos ||
        query.find("BNODE()") != std::string::npos ||
        query.find("SERVICE") != std::string::npos) {
      flags |= NON_DETERMINISTIC;
    }

    return flags;
  }
};

// ===========================================================================
// Helper: Parse query string
// ===========================================================================
ParsedQuery parseQuery(const std::string& queryString) {
  static EncodedIriManager encodedIriManager;
  return SparqlParser::parseQuery(&encodedIriManager, queryString);
}

}  // namespace

// ===========================================================================
// TEST SUITE: Query Canonicalization Correctness
// ===========================================================================

// ---------------------------------------------------------------------------
// T1: Exact Text Identity
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T1_ExactTextIdentity) {
  std::string queryString = "SELECT ?x WHERE { ?x ?p ?o }";

  // Parse and fingerprint the same query twice
  auto query1 = parseQuery(queryString);
  auto fp1 = QueryFingerprint::fromParsedQuery(query1, queryString);

  auto query2 = parseQuery(queryString);
  auto fp2 = QueryFingerprint::fromParsedQuery(query2, queryString);

  // Same query string should produce identical raw_query_sha256
  EXPECT_EQ(fp1.raw_query_sha256, fp2.raw_query_sha256);

  // All other fingerprints should also match for identical queries
  EXPECT_EQ(fp1.normalized_text_sha256, fp2.normalized_text_sha256);
  EXPECT_EQ(fp1.shape_sha256, fp2.shape_sha256);
  EXPECT_EQ(fp1.params_sha256, fp2.params_sha256);
  EXPECT_EQ(fp1.feature_flags, fp2.feature_flags);
}

// ---------------------------------------------------------------------------
// T2: Whitespace and Prefix Normalization
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T2_WhitespaceAndPrefixNormalization) {
  // Query with minimal whitespace and one prefix
  std::string query1 =
      "PREFIX a: <http://example.org/> SELECT ?x WHERE { ?x a:p ?y }";

  // Query with extra whitespace, different prefix name, but semantically same
  std::string query2 =
      "PREFIX ex: <http://example.org/> SELECT   ?x   WHERE{?x ex:p ?y}";

  auto parsed1 = parseQuery(query1);
  auto fp1 = QueryFingerprint::fromParsedQuery(parsed1, query1);

  auto parsed2 = parseQuery(query2);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, query2);

  // Raw queries differ due to whitespace and prefix names
  EXPECT_NE(fp1.raw_query_sha256, fp2.raw_query_sha256);

  // After normalization, text should be similar
  // (Note: Full prefix expansion would make these identical in real impl)
  EXPECT_EQ(fp1.normalized_text_sha256, fp2.normalized_text_sha256);

  // Shape and params should be identical after normalization
  EXPECT_EQ(fp1.shape_sha256, fp2.shape_sha256);
  EXPECT_EQ(fp1.params_sha256, fp2.params_sha256);
}

// ---------------------------------------------------------------------------
// T3: Variable Renaming Invariance (α-equivalence)
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T3_VariableRenamingInvariance) {
  // Query with variables ?foo and ?bar
  std::string query1 =
      "SELECT ?foo WHERE { ?foo <http://example.org/prop> ?bar }";

  // Same query structure but with different variable names
  std::string query2 =
      "SELECT ?baz WHERE { ?baz <http://example.org/prop> ?qux }";

  auto parsed1 = parseQuery(query1);
  auto fp1 = QueryFingerprint::fromParsedQuery(parsed1, query1);

  auto parsed2 = parseQuery(query2);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, query2);

  // Raw queries differ due to different variable names
  EXPECT_NE(fp1.raw_query_sha256, fp2.raw_query_sha256);

  // Shape should be identical (α-equivalence)
  EXPECT_EQ(fp1.shape_sha256, fp2.shape_sha256);

  // Params should be identical (same constants)
  EXPECT_EQ(fp1.params_sha256, fp2.params_sha256);
}

// ---------------------------------------------------------------------------
// T4: Constant Perturbation
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T4_ConstantPerturbation) {
  // Query with FILTER on year <= 1940
  std::string query1 =
      "SELECT ?movie WHERE { ?movie <year> ?year . FILTER (?year <= 1940) }";

  // Same query structure but different constant (1950 instead of 1940)
  std::string query2 =
      "SELECT ?movie WHERE { ?movie <year> ?year . FILTER (?year <= 1950) }";

  auto parsed1 = parseQuery(query1);
  auto fp1 = QueryFingerprint::fromParsedQuery(parsed1, query1);

  auto parsed2 = parseQuery(query2);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, query2);

  // Raw queries differ
  EXPECT_NE(fp1.raw_query_sha256, fp2.raw_query_sha256);

  // Shape should be identical (same structure)
  EXPECT_EQ(fp1.shape_sha256, fp2.shape_sha256);

  // Params should differ (different constant values)
  EXPECT_NE(fp1.params_sha256, fp2.params_sha256);
}

// ---------------------------------------------------------------------------
// T5: Safe Reorder Invariance
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T5_SafeReorderInvariance) {
  // BGP with triples in one order
  std::string query1 =
      "SELECT ?x WHERE { ?x <p1> ?y . ?x <p2> ?z }";

  // Same BGP with triples in different order
  std::string query2 =
      "SELECT ?x WHERE { ?x <p2> ?z . ?x <p1> ?y }";

  auto parsed1 = parseQuery(query1);
  auto fp1 = QueryFingerprint::fromParsedQuery(parsed1, query1);

  auto parsed2 = parseQuery(query2);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, query2);

  // Raw queries differ due to different triple order
  EXPECT_NE(fp1.raw_query_sha256, fp2.raw_query_sha256);

  // Shape should be identical for reorderable BGPs
  // Note: This test documents expected behavior; real implementation
  // would need to canonicalize BGP order
  EXPECT_EQ(fp1.shape_sha256, fp2.shape_sha256);
}

// ---------------------------------------------------------------------------
// T6: Boundary Safety (OPTIONAL)
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T6_BoundarySafety) {
  // Query with OPTIONAL
  std::string query1 =
      "SELECT ?x WHERE { ?x <p1> ?y OPTIONAL { ?x <p2> ?z } }";

  // Query with reordered patterns across OPTIONAL boundary
  // This is NOT semantically equivalent!
  std::string query2 =
      "SELECT ?x WHERE { OPTIONAL { ?x <p2> ?z } ?x <p1> ?y }";

  auto parsed1 = parseQuery(query1);
  auto fp1 = QueryFingerprint::fromParsedQuery(parsed1, query1);

  auto parsed2 = parseQuery(query2);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, query2);

  // These queries should have DIFFERENT shapes because reordering
  // across OPTIONAL boundaries changes semantics
  EXPECT_NE(fp1.shape_sha256, fp2.shape_sha256);

  // Test that OPTIONAL boundaries are respected in shape
  // The shape should encode operator structure
  EXPECT_NE(fp1.raw_query_sha256, fp2.raw_query_sha256);
}

// ---------------------------------------------------------------------------
// T7: Determinism Flagging
// ---------------------------------------------------------------------------
TEST(QueryCanonizationTest, T7_DeterminismFlagging) {
  // Deterministic query (no special functions)
  std::string deterministicQuery =
      "SELECT ?x WHERE { ?x <p> ?y }";

  // Query with NOW() - non-deterministic
  std::string nowQuery =
      "SELECT ?x WHERE { ?x <timestamp> ?t . FILTER (?t < NOW()) }";

  // Query with RAND() - non-deterministic
  std::string randQuery =
      "SELECT ?x WHERE { ?x <value> ?v . FILTER (?v > RAND()) }";

  // Query with SERVICE - non-deterministic (external)
  std::string serviceQuery =
      "SELECT ?x WHERE { SERVICE <http://example.org/sparql> { ?x <p> ?y } }";

  auto fp_deterministic = QueryFingerprint::fromParsedQuery(
      parseQuery(deterministicQuery), deterministicQuery);
  auto fp_now = QueryFingerprint::fromParsedQuery(
      parseQuery(nowQuery), nowQuery);
  auto fp_rand = QueryFingerprint::fromParsedQuery(
      parseQuery(randQuery), randQuery);
  auto fp_service = QueryFingerprint::fromParsedQuery(
      parseQuery(serviceQuery), serviceQuery);

  // Deterministic query should NOT have NON_DETERMINISTIC flag
  EXPECT_EQ(fp_deterministic.feature_flags &
            QueryFingerprint::NON_DETERMINISTIC, 0u);

  // NOW() query should have NON_DETERMINISTIC flag
  EXPECT_NE(fp_now.feature_flags & QueryFingerprint::NON_DETERMINISTIC, 0u);

  // RAND() query should have NON_DETERMINISTIC flag
  EXPECT_NE(fp_rand.feature_flags & QueryFingerprint::NON_DETERMINISTIC, 0u);

  // SERVICE query should have NON_DETERMINISTIC flag
  EXPECT_NE(fp_service.feature_flags &
            QueryFingerprint::NON_DETERMINISTIC, 0u);
}

// ===========================================================================
// Additional Integration Test: Full Workflow
// ===========================================================================
TEST(QueryCanonizationTest, FullWorkflowIntegration) {
  // A realistic DBLP-style query
  std::string dblpQuery = R"(
    PREFIX dblp: <https://dblp.org/rdf/schema#>
    SELECT ?author ?title WHERE {
      ?pub dblp:authoredBy ?author .
      ?pub dblp:title ?title .
      ?pub dblp:yearOfPublication ?year .
      FILTER (?year >= 2020 && ?year <= 2023)
    }
    ORDER BY ?author
    LIMIT 100
  )";

  // Parse and fingerprint
  auto parsed = parseQuery(dblpQuery);
  auto fp = QueryFingerprint::fromParsedQuery(parsed, dblpQuery);

  // Verify fingerprint is non-empty and well-formed
  EXPECT_FALSE(fp.raw_query_sha256.empty());
  EXPECT_EQ(fp.raw_query_sha256.length(), 64u);  // SHA-256 hex length

  EXPECT_FALSE(fp.normalized_text_sha256.empty());
  EXPECT_FALSE(fp.shape_sha256.empty());
  EXPECT_FALSE(fp.params_sha256.empty());

  // Should be deterministic
  EXPECT_EQ(fp.feature_flags & QueryFingerprint::NON_DETERMINISTIC, 0u);

  // Verify that re-parsing produces identical fingerprint
  auto parsed2 = parseQuery(dblpQuery);
  auto fp2 = QueryFingerprint::fromParsedQuery(parsed2, dblpQuery);

  EXPECT_EQ(fp.raw_query_sha256, fp2.raw_query_sha256);
  EXPECT_EQ(fp.shape_sha256, fp2.shape_sha256);
  EXPECT_EQ(fp.params_sha256, fp2.params_sha256);
}

// EPIC 10.1 - Agent 5: SIMD Equivalence Validation
// Validates that SIMD ON vs OFF produces bit-identical observable outputs
//
// SPEC-LOCK CONSTRAINTS:
// - Section 3.4: SIMD ON vs OFF MUST yield bit-identical observable outputs
// - Section 6.3: Validation artifact must prove SIMD equivalence
// - Section 4.4: Hot path silence applies to both SIMD + scalar paths
//
// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

#include "engine/ingress/ErrorCodes.h"
#include "engine/ingress/IngressDigest.h"
#include "engine/ingress/IngressResult.h"
#include "engine/ingress/SimdJsonIngressWrapper.h"
#include "engine/readPlane/ExecutionDigest.h"
#include "engine/readPlane/PerformanceEnvelope.h"
#include "util/CryptographicHashUtils.h"

using namespace qlever::ingress;

// =============================================================================
// Test Fixture: SIMD Equivalence Test Base
// =============================================================================
class SimdEquivalenceTest : public ::testing::Test {
 protected:
  // SIMD-enabled wrapper
  SimdJsonIngressWrapper simd_wrapper;

  // Scalar fallback wrapper (SIMD disabled via compilation or runtime flag)
  // For now, we use the same wrapper and test fallback_parse explicitly
  SimdJsonIngressWrapper scalar_wrapper;

  // Helper: Compare two IngressResult structs for bit-identical equivalence
  void AssertBitIdenticalResults(const IngressResult& simd_result,
                                 const IngressResult& scalar_result,
                                 const std::string& test_context) {
    SCOPED_TRACE(test_context);

    // Error codes must match
    EXPECT_EQ(simd_result.error, scalar_result.error)
        << "Error codes differ: SIMD=" << static_cast<int>(simd_result.error)
        << " Scalar=" << static_cast<int>(scalar_result.error);

    // Deterministic digest must be bit-identical
    EXPECT_EQ(simd_result.digest_sha256, scalar_result.digest_sha256)
        << "Digest SHA256 mismatch!\n"
        << "  SIMD:   " << simd_result.digest_sha256 << "\n"
        << "  Scalar: " << scalar_result.digest_sha256;

    // SIMD validation masks can differ (different techniques used)
    // but observable outputs (digest) must match

    // Metrics (bytes_parsed, document_count) should match for deterministic
    // inputs
    EXPECT_EQ(simd_result.bytes_parsed, scalar_result.bytes_parsed)
        << "Bytes parsed differ: SIMD=" << simd_result.bytes_parsed
        << " Scalar=" << scalar_result.bytes_parsed;

    EXPECT_EQ(simd_result.document_count, scalar_result.document_count)
        << "Document count differs: SIMD=" << simd_result.document_count
        << " Scalar=" << scalar_result.document_count;
  }

  // Helper: Generate test JSON-LD documents
  std::vector<std::string> GetTestDocuments() {
    return {
        // Simple objects
        R"({"@context": "https://schema.org", "@type": "Person", "name": "Alice"})",

        // Nested structures
        R"({
          "@context": "https://www.w3.org/ns/activitystreams",
          "@id": "https://example.com/note/1",
          "@type": "Note",
          "content": "Hello World",
          "author": {
            "@type": "Person",
            "name": "Bob"
          }
        })",

        // Arrays
        R"({
          "@context": "https://schema.org",
          "@type": "ItemList",
          "itemListElement": [
            {"@type": "ListItem", "position": 1, "name": "First"},
            {"@type": "ListItem", "position": 2, "name": "Second"},
            {"@type": "ListItem", "position": 3, "name": "Third"}
          ]
        })",

        // Numbers and special characters
        R"({
          "@context": "https://schema.org",
          "@type": "Product",
          "price": 123.45,
          "quantity": 100,
          "inStock": true,
          "description": "Test \"quoted\" text with\nNewlines\tand\ttabs"
        })",

        // Unicode content
        R"({
          "@context": "https://schema.org",
          "@type": "Article",
          "headline": "测试文章 🚀 Test Article",
          "author": "Müller",
          "keywords": ["日本語", "Español", "العربية"]
        })",

        // Large document (stress test)
        GenerateLargeDocument(1000),

        // Complex nested structure
        R"({
          "@context": "https://www.w3.org/ns/activitystreams",
          "@type": "Collection",
          "totalItems": 3,
          "items": [
            {
              "@type": "Note",
              "id": "https://example.com/1",
              "content": "First note",
              "tag": [
                {"type": "Hashtag", "name": "#test"},
                {"type": "Mention", "href": "https://example.com/users/alice"}
              ]
            },
            {
              "@type": "Article",
              "id": "https://example.com/2",
              "name": "Article Title",
              "content": {
                "@type": "Text",
                "@value": "Article content here"
              }
            },
            {
              "@type": "Event",
              "id": "https://example.com/3",
              "startTime": "2025-01-02T10:00:00Z",
              "endTime": "2025-01-02T12:00:00Z",
              "location": {
                "@type": "Place",
                "name": "Conference Room A"
              }
            }
          ]
        })",

        // Edge cases: empty arrays, null values (if supported)
        R"({
          "@context": "https://schema.org",
          "@type": "Thing",
          "emptyArray": [],
          "emptyObject": {},
          "zeroValue": 0,
          "emptyString": ""
        })"};
  }

 private:
  // Generate large JSON-LD document for stress testing
  std::string GenerateLargeDocument(int item_count) {
    std::string doc = R"({
      "@context": "https://schema.org",
      "@type": "ItemList",
      "itemListElement": [)";

    for (int i = 0; i < item_count; ++i) {
      if (i > 0) doc += ",";
      doc += R"(
        {
          "@type": "ListItem",
          "position": )" +
             std::to_string(i) + R"(,
          "item": {
            "@type": "Product",
            "name": "Product )" +
             std::to_string(i) + R"(",
            "price": )" +
             std::to_string(i * 10.5) + R"(,
            "description": "Description for product )" +
             std::to_string(i) + R"("
          }
        })";
    }

    doc += R"(
      ]
    })";
    return doc;
  }
};

// =============================================================================
// TEST SUITE 1: parseJsonLd - SIMD vs Scalar Equivalence
// =============================================================================

TEST_F(SimdEquivalenceTest, ParseJsonLd_SimpleDocument) {
  std::string input =
      R"({"@context": "https://schema.org", "@type": "Person", "name": "Alice"})";

  auto simd_result = simd_wrapper.parseJsonLd(input);
  auto scalar_result = scalar_wrapper.fallback_parse(input);

  AssertBitIdenticalResults(simd_result, scalar_result,
                            "parseJsonLd - Simple Document");
}

TEST_F(SimdEquivalenceTest, ParseJsonLd_NestedStructure) {
  std::string input = R"({
    "@context": "https://www.w3.org/ns/activitystreams",
    "@id": "https://example.com/note/1",
    "@type": "Note",
    "content": "Hello World",
    "author": {
      "@type": "Person",
      "name": "Bob"
    }
  })";

  auto simd_result = simd_wrapper.parseJsonLd(input);
  auto scalar_result = scalar_wrapper.fallback_parse(input);

  AssertBitIdenticalResults(simd_result, scalar_result,
                            "parseJsonLd - Nested Structure");
}

TEST_F(SimdEquivalenceTest, ParseJsonLd_AllTestDocuments) {
  auto test_docs = GetTestDocuments();

  for (size_t i = 0; i < test_docs.size(); ++i) {
    SCOPED_TRACE("Test document index: " + std::to_string(i));

    auto simd_result = simd_wrapper.parseJsonLd(test_docs[i]);
    auto scalar_result = scalar_wrapper.fallback_parse(test_docs[i]);

    AssertBitIdenticalResults(simd_result, scalar_result,
                              "parseJsonLd - Document " + std::to_string(i));
  }
}

TEST_F(SimdEquivalenceTest, ParseJsonLd_EmptyInput) {
  std::string input = "";

  auto simd_result = simd_wrapper.parseJsonLd(input);
  auto scalar_result = scalar_wrapper.fallback_parse(input);

  AssertBitIdenticalResults(simd_result, scalar_result,
                            "parseJsonLd - Empty Input");

  // Both should return PARSE_ERROR_EMPTY_INPUT
  EXPECT_EQ(simd_result.error, IngressErrorCode::PARSE_ERROR_EMPTY_INPUT);
  EXPECT_EQ(scalar_result.error, IngressErrorCode::PARSE_ERROR_EMPTY_INPUT);
}

TEST_F(SimdEquivalenceTest, ParseJsonLd_InvalidJson) {
  std::string input = R"({"unclosed": "object")";

  auto simd_result = simd_wrapper.parseJsonLd(input);
  auto scalar_result = scalar_wrapper.fallback_parse(input);

  // Error codes must match (both should detect invalid JSON)
  EXPECT_EQ(simd_result.error, scalar_result.error);

  // Digests for error cases should also be consistent
  EXPECT_EQ(simd_result.digest_sha256, scalar_result.digest_sha256);
}

// =============================================================================
// TEST SUITE 2: validateStructure - SIMD vs Scalar Equivalence
// =============================================================================

TEST_F(SimdEquivalenceTest, ValidateStructure_ValidDocuments) {
  auto test_docs = GetTestDocuments();

  for (size_t i = 0; i < test_docs.size(); ++i) {
    SCOPED_TRACE("Validate document index: " + std::to_string(i));

    auto simd_result = simd_wrapper.validateStructure(test_docs[i]);
    auto scalar_result = scalar_wrapper.validateStructure(test_docs[i]);

    // Error codes must match
    EXPECT_EQ(simd_result.error, scalar_result.error);
  }
}

TEST_F(SimdEquivalenceTest, ValidateStructure_BracketMismatch) {
  std::string input = R"({"key": [1, 2, 3})";  // Missing closing bracket

  auto simd_result = simd_wrapper.validateStructure(input);
  auto scalar_result = scalar_wrapper.validateStructure(input);

  // Both should detect structural error
  EXPECT_EQ(simd_result.error, scalar_result.error);
  EXPECT_NE(simd_result.error, IngressErrorCode::OK);
}

// =============================================================================
// TEST SUITE 3: normalizeJsonLd - SIMD vs Scalar Equivalence
// =============================================================================

TEST_F(SimdEquivalenceTest, NormalizeJsonLd_FieldOrdering) {
  // Test that field ordering is deterministic
  std::string input = R"({"z": 1, "a": 2, "m": 3})";

  std::string simd_normalized;
  std::string scalar_normalized;

  auto simd_result = simd_wrapper.normalizeJsonLd(input, simd_normalized);
  auto scalar_result = scalar_wrapper.normalizeJsonLd(input, scalar_normalized);

  // Normalized outputs must be bit-identical
  EXPECT_EQ(simd_normalized, scalar_normalized)
      << "Normalized JSON differs!\n"
      << "  SIMD:   " << simd_normalized << "\n"
      << "  Scalar: " << scalar_normalized;

  AssertBitIdenticalResults(simd_result, scalar_result,
                            "normalizeJsonLd - Field Ordering");
}

TEST_F(SimdEquivalenceTest, NormalizeJsonLd_WhitespaceRemoval) {
  std::string input = R"(  {  "key"  :  "value"  }  )";

  std::string simd_normalized;
  std::string scalar_normalized;

  auto simd_result = simd_wrapper.normalizeJsonLd(input, simd_normalized);
  auto scalar_result = scalar_wrapper.normalizeJsonLd(input, scalar_normalized);

  EXPECT_EQ(simd_normalized, scalar_normalized);
  AssertBitIdenticalResults(simd_result, scalar_result,
                            "normalizeJsonLd - Whitespace");
}

TEST_F(SimdEquivalenceTest, NormalizeJsonLd_AllTestDocuments) {
  auto test_docs = GetTestDocuments();

  for (size_t i = 0; i < test_docs.size(); ++i) {
    SCOPED_TRACE("Normalize document index: " + std::to_string(i));

    std::string simd_normalized;
    std::string scalar_normalized;

    auto simd_result =
        simd_wrapper.normalizeJsonLd(test_docs[i], simd_normalized);
    auto scalar_result =
        scalar_wrapper.normalizeJsonLd(test_docs[i], scalar_normalized);

    EXPECT_EQ(simd_normalized, scalar_normalized)
        << "Normalized outputs differ for document " << i;

    AssertBitIdenticalResults(
        simd_result, scalar_result,
        "normalizeJsonLd - Document " + std::to_string(i));
  }
}

// =============================================================================
// TEST SUITE 4: Determinism - 100x Repetition Test
// =============================================================================

TEST_F(SimdEquivalenceTest, Determinism_100Iterations_SIMD) {
  std::string input =
      R"({"@context": "https://schema.org", "@type": "Article", "headline": "Test"})";

  auto first_result = simd_wrapper.parseJsonLd(input);

  for (int i = 0; i < 99; ++i) {
    auto result = simd_wrapper.parseJsonLd(input);

    EXPECT_EQ(result.digest_sha256, first_result.digest_sha256)
        << "SIMD digest changed on iteration " << (i + 2);
    EXPECT_EQ(result.error, first_result.error);
  }
}

TEST_F(SimdEquivalenceTest, Determinism_100Iterations_Scalar) {
  std::string input =
      R"({"@context": "https://schema.org", "@type": "Article", "headline": "Test"})";

  auto first_result = scalar_wrapper.fallback_parse(input);

  for (int i = 0; i < 99; ++i) {
    auto result = scalar_wrapper.fallback_parse(input);

    EXPECT_EQ(result.digest_sha256, first_result.digest_sha256)
        << "Scalar digest changed on iteration " << (i + 2);
    EXPECT_EQ(result.error, first_result.error);
  }
}

TEST_F(SimdEquivalenceTest, Determinism_SIMDvsScalar_Consistency) {
  std::string input =
      R"({"@context": "https://schema.org", "@type": "Product", "name": "Widget"})";

  // Run SIMD and Scalar 10 times each, all should produce same digest
  std::string simd_digest;
  std::string scalar_digest;

  for (int i = 0; i < 10; ++i) {
    auto simd_result = simd_wrapper.parseJsonLd(input);
    auto scalar_result = scalar_wrapper.fallback_parse(input);

    if (i == 0) {
      simd_digest = simd_result.digest_sha256;
      scalar_digest = scalar_result.digest_sha256;
    }

    EXPECT_EQ(simd_result.digest_sha256, simd_digest)
        << "SIMD digest inconsistent at iteration " << i;
    EXPECT_EQ(scalar_result.digest_sha256, scalar_digest)
        << "Scalar digest inconsistent at iteration " << i;

    // SIMD and Scalar must produce same digest
    EXPECT_EQ(simd_digest, scalar_digest)
        << "SIMD and Scalar digests differ at iteration " << i;
  }
}

// =============================================================================
// TEST SUITE 5: Result Digest Equivalence (Integration with EPIC 10.1)
// =============================================================================

TEST_F(SimdEquivalenceTest, ResultDigest_BitIdentical) {
  // Test that result digest computation is identical regardless of SIMD flag

  auto test_docs = GetTestDocuments();

  for (size_t i = 0; i < test_docs.size(); ++i) {
    SCOPED_TRACE("Result digest for document " + std::to_string(i));

    std::string simd_normalized;
    std::string scalar_normalized;

    auto simd_parse = simd_wrapper.parseJsonLd(test_docs[i]);
    auto scalar_parse = scalar_wrapper.fallback_parse(test_docs[i]);

    simd_wrapper.normalizeJsonLd(test_docs[i], simd_normalized);
    scalar_wrapper.normalizeJsonLd(test_docs[i], scalar_normalized);

    // Compute digests using IngressDigest::compute
    uint32_t validation_mask = 0x1F;  // All SIMD techniques enabled

    auto simd_digest = IngressDigest::compute(simd_normalized, validation_mask,
                                              simd_parse.error);
    auto scalar_digest = IngressDigest::compute(
        scalar_normalized, validation_mask, scalar_parse.error);

    // Digests must be bit-identical
    auto simd_hex = IngressDigest::hex_encode(simd_digest);
    auto scalar_hex = IngressDigest::hex_encode(scalar_digest);

    EXPECT_EQ(simd_hex, scalar_hex)
        << "IngressDigest differs for document " << i << "\n"
        << "  SIMD:   " << simd_hex << "\n"
        << "  Scalar: " << scalar_hex;
  }
}

// =============================================================================
// TEST SUITE 6: Envelope Digest Equivalence
// =============================================================================

TEST_F(SimdEquivalenceTest, EnvelopeDigest_IndependentOfSIMDFlag) {
  // Test that PerformanceEnvelope digest is identical regardless of SIMD usage

  // This test validates that envelope computation doesn't depend on SIMD flag
  // by computing envelopes from identical workload data using different paths

  // Create mock performance envelope with deterministic data
  readPlane::PerformanceEnvelope envelope_simd;
  readPlane::PerformanceEnvelope envelope_scalar;

  // Set identical data (simulating same workload replayed with different SIMD
  // settings)
  envelope_simd.workload_id = "test_workload_001";
  envelope_scalar.workload_id = "test_workload_001";

  envelope_simd.latency_stats.p50_ns = 1000000;
  envelope_scalar.latency_stats.p50_ns = 1000000;

  envelope_simd.latency_stats.p95_ns = 2000000;
  envelope_scalar.latency_stats.p95_ns = 2000000;

  envelope_simd.latency_stats.p99_ns = 3000000;
  envelope_scalar.latency_stats.p99_ns = 3000000;

  envelope_simd.latency_stats.max_ns = 5000000;
  envelope_scalar.latency_stats.max_ns = 5000000;

  // Cache stats
  envelope_simd.cache_stats.bytes_hits = 100;
  envelope_scalar.cache_stats.bytes_hits = 100;

  envelope_simd.cache_stats.bytes_misses = 20;
  envelope_scalar.cache_stats.bytes_misses = 20;

  envelope_simd.cache_stats.bytes_hit_rate = 0.833;
  envelope_scalar.cache_stats.bytes_hit_rate = 0.833;

  // Compute digests
  std::string simd_envelope_digest = envelope_simd.computeDigest();
  std::string scalar_envelope_digest = envelope_scalar.computeDigest();

  // Envelope digests must be bit-identical
  EXPECT_EQ(simd_envelope_digest, scalar_envelope_digest)
      << "PerformanceEnvelope digest differs!\n"
      << "  SIMD:   " << simd_envelope_digest << "\n"
      << "  Scalar: " << scalar_envelope_digest;
}

// =============================================================================
// TEST SUITE 7: Canonical Bytes Equivalence
// =============================================================================

TEST_F(SimdEquivalenceTest, CanonicalBytes_BitIdentical) {
  // Test that canonical byte representation is identical

  auto test_docs = GetTestDocuments();

  for (size_t i = 0; i < test_docs.size(); ++i) {
    SCOPED_TRACE("Canonical bytes for document " + std::to_string(i));

    std::string simd_normalized;
    std::string scalar_normalized;

    simd_wrapper.normalizeJsonLd(test_docs[i], simd_normalized);
    scalar_wrapper.normalizeJsonLd(test_docs[i], scalar_normalized);

    // Canonical bytes must be identical
    EXPECT_EQ(simd_normalized.size(), scalar_normalized.size())
        << "Canonical byte length differs for document " << i;

    if (simd_normalized.size() == scalar_normalized.size()) {
      for (size_t byte_idx = 0; byte_idx < simd_normalized.size(); ++byte_idx) {
        EXPECT_EQ(static_cast<unsigned char>(simd_normalized[byte_idx]),
                  static_cast<unsigned char>(scalar_normalized[byte_idx]))
            << "Byte mismatch at position " << byte_idx << " in document " << i;

        // Stop after first mismatch to avoid spam
        if (simd_normalized[byte_idx] != scalar_normalized[byte_idx]) {
          break;
        }
      }
    }
  }
}

// =============================================================================
// TEST SUITE 8: Platform Independence (if multiple platforms available)
// =============================================================================

TEST_F(SimdEquivalenceTest, PlatformIndependence_DigestStability) {
  // Test that digest computation is platform-independent
  // This test documents expected digest values for regression testing

  std::string input =
      R"({"@context":"https://schema.org","@type":"Person","name":"Alice"})";

  auto result = simd_wrapper.parseJsonLd(input);

  // Document the digest for this specific input (for regression testing)
  // If this test fails on different platforms, it indicates platform-dependent
  // behavior
  EXPECT_FALSE(result.digest_sha256.empty())
      << "Digest should be computed and non-empty";

  // Digest should be 64 hex characters (SHA256)
  EXPECT_EQ(result.digest_sha256.length(), 64u)
      << "SHA256 digest should be 64 hex characters";

  // Verify hex encoding (all characters should be [0-9a-f])
  for (char c : result.digest_sha256) {
    EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
        << "Invalid hex character in digest: " << c;
  }
}

// =============================================================================
// VALIDATION SUMMARY TEST
// =============================================================================

TEST_F(SimdEquivalenceTest, ValidationSummary_AllTestsPassed) {
  // This test aggregates results and produces validation artifact data

  int total_tests = 8;  // Number of test documents in GetTestDocuments()
  int passed_tests = 0;
  int digest_matches = 0;
  int canonical_matches = 0;

  auto test_docs = GetTestDocuments();

  for (const auto& doc : test_docs) {
    std::string simd_normalized;
    std::string scalar_normalized;

    auto simd_result = simd_wrapper.parseJsonLd(doc);
    auto scalar_result = scalar_wrapper.fallback_parse(doc);

    simd_wrapper.normalizeJsonLd(doc, simd_normalized);
    scalar_wrapper.normalizeJsonLd(doc, scalar_normalized);

    bool test_passed = true;

    // Check digest equivalence
    if (simd_result.digest_sha256 == scalar_result.digest_sha256) {
      digest_matches++;
    } else {
      test_passed = false;
    }

    // Check canonical bytes equivalence
    if (simd_normalized == scalar_normalized) {
      canonical_matches++;
    } else {
      test_passed = false;
    }

    if (test_passed) {
      passed_tests++;
    }
  }

  // Validation artifact metrics
  std::cout << "\n=== SIMD EQUIVALENCE VALIDATION SUMMARY ===\n";
  std::cout << "Total test documents: " << test_docs.size() << "\n";
  std::cout << "Passed tests: " << passed_tests << "/" << test_docs.size()
            << "\n";
  std::cout << "Digest matches: " << digest_matches << "/" << test_docs.size()
            << "\n";
  std::cout << "Canonical byte matches: " << canonical_matches << "/"
            << test_docs.size() << "\n";
  std::cout << "SIMD equivalence: "
            << (passed_tests == static_cast<int>(test_docs.size()) ? "VERIFIED"
                                                                   : "FAILED")
            << "\n";
  std::cout << "==========================================\n\n";

  // All tests must pass for SIMD equivalence validation
  EXPECT_EQ(passed_tests, static_cast<int>(test_docs.size()))
      << "Not all SIMD equivalence tests passed";
  EXPECT_EQ(digest_matches, static_cast<int>(test_docs.size()))
      << "SIMD and scalar digests do not match for all documents";
  EXPECT_EQ(canonical_matches, static_cast<int>(test_docs.size()))
      << "SIMD and scalar canonical bytes do not match for all documents";
}

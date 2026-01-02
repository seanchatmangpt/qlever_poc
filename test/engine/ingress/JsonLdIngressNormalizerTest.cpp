// EPIC 10.1: JSON-LD Ingress Normalizer Tests
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 6 - JSON-LD Ingress Normalization
//
// VALIDATION ARTIFACTS:
// - Test determinism: same input → same digest 10+ times
// - Test dialect rejection: Turtle, N-Triples, RDF/XML rejected at ingress
// - Test guard enforcement: oversized input → fail-closed
// - Test epoch binding: prevents cross-epoch confusion

#include <gtest/gtest.h>

#include "engine/ingress/JsonLdIngressNormalizer.h"
#include "global/Epoch.h"

namespace qlever::ingress {
namespace {

using namespace ad_utility;

// Test fixture with epoch manager
class JsonLdIngressNormalizerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize epoch manager to INGEST state
    epochManager_.transitionToIngest();
  }

  void TearDown() override {
    // Reset epoch manager
    epochManager_.restart();
  }

  EpochManager epochManager_;
};

// ========================================================================
// TEST SUITE 1: DIALECT DETECTION AND REJECTION
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, DetectJsonLdFormat) {
  // Valid JSON-LD input
  std::string_view json_ld = R"({
    "@context": "http://schema.org",
    "@type": "Person",
    "name": "Alice"
  })";

  auto result = JsonLdIngressNormalizer::detectDialect(json_ld);

  EXPECT_TRUE(result.is_json_ld);
  EXPECT_FALSE(result.is_turtle);
  EXPECT_FALSE(result.is_ntriples);
  EXPECT_FALSE(result.is_rdfxml);
  EXPECT_EQ(result.error, IngressErrorCode::OK);
}

TEST_F(JsonLdIngressNormalizerTest, RejectTurtleFormat) {
  // Turtle input (starts with @prefix)
  std::string_view turtle = R"(@prefix ex: <http://example.org/> .
    ex:alice ex:name "Alice" .)";

  auto result = JsonLdIngressNormalizer::detectDialect(turtle);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_turtle);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
}

TEST_F(JsonLdIngressNormalizerTest, RejectNTriplesFormat) {
  // N-Triples input
  std::string_view ntriples =
      R"(<http://example.org/alice> <http://example.org/name> "Alice" .)";

  auto result = JsonLdIngressNormalizer::detectDialect(ntriples);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_ntriples);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
}

TEST_F(JsonLdIngressNormalizerTest, RejectRdfXmlFormat) {
  // RDF/XML input
  std::string_view rdfxml = R"(<?xml version="1.0"?>
    <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
      <rdf:Description rdf:about="http://example.org/alice">
        <name>Alice</name>
      </rdf:Description>
    </rdf:RDF>)";

  auto result = JsonLdIngressNormalizer::detectDialect(rdfxml);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_rdfxml);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
}

TEST_F(JsonLdIngressNormalizerTest, RejectEmptyInput) {
  std::string_view empty = "";

  auto result = JsonLdIngressNormalizer::detectDialect(empty);

  EXPECT_EQ(result.error, IngressErrorCode::PARSE_ERROR_EMPTY_INPUT);
}

// ========================================================================
// TEST SUITE 2: DETERMINISTIC NORMALIZATION
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, DeterministicNormalizationShaclJsonLd) {
  // SHACL JSON-LD input
  std::string_view shacl_json_ld = R"({
    "@context": {
      "sh": "http://www.w3.org/ns/shacl#",
      "ex": "http://example.org/"
    },
    "@type": "sh:NodeShape",
    "sh:targetClass": "ex:Person",
    "sh:property": {
      "@type": "sh:PropertyShape",
      "sh:path": "ex:name",
      "sh:minCount": 1
    }
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  IngressGuardConfig guards;

  // Run normalization 10 times
  std::vector<std::string> digests;
  for (int i = 0; i < 10; ++i) {
    std::string normalized;
    auto result = normalizer.normalizeForDialect(
        shacl_json_ld, RuleLanguageDialect::SHACL, token, guards, normalized);

    ASSERT_EQ(result.error, IngressErrorCode::OK);
    digests.push_back(result.digest_sha256);
  }

  // All digests must be identical (determinism)
  for (size_t i = 1; i < digests.size(); ++i) {
    EXPECT_EQ(digests[0], digests[i])
        << "Digest mismatch at iteration " << i << ": "
        << "expected " << digests[0] << ", got " << digests[i];
  }
}

TEST_F(JsonLdIngressNormalizerTest, DeterministicNormalizationDatalogJsonLd) {
  // Datalog JSON-LD input
  std::string_view datalog_json_ld = R"({
    "@context": "http://example.org/datalog",
    "rules": [
      {
        "head": "ancestor(?x, ?y)",
        "body": ["parent(?x, ?y)"]
      }
    ]
})";

  JsonLdIngressNormalizer normalizer;
auto token = epochManager_.getIngressCapabilityToken();
IngressGuardConfig guards;

// Use verifyDeterminism helper (100 iterations)
bool is_deterministic = JsonLdIngressNormalizer::verifyDeterminism(
    datalog_json_ld, RuleLanguageDialect::DATALOG, token, guards, 100);

EXPECT_TRUE(is_deterministic);
}

TEST_F(JsonLdIngressNormalizerTest, KeyOrderingIsDeterministic) {
  // Same JSON-LD with different key ordering
  std::string_view json_ld_1 = R"({
    "@context": "http://schema.org",
    "name": "Alice",
    "@type": "Person"
  })";

  std::string_view json_ld_2 = R"({
    "@type": "Person",
    "@context": "http://schema.org",
    "name": "Alice"
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  IngressGuardConfig guards;

  std::string normalized_1, normalized_2;
  auto result_1 = normalizer.normalizeForDialect(
      json_ld_1, RuleLanguageDialect::DATALOG, token, guards, normalized_1);

  auto result_2 = normalizer.normalizeForDialect(
      json_ld_2, RuleLanguageDialect::DATALOG, token, guards, normalized_2);

  ASSERT_EQ(result_1.error, IngressErrorCode::OK);
  ASSERT_EQ(result_2.error, IngressErrorCode::OK);

  // Normalized output should be identical (alphabetical key ordering)
  EXPECT_EQ(normalized_1, normalized_2);

  // Digests should also be identical
  EXPECT_EQ(result_1.digest_sha256, result_2.digest_sha256);
}

// ========================================================================
// TEST SUITE 3: GUARD ENFORCEMENT (BOUNDED COMPUTE)
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, EnforceMaxInputSize) {
  // Create oversized input (exceeds guard limit)
  std::string large_input = "{\"data\":\"";
  large_input.append(10 * 1024 * 1024, 'x');  // 10MB of 'x'
  large_input += "\"}";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  IngressGuardConfig guards;
  guards.max_input_size_bytes = 1024 * 1024;  // 1MB limit

  std::string normalized;
  auto result = normalizer.normalizeForDialect(
      large_input, RuleLanguageDialect::DATALOG, token, guards, normalized);

  // Should fail with BUFFER_OVERFLOW
  EXPECT_EQ(result.error, IngressErrorCode::BUFFER_OVERFLOW);
}

TEST_F(JsonLdIngressNormalizerTest, AcceptInputWithinGuards) {
  // Small input (within guard limits)
  std::string_view small_input = R"({
    "@context": "http://example.org",
    "rules": [{"head": "test(?x)", "body": ["data(?x)"]}]
})";

  JsonLdIngressNormalizer normalizer;
auto token = epochManager_.getIngressCapabilityToken();
IngressGuardConfig guards;
guards.max_input_size_bytes = 1024;  // 1KB limit (sufficient)

std::string normalized;
auto result = normalizer.normalizeForDialect(
    small_input, RuleLanguageDialect::DATALOG, token, guards, normalized);

EXPECT_EQ(result.error, IngressErrorCode::OK);
}

// ========================================================================
// TEST SUITE 4: EPOCH BINDING
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, DigestBindsToEpochId) {
  std::string_view json_ld = R"({
    "@context": "http://schema.org",
    "rules": [{"head": "test(?x)", "body": ["data(?x)"]}]
})";

  JsonLdIngressNormalizer normalizer;
IngressGuardConfig guards;

// Get token for epoch 0
auto token_epoch_0 = epochManager_.getIngressCapabilityToken();
EpochId epoch_0 = token_epoch_0.getEpochId();

std::string normalized_epoch_0;
auto result_epoch_0 =
    normalizer.normalizeForDialect(json_ld, RuleLanguageDialect::DATALOG,
                                   token_epoch_0, guards, normalized_epoch_0);

ASSERT_EQ(result_epoch_0.error, IngressErrorCode::OK);
std::string digest_epoch_0 = result_epoch_0.digest_sha256;

// Transition to new epoch
epochManager_.transitionToSeal();
epochManager_.transitionToServe();
epochManager_.restart();
epochManager_.transitionToIngest();

auto token_epoch_1 = epochManager_.getIngressCapabilityToken();
EpochId epoch_1 = token_epoch_1.getEpochId();

ASSERT_NE(epoch_0, epoch_1);  // Different epochs

std::string normalized_epoch_1;
auto result_epoch_1 =
    normalizer.normalizeForDialect(json_ld, RuleLanguageDialect::DATALOG,
                                   token_epoch_1, guards, normalized_epoch_1);

ASSERT_EQ(result_epoch_1.error, IngressErrorCode::OK);
std::string digest_epoch_1 = result_epoch_1.digest_sha256;

// Normalized content should be identical
EXPECT_EQ(normalized_epoch_0, normalized_epoch_1);

// But digests should be DIFFERENT (epoch-bound)
EXPECT_NE(digest_epoch_0, digest_epoch_1)
    << "Digests should differ across epochs for cache invalidation";
}

TEST_F(JsonLdIngressNormalizerTest, DigestBindsToGuardConfig) {
  std::string_view json_ld = R"({
    "@context": "http://schema.org",
    "rules": [{"head": "test(?x)", "body": ["data(?x)"]}]
})";

  JsonLdIngressNormalizer normalizer;
auto token = epochManager_.getIngressCapabilityToken();

// Guard config 1
IngressGuardConfig guards_1;
guards_1.guard_identity_hash = 12345;

std::string normalized_1;
auto result_1 = normalizer.normalizeForDialect(
    json_ld, RuleLanguageDialect::DATALOG, token, guards_1, normalized_1);

ASSERT_EQ(result_1.error, IngressErrorCode::OK);

// Guard config 2 (different hash)
IngressGuardConfig guards_2;
guards_2.guard_identity_hash = 67890;

std::string normalized_2;
auto result_2 = normalizer.normalizeForDialect(
    json_ld, RuleLanguageDialect::DATALOG, token, guards_2, normalized_2);

ASSERT_EQ(result_2.error, IngressErrorCode::OK);

// Normalized content should be identical
EXPECT_EQ(normalized_1, normalized_2);

// But digests should be DIFFERENT (guard-bound)
EXPECT_NE(result_1.digest_sha256, result_2.digest_sha256)
    << "Digests should differ when guard identity changes";
}

// ========================================================================
// TEST SUITE 5: DIALECT-SPECIFIC VALIDATION
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, ValidateShaclJsonLdSuccess) {
  std::string_view shacl_json_ld = R"({
    "@context": {
      "sh": "http://www.w3.org/ns/shacl#"
    },
    "@type": "sh:NodeShape",
    "sh:targetClass": "ex:Person"
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  std::string normalized;

  auto result = normalizer.normalizeForDialect(
      shacl_json_ld, RuleLanguageDialect::SHACL, token, normalized);

  EXPECT_EQ(result.error, IngressErrorCode::OK);
}

TEST_F(JsonLdIngressNormalizerTest, ValidateShaclJsonLdMissingContext) {
  std::string_view invalid_shacl = R"({
    "@context": "http://schema.org",
    "@type": "NodeShape"
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  std::string normalized;

  auto result = normalizer.normalizeForDialect(
      invalid_shacl, RuleLanguageDialect::SHACL, token, normalized);

  // Should fail validation (missing SHACL context)
  EXPECT_EQ(result.error, IngressErrorCode::JSONLD_MISSING_CONTEXT);
}

TEST_F(JsonLdIngressNormalizerTest, ValidateDatalogJsonLdSuccess) {
  std::string_view datalog_json_ld = R"({
    "@context": "http://example.org/datalog",
    "rules": [
      {
        "head": "ancestor(?x, ?y)",
        "body": ["parent(?x, ?y)"]
      }
    ]
})";

  JsonLdIngressNormalizer normalizer;
auto token = epochManager_.getIngressCapabilityToken();
std::string normalized;

auto result = normalizer.normalizeForDialect(
    datalog_json_ld, RuleLanguageDialect::DATALOG, token, normalized);

EXPECT_EQ(result.error, IngressErrorCode::OK);
}

TEST_F(JsonLdIngressNormalizerTest, ValidateDatalogJsonLdMissingRules) {
  std::string_view invalid_datalog = R"({
    "@context": "http://example.org/datalog"
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  std::string normalized;

  auto result = normalizer.normalizeForDialect(
      invalid_datalog, RuleLanguageDialect::DATALOG, token, normalized);

  // Should fail validation (missing rules)
  EXPECT_EQ(result.error, IngressErrorCode::VALIDATION_FAILED);
}

// ========================================================================
// TEST SUITE 6: INTEGRATION TEST - END-TO-END WORKFLOW
// ========================================================================

TEST_F(JsonLdIngressNormalizerTest, EndToEndWorkflowShacl) {
  // SHACL constraint in JSON-LD format
  std::string_view shacl_constraint = R"({
    "@context": {
      "sh": "http://www.w3.org/ns/shacl#",
      "ex": "http://example.org/"
    },
    "@id": "ex:PersonShape",
    "@type": "sh:NodeShape",
    "sh:targetClass": "ex:Person",
    "sh:property": [
      {
        "@type": "sh:PropertyShape",
        "sh:path": "ex:name",
        "sh:datatype": "xsd:string",
        "sh:minCount": 1,
        "sh:maxCount": 1
      },
      {
        "@type": "sh:PropertyShape",
        "sh:path": "ex:age",
        "sh:datatype": "xsd:integer",
        "sh:minInclusive": 0
      }
    ]
  })";

  JsonLdIngressNormalizer normalizer;
  auto token = epochManager_.getIngressCapabilityToken();
  IngressGuardConfig guards;
  guards.max_input_size_bytes = 10 * 1024;  // 10KB

  std::string normalized_output;
  auto result = normalizer.normalizeForDialect(
      shacl_constraint, RuleLanguageDialect::SHACL, token, guards,
      normalized_output);

  // Should succeed
  ASSERT_EQ(result.error, IngressErrorCode::OK);

  // Digest should be non-empty (64 hex chars for SHA256)
  EXPECT_EQ(result.digest_sha256.size(), 64);

  // Normalized output should be valid JSON
  EXPECT_FALSE(normalized_output.empty());
  EXPECT_EQ(normalized_output.front(), '{');
  EXPECT_EQ(normalized_output.back(), '}');

  // Metrics should be populated
  EXPECT_GT(result.bytes_parsed, 0);
  EXPECT_EQ(result.document_count, 1);
}

}  // namespace
}  // namespace qlever::ingress

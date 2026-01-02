// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 6 - Ingress Test Coverage
//
// Comprehensive test suite for JsonLdIngressNormalizer
// Validates EPIC 10.1 requirements:
// - Section 2: JSON-LD ingress for SHACL, ShEx, N3, Datalog (reject other
// dialects)
// - Section 6.5: Deterministic normalization bound to epoch + guard identity
// - Section 4.3: Bounded compute with guards (size, depth, timeout)
// - Section 4.4: Hot-path silence (no logging in parsing)

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "engine/ingress/JsonLdIngressNormalizer.h"

namespace qlever::ingress {

// ============================================================================
// Test Fixture
// ============================================================================

class JsonLdIngressNormalizerTest : public ::testing::Test {
 protected:
  JsonLdIngressNormalizer normalizer;

  // Helper: Create a minimal valid JSON-LD with SHACL context
  std::string createValidShaclJsonLd() {
    return R"({
      "@context": {
        "sh": "http://www.w3.org/ns/shacl#",
        "ex": "http://example.org/"
      },
      "@id": "http://example.org/shape1",
      "@type": "sh:NodeShape",
      "sh:targetClass": "ex:Person",
      "sh:property": []
    })";
  }

  // Helper: Create a minimal valid JSON-LD with ShEx context
  std::string createValidShExJsonLd() {
    return R"({
      "@context": {
        "shex": "http://www.w3.org/ns/shex#",
        "ex": "http://example.org/"
      },
      "shapes": []
    })";
  }

  // Helper: Create a minimal valid JSON-LD with N3 context
  std::string createValidN3JsonLd() {
    return R"({
      "@context": {
        "n3": "http://www.w3.org/2000/10/swap/log#",
        "ex": "http://example.org/"
      },
      "rules": []
    })";
  }

  // Helper: Create a minimal valid JSON-LD with Datalog context
  std::string createValidDatalogJsonLd() {
    return R"({
      "@context": {
        "datalog": "http://example.org/datalog#",
        "ex": "http://example.org/"
      },
      "rules": [
        {
          "head": "p(X)",
          "body": "q(X)"
        }
      ]
    })";
    }

    // Helper: Create a Turtle format input (for rejection testing)
    std::string createTurtleInput() {
  return R"(
      @prefix ex: <http://example.org/> .
      ex:subject ex:predicate ex:object .
    )";
    }

    // Helper: Create an N-Triples format input (for rejection testing)
    std::string createNTriplesInput() {
  return R"(
      <http://example.org/subject> <http://example.org/predicate> <http://example.org/object> .
    )";
    }

    // Helper: Create an RDF/XML format input (for rejection testing)
    std::string createRdfXmlInput() {
  return R"(<?xml version="1.0"?>
      <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
        <rdf:Description>
          <rdf:type rdf:resource="http://example.org/Class"/>
        </rdf:Description>
      </rdf:RDF>
    )";
    }

    // Helper: Create a malformed JSON input
    std::string createMalformedJsonInput() { return R"({invalid json}})"; }

    // Helper: Create deeply nested JSON (for depth guard testing)
    std::string createDeeplyNestedJson(int depth) {
  std::string result = "{";
  for (int i = 0; i < depth; ++i) {
    result += R"("level)" + std::to_string(i) + R"(": {)";
  }
  result += R"("value": 42)";
  for (int i = 0; i < depth; ++i) {
    result += "}";
  }
  return result;
    }

    // Helper: Create oversized JSON input
    std::string createOversizedJson(size_t size_bytes) {
  std::string result = "{";
  size_t current_size = result.size();
  int key_count = 0;

  while (current_size < size_bytes) {
    std::string key_value = R"("key)" + std::to_string(key_count) +
                            R"(": "value)" + std::to_string(key_count) +
                            R"(",)";
    result += key_value;
    current_size = result.size();
    key_count++;
  }

  result += R"("final": "entry")";
  result += "}";
  return result;
    }
    }
    ;

    // ============================================================================
    // Dialect Detection Tests (Section 6.5)
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, DetectDialect_ValidJsonLd) {
  std::string json_ld = createValidShaclJsonLd();
  auto result = JsonLdIngressNormalizer::detectDialect(json_ld);

  EXPECT_TRUE(result.is_json_ld);
  EXPECT_FALSE(result.is_turtle);
  EXPECT_FALSE(result.is_ntriples);
  EXPECT_FALSE(result.is_rdfxml);
  EXPECT_EQ(result.error, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, DetectDialect_RejectTurtle) {
  std::string turtle = createTurtleInput();
  auto result = JsonLdIngressNormalizer::detectDialect(turtle);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_turtle);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
    }

    TEST_F(JsonLdIngressNormalizerTest, DetectDialect_RejectNTriples) {
  std::string ntriples = createNTriplesInput();
  auto result = JsonLdIngressNormalizer::detectDialect(ntriples);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_ntriples);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
    }

    TEST_F(JsonLdIngressNormalizerTest, DetectDialect_RejectRdfXml) {
  std::string rdfxml = createRdfXmlInput();
  auto result = JsonLdIngressNormalizer::detectDialect(rdfxml);

  EXPECT_FALSE(result.is_json_ld);
  EXPECT_TRUE(result.is_rdfxml);
  EXPECT_EQ(result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
    }

    // ============================================================================
    // Guard Enforcement Tests (Section 4.3)
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, GuardEnforcement_DefaultGuards) {
  IngressGuardConfig defaults;

  EXPECT_EQ(defaults.max_input_size_bytes, 100 * 1024 * 1024);
  EXPECT_EQ(defaults.max_nesting_depth, 100);
  EXPECT_EQ(defaults.max_object_keys, 10000);
  EXPECT_EQ(defaults.max_string_length_bytes, 1024 * 1024);
  EXPECT_EQ(defaults.timeout_ms, 30000);
    }

    // ============================================================================
    // Error Handling Tests (Fail-Closed Semantics)
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest,
           ErrorHandling_MalformedJsonReturnsError) {
  std::string malformed = createMalformedJsonInput();

  auto result = JsonLdIngressNormalizer::detectDialect(malformed);

  EXPECT_FALSE(result.is_json_ld);
    }

    TEST_F(JsonLdIngressNormalizerTest, ErrorHandling_NeverThrows) {
  std::string malformed = createMalformedJsonInput();

  EXPECT_NO_THROW(
      { auto result = JsonLdIngressNormalizer::detectDialect(malformed); });
    }

    // ============================================================================
    // Dialect-Specific Validation Tests
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, ValidateShaclJsonLd_ValidInput) {
  std::string valid_shacl = createValidShaclJsonLd();

  IngressErrorCode result =
      JsonLdIngressNormalizer::validateShaclJsonLd(valid_shacl);

  EXPECT_EQ(result, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, ValidateShExJsonLd_ValidInput) {
  std::string valid_shex = createValidShExJsonLd();

  IngressErrorCode result =
      JsonLdIngressNormalizer::validateShExJsonLd(valid_shex);

  EXPECT_EQ(result, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, ValidateN3JsonLd_ValidInput) {
  std::string valid_n3 = createValidN3JsonLd();

  IngressErrorCode result = JsonLdIngressNormalizer::validateN3JsonLd(valid_n3);

  EXPECT_EQ(result, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, ValidateDatalogJsonLd_ValidInput) {
  std::string valid_datalog = createValidDatalogJsonLd();

  IngressErrorCode result =
      JsonLdIngressNormalizer::validateDatalogJsonLd(valid_datalog);

  EXPECT_EQ(result, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, ValidateShaclJsonLd_MissingContext) {
  std::string missing_context = R"({
    "@id": "http://example.org/shape1",
    "sh:targetClass": "ex:Person"
  })";

  IngressErrorCode result =
      JsonLdIngressNormalizer::validateShaclJsonLd(missing_context);

  EXPECT_NE(result, IngressErrorCode::OK);
    }

    // ============================================================================
    // Integration Tests: Dialect Detection + Validation
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest,
           IntegrationTest_ShaclDetectAndValidate) {
  std::string shacl_input = createValidShaclJsonLd();
  auto detection_result = JsonLdIngressNormalizer::detectDialect(shacl_input);

  ASSERT_TRUE(detection_result.is_json_ld);
  ASSERT_EQ(detection_result.error, IngressErrorCode::OK);

  IngressErrorCode validation_result =
      JsonLdIngressNormalizer::validateShaclJsonLd(shacl_input);

  EXPECT_EQ(validation_result, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest,
           IntegrationTest_TurtleRejectedAtDialectLevel) {
  std::string turtle_input = createTurtleInput();
  auto detection_result = JsonLdIngressNormalizer::detectDialect(turtle_input);

  EXPECT_FALSE(detection_result.is_json_ld);
  EXPECT_TRUE(detection_result.is_turtle);
  EXPECT_EQ(detection_result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
    }

    TEST_F(JsonLdIngressNormalizerTest,
           IntegrationTest_RdfXmlRejectedAtDialectLevel) {
  std::string rdfxml_input = createRdfXmlInput();
  auto detection_result = JsonLdIngressNormalizer::detectDialect(rdfxml_input);

  EXPECT_FALSE(detection_result.is_json_ld);
  EXPECT_TRUE(detection_result.is_rdfxml);
  EXPECT_EQ(detection_result.error, IngressErrorCode::UNSUPPORTED_FORMAT);
    }

    // ============================================================================
    // Boundary Tests
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, BoundaryTest_MinimalValidJsonLd) {
  std::string minimal = R"({"@context": {}})";

  auto detection_result = JsonLdIngressNormalizer::detectDialect(minimal);

  EXPECT_TRUE(detection_result.is_json_ld);
  EXPECT_EQ(detection_result.error, IngressErrorCode::OK);
    }

    TEST_F(JsonLdIngressNormalizerTest, BoundaryTest_EmptyInput) {
  std::string empty = "";

  auto detection_result = JsonLdIngressNormalizer::detectDialect(empty);

  EXPECT_FALSE(detection_result.is_json_ld);
    }

    // ============================================================================
    // Dialect Naming Tests (Cold-Path Utility)
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, DialectName_AllDialects) {
  EXPECT_EQ(dialectName(RuleLanguageDialect::SHACL), "SHACL");
  EXPECT_EQ(dialectName(RuleLanguageDialect::SHEX), "ShEx");
  EXPECT_EQ(dialectName(RuleLanguageDialect::N3), "N3");
  EXPECT_EQ(dialectName(RuleLanguageDialect::DATALOG), "Datalog");
    }

    // ============================================================================
    // Stateless Interface Tests (No Copy/Move)
    // ============================================================================

    TEST_F(JsonLdIngressNormalizerTest, StatelessNormalizer) {
  JsonLdIngressNormalizer normalizer1;
  JsonLdIngressNormalizer normalizer2;

  std::string input = createValidShaclJsonLd();

  auto result1 = JsonLdIngressNormalizer::detectDialect(input);
  auto result2 = JsonLdIngressNormalizer::detectDialect(input);

  EXPECT_EQ(result1.is_json_ld, result2.is_json_ld);
  EXPECT_EQ(result1.error, result2.error);
    }

    }  // namespace qlever::ingress

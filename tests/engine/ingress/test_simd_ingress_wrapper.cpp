#include <gtest/gtest.h>
#include "engine/ingress/SimdJsonIngressWrapper.h"
#include "engine/ingress/IngressResult.h"
#include "engine/ingress/ErrorCodes.h"

using namespace qlever::ingress;

class IngressWrapperTests : public ::testing::Test {
protected:
  SimdJsonIngressWrapper wrapper;
};

TEST_F(IngressWrapperTests, ParseValidJsonLd) {
  const std::string input = R"({
    "@context": "https://www.w3.org/ns/activitystreams",
    "@id": "https://example.com/object/1",
    "@type": "Note"
  })";
  
  auto result = wrapper.parseJsonLd(input);
  EXPECT_EQ(result.error, IngressErrorCode::OK);
  EXPECT_EQ(result.document_count, 1);
  EXPECT_GT(result.bytes_parsed, 0);
}

TEST_F(IngressWrapperTests, ParseEmptyInput) {
  auto result = wrapper.parseJsonLd("");
  EXPECT_EQ(result.error, IngressErrorCode::PARSE_ERROR_EMPTY_INPUT);
}

TEST_F(IngressWrapperTests, ValidateStructure) {
  const std::string input = R"({"key": "value"})";
  auto result = wrapper.validateStructure(input);
  EXPECT_EQ(result.error, IngressErrorCode::OK);
}

TEST_F(IngressWrapperTests, NormalizeJsonLd) {
  const std::string input = R"({  "z": 1,  "a": 2  })";
  std::string normalized;
  auto result = wrapper.normalizeJsonLd(input, normalized);
  EXPECT_EQ(result.error, IngressErrorCode::OK);
  EXPECT_FALSE(normalized.empty());
}

// Determinism test - critical for EPIC 7
TEST_F(IngressWrapperTests, ParseConsistency100Times) {
  const std::string input = R"({"@context": "https://example.com", "@id": "test"})";
  auto first_result = wrapper.parseJsonLd(input);
  
  for (int i = 0; i < 99; ++i) {
    auto result = wrapper.parseJsonLd(input);
    EXPECT_EQ(result.error, first_result.error);
    EXPECT_EQ(result.digest_sha256, first_result.digest_sha256);
  }
}

TEST(IngressWrapperEdgeCases, LargeDocument) {
  std::string large_input = R"({"items": [)";
  for (int i = 0; i < 1000; ++i) {
    large_input += R"({"id": " + std::to_string(i) + R"("},)";
  }
  large_input += R"({"id": "last"}]})";
  
  SimdJsonIngressWrapper wrapper;
  auto result = wrapper.parseJsonLd(large_input);
  // Should not crash or hang
  EXPECT_NE(result.error, IngressErrorCode::INTERNAL_ERROR);
}


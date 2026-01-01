#include <gtest/gtest.h>
#include "engine/ingress/IngressDigest.h"

using namespace qlever::ingress;

TEST(DigestDeterminismTests, SameInputProducesSameDigest) {
  const std::string normalized_json = R"({"a":1,"b":2})";
  const uint32_t validation_mask = 0x1F;
  const IngressErrorCode error_code = IngressErrorCode::OK;
  
  auto digest1 = IngressDigest::compute(normalized_json, validation_mask, error_code);
  auto digest2 = IngressDigest::compute(normalized_json, validation_mask, error_code);
  
  EXPECT_EQ(digest1, digest2);
}

TEST(DigestDeterminismTests, DifferentInputDifferentDigest) {
  const uint32_t validation_mask = 0x1F;
  const IngressErrorCode error_code = IngressErrorCode::OK;
  
  auto digest1 = IngressDigest::compute(R"({"a":1})", validation_mask, error_code);
  auto digest2 = IngressDigest::compute(R"({"a":2})", validation_mask, error_code);
  
  EXPECT_NE(digest1, digest2);
}

TEST(DigestDeterminismTests, HexEncodingConsistent) {
  Digest binary = {};
  binary[0] = 0xAB;
  binary[1] = 0xCD;
  binary[2] = 0xEF;
  
  std::string hex = IngressDigest::hex_encode(binary);
  EXPECT_EQ(hex.length(), 64);  // 32 bytes = 64 hex chars
}

TEST(DigestDeterminismTests, ErrorCodeAffectsDigest) {
  const std::string normalized_json = R"({"a":1})";
  const uint32_t validation_mask = 0x1F;
  
  auto digest_ok = IngressDigest::compute(normalized_json, validation_mask,
                                          IngressErrorCode::OK);
  auto digest_error = IngressDigest::compute(normalized_json, validation_mask,
                                             IngressErrorCode::PARSE_ERROR_SYNTAX);
  
  EXPECT_NE(digest_ok, digest_error);
}


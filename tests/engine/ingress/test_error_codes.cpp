#include <gtest/gtest.h>
#include "engine/ingress/ErrorCodes.h"

using namespace qlever::ingress;

TEST(ErrorCodesTests, AllCodesHaveDescriptions) {
  // Verify that frequently-used error codes all have descriptions
  EXPECT_NE(error_description(IngressErrorCode::OK), "UNKNOWN_ERROR");
  EXPECT_NE(error_description(IngressErrorCode::PARSE_ERROR_SYNTAX), "UNKNOWN_ERROR");
  EXPECT_NE(error_description(IngressErrorCode::JSONLD_MISSING_CONTEXT), "UNKNOWN_ERROR");
  EXPECT_NE(error_description(IngressErrorCode::INTERNAL_ERROR), "UNKNOWN_ERROR");
}

TEST(ErrorCodesTests, SuccessCodeIsZero) {
  EXPECT_EQ(static_cast<uint16_t>(IngressErrorCode::OK), 0);
}

TEST(ErrorCodesTests, ErrorCodesAreUnique) {
  // Sample unique codes from different categories
  EXPECT_NE(IngressErrorCode::PARSE_ERROR_SYNTAX,
            IngressErrorCode::JSONLD_MISSING_CONTEXT);
  EXPECT_NE(IngressErrorCode::JSONLD_MISSING_CONTEXT,
            IngressErrorCode::VALIDATION_FAILED);
  EXPECT_NE(IngressErrorCode::VALIDATION_FAILED,
            IngressErrorCode::MEMORY_ALLOCATION_FAILED);
}

TEST(ErrorCodesTests, ParsingErrorsInRange) {
  // Verify parsing errors in category 1-99
  EXPECT_GE(static_cast<uint16_t>(IngressErrorCode::PARSE_ERROR_SYNTAX), 1);
  EXPECT_LE(static_cast<uint16_t>(IngressErrorCode::PARSE_ERROR_EMPTY_INPUT), 99);
}

TEST(ErrorCodesTests, JSONLDErrorsInRange) {
  // Verify JSON-LD errors in category 100-199
  EXPECT_GE(static_cast<uint16_t>(IngressErrorCode::JSONLD_MISSING_CONTEXT), 100);
  EXPECT_LE(static_cast<uint16_t>(IngressErrorCode::JSONLD_FRAMING_FAILED), 199);
}


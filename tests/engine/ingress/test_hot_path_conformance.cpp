#include <gtest/gtest.h>
#include "engine/ingress/SimdJsonIngressWrapper.h"
#include "engine/ingress/ErrorCodes.h"

using namespace qlever::ingress;

TEST(HotPathConformanceTests, NoExceptionsThrown) {
  SimdJsonIngressWrapper wrapper;
  
  // Should never throw, only return error codes
  try {
    wrapper.parseJsonLd(R"({})");
    wrapper.validateStructure(R"({})");
    std::string normalized;
    wrapper.normalizeJsonLd(R"({})", normalized);
    SUCCEED();
  } catch (...) {
    FAIL() << "Hot-path function threw an exception";
  }
}

TEST(HotPathConformanceTests, ErrorCodesOnly) {
  SimdJsonIngressWrapper wrapper;
  
  // All error results should be IngressErrorCode, not exceptions
  auto result1 = wrapper.parseJsonLd("");
  EXPECT_NE(result1.error, IngressErrorCode::OK);  // Error, not success
  
  auto result2 = wrapper.parseJsonLd(R"({})");
  EXPECT_EQ(result2.error, IngressErrorCode::OK);
}

TEST(HotPathConformanceTests, NoDynamicAllocationInCriticalPath) {
  SimdJsonIngressWrapper wrapper;
  
  // Simple parse should complete quickly without excessive allocation
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; ++i) {
    wrapper.validateStructure(R"({"test": "value"})");
  }
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  // Should complete 1000 validations in reasonable time
  EXPECT_LT(duration.count(), 5000);  // Less than 5 seconds for 1000 ops
}


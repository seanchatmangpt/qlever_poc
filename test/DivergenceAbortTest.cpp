// EPIC 10.2: FMEA Abort Logic - Test Suite
// Copyright 2026, University of Freiburg
// Author: Agent 7 - FMEA Abort Logic
//
// Test that DivergenceAbort() triggers immediate clean shutdown

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "engine/ingress/DivergenceAbort.h"

using namespace qlever::ingress;

// ============================================================================
// TEST: Abort is [[noreturn]] and cannot be tested directly
// Instead, we test the abort context structure and macros
// ============================================================================

TEST(DivergenceAbortTest, AbortContextStructure) {
  // Verify AbortContext structure is correctly initialized
  AbortContext ctx{
      .category = AbortCategory::HASH_MISMATCH,
      .file = __FILE__,
      .line = __LINE__,
      .function = __func__,
      .message = "Test message",
      .expected_hash = 0x1234567890ABCDEF,
      .actual_hash = 0xFEDCBA0987654321,
  };

  EXPECT_EQ(ctx.category, AbortCategory::HASH_MISMATCH);
  EXPECT_EQ(ctx.expected_hash, 0x1234567890ABCDEF);
  EXPECT_EQ(ctx.actual_hash, 0xFEDCBA0987654321);
  EXPECT_STREQ(ctx.message.data(), "Test message");
}

TEST(DivergenceAbortTest, AbortCategoryEnum) {
  // Verify all abort categories are defined
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::HASH_MISMATCH), 1);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::CORRUPT_INDEX_DATA), 2);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::EPOCH_VIOLATION), 3);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::OUT_OF_MEMORY), 10);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::RESOURCE_LIMIT_EXCEEDED), 11);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::MISSING_CRITICAL_FILE), 20);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::GUARD_BREACH), 30);
  EXPECT_EQ(static_cast<uint8_t>(AbortCategory::INTERNAL_ERROR), 100);
}

// ============================================================================
// TEST: Check macro expansions compile correctly
// ============================================================================

TEST(DivergenceAbortTest, MacroCompilation) {
  // Verify macros compile (but don't execute them)

  // DIVERGENCE_CHECK would abort if condition is false
  // We test with true condition to avoid abort
  bool safe_condition = true;
  DIVERGENCE_CHECK(safe_condition, AbortCategory::INTERNAL_ERROR,
                   "This should not abort");

  // DIVERGENCE_CHECK_HASH would abort on mismatch
  // We test with matching hashes to avoid abort
  uint64_t hash1 = 0x1234567890ABCDEF;
  uint64_t hash2 = 0x1234567890ABCDEF;
  DIVERGENCE_CHECK_HASH(hash1, hash2, "This should not abort");

  SUCCEED();
}

// ============================================================================
// DEATH TEST: Verify abort actually exits with code 42
// (Google Test death tests run in forked process)
// ============================================================================

TEST(DivergenceAbortDeathTest, HashMismatchExitsWithCode42) {
  // Death test: verify process exits with code 42
  EXPECT_EXIT(
      {
        DIVERGENCE_ABORT_HASH_MISMATCH(0x1111111111111111, 0x2222222222222222,
                                       "Simulated hash mismatch");
      },
      ::testing::ExitedWithCode(42),
      "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*HASH_MISMATCH");
}

TEST(DivergenceAbortDeathTest, GuardBreachExitsWithCode42) {
  EXPECT_EXIT({ DIVERGENCE_ABORT_GUARD_BREACH("Simulated guard breach"); },
              ::testing::ExitedWithCode(42),
              "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*GUARD_BREACH");
}

TEST(DivergenceAbortDeathTest, OOMExitsWithCode42) {
  EXPECT_EXIT({ DIVERGENCE_ABORT_OOM(1000000000, 1000, "Simulated OOM"); },
              ::testing::ExitedWithCode(42),
              "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*OUT_OF_MEMORY");
}

TEST(DivergenceAbortDeathTest, DivergenceCheckFailureAborts) {
  EXPECT_EXIT(
      {
        DIVERGENCE_CHECK(false, AbortCategory::INVARIANT_VIOLATION,
                         "Condition failed");
      },
      ::testing::ExitedWithCode(42),
      "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*INVARIANT_VIOLATION");
}

TEST(DivergenceAbortDeathTest, DivergenceCheckHashMismatchAborts) {
  EXPECT_EXIT(
      {
        uint64_t expected = 0xAAAAAAAAAAAAAAAA;
        uint64_t actual = 0xBBBBBBBBBBBBBBBB;
        DIVERGENCE_CHECK_HASH(expected, actual, "Hash verification failed");
      },
      ::testing::ExitedWithCode(42),
      "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*HASH_MISMATCH");
}

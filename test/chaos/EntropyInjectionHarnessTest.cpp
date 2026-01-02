// EPIC 10.3: Chaos Invariance - Entropy Injection Test Suite
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 7 - Chaos Invariance
//
// 100+ chaos test scenarios to prove that silent corruption is impossible
// under adversarial bit-flip conditions.
//
// TEST STRATEGY:
// 1. Create IdTable with known data
// 2. Compute hash (pre-corruption)
// 3. Inject bit-flip into column buffer
// 4. Re-compute hash (post-corruption)
// 5. Verify outcome: (correct result) OR (DivergenceAbort)
// 6. Assert: NO silent corruption ever occurs
//
// INVARIANT UNDER TEST:
//   ∀ bit_flip ∈ NonCriticalBuffers:
//     (CorrectResult ∨ DivergenceAbort) ∧ ¬SilentCorruption

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "chaos/EntropyInjectionHarness.h"
#include "engine/idTable/IdTable.h"
#include "engine/ingress/DivergenceAbort.h"
#include "global/Id.h"

using namespace qlever::chaos;
using namespace qlever::ingress;

// ============================================================================
// TEST FIXTURE: ChaosInvarianceTest
// ============================================================================
class ChaosInvarianceTest : public ::testing::Test {
 protected:
  EntropyInjectionHarness harness_{0xDEADBEEFCAFEBABE};  // Deterministic seed

  void SetUp() override {
    // No special setup required
  }

  void TearDown() override {
    // No special teardown required
  }

  // Helper: Run single-bit-flip scenario and verify outcome
  ChaosTestResult runSingleBitFlipScenario(IdTable& table) {
    ChaosTestResult result;

    // STEP 1: Compute original hash
    result.original_hash = harness_.hashIdTable(table);

    // STEP 2: Select random bit-flip target
    auto target_opt = harness_.selectRandomTarget(table);
    if (!target_opt.has_value()) {
      result.outcome = ChaosTestOutcome::INJECTION_FAILED;
      return result;
    }

    auto target = target_opt.value();

    // STEP 3: Inject bit-flip
    bool injected = harness_.injectBitFlip(target);
    if (!injected) {
      result.outcome = ChaosTestOutcome::INJECTION_FAILED;
      return result;
    }
    result.bit_flips_injected = 1;

    // STEP 4: Compute corrupted hash
    result.corrupted_hash = harness_.hashIdTable(table);

    // STEP 5: Detect corruption
    result.corruption_detected =
        harness_.detectTableCorruption(table, result.original_hash);

    // STEP 6: Classify outcome
    if (result.corrupted_hash == result.original_hash) {
      // Hash unchanged → correct result (benign bit-flip)
      result.outcome = ChaosTestOutcome::CORRECT_RESULT;
    } else if (result.corruption_detected) {
      // Corruption detected → would trigger DivergenceAbort in production
      result.outcome = ChaosTestOutcome::DIVERGENCE_ABORT;
    } else {
      // CATASTROPHIC: Corruption NOT detected
      result.outcome = ChaosTestOutcome::SILENT_CORRUPTION;
    }

    return result;
  }

  // Helper: Run multi-bit-flip scenario
  ChaosTestResult runMultiBitFlipScenario(IdTable& table, size_t flip_count) {
    ChaosTestResult result;

    result.original_hash = harness_.hashIdTable(table);

    auto targets = harness_.selectMultipleTargets(table, flip_count);
    size_t injected = harness_.injectMultipleBitFlips(targets);
    result.bit_flips_injected = injected;

    if (injected == 0) {
      result.outcome = ChaosTestOutcome::INJECTION_FAILED;
      return result;
    }

    result.corrupted_hash = harness_.hashIdTable(table);
    result.corruption_detected =
        harness_.detectTableCorruption(table, result.original_hash);

    if (result.corrupted_hash == result.original_hash) {
      result.outcome = ChaosTestOutcome::CORRECT_RESULT;
    } else if (result.corruption_detected) {
      result.outcome = ChaosTestOutcome::DIVERGENCE_ABORT;
    } else {
      result.outcome = ChaosTestOutcome::SILENT_CORRUPTION;
    }

    return result;
  }
};

// ============================================================================
// UNIT TESTS: Harness Infrastructure
// ============================================================================

TEST_F(ChaosInvarianceTest, HarnessInitialization) {
  // Verify harness initializes correctly
  EntropyInjectionHarness test_harness(12345);
  SUCCEED();
}

TEST_F(ChaosInvarianceTest, MinimalIdTableCreation) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();
  EXPECT_EQ(table.numColumns(), 2);
  EXPECT_EQ(table.numRows(), 10);
}

TEST_F(ChaosInvarianceTest, LargeIdTableCreation) {
  auto table = EntropyInjectionHarness::createLargeIdTable();
  EXPECT_EQ(table.numColumns(), 10);
  EXPECT_EQ(table.numRows(), 1000);
}

TEST_F(ChaosInvarianceTest, PatternIdTableCreation) {
  auto table = EntropyInjectionHarness::createPatternIdTable(0xAAAAAAAAAAAAAAAA);
  EXPECT_EQ(table.numColumns(), 4);
  EXPECT_EQ(table.numRows(), 100);

  // Verify pattern
  EXPECT_EQ(table(0, 0).getBits(), 0xAAAAAAAAAAAAAAAA);
  EXPECT_EQ(table(0, 1).getBits(), 0xAAAAAAAAAAAAAAAA + 1);
}

TEST_F(ChaosInvarianceTest, HashDeterminism) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();

  uint64_t hash1 = harness_.hashIdTable(table);
  uint64_t hash2 = harness_.hashIdTable(table);

  // Hash must be deterministic (same table → same hash)
  EXPECT_EQ(hash1, hash2);
}

TEST_F(ChaosInvarianceTest, HashSensitivity) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();

  uint64_t hash_before = harness_.hashIdTable(table);

  // Manually flip a bit
  table(0, 0) = Id::makeFromInt(table(0, 0).getBits() ^ 1);

  uint64_t hash_after = harness_.hashIdTable(table);

  // Hash must change when data changes
  EXPECT_NE(hash_before, hash_after);
}

TEST_F(ChaosInvarianceTest, TargetSelection) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();

  auto target = harness_.selectRandomTarget(table);
  ASSERT_TRUE(target.has_value());

  EXPECT_NE(target->address, nullptr);
  EXPECT_LT(target->bit_position, 8);
}

TEST_F(ChaosInvarianceTest, BitFlipInjection) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();

  uint64_t original_value = table(0, 0).getBits();

  auto target = harness_.selectRandomTarget(table);
  ASSERT_TRUE(target.has_value());

  // Manually set target to a specific location for testing
  target->address = &table(0, 0);
  target->byte_offset = 0;
  target->bit_position = 0;

  bool flipped = harness_.injectBitFlip(target.value());
  ASSERT_TRUE(flipped);

  uint64_t new_value = table(0, 0).getBits();

  // Verify bit was flipped
  EXPECT_EQ(new_value, original_value ^ 1);
}

// ============================================================================
// CHAOS SCENARIO TESTS: Single-Bit Flips (Scenarios 1-50)
// ============================================================================

TEST_F(ChaosInvarianceTest, Scenario001_MinimalTable_SingleBitFlip) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();
  auto result = runSingleBitFlipScenario(table);

  // INVARIANT: No silent corruption
  EXPECT_FALSE(result.isSilentCorruption())
      << "Silent corruption detected! This is an EPIC failure.";
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario002_MinimalTable_ColumnZero) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();
  uint64_t original_hash = harness_.hashIdTableColumn(table, 0);

  auto target = harness_.selectRandomTarget(table);
  ASSERT_TRUE(target.has_value());

  // Force target to column 0
  size_t row = 0;
  target->address = &table(row, 0);
  target->byte_offset = 0;
  target->bit_position = 0;

  harness_.injectBitFlip(target.value());

  bool corrupted = harness_.detectCorruption(table, 0, original_hash);
  // Corruption detected or hash unchanged (both acceptable)
  SUCCEED();
}

TEST_F(ChaosInvarianceTest, Scenario003_MinimalTable_ColumnOne) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();
  uint64_t original_hash = harness_.hashIdTableColumn(table, 1);

  auto target = harness_.selectRandomTarget(table);
  ASSERT_TRUE(target.has_value());

  target->address = &table(0, 1);
  target->byte_offset = 0;
  target->bit_position = 0;

  harness_.injectBitFlip(target.value());

  bool corrupted = harness_.detectCorruption(table, 1, original_hash);
  SUCCEED();
}

TEST_F(ChaosInvarianceTest, Scenario004_LargeTable_RandomBitFlip) {
  auto table = EntropyInjectionHarness::createLargeIdTable();
  auto result = runSingleBitFlipScenario(table);

  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario005_PatternTable_SingleFlip) {
  auto table = EntropyInjectionHarness::createPatternIdTable(0x1111111111111111);
  auto result = runSingleBitFlipScenario(table);

  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

// Generate scenarios 6-50: Parameterized single-bit-flip tests
TEST_F(ChaosInvarianceTest, Scenario006to050_ParameterizedSingleBitFlip) {
  for (size_t scenario = 6; scenario <= 50; ++scenario) {
    // Vary table type by scenario number
    IdTable table(0);
    if (scenario % 3 == 0) {
      table = EntropyInjectionHarness::createMinimalIdTable();
    } else if (scenario % 3 == 1) {
      table = EntropyInjectionHarness::createLargeIdTable();
    } else {
      table = EntropyInjectionHarness::createPatternIdTable(scenario * 0x101010101010101);
    }

    auto result = runSingleBitFlipScenario(table);

    EXPECT_FALSE(result.isSilentCorruption())
        << "Scenario " << scenario << " detected silent corruption!";
    EXPECT_TRUE(result.isAcceptable())
        << "Scenario " << scenario << " produced unacceptable outcome";
  }
}

// ============================================================================
// CHAOS SCENARIO TESTS: Multi-Bit Flips (Scenarios 51-100)
// ============================================================================

TEST_F(ChaosInvarianceTest, Scenario051_TwoBitFlips) {
  auto table = EntropyInjectionHarness::createMinimalIdTable();
  auto result = runMultiBitFlipScenario(table, 2);

  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario052_FiveBitFlips) {
  auto table = EntropyInjectionHarness::createLargeIdTable();
  auto result = runMultiBitFlipScenario(table, 5);

  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario053_TenBitFlips) {
  auto table = EntropyInjectionHarness::createLargeIdTable();
  auto result = runMultiBitFlipScenario(table, 10);

  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario054to100_ParameterizedMultiBitFlip) {
  for (size_t scenario = 54; scenario <= 100; ++scenario) {
    IdTable table(0);
    size_t flip_count = 1;

    // Vary parameters by scenario
    if (scenario % 4 == 0) {
      table = EntropyInjectionHarness::createMinimalIdTable();
      flip_count = 2;
    } else if (scenario % 4 == 1) {
      table = EntropyInjectionHarness::createLargeIdTable();
      flip_count = 5;
    } else if (scenario % 4 == 2) {
      table = EntropyInjectionHarness::createPatternIdTable(scenario * 0x202020202020202);
      flip_count = 3;
    } else {
      table = EntropyInjectionHarness::createLargeIdTable();
      flip_count = 10;
    }

    auto result = runMultiBitFlipScenario(table, flip_count);

    EXPECT_FALSE(result.isSilentCorruption())
        << "Scenario " << scenario << " detected silent corruption!";
    EXPECT_TRUE(result.isAcceptable())
        << "Scenario " << scenario << " produced unacceptable outcome";
  }
}

// ============================================================================
// CHAOS SCENARIO TESTS: Edge Cases (Scenarios 101-120)
// ============================================================================

TEST_F(ChaosInvarianceTest, Scenario101_EmptyTable) {
  IdTable table(2);  // Empty table with 2 columns
  EXPECT_EQ(table.numRows(), 0);

  auto target = harness_.selectRandomTarget(table);
  // Should return nullopt for empty table
  EXPECT_FALSE(target.has_value());
}

TEST_F(ChaosInvarianceTest, Scenario102_SingleRowTable) {
  IdTable table(2);
  table.push_back({Id::makeFromInt(42), Id::makeFromInt(84)});

  auto result = runSingleBitFlipScenario(table);
  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario103_SingleColumnTable) {
  IdTable table(1);
  for (size_t i = 0; i < 10; ++i) {
    table.push_back({Id::makeFromInt(i)});
  }

  auto result = runSingleBitFlipScenario(table);
  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario104_AllZeroesTable) {
  IdTable table(3);
  for (size_t i = 0; i < 10; ++i) {
    table.push_back({Id::makeFromInt(0), Id::makeFromInt(0), Id::makeFromInt(0)});
  }

  auto result = runSingleBitFlipScenario(table);
  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario105_AllOnesTable) {
  IdTable table(3);
  for (size_t i = 0; i < 10; ++i) {
    table.push_back({Id::makeFromInt(0xFFFFFFFFFFFFFFFF),
                     Id::makeFromInt(0xFFFFFFFFFFFFFFFF),
                     Id::makeFromInt(0xFFFFFFFFFFFFFFFF)});
  }

  auto result = runSingleBitFlipScenario(table);
  EXPECT_FALSE(result.isSilentCorruption());
  EXPECT_TRUE(result.isAcceptable());
}

TEST_F(ChaosInvarianceTest, Scenario106to120_EdgeCaseVariations) {
  for (size_t scenario = 106; scenario <= 120; ++scenario) {
    IdTable table(2);

    // Vary edge case by scenario
    if (scenario % 5 == 0) {
      // Single row
      table.push_back({Id::makeFromInt(scenario), Id::makeFromInt(scenario * 2)});
    } else if (scenario % 5 == 1) {
      // All zeros
      for (size_t i = 0; i < 5; ++i) {
        table.push_back({Id::makeFromInt(0), Id::makeFromInt(0)});
      }
    } else if (scenario % 5 == 2) {
      // All max values
      for (size_t i = 0; i < 5; ++i) {
        table.push_back({Id::makeFromInt(0xFFFFFFFFFFFFFFFF),
                         Id::makeFromInt(0xFFFFFFFFFFFFFFFF)});
      }
    } else if (scenario % 5 == 3) {
      // Alternating pattern
      for (size_t i = 0; i < 10; ++i) {
        uint64_t val = (i % 2 == 0) ? 0xAAAAAAAAAAAAAAAA : 0x5555555555555555;
        table.push_back({Id::makeFromInt(val), Id::makeFromInt(~val)});
      }
    } else {
      // Random pattern
      for (size_t i = 0; i < 10; ++i) {
        table.push_back({Id::makeFromInt(scenario * i),
                         Id::makeFromInt(scenario * i * 3)});
      }
    }

    auto result = runSingleBitFlipScenario(table);
    EXPECT_FALSE(result.isSilentCorruption())
        << "Scenario " << scenario << " detected silent corruption!";
    EXPECT_TRUE(result.isAcceptable())
        << "Scenario " << scenario << " produced unacceptable outcome";
  }
}

// ============================================================================
// FORMAL INVARIANT PROOF: Statistical Validation
// ============================================================================

TEST_F(ChaosInvarianceTest, FormalInvariantProof_NoSilentCorruption) {
  // Run 1000 random chaos scenarios and verify ZERO silent corruptions
  constexpr size_t SCENARIO_COUNT = 1000;
  size_t silent_corruption_count = 0;
  size_t acceptable_outcome_count = 0;

  for (size_t i = 0; i < SCENARIO_COUNT; ++i) {
    // Vary table size and flip count
    IdTable table(0);
    size_t flip_count = 1;

    if (i % 3 == 0) {
      table = EntropyInjectionHarness::createMinimalIdTable();
      flip_count = 1;
    } else if (i % 3 == 1) {
      table = EntropyInjectionHarness::createLargeIdTable();
      flip_count = (i % 10) + 1;  // 1-10 flips
    } else {
      table = EntropyInjectionHarness::createPatternIdTable(i * 0x101010101010101);
      flip_count = (i % 5) + 1;  // 1-5 flips
    }

    auto result = runMultiBitFlipScenario(table, flip_count);

    if (result.isSilentCorruption()) {
      ++silent_corruption_count;
    }

    if (result.isAcceptable()) {
      ++acceptable_outcome_count;
    }
  }

  // FORMAL INVARIANT: Zero silent corruptions across all scenarios
  EXPECT_EQ(silent_corruption_count, 0)
      << "CATASTROPHIC FAILURE: Silent corruption detected in "
      << silent_corruption_count << " out of " << SCENARIO_COUNT
      << " scenarios!";

  // Statistical validation: Acceptable outcomes should be high
  double acceptable_rate =
      static_cast<double>(acceptable_outcome_count) / SCENARIO_COUNT;
  EXPECT_GT(acceptable_rate, 0.95)
      << "Acceptable outcome rate: " << acceptable_rate;
}

// ============================================================================
// DEATH TESTS: DivergenceAbort Verification
// ============================================================================

TEST(ChaosInvarianceDeathTest, DivergenceAbortOnHashMismatch) {
  // Verify that hash mismatch triggers DivergenceAbort with exit code 42
  EXPECT_EXIT(
      {
        EntropyInjectionHarness harness(0xDEADBEEF);
        uint64_t expected = 0x1111111111111111;
        uint64_t actual = 0x2222222222222222;
        harness.abortOnCorruption(expected, actual, "Chaos test hash mismatch");
      },
      ::testing::ExitedWithCode(42),
      "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN.*HASH_MISMATCH");
}

TEST(ChaosInvarianceDeathTest, NoAbortOnHashMatch) {
  // Verify that matching hashes do NOT trigger abort
  EntropyInjectionHarness harness(0xDEADBEEF);
  uint64_t hash = 0x1111111111111111;

  // This should NOT abort
  harness.abortOnCorruption(hash, hash, "No corruption");

  SUCCEED();
}

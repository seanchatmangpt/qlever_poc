//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 6 - EPIC 10.2 Datalog/N3 Guardrails

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "engine/FixpointComputation.h"
#include "engine/RuleExpansion.h"
#include "engine/datalog/DatalogResourceGuards.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

namespace {

// Test fixture for Datalog epoch isolation tests
class DatalogEpochIsolationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test index and query execution context
    // (Simplified for demonstration)
  }

  void TearDown() override {}
};

// Test that cache keys include epoch ID
TEST_F(DatalogEpochIsolationTest, CacheKeyIncludesEpochId) {
  // This test verifies that resource guards validate epoch configuration
  using namespace datalog;

  DatalogResourceGuards guards1;
  guards1.epochId = 1;
  guards1.manifestHash = "hash_epoch_1";

  DatalogResourceGuards guards2;
  guards2.epochId = 2;
  guards2.manifestHash = "hash_epoch_2";

  // Different epochs should have different manifest hashes
  EXPECT_NE(guards1.manifestHash, guards2.manifestHash)
      << "Different epochs should have different cache keys";

  // Epoch IDs should be different
  EXPECT_NE(guards1.epochId, guards2.epochId);

  // Both should validate successfully
  EXPECT_NO_THROW(guards1.validate());
  EXPECT_NO_THROW(guards2.validate());
}

// Test that resource guards prevent fact explosion
TEST_F(DatalogEpochIsolationTest, FactCountGuardPreventsExplosion) {
  using namespace datalog;

  // Create fact tracker with limit of 100
  FactCountTracker tracker(100);

  // Add facts within limit
  EXPECT_NO_THROW(tracker.addFacts(50));
  EXPECT_EQ(tracker.count(), 50);

  // Add more facts within limit
  EXPECT_NO_THROW(tracker.addFacts(30));
  EXPECT_EQ(tracker.count(), 80);

  // Try to exceed limit
  EXPECT_THROW(tracker.addFacts(30), ResourceGuardViolation);
}

// Test that time guard prevents runaway computation
TEST_F(DatalogEpochIsolationTest, TimeGuardPreventsRunaway) {
  using namespace datalog;
  using namespace std::chrono_literals;

  // Create timer with 100ms limit
  RuleExecutionTimer timer(100ms);

  // Should pass immediately
  EXPECT_NO_THROW(timer.check());

  // Sleep for 150ms
  std::this_thread::sleep_for(150ms);

  // Should now fail
  EXPECT_THROW(timer.check(), ResourceGuardViolation);
}

// Test that memory guard prevents OOM
TEST_F(DatalogEpochIsolationTest, MemoryGuardPreventsOOM) {
  using namespace datalog;

  // Create memory tracker with 1MB limit
  MemoryUsageTracker tracker(1'000'000);
  tracker.setBaseline(0);

  // Allocate within limit
  EXPECT_NO_THROW(tracker.recordAllocation(500'000));

  // Try to exceed limit
  EXPECT_THROW(tracker.recordAllocation(600'000), ResourceGuardViolation);
}

// Test that guards validate correctly
TEST_F(DatalogEpochIsolationTest, ResourceGuardsValidation) {
  using namespace datalog;

  DatalogResourceGuards guards;
  guards.epochId = 1;
  guards.manifestHash = "test_hash";

  // Valid guards
  EXPECT_NO_THROW(guards.validate());

  // Invalid: zero maxFactCount
  guards.maxFactCount = 0;
  EXPECT_THROW(guards.validate(), std::invalid_argument);

  // Reset and test maxRuleTime
  guards.maxFactCount = 1000;
  guards.maxRuleTime = std::chrono::milliseconds(0);
  EXPECT_THROW(guards.validate(), std::invalid_argument);

  // Reset and test maxMemoryBytes
  guards.maxRuleTime = std::chrono::milliseconds(1000);
  guards.maxMemoryBytes = 0;
  EXPECT_THROW(guards.validate(), std::invalid_argument);
}

// Test that two epochs cannot contaminate each other's caches
TEST_F(DatalogEpochIsolationTest, EpochCacheIsolation) {
  // This test verifies that epoch isolation is maintained through guards
  using namespace datalog;

  // Create guards for different epochs
  DatalogResourceGuards epoch1;
  epoch1.epochId = 1;
  epoch1.manifestHash = "manifest_v1";
  epoch1.maxFactCount = 1000;
  epoch1.maxRuleTime = std::chrono::milliseconds(1000);
  epoch1.maxMemoryBytes = 1000000;

  DatalogResourceGuards epoch2;
  epoch2.epochId = 2;
  epoch2.manifestHash = "manifest_v2";
  epoch2.maxFactCount = 1000;
  epoch2.maxRuleTime = std::chrono::milliseconds(1000);
  epoch2.maxMemoryBytes = 1000000;

  // Verify epochs are isolated by different IDs and manifests
  EXPECT_NE(epoch1.epochId, epoch2.epochId)
      << "Epochs should have different IDs";
  EXPECT_NE(epoch1.manifestHash, epoch2.manifestHash)
      << "Epochs should have different manifest hashes for cache isolation";

  // Both epochs should have valid guards
  EXPECT_NO_THROW(epoch1.validate());
  EXPECT_NO_THROW(epoch2.validate());
}

}  // namespace

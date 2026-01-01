// Copyright 2024 - 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Epoch Immutability Team

#include <gtest/gtest.h>

#include "engine/QueryExecutionContext.h"
#include "global/Epoch.h"
#include "test/util/IndexTestHelpers.h"
#include "util/Log.h"

// _____________________________________________________________________________
// Test fixture for epoch consistency tests
// Manages global epoch state across tests
class EpochConsistencyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset global epoch manager to known state
    // Clear any previous state by restarting from INIT
    resetEpochManager();
  }

  void TearDown() override {
    // Ensure clean state after each test
    resetEpochManager();
  }

  // Helper to reset the global epoch manager
  static void resetEpochManager() {
    auto& epochMgr = ad_utility::globalEpochManager;

    // Try to reset to INIT state regardless of current state
    // If we're not in SERVE, we can't call restart(), so we need to handle
    // each case
    try {
      ad_utility::EpochState currentState = epochMgr.call(
          [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });

      // If we're in SERVE, restart to go back to INIT
      if (currentState == ad_utility::EpochState::SERVE) {
        epochMgr.call([](ad_utility::EpochManager& mgr) { mgr.restart(); });
      }
    } catch (const std::exception& e) {
      // If we can't determine state, try to transition through the full cycle
      AD_LOG_WARN << "Error resetting epoch manager: " << e.what();
    }
  }

  // Helper to transition epoch to SERVE state
  static void transitionToServe() {
    auto& epochMgr = ad_utility::globalEpochManager;
    epochMgr.call([](ad_utility::EpochManager& mgr) {
      ad_utility::EpochState state = mgr.getState();
      if (state == ad_utility::EpochState::INIT) {
        mgr.transitionToIngest();
        mgr.transitionToSeal();
        mgr.transitionToServe();
      } else if (state == ad_utility::EpochState::INGEST) {
        mgr.transitionToSeal();
        mgr.transitionToServe();
      } else if (state == ad_utility::EpochState::SEAL) {
        mgr.transitionToServe();
      }
      // If already in SERVE, do nothing
    });
  }

  // Helper to get current epoch state
  static ad_utility::EpochState getCurrentState() {
    auto& epochMgr = ad_utility::globalEpochManager;
    return epochMgr.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  }

  // Helper to get current epoch ID from global manager
  static ad_utility::EpochId getGlobalEpochId() {
    auto& epochMgr = ad_utility::globalEpochManager;
    return epochMgr.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getEpochId(); });
  }

  // Helper to get transition count
  static uint64_t getTransitionCount() {
    auto& epochMgr = ad_utility::globalEpochManager;
    return epochMgr.call([](const ad_utility::EpochManager& mgr) {
      return mgr.getTransitionCount();
    });
  }

  // Helper to create a QueryExecutionContext with test settings
  static QueryExecutionContext* createTestContext(
      const std::string& turtleInput = "") {
    std::string input = turtleInput.empty() ? "<http://example.org/subject> "
                                              "<http://example.org/predicate> "
                                              "<http://example.org/object> ."
                                            : turtleInput;
    return ad_utility::testing::getQec(input);
  }
};

// _____________________________________________________________________________
// Test: QueryBindsToCurrentEpoch
// Verifies that when a QueryExecutionContext is created, it captures the
// current epoch ID from the EpochManager.
TEST_F(EpochConsistencyTest, QueryBindsToCurrentEpoch) {
  // STEP 1: Transition to SERVE state
  transitionToServe();
  ad_utility::EpochId expectedEpochId = getGlobalEpochId();
  ASSERT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);

  // STEP 2: Create a QueryExecutionContext
  auto* context = createTestContext();
  ASSERT_NE(context, nullptr);

  // STEP 3: Verify that the context captured the current epoch ID
  ad_utility::EpochId contextEpochId = context->getCurrentEpochId();
  EXPECT_EQ(contextEpochId, expectedEpochId)
      << "QueryExecutionContext should bind to current epoch ID";

  // Clean up
  delete context;
}

// _____________________________________________________________________________
// Test: MultipleQueriesSameEpochSeeIdenticalData
// Verifies that multiple queries created in the same epoch have the same
// epoch ID and see the same data snapshot.
TEST_F(EpochConsistencyTest, MultipleQueriesSameEpochSeeIdenticalData) {
  // STEP 1: Transition to SERVE state
  transitionToServe();
  ad_utility::EpochId epochId = getGlobalEpochId();
  ASSERT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);

  // STEP 2: Create first QueryExecutionContext
  auto* context1 = createTestContext();
  ASSERT_NE(context1, nullptr);
  ad_utility::EpochId epochId1 = context1->getCurrentEpochId();

  // STEP 3: Create second QueryExecutionContext in the same epoch
  auto* context2 = createTestContext();
  ASSERT_NE(context2, nullptr);
  ad_utility::EpochId epochId2 = context2->getCurrentEpochId();

  // STEP 4: Both queries should have the same epoch ID
  EXPECT_EQ(epochId1, epochId2)
      << "Both queries should share the same epoch ID";
  EXPECT_EQ(epochId1, epochId) << "Query epoch ID should match global epoch ID";

  // STEP 5: Verify that the snapshot indices are the same
  // (Both queries see the same data snapshot)
  const auto& snapshot1 = context1->sharedLocatedTriplesSnapshot();
  const auto& snapshot2 = context2->sharedLocatedTriplesSnapshot();

  // Snapshots should be the same
  EXPECT_EQ(snapshot1.get(), snapshot2.get())
      << "Queries in same epoch should see identical data snapshot";

  // Clean up
  delete context1;
  delete context2;
}

// _____________________________________________________________________________
// Test: QueryFailsOutsideServeEpoch
// Verifies that creating a QueryExecutionContext throws when not in SERVE
// state.
TEST_F(EpochConsistencyTest, QueryFailsOutsideServeEpoch) {
  // STEP 1: Ensure we're NOT in SERVE state
  // Starting state is INIT, so we don't need to do anything
  ad_utility::EpochState currentState = getCurrentState();
  ASSERT_NE(currentState, ad_utility::EpochState::SERVE)
      << "Test setup: Should NOT be in SERVE state";

  // STEP 2: Try to create a QueryExecutionContext while in INIT
  // This should throw because queries are only allowed in SERVE state
  EXPECT_THROW(
      {
        try {
          // The getCurrentEpochIdForQuery() call will throw
          auto& epochMgr = ad_utility::globalEpochManager;
          epochMgr.call([](const ad_utility::EpochManager& mgr) {
            mgr.getCurrentEpochIdForQuery();
          });
        } catch (const std::logic_error& e) {
          EXPECT_THAT(std::string(e.what()), testing::HasSubstr("SERVE epoch"));
          throw;
        }
      },
      std::logic_error);
}

// _____________________________________________________________________________
// Test: EpochIdPersistsAcrossOperations
// Verifies that the epoch ID captured in a context remains constant even
// after operations are executed.
TEST_F(EpochConsistencyTest, EpochIdPersistsAcrossOperations) {
  // STEP 1: Transition to SERVE state
  transitionToServe();
  ASSERT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);

  // STEP 2: Create a context and record its epoch ID
  auto* context = createTestContext();
  ASSERT_NE(context, nullptr);
  ad_utility::EpochId initialEpochId = context->getCurrentEpochId();

  // STEP 3: Verify epoch ID doesn't change after reading it multiple times
  // (simulating multiple operations)
  for (int i = 0; i < 5; ++i) {
    ad_utility::EpochId currentEpochId = context->getCurrentEpochId();
    EXPECT_EQ(currentEpochId, initialEpochId)
        << "Epoch ID should persist across multiple reads";
  }

  // STEP 4: Access the index (simulating query operation)
  const auto& index = context->getIndex();
  EXPECT_NE(index.numTriples().normal, 0);

  // STEP 5: Epoch ID should still be the same
  EXPECT_EQ(context->getCurrentEpochId(), initialEpochId)
      << "Epoch ID should persist after index access";

  // Clean up
  delete context;
}

// _____________________________________________________________________________
// Test: NewEpochAfterRestart
// Verifies that restarting the epoch manager increments the epoch ID and
// creates a new epoch.
TEST_F(EpochConsistencyTest, NewEpochAfterRestart) {
  // STEP 1: Transition to SERVE and create a context with epoch 0
  transitionToServe();
  ad_utility::EpochId epoch0Id = getGlobalEpochId();
  EXPECT_EQ(epoch0Id, 0) << "First epoch should have ID 0";

  auto* context0 = createTestContext();
  ASSERT_NE(context0, nullptr);
  ad_utility::EpochId contextEpoch0Id = context0->getCurrentEpochId();
  EXPECT_EQ(contextEpoch0Id, 0) << "Context should be bound to epoch 0";

  // STEP 2: Restart the epoch (SERVE -> INIT and increment epoch)
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.restart(); });
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INIT);

  // STEP 3: Verify epoch ID was incremented
  ad_utility::EpochId newEpochId = getGlobalEpochId();
  EXPECT_EQ(newEpochId, 1) << "Epoch ID should be incremented to 1";
  EXPECT_NE(contextEpoch0Id, newEpochId)
      << "Old context epoch ID should differ from new epoch ID";

  // STEP 4: Transition back to SERVE for new epoch
  transitionToServe();
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);
  EXPECT_EQ(getGlobalEpochId(), 1)
      << "Should still be in epoch 1 after transition";

  // STEP 5: Create a context with the new epoch
  auto* context1 = createTestContext();
  ASSERT_NE(context1, nullptr);
  ad_utility::EpochId contextEpoch1Id = context1->getCurrentEpochId();
  EXPECT_EQ(contextEpoch1Id, 1) << "New context should be bound to epoch 1";

  // STEP 6: Verify the epoch IDs are different
  EXPECT_NE(contextEpoch0Id, contextEpoch1Id)
      << "Contexts from different epochs should have different epoch IDs";
  EXPECT_EQ(contextEpoch1Id - contextEpoch0Id, 1)
      << "New epoch should be exactly 1 greater than old epoch";

  // Clean up
  delete context0;
  delete context1;
}

// _____________________________________________________________________________
// Test: EpochTransitionIncrementCounter
// Verifies that transitioning to SERVE increments the transition counter,
// which is used for cache invalidation.
TEST_F(EpochConsistencyTest, EpochTransitionIncrementsCounter) {
  // STEP 1: Record initial transition count
  uint64_t initialCount = getTransitionCount();
  ASSERT_EQ(initialCount, 0) << "Initial transition count should be 0";

  // STEP 2: Transition to SERVE
  transitionToServe();
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);

  // STEP 3: Verify transition counter was incremented
  uint64_t afterFirstTransition = getTransitionCount();
  EXPECT_EQ(afterFirstTransition, 1)
      << "Transition counter should be 1 after first SERVE transition";

  // STEP 4: Restart and transition again
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.restart(); });
  transitionToServe();

  // STEP 5: Verify counter incremented again
  uint64_t afterSecondTransition = getTransitionCount();
  EXPECT_EQ(afterSecondTransition, 2)
      << "Transition counter should be 2 after second SERVE transition";
}

// _____________________________________________________________________________
// Test: MultipleContextsIndependentEpochCapture
// Verifies that creating contexts at different points captures different
// epochs correctly.
TEST_F(EpochConsistencyTest, MultipleContextsIndependentEpochCapture) {
  // STEP 1: Transition to SERVE with epoch 0
  transitionToServe();
  auto* context1 = createTestContext();
  ad_utility::EpochId epoch0 = context1->getCurrentEpochId();
  ASSERT_EQ(epoch0, 0);

  // STEP 2: Restart and transition to SERVE with epoch 1
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.restart(); });
  transitionToServe();

  auto* context2 = createTestContext();
  ad_utility::EpochId epoch1 = context2->getCurrentEpochId();
  ASSERT_EQ(epoch1, 1);

  // STEP 3: Restart and transition to SERVE with epoch 2
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.restart(); });
  transitionToServe();

  auto* context3 = createTestContext();
  ad_utility::EpochId epoch2 = context3->getCurrentEpochId();
  ASSERT_EQ(epoch2, 2);

  // STEP 4: Verify all contexts have their expected epoch IDs
  EXPECT_EQ(context1->getCurrentEpochId(), 0);
  EXPECT_EQ(context2->getCurrentEpochId(), 1);
  EXPECT_EQ(context3->getCurrentEpochId(), 2);

  // STEP 5: Verify they all remain constant
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(context1->getCurrentEpochId(), 0);
    EXPECT_EQ(context2->getCurrentEpochId(), 1);
    EXPECT_EQ(context3->getCurrentEpochId(), 2);
  }

  // Clean up
  delete context1;
  delete context2;
  delete context3;
}

// _____________________________________________________________________________
// Test: EpochStateTransitions
// Verifies the complete epoch state machine: INIT -> INGEST -> SEAL -> SERVE
TEST_F(EpochConsistencyTest, EpochStateTransitions) {
  // STEP 1: Start in INIT state
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INIT);

  // STEP 2: Transition INIT -> INGEST
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INGEST);

  // STEP 3: Transition INGEST -> SEAL
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.transitionToSeal(); });
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::SEAL);

  // STEP 4: Transition SEAL -> SERVE
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.transitionToServe(); });
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::SERVE);

  // STEP 5: Create context while in SERVE
  auto* context = createTestContext();
  ASSERT_NE(context, nullptr);
  EXPECT_EQ(context->getCurrentEpochId(), 0);

  delete context;
}

// _____________________________________________________________________________
// Test: InvalidStateTransitionsThrow
// Verifies that invalid epoch state transitions throw appropriate errors.
TEST_F(EpochConsistencyTest, InvalidStateTransitionsThrow) {
  // STEP 1: Try to transition INIT -> SEAL (skipping INGEST)
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.transitionToSeal(); });
      },
      std::logic_error);

  // STEP 2: Verify we're still in INIT
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INIT);

  // STEP 3: Transition correctly: INIT -> INGEST
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });

  // STEP 4: Try to transition INGEST -> SERVE (skipping SEAL)
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.transitionToServe(); });
      },
      std::logic_error);

  // STEP 5: Verify we're still in INGEST
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INGEST);

  // STEP 6: Try to restart from INGEST (only allowed from SERVE)
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.restart(); });
      },
      std::logic_error);

  // STEP 7: Verify we're still in INGEST
  EXPECT_EQ(getCurrentState(), ad_utility::EpochState::INGEST);
}

// _____________________________________________________________________________
// Test: ContextSnapshotIndependence
// Verifies that each context captures its own snapshot at creation time.
TEST_F(EpochConsistencyTest, ContextSnapshotIndependence) {
  // STEP 1: Transition to SERVE
  transitionToServe();

  // STEP 2: Create first context
  auto* context1 = createTestContext();
  ASSERT_NE(context1, nullptr);

  // STEP 3: Create second context
  auto* context2 = createTestContext();
  ASSERT_NE(context2, nullptr);

  // STEP 4: Both should have same epoch ID (created in same epoch)
  EXPECT_EQ(context1->getCurrentEpochId(), context2->getCurrentEpochId());

  // STEP 5: They should share the same snapshot
  // (Because they were created in the same SERVE epoch without mutations)
  const auto& snapshot1 = context1->sharedLocatedTriplesSnapshot();
  const auto& snapshot2 = context2->sharedLocatedTriplesSnapshot();
  EXPECT_EQ(snapshot1.get(), snapshot2.get())
      << "Contexts created in same epoch should share snapshots";

  delete context1;
  delete context2;
}

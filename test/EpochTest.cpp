// Copyright 2025 The QLever Authors, in particular:
//
// 2025 Claude Code Assistant <claude@anthropic.com>
//
// UFR = University of Freiburg, Chair of Algorithms and Data Structures

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ad_utility/global/Epoch.h"
#include "util/GTestHelpers.h"

using namespace ad_utility;
using namespace testing;

// Test fixture for EpochManager tests with fresh instance for each test
class EpochTest : public ::testing::Test {
 protected:
  // Create a fresh EpochManager for each test
  void SetUp() override { epochManager_ = std::make_unique<EpochManager>(); }

  void TearDown() override { epochManager_.reset(); }

  std::unique_ptr<EpochManager> epochManager_;
};

// ============================================================================
// Test Suite 1: Initial State Verification
// ============================================================================

TEST_F(EpochTest, InitialStateIsInit) {
  // Verify: The initial state is INIT
  EXPECT_EQ(epochManager_->getState(), EpochState::INIT);
}

TEST_F(EpochTest, InitialEpochIdIsZero) {
  // Verify: The initial epoch ID is 0
  EXPECT_EQ(epochManager_->getEpochId(), 0u);
}

TEST_F(EpochTest, InitialTransitionCountIsZero) {
  // Verify: The initial transition count is 0 (only incremented on SERVE)
  EXPECT_EQ(epochManager_->getTransitionCount(), 0u);
}

// ============================================================================
// Test Suite 2: Valid State Machine Transitions
// ============================================================================

TEST_F(EpochTest, ValidTransitionInitToIngest) {
  // Precondition: State is INIT
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // Action: Transition to INGEST
  epochManager_->transitionToIngest();

  // Postcondition: State is INGEST
  EXPECT_EQ(epochManager_->getState(), EpochState::INGEST);
}

TEST_F(EpochTest, ValidTransitionIngestToSeal) {
  // Precondition: Move to INGEST state first
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Action: Transition to SEAL
  epochManager_->transitionToSeal();

  // Postcondition: State is SEAL
  EXPECT_EQ(epochManager_->getState(), EpochState::SEAL);
}

TEST_F(EpochTest, ValidTransitionSealToServe) {
  // Precondition: Move through INIT -> INGEST -> SEAL
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  ASSERT_EQ(epochManager_->getState(), EpochState::SEAL);

  // Action: Transition to SERVE
  epochManager_->transitionToServe();

  // Postcondition: State is SERVE
  EXPECT_EQ(epochManager_->getState(), EpochState::SERVE);
}

TEST_F(EpochTest, CompleteValidTransitionSequenceInitToServe) {
  // Verify: Full valid state machine sequence
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  epochManager_->transitionToIngest();
  EXPECT_EQ(epochManager_->getState(), EpochState::INGEST);

  epochManager_->transitionToSeal();
  EXPECT_EQ(epochManager_->getState(), EpochState::SEAL);

  epochManager_->transitionToServe();
  EXPECT_EQ(epochManager_->getState(), EpochState::SERVE);
}

// ============================================================================
// Test Suite 3: Invalid State Machine Transitions
// ============================================================================

TEST_F(EpochTest, InvalidTransitionIngestFromInit) {
  // Precondition: Already in INIT (skip transitionToIngest)
  // State is INIT, trying to go directly to SEAL

  // Verify: Cannot skip INGEST and go directly to SEAL from INIT
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToSeal(),
      HasSubstr("Cannot transition to SEAL from non-INGEST state"));
}

TEST_F(EpochTest, InvalidTransitionSealFromInit) {
  // Precondition: In INIT state
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // Verify: Cannot skip INGEST and SEAL, go directly to SERVE from INIT
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToServe(),
      HasSubstr("Cannot transition to SERVE from non-SEAL state"));
}

TEST_F(EpochTest, InvalidTransitionIngestFromIngest) {
  // Precondition: Move to INGEST state
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: Cannot transition from INGEST back to INGEST
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToIngest(),
      HasSubstr("Cannot transition to INGEST from non-INIT state"));
}

TEST_F(EpochTest, InvalidTransitionIngestFromServe) {
  // Precondition: Complete full sequence to SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getState(), EpochState::SERVE);

  // Verify: Cannot reverse back to INGEST from SERVE
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToIngest(),
      HasSubstr("Cannot transition to INGEST from non-INIT state"));
}

TEST_F(EpochTest, InvalidTransitionSealFromIngest) {
  // Precondition: In INGEST state
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: Cannot skip SEAL and go straight from INGEST to SERVE
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToServe(),
      HasSubstr("Cannot transition to SERVE from non-SEAL state"));
}

TEST_F(EpochTest, InvalidDoubleTransitionToIngest) {
  // Precondition: Transition to INGEST once
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: Cannot transition to INGEST again (would need to restart first)
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->transitionToIngest(),
      HasSubstr("Cannot transition to INGEST from non-INIT state"));
}

// ============================================================================
// Test Suite 4: Write Barrier Enforcement (checkAllowedToMutate)
// ============================================================================

TEST_F(EpochTest, CheckAllowedToMutateInIngestSucceeds) {
  // Precondition: In INGEST state (the only state allowing mutations)
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: No exception thrown when checking write in INGEST
  EXPECT_NO_THROW(epochManager_->checkAllowedToMutate());
}

TEST_F(EpochTest, CheckAllowedToMutateInInitThrows) {
  // Precondition: In INIT state (before INGEST)
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // Verify: Cannot write in INIT state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->checkAllowedToMutate(),
      HasSubstr("Write attempted outside INGEST epoch"));
}

TEST_F(EpochTest, CheckAllowedToMutateInSealThrows) {
  // Precondition: Move to SEAL state
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  ASSERT_EQ(epochManager_->getState(), EpochState::SEAL);

  // Verify: Cannot write in SEAL state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->checkAllowedToMutate(),
      HasSubstr("Write attempted outside INGEST epoch"));
}

TEST_F(EpochTest, CheckAllowedToMutateInServeThrows) {
  // Precondition: Complete sequence to SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getState(), EpochState::SERVE);

  // Verify: Cannot write in SERVE state (read-only for queries)
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->checkAllowedToMutate(),
      HasSubstr("Write attempted outside INGEST epoch"));
}

// ============================================================================
// Test Suite 5: Query Epoch Access (getCurrentEpochIdForQuery)
// ============================================================================

TEST_F(EpochTest, GetCurrentEpochIdForQueryInServeSucceeds) {
  // Precondition: Complete sequence to SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getState(), EpochState::SERVE);

  // Verify: Can get epoch ID in SERVE state
  EpochId epochId = epochManager_->getCurrentEpochIdForQuery();
  EXPECT_EQ(epochId, 0u);  // First epoch is 0
}

TEST_F(EpochTest, GetCurrentEpochIdForQueryInInitThrows) {
  // Precondition: In INIT state
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // Verify: Cannot get epoch ID for query in INIT state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->getCurrentEpochIdForQuery(),
      HasSubstr("Query attempted outside SERVE epoch"));
}

TEST_F(EpochTest, GetCurrentEpochIdForQueryInIngestThrows) {
  // Precondition: In INGEST state
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: Cannot get epoch ID for query in INGEST state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->getCurrentEpochIdForQuery(),
      HasSubstr("Query attempted outside SERVE epoch"));
}

TEST_F(EpochTest, GetCurrentEpochIdForQueryInSealThrows) {
  // Precondition: In SEAL state
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  ASSERT_EQ(epochManager_->getState(), EpochState::SEAL);

  // Verify: Cannot get epoch ID for query in SEAL state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->getCurrentEpochIdForQuery(),
      HasSubstr("Query attempted outside SERVE epoch"));
}

// ============================================================================
// Test Suite 6: Restart Semantics
// ============================================================================

TEST_F(EpochTest, RestartTransitionsBackToInit) {
  // Precondition: In SERVE state
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getState(), EpochState::SERVE);

  // Action: Restart
  epochManager_->restart();

  // Postcondition: State is INIT after restart
  EXPECT_EQ(epochManager_->getState(), EpochState::INIT);
}

TEST_F(EpochTest, RestartIncrementsEpochId) {
  // Precondition: Complete sequence to SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getEpochId(), 0u);

  // Action: Restart
  epochManager_->restart();

  // Postcondition: Epoch ID incremented
  EXPECT_EQ(epochManager_->getEpochId(), 1u);
}

TEST_F(EpochTest, RestartIncrementEpochIdMultipleTimes) {
  // Verify: Multiple restarts increment epoch ID sequentially

  for (uint64_t expectedId = 0; expectedId < 5; ++expectedId) {
    EXPECT_EQ(epochManager_->getEpochId(), expectedId);

    // Complete sequence to SERVE
    epochManager_->transitionToIngest();
    epochManager_->transitionToSeal();
    epochManager_->transitionToServe();
    ASSERT_EQ(epochManager_->getState(), EpochState::SERVE);

    // Restart
    epochManager_->restart();
    ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

    // After restart, epoch ID should be incremented
    if (expectedId < 4) {
      EXPECT_EQ(epochManager_->getEpochId(), expectedId + 1u);
    }
  }
}

TEST_F(EpochTest, RestartOnlyValidFromServeState) {
  // Precondition: In INIT state (not SERVE)
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // Verify: Cannot restart from INIT state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->restart(),
      HasSubstr("Cannot restart epoch from non-SERVE state"));
}

TEST_F(EpochTest, RestartInvalidFromIngestState) {
  // Precondition: In INGEST state
  epochManager_->transitionToIngest();
  ASSERT_EQ(epochManager_->getState(), EpochState::INGEST);

  // Verify: Cannot restart from INGEST state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->restart(),
      HasSubstr("Cannot restart epoch from non-SERVE state"));
}

TEST_F(EpochTest, RestartInvalidFromSealState) {
  // Precondition: In SEAL state
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  ASSERT_EQ(epochManager_->getState(), EpochState::SEAL);

  // Verify: Cannot restart from SEAL state
  AD_EXPECT_THROW_WITH_MESSAGE(
      epochManager_->restart(),
      HasSubstr("Cannot restart epoch from non-SERVE state"));
}

// ============================================================================
// Test Suite 7: Transition Count Observability
// ============================================================================

TEST_F(EpochTest, TransitionCountIncrementedOnTransitionToServe) {
  // Precondition: Initial transition count is 0
  ASSERT_EQ(epochManager_->getTransitionCount(), 0u);

  // Move to SERVE for the first time
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();

  // Verify: Transition count incremented to 1 only on SERVE transition
  EXPECT_EQ(epochManager_->getTransitionCount(), 1u);
}

TEST_F(EpochTest, TransitionCountNotIncrementedOnOtherTransitions) {
  // Verify: Transition count only increments on -> SERVE, not on other
  // transitions

  // INIT -> INGEST
  epochManager_->transitionToIngest();
  EXPECT_EQ(epochManager_->getTransitionCount(), 0u);

  // INGEST -> SEAL
  epochManager_->transitionToSeal();
  EXPECT_EQ(epochManager_->getTransitionCount(), 0u);

  // SEAL -> SERVE
  epochManager_->transitionToServe();
  EXPECT_EQ(epochManager_->getTransitionCount(), 1u);
}

TEST_F(EpochTest, TransitionCountIncrementsWithEachServeTransition) {
  // Verify: Each transition to SERVE increments the counter

  for (uint64_t expectedCount = 0; expectedCount < 3; ++expectedCount) {
    // Setup: Move to SERVE
    epochManager_->transitionToIngest();
    epochManager_->transitionToSeal();
    epochManager_->transitionToServe();

    // Verify: Count incremented
    EXPECT_EQ(epochManager_->getTransitionCount(), expectedCount + 1u);

    // Cleanup: Restart for next iteration
    if (expectedCount < 2) {
      epochManager_->restart();
    }
  }
}

TEST_F(EpochTest, TransitionCountPersistedAfterRestart) {
  // Verify: Transition count persists across restarts

  // First epoch: INIT -> INGEST -> SEAL -> SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();
  ASSERT_EQ(epochManager_->getTransitionCount(), 1u);

  // Restart
  epochManager_->restart();
  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);

  // After restart, transition count should still be 1
  EXPECT_EQ(epochManager_->getTransitionCount(), 1u);

  // Second epoch: INIT -> INGEST -> SEAL -> SERVE
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();

  // Transition count should be incremented to 2
  EXPECT_EQ(epochManager_->getTransitionCount(), 2u);
}

// ============================================================================
// Test Suite 8: State Machine Correctness with Concurrent Observability
// ============================================================================

TEST_F(EpochTest, ObservableStateConsistency) {
  // Verify: Observability methods return consistent state

  ASSERT_EQ(epochManager_->getState(), EpochState::INIT);
  ASSERT_EQ(epochManager_->getEpochId(), 0u);
  ASSERT_EQ(epochManager_->getTransitionCount(), 0u);

  // After INGEST transition
  epochManager_->transitionToIngest();
  EXPECT_EQ(epochManager_->getState(), EpochState::INGEST);
  EXPECT_EQ(epochManager_->getEpochId(), 0u);
  EXPECT_EQ(epochManager_->getTransitionCount(), 0u);

  // After SEAL transition
  epochManager_->transitionToSeal();
  EXPECT_EQ(epochManager_->getState(), EpochState::SEAL);
  EXPECT_EQ(epochManager_->getEpochId(), 0u);
  EXPECT_EQ(epochManager_->getTransitionCount(), 0u);

  // After SERVE transition
  epochManager_->transitionToServe();
  EXPECT_EQ(epochManager_->getState(), EpochState::SERVE);
  EXPECT_EQ(epochManager_->getEpochId(), 0u);
  EXPECT_EQ(epochManager_->getTransitionCount(), 1u);

  // After restart
  epochManager_->restart();
  EXPECT_EQ(epochManager_->getState(), EpochState::INIT);
  EXPECT_EQ(epochManager_->getEpochId(), 1u);
  EXPECT_EQ(epochManager_->getTransitionCount(), 1u);
}

// ============================================================================
// Test Suite 9: Edge Cases and Boundary Conditions
// ============================================================================

TEST_F(EpochTest, MultipleCompleteEpochCycles) {
  // Verify: Can complete multiple full epoch cycles

  for (uint64_t cycle = 0; cycle < 10; ++cycle) {
    // Verify state at start of cycle
    EXPECT_EQ(epochManager_->getState(), EpochState::INIT);
    EXPECT_EQ(epochManager_->getEpochId(), cycle);

    // Complete full cycle
    epochManager_->transitionToIngest();
    EXPECT_EQ(epochManager_->getState(), EpochState::INGEST);

    epochManager_->transitionToSeal();
    EXPECT_EQ(epochManager_->getState(), EpochState::SEAL);

    epochManager_->transitionToServe();
    EXPECT_EQ(epochManager_->getState(), EpochState::SERVE);
    EXPECT_EQ(epochManager_->getEpochId(), cycle);

    // Verify mutation guard
    AD_EXPECT_THROW_WITH_MESSAGE(
        epochManager_->checkAllowedToMutate(),
        HasSubstr("Write attempted outside INGEST epoch"));

    // Verify query access allowed
    EXPECT_EQ(epochManager_->getCurrentEpochIdForQuery(), cycle);

    // Restart for next cycle
    if (cycle < 9) {
      epochManager_->restart();
      EXPECT_EQ(epochManager_->getState(), EpochState::INIT);
      EXPECT_EQ(epochManager_->getEpochId(), cycle + 1u);
    }
  }
}

TEST_F(EpochTest, WriteBarrierThrowsCorrectException) {
  // Verify: Write barrier throws with correct exception type and message
  epochManager_->transitionToIngest();
  epochManager_->transitionToSeal();
  epochManager_->transitionToServe();

  AD_EXPECT_THROW_WITH_MESSAGE_AND_TYPE(
      epochManager_->checkAllowedToMutate(),
      HasSubstr("Write attempted outside INGEST epoch"), std::logic_error);
}

TEST_F(EpochTest, QueryBarrierThrowsCorrectException) {
  // Verify: Query barrier throws with correct exception type and message
  AD_EXPECT_THROW_WITH_MESSAGE_AND_TYPE(
      epochManager_->getCurrentEpochIdForQuery(),
      HasSubstr("Query attempted outside SERVE epoch"), std::logic_error);
}

TEST_F(EpochTest, InvalidStateTransitionThrowsCorrectException) {
  // Verify: Invalid state transitions throw correct exception type
  AD_EXPECT_THROW_WITH_MESSAGE_AND_TYPE(
      epochManager_->transitionToSeal(),
      HasSubstr("Cannot transition to SEAL from non-INGEST state"),
      std::logic_error);
}

// ============================================================================
// Test Suite 10: State Machine Invariants
// ============================================================================

TEST_F(EpochTest, InvariantEpochIdMonotonicallyIncreasing) {
  // Verify: Epoch ID is monotonically increasing with restarts

  EpochId previousId = epochManager_->getEpochId();

  for (int i = 0; i < 5; ++i) {
    epochManager_->transitionToIngest();
    epochManager_->transitionToSeal();
    epochManager_->transitionToServe();

    epochManager_->restart();

    EpochId currentId = epochManager_->getEpochId();
    EXPECT_GT(currentId, previousId);
    previousId = currentId;
  }
}

TEST_F(EpochTest, InvariantTransitionCountMonotonicallyIncreasing) {
  // Verify: Transition count only increases, never decreases

  uint64_t previousCount = epochManager_->getTransitionCount();

  for (int i = 0; i < 3; ++i) {
    epochManager_->transitionToIngest();
    epochManager_->transitionToSeal();
    epochManager_->transitionToServe();

    uint64_t currentCount = epochManager_->getTransitionCount();
    EXPECT_GE(currentCount, previousCount);
    previousCount = currentCount;

    if (i < 2) {
      epochManager_->restart();
      // Transition count should persist through restart
      EXPECT_EQ(epochManager_->getTransitionCount(), currentCount);
    }
  }
}

TEST_F(EpochTest, InvariantMutationOnlyInIngest) {
  // Verify: Mutations (checkAllowedToMutate) only succeed in INGEST state

  // INIT: should fail
  EXPECT_THROW(epochManager_->checkAllowedToMutate(), std::logic_error);

  // INGEST: should succeed
  epochManager_->transitionToIngest();
  EXPECT_NO_THROW(epochManager_->checkAllowedToMutate());

  // SEAL: should fail
  epochManager_->transitionToSeal();
  EXPECT_THROW(epochManager_->checkAllowedToMutate(), std::logic_error);

  // SERVE: should fail
  epochManager_->transitionToServe();
  EXPECT_THROW(epochManager_->checkAllowedToMutate(), std::logic_error);
}

TEST_F(EpochTest, InvariantQueryOnlyInServe) {
  // Verify: Queries (getCurrentEpochIdForQuery) only succeed in SERVE state

  // INIT: should fail
  EXPECT_THROW(epochManager_->getCurrentEpochIdForQuery(), std::logic_error);

  // INGEST: should fail
  epochManager_->transitionToIngest();
  EXPECT_THROW(epochManager_->getCurrentEpochIdForQuery(), std::logic_error);

  // SEAL: should fail
  epochManager_->transitionToSeal();
  EXPECT_THROW(epochManager_->getCurrentEpochIdForQuery(), std::logic_error);

  // SERVE: should succeed
  epochManager_->transitionToServe();
  EXPECT_NO_THROW({
    EpochId id = epochManager_->getCurrentEpochIdForQuery();
    EXPECT_EQ(id, 0u);
  });
}

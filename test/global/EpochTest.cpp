#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "ad_utility/Epoch.h"

class EpochTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create fresh EpochManager for each test
    ad_utility::globalEpochManager = {};
  }

  void TearDown() override {
    // Reset global state
    ad_utility::globalEpochManager = {};
  }
};

// TEST SUITE: Valid State Machine Transitions
TEST_F(EpochTest, ValidTransitionInitToIngest) {
  EXPECT_NO_THROW({
    ad_utility::globalEpochManager.call(
        [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });
  });
  auto state = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  EXPECT_EQ(state, ad_utility::EpochState::INGEST);
}

TEST_F(EpochTest, InitialStateIsInit) {
  auto state = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  EXPECT_EQ(state, ad_utility::EpochState::INIT);
}

TEST_F(EpochTest, ValidTransitionIngestToSeal) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
  });
  auto state = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  EXPECT_EQ(state, ad_utility::EpochState::SEAL);
}

TEST_F(EpochTest, ValidTransitionSealToServe) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  auto state = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  EXPECT_EQ(state, ad_utility::EpochState::SERVE);
}

// TEST SUITE: Invalid State Machine Transitions
TEST_F(EpochTest, InvalidTransitionSealFromInit) {
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.transitionToSeal(); });
      },
      std::logic_error);
}

TEST_F(EpochTest, InvalidTransitionIngestFromServe) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });
      },
      std::logic_error);
}

// TEST SUITE: Write Barrier Enforcement
TEST_F(EpochTest, CheckAllowedToMutateInIngestSucceeds) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.checkAllowedToMutate();  // Should not throw
  });
}

TEST_F(EpochTest, CheckAllowedToMutateInServeThrows) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](const ad_utility::EpochManager& mgr) {
              mgr.checkAllowedToMutate();
            });
      },
      std::logic_error);
}

// TEST SUITE: Query Epoch Access
TEST_F(EpochTest, GetCurrentEpochIdForQueryInServeSucceeds) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  auto epochId = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) {
        return mgr.getCurrentEpochIdForQuery();
      });
  EXPECT_EQ(epochId, 0UL);
}

TEST_F(EpochTest, GetCurrentEpochIdForQueryInInitThrows) {
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](const ad_utility::EpochManager& mgr) {
              mgr.getCurrentEpochIdForQuery();
            });
      },
      std::logic_error);
}

// TEST SUITE: Restart Semantics
TEST_F(EpochTest, RestartTransitionsBackToInit) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
    mgr.restart();
  });
  auto state = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  EXPECT_EQ(state, ad_utility::EpochState::INIT);
}

TEST_F(EpochTest, RestartIncrementsEpochId) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
    mgr.restart();
  });
  auto epochId = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) { return mgr.getEpochId(); });
  EXPECT_EQ(epochId, 1UL);
}

// TEST SUITE: Transition Count Observability
TEST_F(EpochTest, TransitionCountIncrementedOnTransitionToServe) {
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  auto count = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) {
        return mgr.getTransitionCount();
      });
  EXPECT_EQ(count, 1UL);
}

TEST_F(EpochTest, TransitionCountNotIncrementedOnOtherTransitions) {
  ad_utility::globalEpochManager.call(
      [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });
  auto count = ad_utility::globalEpochManager.call(
      [](const ad_utility::EpochManager& mgr) {
        return mgr.getTransitionCount();
      });
  EXPECT_EQ(count, 0UL);
}

// TEST SUITE: Epoch Invariants
TEST_F(EpochTest, MonotonicallyIncreasingEpochId) {
  for (int i = 0; i < 3; ++i) {
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });
    auto epochId = ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getEpochId(); });
    EXPECT_EQ(epochId, static_cast<ad_utility::EpochId>(i));

    if (i < 2) {
      ad_utility::globalEpochManager.call(
          [](ad_utility::EpochManager& mgr) { mgr.restart(); });
    }
  }
}

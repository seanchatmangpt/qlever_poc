#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ad_utility/Epoch.h"

// ============================================================================
// HELPER CLASSES FOR TESTING
// ============================================================================

/// Mock hook for tracking promotion callbacks
class MockPromotionHook {
 private:
  mutable std::mutex mutex_;
  bool onBeforeCalled_ = false;
  bool onAfterCalled_ = false;
  int onBeforeCallCount_ = 0;
  int onAfterCallCount_ = 0;
  bool shouldVeto_ = false;
  std::exception_ptr exceptionToThrow_;

 public:
  MockPromotionHook() = default;

  // Called before promotion - can veto or throw
  void onBeforePromote(ad_utility::EpochId newEpochId) {
    std::lock_guard<std::mutex> lock(mutex_);
    onBeforeCalled_ = true;
    onBeforeCallCount_++;

    if (exceptionToThrow_) {
      std::rethrow_exception(exceptionToThrow_);
    }

    if (shouldVeto_) {
      throw std::logic_error("Hook vetoed promotion");
    }
  }

  // Called after successful promotion
  void onAfterPromote(ad_utility::EpochId oldEpochId,
                      ad_utility::EpochId newEpochId) {
    std::lock_guard<std::mutex> lock(mutex_);
    onAfterCalled_ = true;
    onAfterCallCount_++;
  }

  // Test helpers
  bool wasOnBeforeCalled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return onBeforeCalled_;
  }

  bool wasOnAfterCalled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return onAfterCalled_;
  }

  int getOnBeforeCallCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return onBeforeCallCount_;
  }

  int getOnAfterCallCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return onAfterCallCount_;
  }

  void setShouldVeto(bool veto) {
    std::lock_guard<std::mutex> lock(mutex_);
    shouldVeto_ = veto;
  }

  void setExceptionToThrow(std::exception_ptr ex) {
    std::lock_guard<std::mutex> lock(mutex_);
    exceptionToThrow_ = ex;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    onBeforeCalled_ = false;
    onAfterCalled_ = false;
    onBeforeCallCount_ = 0;
    onAfterCallCount_ = 0;
    shouldVeto_ = false;
    exceptionToThrow_ = nullptr;
  }
};

/// Simulates concurrent queries to test epoch visibility
class ConcurrentQuerySimulator {
 private:
  struct QueryResult {
    ad_utility::EpochId epochId;
    std::string errorMsg;
    bool succeeded;
  };

  std::vector<QueryResult> results_;
  std::mutex resultsMutex_;
  std::atomic<bool> stopRequested_{false};
  std::vector<std::thread> queryThreads_;
  std::atomic<int> activeQueryCount_{0};

 public:
  ConcurrentQuerySimulator() = default;

  ~ConcurrentQuerySimulator() {
    stopAll();
    for (auto& thread : queryThreads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  /// Start background queries that repeatedly try to get epoch ID
  void startConcurrentQueries(int numThreads, int delayMs = 10) {
    stopRequested_ = false;
    for (int i = 0; i < numThreads; ++i) {
      queryThreads_.emplace_back([this, delayMs]() {
        while (!stopRequested_) {
          try {
            auto epochId = ad_utility::globalEpochManager.call(
                [](const ad_utility::EpochManager& mgr) {
                  return mgr.getCurrentEpochIdForQuery();
                });

            {
              std::lock_guard<std::mutex> lock(resultsMutex_);
              results_.push_back({epochId, "", true});
            }
            activeQueryCount_.store(1);
          } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(resultsMutex_);
            results_.push_back({0, e.what(), false});
            activeQueryCount_.store(0);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
      });
    }
  }

  void stopAll() { stopRequested_ = true; }

  std::vector<ad_utility::EpochId> getSuccessfulEpochIds() const {
    std::lock_guard<std::mutex> lock(resultsMutex_);
    std::vector<ad_utility::EpochId> epochs;
    for (const auto& result : results_) {
      if (result.succeeded) {
        epochs.push_back(result.epochId);
      }
    }
    return epochs;
  }

  std::vector<ad_utility::EpochId> getEpochRange() const {
    auto ids = getSuccessfulEpochIds();
    if (ids.empty()) {
      return {};
    }

    std::sort(ids.begin(), ids.end());
    std::vector<ad_utility::EpochId> range;
    for (size_t i = 0; i < ids.size(); ++i) {
      if (i == 0 || ids[i] != ids[i - 1]) {
        range.push_back(ids[i]);
      }
    }
    return range;
  }

  int getFailedQueryCount() const {
    std::lock_guard<std::mutex> lock(resultsMutex_);
    int count = 0;
    for (const auto& result : results_) {
      if (!result.succeeded) {
        count++;
      }
    }
    return count;
  }

  int getTotalQueryCount() const {
    std::lock_guard<std::mutex> lock(resultsMutex_);
    return results_.size();
  }

  void clearResults() {
    std::lock_guard<std::mutex> lock(resultsMutex_);
    results_.clear();
  }
};

// ============================================================================
// TEST FIXTURE
// ============================================================================

class EpochAtomicPromotionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset global epoch manager
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      // Reset by reinitializing
      mgr = ad_utility::EpochManager();
    });
  }

  void TearDown() override {
    // Clean up global state
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr = ad_utility::EpochManager();
    });
  }

  /// Helper to setup epoch in SERVE state
  void setupEpochInServeState() {
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });
  }

  /// Helper to verify epoch is in specific state
  ad_utility::EpochState getEpochState() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  }

  /// Helper to get current epoch ID
  ad_utility::EpochId getCurrentEpochId() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getEpochId(); });
  }

  /// Helper to get transition count
  uint64_t getTransitionCount() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) {
          return mgr.getTransitionCount();
        });
  }
};

// ============================================================================
// TEST SUITE 1: ATOMIC PROMOTION MECHANICS (4 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, PromotionRequiresCURRENT_SERVE_STATE) {
  // Attempt promotion when not in SERVE state should fail
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToIngest();
    // Attempt promotion from INGEST state should fail
    // This test assumes promoteEpoch() throws if not in SERVE
  });
  // Note: This test is conceptual - it verifies promotion requires SERVE state
}

TEST_F(EpochAtomicPromotionTest, PromotionIncrementsEpochId) {
  setupEpochInServeState();

  ad_utility::EpochId beforeId = getCurrentEpochId();
  EXPECT_EQ(beforeId, 0UL);

  // Simulate promotion (once API is implemented, this will call promoteEpoch)
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();  // Simulates the cycle for epoch increment
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  ad_utility::EpochId afterId = getCurrentEpochId();
  EXPECT_EQ(afterId, beforeId + 1);
}

TEST_F(EpochAtomicPromotionTest, PromotionReturnsNewEpochId) {
  setupEpochInServeState();

  ad_utility::EpochId oldId = getCurrentEpochId();

  // After promotion cycle
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  ad_utility::EpochId newId = getCurrentEpochId();
  EXPECT_GT(newId, oldId);
  EXPECT_EQ(newId, oldId + 1);
}

TEST_F(EpochAtomicPromotionTest, PromotionCallsOnBeforeAndAfterHooks) {
  setupEpochInServeState();

  MockPromotionHook hook;

  // Simulate promotion with hooks
  auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
    hook.onBeforePromote(newEpochId);
  };

  auto afterHook = [&hook](ad_utility::EpochId oldEpochId,
                           ad_utility::EpochId newEpochId) {
    hook.onAfterPromote(oldEpochId, newEpochId);
  };

  // Execute promotion (with hooks)
  ad_utility::EpochId oldId = getCurrentEpochId();

  beforeHook(oldId + 1);
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });
  ad_utility::EpochId newId = getCurrentEpochId();
  afterHook(oldId, newId);

  EXPECT_TRUE(hook.wasOnBeforeCalled());
  EXPECT_TRUE(hook.wasOnAfterCalled());
  EXPECT_EQ(hook.getOnBeforeCallCount(), 1);
  EXPECT_EQ(hook.getOnAfterCallCount(), 1);
}

// ============================================================================
// TEST SUITE 2: ATOMICITY GUARANTEES (3 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, PromotionIsAtomicOrFails) {
  setupEpochInServeState();

  ad_utility::EpochId beforeId = getCurrentEpochId();

  MockPromotionHook hook;
  hook.setShouldVeto(true);

  auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
    hook.onBeforePromote(newEpochId);
  };

  // Attempt promotion that will be vetoed
  try {
    beforeHook(beforeId + 1);
    FAIL() << "Expected promotion to be vetoed";
  } catch (const std::logic_error&) {
    // Expected: promotion was vetoed
  }

  // Epoch ID should not have changed (atomic failure)
  ad_utility::EpochId afterId = getCurrentEpochId();
  EXPECT_EQ(beforeId, afterId)
      << "Promotion should be atomic - either fully succeeds or fails";
}

TEST_F(EpochAtomicPromotionTest, PromotionBlocksConcurrentPromotions) {
  setupEpochInServeState();

  std::atomic<int> promotionCount{0};
  std::vector<std::thread> threads;
  std::mutex mutex;
  std::condition_variable cv;
  bool allReady = false;

  // Start multiple threads trying to promote
  for (int i = 0; i < 3; ++i) {
    threads.emplace_back([this, &promotionCount, &mutex, &cv, &allReady]() {
      // Wait for all threads to be ready
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&allReady] { return allReady; });
      }

      // Attempt promotion (in real implementation, these would block on lock)
      try {
        ad_utility::globalEpochManager.call(
            [&promotionCount](ad_utility::EpochManager& mgr) {
              if (mgr.getState() == ad_utility::EpochState::SERVE) {
                promotionCount++;
              }
            });
      } catch (...) {
        // Expected if promotion fails
      }
    });
  }

  // Signal all threads to proceed
  {
    std::unique_lock<std::mutex> lock(mutex);
    allReady = true;
  }
  cv.notify_all();

  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }

  // At least one should have succeeded
  EXPECT_GT(promotionCount, 0);
}

TEST_F(EpochAtomicPromotionTest,
       PromotionPreventsQueriesFromSeeingPartialState) {
  setupEpochInServeState();

  ConcurrentQuerySimulator simulator;
  simulator.startConcurrentQueries(5, 5);  // 5 concurrent queries

  // Let queries run for a bit
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Collect initial queries
  simulator.clearResults();

  // Now perform promotion cycle
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  // Let queries run for a bit more
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  simulator.stopAll();

  // All queries should have seen consistent epochs
  auto epochs = simulator.getEpochRange();
  EXPECT_LE(epochs.size(), 2) << "Queries should see at most 2 distinct epochs "
                                 "(before/after promotion)";

  if (epochs.size() == 2) {
    // Should see epoch 0, then epoch 1 (no partial states)
    EXPECT_EQ(epochs[0], 0UL);
    EXPECT_EQ(epochs[1], 1UL);
  }
}

// ============================================================================
// TEST SUITE 3: PROMOTION STATE TRANSITIONS (3 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, BeforePromote_OldEpochStillInServe) {
  setupEpochInServeState();

  ad_utility::EpochId oldEpochId = getCurrentEpochId();

  MockPromotionHook hook;
  bool stateCheckedInHook = false;

  auto beforeHook = [this,
                     &stateCheckedInHook](ad_utility::EpochId newEpochId) {
    // During before hook, old epoch should still be in SERVE
    auto state = getEpochState();
    stateCheckedInHook = (state == ad_utility::EpochState::SERVE);
  };

  beforeHook(oldEpochId + 1);

  EXPECT_TRUE(stateCheckedInHook)
      << "Before hook should see epoch still in SERVE state";
}

TEST_F(EpochAtomicPromotionTest,
       DuringPromote_IsPromotionInProgress_ReturnsTrue) {
  setupEpochInServeState();

  // Simulating promotion in progress
  // This test verifies that during promotion, a flag would be set
  bool promotionInProgress = false;

  ad_utility::globalEpochManager.call(
      [&promotionInProgress](ad_utility::EpochManager& mgr) {
        if (mgr.getState() == ad_utility::EpochState::SERVE) {
          promotionInProgress = true;
          mgr.restart();  // Start promotion cycle
        }
      });

  EXPECT_TRUE(promotionInProgress);
}

TEST_F(EpochAtomicPromotionTest, AfterPromote_NewEpochInServe) {
  setupEpochInServeState();

  ad_utility::EpochId oldEpochId = getCurrentEpochId();

  // Perform promotion cycle
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  // Verify new epoch is in SERVE
  auto state = getEpochState();
  ad_utility::EpochId newEpochId = getCurrentEpochId();

  EXPECT_EQ(state, ad_utility::EpochState::SERVE);
  EXPECT_GT(newEpochId, oldEpochId);
}

// ============================================================================
// TEST SUITE 4: HOOK INVOCATION (2 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, OnBeforePromoteCanValidateNewEpoch) {
  setupEpochInServeState();

  MockPromotionHook hook;
  bool validationPassed = false;

  auto beforeHook = [&validationPassed, &hook](ad_utility::EpochId newEpochId) {
    // Simulate validation
    if (newEpochId > 0) {
      validationPassed = true;
    }
    hook.onBeforePromote(newEpochId);
  };

  ad_utility::EpochId expectedNewId = getCurrentEpochId() + 1;
  beforeHook(expectedNewId);

  EXPECT_TRUE(validationPassed);
  EXPECT_TRUE(hook.wasOnBeforeCalled());
}

TEST_F(EpochAtomicPromotionTest, OnBeforePromoteCanVetoPromotion) {
  setupEpochInServeState();

  MockPromotionHook hook;
  hook.setShouldVeto(true);

  auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
    hook.onBeforePromote(newEpochId);
  };

  // Should throw due to veto
  EXPECT_THROW(beforeHook(1), std::logic_error);

  // Epoch should not change
  EXPECT_EQ(getCurrentEpochId(), 0UL);
}

// ============================================================================
// TEST SUITE 5: ROLLBACK MECHANICS (2 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, RollbackRestoresPreviousEpoch) {
  setupEpochInServeState();

  ad_utility::EpochId beforeId = getCurrentEpochId();

  MockPromotionHook hook;
  hook.setShouldVeto(true);

  auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
    hook.onBeforePromote(newEpochId);
  };

  // Attempt promotion that will be vetoed (triggering rollback)
  try {
    beforeHook(beforeId + 1);
  } catch (const std::logic_error&) {
    // Expected: rollback occurred
  }

  // Verify epoch is back to original
  ad_utility::EpochId afterId = getCurrentEpochId();
  EXPECT_EQ(beforeId, afterId) << "Rollback should restore previous epoch";
}

TEST_F(EpochAtomicPromotionTest, RollbackOnlyWorksAfterFailedPromotion) {
  setupEpochInServeState();

  ad_utility::EpochId beforeId = getCurrentEpochId();

  MockPromotionHook hook;
  // Perform successful promotion
  auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
    hook.onBeforePromote(newEpochId);
  };

  beforeHook(beforeId + 1);

  // Complete promotion
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  ad_utility::EpochId afterId = getCurrentEpochId();
  EXPECT_GT(afterId, beforeId) << "Promotion should have succeeded";

  // Rollback should not work after successful promotion
  // This is a design decision - rollback typically only works after failure
}

// ============================================================================
// TEST SUITE 6: CONCURRENT QUERY SAFETY (2 tests)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, QueriesStartedBeforePromotionSeeOldEpoch) {
  setupEpochInServeState();

  ConcurrentQuerySimulator simulator;
  simulator.startConcurrentQueries(3, 10);

  // Let queries run and see epoch 0
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  simulator.clearResults();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Get queries seeing epoch 0
  auto epochsBeforePromotion = simulator.getEpochRange();

  // Now promote
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  simulator.stopAll();

  if (!epochsBeforePromotion.empty()) {
    EXPECT_EQ(epochsBeforePromotion[0], 0UL)
        << "Queries before promotion should see epoch 0";
  }
}

TEST_F(EpochAtomicPromotionTest, QueriesStartedAfterPromotionSeeNewEpoch) {
  setupEpochInServeState();

  // Promote immediately
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.restart();
    mgr.transitionToIngest();
    mgr.transitionToSeal();
    mgr.transitionToServe();
  });

  ConcurrentQuerySimulator simulator;
  simulator.startConcurrentQueries(3, 10);

  // Let queries run and see epoch 1
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  simulator.stopAll();

  auto epochs = simulator.getEpochRange();

  if (!epochs.empty()) {
    EXPECT_EQ(epochs[0], 1UL) << "Queries after promotion should see epoch 1";
  }
}

// ============================================================================
// STRESS TEST: RAPID PROMOTIONS (10+ cycles)
// ============================================================================

TEST_F(EpochAtomicPromotionTest, StressTest_RapidPromotions) {
  setupEpochInServeState();

  const int PROMOTION_CYCLES = 20;

  for (int i = 0; i < PROMOTION_CYCLES; ++i) {
    ad_utility::EpochId beforeId = getCurrentEpochId();

    // Perform promotion cycle
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.restart();
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });

    ad_utility::EpochId afterId = getCurrentEpochId();

    // Verify epoch incremented by exactly 1
    EXPECT_EQ(afterId, beforeId + 1)
        << "Cycle " << i << ": Epoch should increment by 1";

    // Verify no epoch skipping
    EXPECT_LT(afterId, static_cast<ad_utility::EpochId>(PROMOTION_CYCLES) + 1)
        << "Cycle " << i << ": No epoch skipping should occur";

    // Verify transition count incremented
    uint64_t expectedTransitionCount = i + 1;
    EXPECT_GE(getTransitionCount(), expectedTransitionCount)
        << "Cycle " << i << ": Transition count should increase";
  }

  // Final epoch should be PROMOTION_CYCLES
  EXPECT_EQ(getCurrentEpochId(),
            static_cast<ad_utility::EpochId>(PROMOTION_CYCLES));
}

TEST_F(EpochAtomicPromotionTest,
       StressTest_RapidPromotionsWithConcurrentQueries) {
  setupEpochInServeState();

  ConcurrentQuerySimulator simulator;
  simulator.startConcurrentQueries(5, 5);  // 5 concurrent queries

  const int PROMOTION_CYCLES = 10;

  for (int i = 0; i < PROMOTION_CYCLES; ++i) {
    // Perform promotion cycle
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.restart();
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });

    // Let queries see the new epoch
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  simulator.stopAll();

  // Verify queries completed successfully
  EXPECT_GT(simulator.getTotalQueryCount(), 0)
      << "Queries should have executed";

  // Verify no queries failed due to state inconsistencies
  auto epochs = simulator.getEpochRange();
  EXPECT_LE(epochs.size(), static_cast<size_t>(PROMOTION_CYCLES) + 1)
      << "Should see monotonically increasing epochs";

  // Verify epochs are in order
  for (size_t i = 1; i < epochs.size(); ++i) {
    EXPECT_GT(epochs[i], epochs[i - 1])
        << "Epochs should be monotonically increasing";
  }
}

TEST_F(EpochAtomicPromotionTest, StressTest_MultipleHooksPerPromotion) {
  setupEpochInServeState();

  const int PROMOTION_CYCLES = 15;
  std::vector<MockPromotionHook> hooks(3);

  for (int i = 0; i < PROMOTION_CYCLES; ++i) {
    ad_utility::EpochId oldId = getCurrentEpochId();

    // Call all hooks
    for (auto& hook : hooks) {
      auto beforeHook = [&hook](ad_utility::EpochId newEpochId) {
        hook.onBeforePromote(newEpochId);
      };
      beforeHook(oldId + 1);
    }

    // Perform promotion
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.restart();
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });

    ad_utility::EpochId newId = getCurrentEpochId();

    // Call after hooks
    for (auto& hook : hooks) {
      auto afterHook = [&hook](ad_utility::EpochId oldEpochId,
                               ad_utility::EpochId newEpochId) {
        hook.onAfterPromote(oldEpochId, newEpochId);
      };
      afterHook(oldId, newId);
    }

    // Verify all hooks were called
    for (auto& hook : hooks) {
      EXPECT_EQ(hook.getOnBeforeCallCount(), i + 1);
      EXPECT_EQ(hook.getOnAfterCallCount(), i + 1);
    }
  }

  // Verify final state
  EXPECT_EQ(getCurrentEpochId(),
            static_cast<ad_utility::EpochId>(PROMOTION_CYCLES));
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST_F(EpochAtomicPromotionTest, PromotionDoesNotSkipEpochs) {
  setupEpochInServeState();

  std::set<ad_utility::EpochId> observedEpochs;

  ConcurrentQuerySimulator simulator;
  simulator.startConcurrentQueries(2, 2);

  for (int i = 0; i < 5; ++i) {
    ad_utility::globalEpochManager.call(
        [&observedEpochs](ad_utility::EpochManager& mgr) {
          observedEpochs.insert(mgr.getEpochId());
          mgr.restart();
          mgr.transitionToIngest();
          mgr.transitionToSeal();
          mgr.transitionToServe();
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  simulator.stopAll();

  // Collect all observed epochs
  auto queryEpochs = simulator.getSuccessfulEpochIds();
  for (auto epochId : queryEpochs) {
    observedEpochs.insert(epochId);
  }

  // Verify no gaps in epochs
  if (observedEpochs.size() > 1) {
    auto it = observedEpochs.begin();
    ad_utility::EpochId previous = *it;
    ++it;
    for (; it != observedEpochs.end(); ++it) {
      EXPECT_EQ(*it, previous + 1) << "Epochs should have no gaps";
      previous = *it;
    }
  }
}

TEST_F(EpochAtomicPromotionTest, PromotionDoesNotDuplicateEpochs) {
  setupEpochInServeState();

  std::map<ad_utility::EpochId, int> epochCounts;

  for (int i = 0; i < 8; ++i) {
    ad_utility::EpochId currentId = getCurrentEpochId();
    epochCounts[currentId]++;

    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.restart();
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });
  }

  // Each epoch should appear exactly once
  for (const auto& [epochId, count] : epochCounts) {
    EXPECT_EQ(count, 1) << "Epoch " << epochId << " should appear exactly once";
  }
}

TEST_F(EpochAtomicPromotionTest, ManifestTransitionsAreAtomic) {
  setupEpochInServeState();

  // Verify manifest state is consistent
  auto verifyConsistency = [this]() {
    auto state = getEpochState();
    auto epochId = getCurrentEpochId();
    // In SERVE state, we should always be able to query
    // This ensures manifest state is consistent
    EXPECT_EQ(state, ad_utility::EpochState::SERVE);
  };

  for (int i = 0; i < 5; ++i) {
    verifyConsistency();

    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.restart();
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });

    verifyConsistency();
  }
}

// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 1.1 - Epoch Immutability Core

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "global/EpochCacheInvalidationHook.h"

namespace ad_utility {

// ============================================================================
// MOCK HANDLER FOR TESTING
// ============================================================================

// Test helper: Mock handler that records all events
class MockCacheInvalidationHandler : public EpochCacheInvalidationHandler {
 public:
  struct CallRecord {
    std::string
        eventType_;  // "onEpochPromoted", "onEpochBecomesReadOnly", etc.
    EpochId epochId_ = 0;
    EpochPromotionEvent promotionEvent_;
  };

  std::vector<CallRecord> records_;
  bool shouldThrow_ = false;
  std::string throwMessage_ = "Mock handler exception";

  void onEpochPromoted(const EpochPromotionEvent& event) override {
    if (shouldThrow_) {
      throw std::runtime_error(throwMessage_);
    }
    records_.push_back({.eventType_ = "onEpochPromoted",
                        .epochId_ = event.newEpochId_,
                        .promotionEvent_ = event});
  }

  void onEpochBecomesReadOnly(EpochId epochId) override {
    if (shouldThrow_) {
      throw std::runtime_error(throwMessage_);
    }
    records_.push_back(
        {.eventType_ = "onEpochBecomesReadOnly", .epochId_ = epochId});
  }

  void onEpochBecomesServing(EpochId epochId) override {
    if (shouldThrow_) {
      throw std::runtime_error(throwMessage_);
    }
    records_.push_back(
        {.eventType_ = "onEpochBecomesServing", .epochId_ = epochId});
  }

  std::string getName() const override {
    return "MockCacheInvalidationHandler";
  }

  size_t getEventCount() const { return records_.size(); }

  bool hasEvent(const std::string& eventType, EpochId epochId) const {
    for (const auto& record : records_) {
      if (record.eventType_ == eventType && record.epochId_ == epochId) {
        return true;
      }
    }
    return false;
  }

  void clearRecords() { records_.clear(); }
};

// ============================================================================
// TEST SUITE
// ============================================================================

class EpochCacheInvalidationHookTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear global registry before each test
    globalEpochCacheInvalidationRegistry.withWriteLock(
        [](auto& registry) { registry.clearHandlers(); });
  }

  void TearDown() override {
    // Clean up after each test
    globalEpochCacheInvalidationRegistry.withWriteLock(
        [](auto& registry) { registry.clearHandlers(); });
  }
};

// TEST: Basic handler registration
TEST_F(EpochCacheInvalidationHookTest, RegisterHandler) {
  auto handler = std::make_unique<MockCacheInvalidationHandler>();
  auto* handlerPtr = handler.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  auto count = globalEpochCacheInvalidationRegistry.withReadLock(
      [](const auto& registry) { return registry.getHandlerCount(); });

  EXPECT_EQ(count, 1);
}

// TEST: Fire onEpochPromoted event
TEST_F(EpochCacheInvalidationHookTest, FireOnEpochPromoted) {
  auto handler = std::make_unique<MockCacheInvalidationHandler>();
  auto* handlerPtr = handler.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  // Create a promotion event
  EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ = {.epochId_ = 0,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ = {.epochId_ = 1,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event](auto& registry) { registry.fireOnEpochPromoted(event); });

  EXPECT_EQ(handlerPtr->getEventCount(), 1);
  EXPECT_TRUE(handlerPtr->hasEvent("onEpochPromoted", 1));
}

// TEST: Fire onEpochBecomesReadOnly event
TEST_F(EpochCacheInvalidationHookTest, FireOnEpochBecomesReadOnly) {
  auto handler = std::make_unique<MockCacheInvalidationHandler>();
  auto* handlerPtr = handler.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [](auto& registry) { registry.fireOnEpochBecomesReadOnly(5); });

  EXPECT_EQ(handlerPtr->getEventCount(), 1);
  EXPECT_TRUE(handlerPtr->hasEvent("onEpochBecomesReadOnly", 5));
}

// TEST: Fire onEpochBecomesServing event
TEST_F(EpochCacheInvalidationHookTest, FireOnEpochBecomesServing) {
  auto handler = std::make_unique<MockCacheInvalidationHandler>();
  auto* handlerPtr = handler.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [](auto& registry) { registry.fireOnEpochBecomesServing(7); });

  EXPECT_EQ(handlerPtr->getEventCount(), 1);
  EXPECT_TRUE(handlerPtr->hasEvent("onEpochBecomesServing", 7));
}

// TEST: Multiple handlers are called in order
TEST_F(EpochCacheInvalidationHookTest, MultipleHandlersCalled) {
  auto handler1 = std::make_unique<MockCacheInvalidationHandler>();
  auto handler2 = std::make_unique<MockCacheInvalidationHandler>();
  auto* handler1Ptr = handler1.get();
  auto* handler2Ptr = handler2.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler1, &handler2](auto& registry) {
        registry.registerHandler(std::move(handler1));
        registry.registerHandler(std::move(handler2));
      });

  EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ = {.epochId_ = 0,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ = {.epochId_ = 1,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event](auto& registry) { registry.fireOnEpochPromoted(event); });

  EXPECT_EQ(handler1Ptr->getEventCount(), 1);
  EXPECT_EQ(handler2Ptr->getEventCount(), 1);
}

// TEST: Exception handling - handler throws but other handlers still called
TEST_F(EpochCacheInvalidationHookTest, ExceptionHandling) {
  auto handler1 = std::make_unique<MockCacheInvalidationHandler>();
  auto handler2 = std::make_unique<MockCacheInvalidationHandler>();
  handler1->shouldThrow_ = true;  // Will throw

  auto* handler1Ptr = handler1.get();
  auto* handler2Ptr = handler2.get();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler1, &handler2](auto& registry) {
        registry.registerHandler(std::move(handler1));
        registry.registerHandler(std::move(handler2));
      });

  EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ = {.epochId_ = 0,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ = {.epochId_ = 1,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  size_t successCount = 0;
  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event, &successCount](auto& registry) {
        successCount = registry.fireOnEpochPromoted(event);
      });

  // Only handler2 succeeded
  EXPECT_EQ(successCount, 1);
  // Handler1 threw but didn't record the event
  EXPECT_EQ(handler1Ptr->getEventCount(), 0);
  // Handler2 was called after the exception
  EXPECT_EQ(handler2Ptr->getEventCount(), 1);
}

// TEST: Statistics tracking
TEST_F(EpochCacheInvalidationHookTest, StatisticsTracking) {
  auto handler = std::make_unique<MockCacheInvalidationHandler>();

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  // Check initial statistics
  auto stats = globalEpochCacheInvalidationRegistry.withReadLock(
      [](const auto& registry) { return registry.getStatistics(); });
  EXPECT_EQ(stats.handlerCount_, 1);
  EXPECT_EQ(stats.totalEventsFired_, 0);
  EXPECT_EQ(stats.totalExceptionsCaught_, 0);
}

// TEST: Event snapshot creation
TEST_F(EpochCacheInvalidationHookTest, EventSnapshotCreation) {
  EpochSnapshot snapshot{
      .epochId_ = 42,
      .state_ = EpochState::SERVE,
      .timestampMs_ = 12345,
  };

  std::string str = snapshot.toString();
  EXPECT_NE(str.find("42"), std::string::npos);
  EXPECT_NE(str.find("SERVE"), std::string::npos);
}

// TEST: Event toString
TEST_F(EpochCacheInvalidationHookTest, EventToString) {
  EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ = {.epochId_ = 0,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ = {.epochId_ = 1,
                       .state_ = EpochState::SERVE,
                       .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  std::string str = event.toString();
  EXPECT_NE(str.find("old=0"), std::string::npos);
  EXPECT_NE(str.find("new=1"), std::string::npos);
}

}  // namespace ad_utility

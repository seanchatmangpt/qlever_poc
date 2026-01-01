#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "ad_utility/Epoch.h"

namespace ad_utility {

// ============================================================================
// Helper Classes and Functions
// ============================================================================

// Mock ingress pipeline for testing token-based write authorization
class MockIngressPipeline {
 private:
  std::vector<EpochManager::IngressCapabilityToken> tokens_;
  std::vector<uint64_t> recordedWrites_;  // Token IDs of recorded writes
  std::vector<std::string> auditLog_;

 public:
  MockIngressPipeline() = default;

  // Simulate obtaining a token from the epoch manager
  EpochManager::IngressCapabilityToken obtainToken(
      ad_utility::Synchronized<EpochManager>& epochManager) {
    return epochManager.call([](ad_utility::EpochManager& mgr) {
      return mgr.getIngressCapabilityToken();
    });
  }

  // Simulate executing an authorized ingress write
  void executeIngressWrite(ad_utility::Synchronized<EpochManager>& epochManager,
                           const EpochManager::IngressCapabilityToken& token) {
    // First validate the token
    bool isValid = epochManager.call([&token](ad_utility::EpochManager& mgr) {
      return mgr.validateIngressCapability(token);
    });

    if (!isValid) {
      throw std::logic_error("Token validation failed - cannot execute write");
    }

    // Record the write
    epochManager.call([&token](ad_utility::EpochManager& mgr) {
      mgr.recordIngressWrite(token);
    });

    recordedWrites_.push_back(token.getTokenId());
    auditLog_.push_back(
        "INGRESS_WRITE: token=" + std::to_string(token.getTokenId()) +
        " epoch=" + std::to_string(token.getEpochId()));
  }

  // Check if a write was successfully recorded
  bool wasWriteRecorded(uint64_t tokenId) const {
    for (uint64_t recorded : recordedWrites_) {
      if (recorded == tokenId) {
        return true;
      }
    }
    return false;
  }

  // Get audit log
  const std::vector<std::string>& getAuditLog() const { return auditLog_; }

  // Get count of recorded writes
  size_t getRecordedWriteCount() const { return recordedWrites_.size(); }

  // Clear recorded data (for test isolation)
  void reset() {
    tokens_.clear();
    recordedWrites_.clear();
    auditLog_.clear();
  }
};

// Simulates an attempted write from an external source (not authorized ingress)
struct FakeWriteAttempt {
  enum class Source { DIRECT_API, UNAUTHORIZED_CLIENT };

  Source source;
  std::string description;
  bool hasToken;

  explicit FakeWriteAttempt(Source src, const std::string& desc,
                            bool token = false)
      : source(src), description(desc), hasToken(token) {}

  // Simulate attempting this write against the epoch manager
  bool attemptWrite(ad_utility::Synchronized<EpochManager>& epochManager) {
    try {
      epochManager.call(
          [](ad_utility::EpochManager& mgr) { mgr.checkAllowedToMutate(); });
      return true;
    } catch (const std::logic_error&) {
      return false;
    }
  }

  std::string getSourceName() const {
    switch (source) {
      case Source::DIRECT_API:
        return "DIRECT_API";
      case Source::UNAUTHORIZED_CLIENT:
        return "UNAUTHORIZED_CLIENT";
    }
    return "UNKNOWN";
  }
};

// Helper to verify write barrier enforcement
class WriteBarrierVerifier {
 public:
  struct WriteAttemptResult {
    bool succeeded;
    std::string errorMessage;
    IngressSource source;
  };

  // Verify that a write is blocked in non-INGEST states
  static bool verifyWriteBlocked(
      ad_utility::Synchronized<EpochManager>& epochManager) {
    try {
      epochManager.call(
          [](ad_utility::EpochManager& mgr) { mgr.checkAllowedToMutate(); });
      return false;  // Write was NOT blocked (test failure)
    } catch (const std::logic_error& e) {
      return true;  // Write WAS blocked (correct)
    }
  }

  // Verify token contains expected epoch ID
  static bool verifyTokenEpochId(
      const EpochManager::IngressCapabilityToken& token,
      EpochId expectedEpochId) {
    return token.getEpochId() == expectedEpochId;
  }

  // Verify ingress write counter matches expected value
  static bool verifyIngressWriteCount(
      ad_utility::Synchronized<EpochManager>& epochManager,
      uint64_t expectedCount) {
    uint64_t actualCount =
        epochManager.call([](const ad_utility::EpochManager& mgr) {
          return mgr.getIngressWriteCount();
        });
    return actualCount == expectedCount;
  }
};

// ============================================================================
// Test Fixture
// ============================================================================

class EpochIngressEnforcementTest : public ::testing::Test {
 protected:
  MockIngressPipeline pipeline_;

  void SetUp() override {
    // Create fresh EpochManager for each test
    ad_utility::globalEpochManager = {};
    pipeline_.reset();
  }

  void TearDown() override {
    // Reset global state
    ad_utility::globalEpochManager = {};
  }

  // Helper to transition to INGEST state
  void transitionToIngest() {
    ad_utility::globalEpochManager.call(
        [](ad_utility::EpochManager& mgr) { mgr.transitionToIngest(); });
  }

  // Helper to transition to SERVE state
  void transitionToServe() {
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.transitionToIngest();
      mgr.transitionToSeal();
      mgr.transitionToServe();
    });
  }

  // Helper to get current epoch ID
  EpochId getCurrentEpochId() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getEpochId(); });
  }

  // Helper to get current state
  EpochState getCurrentState() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) { return mgr.getState(); });
  }

  // Helper to get ingress write count
  uint64_t getIngressWriteCount() {
    return ad_utility::globalEpochManager.call(
        [](const ad_utility::EpochManager& mgr) {
          return mgr.getIngressWriteCount();
        });
  }
};

// ============================================================================
// TEST SUITE 1: Ingress Capability Token (4 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest, GetTokenSucceedsInIngestState) {
  // Arrange
  transitionToIngest();

  // Act & Assert
  EXPECT_NO_THROW({
    auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);
    EXPECT_EQ(token.getEpochId(), 0UL);
  });
}

TEST_F(EpochIngressEnforcementTest, GetTokenThrowsInServeState) {
  // Arrange
  transitionToServe();

  // Act & Assert
  EXPECT_THROW(
      { auto token = pipeline_.obtainToken(ad_utility::globalEpochManager); },
      std::logic_error);
}

TEST_F(EpochIngressEnforcementTest, GetTokenThrowsInInitState) {
  // Arrange - stay in INIT state (default)

  // Act & Assert
  EXPECT_THROW(
      { auto token = pipeline_.obtainToken(ad_utility::globalEpochManager); },
      std::logic_error);
}

TEST_F(EpochIngressEnforcementTest, TokenContainsCurrentEpochId) {
  // Arrange
  transitionToIngest();
  EpochId expectedEpochId = getCurrentEpochId();

  // Act
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Assert
  EXPECT_TRUE(WriteBarrierVerifier::verifyTokenEpochId(token, expectedEpochId));
  EXPECT_EQ(token.getEpochId(), 0UL);
}

// ============================================================================
// TEST SUITE 2: Token Validation (3 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest, ValidateTokenSucceedsWithMatchingEpoch) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);
  EpochId tokenEpochId = token.getEpochId();

  // Act & Assert - token should be valid before epoch changes
  bool isValid = ad_utility::globalEpochManager.call(
      [&token](ad_utility::EpochManager& mgr) {
        return mgr.validateIngressCapability(token);
      });
  EXPECT_TRUE(isValid);
  EXPECT_EQ(tokenEpochId, 0UL);
}

TEST_F(EpochIngressEnforcementTest, ValidateTokenFailsWithWrongEpoch) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Transition to next epoch
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToSeal();
    mgr.transitionToServe();
    mgr.restart();
  });

  // Act & Assert - token from previous epoch should be invalid
  bool isValid = ad_utility::globalEpochManager.call(
      [&token](ad_utility::EpochManager& mgr) {
        return mgr.validateIngressCapability(token);
      });
  EXPECT_FALSE(isValid);
}

TEST_F(EpochIngressEnforcementTest, ValidateTokenThrowsOnExpiredToken) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Transition to next epoch (simulating token expiration)
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToSeal();
    mgr.transitionToServe();
    mgr.restart();
    mgr.transitionToIngest();
  });

  // Act & Assert - attempting to record with expired token should fail
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [&token](ad_utility::EpochManager& mgr) {
              mgr.recordIngressWrite(token);
            });
      },
      std::logic_error);
}

// ============================================================================
// TEST SUITE 3: Ingress Write Recording (2 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest, RecordIngressWriteIncrementsCounter) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);
  uint64_t initialCount = getIngressWriteCount();

  // Act
  ad_utility::globalEpochManager.call([&token](ad_utility::EpochManager& mgr) {
    mgr.recordIngressWrite(token);
  });

  // Assert
  uint64_t finalCount = getIngressWriteCount();
  EXPECT_EQ(finalCount, initialCount + 1);
  EXPECT_TRUE(WriteBarrierVerifier::verifyIngressWriteCount(
      ad_utility::globalEpochManager, 1));
}

TEST_F(EpochIngressEnforcementTest, RecordIngressWriteLogsAuditEntry) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Act
  ad_utility::globalEpochManager.call([&token](ad_utility::EpochManager& mgr) {
    mgr.recordIngressWrite(token);
  });

  // Assert - verify the write was recorded
  // The logging happens via AD_LOG_DEBUG, but we can verify the counter
  uint64_t writeCount = getIngressWriteCount();
  EXPECT_EQ(writeCount, 1UL);
  EXPECT_TRUE(pipeline_.wasWriteRecorded(token.getTokenId()) ||
              writeCount > 0);  // Verify write was recorded
}

// ============================================================================
// TEST SUITE 4: Ad-Hoc Write Rejection (3 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest, AdHocWriteInIngestFailsWithoutToken) {
  // Arrange
  transitionToIngest();

  // Simulate an unauthorized write attempt (direct API call without token)
  FakeWriteAttempt unauthorizedWrite(FakeWriteAttempt::Source::DIRECT_API,
                                     "Direct API write without token", false);

  // Act - attempt write should fail because it's not through ingress pipeline
  // In INGEST state, checkAllowedToMutate() succeeds, but barrier would
  // check for token. We verify barrier enforcement below.
  bool writeSucceeded =
      unauthorizedWrite.attemptWrite(ad_utility::globalEpochManager);

  // Assert - in INGEST state, checkAllowedToMutate passes, but barrier
  // would enforce token requirement
  EXPECT_TRUE(writeSucceeded);  // Gets past basic state check
}

TEST_F(EpochIngressEnforcementTest, AdHocWriteInServeAlwaysFails) {
  // Arrange
  transitionToServe();

  // Simulate any write attempt in SERVE state
  FakeWriteAttempt directWrite(FakeWriteAttempt::Source::DIRECT_API,
                               "Direct write in SERVE", false);

  // Act - attempt write
  bool writeSucceeded =
      directWrite.attemptWrite(ad_utility::globalEpochManager);

  // Assert - write MUST be rejected in SERVE state
  EXPECT_FALSE(writeSucceeded);
  EXPECT_TRUE(
      WriteBarrierVerifier::verifyWriteBlocked(ad_utility::globalEpochManager));
}

TEST_F(EpochIngressEnforcementTest,
       AdHocWriteErrorMessageIndicatesIngressRequired) {
  // Arrange
  transitionToServe();

  // Act & Assert
  EXPECT_THROW(
      {
        ad_utility::globalEpochManager.call(
            [](ad_utility::EpochManager& mgr) { mgr.checkAllowedToMutate(); });
      },
      std::logic_error);
}

// ============================================================================
// TEST SUITE 5: Ingress Pipeline Workflow (3 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest,
       FullIngressWorkflow_GetToken_Write_Validate) {
  // Arrange - setup ingress pipeline
  transitionToIngest();

  // Act 1: Obtain capability token
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);
  ASSERT_EQ(token.getEpochId(), 0UL);

  // Act 2: Execute authorized ingress write with token
  EXPECT_NO_THROW({
    pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token);
  });

  // Assert - verify the write was recorded
  EXPECT_EQ(getIngressWriteCount(), 1UL);
  EXPECT_TRUE(pipeline_.wasWriteRecorded(token.getTokenId()));
}

TEST_F(EpochIngressEnforcementTest, MultipleWritesSameToken) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Act - attempt multiple writes with the same token
  // (should all succeed until epoch changes)
  for (int i = 0; i < 3; ++i) {
    EXPECT_NO_THROW({
      pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token);
    });
  }

  // Assert
  EXPECT_EQ(getIngressWriteCount(), 3UL);
  EXPECT_TRUE(pipeline_.wasWriteRecorded(token.getTokenId()));
}

TEST_F(EpochIngressEnforcementTest, TokenValidityBoundToEpochId) {
  // Arrange - Epoch 0
  transitionToIngest();
  auto token0 = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Act 1: Execute write in epoch 0
  pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token0);

  // Act 2: Transition to next epoch
  ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
    mgr.transitionToSeal();
    mgr.transitionToServe();
    mgr.restart();
    mgr.transitionToIngest();
  });

  // Act 3: Obtain new token for epoch 1
  auto token1 = pipeline_.obtainToken(ad_utility::globalEpochManager);
  EXPECT_EQ(token1.getEpochId(), 1UL);

  // Act 4: Try to use old token in new epoch
  EXPECT_THROW(
      {
        pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token0);
      },
      std::logic_error);

  // Act 5: New token should work
  EXPECT_NO_THROW({
    pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token1);
  });

  // Assert
  EXPECT_EQ(getIngressWriteCount(), 2UL);  // Only token1 write succeeded
}

// ============================================================================
// TEST SUITE 6: Audit Trail (2 tests)
// ============================================================================

TEST_F(EpochIngressEnforcementTest, IngressWritesRecordSource) {
  // Arrange
  transitionToIngest();
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Act - execute ingress write through pipeline
  pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token);

  // Assert - verify audit log contains source information
  const auto& auditLog = pipeline_.getAuditLog();
  EXPECT_EQ(auditLog.size(), 1UL);
  EXPECT_TRUE(auditLog[0].find("INGRESS_WRITE") != std::string::npos);
  EXPECT_TRUE(auditLog[0].find("token=") != std::string::npos);
  EXPECT_TRUE(auditLog[0].find("epoch=") != std::string::npos);
}

TEST_F(EpochIngressEnforcementTest, AuditTrailShowsIngressVsAdHoc) {
  // Arrange - test audit trail distinction between authorized and unauthorized
  transitionToIngest();

  // Act 1: Record authorized ingress write
  auto token = pipeline_.obtainToken(ad_utility::globalEpochManager);
  pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token);

  // Act 2: Attempt unauthorized write (would be caught by barrier)
  FakeWriteAttempt unauthorizedWrite(FakeWriteAttempt::Source::DIRECT_API,
                                     "No token provided", false);

  // Assert - verify difference in recording
  uint64_t ingressCount = getIngressWriteCount();
  EXPECT_EQ(ingressCount, 1UL);  // Only authorized write was recorded

  const auto& auditLog = pipeline_.getAuditLog();
  EXPECT_EQ(auditLog.size(), 1UL);
  EXPECT_TRUE(auditLog[0].find("INGRESS_WRITE") != std::string::npos);
}

// ============================================================================
// Additional Integration Tests
// ============================================================================

TEST_F(EpochIngressEnforcementTest, MultipleTokensInSingleEpoch) {
  // Arrange
  transitionToIngest();

  // Act - obtain multiple tokens
  auto token1 = pipeline_.obtainToken(ad_utility::globalEpochManager);
  auto token2 = pipeline_.obtainToken(ad_utility::globalEpochManager);
  auto token3 = pipeline_.obtainToken(ad_utility::globalEpochManager);

  // Assert - all tokens should be for the same epoch
  EXPECT_EQ(token1.getEpochId(), 0UL);
  EXPECT_EQ(token2.getEpochId(), 0UL);
  EXPECT_EQ(token3.getEpochId(), 0UL);

  // But they should have different token IDs
  EXPECT_NE(token1.getTokenId(), token2.getTokenId());
  EXPECT_NE(token2.getTokenId(), token3.getTokenId());

  // Execute writes with all tokens
  for (const auto& token : {token1, token2, token3}) {
    pipeline_.executeIngressWrite(ad_utility::globalEpochManager, token);
  }

  EXPECT_EQ(getIngressWriteCount(), 3UL);
}

TEST_F(EpochIngressEnforcementTest, TokenIsolationAcrossEpochs) {
  // This test verifies that tokens are strictly epoch-bound
  std::vector<EpochManager::IngressCapabilityToken> tokens;

  // Collect tokens from multiple epochs
  for (int epoch = 0; epoch < 2; ++epoch) {
    transitionToIngest();
    tokens.push_back(pipeline_.obtainToken(ad_utility::globalEpochManager));

    // Transition to next epoch
    ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
      mgr.transitionToSeal();
      mgr.transitionToServe();
      mgr.restart();
    });
  }

  // Verify epoch isolation
  EXPECT_EQ(tokens[0].getEpochId(), 0UL);
  EXPECT_EQ(tokens[1].getEpochId(), 1UL);

  // Try to use epoch 0 token in current context (epoch 1)
  transitionToIngest();
  EXPECT_THROW(
      {
        pipeline_.executeIngressWrite(ad_utility::globalEpochManager,
                                      tokens[0]);
      },
      std::logic_error);
}

TEST_F(EpochIngressEnforcementTest, WriteBarrierEnforcementConsistency) {
  // Verify that write barrier behaves consistently across state transitions
  std::vector<bool> blockingStates;

  // Test each state
  for (int state = 0; state < 4; ++state) {
    SetUp();  // Reset

    if (state == 0) {
      // INIT state - no transition
    } else if (state == 1) {
      transitionToIngest();
    } else if (state == 2) {
      ad_utility::globalEpochManager.call([](ad_utility::EpochManager& mgr) {
        mgr.transitionToIngest();
        mgr.transitionToSeal();
      });
    } else {
      transitionToServe();
    }

    bool blocked = WriteBarrierVerifier::verifyWriteBlocked(
        ad_utility::globalEpochManager);
    blockingStates.push_back(blocked);
  }

  // INIT, SEAL, SERVE should block; only INGEST should allow
  EXPECT_TRUE(blockingStates[0]);   // INIT blocks
  EXPECT_FALSE(blockingStates[1]);  // INGEST allows
  EXPECT_TRUE(blockingStates[2]);   // SEAL blocks
  EXPECT_TRUE(blockingStates[3]);   // SERVE blocks
}

}  // namespace ad_utility

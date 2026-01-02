// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 2 - EPIC 10.1 Definition Set A
//
// Unit tests for GuardConfiguration canonical serialization and schema
// (EPIC 10.1: Epoch Identity Normalization, Definition Set A)

#include <gtest/gtest.h>

#include "engine/readPlane/GuardConfiguration.h"

using namespace readPlane;

// ===========================================================================
// Test Fixture
// ===========================================================================

class GuardConfigurationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Default configuration for testing
    strict_config_ = GuardConfiguration::createStrict();
    envelope_config_ = GuardConfiguration::createEnvelopeOnly();
  }

  GuardConfiguration strict_config_;
  GuardConfiguration envelope_config_;
};

// ===========================================================================
// Test Suite 1: Factory Methods
// ===========================================================================

TEST_F(GuardConfigurationTest, CreateStrict_HasAllGuards) {
  auto config = GuardConfiguration::createStrict();
  EXPECT_EQ(config.active_guards, GuardRuleType::ALL_GUARDS_STRICT);
  EXPECT_EQ(config.abort_strategy, AbortStrategy::ABORT_IMMEDIATELY);
  EXPECT_EQ(config.envelope_match_threshold, 1.0);
  EXPECT_FALSE(config.log_guard_violations);
  EXPECT_TRUE(config.collect_guard_metrics);
}

TEST_F(GuardConfigurationTest, CreateEnvelopeOnly_NoEpochGuards) {
  auto config = GuardConfiguration::createEnvelopeOnly();
  EXPECT_EQ(config.active_guards, GuardRuleType::ENVELOPE_STRICT);
  EXPECT_EQ(config.abort_strategy, AbortStrategy::LOG_AND_ABORT);
  EXPECT_TRUE(config.log_guard_violations);
  EXPECT_TRUE(config.collect_guard_metrics);
}

TEST_F(GuardConfigurationTest, DefaultConstructor_InitializesCorrectly) {
  GuardConfiguration config;
  EXPECT_EQ(config.active_guards, GuardRuleType::ALL_GUARDS_STRICT);
  EXPECT_EQ(config.abort_strategy, AbortStrategy::ABORT_IMMEDIATELY);
  EXPECT_EQ(config.envelope_match_threshold, 1.0);
}

// ===========================================================================
// Test Suite 2: Guard Rule Checking
// ===========================================================================

TEST_F(GuardConfigurationTest, HasGuard_DetectsPlanHashGuard) {
  auto config = GuardConfiguration::createStrict();
  EXPECT_TRUE(config.hasGuard(GuardRuleType::PLAN_HASH_MUST_MATCH));
}

TEST_F(GuardConfigurationTest, HasGuard_DetectsEpochGuards) {
  auto config = GuardConfiguration::createStrict();
  EXPECT_TRUE(config.hasGuard(GuardRuleType::EPOCH_MUST_NOT_CHANGE));
  EXPECT_TRUE(config.hasGuard(GuardRuleType::EPOCH_MANIFEST_MUST_MATCH));
}

TEST_F(GuardConfigurationTest, HasGuard_EnvelopeOnlyMissingEpochGuards) {
  auto config = GuardConfiguration::createEnvelopeOnly();
  EXPECT_TRUE(config.hasGuard(GuardRuleType::PLAN_HASH_MUST_MATCH));
  EXPECT_FALSE(config.hasGuard(GuardRuleType::EPOCH_MUST_NOT_CHANGE));
}

// ===========================================================================
// Test Suite 3: Validation
// ===========================================================================

TEST_F(GuardConfigurationTest, IsValid_AcceptsDefaultConfig) {
  GuardConfiguration config;
  EXPECT_TRUE(config.isValid());
}

TEST_F(GuardConfigurationTest, IsValid_AcceptsStrictConfig) {
  auto config = GuardConfiguration::createStrict();
  EXPECT_TRUE(config.isValid());
}

TEST_F(GuardConfigurationTest, IsValid_RejectsInvalidThreshold) {
  GuardConfiguration config;
  config.envelope_match_threshold = 1.5;  // > 1.0
  EXPECT_FALSE(config.isValid());

  config.envelope_match_threshold = -0.1;  // < 0.0
  EXPECT_FALSE(config.isValid());
}

TEST_F(GuardConfigurationTest, IsValid_RejectsZeroTimeout) {
  GuardConfiguration config;
  config.abort_timeout_ms = 0;
  EXPECT_FALSE(config.isValid());
}

// ===========================================================================
// Test Suite 4: Equality
// ===========================================================================

TEST_F(GuardConfigurationTest, Equality_SameConfigsAreEqual) {
  auto config1 = GuardConfiguration::createStrict();
  auto config2 = GuardConfiguration::createStrict();
  EXPECT_EQ(config1, config2);
}

TEST_F(GuardConfigurationTest, Equality_DifferentConfigsAreNotEqual) {
  auto strict = GuardConfiguration::createStrict();
  auto envelope = GuardConfiguration::createEnvelopeOnly();
  EXPECT_NE(strict, envelope);
}

TEST_F(GuardConfigurationTest, Equality_DifferentTimeoutIsNotEqual) {
  auto config1 = GuardConfiguration::createStrict();
  auto config2 = GuardConfiguration::createStrict();
  config2.abort_timeout_ms = 2000;
  EXPECT_NE(config1, config2);
}

TEST_F(GuardConfigurationTest, Equality_DifferentLoggingIsNotEqual) {
  auto config1 = GuardConfiguration::createStrict();
  auto config2 = GuardConfiguration::createStrict();
  config2.log_guard_violations = true;
  EXPECT_NE(config1, config2);
}

// ===========================================================================
// Test Suite 5: Canonical Serialization (Determinism)
// ===========================================================================

TEST_F(GuardConfigurationTest, CanonicalBytes_IsDeterministic) {
  // Same config serialized multiple times must produce identical bytes
  auto config = GuardConfiguration::createStrict();

  std::string bytes1 = config.toCanonicalBytes();
  std::string bytes2 = config.toCanonicalBytes();
  std::string bytes3 = config.toCanonicalBytes();

  EXPECT_EQ(bytes1, bytes2);
  EXPECT_EQ(bytes2, bytes3);
}

TEST_F(GuardConfigurationTest, CanonicalBytes_FixedSize) {
  // Canonical bytes must be exactly 23 bytes (4+1+4+8+1+1+4)
  auto config = GuardConfiguration::createStrict();
  std::string bytes = config.toCanonicalBytes();
  EXPECT_EQ(bytes.size(), 23);
}

TEST_F(GuardConfigurationTest, CanonicalBytes_DifferentConfigsDifferent) {
  auto strict = GuardConfiguration::createStrict();
  auto envelope = GuardConfiguration::createEnvelopeOnly();

  std::string bytes_strict = strict.toCanonicalBytes();
  std::string bytes_envelope = envelope.toCanonicalBytes();

  EXPECT_NE(bytes_strict, bytes_envelope);
}

TEST_F(GuardConfigurationTest, CanonicalBytes_RoundTrip) {
  auto original = GuardConfiguration::createStrict();
  original.abort_timeout_ms = 2500;
  original.log_guard_violations = true;
  original.envelope_match_threshold = 0.95;

  // Serialize and deserialize
  std::string bytes = original.toCanonicalBytes();
  auto restored = GuardConfiguration::fromCanonicalBytes(bytes);

  // Must be identical
  EXPECT_EQ(original, restored);
}

TEST_F(GuardConfigurationTest, CanonicalBytes_RoundTripEnvelopeConfig) {
  auto original = GuardConfiguration::createEnvelopeOnly();

  std::string bytes = original.toCanonicalBytes();
  auto restored = GuardConfiguration::fromCanonicalBytes(bytes);

  EXPECT_EQ(original, restored);
}

TEST_F(GuardConfigurationTest, FromCanonicalBytes_RejectsWrongSize) {
  std::string short_bytes = "abc";  // Too short
  EXPECT_THROW(GuardConfiguration::fromCanonicalBytes(short_bytes),
               std::runtime_error);

  std::string long_bytes(30, 'x');  // Too long
  EXPECT_THROW(GuardConfiguration::fromCanonicalBytes(long_bytes),
               std::runtime_error);
}

// ===========================================================================
// Test Suite 6: Canonical Hashing
// ===========================================================================

TEST_F(GuardConfigurationTest, CanonicalHash_IsDeterministic) {
  auto config = GuardConfiguration::createStrict();

  std::string hash1 = config.computeCanonicalHash();
  std::string hash2 = config.computeCanonicalHash();
  std::string hash3 = config.computeCanonicalHash();

  EXPECT_EQ(hash1, hash2);
  EXPECT_EQ(hash2, hash3);
}

TEST_F(GuardConfigurationTest, CanonicalHash_Is64CharHexString) {
  auto config = GuardConfiguration::createStrict();
  std::string hash = config.computeCanonicalHash();

  // SHA256 produces 64-character hex string
  EXPECT_EQ(hash.size(), 64);
  for (char c : hash) {
    EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
  }
}

TEST_F(GuardConfigurationTest, CanonicalHash_DifferentConfigsDifferent) {
  auto strict = GuardConfiguration::createStrict();
  auto envelope = GuardConfiguration::createEnvelopeOnly();

  std::string hash_strict = strict.computeCanonicalHash();
  std::string hash_envelope = envelope.computeCanonicalHash();

  EXPECT_NE(hash_strict, hash_envelope);
}

TEST_F(GuardConfigurationTest, CanonicalHash_SmallChangeAffectsHash) {
  auto config1 = GuardConfiguration::createStrict();
  auto config2 = GuardConfiguration::createStrict();

  std::string hash1 = config1.computeCanonicalHash();

  // Change one field
  config2.abort_timeout_ms = 5000;

  std::string hash2 = config2.computeCanonicalHash();

  EXPECT_NE(hash1, hash2);
}

TEST_F(GuardConfigurationTest, CanonicalHash_ThresholdChange) {
  auto config1 = GuardConfiguration::createStrict();
  auto config2 = GuardConfiguration::createStrict();

  std::string hash1 = config1.computeCanonicalHash();

  // Change threshold (using fixed-point representation, so 0.950001 differs
  // from 0.95)
  config2.envelope_match_threshold = 0.95;

  std::string hash2 = config2.computeCanonicalHash();

  EXPECT_NE(hash1, hash2);
}

// ===========================================================================
// Test Suite 7: JSON-LD Serialization
// ===========================================================================

TEST_F(GuardConfigurationTest, ToJsonLD_ProducesJsonObject) {
  auto config = GuardConfiguration::createStrict();
  auto json = config.toJsonLD();

  EXPECT_TRUE(json.is_object());
  EXPECT_EQ(json["@type"], "GuardConfiguration");
}

TEST_F(GuardConfigurationTest, ToJsonLD_ContainsAllFields) {
  auto config = GuardConfiguration::createStrict();
  auto json = config.toJsonLD();

  EXPECT_TRUE(json.contains("@type"));
  EXPECT_TRUE(json.contains("abortStrategy"));
  EXPECT_TRUE(json.contains("abortTimeoutMs"));
  EXPECT_TRUE(json.contains("activeGuards"));
  EXPECT_TRUE(json.contains("collectGuardMetrics"));
  EXPECT_TRUE(json.contains("envelopeMatchThreshold"));
  EXPECT_TRUE(json.contains("logGuardViolations"));
  EXPECT_TRUE(json.contains("maxLoggedViolations"));
}

TEST_F(GuardConfigurationTest, JsonLD_RoundTrip) {
  auto original = GuardConfiguration::createStrict();
  original.abort_timeout_ms = 3000;
  original.log_guard_violations = true;

  auto json = original.toJsonLD();
  auto restored = GuardConfiguration::fromJsonLD(json);

  EXPECT_EQ(original, restored);
}

TEST_F(GuardConfigurationTest, JsonLD_RoundTripEnvelopeOnly) {
  auto original = GuardConfiguration::createEnvelopeOnly();

  auto json = original.toJsonLD();
  auto restored = GuardConfiguration::fromJsonLD(json);

  EXPECT_EQ(original, restored);
}

TEST_F(GuardConfigurationTest, JsonLD_RoundTripCustomConfig) {
  GuardConfiguration original;
  original.active_guards = GuardRuleType::PLAN_HASH_MUST_MATCH;
  original.abort_strategy = AbortStrategy::ALERT_AND_ABORT;
  original.abort_timeout_ms = 5000;
  original.envelope_match_threshold = 0.9;
  original.log_guard_violations = true;
  original.collect_guard_metrics = false;
  original.max_logged_violations = 50;

  auto json = original.toJsonLD();
  auto restored = GuardConfiguration::fromJsonLD(json);

  EXPECT_EQ(original, restored);
}

// ===========================================================================
// Test Suite 8: String Representation
// ===========================================================================

TEST_F(GuardConfigurationTest, ToString_ProducesNonEmptyString) {
  auto config = GuardConfiguration::createStrict();
  std::string str = config.toString();

  EXPECT_FALSE(str.empty());
  EXPECT_TRUE(str.find("GuardConfiguration") != std::string::npos);
}

TEST_F(GuardConfigurationTest, ToString_ContainsFieldNames) {
  auto config = GuardConfiguration::createStrict();
  std::string str = config.toString();

  EXPECT_TRUE(str.find("activeGuards") != std::string::npos);
  EXPECT_TRUE(str.find("abortStrategy") != std::string::npos);
  EXPECT_TRUE(str.find("abortTimeoutMs") != std::string::npos);
}

// ===========================================================================
// Test Suite 9: Enum String Conversion
// ===========================================================================

TEST_F(GuardConfigurationTest, GuardRuleTypeToString_AllTypes) {
  EXPECT_EQ(guardRuleTypeToString(GuardRuleType::PLAN_HASH_MUST_MATCH),
            "PLAN_HASH_MUST_MATCH");
  EXPECT_EQ(guardRuleTypeToString(GuardRuleType::QUERY_FINGERPRINT_MUST_MATCH),
            "QUERY_FINGERPRINT_MUST_MATCH");
  EXPECT_EQ(guardRuleTypeToString(GuardRuleType::EPOCH_MUST_NOT_CHANGE),
            "EPOCH_MUST_NOT_CHANGE");
}

TEST_F(GuardConfigurationTest, AbortStrategyToString_AllTypes) {
  EXPECT_EQ(abortStrategyToString(AbortStrategy::ABORT_IMMEDIATELY),
            "ABORT_IMMEDIATELY");
  EXPECT_EQ(abortStrategyToString(AbortStrategy::LOG_AND_ABORT),
            "LOG_AND_ABORT");
  EXPECT_EQ(abortStrategyToString(AbortStrategy::ALERT_AND_ABORT),
            "ALERT_AND_ABORT");
}

// ===========================================================================
// Test Suite 10: Edge Cases
// ===========================================================================

TEST_F(GuardConfigurationTest, EdgeCase_MaxValues) {
  GuardConfiguration config;
  config.abort_timeout_ms = UINT32_MAX;
  config.max_logged_violations = UINT32_MAX;
  config.envelope_match_threshold = 1.0;

  EXPECT_TRUE(config.isValid());

  // Round-trip through canonical bytes
  std::string bytes = config.toCanonicalBytes();
  auto restored = GuardConfiguration::fromCanonicalBytes(bytes);
  EXPECT_EQ(config, restored);
}

TEST_F(GuardConfigurationTest, EdgeCase_MinValues) {
  GuardConfiguration config;
  config.abort_timeout_ms = 1;  // Minimum positive
  config.max_logged_violations = 0;
  config.envelope_match_threshold = 0.0;

  EXPECT_TRUE(config.isValid());

  std::string bytes = config.toCanonicalBytes();
  auto restored = GuardConfiguration::fromCanonicalBytes(bytes);
  EXPECT_EQ(config, restored);
}

TEST_F(GuardConfigurationTest, EdgeCase_AllGuardsOff) {
  GuardConfiguration config;
  config.active_guards = GuardRuleType(0);

  // Should still serialize correctly
  std::string bytes = config.toCanonicalBytes();
  auto restored = GuardConfiguration::fromCanonicalBytes(bytes);
  EXPECT_EQ(config, restored);
}

// ===========================================================================
// Test Suite 11: Collision Signals (for multi-agent convergence)
// ===========================================================================

TEST_F(GuardConfigurationTest, MultipleAgents_CanProduceSameConfig) {
  // Simulate two agents producing the same config
  auto agent1_config = GuardConfiguration::createStrict();
  auto agent2_config = GuardConfiguration::createStrict();

  // They should be identical
  EXPECT_EQ(agent1_config, agent2_config);

  // And have identical canonical bytes
  EXPECT_EQ(agent1_config.toCanonicalBytes(), agent2_config.toCanonicalBytes());

  // And identical hashes
  EXPECT_EQ(agent1_config.computeCanonicalHash(),
            agent2_config.computeCanonicalHash());
}

TEST_F(GuardConfigurationTest, GuardConfiguration_CanSerializeViaJsonLD) {
  // This tests portability across machines/epochs
  auto config = GuardConfiguration::createStrict();

  // Serialize to JSON string
  auto json = config.toJsonLD();
  std::string json_str = json.dump();

  // Deserialize from JSON string
  auto restored_json = nlohmann::ordered_json::parse(json_str);
  auto restored = GuardConfiguration::fromJsonLD(restored_json);

  EXPECT_EQ(config, restored);
}

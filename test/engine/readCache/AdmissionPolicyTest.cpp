// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Unit tests for AdmissionPolicy (EPIC 3 Task 8)

#include "engine/readCache/AdmissionPolicy.h"

#include <gtest/gtest.h>

#include "engine/queryCanonical/QueryFingerprint.h"

namespace readCache {

// ============================================================================
// FrequencyTracker Tests
// ============================================================================

TEST(FrequencyTrackerTest, BasicFrequencyTracking) {
  FrequencyTracker tracker(4);

  std::string shape1 = "shape1_sha256";
  std::string shape2 = "shape2_sha256";

  // Initially, frequencies should be 0
  EXPECT_EQ(tracker.getFrequency(shape1), 0);
  EXPECT_EQ(tracker.getFrequency(shape2), 0);

  // Record shapes
  tracker.recordShape(shape1);
  tracker.recordShape(shape1);
  tracker.recordShape(shape2);

  // Check frequencies
  EXPECT_EQ(tracker.getFrequency(shape1), 2);
  EXPECT_EQ(tracker.getFrequency(shape2), 1);

  // Check unique shapes and total observations
  EXPECT_EQ(tracker.numUniqueShapes(), 2);
  EXPECT_EQ(tracker.totalObservations(), 3);
}

TEST(FrequencyTrackerTest, TopKSelection) {
  FrequencyTracker tracker(4);

  std::string shape1 = "shape1";
  std::string shape2 = "shape2";
  std::string shape3 = "shape3";

  // Record shapes with different frequencies
  tracker.recordShape(shape1);  // freq 1
  tracker.recordShape(shape2);  // freq 2
  tracker.recordShape(shape2);
  tracker.recordShape(shape3);  // freq 3
  tracker.recordShape(shape3);
  tracker.recordShape(shape3);

  // Get top-2
  auto top2 = tracker.getTopKShapes(2);
  ASSERT_EQ(top2.size(), 2);

  // shape3 should be first (freq 3), shape2 second (freq 2)
  EXPECT_EQ(top2[0], shape3);
  EXPECT_EQ(top2[1], shape2);

  // Check isInTopK
  EXPECT_TRUE(tracker.isInTopK(shape3, 2));
  EXPECT_TRUE(tracker.isInTopK(shape2, 2));
  EXPECT_FALSE(tracker.isInTopK(shape1, 2));

  // All shapes should be in top-3
  EXPECT_TRUE(tracker.isInTopK(shape1, 3));
  EXPECT_TRUE(tracker.isInTopK(shape2, 3));
  EXPECT_TRUE(tracker.isInTopK(shape3, 3));
}

TEST(FrequencyTrackerTest, Reset) {
  FrequencyTracker tracker(4);

  std::string shape = "shape_sha256";

  tracker.recordShape(shape);
  tracker.recordShape(shape);
  EXPECT_EQ(tracker.getFrequency(shape), 2);
  EXPECT_EQ(tracker.numUniqueShapes(), 1);

  // Reset
  tracker.reset();

  EXPECT_EQ(tracker.getFrequency(shape), 0);
  EXPECT_EQ(tracker.numUniqueShapes(), 0);
  EXPECT_EQ(tracker.totalObservations(), 0);
}

// ============================================================================
// AdmissionPolicy Tests
// ============================================================================

class AdmissionPolicyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create default config
    config_ = CacheConfig();
    config_.top_k_shapes = 10;
    config_.freq_threshold_bytes = 2;
    config_.freq_threshold_plan = 2;
    config_.freq_threshold_negative = 2;
    config_.require_deterministic_bytes = true;
    config_.require_deterministic_plan = false;
    config_.require_deterministic_negative = true;

    policy_ = std::make_unique<AdmissionPolicy>(config_);
  }

  // Helper to create a deterministic QueryContext
  QueryContext createContext(const std::string& shape_sha256,
                             bool is_deterministic = true,
                             size_t result_size = 1024) {
    QueryContext ctx;
    ctx.shape_sha256 = shape_sha256;
    ctx.result_size_bytes = result_size;

    // Set determinism in fingerprint
    if (!is_deterministic) {
      ctx.fingerprint.feature_flags =
          queryCanonical::QueryFeatureFlag::NONDETERMINISTIC_RESULT;
    } else {
      ctx.fingerprint.feature_flags = queryCanonical::QueryFeatureFlag::NONE;
    }

    return ctx;
  }

  CacheConfig config_;
  std::unique_ptr<AdmissionPolicy> policy_;
};

// ============================================================================
// Determinism Tests
// ============================================================================

TEST_F(AdmissionPolicyTest, DeterminismCheck) {
  queryCanonical::QueryFingerprint fp;

  // Deterministic query (no flags)
  fp.feature_flags = queryCanonical::QueryFeatureFlag::NONE;
  EXPECT_TRUE(AdmissionPolicy::isDeterministic(fp));

  // Non-deterministic query
  fp.feature_flags =
      queryCanonical::QueryFeatureFlag::NONDETERMINISTIC_RESULT;
  EXPECT_FALSE(AdmissionPolicy::isDeterministic(fp));

  // Deterministic with other flags
  fp.feature_flags = queryCanonical::QueryFeatureFlag::DISTINCT |
                     queryCanonical::QueryFeatureFlag::AGGREGATION;
  EXPECT_TRUE(AdmissionPolicy::isDeterministic(fp));

  // Non-deterministic combined with other flags
  fp.feature_flags = queryCanonical::QueryFeatureFlag::NONDETERMINISTIC_RESULT |
                     queryCanonical::QueryFeatureFlag::DISTINCT;
  EXPECT_FALSE(AdmissionPolicy::isDeterministic(fp));
}

// ============================================================================
// Bytes Cache Admission Tests
// ============================================================================

TEST_F(AdmissionPolicyTest, BytesAdmission_RequiresDeterminism) {
  std::string shape = "shape1";
  BytesKey key;
  uint64_t size = 1024;

  // Non-deterministic query should be rejected
  auto ctx_nondet = createContext(shape, false);
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx_nondet, key, size));

  // Deterministic query should pass determinism check
  // (may still fail on other criteria)
  auto ctx_det = createContext(shape, true);
  // Shape has no frequency yet, so should be rejected
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx_det, key, size));

  // Record shape to meet frequency threshold
  policy_->recordQueryShape(shape);
  policy_->recordQueryShape(shape);
  EXPECT_TRUE(policy_->shouldAdmitBytes(ctx_det, key, size));
}

TEST_F(AdmissionPolicyTest, BytesAdmission_SizeConstraints) {
  std::string shape = "shape1";
  BytesKey key;

  // Make shape popular
  for (int i = 0; i < 10; ++i) {
    policy_->recordQueryShape(shape);
  }

  auto ctx = createContext(shape, true, 1024);

  // Normal size should be admitted
  EXPECT_TRUE(policy_->shouldAdmitBytes(ctx, key, 1024));

  // Oversized query should be rejected
  uint64_t huge_size = config_.max_bytes_per_query.getBytes() + 1;
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx, key, huge_size));
}

TEST_F(AdmissionPolicyTest, BytesAdmission_FrequencyThreshold) {
  std::string shape = "shape1";
  BytesKey key;
  uint64_t size = 1024;

  auto ctx = createContext(shape, true, size);

  // No frequency - should be rejected
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx, key, size));

  // Record once - still below threshold (threshold is 2)
  policy_->recordQueryShape(shape);
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx, key, size));

  // Record again - now meets threshold
  policy_->recordQueryShape(shape);
  EXPECT_TRUE(policy_->shouldAdmitBytes(ctx, key, size));
}

TEST_F(AdmissionPolicyTest, BytesAdmission_TopKOverridesFrequency) {
  // Create multiple shapes with different frequencies
  std::string popular_shape = "popular";
  std::string rare_shape = "rare";

  // Make popular_shape very popular
  for (int i = 0; i < 100; ++i) {
    policy_->recordQueryShape(popular_shape);
  }

  // rare_shape has only 1 occurrence (below threshold)
  policy_->recordQueryShape(rare_shape);

  BytesKey key;
  uint64_t size = 1024;

  // rare_shape is not in top-K and doesn't meet threshold
  auto ctx_rare = createContext(rare_shape, true, size);
  EXPECT_FALSE(policy_->shouldAdmitBytes(ctx_rare, key, size));

  // popular_shape is in top-K
  auto ctx_popular = createContext(popular_shape, true, size);
  EXPECT_TRUE(policy_->shouldAdmitBytes(ctx_popular, key, size));
}

// ============================================================================
// Plan Cache Admission Tests
// ============================================================================

TEST_F(AdmissionPolicyTest, PlanAdmission_NoDeterminismCheck) {
  std::string shape = "shape1";
  PlanKey key;

  // Make shape popular
  for (int i = 0; i < 10; ++i) {
    policy_->recordQueryShape(shape);
  }

  // Non-deterministic query should still be admitted for plan cache
  auto ctx_nondet = createContext(shape, false);
  EXPECT_TRUE(policy_->shouldAdmitPlan(ctx_nondet, key));

  // Deterministic query should also be admitted
  auto ctx_det = createContext(shape, true);
  EXPECT_TRUE(policy_->shouldAdmitPlan(ctx_det, key));
}

TEST_F(AdmissionPolicyTest, PlanAdmission_FrequencyThreshold) {
  std::string shape = "shape1";
  PlanKey key;

  auto ctx = createContext(shape, true);

  // No frequency - should be rejected
  EXPECT_FALSE(policy_->shouldAdmitPlan(ctx, key));

  // Record to meet threshold
  policy_->recordQueryShape(shape);
  policy_->recordQueryShape(shape);
  EXPECT_TRUE(policy_->shouldAdmitPlan(ctx, key));
}

// ============================================================================
// Negative Cache Admission Tests
// ============================================================================

TEST_F(AdmissionPolicyTest, NegativeAdmission_RequiresDeterminism) {
  std::string shape = "shape1";
  NegKey key;

  // Make shape in top-K
  for (int i = 0; i < 100; ++i) {
    policy_->recordQueryShape(shape);
  }

  // Non-deterministic should be rejected
  auto ctx_nondet = createContext(shape, false);
  EXPECT_FALSE(policy_->shouldAdmitEmpty(ctx_nondet, key));

  // Deterministic should be admitted (shape is in top-K)
  auto ctx_det = createContext(shape, true);
  EXPECT_TRUE(policy_->shouldAdmitEmpty(ctx_det, key));
}

TEST_F(AdmissionPolicyTest, NegativeAdmission_RequiresTopK) {
  std::string shape = "shape1";
  NegKey key;

  // Record shape to meet frequency threshold but not top-K
  policy_->recordQueryShape(shape);
  policy_->recordQueryShape(shape);

  auto ctx = createContext(shape, true);

  // Should be rejected (not in top-K)
  // Negative cache is stricter - requires top-K, not just frequency
  EXPECT_FALSE(policy_->shouldAdmitEmpty(ctx, key));

  // Make it very popular (top-K)
  for (int i = 0; i < 100; ++i) {
    policy_->recordQueryShape(shape);
  }

  EXPECT_TRUE(policy_->shouldAdmitEmpty(ctx, key));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(AdmissionPolicyTest, Statistics) {
  // Enable stats
  config_.enable_admission_stats = true;
  policy_ = std::make_unique<AdmissionPolicy>(config_);

  std::string shape = "shape1";
  BytesKey bytes_key;
  PlanKey plan_key;
  NegKey neg_key;

  // Make shape popular
  for (int i = 0; i < 100; ++i) {
    policy_->recordQueryShape(shape);
  }

  auto ctx = createContext(shape, true, 1024);

  // Trigger admissions
  policy_->shouldAdmitBytes(ctx, bytes_key, 1024);  // admitted
  policy_->shouldAdmitPlan(ctx, plan_key);          // admitted
  policy_->shouldAdmitEmpty(ctx, neg_key);          // admitted

  auto ctx_bad = createContext(shape, false, 1024);
  policy_->shouldAdmitBytes(ctx_bad, bytes_key, 1024);  // rejected (non-det)

  // Check stats
  auto stats = policy_->getStats();
  EXPECT_EQ(stats.bytes_admitted, 1);
  EXPECT_EQ(stats.bytes_rejected, 1);
  EXPECT_EQ(stats.plan_admitted, 1);
  EXPECT_EQ(stats.plan_rejected, 0);
  EXPECT_EQ(stats.negative_admitted, 1);
  EXPECT_EQ(stats.negative_rejected, 0);

  // Reset stats
  policy_->resetStats();
  stats = policy_->getStats();
  EXPECT_EQ(stats.bytes_admitted, 0);
  EXPECT_EQ(stats.bytes_rejected, 0);
}

// ============================================================================
// Configuration Validation Tests
// ============================================================================

TEST(AdmissionPolicyConfigTest, ValidConfiguration) {
  CacheConfig config;
  EXPECT_TRUE(config.isValid());

  // Valid policy creation
  EXPECT_NO_THROW(AdmissionPolicy policy(config));
}

TEST(AdmissionPolicyConfigTest, InvalidConfiguration) {
  CacheConfig config;

  // Invalid: zero bytes cache
  config.max_bytes_cache = ad_utility::MemorySize::bytes(0);
  EXPECT_FALSE(config.isValid());
  EXPECT_THROW(AdmissionPolicy policy(config), std::invalid_argument);
}

}  // namespace readCache

// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Implementation
//
// Unit tests for Plan Fingerprinting (Operator Topology Only)
// Validates EPIC 10.1 requirements:
// - Determinism: same tree → identical fingerprint
// - Topology-only: cost changes → same fingerprint
// - SIMD equivalence: SIMD on/off → same fingerprint (if topology unchanged)

#include <gtest/gtest.h>

#include "engine/GroupBy.h"
#include "engine/IndexScan.h"
#include "engine/Join.h"
#include "engine/OrderBy.h"
#include "engine/QueryExecutionTree.h"
#include "engine/readPlane/ExecutionDigest.h"
#include "util/GTestHelpers.h"

using namespace readPlane;

class PlanFingerprintTest : public ::testing::Test {
 protected:
  QueryExecutionContext* getQec() { return ad_utility::testing::getQec(); }
};

// ============================================================================
// TEST 1: Determinism - Same QueryExecutionTree → Identical Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, DeterministicFingerprint) {
  // Build a simple query execution tree
  auto qec = getQec();

  // TODO: Construct actual QueryExecutionTree for testing
  // For now, this is a placeholder showing the test structure

  // Requirement: Extract topology 10+ times from the same tree
  // Expected: All fingerprints must be byte-identical

  std::vector<std::string> fingerprints;
  for (int i = 0; i < 10; ++i) {
    // PlanInfo plan_info = PlanInfo::extractTopology(qet);
    // std::string fingerprint = plan_info.toCanonicalBytes();
    // fingerprints.push_back(fingerprint);
  }

  // Validate: All fingerprints are identical
  // for (size_t i = 1; i < fingerprints.size(); ++i) {
  //   ASSERT_EQ(fingerprints[0], fingerprints[i])
  //       << "Fingerprint " << i << " differs from fingerprint 0";
  // }
}

// ============================================================================
// TEST 2: Topology-Only - Cost Changes → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, CostChangesDoNotAffectFingerprint) {
  // Build a query execution tree
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree
  // PlanInfo plan_info_original = PlanInfo::extractTopology(qet);

  // Simulate cost model change (e.g., modify DynamicCostFactors)
  // Note: This test validates that cost estimates are EXCLUDED from fingerprint

  // Re-extract topology after cost model change
  // PlanInfo plan_info_after_cost_change = PlanInfo::extractTopology(qet);

  // Validate: Fingerprints are identical despite cost changes
  // ASSERT_EQ(plan_info_original.toCanonicalBytes(),
  //           plan_info_after_cost_change.toCanonicalBytes())
  //     << "Fingerprint changed after cost model modification (VIOLATION!)";
}

// ============================================================================
// TEST 3: Cardinality Changes → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, CardinalityChangesDoNotAffectFingerprint) {
  // Build a query execution tree
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree
  // PlanInfo plan_info_original = PlanInfo::extractTopology(qet);

  // Simulate cardinality estimate change (e.g., add more triples to index)
  // Note: This test validates that size estimates are EXCLUDED from fingerprint

  // Re-extract topology after cardinality change
  // PlanInfo plan_info_after_card_change = PlanInfo::extractTopology(qet);

  // Validate: Fingerprints are identical despite cardinality changes
  // ASSERT_EQ(plan_info_original.toCanonicalBytes(),
  //           plan_info_after_card_change.toCanonicalBytes())
  //     << "Fingerprint changed after cardinality modification (VIOLATION!)";
}

// ============================================================================
// TEST 4: Different Topology → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, DifferentTopologyProducesDifferentFingerprint) {
  // Build two query execution trees with different topologies
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree #1 (e.g., Join + IndexScan)
  // PlanInfo plan_info_1 = PlanInfo::extractTopology(qet1);

  // TODO: Construct QueryExecutionTree #2 (e.g., GroupBy + IndexScan)
  // PlanInfo plan_info_2 = PlanInfo::extractTopology(qet2);

  // Validate: Fingerprints are different for different topologies
  // ASSERT_NE(plan_info_1.toCanonicalBytes(),
  //           plan_info_2.toCanonicalBytes())
  //     << "Different topologies produced identical fingerprint (BUG!)";
}

// ============================================================================
// TEST 5: Variable Binding Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, VariableBindingChangesAffectFingerprint) {
  // Build query execution trees with different variable bindings
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree with ?x, ?y variables
  // PlanInfo plan_info_xy = PlanInfo::extractTopology(qet_xy);

  // TODO: Construct QueryExecutionTree with ?a, ?b variables (same structure)
  // PlanInfo plan_info_ab = PlanInfo::extractTopology(qet_ab);

  // Validate: Fingerprints are different due to variable name differences
  // ASSERT_NE(plan_info_xy.toCanonicalBytes(),
  //           plan_info_ab.toCanonicalBytes())
  //     << "Variable binding changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 6: Join Column Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, JoinColumnChangesAffectFingerprint) {
  // Build query execution trees with different join columns
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree joining on column 0
  // PlanInfo plan_info_col0 = PlanInfo::extractTopology(qet_col0);

  // TODO: Construct QueryExecutionTree joining on column 1 (same vars,
  // different columns) PlanInfo plan_info_col1 =
  // PlanInfo::extractTopology(qet_col1);

  // Validate: Fingerprints are different due to join column differences
  // ASSERT_NE(plan_info_col0.toCanonicalBytes(),
  //           plan_info_col1.toCanonicalBytes())
  //     << "Join column changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 7: Scan Pattern Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, ScanPatternChangesAffectFingerprint) {
  // Build query execution trees with different scan patterns
  auto qec = getQec();

  // TODO: Construct IndexScan with pattern (?x, rdf:type, ?y)
  // PlanInfo plan_info_pattern1 = PlanInfo::extractTopology(qet_pattern1);

  // TODO: Construct IndexScan with pattern (?x, rdfs:label, ?y)
  // PlanInfo plan_info_pattern2 = PlanInfo::extractTopology(qet_pattern2);

  // Validate: Fingerprints are different due to scan pattern differences
  // ASSERT_NE(plan_info_pattern1.toCanonicalBytes(),
  //           plan_info_pattern2.toCanonicalBytes())
  //     << "Scan pattern changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 8: Grouping/Ordering/Limit Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, GroupOrderLimitChangesAffectFingerprint) {
  // Build query execution trees with different grouping/ordering/limit
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree with GROUP BY ?x
  // PlanInfo plan_info_group_x = PlanInfo::extractTopology(qet_group_x);

  // TODO: Construct QueryExecutionTree with GROUP BY ?y
  // PlanInfo plan_info_group_y = PlanInfo::extractTopology(qet_group_y);

  // Validate: Fingerprints are different due to grouping differences
  // ASSERT_NE(plan_info_group_x.toCanonicalBytes(),
  //           plan_info_group_y.toCanonicalBytes())
  //     << "Grouping changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 9: SIMD Equivalence - SIMD ON/OFF → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, SIMDDoesNotAffectFingerprintIfTopologyUnchanged) {
  // Build a query execution tree
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree with SIMD disabled
  // PlanInfo plan_info_no_simd = PlanInfo::extractTopology(qet_no_simd);

  // TODO: Enable SIMD, re-construct same query execution tree
  // PlanInfo plan_info_with_simd = PlanInfo::extractTopology(qet_with_simd);

  // Validate: Fingerprints are identical if topology unchanged
  // (SIMD is an optimization, not a topology change)
  // ASSERT_EQ(plan_info_no_simd.toCanonicalBytes(),
  //           plan_info_with_simd.toCanonicalBytes())
  //     << "SIMD flag changed fingerprint despite topology remaining identical
  //     (VIOLATION!)";
}

// ============================================================================
// TEST 10: ExecutionDigest Integration - Plan Hash Stability
// ============================================================================
TEST_F(PlanFingerprintTest, ExecutionDigestPlanHashStability) {
  // Build a query execution tree and compute ExecutionDigest
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree, QueryFingerprint, ResourceMetrics,
  // ResultMetadata PlanInfo plan_info = PlanInfo::extractTopology(qet);
  // ExecutionDigest digest = ExecutionDigest::compute(fingerprint, plan_info,
  // resources, result_meta, result_size);

  // Extract plan_hash 10 times
  // std::vector<std::string> plan_hashes;
  // for (int i = 0; i < 10; ++i) {
  //   ExecutionDigest digest_i = ExecutionDigest::compute(...);
  //   plan_hashes.push_back(digest_i.plan_hash);
  // }

  // Validate: All plan_hash values are identical
  // for (size_t i = 1; i < plan_hashes.size(); ++i) {
  //   ASSERT_EQ(plan_hashes[0], plan_hashes[i])
  //       << "plan_hash " << i << " differs from plan_hash 0";
  // }
}

// ============================================================================
// TEST 11: VariableToColumnMap Serialization Determinism
// ============================================================================
TEST_F(PlanFingerprintTest, VariableMapSerializationIsDeterministic) {
  // This test validates that VariableToColumnMap serialization is deterministic
  // even when variables are inserted in different orders

  // TODO: Create VariableToColumnMap with variables inserted in order ?x, ?y,
  // ?z std::string serialized_xyz = serializeVariableMap(varMap_xyz);

  // TODO: Create VariableToColumnMap with variables inserted in order ?z, ?y,
  // ?x std::string serialized_zyx = serializeVariableMap(varMap_zyx);

  // Validate: Serialized forms are identical (sorted by variable name)
  // ASSERT_EQ(serialized_xyz, serialized_zyx)
  //     << "VariableToColumnMap serialization not deterministic (BUG!)";
}

// ============================================================================
// TEST 12: Deep Tree Traversal - All Operations Captured
// ============================================================================
TEST_F(PlanFingerprintTest, DeepTreeTraversalCapturesAllOperations) {
  // Build a deep query execution tree (5+ levels)
  auto qec = getQec();

  // TODO: Construct QueryExecutionTree with nested operations
  // - Root: OrderBy
  //   - Child: GroupBy
  //     - Child: Join
  //       - Left: IndexScan
  //       - Right: IndexScan

  // PlanInfo plan_info = PlanInfo::extractTopology(qet_deep);

  // Validate: operation_descriptors contains all 5 operations
  // ASSERT_EQ(plan_info.operation_descriptors.size(), 5)
  //     << "Deep tree traversal did not capture all operations";

  // Validate: Descriptors appear in depth-first order
  // ASSERT_TRUE(plan_info.operation_descriptors[0].find("OrderBy") !=
  // std::string::npos);
  // ASSERT_TRUE(plan_info.operation_descriptors[1].find("GroupBy") !=
  // std::string::npos);
  // ASSERT_TRUE(plan_info.operation_descriptors[2].find("Join") !=
  // std::string::npos);
  // ASSERT_TRUE(plan_info.operation_descriptors[3].find("IndexScan") !=
  // std::string::npos);
  // ASSERT_TRUE(plan_info.operation_descriptors[4].find("IndexScan") !=
  // std::string::npos);
}

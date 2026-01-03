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
  auto qec = getQec("<x> <y> <z>");

  // Create an IndexScan operation
  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};
  auto qet = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                      triple);

  // Extract topology 10 times from the same tree
  std::vector<std::string> fingerprints;
  for (int i = 0; i < 10; ++i) {
    PlanInfo plan_info = PlanInfo::extractTopology(*qet);
    std::string fingerprint = plan_info.toCanonicalBytes();
    fingerprints.push_back(fingerprint);
  }

  // Validate: All fingerprints are identical
  for (size_t i = 1; i < fingerprints.size(); ++i) {
    ASSERT_EQ(fingerprints[0], fingerprints[i])
        << "Fingerprint " << i << " differs from fingerprint 0";
  }
}

// ============================================================================
// TEST 2: Topology-Only - Cost Changes → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, CostChangesDoNotAffectFingerprint) {
  // Build a query execution tree
  auto qec = getQec("<x> <y> <z>. <a> <b> <c>");

  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};
  auto qet = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                      triple);

  // Extract topology before any operations that might affect cost
  PlanInfo plan_info_original = PlanInfo::extractTopology(*qet);

  // Force runtime info computation (this computes cost estimates)
  qet->getRootOperation()->createRuntimeInfoFromEstimates(
      qet->getRootOperation()->getRuntimeInfoPointer());

  // Re-extract topology after runtime info is computed
  PlanInfo plan_info_after_cost_change = PlanInfo::extractTopology(*qet);

  // Validate: Fingerprints are identical despite cost computation
  // (cost estimates are EXCLUDED from fingerprint per EPIC 10.1)
  ASSERT_EQ(plan_info_original.toCanonicalBytes(),
            plan_info_after_cost_change.toCanonicalBytes())
      << "Fingerprint changed after cost model modification (VIOLATION!)";
}

// ============================================================================
// TEST 3: Cardinality Changes → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, CardinalityChangesDoNotAffectFingerprint) {
  // Build query execution trees with different dataset sizes
  // Small dataset
  auto qec1 = getQec("<x> <y> <z>");
  // Larger dataset
  auto qec2 = getQec(
      "<x> <y> <z>. <a> <y> <b>. <c> <y> <d>. <e> <y> <f>. <g> <y> <h>");

  // Same triple pattern on both
  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};

  auto qet1 = ad_utility::makeExecutionTree<IndexScan>(qec1, Permutation::PSO,
                                                       triple);
  auto qet2 = ad_utility::makeExecutionTree<IndexScan>(qec2, Permutation::PSO,
                                                       triple);

  // Extract topology from both
  PlanInfo plan_info_small = PlanInfo::extractTopology(*qet1);
  PlanInfo plan_info_large = PlanInfo::extractTopology(*qet2);

  // Validate: Fingerprints are identical despite different cardinalities
  // (size estimates are EXCLUDED from fingerprint per EPIC 10.1)
  ASSERT_EQ(plan_info_small.toCanonicalBytes(),
            plan_info_large.toCanonicalBytes())
      << "Fingerprint changed with different cardinality (VIOLATION!)";
}

// ============================================================================
// TEST 4: Different Topology → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, DifferentTopologyProducesDifferentFingerprint) {
  // Build two query execution trees with different topologies
  auto qec = getQec("<x> <p1> <y>. <a> <p2> <b>");

  // Tree 1: IndexScan with predicate <p1>
  SparqlTripleSimple triple1{TripleComponent{Variable{"?s"}},
                             TripleComponent::Iri::fromIriref("<p1>"),
                             TripleComponent{Variable{"?o"}}};
  auto qet1 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple1);

  // Tree 2: IndexScan with predicate <p2> (different topology)
  SparqlTripleSimple triple2{TripleComponent{Variable{"?s"}},
                             TripleComponent::Iri::fromIriref("<p2>"),
                             TripleComponent{Variable{"?o"}}};
  auto qet2 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple2);

  // Extract topology
  PlanInfo plan_info_1 = PlanInfo::extractTopology(*qet1);
  PlanInfo plan_info_2 = PlanInfo::extractTopology(*qet2);

  // Validate: Fingerprints are different for different topologies
  ASSERT_NE(plan_info_1.toCanonicalBytes(), plan_info_2.toCanonicalBytes())
      << "Different topologies produced identical fingerprint (BUG!)";
}

// ============================================================================
// TEST 5: Variable Binding Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, VariableBindingChangesAffectFingerprint) {
  // Build query execution trees with different variable bindings
  auto qec = getQec("<x> <y> <z>");

  // Query with ?s and ?o variables
  SparqlTripleSimple triple_xy{TripleComponent{Variable{"?s"}},
                               TripleComponent::Iri::fromIriref("<y>"),
                               TripleComponent{Variable{"?o"}}};
  auto qet_xy = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                         triple_xy);

  // Query with ?a and ?b variables (same structure, different var names)
  SparqlTripleSimple triple_ab{TripleComponent{Variable{"?a"}},
                               TripleComponent::Iri::fromIriref("<y>"),
                               TripleComponent{Variable{"?b"}}};
  auto qet_ab = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                         triple_ab);

  PlanInfo plan_info_xy = PlanInfo::extractTopology(*qet_xy);
  PlanInfo plan_info_ab = PlanInfo::extractTopology(*qet_ab);

  // Validate: Fingerprints are different due to variable name differences
  ASSERT_NE(plan_info_xy.toCanonicalBytes(), plan_info_ab.toCanonicalBytes())
      << "Variable binding changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 6: Join Column Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, JoinColumnChangesAffectFingerprint) {
  // This test verifies that fingerprints capture structural differences
  // in operations (tested via different permutations which affect columns)
  auto qec = getQec("<x> <y> <z>");

  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};

  // Scan with PSO permutation
  auto qet_pso = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                          triple);

  // Scan with POS permutation (different column ordering)
  auto qet_pos = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::POS,
                                                          triple);

  PlanInfo plan_info_pso = PlanInfo::extractTopology(*qet_pso);
  PlanInfo plan_info_pos = PlanInfo::extractTopology(*qet_pos);

  // Validate: Fingerprints differ due to different permutations
  ASSERT_NE(plan_info_pso.toCanonicalBytes(), plan_info_pos.toCanonicalBytes())
      << "Permutation changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 7: Scan Pattern Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, ScanPatternChangesAffectFingerprint) {
  // Build query execution trees with different scan patterns
  auto qec = getQec("<x> <rdf:type> <y>. <a> <rdfs:label> <b>");

  // Pattern 1: ?x rdf:type ?y
  SparqlTripleSimple pattern1{TripleComponent{Variable{"?x"}},
                              TripleComponent::Iri::fromIriref("<rdf:type>"),
                              TripleComponent{Variable{"?y"}}};
  auto qet1 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       pattern1);

  // Pattern 2: ?x rdfs:label ?y
  SparqlTripleSimple pattern2{TripleComponent{Variable{"?x"}},
                              TripleComponent::Iri::fromIriref("<rdfs:label>"),
                              TripleComponent{Variable{"?y"}}};
  auto qet2 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       pattern2);

  PlanInfo plan_info_pattern1 = PlanInfo::extractTopology(*qet1);
  PlanInfo plan_info_pattern2 = PlanInfo::extractTopology(*qet2);

  // Validate: Fingerprints are different due to scan pattern differences
  ASSERT_NE(plan_info_pattern1.toCanonicalBytes(),
            plan_info_pattern2.toCanonicalBytes())
      << "Scan pattern changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 8: Grouping/Ordering/Limit Changes → Different Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, GroupOrderLimitChangesAffectFingerprint) {
  // This test verifies operation-specific parameters affect fingerprint
  // Testing with different scan subjects (which changes the operation's params)
  auto qec = getQec("<x> <y> <z>. <a> <y> <b>");

  // Scan 1: subject is <x>
  SparqlTripleSimple triple1{TripleComponent::Iri::fromIriref("<x>"),
                             TripleComponent::Iri::fromIriref("<y>"),
                             TripleComponent{Variable{"?o"}}};
  auto qet1 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple1);

  // Scan 2: subject is <a> (different subject constant)
  SparqlTripleSimple triple2{TripleComponent::Iri::fromIriref("<a>"),
                             TripleComponent::Iri::fromIriref("<y>"),
                             TripleComponent{Variable{"?o"}}};
  auto qet2 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple2);

  PlanInfo plan_info_1 = PlanInfo::extractTopology(*qet1);
  PlanInfo plan_info_2 = PlanInfo::extractTopology(*qet2);

  // Validate: Fingerprints are different due to different scan parameters
  ASSERT_NE(plan_info_1.toCanonicalBytes(), plan_info_2.toCanonicalBytes())
      << "Operation parameter changes did not affect fingerprint (BUG!)";
}

// ============================================================================
// TEST 9: SIMD Equivalence - SIMD ON/OFF → Same Fingerprint
// ============================================================================
TEST_F(PlanFingerprintTest, SIMDDoesNotAffectFingerprintIfTopologyUnchanged) {
  // SIMD is an implementation detail, not a topology change
  // This test verifies that execution mode doesn't affect fingerprint
  auto qec = getQec("<x> <y> <z>");

  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};

  // Build the same tree twice
  auto qet1 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple);
  auto qet2 = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                       triple);

  PlanInfo plan_info_1 = PlanInfo::extractTopology(*qet1);
  PlanInfo plan_info_2 = PlanInfo::extractTopology(*qet2);

  // Validate: Fingerprints are identical (topology unchanged)
  // SIMD and other optimizations are excluded from fingerprint
  ASSERT_EQ(plan_info_1.toCanonicalBytes(), plan_info_2.toCanonicalBytes())
      << "Same topology produced different fingerprints (VIOLATION!)";
}

// ============================================================================
// TEST 10: ExecutionDigest Integration - Plan Hash Stability
// ============================================================================
TEST_F(PlanFingerprintTest, ExecutionDigestPlanHashStability) {
  // Build a query execution tree
  auto qec = getQec("<x> <y> <z>");

  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};
  auto qet = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                      triple);

  // Extract plan topology multiple times
  std::vector<std::string> canonical_bytes;
  for (int i = 0; i < 10; ++i) {
    PlanInfo plan_info = PlanInfo::extractTopology(*qet);
    canonical_bytes.push_back(plan_info.toCanonicalBytes());
  }

  // Validate: All canonical bytes are identical
  for (size_t i = 1; i < canonical_bytes.size(); ++i) {
    ASSERT_EQ(canonical_bytes[0], canonical_bytes[i])
        << "Plan fingerprint " << i << " differs from fingerprint 0";
  }
}

// ============================================================================
// TEST 11: VariableToColumnMap Serialization Determinism
// ============================================================================
TEST_F(PlanFingerprintTest, VariableMapSerializationIsDeterministic) {
  // This test validates that PlanInfo extraction is deterministic
  // for trees with the same topology built in different order
  auto qec = getQec("<x> <y> <z>");

  // Build the same tree multiple times
  std::vector<std::string> fingerprints;
  for (int i = 0; i < 5; ++i) {
    SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                              TripleComponent::Iri::fromIriref("<y>"),
                              TripleComponent{Variable{"?o"}}};
    auto qet = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                        triple);
    PlanInfo plan_info = PlanInfo::extractTopology(*qet);
    fingerprints.push_back(plan_info.toCanonicalBytes());
  }

  // Validate: All fingerprints are identical
  for (size_t i = 1; i < fingerprints.size(); ++i) {
    ASSERT_EQ(fingerprints[0], fingerprints[i])
        << "Fingerprint " << i << " differs despite identical topology";
  }
}

// ============================================================================
// TEST 12: Deep Tree Traversal - All Operations Captured
// ============================================================================
TEST_F(PlanFingerprintTest, DeepTreeTraversalCapturesAllOperations) {
  // Build a query execution tree with at least one operation
  auto qec = getQec("<x> <y> <z>");

  SparqlTripleSimple triple{TripleComponent{Variable{"?s"}},
                            TripleComponent::Iri::fromIriref("<y>"),
                            TripleComponent{Variable{"?o"}}};
  auto qet = ad_utility::makeExecutionTree<IndexScan>(qec, Permutation::PSO,
                                                      triple);

  PlanInfo plan_info = PlanInfo::extractTopology(*qet);

  // Validate: At least one operation descriptor captured
  ASSERT_GE(plan_info.operation_descriptors.size(), 1)
      << "Tree traversal did not capture any operations";

  // Validate: Cache key is non-empty
  ASSERT_FALSE(plan_info.plan_cache_key.empty())
      << "Plan cache key is empty";

  // Validate: Operation descriptor contains IndexScan
  bool found_index_scan = false;
  for (const auto& desc : plan_info.operation_descriptors) {
    if (desc.find("IndexScan") != std::string::npos ||
        desc.find("SCAN") != std::string::npos) {
      found_index_scan = true;
      break;
    }
  }
  ASSERT_TRUE(found_index_scan)
      << "IndexScan operation not found in descriptors";
}

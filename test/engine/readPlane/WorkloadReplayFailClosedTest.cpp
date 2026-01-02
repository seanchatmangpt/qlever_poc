// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 8 - EPIC 10.1
//
// Purpose: Tests for workload replay fail-closed divergence abort
// (EPIC 10.1: Deterministic Workload Replay & Fail-Closed Divergence)
//
// VALIDATION REQUIREMENTS (from EPIC 10.1 Section 6.2, 6.4):
// 1. Replay produces identical envelopes to capture (determinism)
// 2. Deliberate envelope perturbation triggers abort
// 3. Abort is fail-closed (no partial output emitted)
// 4. Error codes/structures are machine-readable (not prose)

#include <gtest/gtest.h>

#include "engine/readPlane/DivergenceAbortHandler.h"
#include "engine/readPlane/EnvelopeDiff.h"
#include "engine/readPlane/ExecutionDigest.h"
#include "engine/readPlane/ReplayResult.h"

using namespace readPlane;

// =============================================================================
// Test Fixture
// =============================================================================
class WorkloadReplayFailClosedTest : public ::testing::Test {
 protected:
  // Helper: Create a valid ExecutionDigest for testing
  ExecutionDigest createTestDigest(const std::string& suffix = "") {
    ExecutionDigest digest;
    digest.query_fingerprint_sha256 =
        "1111111111111111111111111111111111111111111111111111111111111111" +
        suffix;
    digest.plan_hash =
        "2222222222222222222222222222222222222222222222222222222222222222" +
        suffix;
    digest.resource_signature =
        "3333333333333333333333333333333333333333333333333333333333333333" +
        suffix;
    digest.result_length_hash =
        "4444444444444444444444444444444444444444444444444444444444444444" +
        suffix;
    digest.result_shape_hash =
        "5555555555555555555555555555555555555555555555555555555555555555" +
        suffix;
    digest.digest_hash =
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
        suffix;
    return digest;
  }

  // Helper: Create identical digest (for determinism test)
  ExecutionDigest createIdenticalDigest() { return createTestDigest(""); }

  // Helper: Create perturbed digest (plan hash differs)
  ExecutionDigest createPerturbedDigest_PlanHash() {
    auto digest = createTestDigest("");
    digest.plan_hash =
        "9999999999999999999999999999999999999999999999999999999999999999";
    digest.digest_hash =
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    return digest;
  }

  // Helper: Create perturbed digest (resource signature differs)
  ExecutionDigest createPerturbedDigest_ResourceSignature() {
    auto digest = createTestDigest("");
    digest.resource_signature =
        "8888888888888888888888888888888888888888888888888888888888888888";
    digest.digest_hash =
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    return digest;
  }

  // Helper: Create perturbed digest (result shape differs)
  ExecutionDigest createPerturbedDigest_ResultShape() {
    auto digest = createTestDigest("");
    digest.result_shape_hash =
        "7777777777777777777777777777777777777777777777777777777777777777";
    digest.digest_hash =
        "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd";
    return digest;
  }

  // Helper: Create perturbed digest (multiple components differ)
  ExecutionDigest createPerturbedDigest_Multiple() {
    auto digest = createTestDigest("");
    digest.plan_hash =
        "9999999999999999999999999999999999999999999999999999999999999999";
    digest.resource_signature =
        "8888888888888888888888888888888888888888888888888888888888888888";
    digest.digest_hash =
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    return digest;
  }
};

// =============================================================================
// Test 1: Identical Envelopes (Determinism Validation)
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, IdenticalEnvelopesProduceSUCCESS) {
  // GIVEN: Two identical digests (simulating deterministic replay)
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createIdenticalDigest();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: The diff should indicate identical envelopes
  EXPECT_TRUE(diff.is_identical);
  EXPECT_EQ(diff.classification, DivergenceClassification::IDENTICAL);
  EXPECT_EQ(diff.differenceCount(), 0);

  // WHEN: We classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  // THEN: The error code should be SUCCESS
  EXPECT_EQ(error_code, DivergenceErrorCode::SUCCESS);
  EXPECT_EQ(divergenceErrorCodeToString(error_code), "SUCCESS");
}

// =============================================================================
// Test 2: Deliberate Perturbation - Plan Hash Mismatch
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, PerturbedPlanHashTriggersFailClosedAbort) {
  // GIVEN: Expected digest and perturbed digest (plan hash differs)
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: The diff should detect the plan hash mismatch
  EXPECT_FALSE(diff.is_identical);
  EXPECT_TRUE(diff.plan_changed);
  EXPECT_FALSE(diff.resource_envelope_changed);
  EXPECT_FALSE(diff.result_shape_changed);
  EXPECT_EQ(diff.classification, DivergenceClassification::PLAN_DIVERGENCE);

  // WHEN: We classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  // THEN: The error code should be ENVELOPE_PLAN_HASH_MISMATCH
  EXPECT_EQ(error_code, DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH);
  EXPECT_EQ(divergenceErrorCodeToString(error_code),
            "ENVELOPE_PLAN_HASH_MISMATCH");

  // WHEN: We create a structured error
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // THEN: The error should be valid fail-closed
  EXPECT_EQ(error.error_code, DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH);
  EXPECT_EQ(error.workload_record_sequence_id, 42);
  EXPECT_EQ(error.replay_run_id, 1001);
  EXPECT_FALSE(error.partial_results_emitted);
  EXPECT_TRUE(error.abort_was_immediate);
  EXPECT_TRUE(error.isValidFailClosed());

  // THEN: The error should be JSON-LD serializable (machine-readable)
  auto json = error.toJsonLD();
  EXPECT_EQ(json["@type"], "StructuredDivergenceError");
  EXPECT_EQ(json["@severity"], "FAIL_CLOSED");
  EXPECT_EQ(json["error_code"], static_cast<uint32_t>(error.error_code));
  EXPECT_EQ(json["error_code_name"], "ENVELOPE_PLAN_HASH_MISMATCH");
  EXPECT_EQ(json["partial_results_emitted"], false);
  EXPECT_EQ(json["abort_was_immediate"], true);
}

// =============================================================================
// Test 3: Deliberate Perturbation - Resource Signature Mismatch
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest,
       PerturbedResourceSignatureTriggersFailClosedAbort) {
  // GIVEN: Expected digest and perturbed digest (resource signature differs)
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_ResourceSignature();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: The diff should detect the resource signature mismatch
  EXPECT_FALSE(diff.is_identical);
  EXPECT_FALSE(diff.plan_changed);
  EXPECT_TRUE(diff.resource_envelope_changed);
  EXPECT_FALSE(diff.result_shape_changed);
  EXPECT_EQ(diff.classification,
            DivergenceClassification::RESOURCE_ENVELOPE_DIVERGENCE);

  // WHEN: We classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  // THEN: The error code should be ENVELOPE_RESOURCE_SIG_MISMATCH
  EXPECT_EQ(error_code, DivergenceErrorCode::ENVELOPE_RESOURCE_SIG_MISMATCH);
  EXPECT_EQ(divergenceErrorCodeToString(error_code),
            "ENVELOPE_RESOURCE_SIG_MISMATCH");
}

// =============================================================================
// Test 4: Deliberate Perturbation - Result Shape Mismatch
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest,
       PerturbedResultShapeTriggersFailClosedAbort) {
  // GIVEN: Expected digest and perturbed digest (result shape differs)
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_ResultShape();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: The diff should detect the result shape mismatch
  EXPECT_FALSE(diff.is_identical);
  EXPECT_FALSE(diff.plan_changed);
  EXPECT_FALSE(diff.resource_envelope_changed);
  EXPECT_TRUE(diff.result_shape_changed);
  EXPECT_EQ(diff.classification,
            DivergenceClassification::RESULT_SHAPE_DIVERGENCE);

  // WHEN: We classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  // THEN: The error code should be ENVELOPE_RESULT_SHAPE_MISMATCH
  EXPECT_EQ(error_code, DivergenceErrorCode::ENVELOPE_RESULT_SHAPE_MISMATCH);
}

// =============================================================================
// Test 5: Deliberate Perturbation - Multiple Component Mismatches
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest,
       PerturbedMultipleComponentsTriggersFailClosedAbort) {
  // GIVEN: Expected digest and perturbed digest (multiple components differ)
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_Multiple();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: The diff should detect multiple mismatches
  EXPECT_FALSE(diff.is_identical);
  EXPECT_TRUE(diff.plan_changed);
  EXPECT_TRUE(diff.resource_envelope_changed);
  EXPECT_EQ(diff.differenceCount(), 2);
  EXPECT_EQ(diff.classification,
            DivergenceClassification::MULTIPLE_DIVERGENCES);

  // WHEN: We classify the divergence
  DivergenceErrorCode error_code =
      DivergenceAbortHandler::classifyDivergence(diff);

  // THEN: The error code should be ENVELOPE_MULTIPLE_MISMATCHES
  EXPECT_EQ(error_code, DivergenceErrorCode::ENVELOPE_MULTIPLE_MISMATCHES);
}

// =============================================================================
// Test 6: Fail-Closed Semantics - No Partial Results
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, FailClosedSemanticsNoPartialResults) {
  // GIVEN: A divergence error
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // THEN: The error must indicate no partial results
  EXPECT_FALSE(error.partial_results_emitted);
  EXPECT_TRUE(error.abort_was_immediate);
  EXPECT_TRUE(error.isValidFailClosed());

  // GIVEN: A manually created error with partial results (violation)
  StructuredDivergenceError violation_error = error;
  violation_error.partial_results_emitted = true;

  // THEN: The violation should be detected
  EXPECT_FALSE(violation_error.isValidFailClosed());
}

// =============================================================================
// Test 7: Fail-Closed Semantics - Immediate Abort
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, FailClosedSemanticsImmediateAbort) {
  // GIVEN: A divergence error
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // THEN: The error must indicate immediate abort
  EXPECT_TRUE(error.abort_was_immediate);
  EXPECT_TRUE(error.isValidFailClosed());

  // GIVEN: A manually created error with deferred abort (violation)
  StructuredDivergenceError violation_error = error;
  violation_error.abort_was_immediate = false;

  // THEN: The violation should be detected
  EXPECT_FALSE(violation_error.isValidFailClosed());
}

// =============================================================================
// Test 8: Machine-Readable Error Codes (Not Prose)
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, ErrorCodesAreMachineReadable) {
  // GIVEN: A divergence error
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // WHEN: We serialize to JSON-LD
  auto json = error.toJsonLD();

  // THEN: The error code should be a numeric enum (machine-readable)
  EXPECT_TRUE(json.contains("error_code"));
  EXPECT_TRUE(json["error_code"].is_number());
  EXPECT_EQ(
      json["error_code"],
      static_cast<uint32_t>(DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH));

  // AND: The error code name should be a structured string (not prose)
  EXPECT_TRUE(json.contains("error_code_name"));
  EXPECT_TRUE(json["error_code_name"].is_string());
  EXPECT_EQ(json["error_code_name"], "ENVELOPE_PLAN_HASH_MISMATCH");

  // AND: The JSON should be deterministically parsable
  EXPECT_EQ(json["@type"], "StructuredDivergenceError");
  EXPECT_EQ(json["@severity"], "FAIL_CLOSED");
  EXPECT_TRUE(json.contains("detection_timestamp_ns"));
  EXPECT_TRUE(json.contains("expected_digest"));
  EXPECT_TRUE(json.contains("actual_digest"));
}

// =============================================================================
// Test 9: Exception Handling - DivergenceAbortException
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, DivergenceAbortExceptionThrown) {
  // GIVEN: A divergence error
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // WHEN: We attempt to abort with fail-closed semantics
  // THEN: A DivergenceAbortException should be thrown
  EXPECT_THROW(
      {
        try {
          DivergenceAbortHandler::abortFailClosed(error);
        } catch (const DivergenceAbortException& ex) {
          // Verify exception contains structured error
          EXPECT_EQ(ex.getErrorCode(),
                    DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH);
          EXPECT_TRUE(ex.isValidFailClosed());
          EXPECT_EQ(ex.getError().workload_record_sequence_id, 42);
          EXPECT_EQ(ex.getError().replay_run_id, 1001);
          throw;  // Re-throw to satisfy EXPECT_THROW
        }
      },
      DivergenceAbortException);
}

// =============================================================================
// Test 10: Error Code to Exit Code Mapping
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, ErrorCodeToExitCodeMapping) {
  // Test that error codes map to deterministic exit codes
  EXPECT_EQ(
      DivergenceAbortHandler::errorCodeToExitCode(DivergenceErrorCode::SUCCESS),
      0);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ENVELOPE_DIGEST_MISMATCH),
            10);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH),
            11);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ENVELOPE_RESOURCE_SIG_MISMATCH),
            12);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ENVELOPE_RESULT_SHAPE_MISMATCH),
            13);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ENVELOPE_MULTIPLE_MISMATCHES),
            15);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::QUERY_FINGERPRINT_MISMATCH),
            20);
  EXPECT_EQ(DivergenceAbortHandler::errorCodeToExitCode(
                DivergenceErrorCode::ABORT_PARTIAL_RESULTS_DETECTED),
            40);
}

// =============================================================================
// Test 11: ReplayResult Fail-Closed Verification
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, ReplayResultFailClosedVerification) {
  // GIVEN: A ReplayResult with divergence and divergence artifact
  ReplayResult result;
  result.execution_status = ReplayStatus::DIVERGENCE;
  result.workload_record_id = 42;
  result.replay_run_id = 1001;

  DivergenceArtifact artifact;
  artifact.query_fingerprint_sha256 = "test";
  artifact.workload_record_sequence_id = 42;
  result.divergence_artifact = artifact;

  // WHEN: We verify fail-closed semantics
  // THEN: Verification should pass
  EXPECT_TRUE(DivergenceAbortHandler::verifyFailClosedSemantics(result));

  // GIVEN: A ReplayResult with divergence but NO divergence artifact
  // (violation)
  ReplayResult invalid_result;
  invalid_result.execution_status = ReplayStatus::DIVERGENCE;
  invalid_result.workload_record_id = 42;
  invalid_result.replay_run_id = 1001;
  invalid_result.divergence_artifact = std::nullopt;

  // WHEN: We verify fail-closed semantics
  // THEN: An exception should be thrown
  EXPECT_THROW(
      DivergenceAbortHandler::verifyFailClosedSemantics(invalid_result),
      DivergenceAbortException);
}

// =============================================================================
// Test 12: Envelope Diff Component Extraction
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, EnvelopeDiffComponentExtraction) {
  // GIVEN: Expected digest and perturbed digest
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();

  // WHEN: We compute the envelope diff
  EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

  // THEN: We should be able to query specific component diffs
  EXPECT_TRUE(diff.hasComponentDiff("plan_hash"));
  EXPECT_FALSE(diff.hasComponentDiff("resource_signature"));

  auto plan_diff = diff.getComponentDiff("plan_hash");
  ASSERT_TRUE(plan_diff.has_value());
  EXPECT_EQ(plan_diff->component_name, "plan_hash");
  EXPECT_EQ(plan_diff->before_hash, expected.plan_hash);
  EXPECT_EQ(plan_diff->after_hash, actual.plan_hash);
}

// =============================================================================
// Test 13: Structured Error Summary Generation
// =============================================================================
TEST_F(WorkloadReplayFailClosedTest, StructuredErrorSummaryGeneration) {
  // GIVEN: A divergence error
  ExecutionDigest expected = createIdenticalDigest();
  ExecutionDigest actual = createPerturbedDigest_PlanHash();
  auto error = DivergenceAbortHandler::createError(expected, actual, 42, 1001);

  // WHEN: We generate a summary
  std::string summary = error.getSummary();

  // THEN: The summary should contain structured information
  EXPECT_NE(summary.find("ENVELOPE_PLAN_HASH_MISMATCH"), std::string::npos);
  EXPECT_NE(summary.find("run_id=1001"), std::string::npos);
  EXPECT_NE(summary.find("seq_id=42"), std::string::npos);
  EXPECT_EQ(summary.find("FAIL-CLOSED VIOLATION"),
            std::string::npos);  // Should NOT contain violation

  // GIVEN: A violation error
  StructuredDivergenceError violation = error;
  violation.partial_results_emitted = true;

  // WHEN: We generate a summary
  std::string violation_summary = violation.getSummary();

  // THEN: The summary should contain violation warning
  EXPECT_NE(violation_summary.find("FAIL-CLOSED VIOLATION"), std::string::npos);
}

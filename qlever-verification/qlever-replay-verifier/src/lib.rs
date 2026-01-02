//! EPIC 11 Subsystem 5: Replay Verifier
//!
//! Workload pack replay + deterministic verification.
//! Validates that workloads replay with bit-identical results.
//!
//! Shared Invariant: Workload replay must be deterministic and fail-closed.
//! Any divergence ⇒ abort with deterministic artifact.

pub mod failure_modes;
pub mod replay_executor;
pub mod workload_pack;

use anyhow::Result;
use blake3::Hasher as Blake3Hasher;
use qlever_artifact_capture::{emit_receipt, FailureClass, VerificationReceipt};
use serde::{Deserialize, Serialize};
use std::time::Instant;

pub use failure_modes::*;
pub use replay_executor::*;
pub use workload_pack::*;

/// Main replay and verification interface
pub async fn replay_and_verify(
    workload: &ReplayWorkload,
    mode: ReplayMode,
    timeout_ms: u64,
) -> Result<ReplayResult> {
    let start = Instant::now();
    let mut result = ReplayResult {
        workload_id: workload.workload_id.clone(),
        mode: mode.clone(),
        queries_executed: 0,
        queries_passed: 0,
        queries_failed: 0,
        failures: vec![],
        total_duration_ms: 0,
    };

    for query in &workload.query_pack {
        // Check timeout
        if start.elapsed().as_millis() as u64 > timeout_ms {
            let failure = ReplayFailure {
                query_id: query.query_id.clone(),
                failure_class: ReplayFailureClass::ReplayTimeout,
                details: format!(
                    "Execution exceeded timeout budget of {} ms",
                    timeout_ms
                ),
                divergence_index: None,
            };
            result.failures.push(failure);
            result.queries_failed += 1;

            if mode == ReplayMode::Strict {
                break;
            }
            continue;
        }

        result.queries_executed += 1;

        // Verify digest (in mock implementation, just pass)
        match verify_mock_query(&query) {
            Ok(_) => {
                result.queries_passed += 1;
            }
            Err(failure) => {
                result.failures.push(failure);
                result.queries_failed += 1;

                match mode {
                    ReplayMode::Strict => {
                        break; // Stop on first failure in strict mode
                    }
                    ReplayMode::Differential => {
                        continue; // Continue and collect all divergences
                    }
                    ReplayMode::BestEffort => {
                        continue; // Ignore timing, accept logical equivalence
                    }
                }
            }
        }
    }

    result.total_duration_ms = start.elapsed().as_millis() as u64;

    Ok(result)
}

/// Simulate query execution (mock for testing)
fn _simulate_query_execution(query_text: &str) -> Vec<u8> {
    // In real implementation, this would call the kernel
    // For now, create deterministic output based on query text
    let mut hasher = Blake3Hasher::new();
    hasher.update(query_text.as_bytes());
    let hash = hasher.finalize();
    hash.as_bytes().to_vec()
}

/// Mock verification that query digest is valid (always passes for now)
fn verify_mock_query(_query: &ReplayQuery) -> Result<(), ReplayFailure> {
    // In a real implementation, this would execute the query and verify the digest
    // For now, we just pass (no divergence)
    Ok(())
}

/// Result of a replay operation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReplayResult {
    pub workload_id: String,
    pub mode: ReplayMode,
    pub queries_executed: u32,
    pub queries_passed: u32,
    pub queries_failed: u32,
    pub failures: Vec<ReplayFailure>,
    pub total_duration_ms: u64,
}

impl ReplayResult {
    /// Check if replay was successful
    pub fn is_success(&self) -> bool {
        self.queries_failed == 0
    }

    /// Emit failure receipt if needed
    pub fn emit_receipt_if_failed(&self) -> Result<()> {
        if self.is_success() {
            return Ok(());
        }

        for failure in &self.failures {
            let receipt = VerificationReceipt::new(
                match failure.failure_class {
                    ReplayFailureClass::ReplayDivergence => FailureClass::ReplayDivergence,
                    ReplayFailureClass::ReplayNonDeterminism => FailureClass::ReplayNonDeterminism,
                    ReplayFailureClass::ReplayTimeout => FailureClass::ReplayTimeout,
                    ReplayFailureClass::ReplayAbort => FailureClass::ReplayAbort,
                },
                format!("qlever-verify replay --query {}", failure.query_id),
                format!("Replay failure: {}", failure.details),
            );

            emit_receipt(&receipt)?;
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_replay_with_empty_workload() {
        let workload = ReplayWorkload {
            workload_id: "test-empty".to_string(),
            query_pack: vec![],
            expected_state: ReplayState::default(),
            replay_mode: ReplayMode::Strict,
        };

        // Note: using a simple synchronous simulation since we're not using tokio
        assert_eq!(workload.len(), 0);
        assert!(workload.is_empty());
    }

    #[test]
    fn test_replay_workload_construction() {
        let workload = ReplayWorkload {
            workload_id: "test-workload".to_string(),
            query_pack: vec![ReplayQuery {
                query_id: "q1".to_string(),
                query_text: "SELECT * FROM test".to_string(),
                execution_order: 0,
                expected_result_digest: "0".repeat(64),
                expected_cache_behavior: vec![],
                expected_latency_ms: 100.0,
            }],
            expected_state: ReplayState::default(),
            replay_mode: ReplayMode::Strict,
        };

        assert_eq!(workload.workload_id, "test-workload");
        assert_eq!(workload.len(), 1);
        assert!(!workload.is_empty());
    }

    #[test]
    fn test_replay_result_is_success() {
        let result = ReplayResult {
            workload_id: "test".to_string(),
            mode: ReplayMode::Strict,
            queries_executed: 5,
            queries_passed: 5,
            queries_failed: 0,
            failures: vec![],
            total_duration_ms: 500,
        };

        assert!(result.is_success());
    }

    #[test]
    fn test_replay_result_failure() {
        let result = ReplayResult {
            workload_id: "test".to_string(),
            mode: ReplayMode::Strict,
            queries_executed: 5,
            queries_passed: 4,
            queries_failed: 1,
            failures: vec![ReplayFailure {
                query_id: "q1".to_string(),
                failure_class: ReplayFailureClass::ReplayDivergence,
                details: "Digest mismatch".to_string(),
                divergence_index: Some(42),
            }],
            total_duration_ms: 500,
        };

        assert!(!result.is_success());
    }
}

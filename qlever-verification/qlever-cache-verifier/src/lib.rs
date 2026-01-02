//! EPIC 11 Subsystem 4: Cache Verifier
//!
//! Cache decision log verification - ensures no silent cache behavior.
//! Implements Invariant B4 (Cache Transparency) from EPIC 11.
//!
//! Shared Invariant: Rust is the verification plane that makes cache correctness
//! and epoch isolation non-negotiable. All failures are fail-closed with
//! deterministic receipts.

pub mod decision_log;
pub mod decision_classifier;

pub use decision_classifier::{classify_decision, is_get_operation, is_put_operation, is_success, CacheTier};
pub use decision_log::{CacheDecision, CacheDecisionLog, CacheDecisionType};
use qlever_artifact_capture::{emit_receipt, FailureClass, VerificationReceipt};
use thiserror::Error;

/// Cache transparency verification errors
#[derive(Debug, Error)]
pub enum CacheVerifierError {
    #[error("Silent cache behavior detected: no decision record for query {query_id}")]
    SilentCacheBehavior { query_id: String },

    #[error("Missing decision record at index {index}")]
    MissingDecisionRecord { index: usize },

    #[error("Cache behavior divergence: expected {expected:?}, got {actual:?}")]
    CacheBehaviorDivergence {
        expected: Vec<CacheDecisionType>,
        actual: Vec<CacheDecisionType>,
    },

    #[error("Decision log parse error: {0}")]
    ParseError(String),

    #[error("Decision log hash mismatch")]
    LogHashMismatch,
}

impl CacheVerifierError {
    /// Convert to failure class for receipt generation
    pub fn to_failure_class(&self) -> FailureClass {
        match self {
            CacheVerifierError::SilentCacheBehavior { .. } => FailureClass::SilentCacheBehavior,
            CacheVerifierError::MissingDecisionRecord { .. } => FailureClass::SilentCacheBehavior,
            CacheVerifierError::CacheBehaviorDivergence { .. } => {
                FailureClass::CacheBehaviorDivergence
            }
            CacheVerifierError::ParseError(_) => FailureClass::KernelContractViolation,
            CacheVerifierError::LogHashMismatch => FailureClass::CacheBehaviorDivergence,
        }
    }
}

/// Verify cache transparency invariant
///
/// CACHE_TRANSPARENCY_INVARIANT:
///   For all cache operation (get, put, evict, admit, reject, guarded):
///     Decision MUST be recorded in machine-queryable decision_log
///
///   VIOLATION: Missing decision record -> ABORT with RECEIPT
pub fn verify_transparency(
    decision_log: &CacheDecisionLog,
    expected_query_ids: &[String],
) -> Result<(), CacheVerifierError> {
    for query_id in expected_query_ids {
        let decisions = decision_log.get_decisions_for_query(query_id);
        if decisions.is_empty() {
            return Err(CacheVerifierError::SilentCacheBehavior {
                query_id: query_id.clone(),
            });
        }
    }
    Ok(())
}

/// Verify that decision log matches expected behavior sequence
pub fn verify_behavior_sequence(
    decision_log: &CacheDecisionLog,
    query_id: &str,
    expected_decisions: &[CacheDecisionType],
) -> Result<(), CacheVerifierError> {
    let actual_decisions: Vec<CacheDecisionType> = decision_log
        .get_decisions_for_query(query_id)
        .iter()
        .map(|d| d.decision.clone())
        .collect();

    if actual_decisions != expected_decisions {
        return Err(CacheVerifierError::CacheBehaviorDivergence {
            expected: expected_decisions.to_vec(),
            actual: actual_decisions,
        });
    }

    Ok(())
}

/// Verify complete decision log coverage
pub fn verify_complete_coverage(
    decision_log: &CacheDecisionLog,
    total_operations: usize,
) -> Result<(), CacheVerifierError> {
    if decision_log.len() < total_operations {
        return Err(CacheVerifierError::MissingDecisionRecord {
            index: decision_log.len(),
        });
    }
    Ok(())
}

/// Verify no silent hits (every cache hit must be logged)
pub fn verify_no_silent_hits(
    decision_log: &CacheDecisionLog,
    cache_stats_hits: u64,
) -> Result<(), CacheVerifierError> {
    let logged_hits = decision_log
        .decisions()
        .iter()
        .filter(|d| matches!(d.decision, CacheDecisionType::Hit))
        .count() as u64;

    if logged_hits < cache_stats_hits {
        return Err(CacheVerifierError::SilentCacheBehavior {
            query_id: format!("missing {} hit records", cache_stats_hits - logged_hits),
        });
    }

    Ok(())
}

/// Emit a failure receipt for cache verification errors
pub fn emit_cache_failure_receipt(error: &CacheVerifierError) -> std::path::PathBuf {
    let receipt = VerificationReceipt::new(
        error.to_failure_class(),
        "qlever-verify cache --check-transparency".to_string(),
        format!("Investigate cache transparency error: {}", error),
    );

    emit_receipt(&receipt).unwrap_or_else(|_| std::path::PathBuf::from("/tmp/error.receipt"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_verify_transparency_passes() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        let result = verify_transparency(&log, &["q1".to_string()]);
        assert!(result.is_ok());
    }

    #[test]
    fn test_silent_cache_behavior_detected() {
        let log = CacheDecisionLog::new();

        let result = verify_transparency(&log, &["q1".to_string()]);
        assert!(matches!(
            result,
            Err(CacheVerifierError::SilentCacheBehavior { .. })
        ));
    }

    #[test]
    fn test_behavior_sequence_verification() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });
        log.record(CacheDecision {
            timestamp_ns: 2000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Admit,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        let expected = vec![CacheDecisionType::Miss, CacheDecisionType::Admit];
        let result = verify_behavior_sequence(&log, "q1", &expected);
        assert!(result.is_ok());
    }

    #[test]
    fn test_no_silent_hits() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Hit,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        // One logged hit, one reported hit = OK
        assert!(verify_no_silent_hits(&log, 1).is_ok());

        // One logged hit, two reported hits = silent hit
        assert!(matches!(
            verify_no_silent_hits(&log, 2),
            Err(CacheVerifierError::SilentCacheBehavior { .. })
        ));
    }
}

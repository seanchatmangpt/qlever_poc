//! EPIC 11 Subsystem 3: Digest Verifier
//!
//! Result digest verification using BLAKE3.
//! Implements the determinism formula:
//! BLAKE3(result_bytes || cache_decision_log) = expected_digest
//!
//! Shared Invariant: Rust is the verification plane that makes cache correctness
//! and epoch isolation non-negotiable. All failures are fail-closed with
//! deterministic receipts.

pub mod blake3_hash;
pub mod equivalence_rules;

use blake3_hash::Blake3Hash;
use qlever_artifact_capture::{emit_receipt, FailureClass, VerificationReceipt};
use serde::{Deserialize, Serialize};
use thiserror::Error;

/// Determinism digest structure (per EPIC 11 Invariant B2)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DeterminismDigest {
    pub query_id: String,
    /// BLAKE3 of result_bytes
    pub result_hash: Blake3Hash,
    /// BLAKE3 of cache decision log
    pub cache_log_hash: Blake3Hash,
    /// BLAKE3(result_hash || cache_log_hash)
    pub combined_digest: Blake3Hash,
    /// Machine fingerprint for reproducibility
    pub machine_fingerprint: MachineInfo,
}

/// Machine information for cross-machine reproducibility
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct MachineInfo {
    pub cpu_model: String,
    pub os_name: String,
    pub libc_version: String,
    pub architecture: String,
}

impl MachineInfo {
    /// Generate a machine identifier string for cross-machine verification
    pub fn machine_id(&self) -> String {
        format!(
            "{}-{}-{}-{}",
            self.architecture, self.os_name, self.libc_version, self.cpu_model
        )
    }
}

/// Digest verification errors
#[derive(Debug, Error)]
pub enum DigestError {
    #[error("Result hash mismatch: expected {expected}, got {actual}")]
    ResultHashMismatch { expected: String, actual: String },

    #[error("Cache log hash mismatch: expected {expected}, got {actual}")]
    CacheLogHashMismatch { expected: String, actual: String },

    #[error("Combined digest mismatch: expected {expected}, got {actual}")]
    CombinedDigestMismatch { expected: String, actual: String },

    #[error("Divergence detected at byte index {index}")]
    DivergenceDetected { index: usize },

    #[error("Non-determinism detected: run {run} produced different result")]
    NonDeterminismDetected { run: u32 },
}

impl DigestError {
    /// Convert to failure class for receipt generation
    pub fn to_failure_class(&self) -> FailureClass {
        match self {
            DigestError::ResultHashMismatch { .. } => FailureClass::ReplayDivergence,
            DigestError::CacheLogHashMismatch { .. } => FailureClass::CacheBehaviorDivergence,
            DigestError::CombinedDigestMismatch { .. } => FailureClass::ReplayDivergence,
            DigestError::DivergenceDetected { .. } => FailureClass::ReplayDivergence,
            DigestError::NonDeterminismDetected { .. } => FailureClass::ReplayNonDeterminism,
        }
    }
}

/// Verification result
#[derive(Debug, Clone)]
pub enum VerificationResult {
    /// Digests match exactly
    Match,
    /// Divergence detected with evidence
    Divergence {
        expected: DeterminismDigest,
        actual: DeterminismDigest,
        first_divergence_index: Option<usize>,
    },
}

/// Compute a determinism digest from result bytes and cache log
pub fn compute_digest(
    query_id: &str,
    result_bytes: &[u8],
    cache_log_bytes: &[u8],
) -> DeterminismDigest {
    let result_hash = blake3_hash::hash_bytes(result_bytes);
    let cache_log_hash = blake3_hash::hash_bytes(cache_log_bytes);

    // Combined digest: BLAKE3(result_hash || cache_log_hash)
    let mut combined_input = Vec::with_capacity(64);
    combined_input.extend_from_slice(result_hash.as_bytes());
    combined_input.extend_from_slice(cache_log_hash.as_bytes());
    let combined_digest = blake3_hash::hash_bytes(&combined_input);

    DeterminismDigest {
        query_id: query_id.to_string(),
        result_hash,
        cache_log_hash,
        combined_digest,
        machine_fingerprint: MachineInfo::default(),
    }
}

/// Verify that actual digest matches expected digest
pub fn verify_digest(
    expected: &DeterminismDigest,
    actual: &DeterminismDigest,
) -> Result<VerificationResult, DigestError> {
    // Check result hash
    if expected.result_hash != actual.result_hash {
        return Err(DigestError::ResultHashMismatch {
            expected: expected.result_hash.to_hex(),
            actual: actual.result_hash.to_hex(),
        });
    }

    // Check cache log hash
    if expected.cache_log_hash != actual.cache_log_hash {
        return Err(DigestError::CacheLogHashMismatch {
            expected: expected.cache_log_hash.to_hex(),
            actual: actual.cache_log_hash.to_hex(),
        });
    }

    // Check combined digest
    if expected.combined_digest != actual.combined_digest {
        return Err(DigestError::CombinedDigestMismatch {
            expected: expected.combined_digest.to_hex(),
            actual: actual.combined_digest.to_hex(),
        });
    }

    Ok(VerificationResult::Match)
}

/// Verify digest with raw bytes comparison for finding divergence index
pub fn verify_with_evidence(
    expected_result: &[u8],
    actual_result: &[u8],
    expected_cache_log: &[u8],
    actual_cache_log: &[u8],
) -> Result<VerificationResult, DigestError> {
    // Find first divergence in result bytes
    if expected_result != actual_result {
        let divergence_index = expected_result
            .iter()
            .zip(actual_result.iter())
            .position(|(a, b)| a != b)
            .or_else(|| {
                if expected_result.len() != actual_result.len() {
                    Some(expected_result.len().min(actual_result.len()))
                } else {
                    None
                }
            });

        return Err(DigestError::DivergenceDetected {
            index: divergence_index.unwrap_or(0),
        });
    }

    // Find first divergence in cache log
    if expected_cache_log != actual_cache_log {
        let divergence_index = expected_cache_log
            .iter()
            .zip(actual_cache_log.iter())
            .position(|(a, b)| a != b)
            .or_else(|| {
                if expected_cache_log.len() != actual_cache_log.len() {
                    Some(expected_cache_log.len().min(actual_cache_log.len()))
                } else {
                    None
                }
            });

        return Err(DigestError::DivergenceDetected {
            index: divergence_index.unwrap_or(0),
        });
    }

    Ok(VerificationResult::Match)
}

/// Run multiple verification runs to detect non-determinism
pub fn verify_determinism(
    query_id: &str,
    compute_fn: impl Fn() -> (Vec<u8>, Vec<u8>),
    runs: u32,
) -> Result<DeterminismDigest, DigestError> {
    let (first_result, first_log) = compute_fn();
    let baseline = compute_digest(query_id, &first_result, &first_log);

    for run in 2..=runs {
        let (result, log) = compute_fn();
        let current = compute_digest(query_id, &result, &log);

        if baseline.combined_digest != current.combined_digest {
            return Err(DigestError::NonDeterminismDetected { run });
        }
    }

    Ok(baseline)
}

/// Emit a failure receipt for digest verification errors
pub fn emit_digest_failure_receipt(error: &DigestError, query_id: &str) -> std::path::PathBuf {
    let receipt = VerificationReceipt::new(
        error.to_failure_class(),
        format!("qlever-verify replay --query {}", query_id),
        format!("Investigate digest verification error: {}", error),
    );

    emit_receipt(&receipt).unwrap_or_else(|_| std::path::PathBuf::from("/tmp/error.receipt"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_same_input_produces_same_digest() {
        let result_bytes = b"test result data";
        let cache_log = b"[{\"decision\":\"HIT\"}]";

        let digest1 = compute_digest("q1", result_bytes, cache_log);
        let digest2 = compute_digest("q1", result_bytes, cache_log);

        assert_eq!(digest1.result_hash, digest2.result_hash);
        assert_eq!(digest1.cache_log_hash, digest2.cache_log_hash);
        assert_eq!(digest1.combined_digest, digest2.combined_digest);
    }

    #[test]
    fn test_different_input_produces_different_digest() {
        let result1 = b"result A";
        let result2 = b"result B";
        let cache_log = b"[]";

        let digest1 = compute_digest("q1", result1, cache_log);
        let digest2 = compute_digest("q1", result2, cache_log);

        assert_ne!(digest1.result_hash, digest2.result_hash);
        assert_ne!(digest1.combined_digest, digest2.combined_digest);
    }

    #[test]
    fn test_verify_matching_digests() {
        let result = b"test data";
        let log = b"cache log";

        let expected = compute_digest("q1", result, log);
        let actual = compute_digest("q1", result, log);

        let verification = verify_digest(&expected, &actual).unwrap();
        assert!(matches!(verification, VerificationResult::Match));
    }

    #[test]
    fn test_verify_mismatched_digests() {
        let expected = compute_digest("q1", b"data A", b"log");
        let actual = compute_digest("q1", b"data B", b"log");

        let result = verify_digest(&expected, &actual);
        assert!(matches!(result, Err(DigestError::ResultHashMismatch { .. })));
    }

    #[test]
    fn test_determinism_verification_passes() {
        let result = verify_determinism("q1", || (b"constant".to_vec(), b"log".to_vec()), 100);
        assert!(result.is_ok());
    }

    #[test]
    fn test_100_runs_same_digest() {
        let result_bytes = b"deterministic result";
        let cache_log = b"[{\"decision\":\"MISS\"}]";

        let baseline = compute_digest("q1", result_bytes, cache_log);

        for _ in 0..100 {
            let current = compute_digest("q1", result_bytes, cache_log);
            assert_eq!(baseline.combined_digest, current.combined_digest);
        }
    }
}

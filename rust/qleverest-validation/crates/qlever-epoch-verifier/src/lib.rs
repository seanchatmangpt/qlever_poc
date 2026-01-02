//! EPIC 11 Subsystem 7: Epoch Verifier
//!
//! Epoch isolation verification - ensures no cross-epoch cache contamination.
//! Implements Invariant A from EPIC 11.
//!
//! Shared Invariant: Rust is the verification plane that makes cache correctness
//! and epoch isolation non-negotiable. All failures are fail-closed with
//! deterministic receipts.

pub mod epoch_key;

pub use qlever_artifact_capture::FailureClass;
pub use qlever_kernel_runner::CacheTier;
use qlever_artifact_capture::{emit_receipt, VerificationReceipt};
use serde::{Deserialize, Serialize};
use thiserror::Error;

/// Epoch structure (per EPIC 11 Invariant A1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Epoch {
    /// Monotonic counter (0, 1, 2, ...)
    pub generation_id: u64,
    /// Hash of (manifest_digest + guard_config_digest)
    pub epoch_key: [u8; 32],
    /// Cache generation at epoch creation
    pub cache_snapshot_version: u64,
    /// Nanoseconds since UNIX epoch (start)
    pub start_timestamp_ns: u64,
    /// Nanoseconds since UNIX epoch (if closed)
    pub end_timestamp_ns: Option<u64>,
}

impl Epoch {
    /// Create a new epoch
    pub fn new(
        generation_id: u64,
        epoch_key: [u8; 32],
        cache_snapshot_version: u64,
        start_timestamp_ns: u64,
    ) -> Self {
        Self {
            generation_id,
            epoch_key,
            cache_snapshot_version,
            start_timestamp_ns,
            end_timestamp_ns: None,
        }
    }

    /// Get the epoch prefix (first 8 bytes of epoch_key)
    /// Used for cache key binding
    pub fn epoch_prefix(&self) -> [u8; 8] {
        let mut prefix = [0u8; 8];
        prefix.copy_from_slice(&self.epoch_key[0..8]);
        prefix
    }

    /// Close this epoch
    pub fn close(&mut self, end_timestamp_ns: u64) {
        self.end_timestamp_ns = Some(end_timestamp_ns);
    }

    /// Check if this epoch is closed
    pub fn is_closed(&self) -> bool {
        self.end_timestamp_ns.is_some()
    }
}

/// Cache key with epoch binding (per EPIC 11 Invariant A3)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CacheKeyWithEpoch {
    /// E_i.epoch_key[0:8]
    pub epoch_prefix: [u8; 8],
    /// BLAKE3(query_text)
    pub query_hash: [u8; 32],
    /// Cache tier (bytes/neg/plan)
    pub cache_tier: CacheTier,
}

impl CacheKeyWithEpoch {
    /// Create a cache key bound to an epoch
    pub fn new(epoch: &Epoch, query_hash: [u8; 32], cache_tier: CacheTier) -> Self {
        Self {
            epoch_prefix: epoch.epoch_prefix(),
            query_hash,
            cache_tier,
        }
    }

    /// Verify this cache key belongs to the given epoch
    pub fn belongs_to_epoch(&self, epoch: &Epoch) -> bool {
        self.epoch_prefix == epoch.epoch_prefix()
    }

    /// Get the full cache key as bytes
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(48 + self.cache_tier.len());
        bytes.extend_from_slice(&self.epoch_prefix);
        bytes.extend_from_slice(&self.query_hash);
        bytes.extend_from_slice(self.cache_tier.as_bytes());
        bytes
    }
}

/// Epoch isolation errors
#[derive(Debug, Error)]
pub enum EpochError {
    #[error("Cross-epoch contamination: query in epoch {0} used cache from epoch {1}")]
    EpochContamination(u64, u64), // (query_epoch, cache_epoch)

    #[error("Epoch key mismatch: expected {0:?}, got {1:?}")]
    EpochKeyMismatch([u8; 8], [u8; 8]), // (expected, actual)

    #[error("Promotion boundary violation: epoch-mortal result crossed from epoch {0} to {1}")]
    PromotionBoundaryViolation(u64, u64), // (source, target)

    #[error("SIMD result cross-epoch: SIMD results cannot cross epoch boundaries")]
    SimdCrossEpoch,
}

impl EpochError {
    /// Convert to failure class for receipt generation
    pub fn to_failure_class(&self) -> FailureClass {
        match self {
            EpochError::EpochContamination { .. } => FailureClass::EpochContamination,
            EpochError::EpochKeyMismatch { .. } => FailureClass::EpochKeyMismatch,
            EpochError::PromotionBoundaryViolation { .. } => {
                FailureClass::PromotionBoundaryViolation
            }
            EpochError::SimdCrossEpoch => FailureClass::PromotionBoundaryViolation,
        }
    }
}

/// Cache result metadata for isolation verification
#[derive(Debug, Clone)]
pub struct CacheResultMeta {
    /// The cache key used
    pub cache_key: CacheKeyWithEpoch,
    /// Generation ID of the epoch that created this cache entry
    pub source_epoch_id: u64,
    /// Whether this is a SIMD-computed result
    pub is_simd_result: bool,
    /// Whether this result is marked epoch-immortal
    pub is_epoch_immortal: bool,
}

/// Verify epoch isolation invariant
///
/// EPOCH_ISOLATION_INVARIANT:
///   For all query Q executed in epoch E_i:
///     For all cache result R returned for Q:
///       cache_key(Q).epoch_prefix == E_i.epoch_key[0:8]
///
///   VIOLATION: Cross-epoch hit detected -> ABORT
pub fn verify_isolation(
    current_epoch: &Epoch,
    cache_result: &CacheResultMeta,
) -> Result<(), EpochError> {
    // Check epoch prefix matches
    if !cache_result.cache_key.belongs_to_epoch(current_epoch) {
        return Err(EpochError::EpochContamination(
            current_epoch.generation_id,
            cache_result.source_epoch_id,
        ));
    }

    // SIMD results cannot cross epoch boundaries (always epoch-mortal)
    if cache_result.is_simd_result && cache_result.source_epoch_id != current_epoch.generation_id {
        return Err(EpochError::SimdCrossEpoch);
    }

    Ok(())
}

/// Verify promotion/swap boundaries (Invariant A4)
pub fn verify_promotion_boundary(
    source_epoch: &Epoch,
    target_epoch: &Epoch,
    cache_result: &CacheResultMeta,
) -> Result<(), EpochError> {
    // SIMD results cannot be promoted across epochs
    if cache_result.is_simd_result {
        return Err(EpochError::SimdCrossEpoch);
    }

    // Non-immortal results cannot be promoted
    if !cache_result.is_epoch_immortal {
        return Err(EpochError::PromotionBoundaryViolation(
            source_epoch.generation_id,
            target_epoch.generation_id,
        ));
    }

    Ok(())
}

/// Emit a failure receipt for epoch errors
pub fn emit_epoch_failure_receipt(
    error: &EpochError,
    query_id: &str,
    epoch: &Epoch,
) -> std::path::PathBuf {
    let receipt = VerificationReceipt::new(
        error.to_failure_class(),
        format!(
            "qlever-verify replay --epoch {} --query {}",
            epoch.generation_id, query_id
        ),
        format!("Investigate epoch isolation error: {}", error),
    );

    emit_receipt(&receipt).unwrap_or_else(|_| std::path::PathBuf::from("/tmp/error.receipt"))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_epoch(gen_id: u64) -> Epoch {
        let mut epoch_key = [0u8; 32];
        epoch_key[0] = gen_id as u8;
        Epoch::new(gen_id, epoch_key, gen_id, gen_id * 1000)
    }

    #[test]
    fn test_epoch_prefix() {
        let epoch = create_test_epoch(1);
        let prefix = epoch.epoch_prefix();
        assert_eq!(prefix[0], 1);
        assert_eq!(prefix.len(), 8);
    }

    #[test]
    fn test_cache_key_epoch_binding() {
        let epoch = create_test_epoch(1);
        let cache_key = CacheKeyWithEpoch::new(&epoch, [0u8; 32], CacheTier::Bytes);

        assert!(cache_key.belongs_to_epoch(&epoch));

        let other_epoch = create_test_epoch(2);
        assert!(!cache_key.belongs_to_epoch(&other_epoch));
    }

    #[test]
    fn test_verify_isolation_passes() {
        let epoch = create_test_epoch(1);
        let cache_key = CacheKeyWithEpoch::new(&epoch, [0u8; 32], CacheTier::Bytes);

        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: false,
        };

        assert!(verify_isolation(&epoch, &result).is_ok());
    }

    #[test]
    fn test_epoch_contamination_detected() {
        let epoch1 = create_test_epoch(1);
        let epoch2 = create_test_epoch(2);

        let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);

        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: false,
        };

        let verification = verify_isolation(&epoch2, &result);
        assert!(matches!(
            verification,
            Err(EpochError::EpochContamination(2, 1))
        ));
    }

    #[test]
    fn test_simd_cross_epoch_blocked() {
        let _epoch1 = create_test_epoch(1);
        let epoch2 = create_test_epoch(2);

        let cache_key = CacheKeyWithEpoch::new(&epoch2, [0u8; 32], CacheTier::Bytes);

        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: true,
            is_epoch_immortal: false,
        };

        let verification = verify_isolation(&epoch2, &result);
        assert!(matches!(verification, Err(EpochError::SimdCrossEpoch)));
    }

    #[test]
    fn test_promotion_boundary_enforced() {
        let epoch1 = create_test_epoch(1);
        let epoch2 = create_test_epoch(2);

        let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);

        // Non-immortal result cannot be promoted
        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: false,
        };

        let verification = verify_promotion_boundary(&epoch1, &epoch2, &result);
        assert!(matches!(
            verification,
            Err(EpochError::PromotionBoundaryViolation(1, 2))
        ));
    }
}

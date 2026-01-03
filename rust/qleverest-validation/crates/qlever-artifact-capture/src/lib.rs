//! EPIC 11 Subsystem 2: Artifact Capture
//!
//! Receipt generation + CBOR serialization for verification artifacts.
//! All failures produce machine-checkable CBOR artifacts with hash evidence.
//!
//! Shared Invariant: Rust is the verification plane that makes cache correctness
//! and epoch isolation non-negotiable. All failures are fail-closed with
//! deterministic receipts.

pub mod receipt_format;

use chrono::Utc;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::io::Write;
use std::path::PathBuf;
use thiserror::Error;

/// Receipt version for EPIC 11
pub const RECEIPT_VERSION: u32 = 1;

/// Default receipt storage path
pub const RECEIPT_STORAGE_PATH: &str = "/tmp/qlever-verification-receipts";

/// Failure class taxonomy (10 major classes, 13 sub-classes)
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(tag = "category", content = "variant")]
pub enum FailureClass {
    // EPOCH_ISOLATION_FAILURES
    EpochContamination,
    EpochKeyMismatch,
    PromotionBoundaryViolation,

    // DETERMINISM_FAILURES
    ReplayDivergence,
    ReplayNonDeterminism,
    CacheBehaviorDivergence,
    SilentCacheBehavior,

    // SIMD_EQUIVALENCE_FAILURES
    SimdScalarMismatch,
    ArchitectureDivergence,
    SIMDNondeterminism,

    // PERFORMANCE_FAILURES
    LatencyRegression,
    ThroughputRegression,
    MemoryRegression,
    CacheHitRateRegression,

    // REPLAY_FAILURES
    ReplayTimeout,
    ReplayAbort,
    WorkloadPackMismatch,

    // CROSS_MACHINE_FAILURES
    MachineNondeterminism,
    OSNondeterminism,
    LibcNondeterminism,

    // CONTRACT_FAILURES
    KernelContractViolation,
    FFIMemorySafety,
    CacheTierMismatch,
}

impl FailureClass {
    /// Returns whether this failure class should block the build
    pub fn is_blocking(&self) -> bool {
        match self {
            // Epoch isolation failures are always blocking
            FailureClass::EpochContamination => true,
            FailureClass::EpochKeyMismatch => true,
            FailureClass::PromotionBoundaryViolation => true,

            // Determinism failures are always blocking
            FailureClass::ReplayDivergence => true,
            FailureClass::ReplayNonDeterminism => true,
            FailureClass::CacheBehaviorDivergence => true,
            FailureClass::SilentCacheBehavior => true,

            // SIMD failures are blocking
            FailureClass::SimdScalarMismatch => true,
            FailureClass::ArchitectureDivergence => true,
            FailureClass::SIMDNondeterminism => true,

            // Performance failures: latency/throughput blocking, memory advisory
            FailureClass::LatencyRegression => true,
            FailureClass::ThroughputRegression => true,
            FailureClass::MemoryRegression => false, // Advisory
            FailureClass::CacheHitRateRegression => true,

            // Replay failures are blocking
            FailureClass::ReplayTimeout => true,
            FailureClass::ReplayAbort => true,
            FailureClass::WorkloadPackMismatch => true,

            // Cross-machine failures are advisory (nightly only)
            FailureClass::MachineNondeterminism => false,
            FailureClass::OSNondeterminism => false,
            FailureClass::LibcNondeterminism => false,

            // Contract failures are always blocking
            FailureClass::KernelContractViolation => true,
            FailureClass::FFIMemorySafety => true,
            FailureClass::CacheTierMismatch => true,
        }
    }

    /// Returns tags for this failure class
    pub fn tags(&self) -> Vec<String> {
        match self {
            FailureClass::EpochContamination
            | FailureClass::EpochKeyMismatch
            | FailureClass::PromotionBoundaryViolation => {
                vec!["epoch".to_string(), "isolation".to_string()]
            }

            FailureClass::ReplayDivergence
            | FailureClass::ReplayNonDeterminism
            | FailureClass::CacheBehaviorDivergence
            | FailureClass::SilentCacheBehavior => {
                vec!["determinism".to_string(), "replay".to_string()]
            }

            FailureClass::SimdScalarMismatch
            | FailureClass::ArchitectureDivergence
            | FailureClass::SIMDNondeterminism => {
                vec!["simd".to_string(), "architecture".to_string()]
            }

            FailureClass::LatencyRegression
            | FailureClass::ThroughputRegression
            | FailureClass::MemoryRegression
            | FailureClass::CacheHitRateRegression => {
                vec!["performance".to_string(), "regression".to_string()]
            }

            FailureClass::ReplayTimeout
            | FailureClass::ReplayAbort
            | FailureClass::WorkloadPackMismatch => {
                vec!["replay".to_string(), "workload".to_string()]
            }

            FailureClass::MachineNondeterminism
            | FailureClass::OSNondeterminism
            | FailureClass::LibcNondeterminism => {
                vec!["cross-machine".to_string(), "reproducibility".to_string()]
            }

            FailureClass::KernelContractViolation
            | FailureClass::FFIMemorySafety
            | FailureClass::CacheTierMismatch => {
                vec!["contract".to_string(), "ffi".to_string()]
            }
        }
    }
}

/// Machine fingerprint for reproducibility tracking
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MachineFingerprint {
    pub cpu_model: String,
    pub cpu_features: Vec<String>,
    pub os_name: String,
    pub os_version: String,
    pub libc_version: String,
    pub architecture: String,
}

impl Default for MachineFingerprint {
    fn default() -> Self {
        Self {
            cpu_model: "unknown".to_string(),
            cpu_features: vec![],
            os_name: std::env::consts::OS.to_string(),
            os_version: "unknown".to_string(),
            libc_version: "unknown".to_string(),
            architecture: std::env::consts::ARCH.to_string(),
        }
    }
}

/// Verification receipt - all failures produce machine-checkable CBOR artifacts
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VerificationReceipt {
    // Metadata
    pub receipt_version: u32,
    pub timestamp_iso8601: String,
    pub machine_fingerprint: MachineFingerprint,
    pub qlever_version: String,

    // Failure classification
    pub failure_class: FailureClass,
    pub is_blocking: bool,

    // Evidence
    pub evidence: HashMap<String, Vec<u8>>,
    pub reproduction_command: String,
    pub digest_evidence: Option<String>,

    // Metadata for triage
    pub tags: Vec<String>,
    pub recommended_action: String,
}

impl VerificationReceipt {
    /// Create a new verification receipt
    pub fn new(
        failure_class: FailureClass,
        reproduction_command: String,
        recommended_action: String,
    ) -> Self {
        let is_blocking = failure_class.is_blocking();
        let tags = failure_class.tags();

        Self {
            receipt_version: RECEIPT_VERSION,
            timestamp_iso8601: Utc::now().to_rfc3339(),
            machine_fingerprint: MachineFingerprint::default(),
            qlever_version: env!("CARGO_PKG_VERSION").to_string(),
            failure_class,
            is_blocking,
            evidence: HashMap::new(),
            reproduction_command,
            digest_evidence: None,
            tags,
            recommended_action,
        }
    }

    /// Add evidence to the receipt
    pub fn with_evidence(mut self, key: &str, data: Vec<u8>) -> Self {
        self.evidence.insert(key.to_string(), data);
        self
    }

    /// Add digest evidence
    pub fn with_digest_evidence(mut self, digest: String) -> Self {
        self.digest_evidence = Some(digest);
        self
    }

    /// Set machine fingerprint
    pub fn with_machine_fingerprint(mut self, fingerprint: MachineFingerprint) -> Self {
        self.machine_fingerprint = fingerprint;
        self
    }

    /// Generate the receipt filename
    pub fn filename(&self) -> String {
        let timestamp = &self.timestamp_iso8601.replace([':', '-'], "");
        let class_name = format!("{:?}", self.failure_class);
        format!("{}-{}.receipt.cbor", timestamp, class_name)
    }
}

/// Errors that can occur during receipt operations
#[derive(Debug, Error)]
pub enum ReceiptError {
    #[error("Failed to create receipt directory: {0}")]
    DirectoryCreationFailed(std::io::Error),

    #[error("Failed to serialize receipt: {0}")]
    SerializationFailed(String),

    #[error("Failed to write receipt: {0}")]
    WriteFailed(std::io::Error),

    #[error("Failed to read receipt: {0}")]
    ReadFailed(std::io::Error),

    #[error("Failed to deserialize receipt: {0}")]
    DeserializationFailed(String),
}

/// Emit a verification receipt to disk
pub fn emit_receipt(receipt: &VerificationReceipt) -> Result<PathBuf, ReceiptError> {
    emit_receipt_to_path(receipt, RECEIPT_STORAGE_PATH)
}

/// Emit a verification receipt to a specific path
pub fn emit_receipt_to_path(
    receipt: &VerificationReceipt,
    base_path: &str,
) -> Result<PathBuf, ReceiptError> {
    // Create directory if it doesn't exist
    fs::create_dir_all(base_path).map_err(ReceiptError::DirectoryCreationFailed)?;

    // Serialize to CBOR
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(receipt, &mut cbor_bytes)
        .map_err(|e| ReceiptError::SerializationFailed(e.to_string()))?;

    // Write to file
    let filepath = PathBuf::from(base_path).join(receipt.filename());
    let mut file = fs::File::create(&filepath).map_err(ReceiptError::WriteFailed)?;
    file.write_all(&cbor_bytes)
        .map_err(ReceiptError::WriteFailed)?;

    Ok(filepath)
}

/// Read a verification receipt from disk
pub fn read_receipt(filepath: &PathBuf) -> Result<VerificationReceipt, ReceiptError> {
    let bytes = fs::read(filepath).map_err(ReceiptError::ReadFailed)?;
    ciborium::from_reader(&bytes[..])
        .map_err(|e| ReceiptError::DeserializationFailed(e.to_string()))
}

/// Success receipt (even passing tests produce metadata receipt)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SuccessReceipt {
    pub receipt_version: u32,
    pub timestamp_iso8601: String,
    pub machine_fingerprint: MachineFingerprint,
    pub qlever_version: String,
    pub test_category: String,
    pub tests_passed: u32,
    pub total_duration_ms: u64,
    pub digest: String,
}

impl SuccessReceipt {
    /// Create a new success receipt
    pub fn new(test_category: &str, tests_passed: u32, total_duration_ms: u64, digest: String) -> Self {
        Self {
            receipt_version: RECEIPT_VERSION,
            timestamp_iso8601: Utc::now().to_rfc3339(),
            machine_fingerprint: MachineFingerprint::default(),
            qlever_version: env!("CARGO_PKG_VERSION").to_string(),
            test_category: test_category.to_string(),
            tests_passed,
            total_duration_ms,
            digest,
        }
    }

    /// Generate the receipt filename
    pub fn filename(&self) -> String {
        let timestamp = &self.timestamp_iso8601.replace([':', '-'], "");
        format!("{}-{}-success.receipt.cbor", timestamp, self.test_category)
    }
}

/// Emit a success receipt to disk
pub fn emit_success_receipt(receipt: &SuccessReceipt) -> Result<PathBuf, ReceiptError> {
    emit_success_receipt_to_path(receipt, RECEIPT_STORAGE_PATH)
}

/// Emit a success receipt to a specific path
pub fn emit_success_receipt_to_path(
    receipt: &SuccessReceipt,
    base_path: &str,
) -> Result<PathBuf, ReceiptError> {
    fs::create_dir_all(base_path).map_err(ReceiptError::DirectoryCreationFailed)?;

    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(receipt, &mut cbor_bytes)
        .map_err(|e| ReceiptError::SerializationFailed(e.to_string()))?;

    let filepath = PathBuf::from(base_path).join(receipt.filename());
    let mut file = fs::File::create(&filepath).map_err(ReceiptError::WriteFailed)?;
    file.write_all(&cbor_bytes)
        .map_err(ReceiptError::WriteFailed)?;

    Ok(filepath)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_failure_class_is_blocking() {
        assert!(FailureClass::EpochContamination.is_blocking());
        assert!(FailureClass::ReplayDivergence.is_blocking());
        assert!(!FailureClass::MemoryRegression.is_blocking());
        assert!(!FailureClass::MachineNondeterminism.is_blocking());
    }

    #[test]
    fn test_receipt_creation() {
        let receipt = VerificationReceipt::new(
            FailureClass::EpochContamination,
            "qlever-verify replay --epoch-A 1 --epoch-B 2".to_string(),
            "Investigate cache eviction".to_string(),
        );

        assert_eq!(receipt.receipt_version, RECEIPT_VERSION);
        assert!(receipt.is_blocking);
        assert!(receipt.tags.contains(&"epoch".to_string()));
    }

    #[test]
    fn test_all_failure_classes_serializable() {
        let all_classes = vec![
            FailureClass::EpochContamination,
            FailureClass::EpochKeyMismatch,
            FailureClass::PromotionBoundaryViolation,
            FailureClass::ReplayDivergence,
            FailureClass::ReplayNonDeterminism,
            FailureClass::CacheBehaviorDivergence,
            FailureClass::SilentCacheBehavior,
            FailureClass::SimdScalarMismatch,
            FailureClass::ArchitectureDivergence,
            FailureClass::SIMDNondeterminism,
            FailureClass::LatencyRegression,
            FailureClass::ThroughputRegression,
            FailureClass::MemoryRegression,
            FailureClass::CacheHitRateRegression,
            FailureClass::ReplayTimeout,
            FailureClass::ReplayAbort,
            FailureClass::WorkloadPackMismatch,
            FailureClass::MachineNondeterminism,
            FailureClass::OSNondeterminism,
            FailureClass::LibcNondeterminism,
            FailureClass::KernelContractViolation,
            FailureClass::FFIMemorySafety,
            FailureClass::CacheTierMismatch,
        ];

        // 23 failure classes total (10 major + 13 sub)
        assert_eq!(all_classes.len(), 23);

        for class in all_classes {
            let receipt = VerificationReceipt::new(
                class.clone(),
                "test".to_string(),
                "test".to_string(),
            );

            // Serialize to CBOR
            let mut cbor_bytes = Vec::new();
            ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();

            // Deserialize back
            let deserialized: VerificationReceipt =
                ciborium::from_reader(&cbor_bytes[..]).unwrap();

            assert_eq!(deserialized.failure_class, class);
        }
    }
}

//! EPIC 11 Subsystem 2: Receipt Serialization Tests
//!
//! Comprehensive tests for CBOR receipt round-trip serialization,
//! all failure classes, and file I/O operations.

use qlever_artifact_capture::{
    emit_receipt_to_path, emit_success_receipt_to_path, read_receipt,
    FailureClass, MachineFingerprint, SuccessReceipt, VerificationReceipt,
};
use std::fs;
use tempfile::TempDir;

#[test]
fn test_receipt_roundtrip_serialization() {
    let receipt = VerificationReceipt::new(
        FailureClass::EpochContamination,
        "qlever-verify replay --epoch-A 1 --epoch-B 2 --query q0001".to_string(),
        "Investigate cross-epoch cache hit".to_string(),
    )
    .with_digest_evidence("abc123def456".to_string());

    // Serialize to CBOR
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).expect("Failed to serialize");

    // Deserialize from CBOR
    let deserialized: VerificationReceipt =
        ciborium::from_reader(&cbor_bytes[..]).expect("Failed to deserialize");

    // Verify all fields match
    assert_eq!(deserialized.receipt_version, receipt.receipt_version);
    assert_eq!(deserialized.machine_fingerprint.os_name, receipt.machine_fingerprint.os_name);
    assert_eq!(deserialized.failure_class, receipt.failure_class);
    assert_eq!(deserialized.is_blocking, receipt.is_blocking);
    assert_eq!(deserialized.reproduction_command, receipt.reproduction_command);
    assert_eq!(deserialized.digest_evidence, receipt.digest_evidence);
    assert_eq!(deserialized.tags, receipt.tags);
    assert_eq!(deserialized.recommended_action, receipt.recommended_action);
}

#[test]
fn test_all_failure_classes_roundtrip() {
    let failure_classes = vec![
        // Epoch isolation failures
        FailureClass::EpochContamination,
        FailureClass::EpochKeyMismatch,
        FailureClass::PromotionBoundaryViolation,
        // Determinism failures
        FailureClass::ReplayDivergence,
        FailureClass::ReplayNonDeterminism,
        FailureClass::CacheBehaviorDivergence,
        FailureClass::SilentCacheBehavior,
        // SIMD equivalence failures
        FailureClass::SimdScalarMismatch,
        FailureClass::ArchitectureDivergence,
        FailureClass::SIMDNondeterminism,
        // Performance failures
        FailureClass::LatencyRegression,
        FailureClass::ThroughputRegression,
        FailureClass::MemoryRegression,
        FailureClass::CacheHitRateRegression,
        // Replay failures
        FailureClass::ReplayTimeout,
        FailureClass::ReplayAbort,
        FailureClass::WorkloadPackMismatch,
        // Cross-machine failures
        FailureClass::MachineNondeterminism,
        FailureClass::OSNondeterminism,
        FailureClass::LibcNondeterminism,
        // Contract failures
        FailureClass::KernelContractViolation,
        FailureClass::FFIMemorySafety,
        FailureClass::CacheTierMismatch,
    ];

    assert_eq!(failure_classes.len(), 23, "All 23 failure classes must be tested");

    for class in failure_classes {
        let receipt = VerificationReceipt::new(
            class.clone(),
            format!("reproduce-{:?}", class),
            format!("action-for-{:?}", class),
        );

        // Serialize
        let mut cbor_bytes = Vec::new();
        ciborium::into_writer(&receipt, &mut cbor_bytes)
            .expect("Failed to serialize failure class");

        // Deserialize
        let deserialized: VerificationReceipt =
            ciborium::from_reader(&cbor_bytes[..]).expect("Failed to deserialize failure class");

        // Verify failure class survived round-trip
        assert_eq!(
            deserialized.failure_class, class,
            "Failure class {:?} did not survive round-trip",
            class
        );
    }
}

#[test]
fn test_receipt_with_evidence() {
    let evidence_data = b"This is test evidence";

    let receipt = VerificationReceipt::new(
        FailureClass::ReplayDivergence,
        "test-replay".to_string(),
        "Investigate divergence".to_string(),
    )
    .with_evidence("expected_digest", b"abc123".to_vec())
    .with_evidence("actual_digest", b"xyz789".to_vec())
    .with_evidence("divergence_bytes", evidence_data.to_vec());

    // Serialize and deserialize
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();
    let deserialized: VerificationReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    // Verify evidence
    assert_eq!(deserialized.evidence.len(), 3);
    assert_eq!(
        deserialized.evidence.get("expected_digest"),
        Some(&b"abc123".to_vec())
    );
    assert_eq!(
        deserialized.evidence.get("actual_digest"),
        Some(&b"xyz789".to_vec())
    );
    assert_eq!(
        deserialized.evidence.get("divergence_bytes"),
        Some(&evidence_data.to_vec())
    );
}

#[test]
fn test_receipt_file_emission() {
    let temp_dir = TempDir::new().expect("Failed to create temp dir");
    let temp_path = temp_dir.path().to_str().unwrap();

    let receipt = VerificationReceipt::new(
        FailureClass::LatencyRegression,
        "benchmark-regression".to_string(),
        "Review recent commits for performance impact".to_string(),
    );

    let filepath = emit_receipt_to_path(&receipt, temp_path).expect("Failed to emit receipt");

    // Verify file was created
    assert!(filepath.exists(), "Receipt file was not created");
    assert!(filepath.file_name().unwrap().to_str().unwrap().ends_with(".receipt.cbor"));

    // Read back and verify
    let read_receipt = read_receipt(&filepath).expect("Failed to read receipt");
    assert_eq!(read_receipt.failure_class, FailureClass::LatencyRegression);
    assert_eq!(read_receipt.reproduction_command, "benchmark-regression");
}

#[test]
fn test_blocking_status_persistence() {
    let blocking_classes = vec![
        FailureClass::EpochContamination,
        FailureClass::ReplayDivergence,
        FailureClass::SimdScalarMismatch,
        FailureClass::LatencyRegression,
        FailureClass::KernelContractViolation,
    ];

    let non_blocking_classes = vec![
        FailureClass::MemoryRegression,
        FailureClass::MachineNondeterminism,
        FailureClass::OSNondeterminism,
    ];

    for class in blocking_classes {
        let receipt = VerificationReceipt::new(
            class.clone(),
            "test".to_string(),
            "test".to_string(),
        );

        let mut cbor_bytes = Vec::new();
        ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();
        let deserialized: VerificationReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

        assert!(
            deserialized.is_blocking,
            "Failure class {:?} should be blocking",
            class
        );
    }

    for class in non_blocking_classes {
        let receipt = VerificationReceipt::new(
            class.clone(),
            "test".to_string(),
            "test".to_string(),
        );

        let mut cbor_bytes = Vec::new();
        ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();
        let deserialized: VerificationReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

        assert!(
            !deserialized.is_blocking,
            "Failure class {:?} should not be blocking",
            class
        );
    }
}

#[test]
fn test_tags_persistence() {
    let receipt = VerificationReceipt::new(
        FailureClass::EpochContamination,
        "test".to_string(),
        "test".to_string(),
    );

    // Verify tags contain expected values
    assert!(receipt.tags.contains(&"epoch".to_string()));
    assert!(receipt.tags.contains(&"isolation".to_string()));

    // Serialize and deserialize
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();
    let deserialized: VerificationReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    // Verify tags survived round-trip
    assert_eq!(deserialized.tags, receipt.tags);
}

#[test]
fn test_machine_fingerprint_serialization() {
    let fingerprint = MachineFingerprint {
        cpu_model: "Intel Core i7".to_string(),
        cpu_features: vec!["avx512f".to_string(), "sse4.2".to_string()],
        os_name: "linux".to_string(),
        os_version: "5.15.0".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    let receipt = VerificationReceipt::new(
        FailureClass::ArchitectureDivergence,
        "test".to_string(),
        "test".to_string(),
    )
    .with_machine_fingerprint(fingerprint.clone());

    // Serialize and deserialize
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();
    let deserialized: VerificationReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    // Verify fingerprint survived round-trip
    assert_eq!(deserialized.machine_fingerprint.cpu_model, fingerprint.cpu_model);
    assert_eq!(deserialized.machine_fingerprint.cpu_features, fingerprint.cpu_features);
    assert_eq!(deserialized.machine_fingerprint.os_name, fingerprint.os_name);
    assert_eq!(deserialized.machine_fingerprint.os_version, fingerprint.os_version);
    assert_eq!(deserialized.machine_fingerprint.libc_version, fingerprint.libc_version);
    assert_eq!(deserialized.machine_fingerprint.architecture, fingerprint.architecture);
}

#[test]
fn test_success_receipt_serialization() {
    let success_receipt = SuccessReceipt::new(
        "contract",
        42,
        5000,
        "abc123def456abc123def456".to_string(),
    );

    // Serialize
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&success_receipt, &mut cbor_bytes).unwrap();

    // Deserialize
    let deserialized: SuccessReceipt = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    // Verify all fields
    assert_eq!(deserialized.receipt_version, success_receipt.receipt_version);
    assert_eq!(deserialized.test_category, "contract");
    assert_eq!(deserialized.tests_passed, 42);
    assert_eq!(deserialized.total_duration_ms, 5000);
    assert_eq!(deserialized.digest, "abc123def456abc123def456");
}

#[test]
fn test_success_receipt_file_emission() {
    let temp_dir = TempDir::new().expect("Failed to create temp dir");
    let temp_path = temp_dir.path().to_str().unwrap();

    let success_receipt = SuccessReceipt::new(
        "extended",
        150,
        30000,
        "xyz789abc123xyz789abc123".to_string(),
    );

    let filepath = emit_success_receipt_to_path(&success_receipt, temp_path)
        .expect("Failed to emit success receipt");

    // Verify file was created
    assert!(filepath.exists(), "Success receipt file was not created");
    assert!(filepath.file_name().unwrap().to_str().unwrap().contains("-success.receipt.cbor"));

    // Read back and verify
    let cbor_bytes = fs::read(&filepath).expect("Failed to read file");
    let read_receipt: SuccessReceipt = ciborium::from_reader(&cbor_bytes[..])
        .expect("Failed to deserialize success receipt");

    assert_eq!(read_receipt.test_category, "extended");
    assert_eq!(read_receipt.tests_passed, 150);
}

#[test]
fn test_receipt_directory_creation() {
    let temp_dir = TempDir::new().expect("Failed to create temp dir");
    let nested_path = temp_dir.path().join("nested/path/to/receipts");
    let nested_str = nested_path.to_str().unwrap();

    // Ensure directory doesn't exist yet
    assert!(!nested_path.exists());

    let receipt = VerificationReceipt::new(
        FailureClass::ReplayAbort,
        "test".to_string(),
        "test".to_string(),
    );

    let filepath = emit_receipt_to_path(&receipt, nested_str).expect("Failed to emit receipt");

    // Verify directory was created
    assert!(filepath.parent().unwrap().exists(), "Receipt directory was not created");
    assert!(filepath.exists(), "Receipt file was not created");
}

#[test]
fn test_cbor_schema_validation() {
    use qlever_artifact_capture::receipt_format::validate_receipt_schema;

    let receipt = VerificationReceipt::new(
        FailureClass::CacheTierMismatch,
        "test".to_string(),
        "test".to_string(),
    );

    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).unwrap();

    // Valid CBOR should pass validation
    assert!(validate_receipt_schema(&cbor_bytes).is_ok());

    // Invalid CBOR should fail validation
    let invalid_cbor = vec![0xFF, 0xFF, 0xFF]; // Invalid CBOR
    assert!(validate_receipt_schema(&invalid_cbor).is_err());
}

#[test]
fn test_receipt_filename_generation() {
    let receipt = VerificationReceipt::new(
        FailureClass::WorkloadPackMismatch,
        "test".to_string(),
        "test".to_string(),
    );

    let filename = receipt.filename();

    // Verify filename format
    assert!(filename.ends_with(".receipt.cbor"), "Filename should end with .receipt.cbor");
    assert!(filename.contains("WorkloadPackMismatch"), "Filename should contain failure class");
}

#[test]
fn test_large_evidence_serialization() {
    // Create large evidence payload (1MB)
    let large_evidence = vec![0xAB; 1024 * 1024];

    let receipt = VerificationReceipt::new(
        FailureClass::ReplayDivergence,
        "test".to_string(),
        "test".to_string(),
    )
    .with_evidence("large_payload", large_evidence.clone());

    // Serialize (should handle large payloads)
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&receipt, &mut cbor_bytes).expect("Failed to serialize large evidence");

    // Deserialize and verify
    let deserialized: VerificationReceipt =
        ciborium::from_reader(&cbor_bytes[..]).expect("Failed to deserialize large evidence");

    assert_eq!(
        deserialized.evidence.get("large_payload"),
        Some(&large_evidence)
    );
}

#[test]
fn test_receipt_emission_creates_receipt_storage_path() {
    let temp_dir = TempDir::new().expect("Failed to create temp dir");
    let temp_path = temp_dir.path().to_str().unwrap();

    let receipt = VerificationReceipt::new(
        FailureClass::EpochKeyMismatch,
        "test".to_string(),
        "test".to_string(),
    );

    // Directory should be created by emit_receipt_to_path
    let filepath = emit_receipt_to_path(&receipt, temp_path).expect("Failed to emit receipt");

    // Verify the directory exists and file was written
    assert!(filepath.parent().unwrap().exists());
    assert!(filepath.exists());

    // Verify we can read it back
    let read_back = read_receipt(&filepath).expect("Failed to read receipt");
    assert_eq!(read_back.failure_class, FailureClass::EpochKeyMismatch);
}

#[test]
fn test_multiple_receipts_in_same_directory() {
    let temp_dir = TempDir::new().expect("Failed to create temp dir");
    let temp_path = temp_dir.path().to_str().unwrap();

    // Emit multiple receipts
    let classes = vec![
        FailureClass::EpochContamination,
        FailureClass::ReplayDivergence,
        FailureClass::SimdScalarMismatch,
    ];

    let mut filepaths = Vec::new();
    for class in &classes {
        let receipt = VerificationReceipt::new(
            class.clone(),
            "test".to_string(),
            "test".to_string(),
        );

        let filepath = emit_receipt_to_path(&receipt, temp_path).expect("Failed to emit receipt");
        filepaths.push(filepath);

        // Small delay to ensure different timestamps
        std::thread::sleep(std::time::Duration::from_millis(10));
    }

    // Verify all files were created
    assert_eq!(filepaths.len(), 3);
    for filepath in &filepaths {
        assert!(filepath.exists(), "Receipt file was not created");
    }

    // Verify we can read all of them back
    for (i, filepath) in filepaths.iter().enumerate() {
        let read_back = read_receipt(filepath).expect("Failed to read receipt");
        assert_eq!(read_back.failure_class, classes[i]);
    }
}

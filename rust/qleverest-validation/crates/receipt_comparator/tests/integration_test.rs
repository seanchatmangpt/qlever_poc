//! Integration test: Demonstrate successful comparison of identical receipt bundles

use qlever_artifact_capture::{
    emit_receipt_to_path, emit_success_receipt_to_path, FailureClass, SuccessReceipt,
    VerificationReceipt,
};
use std::fs;
use std::process::Command;
use tempfile::TempDir;

#[test]
fn test_comparator_on_identical_bundles() {
    // Create two temporary directories to simulate x86_64 and aarch64 bundles
    let bundle_x86 = TempDir::new().unwrap();
    let bundle_arm = TempDir::new().unwrap();

    // Create identical receipts in both bundles
    let verification_receipt = VerificationReceipt::new(
        FailureClass::ReplayDivergence,
        "qlever-verify replay --workload test.json".to_string(),
        "Investigate cache behavior".to_string(),
    )
    .with_digest_evidence("sha256:abc123def456".to_string());

    let success_receipt = SuccessReceipt::new(
        "integration_test",
        42,
        5000,
        "sha256:success_digest_789".to_string(),
    );

    // Emit receipts to both bundles
    emit_receipt_to_path(&verification_receipt, bundle_x86.path().to_str().unwrap()).unwrap();
    emit_receipt_to_path(&verification_receipt, bundle_arm.path().to_str().unwrap()).unwrap();

    emit_success_receipt_to_path(&success_receipt, bundle_x86.path().to_str().unwrap()).unwrap();
    emit_success_receipt_to_path(&success_receipt, bundle_arm.path().to_str().unwrap()).unwrap();

    // Create identical verdict.json files
    let verdict = r#"{
  "verdict": "FAIL",
  "receipt_id": "test-receipt-001",
  "artifacts": [
    "verification_receipt.cbor",
    "success_receipt.cbor"
  ]
}"#;

    fs::write(bundle_x86.path().join("verdict.json"), verdict).unwrap();
    fs::write(bundle_arm.path().join("verdict.json"), verdict).unwrap();

    // Run the comparator
    let output_report = bundle_x86.path().join("comparison_report.md");

    let output = Command::new(env!("CARGO_BIN_EXE_receipt_comparator"))
        .arg("--bundle-a")
        .arg(bundle_x86.path())
        .arg("--bundle-b")
        .arg(bundle_arm.path())
        .arg("--output")
        .arg(&output_report)
        .arg("--verbose")
        .output()
        .expect("Failed to run receipt_comparator");

    // Verify exit code is 0 (match)
    assert_eq!(
        output.status.code(),
        Some(0),
        "Comparator should exit with 0 for identical bundles. stdout: {}\nstderr: {}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );

    // Verify report was created
    assert!(output_report.exists(), "Report should be created");

    // Verify report content
    let report_content = fs::read_to_string(&output_report).unwrap();
    assert!(
        report_content.contains("DETERMINISTIC"),
        "Report should indicate determinism"
    );
    assert!(
        report_content.contains("✅"),
        "Report should show success markers"
    );
}

#[test]
fn test_comparator_detects_differences() {
    // Create two bundles with different receipts
    let bundle_x86 = TempDir::new().unwrap();
    let bundle_arm = TempDir::new().unwrap();

    // Create receipts with different digest evidence
    let receipt_x86 = VerificationReceipt::new(
        FailureClass::ReplayDivergence,
        "test".to_string(),
        "test".to_string(),
    )
    .with_digest_evidence("x86_digest_123".to_string());

    let receipt_arm = VerificationReceipt::new(
        FailureClass::ReplayDivergence,
        "test".to_string(),
        "test".to_string(),
    )
    .with_digest_evidence("arm_digest_789".to_string());

    emit_receipt_to_path(&receipt_x86, bundle_x86.path().to_str().unwrap()).unwrap();
    emit_receipt_to_path(&receipt_arm, bundle_arm.path().to_str().unwrap()).unwrap();

    // Create matching verdicts (but receipts differ)
    let verdict = r#"{"verdict": "FAIL", "receipt_id": "test", "artifacts": []}"#;
    fs::write(bundle_x86.path().join("verdict.json"), verdict).unwrap();
    fs::write(bundle_arm.path().join("verdict.json"), verdict).unwrap();

    // Run comparator
    let output_report = bundle_x86.path().join("comparison_report.md");

    let output = Command::new(env!("CARGO_BIN_EXE_receipt_comparator"))
        .arg("--bundle-a")
        .arg(bundle_x86.path())
        .arg("--bundle-b")
        .arg(bundle_arm.path())
        .arg("--output")
        .arg(&output_report)
        .output()
        .expect("Failed to run receipt_comparator");

    // Debug: print stdout/stderr
    eprintln!("stdout: {}", String::from_utf8_lossy(&output.stdout));
    eprintln!("stderr: {}", String::from_utf8_lossy(&output.stderr));

    // Should exit with 1 (non-deterministic)
    assert_eq!(
        output.status.code(),
        Some(1),
        "Comparator should exit with 1 for different receipts. stdout: {}\nstderr: {}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );

    // Verify report shows differences
    let report_content = fs::read_to_string(&output_report).unwrap();
    assert!(
        report_content.contains("NON-DETERMINISTIC"),
        "Report should indicate non-determinism"
    );
    assert!(
        report_content.contains("Digest mismatch"),
        "Report should list digest differences"
    );
}

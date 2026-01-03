//! Integration tests for qlever-gate binary

use std::process::Command;
use tempfile::TempDir;

#[test]
fn test_gate_help_message() {
    let output = Command::new("cargo")
        .args(["run", "--bin", "qlever-gate", "--", "--help"])
        .current_dir(env!("CARGO_MANIFEST_DIR"))
        .output()
        .expect("Failed to execute qlever-gate");

    assert!(output.status.success());
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(stdout.contains("qlever-gate"));
    assert!(stdout.contains("--workload"));
    assert!(stdout.contains("--env"));
    assert!(stdout.contains("--output"));
}

#[test]
fn test_gate_version() {
    let output = Command::new("cargo")
        .args(["run", "--bin", "qlever-gate", "--", "--version"])
        .current_dir(env!("CARGO_MANIFEST_DIR"))
        .output()
        .expect("Failed to execute qlever-gate");

    assert!(output.status.success());
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(stdout.contains("1.0.0"));
}

#[test]
fn test_gate_missing_arguments() {
    let output = Command::new("cargo")
        .args(["run", "--bin", "qlever-gate"])
        .current_dir(env!("CARGO_MANIFEST_DIR"))
        .output()
        .expect("Failed to execute qlever-gate");

    assert!(!output.status.success());
    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(stderr.contains("required") || stderr.contains("missing"));
}

#[test]
fn test_gate_dry_run() {
    let temp_dir = TempDir::new().unwrap();
    let workload_path = temp_dir.path().join("test-workload.json");
    let env_path = temp_dir.path().join("test-env.json");
    let output_path = temp_dir.path().join("output");

    // Create minimal test workload
    std::fs::write(
        &workload_path,
        r#"{
  "workload_id": "test",
  "query_pack": [{
    "query_id": "q1",
    "query_text": "SELECT 1",
    "execution_order": 0,
    "expected_result_digest": "0000000000000000000000000000000000000000000000000000000000000000",
    "expected_cache_behavior": [],
    "expected_latency_ms": 0
  }],
  "expected_state": {
    "cache_size_bytes": 0,
    "cache_entries": 0,
    "hit_rate_pct": 0,
    "epoch_key": []
  },
  "replay_mode": "Strict"
}"#,
    )
    .unwrap();

    // Create minimal environment config
    std::fs::write(
        &env_path,
        r#"{
  "git_commit": "test",
  "architecture": "x86_64"
}"#,
    )
    .unwrap();

    let output = Command::new("cargo")
        .args([
            "run",
            "--bin",
            "qlever-gate",
            "--",
            "--workload",
            workload_path.to_str().unwrap(),
            "--env",
            env_path.to_str().unwrap(),
            "--output",
            output_path.to_str().unwrap(),
            "--dry-run",
        ])
        .current_dir(env!("CARGO_MANIFEST_DIR"))
        .output()
        .expect("Failed to execute qlever-gate");

    assert!(output.status.success());
    let stdout = String::from_utf8_lossy(&output.stdout);
    assert!(stdout.contains("DRY_RUN"));
    assert!(stdout.contains("verdict"));
}

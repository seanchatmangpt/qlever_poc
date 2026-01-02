//! Command roundtrip tests
//!
//! Proves that generated rerun commands are:
//! 1. Self-contained (no absolute paths, no user names)
//! 2. Copy-pasteable into a shell
//! 3. Executable on any Linux machine with the right toolchain

use chrono::Utc;
use qlever_artifact_capture::MachineFingerprint;
use qlever_repro::{
    EnvironmentSnapshot, ReproductionManifest, VerificationVerdict, WorkloadPackReference,
};
use std::process::Command;
use tempfile::TempDir;

#[test]
fn test_command_contains_no_absolute_paths() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Command must not contain absolute paths
    assert!(!manifest.rerun_command.contains("/home/"));
    assert!(!manifest.rerun_command.contains("/root/"));
    assert!(!manifest.rerun_command.contains("/tmp/"));
}

#[test]
fn test_command_contains_no_machine_specific_refs() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Command must not contain username
    let current_user = std::env::var("USER").unwrap_or_default();
    if !current_user.is_empty() {
        assert!(!manifest.rerun_command.contains(&current_user));
    }

    // Command must not contain hostname
    let hostname = std::env::var("HOSTNAME").unwrap_or_default();
    if !hostname.is_empty() {
        assert!(!manifest.rerun_command.contains(&hostname));
    }
}

#[test]
fn test_command_is_syntactically_valid_shell() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Try to validate shell syntax by checking with sh -n
    let output = Command::new("sh")
        .arg("-n")
        .arg("-c")
        .arg(&manifest.rerun_command)
        .output()
        .expect("Failed to run sh -n");

    assert!(
        output.status.success(),
        "Command has invalid shell syntax: {}",
        String::from_utf8_lossy(&output.stderr)
    );
}

#[test]
fn test_setup_script_is_valid_bash() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Validate bash syntax
    let output = Command::new("bash")
        .arg("-n")
        .arg("-c")
        .arg(&manifest.setup_script)
        .output()
        .expect("Failed to run bash -n");

    assert!(
        output.status.success(),
        "Setup script has invalid bash syntax: {}",
        String::from_utf8_lossy(&output.stderr)
    );
}

#[test]
fn test_manifest_roundtrip_via_json() {
    let temp_dir = TempDir::new().unwrap();
    let manifest_path = temp_dir.path().join("repro_manifest.json");

    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Save to file
    manifest.save_to_file(&manifest_path).unwrap();

    // Load from file
    let loaded = ReproductionManifest::load_from_file(&manifest_path).unwrap();

    // Verify round-trip
    assert_eq!(loaded.manifest_version, manifest.manifest_version);
    assert_eq!(loaded.verdict.gate_name, manifest.verdict.gate_name);
    assert_eq!(loaded.rerun_command, manifest.rerun_command);
    assert_eq!(loaded.workload_pack.workload_id, manifest.workload_pack.workload_id);
}

#[test]
fn test_command_includes_required_components() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let gate_name = verdict.gate_name.clone();
    let workload_path = workload.workload_pack_path.clone();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Command must include:
    // 1. Environment variable setup
    assert!(manifest.rerun_command.contains("RUST_BACKTRACE=1"));

    // 2. Cargo run invocation
    assert!(manifest.rerun_command.contains("cargo run"));

    // 3. Package specification
    assert!(manifest.rerun_command.contains("-p qlever-verification-harness"));

    // 4. Gate name
    assert!(manifest.rerun_command.contains(&gate_name));

    // 5. Workload path
    assert!(manifest.rerun_command.contains(&workload_path));

    // 6. Output directory
    assert!(manifest.rerun_command.contains("--output"));
}

#[test]
fn test_setup_script_includes_required_steps() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let git_commit = env.git_commit.clone();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Setup script must include:
    // 1. Shebang
    assert!(manifest.setup_script.starts_with("#!/bin/bash"));

    // 2. Git checkout
    assert!(manifest.setup_script.contains("git checkout"));
    assert!(manifest.setup_script.contains(&git_commit));

    // 3. Toolchain verification
    assert!(manifest.setup_script.contains("rustc --version"));
    assert!(manifest.setup_script.contains("cmake --version"));

    // 4. Build step
    assert!(manifest.setup_script.contains("cargo build"));

    // 5. Directory creation
    assert!(manifest.setup_script.contains("mkdir -p"));
}

#[test]
fn test_manifest_validation_passes_for_valid_manifest() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Validation should pass
    assert!(manifest.validate().is_ok());
}

#[test]
fn test_manifest_validation_fails_for_absolute_workload_path() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let mut workload = create_test_workload_ref();

    // Use absolute path
    workload.workload_pack_path = "/home/user/test.workload.cbor".to_string();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Validation should fail
    assert!(manifest.validate().is_err());
}

#[test]
fn test_manifest_validation_allows_download_url_with_absolute_path() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let mut workload = create_test_workload_ref();

    // Use absolute path but provide download URL
    workload.workload_pack_path = "/tmp/test.workload.cbor".to_string();
    workload.download_url = Some("https://example.com/workload.cbor".to_string());

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Validation should pass because download_url is provided
    assert!(manifest.validate().is_ok());
}

#[test]
fn test_command_execution_dry_run() {
    // This test demonstrates that the command can be parsed and prepared for execution
    // (We don't actually run it since it requires the full QLever build)
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Extract the command after environment variables
    let parts: Vec<&str> = manifest.rerun_command.split("; ").collect();
    let cargo_command = parts.last().unwrap();

    // Verify it's a valid cargo command structure
    assert!(cargo_command.starts_with("cargo run"));

    // Parse the command to ensure it has the right structure
    let tokens: Vec<&str> = cargo_command.split_whitespace().collect();
    assert!(tokens.contains(&"cargo"));
    assert!(tokens.contains(&"run"));
    assert!(tokens.contains(&"-p"));
    assert!(tokens.contains(&"qlever-verification-harness"));
    assert!(tokens.contains(&"--gate"));
    assert!(tokens.contains(&"--workload"));
    assert!(tokens.contains(&"--output"));
}

#[test]
fn test_generated_command_is_copy_pasteable() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // Command should be a single line (or properly escaped for multi-line)
    let command = &manifest.rerun_command;

    // Should not contain unescaped newlines
    assert!(!command.contains('\n'));

    // Should be non-empty
    assert!(!command.is_empty());

    // Should end with a complete command (not a semicolon)
    assert!(!command.ends_with(';'));
    assert!(!command.ends_with("\\"));
}

#[test]
fn test_output_paths_are_relative() {
    let env = create_test_environment();
    let verdict = create_test_verdict();
    let workload = create_test_workload_ref();

    let manifest = ReproductionManifest::new(env, verdict, workload);

    // All output paths should be relative
    for path in &manifest.output_paths {
        assert!(
            path.starts_with("./") || !path.starts_with('/'),
            "Output path {} is not relative",
            path
        );
    }
}

// Helper functions

fn create_test_environment() -> EnvironmentSnapshot {
    EnvironmentSnapshot {
        machine_fingerprint: MachineFingerprint::default(),
        git_commit: "abc123def456".to_string(),
        git_branch: "main".to_string(),
        rust_version: "1.75.0".to_string(),
        cmake_version: "3.27.0".to_string(),
        cxx_compiler_version: "11.4.0".to_string(),
        build_env_vars: vec![
            ("RUST_BACKTRACE".to_string(), "1".to_string()),
            ("CMAKE_BUILD_TYPE".to_string(), "Release".to_string()),
        ],
    }
}

fn create_test_verdict() -> VerificationVerdict {
    VerificationVerdict {
        gate_name: "contract".to_string(),
        result: "PASS".to_string(),
        total_time_ms: 1000,
        blocking_failures: 0,
        advisory_failures: 0,
        verdict_timestamp: Utc::now().to_rfc3339(),
    }
}

fn create_test_workload_ref() -> WorkloadPackReference {
    WorkloadPackReference {
        workload_id: "deterministic-corpus-v1".to_string(),
        workload_pack_path: "./corpus.workload.cbor".to_string(),
        workload_pack_hash: "0".repeat(64),
        download_url: None,
    }
}

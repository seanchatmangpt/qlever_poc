//! EPIC 11 Integration Phase - Agent 8: Rerun Command Path Generator
//!
//! This module generates portable, reproducible rerun commands that can be
//! executed on any Linux machine to reproduce verification results.
//!
//! Takes as input:
//! - verdict.json (VerificationReport)
//! - environment.json (EnvironmentSnapshot)
//! - receipt_bundle/ (collection of receipts)
//! - workload manifest (ReplayWorkload)
//!
//! Outputs:
//! - repro_manifest.json with self-contained rerun commands

use chrono::Utc;
use qlever_artifact_capture::MachineFingerprint;
use qlever_replay_verifier::ReplayWorkload;
use serde::{Deserialize, Serialize};
use std::path::Path;
use thiserror::Error;

/// Environment snapshot for reproducibility
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EnvironmentSnapshot {
    /// Machine fingerprint (CPU, OS, libc)
    pub machine_fingerprint: MachineFingerprint,

    /// Git commit hash
    pub git_commit: String,

    /// Git branch name
    pub git_branch: String,

    /// Rust toolchain version (e.g., "1.75.0")
    pub rust_version: String,

    /// CMake version
    pub cmake_version: String,

    /// GCC/Clang version
    pub cxx_compiler_version: String,

    /// Environment variables that affect the build
    pub build_env_vars: Vec<(String, String)>,
}

impl Default for EnvironmentSnapshot {
    fn default() -> Self {
        Self {
            machine_fingerprint: MachineFingerprint::default(),
            git_commit: "unknown".to_string(),
            git_branch: "unknown".to_string(),
            rust_version: "unknown".to_string(),
            cmake_version: "unknown".to_string(),
            cxx_compiler_version: "unknown".to_string(),
            build_env_vars: vec![],
        }
    }
}

/// Verdict from a verification run (simplified from VerificationReport)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VerificationVerdict {
    /// Name of the gate (contract, regression, full)
    pub gate_name: String,

    /// Overall result (PASS, FAIL, TIMEOUT, PARTIAL)
    pub result: String,

    /// Total execution time in milliseconds
    pub total_time_ms: u64,

    /// Number of blocking failures
    pub blocking_failures: usize,

    /// Number of advisory failures
    pub advisory_failures: usize,

    /// Timestamp when verdict was generated
    pub verdict_timestamp: String,
}

/// Toolchain constraints for reproducibility
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ToolchainConstraints {
    /// Minimum Rust version required
    pub min_rust_version: String,

    /// Minimum CMake version required
    pub min_cmake_version: String,

    /// C++ compiler (gcc or clang)
    pub cxx_compiler: String,

    /// Minimum compiler version
    pub min_cxx_version: String,
}

impl Default for ToolchainConstraints {
    fn default() -> Self {
        Self {
            min_rust_version: "1.70.0".to_string(),
            min_cmake_version: "3.27".to_string(),
            cxx_compiler: "gcc".to_string(),
            min_cxx_version: "11.0".to_string(),
        }
    }
}

/// Workload pack reference (where to get the workload data)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WorkloadPackReference {
    /// Workload identifier
    pub workload_id: String,

    /// Path to workload pack file (relative or absolute)
    pub workload_pack_path: String,

    /// BLAKE3 hash of the workload pack
    pub workload_pack_hash: String,

    /// Download URL (optional)
    pub download_url: Option<String>,
}

/// Complete reproduction manifest
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReproductionManifest {
    /// Manifest format version
    pub manifest_version: u32,

    /// Timestamp when manifest was generated
    pub generated_at: String,

    /// Environment snapshot from original run
    pub environment: EnvironmentSnapshot,

    /// Verdict from original run
    pub verdict: VerificationVerdict,

    /// Toolchain constraints
    pub toolchain: ToolchainConstraints,

    /// Workload pack reference
    pub workload_pack: WorkloadPackReference,

    /// Full rerun command (self-contained, portable)
    pub rerun_command: String,

    /// Setup instructions (shell script fragment)
    pub setup_script: String,

    /// Expected output directory structure
    pub output_paths: Vec<String>,
}

/// Current manifest version
pub const MANIFEST_VERSION: u32 = 1;

impl ReproductionManifest {
    /// Create a new reproduction manifest
    pub fn new(
        environment: EnvironmentSnapshot,
        verdict: VerificationVerdict,
        workload_pack: WorkloadPackReference,
    ) -> Self {
        let toolchain = ToolchainConstraints::default();
        let rerun_command = Self::generate_rerun_command(&verdict, &workload_pack, &environment);
        let setup_script = Self::generate_setup_script(&environment, &toolchain);

        Self {
            manifest_version: MANIFEST_VERSION,
            generated_at: Utc::now().to_rfc3339(),
            environment,
            verdict,
            toolchain,
            workload_pack,
            rerun_command,
            setup_script,
            output_paths: vec![
                "./verification_results/".to_string(),
                "./verification_results/receipts/".to_string(),
                "./verification_results/reports/".to_string(),
            ],
        }
    }

    /// Generate the portable rerun command
    fn generate_rerun_command(
        verdict: &VerificationVerdict,
        workload: &WorkloadPackReference,
        env: &EnvironmentSnapshot,
    ) -> String {
        let mut cmd = String::new();

        // Environment variables
        cmd.push_str("export RUST_BACKTRACE=1; ");

        for (key, value) in &env.build_env_vars {
            if is_safe_env_var(key) {
                cmd.push_str(&format!("export {}='{}'; ", key, value));
            }
        }

        // The actual verification command
        cmd.push_str(&format!(
            "cargo run -p qlever-verification-harness -- \
             --gate {} \
             --workload {} \
             --output ./verification_results",
            verdict.gate_name,
            workload.workload_pack_path
        ));

        cmd
    }

    /// Generate setup script for environment preparation
    fn generate_setup_script(env: &EnvironmentSnapshot, toolchain: &ToolchainConstraints) -> String {
        format!(
            r#"#!/bin/bash
# Reproduction environment setup
# Generated from commit {} on branch {}

set -euo pipefail

echo "Setting up QLever verification environment..."

# 1. Check out the correct commit
git fetch origin
git checkout {}

# 2. Verify toolchain versions
echo "Checking toolchain versions..."
rustc --version | grep -q "{}" || echo "Warning: Rust version may differ"
cmake --version | grep -q "{}" || echo "Warning: CMake version may differ"

# 3. Build qlever-verification-harness
echo "Building verification harness..."
cd qlever-verification
cargo build --release -p qlever-verification-harness

# 4. Create output directories
mkdir -p verification_results/receipts verification_results/reports

echo "Setup complete. You can now run the rerun command."
"#,
            env.git_commit,
            env.git_branch,
            env.git_commit,
            toolchain.min_rust_version,
            toolchain.min_cmake_version,
        )
    }

    /// Save manifest to JSON file
    pub fn save_to_file(&self, path: &Path) -> Result<(), ReproError> {
        // Ensure parent directory exists
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }

        let json = serde_json::to_string_pretty(self)?;
        std::fs::write(path, json)?;

        Ok(())
    }

    /// Load manifest from JSON file
    pub fn load_from_file(path: &Path) -> Result<Self, ReproError> {
        let json = std::fs::read_to_string(path)?;
        let manifest: Self = serde_json::from_str(&json)?;

        // Validate version
        if manifest.manifest_version != MANIFEST_VERSION {
            return Err(ReproError::UnsupportedVersion {
                found: manifest.manifest_version,
                expected: MANIFEST_VERSION,
            });
        }

        Ok(manifest)
    }

    /// Validate that the manifest is self-contained
    pub fn validate(&self) -> Result<(), ReproError> {
        // Check that workload path is not absolute (except for downloads)
        if self.workload_pack.workload_pack_path.starts_with('/')
            && self.workload_pack.download_url.is_none() {
            return Err(ReproError::ValidationError(
                "Workload pack path must be relative unless download_url is provided".to_string()
            ));
        }

        // Check that rerun command doesn't contain absolute paths
        if self.rerun_command.contains("/home/") || self.rerun_command.contains("/root/") {
            return Err(ReproError::ValidationError(
                "Rerun command contains absolute user paths".to_string()
            ));
        }

        // Check that environment variables are safe
        for (key, _) in &self.environment.build_env_vars {
            if !is_safe_env_var(key) {
                return Err(ReproError::ValidationError(
                    format!("Unsafe environment variable: {}", key)
                ));
            }
        }

        Ok(())
    }
}

/// Check if an environment variable is safe to include
fn is_safe_env_var(key: &str) -> bool {
    const SAFE_ENV_VARS: &[&str] = &[
        "RUST_BACKTRACE",
        "RUST_LOG",
        "CMAKE_BUILD_TYPE",
        "CC",
        "CXX",
        "CFLAGS",
        "CXXFLAGS",
        "LDFLAGS",
    ];

    SAFE_ENV_VARS.contains(&key)
}

/// Generate a reproduction manifest from verification artifacts
pub fn generate_repro_manifest(
    verdict_path: &Path,
    environment_path: &Path,
    workload_path: &Path,
    output_path: &Path,
) -> Result<ReproductionManifest, ReproError> {
    // Load verdict
    let verdict_json = std::fs::read_to_string(verdict_path)?;
    let verdict: VerificationVerdict = serde_json::from_str(&verdict_json)?;

    // Load environment
    let env_json = std::fs::read_to_string(environment_path)?;
    let environment: EnvironmentSnapshot = serde_json::from_str(&env_json)?;

    // Compute workload hash
    let workload_bytes = std::fs::read(workload_path)?;
    let workload_hash = blake3::hash(&workload_bytes);

    // Load workload to get ID
    let workload: ReplayWorkload = ciborium::from_reader(&workload_bytes[..])
        .map_err(|e| ReproError::WorkloadLoadError(e.to_string()))?;

    let workload_pack = WorkloadPackReference {
        workload_id: workload.workload_id.clone(),
        workload_pack_path: format!("./{}.workload.cbor", workload.workload_id),
        workload_pack_hash: workload_hash.to_hex().to_string(),
        download_url: None,
    };

    // Generate manifest
    let manifest = ReproductionManifest::new(environment, verdict, workload_pack);

    // Validate before saving
    manifest.validate()?;

    // Save to output
    manifest.save_to_file(output_path)?;

    Ok(manifest)
}

/// Errors that can occur during repro manifest generation
#[derive(Debug, Error)]
pub enum ReproError {
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("JSON serialization error: {0}")]
    JsonError(#[from] serde_json::Error),

    #[error("Workload load error: {0}")]
    WorkloadLoadError(String),

    #[error("Unsupported manifest version: found {found}, expected {expected}")]
    UnsupportedVersion { found: u32, expected: u32 },

    #[error("Validation error: {0}")]
    ValidationError(String),
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_environment_snapshot_default() {
        let env = EnvironmentSnapshot::default();
        assert_eq!(env.git_commit, "unknown");
        assert_eq!(env.rust_version, "unknown");
    }

    #[test]
    fn test_toolchain_constraints_default() {
        let toolchain = ToolchainConstraints::default();
        assert!(toolchain.min_rust_version.starts_with("1."));
        assert!(toolchain.min_cmake_version.starts_with("3."));
    }

    #[test]
    fn test_safe_env_var() {
        assert!(is_safe_env_var("RUST_BACKTRACE"));
        assert!(is_safe_env_var("CMAKE_BUILD_TYPE"));
        assert!(!is_safe_env_var("HOME"));
        assert!(!is_safe_env_var("USER"));
        assert!(!is_safe_env_var("PASSWORD"));
    }

    #[test]
    fn test_repro_manifest_creation() {
        let env = EnvironmentSnapshot::default();
        let verdict = VerificationVerdict {
            gate_name: "contract".to_string(),
            result: "PASS".to_string(),
            total_time_ms: 1000,
            blocking_failures: 0,
            advisory_failures: 0,
            verdict_timestamp: Utc::now().to_rfc3339(),
        };
        let workload = WorkloadPackReference {
            workload_id: "test-workload".to_string(),
            workload_pack_path: "./test.workload.cbor".to_string(),
            workload_pack_hash: "0".repeat(64),
            download_url: None,
        };

        let manifest = ReproductionManifest::new(env, verdict, workload);

        assert_eq!(manifest.manifest_version, MANIFEST_VERSION);
        assert!(manifest.rerun_command.contains("cargo run"));
        assert!(manifest.setup_script.contains("#!/bin/bash"));
        assert!(!manifest.output_paths.is_empty());
    }

    #[test]
    fn test_manifest_validation_reject_absolute_paths() {
        let env = EnvironmentSnapshot::default();
        let verdict = VerificationVerdict {
            gate_name: "contract".to_string(),
            result: "PASS".to_string(),
            total_time_ms: 1000,
            blocking_failures: 0,
            advisory_failures: 0,
            verdict_timestamp: Utc::now().to_rfc3339(),
        };
        let workload = WorkloadPackReference {
            workload_id: "test-workload".to_string(),
            workload_pack_path: "/home/user/test.workload.cbor".to_string(), // Absolute path!
            workload_pack_hash: "0".repeat(64),
            download_url: None,
        };

        let manifest = ReproductionManifest::new(env, verdict, workload);

        // Validation should fail due to absolute path
        assert!(manifest.validate().is_err());
    }

    #[test]
    fn test_manifest_validation_reject_user_paths_in_command() {
        let mut env = EnvironmentSnapshot::default();
        env.git_commit = "abc123".to_string();

        let verdict = VerificationVerdict {
            gate_name: "contract".to_string(),
            result: "PASS".to_string(),
            total_time_ms: 1000,
            blocking_failures: 0,
            advisory_failures: 0,
            verdict_timestamp: Utc::now().to_rfc3339(),
        };
        let workload = WorkloadPackReference {
            workload_id: "test-workload".to_string(),
            workload_pack_path: "./test.workload.cbor".to_string(),
            workload_pack_hash: "0".repeat(64),
            download_url: None,
        };

        let mut manifest = ReproductionManifest::new(env, verdict, workload);

        // Inject an absolute user path
        manifest.rerun_command = "/home/someuser/run_test.sh".to_string();

        // Validation should fail
        assert!(manifest.validate().is_err());
    }

    #[test]
    fn test_manifest_serialization_roundtrip() {
        let env = EnvironmentSnapshot::default();
        let verdict = VerificationVerdict {
            gate_name: "contract".to_string(),
            result: "PASS".to_string(),
            total_time_ms: 1000,
            blocking_failures: 0,
            advisory_failures: 0,
            verdict_timestamp: Utc::now().to_rfc3339(),
        };
        let workload = WorkloadPackReference {
            workload_id: "test-workload".to_string(),
            workload_pack_path: "./test.workload.cbor".to_string(),
            workload_pack_hash: "0".repeat(64),
            download_url: None,
        };

        let manifest = ReproductionManifest::new(env, verdict, workload);

        // Serialize to JSON
        let json = serde_json::to_string_pretty(&manifest).unwrap();

        // Deserialize back
        let deserialized: ReproductionManifest = serde_json::from_str(&json).unwrap();

        assert_eq!(deserialized.manifest_version, manifest.manifest_version);
        assert_eq!(deserialized.verdict.gate_name, manifest.verdict.gate_name);
        assert_eq!(deserialized.workload_pack.workload_id, manifest.workload_pack.workload_id);
    }
}

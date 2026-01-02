//! # qlever-witness: Fail-Closed Witness Minimization Strategy
//!
//! EPIC 11 Agent 5: When cross-machine receipts diverge, create a minimal reproducible witness.
//!
//! ## Invariants
//! - Witness bundles are self-contained (no external state)
//! - Minimization is deterministic (same input → same minimal witness)
//! - Witness bundles can be executed standalone
//! - No mutable external state permitted
//!
//! ## Core Concepts
//! - **Witness Bundle**: Minimal reproducible case showing divergence
//! - **Minimization**: Binary search to isolate minimum set exhibiting divergence
//! - **Divergence Point**: Exact phase/subsystem where failure occurred
//! - **Rerun Command**: Exact command to re-trigger failure on both machines

pub mod minimization;

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use thiserror::Error;

/// Errors that can occur during witness bundle operations
#[derive(Error, Debug)]
pub enum WitnessError {
    #[error("Serialization failed: {0}")]
    SerializationError(#[from] serde_json::Error),

    #[error("Invalid witness bundle: {0}")]
    InvalidBundle(String),

    #[error("Minimization failed: {0}")]
    MinimizationError(String),

    #[error("Divergence point not found")]
    DivergenceNotFound,
}

pub type Result<T> = std::result::Result<T, WitnessError>;

/// Identifies which phase/subsystem failed
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "snake_case")]
pub enum DivergencePoint {
    /// Kernel runner FFI layer diverged
    KernelRunner,

    /// Artifact capture (receipt generation) diverged
    ArtifactCapture,

    /// Digest verification failed
    DigestVerification,

    /// Cache behavior diverged
    CacheVerification,

    /// Replay diverged
    ReplayVerification,

    /// Regression detected
    RegressionVerification,

    /// Epoch isolation diverged
    EpochVerification,

    /// SIMD behavior diverged
    SimdVerification,

    /// Chaos verification diverged
    ChaosVerification,

    /// Unknown/custom divergence point
    Custom(String),
}

impl std::fmt::Display for DivergencePoint {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::KernelRunner => write!(f, "kernel_runner"),
            Self::ArtifactCapture => write!(f, "artifact_capture"),
            Self::DigestVerification => write!(f, "digest_verification"),
            Self::CacheVerification => write!(f, "cache_verification"),
            Self::ReplayVerification => write!(f, "replay_verification"),
            Self::RegressionVerification => write!(f, "regression_verification"),
            Self::EpochVerification => write!(f, "epoch_verification"),
            Self::SimdVerification => write!(f, "simd_verification"),
            Self::ChaosVerification => write!(f, "chaos_verification"),
            Self::Custom(s) => write!(f, "custom:{}", s),
        }
    }
}

/// Digest comparison showing exact mismatch
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DigestComparison {
    /// Expected digest (from machine A or baseline)
    pub expected: String,

    /// Actual digest (from machine B or current run)
    pub actual: String,

    /// Hash algorithm used (e.g., "blake3")
    pub algorithm: String,

    /// What was hashed (e.g., "query_result", "cache_state", "final_output")
    pub artifact_type: String,
}

/// Exact command that re-triggers failure on both machines
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RerunCommand {
    /// Command to execute (e.g., "qlever-gate")
    pub executable: String,

    /// Command-line arguments
    pub args: Vec<String>,

    /// Environment variables required
    pub env: HashMap<String, String>,

    /// Working directory (relative to project root)
    pub working_dir: String,
}

impl RerunCommand {
    /// Create a new rerun command
    pub fn new(executable: impl Into<String>) -> Self {
        Self {
            executable: executable.into(),
            args: Vec::new(),
            env: HashMap::new(),
            working_dir: ".".to_string(),
        }
    }

    /// Add a command-line argument
    pub fn arg(mut self, arg: impl Into<String>) -> Self {
        self.args.push(arg.into());
        self
    }

    /// Add multiple arguments
    pub fn args<I, S>(mut self, args: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        self.args.extend(args.into_iter().map(|s| s.into()));
        self
    }

    /// Add an environment variable
    pub fn env(mut self, key: impl Into<String>, value: impl Into<String>) -> Self {
        self.env.insert(key.into(), value.into());
        self
    }

    /// Set working directory
    pub fn working_dir(mut self, dir: impl Into<String>) -> Self {
        self.working_dir = dir.into();
        self
    }

    /// Format as shell command string (for documentation/logging)
    pub fn to_shell_string(&self) -> String {
        let mut parts = Vec::new();

        // Add environment variables
        for (key, value) in &self.env {
            parts.push(format!("{}={}", key, shell_escape(value)));
        }

        // Add executable
        parts.push(shell_escape(&self.executable));

        // Add arguments
        for arg in &self.args {
            parts.push(shell_escape(arg));
        }

        parts.join(" ")
    }
}

/// Simple shell escaping for display purposes
fn shell_escape(s: &str) -> String {
    if s.contains(' ') || s.contains('$') || s.contains('"') || s.contains('\'') {
        format!("\"{}\"", s.replace('"', "\\\""))
    } else {
        s.to_string()
    }
}

/// Minimal subset of workload that shows divergence
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RelevantInputs {
    /// SPARQL queries that trigger divergence
    pub queries: Vec<String>,

    /// Input data files (paths relative to workload root)
    pub data_files: Vec<String>,

    /// Configuration parameters
    pub config: HashMap<String, serde_json::Value>,

    /// Additional metadata
    pub metadata: HashMap<String, String>,
}

impl RelevantInputs {
    /// Create empty relevant inputs
    pub fn new() -> Self {
        Self {
            queries: Vec::new(),
            data_files: Vec::new(),
            config: HashMap::new(),
            metadata: HashMap::new(),
        }
    }

    /// Add a query
    pub fn add_query(mut self, query: impl Into<String>) -> Self {
        self.queries.push(query.into());
        self
    }

    /// Add a data file
    pub fn add_data_file(mut self, path: impl Into<String>) -> Self {
        self.data_files.push(path.into());
        self
    }

    /// Add a config parameter
    pub fn add_config(mut self, key: impl Into<String>, value: serde_json::Value) -> Self {
        self.config.insert(key.into(), value);
        self
    }

    /// Add metadata
    pub fn add_metadata(mut self, key: impl Into<String>, value: impl Into<String>) -> Self {
        self.metadata.insert(key.into(), value.into());
        self
    }
}

impl Default for RelevantInputs {
    fn default() -> Self {
        Self::new()
    }
}

/// Complete witness bundle: minimal reproducible case showing divergence
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct WitnessBundle {
    /// Unique identifier for this witness
    pub id: String,

    /// When this witness was created
    pub created_at: DateTime<Utc>,

    /// Which phase/subsystem failed
    pub divergence_point: DivergencePoint,

    /// Exact command to re-trigger failure
    pub rerun_command: RerunCommand,

    /// Minimal subset of workload
    pub relevant_inputs: RelevantInputs,

    /// Digest comparison showing mismatch
    pub digest_comparison: DigestComparison,

    /// Additional context/notes
    pub notes: String,
}

impl WitnessBundle {
    /// Create a new witness bundle
    pub fn new(
        id: impl Into<String>,
        divergence_point: DivergencePoint,
        rerun_command: RerunCommand,
        relevant_inputs: RelevantInputs,
        digest_comparison: DigestComparison,
    ) -> Self {
        Self {
            id: id.into(),
            created_at: Utc::now(),
            divergence_point,
            rerun_command,
            relevant_inputs,
            digest_comparison,
            notes: String::new(),
        }
    }

    /// Add notes to the witness bundle
    pub fn with_notes(mut self, notes: impl Into<String>) -> Self {
        self.notes = notes.into();
        self
    }

    /// Serialize to JSON
    pub fn to_json(&self) -> Result<String> {
        Ok(serde_json::to_string_pretty(self)?)
    }

    /// Deserialize from JSON
    pub fn from_json(json: &str) -> Result<Self> {
        Ok(serde_json::from_str(json)?)
    }

    /// Validate the witness bundle
    pub fn validate(&self) -> Result<()> {
        if self.id.is_empty() {
            return Err(WitnessError::InvalidBundle("id cannot be empty".to_string()));
        }

        if self.rerun_command.executable.is_empty() {
            return Err(WitnessError::InvalidBundle(
                "rerun_command.executable cannot be empty".to_string(),
            ));
        }

        if self.digest_comparison.expected.is_empty() {
            return Err(WitnessError::InvalidBundle(
                "digest_comparison.expected cannot be empty".to_string(),
            ));
        }

        if self.digest_comparison.actual.is_empty() {
            return Err(WitnessError::InvalidBundle(
                "digest_comparison.actual cannot be empty".to_string(),
            ));
        }

        if self.digest_comparison.expected == self.digest_comparison.actual {
            return Err(WitnessError::InvalidBundle(
                "digest_comparison.expected and actual are identical (no divergence)".to_string(),
            ));
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rerun_command_builder() {
        let cmd = RerunCommand::new("qlever-gate")
            .arg("--workload")
            .arg("test.json")
            .env("RUST_LOG", "debug")
            .working_dir("/tmp/test");

        assert_eq!(cmd.executable, "qlever-gate");
        assert_eq!(cmd.args, vec!["--workload", "test.json"]);
        assert_eq!(cmd.env.get("RUST_LOG"), Some(&"debug".to_string()));
        assert_eq!(cmd.working_dir, "/tmp/test");
    }

    #[test]
    fn test_rerun_command_shell_string() {
        let cmd = RerunCommand::new("qlever-gate")
            .arg("--workload")
            .arg("test file.json")
            .env("MY_VAR", "value with spaces");

        let shell = cmd.to_shell_string();
        assert!(shell.contains("MY_VAR=\"value with spaces\""));
        assert!(shell.contains("\"test file.json\""));
    }

    #[test]
    fn test_relevant_inputs_builder() {
        let inputs = RelevantInputs::new()
            .add_query("SELECT ?s WHERE { ?s ?p ?o }")
            .add_data_file("data.ttl")
            .add_config("cache_size", serde_json::json!(1024))
            .add_metadata("description", "test case");

        assert_eq!(inputs.queries.len(), 1);
        assert_eq!(inputs.data_files.len(), 1);
        assert_eq!(inputs.config.len(), 1);
        assert_eq!(inputs.metadata.len(), 1);
    }

    #[test]
    fn test_witness_bundle_validation() {
        let bundle = WitnessBundle::new(
            "test-001",
            DivergencePoint::DigestVerification,
            RerunCommand::new("test"),
            RelevantInputs::new(),
            DigestComparison {
                expected: "abc123".to_string(),
                actual: "def456".to_string(),
                algorithm: "blake3".to_string(),
                artifact_type: "query_result".to_string(),
            },
        );

        assert!(bundle.validate().is_ok());
    }

    #[test]
    fn test_witness_bundle_validation_fails_on_same_digest() {
        let bundle = WitnessBundle::new(
            "test-002",
            DivergencePoint::DigestVerification,
            RerunCommand::new("test"),
            RelevantInputs::new(),
            DigestComparison {
                expected: "abc123".to_string(),
                actual: "abc123".to_string(), // Same as expected!
                algorithm: "blake3".to_string(),
                artifact_type: "query_result".to_string(),
            },
        );

        assert!(bundle.validate().is_err());
    }

    #[test]
    fn test_divergence_point_display() {
        assert_eq!(
            DivergencePoint::DigestVerification.to_string(),
            "digest_verification"
        );
        assert_eq!(
            DivergencePoint::Custom("my_check".to_string()).to_string(),
            "custom:my_check"
        );
    }
}

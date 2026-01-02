//! EPIC 11 Subsystem 5: Workload Pack Format
//!
//! CBOR deserialization for deterministic workload packs.
//! Per EPIC 11 Invariant C1 and C3.

use serde::{Deserialize, Serialize};
use std::path::Path;
use thiserror::Error;

/// Workload replay mode (per EPIC 11 Invariant C1)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "PascalCase")]
pub enum ReplayMode {
    /// Bit-identical results required; divergence = ABORT
    Strict,

    /// Report divergences but continue; emit RECEIPT at end
    Differential,

    /// Ignore timing; accept logical equivalence (for SIMD)
    BestEffort,
}

impl Default for ReplayMode {
    fn default() -> Self {
        ReplayMode::Strict
    }
}

/// Cache decision for cache behavior logging (per EPIC 11 Invariant B4)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum CacheDecision {
    /// Cache hit - result was in cache
    Hit,

    /// Cache miss - result not in cache
    Miss,

    /// Entry admitted to cache
    Admit,

    /// Entry rejected from cache
    Reject,

    /// Entry evicted from cache
    Evict,

    /// Entry protected by guard (cannot be evicted)
    Guarded,
}

impl std::fmt::Display for CacheDecision {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CacheDecision::Hit => write!(f, "HIT"),
            CacheDecision::Miss => write!(f, "MISS"),
            CacheDecision::Admit => write!(f, "ADMIT"),
            CacheDecision::Reject => write!(f, "REJECT"),
            CacheDecision::Evict => write!(f, "EVICT"),
            CacheDecision::Guarded => write!(f, "GUARDED"),
        }
    }
}

/// Single query in a workload pack (per EPIC 11 Invariant C1)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReplayQuery {
    /// Unique query identifier
    pub query_id: String,

    /// SPARQL or query language text
    pub query_text: String,

    /// Position in workload (0-indexed)
    pub execution_order: u32,

    /// Expected BLAKE3(result_bytes) in hex format
    pub expected_result_digest: String,

    /// Expected cache decision sequence
    pub expected_cache_behavior: Vec<CacheDecision>,

    /// Expected latency in milliseconds (for regression detection)
    pub expected_latency_ms: f64,
}

impl ReplayQuery {
    /// Create a new replay query
    pub fn new(
        query_id: impl Into<String>,
        query_text: impl Into<String>,
        execution_order: u32,
        expected_result_digest: impl Into<String>,
    ) -> Self {
        Self {
            query_id: query_id.into(),
            query_text: query_text.into(),
            execution_order,
            expected_result_digest: expected_result_digest.into(),
            expected_cache_behavior: vec![],
            expected_latency_ms: 0.0,
        }
    }

    /// Add expected cache behavior
    pub fn with_cache_behavior(mut self, behavior: Vec<CacheDecision>) -> Self {
        self.expected_cache_behavior = behavior;
        self
    }

    /// Add expected latency
    pub fn with_latency(mut self, latency_ms: f64) -> Self {
        self.expected_latency_ms = latency_ms;
        self
    }
}

/// Expected cache state snapshot (per EPIC 11 Invariant C1)
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct ReplayState {
    /// Total cache size in bytes at snapshot
    pub cache_size_bytes: u64,

    /// Total number of entries in cache
    pub cache_entries: u64,

    /// Cache hit rate as percentage
    pub hit_rate_pct: f64,

    /// Epoch identifier at snapshot
    pub epoch_key: Vec<u8>,
}

/// Complete workload pack (per EPIC 11 Invariant C3)
///
/// File format: CBOR-serialized ReplayWorkload
/// - File extension: .workload.cbor
/// - Size constraint: < 100 MB per workload
/// - Compression: optional ZSTD wrapping
/// - Versioning: immutable and tagged
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReplayWorkload {
    /// Workload identifier (e.g., "deterministic-corpus-v1")
    pub workload_id: String,

    /// Ordered list of queries to replay
    pub query_pack: Vec<ReplayQuery>,

    /// Expected cache state snapshots
    pub expected_state: ReplayState,

    /// How to handle divergence
    pub replay_mode: ReplayMode,
}

impl ReplayWorkload {
    /// Create a new empty workload
    pub fn new(workload_id: impl Into<String>) -> Self {
        Self {
            workload_id: workload_id.into(),
            query_pack: vec![],
            expected_state: ReplayState::default(),
            replay_mode: ReplayMode::Strict,
        }
    }

    /// Add a query to the workload
    pub fn add_query(mut self, query: ReplayQuery) -> Self {
        self.query_pack.push(query);
        self
    }

    /// Set replay mode
    pub fn with_mode(mut self, mode: ReplayMode) -> Self {
        self.replay_mode = mode;
        self
    }

    /// Set expected state
    pub fn with_expected_state(mut self, state: ReplayState) -> Self {
        self.expected_state = state;
        self
    }

    /// Get the number of queries in this workload
    pub fn len(&self) -> usize {
        self.query_pack.len()
    }

    /// Check if workload is empty
    pub fn is_empty(&self) -> bool {
        self.query_pack.is_empty()
    }
}

/// Workload pack errors
#[derive(Debug, Error)]
pub enum WorkloadPackError {
    #[error("Failed to read workload file: {0}")]
    ReadError(#[from] std::io::Error),

    #[error("Failed to deserialize CBOR: {0}")]
    DeserializeError(String),

    #[error("Workload pack too large: {size} > 100 MB")]
    WorkloadTooLarge { size: u64 },

    #[error("Invalid workload: {0}")]
    InvalidWorkload(String),
}

/// Load workload pack from CBOR file
pub fn load_workload_pack(path: &Path) -> Result<ReplayWorkload, WorkloadPackError> {
    // Check file size
    let metadata = std::fs::metadata(path)?;
    let file_size = metadata.len();
    if file_size > 100 * 1024 * 1024 {
        // 100 MB limit per spec
        return Err(WorkloadPackError::WorkloadTooLarge { size: file_size });
    }

    // Read file
    let bytes = std::fs::read(path)?;

    // Deserialize from CBOR
    let workload: ReplayWorkload = ciborium::from_reader(&bytes[..])
        .map_err(|e| WorkloadPackError::DeserializeError(e.to_string()))?;

    // Validate workload
    validate_workload(&workload)?;

    Ok(workload)
}

/// Save workload pack to CBOR file
pub fn save_workload_pack(path: &Path, workload: &ReplayWorkload) -> Result<(), WorkloadPackError> {
    validate_workload(workload)?;

    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(workload, &mut cbor_bytes)
        .map_err(|e| WorkloadPackError::DeserializeError(e.to_string()))?;

    // Check size after serialization
    if cbor_bytes.len() > 100 * 1024 * 1024 {
        return Err(WorkloadPackError::WorkloadTooLarge {
            size: cbor_bytes.len() as u64,
        });
    }

    std::fs::write(path, &cbor_bytes)?;
    Ok(())
}

/// Validate workload structure
fn validate_workload(workload: &ReplayWorkload) -> Result<(), WorkloadPackError> {
    if workload.workload_id.is_empty() {
        return Err(WorkloadPackError::InvalidWorkload(
            "workload_id cannot be empty".to_string(),
        ));
    }

    for (idx, query) in workload.query_pack.iter().enumerate() {
        if query.query_id.is_empty() {
            return Err(WorkloadPackError::InvalidWorkload(format!(
                "query[{}].query_id cannot be empty",
                idx
            )));
        }

        if query.query_text.is_empty() {
            return Err(WorkloadPackError::InvalidWorkload(format!(
                "query[{}].query_text cannot be empty",
                idx
            )));
        }

        if query.expected_result_digest.is_empty() {
            return Err(WorkloadPackError::InvalidWorkload(format!(
                "query[{}].expected_result_digest cannot be empty",
                idx
            )));
        }

        // Verify digest is valid hex
        if query.expected_result_digest.len() != 64 {
            // BLAKE3 produces 256-bit (32 bytes) = 64 hex characters
            return Err(WorkloadPackError::InvalidWorkload(format!(
                "query[{}].expected_result_digest must be 64 hex characters (BLAKE3 output)",
                idx
            )));
        }

        if !query.expected_result_digest.chars().all(|c| c.is_ascii_hexdigit()) {
            return Err(WorkloadPackError::InvalidWorkload(format!(
                "query[{}].expected_result_digest contains invalid hex characters",
                idx
            )));
        }
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_replay_query_creation() {
        let query = ReplayQuery::new(
            "q1",
            "SELECT ?x WHERE { ?x rdf:type ?t }",
            0,
            "0".repeat(64),
        )
        .with_cache_behavior(vec![CacheDecision::Miss])
        .with_latency(100.0);

        assert_eq!(query.query_id, "q1");
        assert_eq!(query.execution_order, 0);
        assert_eq!(query.expected_latency_ms, 100.0);
        assert_eq!(query.expected_cache_behavior.len(), 1);
    }

    #[test]
    fn test_replay_workload_creation() {
        let workload = ReplayWorkload::new("test-workload");
        assert_eq!(workload.workload_id, "test-workload");
        assert!(workload.is_empty());
        assert_eq!(workload.len(), 0);
    }

    #[test]
    fn test_replay_workload_add_queries() {
        let query1 = ReplayQuery::new("q1", "QUERY 1", 0, "0".repeat(64));
        let query2 = ReplayQuery::new("q2", "QUERY 2", 1, "1".repeat(64));

        let workload = ReplayWorkload::new("test")
            .add_query(query1)
            .add_query(query2);

        assert_eq!(workload.len(), 2);
        assert!(!workload.is_empty());
    }

    #[test]
    fn test_cache_decision_display() {
        assert_eq!(CacheDecision::Hit.to_string(), "HIT");
        assert_eq!(CacheDecision::Miss.to_string(), "MISS");
        assert_eq!(CacheDecision::Admit.to_string(), "ADMIT");
        assert_eq!(CacheDecision::Reject.to_string(), "REJECT");
        assert_eq!(CacheDecision::Evict.to_string(), "EVICT");
        assert_eq!(CacheDecision::Guarded.to_string(), "GUARDED");
    }

    #[test]
    fn test_replay_mode_default() {
        assert_eq!(ReplayMode::default(), ReplayMode::Strict);
    }

    #[test]
    fn test_workload_pack_serialization() {
        let query = ReplayQuery::new(
            "q1",
            "SELECT ?x WHERE { ?x rdf:type ?t }",
            0,
            "0".repeat(64),
        );

        let workload = ReplayWorkload::new("test-workload").add_query(query);

        let mut cbor_bytes = Vec::new();
        ciborium::into_writer(&workload, &mut cbor_bytes).unwrap();

        let deserialized: ReplayWorkload = ciborium::from_reader(&cbor_bytes[..]).unwrap();

        assert_eq!(deserialized.workload_id, "test-workload");
        assert_eq!(deserialized.len(), 1);
        assert_eq!(deserialized.query_pack[0].query_id, "q1");
    }

    #[test]
    fn test_workload_validation_empty_id() {
        let workload = ReplayWorkload {
            workload_id: String::new(),
            query_pack: vec![],
            expected_state: ReplayState::default(),
            replay_mode: ReplayMode::Strict,
        };

        assert!(validate_workload(&workload).is_err());
    }

    #[test]
    fn test_workload_validation_invalid_digest() {
        let query = ReplayQuery::new("q1", "QUERY", 0, "invalid_digest");

        let workload = ReplayWorkload::new("test").add_query(query);

        assert!(validate_workload(&workload).is_err());
    }

    #[test]
    fn test_workload_validation_success() {
        let query = ReplayQuery::new("q1", "SELECT 1", 0, "0".repeat(64));
        let workload = ReplayWorkload::new("test").add_query(query);

        assert!(validate_workload(&workload).is_ok());
    }

    #[test]
    fn test_replay_state_default() {
        let state = ReplayState::default();
        assert_eq!(state.cache_size_bytes, 0);
        assert_eq!(state.cache_entries, 0);
        assert_eq!(state.hit_rate_pct, 0.0);
        assert!(state.epoch_key.is_empty());
    }
}

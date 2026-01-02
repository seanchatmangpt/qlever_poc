//! EPIC 11 Subsystem 5: Replay Failure Modes
//!
//! Formal classification of the 4 replay failure modes per EPIC 11 Spec (C2).

use serde::{Deserialize, Serialize};

/// Replay failure classification (4 modes per EPIC 11 Invariant C2)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ReplayFailureClass {
    /// ReplayDivergence: Bit-level mismatch between expected and actual digest
    /// RECEIPT: {
    ///   class: "ReplayDivergence",
    ///   query_id: String,
    ///   expected_digest: String,
    ///   actual_digest: String,
    ///   first_divergence_index: u32,
    ///   divergence_evidence: Vec<u8>,  // First 256 bytes of both
    /// }
    ReplayDivergence,

    /// ReplayNonDeterminism: Same query produces different results on replay
    /// RECEIPT: {
    ///   class: "ReplayNonDeterminism",
    ///   query_id: String,
    ///   run_1_digest: String,
    ///   run_2_digest: String,
    ///   runs_count: u32,  // How many consecutive identical runs before divergence
    /// }
    ReplayNonDeterminism,

    /// ReplayTimeout: Execution exceeded time budget
    /// RECEIPT: {
    ///   class: "ReplayTimeout",
    ///   query_id: String,
    ///   timeout_ms: u64,
    ///   actual_execution_ms: u64,
    /// }
    ReplayTimeout,

    /// ReplayAbort: Expected result not in cache after promotion
    /// RECEIPT: {
    ///   class: "ReplayAbort",
    ///   query_id: String,
    ///   reason: String,   // e.g., "cache_evicted_before_replay"
    /// }
    ReplayAbort,
}

impl ReplayFailureClass {
    /// Returns a human-readable description of the failure class
    pub fn description(&self) -> &'static str {
        match self {
            ReplayFailureClass::ReplayDivergence => {
                "Bit-level result mismatch from expected digest"
            }
            ReplayFailureClass::ReplayNonDeterminism => {
                "Same query produces different results on replay"
            }
            ReplayFailureClass::ReplayTimeout => {
                "Query execution exceeded time budget"
            }
            ReplayFailureClass::ReplayAbort => {
                "Expected result not in cache after promotion"
            }
        }
    }

    /// Returns whether this failure is blocking
    pub fn is_blocking(&self) -> bool {
        true // All replay failures are blocking
    }
}

impl std::fmt::Display for ReplayFailureClass {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ReplayFailureClass::ReplayDivergence => write!(f, "ReplayDivergence"),
            ReplayFailureClass::ReplayNonDeterminism => write!(f, "ReplayNonDeterminism"),
            ReplayFailureClass::ReplayTimeout => write!(f, "ReplayTimeout"),
            ReplayFailureClass::ReplayAbort => write!(f, "ReplayAbort"),
        }
    }
}

/// Single replay failure with full evidence
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReplayFailure {
    /// Query ID that failed
    pub query_id: String,

    /// Failure classification
    pub failure_class: ReplayFailureClass,

    /// Human-readable failure details
    pub details: String,

    /// First byte index where divergence occurred (if applicable)
    pub divergence_index: Option<usize>,
}

impl ReplayFailure {
    /// Create a new replay failure
    pub fn new(
        query_id: impl Into<String>,
        failure_class: ReplayFailureClass,
        details: impl Into<String>,
    ) -> Self {
        Self {
            query_id: query_id.into(),
            failure_class,
            details: details.into(),
            divergence_index: None,
        }
    }

    /// Add divergence index evidence
    pub fn with_divergence_index(mut self, index: usize) -> Self {
        self.divergence_index = Some(index);
        self
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_all_failure_classes_are_blocking() {
        assert!(ReplayFailureClass::ReplayDivergence.is_blocking());
        assert!(ReplayFailureClass::ReplayNonDeterminism.is_blocking());
        assert!(ReplayFailureClass::ReplayTimeout.is_blocking());
        assert!(ReplayFailureClass::ReplayAbort.is_blocking());
    }

    #[test]
    fn test_failure_class_descriptions() {
        assert!(!ReplayFailureClass::ReplayDivergence.description().is_empty());
        assert!(!ReplayFailureClass::ReplayNonDeterminism.description().is_empty());
        assert!(!ReplayFailureClass::ReplayTimeout.description().is_empty());
        assert!(!ReplayFailureClass::ReplayAbort.description().is_empty());
    }

    #[test]
    fn test_failure_class_display() {
        assert_eq!(ReplayFailureClass::ReplayDivergence.to_string(), "ReplayDivergence");
        assert_eq!(ReplayFailureClass::ReplayNonDeterminism.to_string(), "ReplayNonDeterminism");
        assert_eq!(ReplayFailureClass::ReplayTimeout.to_string(), "ReplayTimeout");
        assert_eq!(ReplayFailureClass::ReplayAbort.to_string(), "ReplayAbort");
    }

    #[test]
    fn test_replay_failure_creation() {
        let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Digest mismatch");
        assert_eq!(failure.query_id, "q1");
        assert_eq!(failure.failure_class, ReplayFailureClass::ReplayDivergence);
        assert!(failure.divergence_index.is_none());
    }

    #[test]
    fn test_replay_failure_with_divergence_index() {
        let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence")
            .with_divergence_index(42);
        assert_eq!(failure.divergence_index, Some(42));
    }

    #[test]
    fn test_replay_failure_serialization() {
        let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Test")
            .with_divergence_index(100);

        let json = serde_json::to_string(&failure).unwrap();
        let deserialized: ReplayFailure = serde_json::from_str(&json).unwrap();

        assert_eq!(deserialized.query_id, "q1");
        assert_eq!(deserialized.failure_class, ReplayFailureClass::ReplayDivergence);
        assert_eq!(deserialized.divergence_index, Some(100));
    }
}

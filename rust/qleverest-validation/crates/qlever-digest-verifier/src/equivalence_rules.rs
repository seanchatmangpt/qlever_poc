//! EPIC 11 Equivalence Rules (Invariant B2)
//!
//! Three determinism equivalence rules for digest verification:
//! 1. Replay equivalence: result_hash(query_i, epoch_j, baseline) == result_hash(query_i, epoch_j, cached)
//! 2. Cross-machine equivalence: combined_digest(M1) == combined_digest(M2) for same workload
//! 3. Cache behavior equivalence: cache_log_hash sequences must match exactly (order matters)

use crate::DeterminismDigest;
use thiserror::Error;

/// Equivalence rule identifier
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EquivalenceRuleType {
    /// Replay: baseline vs cached result must have identical result_hash
    Replay,
    /// CrossMachine: results on different machines must have identical combined_digest
    CrossMachine,
    /// CacheBehavior: cache decision log sequences must match exactly (order-sensitive)
    CacheBehavior,
}

/// Equivalence rule for digest validation
#[derive(Debug, Clone)]
pub struct EquivalenceRule {
    /// Rule type (one of three)
    pub rule_type: EquivalenceRuleType,
    /// Query ID being validated
    pub query_id: String,
    /// Expected digest (from baseline or workload pack)
    pub expected: DeterminismDigest,
    /// Actual digest (from current execution)
    pub actual: DeterminismDigest,
}

/// Equivalence validation errors
#[derive(Debug, Error)]
pub enum EquivalenceError {
    #[error("Replay equivalence violated: result hashes differ for query {query_id}")]
    ReplayDivergence { query_id: String },

    #[error("Cross-machine equivalence violated: combined digests differ for query {query_id}. Expected {expected_machine}, got {actual_machine}")]
    CrossMachineDivergence {
        query_id: String,
        expected_machine: String,
        actual_machine: String,
    },

    #[error("Cache behavior equivalence violated: cache decision logs differ for query {query_id}")]
    CacheBehaviorDivergence { query_id: String },
}

/// Validation result for equivalence rules
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum EquivalenceValidationResult {
    /// All equivalence rules satisfied
    Valid,
    /// Validation failed with specific rule violation
    Invalid(EquivalenceRuleType),
}

impl EquivalenceRule {
    /// Create a new equivalence rule
    pub fn new(
        rule_type: EquivalenceRuleType,
        query_id: impl Into<String>,
        expected: DeterminismDigest,
        actual: DeterminismDigest,
    ) -> Self {
        Self {
            rule_type,
            query_id: query_id.into(),
            expected,
            actual,
        }
    }

    /// Validate replay equivalence: result_hash must match exactly
    pub fn validate_replay(&self) -> Result<EquivalenceValidationResult, EquivalenceError> {
        if self.expected.result_hash != self.actual.result_hash {
            return Err(EquivalenceError::ReplayDivergence {
                query_id: self.query_id.clone(),
            });
        }
        Ok(EquivalenceValidationResult::Valid)
    }

    /// Validate cross-machine equivalence: combined_digest must match exactly
    pub fn validate_cross_machine(&self) -> Result<EquivalenceValidationResult, EquivalenceError> {
        if self.expected.combined_digest != self.actual.combined_digest {
            return Err(EquivalenceError::CrossMachineDivergence {
                query_id: self.query_id.clone(),
                expected_machine: self.expected.machine_fingerprint.machine_id(),
                actual_machine: self.actual.machine_fingerprint.machine_id(),
            });
        }
        Ok(EquivalenceValidationResult::Valid)
    }

    /// Validate cache behavior equivalence: cache_log_hash must match exactly (order-sensitive)
    pub fn validate_cache_behavior(&self) -> Result<EquivalenceValidationResult, EquivalenceError> {
        if self.expected.cache_log_hash != self.actual.cache_log_hash {
            return Err(EquivalenceError::CacheBehaviorDivergence {
                query_id: self.query_id.clone(),
            });
        }
        Ok(EquivalenceValidationResult::Valid)
    }

    /// Validate according to rule type
    pub fn validate(&self) -> Result<EquivalenceValidationResult, EquivalenceError> {
        match self.rule_type {
            EquivalenceRuleType::Replay => self.validate_replay(),
            EquivalenceRuleType::CrossMachine => self.validate_cross_machine(),
            EquivalenceRuleType::CacheBehavior => self.validate_cache_behavior(),
        }
    }

    /// Get human-readable description of equivalence rule
    pub fn description(&self) -> &'static str {
        match self.rule_type {
            EquivalenceRuleType::Replay => {
                "Replay equivalence: baseline and cached results must produce identical result_hash"
            }
            EquivalenceRuleType::CrossMachine => {
                "Cross-machine equivalence: same workload on different machines must produce identical combined_digest"
            }
            EquivalenceRuleType::CacheBehavior => {
                "Cache behavior equivalence: cache decision log sequences must match exactly (order matters)"
            }
        }
    }
}

/// Builder for equivalence rules
pub struct EquivalenceRuleBuilder {
    rule_type: Option<EquivalenceRuleType>,
    query_id: Option<String>,
    expected: Option<DeterminismDigest>,
    actual: Option<DeterminismDigest>,
}

impl EquivalenceRuleBuilder {
    /// Create new builder
    pub fn new() -> Self {
        Self {
            rule_type: None,
            query_id: None,
            expected: None,
            actual: None,
        }
    }

    /// Set rule type
    pub fn with_rule_type(mut self, rule_type: EquivalenceRuleType) -> Self {
        self.rule_type = Some(rule_type);
        self
    }

    /// Set query ID
    pub fn with_query_id(mut self, query_id: impl Into<String>) -> Self {
        self.query_id = Some(query_id.into());
        self
    }

    /// Set expected digest
    pub fn with_expected(mut self, expected: DeterminismDigest) -> Self {
        self.expected = Some(expected);
        self
    }

    /// Set actual digest
    pub fn with_actual(mut self, actual: DeterminismDigest) -> Self {
        self.actual = Some(actual);
        self
    }

    /// Build the rule
    pub fn build(self) -> Option<EquivalenceRule> {
        Some(EquivalenceRule {
            rule_type: self.rule_type?,
            query_id: self.query_id?,
            expected: self.expected?,
            actual: self.actual?,
        })
    }
}

impl Default for EquivalenceRuleBuilder {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::blake3_hash::hash_bytes;
    use crate::MachineInfo;

    fn create_test_digest(query_id: &str, data: &[u8]) -> DeterminismDigest {
        let result_hash = hash_bytes(data);
        let cache_log_hash = hash_bytes(b"test cache log");
        let mut combined_input = Vec::new();
        combined_input.extend_from_slice(result_hash.as_bytes());
        combined_input.extend_from_slice(cache_log_hash.as_bytes());
        let combined_digest = hash_bytes(&combined_input);

        DeterminismDigest {
            query_id: query_id.to_string(),
            result_hash,
            cache_log_hash,
            combined_digest,
            machine_fingerprint: MachineInfo::default(),
        }
    }

    #[test]
    fn test_replay_equivalence_valid() {
        let data = b"test result";
        let digest = create_test_digest("q1", data);

        let rule = EquivalenceRule::new(EquivalenceRuleType::Replay, "q1", digest.clone(), digest);

        assert!(rule.validate_replay().is_ok());
    }

    #[test]
    fn test_replay_equivalence_invalid() {
        let digest1 = create_test_digest("q1", b"result A");
        let digest2 = create_test_digest("q1", b"result B");

        let rule = EquivalenceRule::new(EquivalenceRuleType::Replay, "q1", digest1, digest2);

        assert!(rule.validate_replay().is_err());
    }

    #[test]
    fn test_cross_machine_equivalence_valid() {
        let data = b"test result";
        let digest = create_test_digest("q1", data);

        let rule = EquivalenceRule::new(
            EquivalenceRuleType::CrossMachine,
            "q1",
            digest.clone(),
            digest,
        );

        assert!(rule.validate_cross_machine().is_ok());
    }

    #[test]
    fn test_cross_machine_equivalence_invalid() {
        let digest1 = create_test_digest("q1", b"result A");
        let digest2 = create_test_digest("q1", b"result B");

        let rule = EquivalenceRule::new(
            EquivalenceRuleType::CrossMachine,
            "q1",
            digest1,
            digest2,
        );

        assert!(rule.validate_cross_machine().is_err());
    }

    #[test]
    fn test_cache_behavior_equivalence_valid() {
        let digest1 = create_test_digest("q1", b"result A");
        let digest2 = create_test_digest("q1", b"result A");

        let rule = EquivalenceRule::new(
            EquivalenceRuleType::CacheBehavior,
            "q1",
            digest1,
            digest2,
        );

        assert!(rule.validate_cache_behavior().is_ok());
    }

    #[test]
    fn test_cache_behavior_equivalence_invalid() {
        use crate::blake3_hash::hash_bytes;

        // Create digests with different cache logs manually
        let result_data = b"result";
        let result_hash = hash_bytes(result_data);

        let cache_log_hash_a = hash_bytes(b"[{\"decision\":\"HIT\"}]");
        let cache_log_hash_b = hash_bytes(b"[{\"decision\":\"MISS\"}]");

        let mut combined_input_a = Vec::new();
        combined_input_a.extend_from_slice(result_hash.as_bytes());
        combined_input_a.extend_from_slice(cache_log_hash_a.as_bytes());
        let combined_digest_a = hash_bytes(&combined_input_a);

        let mut combined_input_b = Vec::new();
        combined_input_b.extend_from_slice(result_hash.as_bytes());
        combined_input_b.extend_from_slice(cache_log_hash_b.as_bytes());
        let combined_digest_b = hash_bytes(&combined_input_b);

        let digest1 = DeterminismDigest {
            query_id: "q1".to_string(),
            result_hash,
            cache_log_hash: cache_log_hash_a,
            combined_digest: combined_digest_a,
            machine_fingerprint: Default::default(),
        };

        let digest2 = DeterminismDigest {
            query_id: "q1".to_string(),
            result_hash,
            cache_log_hash: cache_log_hash_b,
            combined_digest: combined_digest_b,
            machine_fingerprint: Default::default(),
        };

        let rule = EquivalenceRule::new(
            EquivalenceRuleType::CacheBehavior,
            "q1",
            digest1,
            digest2,
        );

        assert!(rule.validate_cache_behavior().is_err());
    }

    #[test]
    fn test_rule_builder() {
        let digest = create_test_digest("q1", b"test");

        let rule = EquivalenceRuleBuilder::new()
            .with_rule_type(EquivalenceRuleType::Replay)
            .with_query_id("q1")
            .with_expected(digest.clone())
            .with_actual(digest)
            .build()
            .unwrap();

        assert_eq!(rule.rule_type, EquivalenceRuleType::Replay);
        assert_eq!(rule.query_id, "q1");
    }

    #[test]
    fn test_equivalence_rule_description() {
        let digest = create_test_digest("q1", b"test");

        let rule = EquivalenceRule::new(EquivalenceRuleType::Replay, "q1", digest.clone(), digest);
        assert!(!rule.description().is_empty());
        assert!(rule.description().contains("Replay"));
    }
}

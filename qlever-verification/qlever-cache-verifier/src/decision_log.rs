//! Cache decision log parsing and management
//!
//! Implements the decision log format from EPIC 11 Invariant B4.

pub use qlever_kernel_runner::{CacheDecision, CacheDecisionType};
use serde::{Deserialize, Serialize};

/// Cache decision log - records all cache operations
#[derive(Debug, Clone, Default)]
pub struct CacheDecisionLog {
    decisions: Vec<CacheDecision>,
}

impl CacheDecisionLog {
    /// Create a new empty decision log
    pub fn new() -> Self {
        Self {
            decisions: Vec::new(),
        }
    }

    /// Record a decision
    pub fn record(&mut self, decision: CacheDecision) {
        self.decisions.push(decision);
    }

    /// Get all decisions
    pub fn decisions(&self) -> &[CacheDecision] {
        &self.decisions
    }

    /// Get decisions for a specific query
    pub fn get_decisions_for_query(&self, query_id: &str) -> Vec<&CacheDecision> {
        self.decisions
            .iter()
            .filter(|d| d.query_id == query_id)
            .collect()
    }

    /// Get the number of decisions
    pub fn len(&self) -> usize {
        self.decisions.len()
    }

    /// Check if the log is empty
    pub fn is_empty(&self) -> bool {
        self.decisions.is_empty()
    }

    /// Parse from JSON
    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        let decisions: Vec<CacheDecision> = serde_json::from_str(json)?;
        Ok(Self { decisions })
    }

    /// Serialize to JSON
    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string(&self.decisions)
    }

    /// Compute BLAKE3 hash of the decision log
    pub fn compute_hash(&self) -> [u8; 32] {
        let json = self.to_json().unwrap_or_default();
        *blake3::hash(json.as_bytes()).as_bytes()
    }

    /// Get hit rate from the log
    pub fn hit_rate(&self) -> f64 {
        if self.decisions.is_empty() {
            return 0.0;
        }

        let hits = self
            .decisions
            .iter()
            .filter(|d| matches!(d.decision, CacheDecisionType::Hit))
            .count();

        let total_gets = self
            .decisions
            .iter()
            .filter(|d| {
                matches!(
                    d.decision,
                    CacheDecisionType::Hit | CacheDecisionType::Miss
                )
            })
            .count();

        if total_gets == 0 {
            0.0
        } else {
            hits as f64 / total_gets as f64
        }
    }

    /// Get eviction count
    pub fn eviction_count(&self) -> usize {
        self.decisions
            .iter()
            .filter(|d| matches!(d.decision, CacheDecisionType::Evict))
            .count()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_decision_log_creation() {
        let log = CacheDecisionLog::new();
        assert!(log.is_empty());
        assert_eq!(log.len(), 0);
    }

    #[test]
    fn test_record_decision() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        assert_eq!(log.len(), 1);
        assert_eq!(log.decisions()[0].query_id, "q1");
    }

    #[test]
    fn test_get_decisions_for_query() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });
        log.record(CacheDecision {
            timestamp_ns: 2000,
            query_id: "q2".to_string(),
            decision: CacheDecisionType::Hit,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });
        log.record(CacheDecision {
            timestamp_ns: 3000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Admit,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        let q1_decisions = log.get_decisions_for_query("q1");
        assert_eq!(q1_decisions.len(), 2);
    }

    #[test]
    fn test_json_roundtrip() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        let json = log.to_json().unwrap();
        let parsed = CacheDecisionLog::from_json(&json).unwrap();

        assert_eq!(parsed.len(), 1);
        assert_eq!(parsed.decisions()[0].query_id, "q1");
    }

    #[test]
    fn test_hit_rate() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Hit,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });
        log.record(CacheDecision {
            timestamp_ns: 2000,
            query_id: "q2".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        assert!((log.hit_rate() - 0.5).abs() < 0.001);
    }

    #[test]
    fn test_compute_hash() {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Miss,
            cache_tier: "bytes".to_string(),
            evicted_entry_id: None,
        });

        let hash1 = log.compute_hash();
        let hash2 = log.compute_hash();

        // Deterministic
        assert_eq!(hash1, hash2);
    }
}

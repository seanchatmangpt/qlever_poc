//! EPIC 11 Subsystem 5: Replay Executor
//!
//! Workload execution and divergence detection.

use serde::{Deserialize, Serialize};
use crate::workload_pack::CacheDecision;

/// Result of executing a single query
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QueryExecutionResult {
    /// Query ID that was executed
    pub query_id: String,

    /// Raw result bytes from query execution
    pub result_bytes: Vec<u8>,

    /// Cache behavior decisions made during execution
    pub cache_behavior: Vec<CacheDecision>,

    /// Time to execute this query in milliseconds
    pub execution_time_ms: f64,
}

impl QueryExecutionResult {
    /// Create a new query execution result
    pub fn new(
        query_id: impl Into<String>,
        result_bytes: Vec<u8>,
        execution_time_ms: f64,
    ) -> Self {
        Self {
            query_id: query_id.into(),
            result_bytes,
            cache_behavior: vec![],
            execution_time_ms,
        }
    }

    /// Add cache behavior decision
    pub fn with_cache_behavior(mut self, behavior: Vec<CacheDecision>) -> Self {
        self.cache_behavior = behavior;
        self
    }

    /// Get the size of result bytes
    pub fn result_size(&self) -> usize {
        self.result_bytes.len()
    }
}

/// Statistics from a replay execution
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReplayExecutionStats {
    /// Total queries executed
    pub total_queries: u32,

    /// Total bytes of results
    pub total_result_bytes: u64,

    /// Average execution time per query
    pub avg_execution_time_ms: f64,

    /// Total execution time
    pub total_execution_time_ms: f64,

    /// Cache hit count
    pub cache_hits: u32,

    /// Cache miss count
    pub cache_misses: u32,
}

impl ReplayExecutionStats {
    /// Calculate cache hit rate as percentage
    pub fn hit_rate_pct(&self) -> f64 {
        if self.cache_hits + self.cache_misses == 0 {
            0.0
        } else {
            (self.cache_hits as f64 / (self.cache_hits + self.cache_misses) as f64) * 100.0
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_query_execution_result_creation() {
        let result = QueryExecutionResult::new("q1", vec![1, 2, 3, 4], 100.0);
        assert_eq!(result.query_id, "q1");
        assert_eq!(result.result_bytes.len(), 4);
        assert_eq!(result.execution_time_ms, 100.0);
    }

    #[test]
    fn test_query_execution_result_size() {
        let result = QueryExecutionResult::new("q1", vec![1, 2, 3, 4, 5], 50.0);
        assert_eq!(result.result_size(), 5);
    }

    #[test]
    fn test_query_execution_with_cache_behavior() {
        let result = QueryExecutionResult::new("q1", vec![1, 2, 3], 100.0)
            .with_cache_behavior(vec![CacheDecision::Hit, CacheDecision::Miss]);

        assert_eq!(result.cache_behavior.len(), 2);
        assert_eq!(result.cache_behavior[0], CacheDecision::Hit);
        assert_eq!(result.cache_behavior[1], CacheDecision::Miss);
    }

    #[test]
    fn test_replay_execution_stats_hit_rate() {
        let stats = ReplayExecutionStats {
            total_queries: 100,
            total_result_bytes: 10000,
            avg_execution_time_ms: 10.0,
            total_execution_time_ms: 1000.0,
            cache_hits: 80,
            cache_misses: 20,
        };

        assert_eq!(stats.hit_rate_pct(), 80.0);
    }

    #[test]
    fn test_replay_execution_stats_zero_queries() {
        let stats = ReplayExecutionStats {
            total_queries: 0,
            total_result_bytes: 0,
            avg_execution_time_ms: 0.0,
            total_execution_time_ms: 0.0,
            cache_hits: 0,
            cache_misses: 0,
        };

        assert_eq!(stats.hit_rate_pct(), 0.0);
    }

    #[test]
    fn test_replay_execution_stats_serialization() {
        let stats = ReplayExecutionStats {
            total_queries: 50,
            total_result_bytes: 5000,
            avg_execution_time_ms: 10.0,
            total_execution_time_ms: 500.0,
            cache_hits: 40,
            cache_misses: 10,
        };

        let json = serde_json::to_string(&stats).unwrap();
        let deserialized: ReplayExecutionStats = serde_json::from_str(&json).unwrap();

        assert_eq!(deserialized.total_queries, 50);
        assert_eq!(deserialized.cache_hits, 40);
        assert_eq!(deserialized.hit_rate_pct(), 80.0);
    }
}

//! Regression Gate Matrix (Formal Definition)
//!
//! EPIC 11, Part I, Invariant E: Six regression gates from formal matrix.
//! All gates are deterministic, mechanical, and enumerated exhaustively.

use serde::{Deserialize, Serialize};
use std::fmt;

/// Regression gate identifier (enumeration)
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
pub enum RegressionGateId {
    /// p95 query latency (threshold: 10%)
    P95LatencyRegression,
    /// p99 query latency (threshold: 15%)
    P99LatencyRegression,
    /// Cache hit rate (threshold: 5%, baseline = rolling 7-day median)
    CacheHitRateRegression,
    /// Queries per second (threshold: 5%)
    QpsRegression,
    /// Cache memory usage (threshold: 20%, advisory only)
    CacheMemoryRegression,
    /// SIMD equivalence: AVX-512 vs scalar (threshold: 0%, zero tolerance)
    SimdEquivalenceAvx512VsScalar,
}

impl fmt::Display for RegressionGateId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::P95LatencyRegression => write!(f, "p95_latency_regression"),
            Self::P99LatencyRegression => write!(f, "p99_latency_regression"),
            Self::CacheHitRateRegression => write!(f, "cache_hit_rate_regression"),
            Self::QpsRegression => write!(f, "qps_regression"),
            Self::CacheMemoryRegression => write!(f, "cache_memory_regression"),
            Self::SimdEquivalenceAvx512VsScalar => write!(f, "simd_equivalence_avx512_vs_scalar"),
        }
    }
}

/// Regression gate definition from formal matrix
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegressionGate {
    pub gate_id: RegressionGateId,
    pub metric: String,
    pub baseline_source: BaselineSource,
    pub threshold_pct: f64,
    pub blocking: bool,
    pub test_category: String,
}

/// Source for baseline comparison
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub enum BaselineSource {
    /// Compare to git parent commit (previous commit on same branch)
    GitParentCommit,
    /// Compare to rolling 7-day median (for hit rate only)
    Rolling7DayP50,
    /// Compare to baseline from artifact (e.g., AVX-512 baseline)
    ArtifactBaseline(String),
}

impl fmt::Display for BaselineSource {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::GitParentCommit => write!(f, "git_parent_commit"),
            Self::Rolling7DayP50 => write!(f, "rolling_7day_p50"),
            Self::ArtifactBaseline(name) => write!(f, "artifact_baseline:{}", name),
        }
    }
}

/// Create all 6 gates from formal matrix (Invariant E1)
pub fn create_all_gates() -> Vec<RegressionGate> {
    vec![
        // LATENCY GATES
        RegressionGate {
            gate_id: RegressionGateId::P95LatencyRegression,
            metric: "p95_latency_ms".to_string(),
            baseline_source: BaselineSource::GitParentCommit,
            threshold_pct: 10.0,
            blocking: true,
            test_category: "extended".to_string(),
        },
        RegressionGate {
            gate_id: RegressionGateId::P99LatencyRegression,
            metric: "p99_latency_ms".to_string(),
            baseline_source: BaselineSource::GitParentCommit,
            threshold_pct: 15.0,
            blocking: true,
            test_category: "extended".to_string(),
        },
        // CACHE HIT RATE GATE
        RegressionGate {
            gate_id: RegressionGateId::CacheHitRateRegression,
            metric: "cache_hit_rate_pct".to_string(),
            baseline_source: BaselineSource::Rolling7DayP50,
            threshold_pct: 5.0,
            blocking: true,
            test_category: "extended".to_string(),
        },
        // THROUGHPUT GATE
        RegressionGate {
            gate_id: RegressionGateId::QpsRegression,
            metric: "qps".to_string(),
            baseline_source: BaselineSource::GitParentCommit,
            threshold_pct: 5.0,
            blocking: true,
            test_category: "extended".to_string(),
        },
        // MEMORY GATE
        RegressionGate {
            gate_id: RegressionGateId::CacheMemoryRegression,
            metric: "cache_memory_bytes".to_string(),
            baseline_source: BaselineSource::GitParentCommit,
            threshold_pct: 20.0,
            blocking: false, // Advisory only
            test_category: "extended".to_string(),
        },
        // SIMD EQUIVALENCE GATE
        RegressionGate {
            gate_id: RegressionGateId::SimdEquivalenceAvx512VsScalar,
            metric: "digest_equality".to_string(),
            baseline_source: BaselineSource::ArtifactBaseline("avx512_baseline".to_string()),
            threshold_pct: 0.0, // ZERO tolerance
            blocking: true,
            test_category: "contract".to_string(), // Fast gate, runs on every PR
        },
    ]
}

/// Get a single gate by ID
pub fn get_gate(gate_id: RegressionGateId) -> Option<RegressionGate> {
    create_all_gates().into_iter().find(|g| g.gate_id == gate_id)
}

/// Get all gates in a test category
pub fn gates_in_category(test_category: &str) -> Vec<RegressionGate> {
    create_all_gates()
        .into_iter()
        .filter(|g| g.test_category == test_category)
        .collect()
}

/// Get all blocking gates
pub fn blocking_gates() -> Vec<RegressionGate> {
    create_all_gates()
        .into_iter()
        .filter(|g| g.blocking)
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_all_gates_created() {
        let gates = create_all_gates();
        assert_eq!(gates.len(), 6);
    }

    #[test]
    fn test_p95_latency_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::P95LatencyRegression).unwrap();
        assert_eq!(gate.threshold_pct, 10.0);
        assert!(gate.blocking);
        assert_eq!(gate.test_category, "extended");
    }

    #[test]
    fn test_p99_latency_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::P99LatencyRegression).unwrap();
        assert_eq!(gate.threshold_pct, 15.0);
        assert!(gate.blocking);
    }

    #[test]
    fn test_cache_hit_rate_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::CacheHitRateRegression).unwrap();
        assert_eq!(gate.threshold_pct, 5.0);
        assert_eq!(gate.baseline_source, BaselineSource::Rolling7DayP50);
        assert!(gate.blocking);
    }

    #[test]
    fn test_qps_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::QpsRegression).unwrap();
        assert_eq!(gate.threshold_pct, 5.0);
        assert!(gate.blocking);
    }

    #[test]
    fn test_memory_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::CacheMemoryRegression).unwrap();
        assert_eq!(gate.threshold_pct, 20.0);
        assert!(!gate.blocking); // Advisory only
    }

    #[test]
    fn test_simd_equivalence_gate() {
        let gates = create_all_gates();
        let gate = gates.iter().find(|g| g.gate_id == RegressionGateId::SimdEquivalenceAvx512VsScalar).unwrap();
        assert_eq!(gate.threshold_pct, 0.0); // Zero tolerance
        assert!(gate.blocking);
        assert_eq!(gate.test_category, "contract");
    }

    #[test]
    fn test_get_gate() {
        let gate = get_gate(RegressionGateId::P95LatencyRegression).unwrap();
        assert_eq!(gate.metric, "p95_latency_ms");
    }

    #[test]
    fn test_gates_in_category() {
        let extended = gates_in_category("extended");
        assert_eq!(extended.len(), 5); // 5 extended gates

        let contract = gates_in_category("contract");
        assert_eq!(contract.len(), 1); // 1 contract gate (SIMD)
    }

    #[test]
    fn test_blocking_gates() {
        let blocking = blocking_gates();
        assert_eq!(blocking.len(), 5); // 5 blocking gates (all except memory)
    }

    #[test]
    fn test_gate_id_display() {
        assert_eq!(RegressionGateId::P95LatencyRegression.to_string(), "p95_latency_regression");
        assert_eq!(RegressionGateId::QpsRegression.to_string(), "qps_regression");
    }

    #[test]
    fn test_baseline_source_display() {
        assert_eq!(BaselineSource::GitParentCommit.to_string(), "git_parent_commit");
        assert_eq!(BaselineSource::Rolling7DayP50.to_string(), "rolling_7day_p50");
        assert_eq!(
            BaselineSource::ArtifactBaseline("avx512".to_string()).to_string(),
            "artifact_baseline:avx512"
        );
    }
}

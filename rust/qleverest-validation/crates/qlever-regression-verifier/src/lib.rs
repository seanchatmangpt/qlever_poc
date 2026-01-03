//! EPIC 11 Subsystem 6: Performance Regression Detection
//!
//! Deterministic regression gates for performance metrics.
//! All gates from formal matrix enforced mechanically with no human judgment.
//!
//! Shared Invariant: Regression gates are deterministic and mechanical.
//! No human judgment. All gates from formal matrix enforced.

pub mod baseline;
pub mod gate_matrix;

use baseline::Baseline;
use gate_matrix::{RegressionGate, RegressionGateId};
use qlever_artifact_capture::{FailureClass, VerificationReceipt};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Result of a single regression gate check
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegressionCheckResult {
    pub gate_id: String,
    pub metric_name: String,
    pub current_value: f64,
    pub baseline_value: f64,
    pub delta_pct: f64,
    pub threshold_pct: f64,
    pub passed: bool,
    pub blocking: bool,
    pub test_category: String,
}

impl RegressionCheckResult {
    /// Create a new regression check result
    pub fn new(
        gate_id: String,
        metric_name: String,
        current_value: f64,
        baseline_value: f64,
        threshold_pct: f64,
        blocking: bool,
        test_category: String,
    ) -> Self {
        // Delta formula: delta = (M - B) / B * 100
        let delta_pct = ((current_value - baseline_value) / baseline_value) * 100.0;
        let passed = delta_pct.abs() <= threshold_pct;

        Self {
            gate_id,
            metric_name,
            current_value,
            baseline_value,
            delta_pct,
            threshold_pct,
            passed,
            blocking,
            test_category,
        }
    }

    /// Convert to a verification receipt if regression detected
    pub fn to_receipt(&self) -> Option<VerificationReceipt> {
        if self.passed {
            return None;
        }

        let failure_class = match self.metric_name.as_str() {
            "p95_latency_ms" | "p99_latency_ms" => FailureClass::LatencyRegression,
            "qps" => FailureClass::ThroughputRegression,
            "cache_memory_bytes" => FailureClass::MemoryRegression,
            "cache_hit_rate_pct" => FailureClass::CacheHitRateRegression,
            "digest_equality" => FailureClass::SimdScalarMismatch,
            _ => FailureClass::ThroughputRegression, // Default to throughput
        };

        let reproduction_cmd = format!(
            "qlever-verify regression --gate {} --metric {} --baseline {}",
            self.gate_id, self.metric_name, self.baseline_value
        );

        let recommended_action = format!(
            "Performance regression detected: {} increased {:.2}% (threshold: {:.2}%)",
            self.metric_name, self.delta_pct, self.threshold_pct
        );

        let mut receipt = VerificationReceipt::new(failure_class, reproduction_cmd, recommended_action);

        // Add evidence
        let evidence = format!(
            "current_value={}, baseline_value={}, delta_pct={:.2}",
            self.current_value, self.baseline_value, self.delta_pct
        );
        receipt = receipt.with_evidence("metrics", evidence.into_bytes());
        receipt = receipt.with_digest_evidence(format!("gate:{}", self.gate_id));

        Some(receipt)
    }
}

/// Regression check results for multiple gates
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct RegressionCheckResults {
    pub results: Vec<RegressionCheckResult>,
    pub passed: bool,
    pub blocking_failures: Vec<String>,
}

impl RegressionCheckResults {
    /// Create new results container
    pub fn new(results: Vec<RegressionCheckResult>) -> Self {
        let blocking_failures: Vec<String> = results
            .iter()
            .filter(|r| !r.passed && r.blocking)
            .map(|r| r.gate_id.clone())
            .collect();

        let passed = blocking_failures.is_empty();

        Self {
            results,
            passed,
            blocking_failures,
        }
    }

    /// Get all results that failed
    pub fn failed_results(&self) -> Vec<&RegressionCheckResult> {
        self.results.iter().filter(|r| !r.passed).collect()
    }

    /// Get all results that passed
    pub fn passed_results(&self) -> Vec<&RegressionCheckResult> {
        self.results.iter().filter(|r| r.passed).collect()
    }

    /// Generate receipts for all failures
    pub fn generate_receipts(&self) -> Vec<VerificationReceipt> {
        self.results
            .iter()
            .filter_map(|r| r.to_receipt())
            .collect()
    }
}

/// Main regression checker
pub struct RegressionChecker {
    gates: Vec<RegressionGate>,
}

impl RegressionChecker {
    /// Create a new regression checker with all gates from the formal matrix
    pub fn new() -> Self {
        Self {
            gates: gate_matrix::create_all_gates(),
        }
    }

    /// Get all configured gates
    pub fn gates(&self) -> &[RegressionGate] {
        &self.gates
    }

    /// Check regressions given current metrics and baselines
    pub fn check_regressions(
        &self,
        current_metrics: &HashMap<String, f64>,
        baselines: &Baseline,
    ) -> RegressionCheckResults {
        let results = self
            .gates
            .iter()
            .filter_map(|gate| {
                let current_value = current_metrics.get(&gate.metric)?;
                let baseline_value = baselines.get_baseline(&gate.metric)?;

                let result = RegressionCheckResult::new(
                    gate.gate_id.to_string(),
                    gate.metric.clone(),
                    *current_value,
                    baseline_value,
                    gate.threshold_pct,
                    gate.blocking,
                    gate.test_category.clone(),
                );

                Some(result)
            })
            .collect();

        RegressionCheckResults::new(results)
    }

    /// Check a specific gate
    pub fn check_gate(
        &self,
        gate_id: &RegressionGateId,
        current_value: f64,
        baseline: &Baseline,
    ) -> Option<RegressionCheckResult> {
        let gate = self.gates.iter().find(|g| g.gate_id == *gate_id)?;
        let baseline_value = baseline.get_baseline(&gate.metric)?;

        Some(RegressionCheckResult::new(
            gate.gate_id.to_string(),
            gate.metric.clone(),
            current_value,
            baseline_value,
            gate.threshold_pct,
            gate.blocking,
            gate.test_category.clone(),
        ))
    }
}

impl Default for RegressionChecker {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_delta_calculation_increase() {
        // 10% increase at 10% threshold should pass (threshold is boundary)
        let result = RegressionCheckResult::new(
            "test_gate".to_string(),
            "p95_latency_ms".to_string(),
            110.0,
            100.0,
            10.0,
            true,
            "extended".to_string(),
        );

        assert!(result.passed);
        assert!((result.delta_pct - 10.0).abs() < 0.01);
    }

    #[test]
    fn test_delta_calculation_decrease() {
        // 5% decrease at 5% threshold should pass
        let result = RegressionCheckResult::new(
            "test_gate".to_string(),
            "qps".to_string(),
            95.0,
            100.0,
            5.0,
            true,
            "extended".to_string(),
        );

        assert!(result.passed);
        assert!((result.delta_pct - (-5.0)).abs() < 0.01);
    }

    #[test]
    fn test_within_threshold_passes() {
        // 9% increase at 10% threshold should pass
        let result = RegressionCheckResult::new(
            "test_gate".to_string(),
            "p95_latency_ms".to_string(),
            109.0,
            100.0,
            10.0,
            true,
            "extended".to_string(),
        );

        assert!(result.passed);
    }

    #[test]
    fn test_regression_checker_creation() {
        let checker = RegressionChecker::new();
        assert!(!checker.gates().is_empty());
        assert_eq!(checker.gates().len(), 6); // 6 gates from matrix
    }

    #[test]
    fn test_regression_check_results_blocking() {
        let result1 = RegressionCheckResult::new(
            "gate1".to_string(),
            "p95_latency_ms".to_string(),
            110.0,
            100.0,
            5.0,
            true,
            "extended".to_string(),
        );

        let result2 = RegressionCheckResult::new(
            "gate2".to_string(),
            "cache_memory_bytes".to_string(),
            110.0,
            100.0,
            5.0,
            false, // Not blocking
            "extended".to_string(),
        );

        let results = RegressionCheckResults::new(vec![result1, result2]);
        assert!(!results.passed);
        assert_eq!(results.blocking_failures.len(), 1);
        assert!(results.blocking_failures[0].contains("gate1"));
    }
}

//! Baseline Storage and Comparison
//!
//! Baseline storage (JSON/CBOR) and metric comparison logic.
//! Implements the baseline retrieval and delta calculation mechanisms.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::path::Path;

/// Metric value enum for type-safe metric storage
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum MetricValue {
    Latency(f64), // milliseconds
    Throughput(f64), // queries per second
    Memory(f64), // bytes
    HitRate(f64), // percentage (0-100)
    Count(f64), // generic count
}

impl MetricValue {
    /// Convert to f64 for comparison
    pub fn as_f64(self) -> f64 {
        match self {
            Self::Latency(v) | Self::Throughput(v) | Self::Memory(v) | Self::HitRate(v) | Self::Count(v) => v,
        }
    }
}

/// Baseline storage container
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Baseline {
    pub commit_hash: Option<String>,
    pub timestamp_iso8601: String,
    pub metrics: HashMap<String, f64>,
}

impl Baseline {
    /// Create a new baseline
    pub fn new(timestamp_iso8601: String) -> Self {
        Self {
            commit_hash: None,
            timestamp_iso8601,
            metrics: HashMap::new(),
        }
    }

    /// Create with commit hash
    pub fn with_commit(mut self, commit_hash: String) -> Self {
        self.commit_hash = Some(commit_hash);
        self
    }

    /// Add a metric to the baseline
    pub fn with_metric(mut self, name: String, value: f64) -> Self {
        self.metrics.insert(name, value);
        self
    }

    /// Add multiple metrics
    pub fn with_metrics(mut self, metrics: HashMap<String, f64>) -> Self {
        self.metrics.extend(metrics);
        self
    }

    /// Get a baseline value for a metric
    pub fn get_baseline(&self, metric: &str) -> Option<f64> {
        self.metrics.get(metric).copied()
    }

    /// Save baseline to JSON file
    pub fn save_to_json(&self, path: &Path) -> Result<(), std::io::Error> {
        let json = serde_json::to_string_pretty(self)?;
        fs::write(path, json)?;
        Ok(())
    }

    /// Load baseline from JSON file
    pub fn load_from_json(path: &Path) -> Result<Self, Box<dyn std::error::Error>> {
        let content = fs::read_to_string(path)?;
        let baseline: Baseline = serde_json::from_str(&content)?;
        Ok(baseline)
    }

    /// Save baseline to CBOR file
    pub fn save_to_cbor(&self, path: &Path) -> Result<(), Box<dyn std::error::Error>> {
        let file = fs::File::create(path)?;
        ciborium::into_writer(self, file)?;
        Ok(())
    }

    /// Load baseline from CBOR file
    pub fn load_from_cbor(path: &Path) -> Result<Self, Box<dyn std::error::Error>> {
        let file = fs::File::open(path)?;
        let baseline = ciborium::from_reader(file)?;
        Ok(baseline)
    }
}

/// Baseline comparison result
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BaselineComparison {
    pub metric: String,
    pub current_value: f64,
    pub baseline_value: f64,
    pub delta_pct: f64,
    pub delta_absolute: f64,
}

impl BaselineComparison {
    /// Calculate comparison between current and baseline values
    pub fn new(metric: String, current_value: f64, baseline_value: f64) -> Self {
        let delta_absolute = current_value - baseline_value;
        let delta_pct = (delta_absolute / baseline_value) * 100.0;

        Self {
            metric,
            current_value,
            baseline_value,
            delta_pct,
            delta_absolute,
        }
    }

    /// Check if delta exceeds threshold (used for regression detection)
    pub fn exceeds_threshold(&self, threshold_pct: f64) -> bool {
        self.delta_pct.abs() > threshold_pct
    }
}

/// Baseline comparator (helper for bulk comparisons)
pub struct BaselineComparator {
    current_baseline: Baseline,
}

impl BaselineComparator {
    /// Create a new comparator
    pub fn new(current_baseline: Baseline) -> Self {
        Self { current_baseline }
    }

    /// Compare current metrics against baseline
    pub fn compare(
        &self,
        current_metrics: &HashMap<String, f64>,
    ) -> HashMap<String, BaselineComparison> {
        let mut comparisons = HashMap::new();

        for (metric_name, current_value) in current_metrics {
            if let Some(baseline_value) = self.current_baseline.get_baseline(metric_name) {
                let comparison = BaselineComparison::new(
                    metric_name.clone(),
                    *current_value,
                    baseline_value,
                );
                comparisons.insert(metric_name.clone(), comparison);
            }
        }

        comparisons
    }

    /// Summarize comparisons
    pub fn summarize_comparisons(
        &self,
        comparisons: &HashMap<String, BaselineComparison>,
    ) -> String {
        let mut summary = String::new();
        summary.push_str("Baseline Comparison Summary:\n");

        for (metric, comparison) in comparisons {
            summary.push_str(&format!(
                "  {}: {:.2}% change ({:.2} -> {:.2})\n",
                metric, comparison.delta_pct, comparison.baseline_value, comparison.current_value
            ));
        }

        summary
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_metric_value_as_f64() {
        assert_eq!(MetricValue::Latency(100.5).as_f64(), 100.5);
        assert_eq!(MetricValue::Throughput(1000.0).as_f64(), 1000.0);
        assert_eq!(MetricValue::Memory(1048576.0).as_f64(), 1048576.0);
    }

    #[test]
    fn test_baseline_creation() {
        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string());
        assert!(baseline.commit_hash.is_none());
        assert!(baseline.metrics.is_empty());
    }

    #[test]
    fn test_baseline_with_metrics() {
        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_metric("p95_latency_ms".to_string(), 100.0)
            .with_metric("qps".to_string(), 1000.0);

        assert_eq!(baseline.get_baseline("p95_latency_ms"), Some(100.0));
        assert_eq!(baseline.get_baseline("qps"), Some(1000.0));
    }

    #[test]
    fn test_baseline_with_commit() {
        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_commit("abc123def456".to_string());

        assert_eq!(baseline.commit_hash, Some("abc123def456".to_string()));
    }

    #[test]
    fn test_baseline_comparison_increase() {
        // 10% increase
        let comparison = BaselineComparison::new("p95_latency_ms".to_string(), 110.0, 100.0);
        assert!((comparison.delta_pct - 10.0).abs() < 0.01);
        assert_eq!(comparison.delta_absolute, 10.0);
    }

    #[test]
    fn test_baseline_comparison_decrease() {
        // 5% decrease
        let comparison = BaselineComparison::new("qps".to_string(), 950.0, 1000.0);
        assert!((comparison.delta_pct - (-5.0)).abs() < 0.01);
        assert_eq!(comparison.delta_absolute, -50.0);
    }

    #[test]
    fn test_baseline_comparison_exceeds_threshold() {
        let comparison = BaselineComparison::new("metric".to_string(), 110.0, 100.0);
        assert!(comparison.exceeds_threshold(5.0));
        assert!(!comparison.exceeds_threshold(15.0));
    }

    #[test]
    fn test_baseline_json_serialization() {
        let original = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_metric("p95_latency_ms".to_string(), 100.0)
            .with_commit("abc123".to_string());

        let json = serde_json::to_string(&original).unwrap();
        let deserialized: Baseline = serde_json::from_str(&json).unwrap();

        assert_eq!(original.commit_hash, deserialized.commit_hash);
        assert_eq!(original.get_baseline("p95_latency_ms"), deserialized.get_baseline("p95_latency_ms"));
    }

    #[test]
    fn test_baseline_comparator() {
        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_metric("p95_latency_ms".to_string(), 100.0)
            .with_metric("qps".to_string(), 1000.0);

        let comparator = BaselineComparator::new(baseline);

        let mut current = HashMap::new();
        current.insert("p95_latency_ms".to_string(), 110.0);
        current.insert("qps".to_string(), 950.0);

        let comparisons = comparator.compare(&current);

        assert_eq!(comparisons.len(), 2);
        assert!((comparisons.get("p95_latency_ms").unwrap().delta_pct - 10.0).abs() < 0.01);
        assert!((comparisons.get("qps").unwrap().delta_pct - (-5.0)).abs() < 0.01);
    }

    #[test]
    fn test_baseline_comparator_summary() {
        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_metric("p95_latency_ms".to_string(), 100.0);

        let comparator = BaselineComparator::new(baseline);

        let mut current = HashMap::new();
        current.insert("p95_latency_ms".to_string(), 110.0);

        let comparisons = comparator.compare(&current);
        let summary = comparator.summarize_comparisons(&comparisons);

        assert!(summary.contains("10.00%"));
        assert!(summary.contains("100.00"));
        assert!(summary.contains("110.00"));
    }

    #[test]
    fn test_baseline_with_multiple_metrics() {
        let mut metrics = HashMap::new();
        metrics.insert("p95_latency_ms".to_string(), 100.0);
        metrics.insert("p99_latency_ms".to_string(), 150.0);
        metrics.insert("qps".to_string(), 1000.0);
        metrics.insert("cache_hit_rate_pct".to_string(), 85.0);

        let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
            .with_metrics(metrics);

        assert_eq!(baseline.metrics.len(), 4);
        assert_eq!(baseline.get_baseline("p99_latency_ms"), Some(150.0));
        assert_eq!(baseline.get_baseline("cache_hit_rate_pct"), Some(85.0));
    }
}

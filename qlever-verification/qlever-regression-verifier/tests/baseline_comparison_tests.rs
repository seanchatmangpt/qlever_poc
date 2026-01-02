//! Baseline comparison tests
//!
//! Unit tests for baseline storage, loading, and comparison logic.

use qlever_regression_verifier::baseline::{Baseline, BaselineComparison, BaselineComparator, MetricValue};
use std::collections::HashMap;
use tempfile::TempDir;

/// Test fixture: Create a temporary directory for test files
fn temp_dir() -> TempDir {
    TempDir::new().unwrap()
}

#[test]
fn test_baseline_creation_with_metrics() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    assert_eq!(baseline.get_baseline("p95_latency_ms"), Some(100.0));
    assert_eq!(baseline.get_baseline("qps"), Some(1000.0));
    assert_eq!(baseline.get_baseline("nonexistent"), None);
}

#[test]
fn test_baseline_with_commit_hash() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_commit("abc123def456".to_string());

    assert_eq!(baseline.commit_hash, Some("abc123def456".to_string()));
}

#[test]
fn test_baseline_delta_calculation_positive() {
    // 10% increase: (110 - 100) / 100 * 100 = 10%
    let comparison = BaselineComparison::new("metric".to_string(), 110.0, 100.0);
    assert!((comparison.delta_pct - 10.0).abs() < 0.01);
    assert_eq!(comparison.delta_absolute, 10.0);
}

#[test]
fn test_baseline_delta_calculation_negative() {
    // 5% decrease: (95 - 100) / 100 * 100 = -5%
    let comparison = BaselineComparison::new("metric".to_string(), 95.0, 100.0);
    assert!((comparison.delta_pct - (-5.0)).abs() < 0.01);
    assert_eq!(comparison.delta_absolute, -5.0);
}

#[test]
fn test_baseline_delta_calculation_zero() {
    // No change
    let comparison = BaselineComparison::new("metric".to_string(), 100.0, 100.0);
    assert_eq!(comparison.delta_pct, 0.0);
    assert_eq!(comparison.delta_absolute, 0.0);
}

#[test]
fn test_baseline_comparison_threshold_pass() {
    let comparison = BaselineComparison::new("metric".to_string(), 109.0, 100.0);
    assert!(!comparison.exceeds_threshold(10.0)); // 9% > 10%? No
    assert!(comparison.exceeds_threshold(8.0)); // 9% > 8%? Yes
}

#[test]
fn test_baseline_comparison_threshold_fail() {
    let comparison = BaselineComparison::new("metric".to_string(), 115.0, 100.0);
    assert!(comparison.exceeds_threshold(10.0)); // 15% > 10%? Yes
}

#[test]
fn test_baseline_json_roundtrip() {
    let original = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_commit("abc123def456".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    let json = serde_json::to_string(&original).unwrap();
    let deserialized: Baseline = serde_json::from_str(&json).unwrap();

    assert_eq!(original.commit_hash, deserialized.commit_hash);
    assert_eq!(original.timestamp_iso8601, deserialized.timestamp_iso8601);
    assert_eq!(original.get_baseline("p95_latency_ms"), deserialized.get_baseline("p95_latency_ms"));
    assert_eq!(original.get_baseline("qps"), deserialized.get_baseline("qps"));
}

#[test]
fn test_baseline_save_and_load_json() {
    let temp = temp_dir();
    let baseline_path = temp.path().join("baseline.json");

    let original = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_commit("abc123def456".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    // Save to JSON
    original.save_to_json(&baseline_path).unwrap();
    assert!(baseline_path.exists());

    // Load from JSON
    let loaded = Baseline::load_from_json(&baseline_path).unwrap();

    assert_eq!(original.commit_hash, loaded.commit_hash);
    assert_eq!(original.get_baseline("p95_latency_ms"), loaded.get_baseline("p95_latency_ms"));
}

#[test]
fn test_baseline_save_and_load_cbor() {
    let temp = temp_dir();
    let baseline_path = temp.path().join("baseline.cbor");

    let original = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_commit("abc123def456".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    // Save to CBOR
    original.save_to_cbor(&baseline_path).unwrap();
    assert!(baseline_path.exists());

    // Load from CBOR
    let loaded = Baseline::load_from_cbor(&baseline_path).unwrap();

    assert_eq!(original.commit_hash, loaded.commit_hash);
    assert_eq!(original.get_baseline("p95_latency_ms"), loaded.get_baseline("p95_latency_ms"));
}

#[test]
fn test_baseline_comparator_single_metric() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0);

    let comparator = BaselineComparator::new(baseline);

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 110.0);

    let comparisons = comparator.compare(&current);

    assert_eq!(comparisons.len(), 1);
    let comparison = comparisons.get("p95_latency_ms").unwrap();
    assert!((comparison.delta_pct - 10.0).abs() < 0.01);
}

#[test]
fn test_baseline_comparator_multiple_metrics() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0)
        .with_metric("cache_hit_rate_pct".to_string(), 85.0);

    let comparator = BaselineComparator::new(baseline);

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 110.0); // 10% increase
    current.insert("qps".to_string(), 950.0); // 5% decrease
    current.insert("cache_hit_rate_pct".to_string(), 85.0); // No change

    let comparisons = comparator.compare(&current);

    assert_eq!(comparisons.len(), 3);
    assert!((comparisons["p95_latency_ms"].delta_pct - 10.0).abs() < 0.01);
    assert!((comparisons["qps"].delta_pct - (-5.0)).abs() < 0.01);
    assert_eq!(comparisons["cache_hit_rate_pct"].delta_pct, 0.0);
}

#[test]
fn test_baseline_comparator_missing_metrics() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    let comparator = BaselineComparator::new(baseline);

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 110.0);
    current.insert("unknown_metric".to_string(), 42.0);

    let comparisons = comparator.compare(&current);

    // Only metrics that exist in baseline are included
    assert_eq!(comparisons.len(), 1);
    assert!(comparisons.contains_key("p95_latency_ms"));
    assert!(!comparisons.contains_key("unknown_metric"));
}

#[test]
fn test_baseline_comparator_summary() {
    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metric("p95_latency_ms".to_string(), 100.0)
        .with_metric("qps".to_string(), 1000.0);

    let comparator = BaselineComparator::new(baseline);

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 110.0);
    current.insert("qps".to_string(), 950.0);

    let comparisons = comparator.compare(&current);
    let summary = comparator.summarize_comparisons(&comparisons);

    assert!(summary.contains("10.00%"));
    assert!(summary.contains("-5.00%"));
    assert!(summary.contains("100.00"));
    assert!(summary.contains("110.00"));
}

#[test]
fn test_metric_value_conversions() {
    assert_eq!(MetricValue::Latency(100.5).as_f64(), 100.5);
    assert_eq!(MetricValue::Throughput(1000.0).as_f64(), 1000.0);
    assert_eq!(MetricValue::Memory(1048576.0).as_f64(), 1048576.0);
    assert_eq!(MetricValue::HitRate(85.5).as_f64(), 85.5);
    assert_eq!(MetricValue::Count(42.0).as_f64(), 42.0);
}

#[test]
fn test_baseline_large_percentage_changes() {
    // Large percentage increase (100% increase = 2x)
    let comparison = BaselineComparison::new("metric".to_string(), 200.0, 100.0);
    assert_eq!(comparison.delta_pct, 100.0);

    // Large percentage decrease (50% decrease = 0.5x)
    let comparison = BaselineComparison::new("metric".to_string(), 50.0, 100.0);
    assert_eq!(comparison.delta_pct, -50.0);
}

#[test]
fn test_baseline_small_percentage_changes() {
    // Small percentage increase (0.1%)
    let comparison = BaselineComparison::new("metric".to_string(), 100.1, 100.0);
    assert!((comparison.delta_pct - 0.1).abs() < 0.01);

    // Small percentage decrease (0.01%)
    let comparison = BaselineComparison::new("metric".to_string(), 99.99, 100.0);
    assert!((comparison.delta_pct - (-0.01)).abs() < 0.01);
}

#[test]
fn test_baseline_with_bulk_metrics() {
    let mut metrics = HashMap::new();
    metrics.insert("p95_latency_ms".to_string(), 100.0);
    metrics.insert("p99_latency_ms".to_string(), 150.0);
    metrics.insert("qps".to_string(), 1000.0);
    metrics.insert("cache_hit_rate_pct".to_string(), 85.0);
    metrics.insert("cache_memory_bytes".to_string(), 1048576.0);

    let baseline = Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_metrics(metrics);

    assert_eq!(baseline.metrics.len(), 5);
    assert_eq!(baseline.get_baseline("p99_latency_ms"), Some(150.0));
    assert_eq!(baseline.get_baseline("cache_memory_bytes"), Some(1048576.0));
}

#[test]
fn test_baseline_comparison_asymmetry() {
    // Percentage changes are asymmetric due to different bases
    let comp1 = BaselineComparison::new("metric".to_string(), 110.0, 100.0); // 10% increase
    let comp2 = BaselineComparison::new("metric".to_string(), 100.0, 110.0); // ~9.09% decrease

    // 110 vs 100: 10% increase
    assert!((comp1.delta_pct - 10.0).abs() < 0.01);

    // 100 vs 110: ~9.09% decrease
    assert!((comp2.delta_pct - (-9.09)).abs() < 0.1);
}

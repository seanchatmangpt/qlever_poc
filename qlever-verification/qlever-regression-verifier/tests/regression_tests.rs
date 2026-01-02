//! Regression tests for all gate types
//!
//! Tests p95 latency, p99 latency, QPS, cache hit rate, memory, and SIMD equivalence gates.

use qlever_regression_verifier::RegressionChecker;
use qlever_regression_verifier::baseline::Baseline;
use std::collections::HashMap;

/// Test fixture: Create baseline with standard metrics
fn create_baseline() -> Baseline {
    Baseline::new("2026-01-02T10:00:00Z".to_string())
        .with_commit("abc123def456".to_string())
        .with_metrics({
            let mut metrics = HashMap::new();
            metrics.insert("p95_latency_ms".to_string(), 100.0);
            metrics.insert("p99_latency_ms".to_string(), 150.0);
            metrics.insert("qps".to_string(), 1000.0);
            metrics.insert("cache_hit_rate_pct".to_string(), 85.0);
            metrics.insert("cache_memory_bytes".to_string(), 1048576.0); // 1 MB
            metrics.insert("digest_equality".to_string(), 1.0); // Perfect equality
            metrics
        })
}

#[test]
fn test_regression_detection_latency_p95_pass() {
    // p95 latency increases 5% (under 10% threshold) - should PASS
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 105.0); // 5% increase
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let p95_result = results.results.iter().find(|r| r.gate_id == "p95_latency_regression").unwrap();
    assert!(p95_result.passed);
    assert!((p95_result.delta_pct - 5.0).abs() < 0.01);
}

#[test]
fn test_regression_detection_latency_p95_fail() {
    // p95 latency increases 15% (over 10% threshold) - should FAIL
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 115.0); // 15% increase
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let p95_result = results.results.iter().find(|r| r.gate_id == "p95_latency_regression").unwrap();
    assert!(!p95_result.passed);
    assert!((p95_result.delta_pct - 15.0).abs() < 0.01);
}

#[test]
fn test_regression_detection_latency_p99() {
    // p99 latency increases 16% (over 15% threshold) - should FAIL
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 174.0); // 16% increase
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let p99_result = results.results.iter().find(|r| r.gate_id == "p99_latency_regression").unwrap();
    assert!(!p99_result.passed);
    assert!((p99_result.delta_pct - 16.0).abs() < 0.01);
}

#[test]
fn test_regression_detection_qps() {
    // QPS decreases 3% (under 5% threshold) - should PASS
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 970.0); // 3% decrease
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let qps_result = results.results.iter().find(|r| r.gate_id == "qps_regression").unwrap();
    assert!(qps_result.passed);
    assert!((qps_result.delta_pct - (-3.0)).abs() < 0.01);
}

#[test]
fn test_regression_detection_qps_fail() {
    // QPS decreases 6% (over 5% threshold) - should FAIL
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 940.0); // 6% decrease
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let qps_result = results.results.iter().find(|r| r.gate_id == "qps_regression").unwrap();
    assert!(!qps_result.passed);
    assert!((qps_result.delta_pct - (-6.0)).abs() < 0.01);
}

#[test]
fn test_regression_detection_memory() {
    // Memory increases 19% (under 20% threshold) - should PASS (but advisory)
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1248546.56); // 19% increase
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let memory_result = results.results.iter().find(|r| r.gate_id == "cache_memory_regression").unwrap();
    assert!(memory_result.passed);
    assert!(!memory_result.blocking); // Advisory only
}

#[test]
fn test_regression_detection_memory_fail() {
    // Memory increases 25% (over 20% threshold) - should FAIL (advisory)
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1310720.0); // 25% increase
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let memory_result = results.results.iter().find(|r| r.gate_id == "cache_memory_regression").unwrap();
    assert!(!memory_result.passed);
    assert!(!memory_result.blocking); // Advisory only (not blocking)
}

#[test]
fn test_regression_detection_cache_hit_rate() {
    // Cache hit rate decreases 4% (under 5% threshold) - should PASS
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 81.6); // 4% decrease
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let hit_rate_result = results.results.iter().find(|r| r.gate_id == "cache_hit_rate_regression").unwrap();
    assert!(hit_rate_result.passed);
}

#[test]
fn test_regression_detection_cache_hit_rate_fail() {
    // Cache hit rate decreases 6% (over 5% threshold) - should FAIL
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 79.9); // 6% decrease
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0);

    let results = checker.check_regressions(&current, &baseline);

    let hit_rate_result = results.results.iter().find(|r| r.gate_id == "cache_hit_rate_regression").unwrap();
    assert!(!hit_rate_result.passed);
}

#[test]
fn test_regression_detection_simd_equivalence_pass() {
    // SIMD digests are identical (perfect equality) - should PASS
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 1.0); // Perfect equality

    let results = checker.check_regressions(&current, &baseline);

    let simd_result = results.results.iter().find(|r| r.gate_id == "simd_equivalence_avx512_vs_scalar").unwrap();
    assert!(simd_result.passed);
    assert_eq!(simd_result.delta_pct, 0.0);
}

#[test]
fn test_regression_detection_simd_equivalence_fail() {
    // SIMD digests diverge even slightly (zero tolerance) - should FAIL
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0);
    current.insert("p99_latency_ms".to_string(), 150.0);
    current.insert("qps".to_string(), 1000.0);
    current.insert("cache_hit_rate_pct".to_string(), 85.0);
    current.insert("cache_memory_bytes".to_string(), 1048576.0);
    current.insert("digest_equality".to_string(), 0.0); // Divergence!

    let results = checker.check_regressions(&current, &baseline);

    let simd_result = results.results.iter().find(|r| r.gate_id == "simd_equivalence_avx512_vs_scalar").unwrap();
    assert!(!simd_result.passed);
    assert_eq!(simd_result.blocking, true); // SIMD is blocking
}

#[test]
fn test_regression_all_gates_simultaneously() {
    // Test all 6 gates together
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 100.0); // PASS
    current.insert("p99_latency_ms".to_string(), 150.0); // PASS
    current.insert("qps".to_string(), 1000.0); // PASS
    current.insert("cache_hit_rate_pct".to_string(), 85.0); // PASS
    current.insert("cache_memory_bytes".to_string(), 1048576.0); // PASS
    current.insert("digest_equality".to_string(), 1.0); // PASS

    let results = checker.check_regressions(&current, &baseline);

    assert_eq!(results.results.len(), 6);
    assert!(results.passed);
    assert_eq!(results.blocking_failures.len(), 0);
}

#[test]
fn test_regression_mixed_pass_fail() {
    // Some gates pass, some fail
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 115.0); // FAIL (15% > 10%)
    current.insert("p99_latency_ms".to_string(), 150.0); // PASS
    current.insert("qps".to_string(), 1000.0); // PASS
    current.insert("cache_hit_rate_pct".to_string(), 85.0); // PASS
    current.insert("cache_memory_bytes".to_string(), 1310720.0); // FAIL but non-blocking
    current.insert("digest_equality".to_string(), 1.0); // PASS

    let results = checker.check_regressions(&current, &baseline);

    assert_eq!(results.results.len(), 6);
    assert!(!results.passed);
    assert_eq!(results.blocking_failures.len(), 1); // Only p95 is blocking
}

#[test]
fn test_regression_check_receipts_generation() {
    // Test that receipts are generated for failures
    let checker = RegressionChecker::new();
    let baseline = create_baseline();

    let mut current = HashMap::new();
    current.insert("p95_latency_ms".to_string(), 115.0); // FAIL
    current.insert("p99_latency_ms".to_string(), 150.0); // PASS
    current.insert("qps".to_string(), 1000.0); // PASS
    current.insert("cache_hit_rate_pct".to_string(), 85.0); // PASS
    current.insert("cache_memory_bytes".to_string(), 1048576.0); // PASS
    current.insert("digest_equality".to_string(), 1.0); // PASS

    let results = checker.check_regressions(&current, &baseline);
    let receipts = results.generate_receipts();

    assert_eq!(receipts.len(), 1); // One failure
    assert!(receipts[0].is_blocking);
}

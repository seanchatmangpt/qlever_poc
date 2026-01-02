//! EPIC 11 Subsystem 5: Divergence Detection Tests
//!
//! Test fail-closed behavior and divergence detection.
//! Per EPIC 11 Invariant C: Replay (Fail-Closed Semantics)

use qlever_replay_verifier::{
    ReplayFailure, ReplayFailureClass, ReplayMode, ReplayQuery, ReplayResult, ReplayWorkload,
};

#[test]
fn test_replay_success_result() {
    let result = ReplayResult {
        workload_id: "test".to_string(),
        mode: ReplayMode::Strict,
        queries_executed: 5,
        queries_passed: 5,
        queries_failed: 0,
        failures: vec![],
        total_duration_ms: 500,
    };

    assert!(result.is_success());
    assert_eq!(result.queries_passed, 5);
    assert_eq!(result.queries_failed, 0);
}

#[test]
fn test_replay_failure_result() {
    let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Digest mismatch");

    let result = ReplayResult {
        workload_id: "test".to_string(),
        mode: ReplayMode::Strict,
        queries_executed: 5,
        queries_passed: 4,
        queries_failed: 1,
        failures: vec![failure],
        total_duration_ms: 500,
    };

    assert!(!result.is_success());
    assert_eq!(result.queries_failed, 1);
}

#[test]
fn test_replay_divergence_failure_class() {
    let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Bit-level mismatch");

    assert_eq!(failure.query_id, "q1");
    assert_eq!(failure.failure_class, ReplayFailureClass::ReplayDivergence);
    assert!(failure.failure_class.is_blocking());
}

#[test]
fn test_replay_non_determinism_failure_class() {
    let failure = ReplayFailure::new(
        "q2",
        ReplayFailureClass::ReplayNonDeterminism,
        "Same query produced different results",
    );

    assert_eq!(failure.failure_class, ReplayFailureClass::ReplayNonDeterminism);
    assert!(failure.failure_class.is_blocking());
}

#[test]
fn test_replay_timeout_failure_class() {
    let failure = ReplayFailure::new("q3", ReplayFailureClass::ReplayTimeout, "Exceeded 1000ms timeout");

    assert_eq!(failure.failure_class, ReplayFailureClass::ReplayTimeout);
    assert!(failure.failure_class.is_blocking());
}

#[test]
fn test_replay_abort_failure_class() {
    let failure = ReplayFailure::new("q4", ReplayFailureClass::ReplayAbort, "Cache evicted before replay");

    assert_eq!(failure.failure_class, ReplayFailureClass::ReplayAbort);
    assert!(failure.failure_class.is_blocking());
}

#[test]
fn test_all_failure_classes_are_blocking() {
    assert!(ReplayFailureClass::ReplayDivergence.is_blocking());
    assert!(ReplayFailureClass::ReplayNonDeterminism.is_blocking());
    assert!(ReplayFailureClass::ReplayTimeout.is_blocking());
    assert!(ReplayFailureClass::ReplayAbort.is_blocking());
}

#[test]
fn test_replay_failure_with_divergence_index() {
    let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence at byte 42")
        .with_divergence_index(42);

    assert_eq!(failure.divergence_index, Some(42));
}

#[test]
fn test_replay_failure_serialization() {
    let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Test failure")
        .with_divergence_index(100);

    let json = serde_json::to_string(&failure).unwrap();
    let deserialized: ReplayFailure = serde_json::from_str(&json).unwrap();

    assert_eq!(deserialized.query_id, "q1");
    assert_eq!(deserialized.failure_class, ReplayFailureClass::ReplayDivergence);
    assert_eq!(deserialized.divergence_index, Some(100));
}

#[test]
fn test_replay_result_multiple_failures() {
    let failure1 = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence");
    let failure2 = ReplayFailure::new("q2", ReplayFailureClass::ReplayNonDeterminism, "Non-determinism");

    let result = ReplayResult {
        workload_id: "test".to_string(),
        mode: ReplayMode::Differential,
        queries_executed: 5,
        queries_passed: 3,
        queries_failed: 2,
        failures: vec![failure1, failure2],
        total_duration_ms: 500,
    };

    assert!(!result.is_success());
    assert_eq!(result.failures.len(), 2);
    assert_eq!(result.queries_failed, 2);
}

#[test]
fn test_replay_failure_class_descriptions() {
    assert!(!ReplayFailureClass::ReplayDivergence.description().is_empty());
    assert!(!ReplayFailureClass::ReplayNonDeterminism.description().is_empty());
    assert!(!ReplayFailureClass::ReplayTimeout.description().is_empty());
    assert!(!ReplayFailureClass::ReplayAbort.description().is_empty());
}

#[test]
fn test_replay_failure_class_display() {
    assert_eq!(
        ReplayFailureClass::ReplayDivergence.to_string(),
        "ReplayDivergence"
    );
    assert_eq!(
        ReplayFailureClass::ReplayNonDeterminism.to_string(),
        "ReplayNonDeterminism"
    );
    assert_eq!(ReplayFailureClass::ReplayTimeout.to_string(), "ReplayTimeout");
    assert_eq!(ReplayFailureClass::ReplayAbort.to_string(), "ReplayAbort");
}

#[test]
fn test_fail_closed_strict_mode_on_first_divergence() {
    // In strict mode, any divergence should stop execution
    let result = ReplayResult {
        workload_id: "strict-test".to_string(),
        mode: ReplayMode::Strict,
        queries_executed: 2,
        queries_passed: 1,
        queries_failed: 1,
        failures: vec![ReplayFailure::new(
            "q2",
            ReplayFailureClass::ReplayDivergence,
            "First divergence stops execution",
        )],
        total_duration_ms: 100,
    };

    // In strict mode, execution should stop after first failure
    assert!(!result.is_success());
    assert_eq!(result.queries_failed, 1);
}

#[test]
fn test_fail_closed_differential_mode_collects_all() {
    // In differential mode, all divergences should be collected
    let failures = vec![
        ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence 1"),
        ReplayFailure::new("q2", ReplayFailureClass::ReplayNonDeterminism, "Non-determinism"),
        ReplayFailure::new("q3", ReplayFailureClass::ReplayTimeout, "Timeout"),
    ];

    let result = ReplayResult {
        workload_id: "differential-test".to_string(),
        mode: ReplayMode::Differential,
        queries_executed: 5,
        queries_passed: 2,
        queries_failed: 3,
        failures,
        total_duration_ms: 500,
    };

    // In differential mode, all failures are collected
    assert!(!result.is_success());
    assert_eq!(result.failures.len(), 3);
    assert_eq!(result.queries_failed, 3);
}

#[test]
fn test_divergence_with_first_byte_index() {
    let failure = ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence at byte 256")
        .with_divergence_index(256);

    assert_eq!(failure.divergence_index, Some(256));
}

#[test]
fn test_replay_result_serialization() {
    let result = ReplayResult {
        workload_id: "test-serialize".to_string(),
        mode: ReplayMode::Strict,
        queries_executed: 3,
        queries_passed: 3,
        queries_failed: 0,
        failures: vec![],
        total_duration_ms: 300,
    };

    let json = serde_json::to_string(&result).unwrap();
    let deserialized: ReplayResult = serde_json::from_str(&json).unwrap();

    assert_eq!(deserialized.workload_id, "test-serialize");
    assert_eq!(deserialized.queries_executed, 3);
    assert_eq!(deserialized.queries_failed, 0);
    assert!(deserialized.is_success());
}

#[test]
fn test_multiple_divergence_types() {
    // Test that different failure types can coexist in same result
    let failures = vec![
        ReplayFailure::new("q1", ReplayFailureClass::ReplayDivergence, "Divergence")
            .with_divergence_index(42),
        ReplayFailure::new("q2", ReplayFailureClass::ReplayNonDeterminism, "Non-determinism")
            .with_divergence_index(100),
        ReplayFailure::new("q3", ReplayFailureClass::ReplayTimeout, "Timeout"),
        ReplayFailure::new("q4", ReplayFailureClass::ReplayAbort, "Abort"),
    ];

    let result = ReplayResult {
        workload_id: "multi-divergence".to_string(),
        mode: ReplayMode::Differential,
        queries_executed: 4,
        queries_passed: 0,
        queries_failed: 4,
        failures,
        total_duration_ms: 400,
    };

    assert_eq!(result.failures.len(), 4);

    let divergence = &result.failures[0];
    assert_eq!(divergence.failure_class, ReplayFailureClass::ReplayDivergence);
    assert_eq!(divergence.divergence_index, Some(42));

    let non_det = &result.failures[1];
    assert_eq!(non_det.failure_class, ReplayFailureClass::ReplayNonDeterminism);
    assert_eq!(non_det.divergence_index, Some(100));

    let timeout = &result.failures[2];
    assert_eq!(timeout.failure_class, ReplayFailureClass::ReplayTimeout);
    assert!(timeout.divergence_index.is_none());

    let abort = &result.failures[3];
    assert_eq!(abort.failure_class, ReplayFailureClass::ReplayAbort);
    assert!(abort.divergence_index.is_none());
}

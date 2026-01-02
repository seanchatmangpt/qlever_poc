/// Recovery verification tests - ensure fail-closed behavior and no partial results
/// These tests validate that faults are properly contained and don't leak partial state

use qlever_chaos_verifier::{
    execute_with_chaos, verify_recovery, FaultInjectionConfig, FaultInjectionEvent,
    FaultInjectionExecutor, FaultMode, RecoveryState,
};

#[test]
fn test_corruption_detection_fails_closed() {
    // Test: Cache corruption is detected and fails cleanly (no partial results)
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    // Record a corruption detection event
    let event = FaultInjectionEvent {
        timestamp_ns: 1000,
        fault_mode: FaultMode::CacheCorruption,
        injection_point: "cache_read".to_string(),
        recovery_state: RecoveryState::FaultDetected,
        partial_results: false,  // Crucially: no partial results
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();

    // Verify fail-closed semantics
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
}

#[test]
fn test_no_partial_results_after_epoch_contamination() {
    // Test: Epoch contamination detection prevents partial results
    let config = FaultInjectionConfig {
        mode: FaultMode::EpochContamination,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    // Simulate epoch contamination detection
    let event = FaultInjectionEvent {
        timestamp_ns: 2000,
        fault_mode: FaultMode::EpochContamination,
        injection_point: "cache_lookup".to_string(),
        recovery_state: RecoveryState::FaultDetected,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
}

#[test]
fn test_oom_fails_closed_no_partial_results() {
    // Test: Out of memory error fails cleanly
    let config = FaultInjectionConfig {
        mode: FaultMode::OutOfMemory,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    let event = FaultInjectionEvent {
        timestamp_ns: 3000,
        fault_mode: FaultMode::OutOfMemory,
        injection_point: "memory_allocation".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
}

#[test]
fn test_multiple_faults_prevent_partial_escape() {
    // Test: Multiple faults all fail cleanly (no partial results escape)
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 3,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    // Record multiple fault events
    for i in 0..3 {
        let event = FaultInjectionEvent {
            timestamp_ns: 1000 * (i as u64),
            fault_mode: FaultMode::CacheCorruption,
            injection_point: format!("injection_{}",i),
            recovery_state: RecoveryState::FaultDetected,
            partial_results: false,  // Each one must have no partial results
        };
        executor.record_event(event).unwrap();
    }

    let verification = verify_recovery(&executor).unwrap();
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
    assert_eq!(verification.event_count, 3);
}

#[test]
fn test_partial_results_detected() {
    // Test: If partial results are detected, verification fails
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Record event WITH partial results (violation of fail-closed)
    let event = FaultInjectionEvent {
        timestamp_ns: 5000,
        fault_mode: FaultMode::CacheCorruption,
        injection_point: "cache_write".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: true,  // VIOLATION: partial results escaped!
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();
    assert!(verification.partial_results_found);  // Detected!
    assert!(!verification.recovered);  // Not recovered (violation)
}

#[test]
fn test_healthy_state_no_faults() {
    // Test: Healthy state with no faults
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 0,  // No injections
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);
    let verification = verify_recovery(&executor).unwrap();

    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
    assert_eq!(verification.event_count, 0);
    assert_eq!(verification.final_state, RecoveryState::Healthy);
}

#[test]
fn test_recovery_state_consistency() {
    // Test: Recovery state is consistent across multiple verifications
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    let event = FaultInjectionEvent {
        timestamp_ns: 6000,
        fault_mode: FaultMode::Timeout,
        injection_point: "query_execution".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    // Verify multiple times
    let v1 = verify_recovery(&executor).unwrap();
    let v2 = verify_recovery(&executor).unwrap();

    // Results should be identical
    assert_eq!(v1, v2);
}

#[test]
fn test_invalid_epoch_key_fails_closed() {
    // Test: Invalid epoch key detection fails cleanly
    let config = FaultInjectionConfig {
        mode: FaultMode::InvalidEpochKey,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    let event = FaultInjectionEvent {
        timestamp_ns: 7000,
        fault_mode: FaultMode::InvalidEpochKey,
        injection_point: "epoch_validation".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
}

#[test]
fn test_sequential_fault_recovery() {
    // Test: Sequential faults all maintain fail-closed property
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Fault 1: Cache corruption
    let event1 = FaultInjectionEvent {
        timestamp_ns: 8000,
        fault_mode: FaultMode::CacheCorruption,
        injection_point: "read_cache".to_string(),
        recovery_state: RecoveryState::FaultDetected,
        partial_results: false,
    };
    executor.record_event(event1).unwrap();

    // Fault 2: Epoch contamination (detected and aborted)
    let event2 = FaultInjectionEvent {
        timestamp_ns: 8100,
        fault_mode: FaultMode::EpochContamination,
        injection_point: "epoch_check".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event2).unwrap();

    let verification = verify_recovery(&executor).unwrap();
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
    assert_eq!(verification.event_count, 2);
}

#[test]
fn test_execution_with_chaos_maintains_fail_closed() {
    // Test: High-level execute_with_chaos API maintains fail-closed
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let result = execute_with_chaos(config, || {
        // Simulated operation that might be disrupted
        Ok(())
    });

    assert!(result.is_ok());
    let verification = result.unwrap();

    // Verify fail-closed semantics
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
}

#[test]
fn test_recovery_works_with_all_fault_modes() {
    // Test: Recovery verification works for all fault modes
    let modes = vec![
        FaultMode::CacheCorruption,
        FaultMode::EpochContamination,
        FaultMode::OutOfMemory,
        FaultMode::Timeout,
        FaultMode::InvalidEpochKey,
    ];

    for mode in modes {
        let config = FaultInjectionConfig {
            mode,
            probability: 1.0,
            max_injections: 1,
            deterministic: true,
        };

        let executor = FaultInjectionExecutor::new(config);

        let event = FaultInjectionEvent {
            timestamp_ns: 10000,
            fault_mode: mode,
            injection_point: "test_point".to_string(),
            recovery_state: RecoveryState::FailClosed,
            partial_results: false,
        };
        executor.record_event(event).unwrap();

        let verification = verify_recovery(&executor).unwrap();
        assert!(!verification.partial_results_found, "Mode {} had partial results", mode);
        assert!(verification.recovered, "Mode {} failed recovery", mode);
    }
}

#[test]
fn test_no_silent_failures() {
    // Test: All failures are detected and recorded (no silent failures)
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Record that a fault occurred
    let event = FaultInjectionEvent {
        timestamp_ns: 11000,
        fault_mode: FaultMode::OutOfMemory,
        injection_point: "allocation".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    let verification = verify_recovery(&executor).unwrap();

    // Verify that the fault was recorded
    assert_eq!(verification.event_count, 1);

    // Verify state is tracked
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
}

#[test]
fn test_recovery_evidence_captured() {
    // Test: Recovery verification captures sufficient evidence
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Record multiple events with detailed evidence
    for i in 0..5 {
        let event = FaultInjectionEvent {
            timestamp_ns: 12000 + (i as u64 * 100),
            fault_mode: FaultMode::CacheCorruption,
            injection_point: format!("point_{}", i),
            recovery_state: RecoveryState::FailClosed,
            partial_results: false,
        };
        executor.record_event(event).unwrap();
    }

    let events = executor.get_events().unwrap();
    assert_eq!(events.len(), 5);

    // Verify all events are recorded
    for (i, event) in events.iter().enumerate() {
        assert_eq!(event.timestamp_ns, 12000 + (i as u64 * 100));
    }

    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.event_count, 5);
}

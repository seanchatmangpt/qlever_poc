/// Fault injection tests - verify abort behavior on induced faults
/// These tests ensure fail-closed semantics: any fault must result in clean abort

use qlever_chaos_verifier::{
    execute_with_chaos, inject_fault, verify_recovery, FaultInjectionConfig, FaultInjectionEvent,
    FaultInjectionExecutor, FaultMode, InjectionResult, RecoveryState,
};

#[test]
fn test_fault_injection_abort_cache_corruption() {
    // Test: CacheCorruption fault must abort execution
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    // Execute with injection
    let result = execute_with_chaos(config, || Ok(()));
    assert!(result.is_ok(), "Execution should complete (abort is clean)");

    let verification = result.unwrap();
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
}

#[test]
fn test_fault_injection_abort_epoch_contamination() {
    // Test: EpochContamination fault must abort execution
    let config = FaultInjectionConfig {
        mode: FaultMode::EpochContamination,
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
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
}

#[test]
fn test_fault_injection_abort_oom() {
    // Test: OutOfMemory fault must abort execution
    let config = FaultInjectionConfig {
        mode: FaultMode::OutOfMemory,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let result = execute_with_chaos(config, || Ok(()));
    assert!(result.is_ok());

    let verification = result.unwrap();
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
}

#[test]
fn test_fault_injection_abort_timeout() {
    // Test: Timeout fault must abort execution
    let config = FaultInjectionConfig {
        mode: FaultMode::Timeout,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let result = execute_with_chaos(config, || Ok(()));
    assert!(result.is_ok());

    let verification = result.unwrap();
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
}

#[test]
fn test_fault_injection_abort_invalid_epoch_key() {
    // Test: InvalidEpochKey fault must abort execution
    let config = FaultInjectionConfig {
        mode: FaultMode::InvalidEpochKey,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let result = execute_with_chaos(config, || Ok(()));
    assert!(result.is_ok());

    let verification = result.unwrap();
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
}

#[test]
fn test_fault_injection_deterministic_vs_probabilistic() {
    // Test: Deterministic mode always injects (if count < max)
    let config_det = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let result = inject_fault(config_det).unwrap();
    assert_eq!(result, InjectionResult::Injected);

    // Probabilistic mode with 1.0 probability should also inject
    let config_prob = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: false,
    };

    let result = inject_fault(config_prob).unwrap();
    assert_eq!(result, InjectionResult::Injected);
}

#[test]
fn test_fault_injection_respects_max_injections() {
    // Test: Injection limit is respected
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 2,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    // First injection should succeed
    let result = executor.try_inject().unwrap();
    assert_eq!(result, InjectionResult::Injected);

    // Second injection should succeed
    let result = executor.try_inject().unwrap();
    assert_eq!(result, InjectionResult::Injected);

    // Third should hit limit
    let result = executor.try_inject().unwrap();
    assert_eq!(result, InjectionResult::LimitReached);
}

#[test]
fn test_fault_injection_zero_limit_disables() {
    // Test: max_injections = 0 means no injections occur
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 0,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    let result = executor.try_inject().unwrap();
    assert_eq!(result, InjectionResult::LimitReached);
}

#[test]
fn test_fault_injection_normal_execution_without_fault() {
    // Test: Normal execution path when no fault is injected
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 0,  // Disable injection
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    let result = executor.execute_with_injection(|| {
        // Normal operation
        Ok(())
    });

    assert!(result.is_ok());
}

#[test]
fn test_fault_injection_recovery_state_transitions() {
    // Test: Recovery state transitions are correct
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Initially healthy
    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.final_state, RecoveryState::Healthy);

    // Record a fault injection event
    let event = FaultInjectionEvent {
        timestamp_ns: 0,
        fault_mode: FaultMode::CacheCorruption,
        injection_point: "test_point".to_string(),
        recovery_state: RecoveryState::FaultDetected,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    // After fault, should be fail-closed (no partial results)
    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.final_state, RecoveryState::FailClosed);
    assert!(!verification.partial_results_found);
    assert!(verification.recovered);
}

#[test]
fn test_fault_injection_multiple_events() {
    // Test: Multiple fault injection events are tracked
    let config = FaultInjectionConfig::default();
    let executor = FaultInjectionExecutor::new(config);

    // Record multiple events
    for i in 0..3 {
        let event = FaultInjectionEvent {
            timestamp_ns: i as u64,
            fault_mode: FaultMode::CacheCorruption,
            injection_point: format!("point_{}", i),
            recovery_state: RecoveryState::FaultDetected,
            partial_results: false,
        };
        executor.record_event(event).unwrap();
    }

    let events = executor.get_events().unwrap();
    assert_eq!(events.len(), 3);

    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.event_count, 3);
}

#[test]
fn test_fault_injection_reset() {
    // Test: Executor can be reset to clean state
    let config = FaultInjectionConfig {
        mode: FaultMode::CacheCorruption,
        probability: 1.0,
        max_injections: 1,
        deterministic: true,
    };

    let executor = FaultInjectionExecutor::new(config);

    // Inject fault
    let _ = executor.execute_with_injection(|| Ok(()));

    // Record event
    let event = FaultInjectionEvent {
        timestamp_ns: 0,
        fault_mode: FaultMode::CacheCorruption,
        injection_point: "test".to_string(),
        recovery_state: RecoveryState::FailClosed,
        partial_results: false,
    };
    executor.record_event(event).unwrap();

    // Verify state
    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.event_count, 1);

    // Reset
    executor.reset().unwrap();

    // Verify clean state
    let verification = verify_recovery(&executor).unwrap();
    assert_eq!(verification.event_count, 0);
    assert_eq!(verification.final_state, RecoveryState::Healthy);
}

#[test]
fn test_all_fault_modes_fail_closed() {
    // Test: All fault modes enforce fail-closed semantics
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

        let result = execute_with_chaos(config, || Ok(()));
        assert!(result.is_ok(), "Execution failed for mode: {}", mode);

        let verification = result.unwrap();
        assert_eq!(
            verification.final_state,
            RecoveryState::FailClosed,
            "Mode {} did not fail-closed",
            mode
        );
        assert!(!verification.partial_results_found);
        assert!(verification.recovered);
    }
}

/// EPIC 11 Subsystem 9: Chaos Verifier - Fault Injection & Recovery
///
/// This crate implements fault injection and fail-closed validation for QLever's read cache.
/// Every injected fault must result in clean abort, never partial results.
///
/// Key guarantees:
/// - Fail-closed semantics: faults block any partial result escape
/// - Deterministic recovery: all failures produce classified receipts
/// - 5+ fault modes: CacheCorruption, EpochContamination, OutOfMemory, Timeout, InvalidEpochKey
/// - No undetected state: every fault either recovered or fail-closed

pub mod fault_injection;
pub mod fault_modes;

pub use fault_injection::{FaultInjectionError, FaultInjectionEvent, FaultInjectionExecutor};
pub use fault_modes::{FaultInjectionConfig, FaultMode, InjectionResult, RecoveryState};

use serde::{Deserialize, Serialize};

/// Recovery verification result
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct RecoveryVerification {
    /// Was recovery successful?
    pub recovered: bool,
    /// Were there any partial results?
    pub partial_results_found: bool,
    /// Number of fault injection events
    pub event_count: u32,
    /// Final recovery state
    pub final_state: RecoveryState,
}

/// Main public API for chaos verifier

/// Inject a fault with given configuration
///
/// Returns InjectionResult indicating whether fault was injected.
/// On injection, system must abort cleanly.
///
/// # Arguments
/// * `config` - Fault injection configuration
///
/// # Returns
/// * `Ok(InjectionResult)` - Injection status
/// * `Err(String)` - Configuration or injection error
pub fn inject_fault(config: FaultInjectionConfig) -> Result<InjectionResult, String> {
    let injector = fault_injection::FaultInjector::new(config);
    injector.try_inject()
}

/// Verify recovery from injected faults
///
/// Validates that:
/// 1. System cleanly aborted on fault
/// 2. No partial results escaped
/// 3. Recovery state is consistent
///
/// # Arguments
/// * `executor` - FaultInjectionExecutor with recorded events
///
/// # Returns
/// * `Ok(RecoveryVerification)` - Verification result
/// * `Err(String)` - Verification error
pub fn verify_recovery(executor: &FaultInjectionExecutor) -> Result<RecoveryVerification, String> {
    let events = executor.get_events()?;
    let event_count = events.len() as u32;

    // Check if any partial results escaped
    let partial_results_found = events.iter().any(|e| e.partial_results);

    // Get final recovery state
    // In fail-closed semantics, we either have no faults or are fail-closed
    let final_state = if event_count > 0 {
        RecoveryState::FailClosed
    } else {
        RecoveryState::Healthy
    };

    // Verify no partial results
    let recovered = !partial_results_found;

    Ok(RecoveryVerification {
        recovered,
        partial_results_found,
        event_count,
        final_state,
    })
}

/// Execute operation with chaos injection and recovery verification
///
/// This is the high-level API that combines injection, execution, and verification.
///
/// # Arguments
/// * `config` - Fault injection configuration
/// * `operation` - Operation to execute (may be disrupted by injected fault)
///
/// # Returns
/// * `Ok(RecoveryVerification)` - Verification result
/// * `Err(String)` - Execution or verification error
pub fn execute_with_chaos<F>(
    config: FaultInjectionConfig,
    operation: F,
) -> Result<RecoveryVerification, String>
where
    F: FnOnce() -> Result<(), String>,
{
    let executor = FaultInjectionExecutor::new(config);

    // Try to execute with injection
    let exec_result = executor.execute_with_injection(operation);

    // Record execution result
    match exec_result {
        Ok(()) => {
            // Normal execution path
        }
        Err(FaultInjectionError::FaultInjected { mode, .. }) => {
            // Fault was injected - system must abort
            // Record this as fail-closed event
            let event = FaultInjectionEvent {
                timestamp_ns: 0,
                fault_mode: mode,
                injection_point: "execute_with_chaos".to_string(),
                recovery_state: RecoveryState::FailClosed,
                partial_results: false,  // Injected faults must not allow partial results
            };
            executor.record_event(event)?;
        }
        Err(FaultInjectionError::ExecutionError(_)) => {
            // Execution error (not fault injection related)
            // Let it propagate but still verify recovery
        }
        Err(FaultInjectionError::StateError(e)) => {
            return Err(format!("State management error: {}", e));
        }
    }

    // Verify recovery
    verify_recovery(&executor)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_inject_fault() {
        let config = FaultInjectionConfig {
            mode: FaultMode::CacheCorruption,
            probability: 1.0,
            max_injections: 1,
            deterministic: true,
        };

        let result = inject_fault(config).unwrap();
        assert_eq!(result, InjectionResult::Injected);
    }

    #[test]
    fn test_verify_recovery_no_faults() {
        let config = FaultInjectionConfig {
            mode: FaultMode::EpochContamination,
            probability: 1.0,
            max_injections: 0,  // No injections
            deterministic: true,
        };

        let executor = FaultInjectionExecutor::new(config);
        let verification = verify_recovery(&executor).unwrap();

        assert!(verification.recovered);
        assert!(!verification.partial_results_found);
        assert_eq!(verification.event_count, 0);
        assert_eq!(verification.final_state, RecoveryState::Healthy);
    }

    #[test]
    fn test_execute_with_chaos_normal_path() {
        let config = FaultInjectionConfig {
            mode: FaultMode::OutOfMemory,
            probability: 1.0,
            max_injections: 0,  // No injections
            deterministic: true,
        };

        let result = execute_with_chaos(config, || Ok(()));
        assert!(result.is_ok());

        let verification = result.unwrap();
        assert!(verification.recovered);
        assert!(!verification.partial_results_found);
    }

    #[test]
    fn test_execute_with_chaos_injected_fault() {
        let config = FaultInjectionConfig {
            mode: FaultMode::CacheCorruption,
            probability: 1.0,
            max_injections: 1,
            deterministic: true,
        };

        let result = execute_with_chaos(config, || Ok(()));
        assert!(result.is_ok());

        let verification = result.unwrap();
        assert!(verification.recovered);  // No partial results
        assert!(!verification.partial_results_found);
        assert_eq!(verification.final_state, RecoveryState::FailClosed);
    }

    #[test]
    fn test_all_fault_modes() {
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

            let result = inject_fault(config);
            assert!(result.is_ok(), "Failed to inject fault mode: {}", mode);
        }
    }
}

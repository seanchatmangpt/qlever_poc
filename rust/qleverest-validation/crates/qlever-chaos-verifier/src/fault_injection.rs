/// Fault injection implementation with controlled injection points
/// Enforces fail-closed semantics: any injected fault must prevent normal completion

use crate::fault_modes::{FaultInjectionConfig, FaultMode, InjectionResult, RecoveryState};
use serde::{Deserialize, Serialize};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Arc, Mutex};

/// Thread-safe fault injector for chaos testing
pub struct FaultInjector {
    config: FaultInjectionConfig,
    injection_count: Arc<AtomicU32>,
    recovery_state: Arc<Mutex<RecoveryState>>,
}

impl FaultInjector {
    /// Create a new fault injector with given configuration
    pub fn new(config: FaultInjectionConfig) -> Self {
        Self {
            config,
            injection_count: Arc::new(AtomicU32::new(0)),
            recovery_state: Arc::new(Mutex::new(RecoveryState::Healthy)),
        }
    }

    /// Attempt to inject fault at a control point
    /// Returns InjectionResult indicating whether fault was injected
    pub fn try_inject(&self) -> Result<InjectionResult, String> {
        let current_count = self.injection_count.load(Ordering::SeqCst);

        // Check if we've reached injection limit
        if current_count >= self.config.max_injections {
            return Ok(InjectionResult::LimitReached);
        }

        // Deterministic mode: always inject if count < max
        if self.config.deterministic {
            self.injection_count.fetch_add(1, Ordering::SeqCst);
            self._set_recovery_state(RecoveryState::FaultDetected)?;
            return Ok(InjectionResult::Injected);
        }

        // Probabilistic mode: inject based on probability
        if self.config.probability >= 1.0 {
            self.injection_count.fetch_add(1, Ordering::SeqCst);
            self._set_recovery_state(RecoveryState::FaultDetected)?;
            Ok(InjectionResult::Injected)
        } else {
            Ok(InjectionResult::NotInjected)
        }
    }

    /// Get current recovery state
    pub fn get_recovery_state(&self) -> Result<RecoveryState, String> {
        self.recovery_state
            .lock()
            .map(|guard| *guard)
            .map_err(|e| format!("Failed to lock recovery state: {}", e))
    }

    /// Set recovery state (internal use)
    fn _set_recovery_state(&self, state: RecoveryState) -> Result<(), String> {
        self.recovery_state
            .lock()
            .map(|mut guard| *guard = state)
            .map_err(|e| format!("Failed to lock recovery state: {}", e))
    }

    /// Mark system as recovered
    pub fn mark_recovered(&self) -> Result<(), String> {
        self._set_recovery_state(RecoveryState::Recovered)
    }

    /// Mark system as fail-closed (unrecoverable)
    pub fn mark_fail_closed(&self) -> Result<(), String> {
        self._set_recovery_state(RecoveryState::FailClosed)
    }

    /// Get current injection count
    pub fn injection_count(&self) -> u32 {
        self.injection_count.load(Ordering::SeqCst)
    }

    /// Reset injector state
    pub fn reset(&self) -> Result<(), String> {
        self.injection_count.store(0, Ordering::SeqCst);
        self._set_recovery_state(RecoveryState::Healthy)
    }
}

/// Record of a single fault injection event
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FaultInjectionEvent {
    pub timestamp_ns: u64,
    pub fault_mode: FaultMode,
    pub injection_point: String,
    pub recovery_state: RecoveryState,
    pub partial_results: bool,  // Did partial results escape before abort?
}

/// Fault injection executor with result tracking
pub struct FaultInjectionExecutor {
    injector: FaultInjector,
    events: Arc<Mutex<Vec<FaultInjectionEvent>>>,
}

impl FaultInjectionExecutor {
    /// Create new executor with configuration
    pub fn new(config: FaultInjectionConfig) -> Self {
        Self {
            injector: FaultInjector::new(config),
            events: Arc::new(Mutex::new(Vec::new())),
        }
    }

    /// Try to inject fault at a control point
    pub fn try_inject(&self) -> Result<InjectionResult, String> {
        self.injector.try_inject()
    }

    /// Execute operation with fault injection
    /// Returns error if fault was injected (fail-closed semantics)
    pub fn execute_with_injection<F>(&self, op: F) -> Result<(), FaultInjectionError>
    where
        F: FnOnce() -> Result<(), String>,
    {
        // Try to inject fault
        match self.injector.try_inject()? {
            InjectionResult::Injected => {
                // Fault injected: mark fail-closed and return error
                self.injector.mark_fail_closed()?;
                Err(FaultInjectionError::FaultInjected {
                    mode: self.injector.config.mode,
                    message: format!("Fault injected: {}", self.injector.config.mode),
                })
            }
            InjectionResult::NotInjected => {
                // Normal execution path
                op().map_err(|e| FaultInjectionError::ExecutionError(e))
            }
            InjectionResult::LimitReached => {
                // Normal execution path (injection limit reached)
                op().map_err(|e| FaultInjectionError::ExecutionError(e))
            }
        }
    }

    /// Record a fault injection event
    pub fn record_event(&self, event: FaultInjectionEvent) -> Result<(), String> {
        self.events
            .lock()
            .map(|mut guard| guard.push(event))
            .map_err(|e| format!("Failed to record event: {}", e))
    }

    /// Get recorded events
    pub fn get_events(&self) -> Result<Vec<FaultInjectionEvent>, String> {
        self.events
            .lock()
            .map(|guard| guard.clone())
            .map_err(|e| format!("Failed to get events: {}", e))
    }

    /// Reset executor state
    pub fn reset(&self) -> Result<(), String> {
        self.injector.reset()?;
        self.events
            .lock()
            .map(|mut guard| guard.clear())
            .map_err(|e| format!("Failed to reset events: {}", e))
    }

    /// Verify no partial results escaped
    pub fn verify_no_partial_results(&self) -> Result<bool, String> {
        let events = self.get_events()?;
        for event in events {
            if event.partial_results {
                return Ok(false);  // Found partial results!
            }
        }
        Ok(true)
    }
}

/// Error type for fault injection
#[derive(Debug, Clone)]
pub enum FaultInjectionError {
    /// Fault was successfully injected
    FaultInjected { mode: FaultMode, message: String },
    /// Execution error (non-fault related)
    ExecutionError(String),
    /// State management error
    StateError(String),
}

impl From<InjectionResult> for FaultInjectionError {
    fn from(_: InjectionResult) -> Self {
        Self::ExecutionError("Invalid state transition".to_string())
    }
}

impl From<String> for FaultInjectionError {
    fn from(s: String) -> Self {
        Self::StateError(s)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_fault_injector_creation() {
        let config = FaultInjectionConfig::default();
        let injector = FaultInjector::new(config);
        assert_eq!(injector.injection_count(), 0);
    }

    #[test]
    fn test_deterministic_injection() {
        let config = FaultInjectionConfig {
            mode: FaultMode::CacheCorruption,
            probability: 1.0,
            max_injections: 1,
            deterministic: true,
        };
        let injector = FaultInjector::new(config);

        let result = injector.try_inject().unwrap();
        assert_eq!(result, InjectionResult::Injected);
        assert_eq!(injector.injection_count(), 1);

        // Second attempt should hit limit
        let result = injector.try_inject().unwrap();
        assert_eq!(result, InjectionResult::LimitReached);
    }

    #[test]
    fn test_recovery_state_transitions() {
        let config = FaultInjectionConfig::default();
        let injector = FaultInjector::new(config);

        // Initially healthy
        assert_eq!(
            injector.get_recovery_state().unwrap(),
            RecoveryState::Healthy
        );

        // After injection, fault detected
        let _ = injector.try_inject();
        assert_eq!(
            injector.get_recovery_state().unwrap(),
            RecoveryState::FaultDetected
        );

        // Can mark recovered
        injector.mark_recovered().unwrap();
        assert_eq!(
            injector.get_recovery_state().unwrap(),
            RecoveryState::Recovered
        );
    }

    #[test]
    fn test_executor_with_injection() {
        let config = FaultInjectionConfig {
            mode: FaultMode::OutOfMemory,
            probability: 1.0,
            max_injections: 1,
            deterministic: true,
        };
        let executor = FaultInjectionExecutor::new(config);

        let result = executor.execute_with_injection(|| Ok(()));
        assert!(result.is_err());

        match result {
            Err(FaultInjectionError::FaultInjected { mode, .. }) => {
                assert_eq!(mode, FaultMode::OutOfMemory);
            }
            _ => panic!("Expected FaultInjected error"),
        }
    }

    #[test]
    fn test_partial_results_detection() {
        let config = FaultInjectionConfig::default();
        let executor = FaultInjectionExecutor::new(config);

        // Record event with partial results
        let event = FaultInjectionEvent {
            timestamp_ns: 0,
            fault_mode: FaultMode::CacheCorruption,
            injection_point: "test".to_string(),
            recovery_state: RecoveryState::FailClosed,
            partial_results: true,
        };
        executor.record_event(event).unwrap();

        // Verification should fail
        let result = executor.verify_no_partial_results().unwrap();
        assert!(!result);
    }
}

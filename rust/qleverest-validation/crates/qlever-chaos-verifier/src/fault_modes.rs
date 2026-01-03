/// Fault modes for chaos testing in QLever read cache
/// Each mode represents a specific failure condition that must be detected and fail-closed

use serde::{Deserialize, Serialize};
use std::fmt;

/// Enumeration of all supported fault injection modes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum FaultMode {
    /// Cache data corruption: bytes in cache modified without notification
    CacheCorruption,
    /// Epoch contamination: cache result from wrong epoch returned
    EpochContamination,
    /// Out of memory: allocation failure during query execution
    OutOfMemory,
    /// Timeout: query execution exceeds time budget
    Timeout,
    /// Invalid epoch key: mismatched epoch prefix in cache lookup
    InvalidEpochKey,
}

impl fmt::Display for FaultMode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            FaultMode::CacheCorruption => write!(f, "CacheCorruption"),
            FaultMode::EpochContamination => write!(f, "EpochContamination"),
            FaultMode::OutOfMemory => write!(f, "OutOfMemory"),
            FaultMode::Timeout => write!(f, "Timeout"),
            FaultMode::InvalidEpochKey => write!(f, "InvalidEpochKey"),
        }
    }
}

/// Configuration for fault injection
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FaultInjectionConfig {
    /// Which fault mode to inject
    pub mode: FaultMode,
    /// Probability of injection (0.0 to 1.0)
    pub probability: f64,
    /// Maximum number of injections before stopping
    pub max_injections: u32,
    /// Whether to inject on first opportunity (deterministic)
    pub deterministic: bool,
}

impl Default for FaultInjectionConfig {
    fn default() -> Self {
        Self {
            mode: FaultMode::CacheCorruption,
            probability: 1.0,  // Inject deterministically by default
            max_injections: 1,
            deterministic: true,
        }
    }
}

/// Result of fault injection attempt
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum InjectionResult {
    /// Fault was successfully injected
    Injected,
    /// Fault was not injected (probability check failed)
    NotInjected,
    /// Injection limit reached
    LimitReached,
}

/// Recovery state indicator
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum RecoveryState {
    /// System is in normal operating state
    Healthy,
    /// Fault has been detected
    FaultDetected,
    /// System is in recovery process
    Recovering,
    /// System has recovered cleanly
    Recovered,
    /// System is unrecoverable (fail-closed)
    FailClosed,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_fault_mode_display() {
        assert_eq!(FaultMode::CacheCorruption.to_string(), "CacheCorruption");
        assert_eq!(FaultMode::EpochContamination.to_string(), "EpochContamination");
        assert_eq!(FaultMode::OutOfMemory.to_string(), "OutOfMemory");
    }

    #[test]
    fn test_fault_injection_config_default() {
        let config = FaultInjectionConfig::default();
        assert_eq!(config.probability, 1.0);
        assert_eq!(config.max_injections, 1);
        assert!(config.deterministic);
    }

    #[test]
    fn test_injection_result_variants() {
        let injected = InjectionResult::Injected;
        let not_injected = InjectionResult::NotInjected;
        let limit = InjectionResult::LimitReached;

        assert_eq!(injected, InjectionResult::Injected);
        assert_ne!(injected, not_injected);
        assert_ne!(not_injected, limit);
    }

    #[test]
    fn test_recovery_state_variants() {
        let healthy = RecoveryState::Healthy;
        let detected = RecoveryState::FaultDetected;
        let recovered = RecoveryState::Recovered;
        let failed = RecoveryState::FailClosed;

        assert_eq!(healthy, RecoveryState::Healthy);
        assert_ne!(detected, healthy);
        assert_ne!(recovered, detected);
        assert_ne!(failed, recovered);
    }
}

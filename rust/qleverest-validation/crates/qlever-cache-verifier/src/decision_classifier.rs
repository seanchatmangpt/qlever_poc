//! Cache decision classifier
//!
//! Classifies and validates cache decisions according to EPIC 11 spec.

use super::decision_log::CacheDecisionType;
use qlever_kernel_runner::CacheTier;
use thiserror::Error;

/// Decision classification errors
#[derive(Debug, Error, PartialEq)]
pub enum ClassificationError {
    #[error("Unknown decision type: {0}")]
    UnknownDecisionType(String),

    #[error("Invalid cache tier: {0}")]
    InvalidCacheTier(String),
}

/// Classify cache decision types from string representations
pub fn classify_decision(decision_str: &str) -> Result<CacheDecisionType, ClassificationError> {
    match decision_str.to_uppercase().as_str() {
        "HIT" => Ok(CacheDecisionType::Hit),
        "MISS" => Ok(CacheDecisionType::Miss),
        "ADMIT" => Ok(CacheDecisionType::Admit),
        "REJECT" => Ok(CacheDecisionType::Reject),
        "EVICT" => Ok(CacheDecisionType::Evict),
        "GUARDED" => Ok(CacheDecisionType::Guarded),
        _ => Err(ClassificationError::UnknownDecisionType(decision_str.to_string())),
    }
}

/// Check if a decision represents a cache hit/get operation
pub fn is_get_operation(decision: &CacheDecisionType) -> bool {
    matches!(decision, CacheDecisionType::Hit | CacheDecisionType::Miss)
}

/// Check if a decision represents a cache admission/put operation
pub fn is_put_operation(decision: &CacheDecisionType) -> bool {
    matches!(
        decision,
        CacheDecisionType::Admit
            | CacheDecisionType::Reject
            | CacheDecisionType::Evict
            | CacheDecisionType::Guarded
    )
}

/// Check if a decision indicates a successful operation
pub fn is_success(decision: &CacheDecisionType) -> bool {
    matches!(
        decision,
        CacheDecisionType::Hit | CacheDecisionType::Miss | CacheDecisionType::Admit
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_classify_hit() {
        let decision = classify_decision("HIT");
        assert!(decision.is_ok());
        assert_eq!(decision.unwrap(), CacheDecisionType::Hit);
    }

    #[test]
    fn test_classify_all_decision_types() {
        let types = vec!["HIT", "MISS", "ADMIT", "REJECT", "EVICT", "GUARDED"];
        for decision_str in types {
            let result = classify_decision(decision_str);
            assert!(
                result.is_ok(),
                "Failed to classify: {}",
                decision_str
            );
        }
    }

    #[test]
    fn test_classify_case_insensitive() {
        let decision1 = classify_decision("hit");
        let decision2 = classify_decision("HIT");
        let decision3 = classify_decision("Hit");
        assert_eq!(decision1, decision2);
        assert_eq!(decision2, decision3);
    }

    #[test]
    fn test_classify_invalid_decision() {
        let decision = classify_decision("UNKNOWN");
        assert!(decision.is_err());
    }

    #[test]
    fn test_is_get_operation() {
        assert!(is_get_operation(&CacheDecisionType::Hit));
        assert!(is_get_operation(&CacheDecisionType::Miss));
        assert!(!is_get_operation(&CacheDecisionType::Admit));
        assert!(!is_get_operation(&CacheDecisionType::Evict));
    }

    #[test]
    fn test_is_put_operation() {
        assert!(!is_put_operation(&CacheDecisionType::Hit));
        assert!(!is_put_operation(&CacheDecisionType::Miss));
        assert!(is_put_operation(&CacheDecisionType::Admit));
        assert!(is_put_operation(&CacheDecisionType::Reject));
        assert!(is_put_operation(&CacheDecisionType::Evict));
        assert!(is_put_operation(&CacheDecisionType::Guarded));
    }

    #[test]
    fn test_is_success() {
        assert!(is_success(&CacheDecisionType::Hit));
        assert!(is_success(&CacheDecisionType::Miss));
        assert!(is_success(&CacheDecisionType::Admit));
        assert!(!is_success(&CacheDecisionType::Reject));
        assert!(!is_success(&CacheDecisionType::Evict));
    }

    #[test]
    fn test_cache_tier_classification() {
        assert_eq!(CacheTier::from("bytes"), CacheTier::Bytes);
        assert_eq!(CacheTier::from("neg"), CacheTier::Neg);
        assert_eq!(CacheTier::from("plan"), CacheTier::Plan);
    }

    #[test]
    fn test_cache_tier_numeric() {
        assert_eq!(CacheTier::from("0"), CacheTier::Bytes);
        assert_eq!(CacheTier::from("1"), CacheTier::Neg);
        assert_eq!(CacheTier::from("2"), CacheTier::Plan);
    }

    #[test]
    fn test_cache_tier_invalid() {
        // Invalid tier defaults to Bytes
        assert_eq!(CacheTier::from("invalid"), CacheTier::Bytes);
    }
}

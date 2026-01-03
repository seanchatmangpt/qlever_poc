use qlever_kernel_runner::CacheTier;
//! Cache transparency tests
//!
//! Tests for Invariant B4: Silent cache behavior is forbidden
//! Every cache operation must be recorded in the decision log.

use qlever_cache_verifier::{
    verify_transparency, verify_behavior_sequence, verify_no_silent_hits,
    CacheDecision, CacheDecisionLog, CacheDecisionType, CacheVerifierError,
};

#[test]
fn test_no_silent_cache_behavior() {
    // INVARIANT B4: Every cache operation must be recorded
    // VIOLATION: Missing decision record -> ABORT with RECEIPT

    let mut log = CacheDecisionLog::new();

    // Create decision records for all query types
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: "q2".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    // Verify transparency passes when all queries have records
    let expected_queries = vec!["q1".to_string(), "q2".to_string()];
    let result = verify_transparency(&log, &expected_queries);
    assert!(result.is_ok(), "Should pass when all queries have decision records");
}

#[test]
fn test_silent_cache_behavior_detected() {
    // Test that missing decision records are detected

    let log = CacheDecisionLog::new(); // Empty log

    // Try to verify transparency for a query that has no records
    let expected_queries = vec!["q1".to_string()];
    let result = verify_transparency(&log, &expected_queries);

    assert!(result.is_err(), "Should fail when decision record is missing");
    assert!(
        matches!(result, Err(CacheVerifierError::SilentCacheBehavior { .. })),
        "Should return SilentCacheBehavior error"
    );
}

#[test]
fn test_partial_transparency_failure() {
    // Test that transparency fails when some queries are missing records

    let mut log = CacheDecisionLog::new();

    // Only record q1, but expect both q1 and q2
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    let expected_queries = vec!["q1".to_string(), "q2".to_string()];
    let result = verify_transparency(&log, &expected_queries);

    assert!(result.is_err(), "Should fail when q2 has no decision record");
}

#[test]
fn test_all_six_decision_types_recorded() {
    // EPIC 11 specifies 6 decision types: HIT, MISS, ADMIT, REJECT, EVICT, GUARDED
    // Test that all can be recorded and verified

    let mut log = CacheDecisionLog::new();

    let decisions = vec![
        ("q1", CacheDecisionType::Hit),
        ("q2", CacheDecisionType::Miss),
        ("q3", CacheDecisionType::Admit),
        ("q4", CacheDecisionType::Reject),
        ("q5", CacheDecisionType::Evict),
        ("q6", CacheDecisionType::Guarded),
    ];

    for (query_id, decision_type) in &decisions {
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: query_id.to_string(),
            decision: decision_type.clone(),
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });
    }

    // Verify all queries have records
    let expected_queries: Vec<String> = decisions.iter()
        .map(|(q, _)| q.to_string())
        .collect();

    let result = verify_transparency(&log, &expected_queries);
    assert!(result.is_ok(), "All 6 decision types should be recordable");
}

#[test]
fn test_cache_behavior_sequence_verification() {
    // Test that cache behavior sequences can be verified
    // e.g., MISS -> ADMIT -> HIT

    let mut log = CacheDecisionLog::new();

    let query_id = "q1";

    // Record: MISS, ADMIT, HIT (query is fetched, result admitted, then hit)
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: query_id.to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: query_id.to_string(),
        decision: CacheDecisionType::Admit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 3000,
        query_id: query_id.to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    // Verify the sequence
    let expected_sequence = vec![
        CacheDecisionType::Miss,
        CacheDecisionType::Admit,
        CacheDecisionType::Hit,
    ];

    let result = verify_behavior_sequence(&log, query_id, &expected_sequence);
    assert!(result.is_ok(), "Sequence should match");
}

#[test]
fn test_behavior_sequence_divergence_detected() {
    // Test that diverging cache behavior sequences are detected

    let mut log = CacheDecisionLog::new();

    let query_id = "q1";

    // Record: MISS, EVICT (result was evicted before being admitted)
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: query_id.to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: query_id.to_string(),
        decision: CacheDecisionType::Evict,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: Some("entry_x".to_string()),
    });

    // Expect different sequence: MISS, ADMIT
    let expected_sequence = vec![
        CacheDecisionType::Miss,
        CacheDecisionType::Admit,
    ];

    let result = verify_behavior_sequence(&log, query_id, &expected_sequence);
    assert!(result.is_err(), "Should detect divergence");
    assert!(
        matches!(result, Err(CacheVerifierError::CacheBehaviorDivergence { .. })),
        "Should return CacheBehaviorDivergence error"
    );
}

#[test]
fn test_no_silent_hits_detection() {
    // Test that missing HIT records are detected
    // If cache reports N hits, all N must be recorded

    let mut log = CacheDecisionLog::new();

    // Record 1 HIT
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    // But claim cache had 2 hits
    let result = verify_no_silent_hits(&log, 2);
    assert!(result.is_err(), "Should detect missing hit records");
}

#[test]
fn test_no_silent_hits_passes() {
    // Test that transparent hits pass verification

    let mut log = CacheDecisionLog::new();

    // Record 2 HITs
    for i in 0..2 {
        log.record(CacheDecision {
            timestamp_ns: 1000 + i * 1000,
            query_id: format!("q{}", i),
            decision: CacheDecisionType::Hit,
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });
    }

    // Claim cache had 2 hits
    let result = verify_no_silent_hits(&log, 2);
    assert!(result.is_ok(), "Should pass when all hits are recorded");
}

#[test]
fn test_multi_tier_decision_logging() {
    // Test that decisions across different cache tiers are properly recorded

    let mut log = CacheDecisionLog::new();

    // Record decisions for different tiers
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: "q2".to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: "neg".to_string(),
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 3000,
        query_id: "q3".to_string(),
        decision: CacheDecisionType::Admit,
        cache_tier: "plan".to_string(),
        evicted_entry_id: None,
    });

    // Verify all tiers have records
    let expected_queries = vec!["q1".to_string(), "q2".to_string(), "q3".to_string()];
    let result = verify_transparency(&log, &expected_queries);

    assert!(result.is_ok(), "All tier decisions should be recorded");
}

#[test]
fn test_eviction_records_required() {
    // Test that eviction decisions must be recorded with entry identification

    let mut log = CacheDecisionLog::new();

    // Record an eviction with entry ID
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Evict,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: Some("entry_123".to_string()),
    });

    // Verify the decision is recorded
    let decisions = log.get_decisions_for_query("q1");
    assert_eq!(decisions.len(), 1);
    assert!(decisions[0].evicted_entry_id.is_some());
}

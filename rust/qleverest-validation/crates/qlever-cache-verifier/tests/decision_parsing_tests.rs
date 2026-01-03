use qlever_kernel_runner::CacheTier;
//! Decision log parsing and schema validation tests
//!
//! Tests JSON serialization/deserialization of cache decision logs
//! according to EPIC 11 decision log entry format.

use qlever_cache_verifier::{
    CacheDecision, CacheDecisionLog, CacheDecisionType,
};

#[test]
fn test_decision_log_json_roundtrip() {
    // Test JSON serialization and deserialization roundtrip

    let mut original_log = CacheDecisionLog::new();

    // Add various decision records
    original_log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    original_log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Admit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    original_log.record(CacheDecision {
        timestamp_ns: 3000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    // Serialize to JSON
    let json = original_log.to_json().expect("Serialization should succeed");

    // Deserialize from JSON
    let parsed_log = CacheDecisionLog::from_json(&json)
        .expect("Deserialization should succeed");

    // Verify the logs are equivalent
    assert_eq!(original_log.len(), parsed_log.len());
    assert_eq!(original_log.decisions(), parsed_log.decisions());
}

#[test]
fn test_empty_log_serialization() {
    // Test that empty logs can be serialized/deserialized

    let log = CacheDecisionLog::new();
    let json = log.to_json().expect("Serialization should succeed");

    let parsed = CacheDecisionLog::from_json(&json)
        .expect("Deserialization should succeed");

    assert!(parsed.is_empty());
    assert_eq!(parsed.len(), 0);
}

#[test]
fn test_json_format_compliance() {
    // Test that JSON format matches EPIC 11 spec

    let mut log = CacheDecisionLog::new();

    log.record(CacheDecision {
        timestamp_ns: 1234567890,
        query_id: "query_001".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    let json = log.to_json().expect("Serialization should succeed");

    // Verify JSON is valid
    let parsed: Result<serde_json::Value, _> = serde_json::from_str(&json);
    assert!(parsed.is_ok(), "Output should be valid JSON");

    let value = parsed.unwrap();
    assert!(value.is_array(), "JSON should be an array");

    let array = value.as_array().unwrap();
    assert_eq!(array.len(), 1);

    let entry = &array[0];
    assert!(entry.get("timestamp_ns").is_some());
    assert!(entry.get("query_id").is_some());
    assert!(entry.get("decision").is_some());
    assert!(entry.get("cache_tier").is_some());
    assert!(entry.get("evicted_entry_id").is_some());
}

#[test]
fn test_all_decision_types_json() {
    // Test that all 6 decision types serialize correctly

    let decision_types = vec![
        ("HIT", CacheDecisionType::Hit),
        ("MISS", CacheDecisionType::Miss),
        ("ADMIT", CacheDecisionType::Admit),
        ("REJECT", CacheDecisionType::Reject),
        ("EVICT", CacheDecisionType::Evict),
        ("GUARDED", CacheDecisionType::Guarded),
    ];

    for (name, decision_type) in decision_types {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: format!("q_{}", name),
            decision: decision_type.clone(),
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });

        // Should serialize without error
        let json = log.to_json();
        assert!(json.is_ok(), "Decision type {} should serialize", name);

        // Should deserialize without error
        let parsed = CacheDecisionLog::from_json(&json.unwrap());
        assert!(parsed.is_ok(), "Decision type {} should deserialize", name);
    }
}

#[test]
fn test_evicted_entry_id_optional() {
    // Test that evicted_entry_id is properly optional

    let mut log = CacheDecisionLog::new();

    // Decision without evicted_entry_id
    log.record(CacheDecision {
        timestamp_ns: 1000,
        query_id: "q1".to_string(),
        decision: CacheDecisionType::Hit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    // Decision with evicted_entry_id
    log.record(CacheDecision {
        timestamp_ns: 2000,
        query_id: "q2".to_string(),
        decision: CacheDecisionType::Evict,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: Some("entry_abc".to_string()),
    });

    let json = log.to_json().expect("Serialization should succeed");
    let parsed = CacheDecisionLog::from_json(&json)
        .expect("Deserialization should succeed");

    let decisions = parsed.decisions();
    assert!(decisions[0].evicted_entry_id.is_none());
    assert!(decisions[1].evicted_entry_id.is_some());
}

#[test]
fn test_cache_tier_variations() {
    // Test that different cache tier representations work

    let tiers = vec!["bytes", "neg", "plan"];

    for tier in tiers {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: "q1".to_string(),
            decision: CacheDecisionType::Hit,
            cache_tier: tier.to_string(),
            evicted_entry_id: None,
        });

        let json = log.to_json().expect("Serialization should succeed");
        let parsed = CacheDecisionLog::from_json(&json)
            .expect("Deserialization should succeed");

        assert_eq!(parsed.decisions()[0].cache_tier, tier);
    }
}

#[test]
fn test_large_decision_log_serialization() {
    // Test that large logs serialize efficiently

    let mut log = CacheDecisionLog::new();

    // Add 1000 decisions
    for i in 0..1000 {
        log.record(CacheDecision {
            timestamp_ns: 1000 + (i as u64) * 100,
            query_id: format!("q_{}", i % 10),
            decision: match i % 6 {
                0 => CacheDecisionType::Hit,
                1 => CacheDecisionType::Miss,
                2 => CacheDecisionType::Admit,
                3 => CacheDecisionType::Reject,
                4 => CacheDecisionType::Evict,
                _ => CacheDecisionType::Guarded,
            },
            cache_tier: match i % 3 {
                0 => "bytes".to_string(),
                1 => "neg".to_string(),
                _ => "plan".to_string(),
            },
            evicted_entry_id: if i % 5 == 0 {
                Some(format!("entry_{}", i))
            } else {
                None
            },
        });
    }

    // Serialize and deserialize
    let json = log.to_json().expect("Serialization should succeed");
    let parsed = CacheDecisionLog::from_json(&json)
        .expect("Deserialization should succeed");

    assert_eq!(parsed.len(), 1000);
}

#[test]
fn test_deterministic_json_hash() {
    // Test that the same log produces the same hash (determinism)

    let mut log = CacheDecisionLog::new();

    for i in 0..10 {
        log.record(CacheDecision {
            timestamp_ns: 1000 + (i as u64) * 100,
            query_id: format!("q_{}", i),
            decision: CacheDecisionType::Hit,
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });
    }

    let hash1 = log.compute_hash();
    let hash2 = log.compute_hash();

    assert_eq!(hash1, hash2, "Hash should be deterministic");
}

#[test]
fn test_hit_rate_calculation() {
    // Test hit rate statistics calculation

    let mut log = CacheDecisionLog::new();

    // Create a pattern: MISS, MISS, HIT, HIT, MISS, HIT = 3 hits, 3 misses = 50%
    let decisions = vec![
        CacheDecisionType::Miss,
        CacheDecisionType::Miss,
        CacheDecisionType::Hit,
        CacheDecisionType::Hit,
        CacheDecisionType::Miss,
        CacheDecisionType::Hit,
    ];

    for (i, decision) in decisions.iter().enumerate() {
        log.record(CacheDecision {
            timestamp_ns: 1000 + (i as u64) * 100,
            query_id: format!("q_{}", i),
            decision: decision.clone(),
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });
    }

    let hit_rate = log.hit_rate();
    assert!((hit_rate - 0.5).abs() < 0.001, "Hit rate should be 50%");
}

#[test]
fn test_eviction_count() {
    // Test eviction count calculation

    let mut log = CacheDecisionLog::new();

    // Add 5 decisions, 2 of which are evictions
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
        decision: CacheDecisionType::Evict,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: Some("entry_1".to_string()),
    });

    log.record(CacheDecision {
        timestamp_ns: 3000,
        query_id: "q3".to_string(),
        decision: CacheDecisionType::Evict,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: Some("entry_2".to_string()),
    });

    log.record(CacheDecision {
        timestamp_ns: 4000,
        query_id: "q4".to_string(),
        decision: CacheDecisionType::Miss,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    log.record(CacheDecision {
        timestamp_ns: 5000,
        query_id: "q5".to_string(),
        decision: CacheDecisionType::Admit,
        cache_tier: CacheTier::Bytes,
        evicted_entry_id: None,
    });

    assert_eq!(log.eviction_count(), 2);
}

#[test]
fn test_special_characters_in_query_ids() {
    // Test that special characters in query IDs are handled correctly

    let special_ids = vec![
        "SELECT * WHERE",
        "q-with-dashes",
        "q_with_underscores",
        "q.with.dots",
        "q/with/slashes",
    ];

    for query_id in special_ids {
        let mut log = CacheDecisionLog::new();
        log.record(CacheDecision {
            timestamp_ns: 1000,
            query_id: query_id.to_string(),
            decision: CacheDecisionType::Hit,
            cache_tier: CacheTier::Bytes,
            evicted_entry_id: None,
        });

        let json = log.to_json();
        assert!(json.is_ok(), "Should handle special characters in query ID: {}", query_id);

        let parsed = CacheDecisionLog::from_json(&json.unwrap());
        assert!(parsed.is_ok());
        assert_eq!(parsed.unwrap().decisions()[0].query_id, query_id);
    }
}

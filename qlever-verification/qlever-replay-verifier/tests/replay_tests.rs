//! EPIC 11 Subsystem 5: Replay Tests
//!
//! Test cases for workload pack execution with strict and differential modes.
//! Per EPIC 11 Test Architecture (Category 3: Replay Tests)

use qlever_replay_verifier::{ReplayMode, ReplayQuery, ReplayWorkload};

#[test]
fn test_replay_workload_creation() {
    let workload = ReplayWorkload::new("test-workload-v1");
    assert_eq!(workload.workload_id, "test-workload-v1");
    assert!(workload.is_empty());
    assert_eq!(workload.len(), 0);
}

#[test]
fn test_replay_workload_with_queries() {
    let q1 = ReplayQuery::new(
        "q1",
        "SELECT ?x WHERE { ?x rdf:type ?t }",
        0,
        "0".repeat(64),
    );
    let q2 = ReplayQuery::new("q2", "SELECT ?y WHERE { ?y rdf:label ?l }", 1, "1".repeat(64));

    let workload = ReplayWorkload::new("deterministic-corpus-v1")
        .add_query(q1)
        .add_query(q2)
        .with_mode(ReplayMode::Strict);

    assert_eq!(workload.len(), 2);
    assert_eq!(workload.replay_mode, ReplayMode::Strict);
}

#[test]
fn test_replay_determinism_strict_mode() {
    let workload = ReplayWorkload::new("test-strict")
        .add_query(ReplayQuery::new(
            "q1",
            "SELECT 1",
            0,
            "0".repeat(64),
        ))
        .with_mode(ReplayMode::Strict);

    assert_eq!(workload.replay_mode, ReplayMode::Strict);
}

#[test]
fn test_replay_differential_mode() {
    let workload = ReplayWorkload::new("test-differential")
        .add_query(ReplayQuery::new(
            "q1",
            "SELECT 1",
            0,
            "0".repeat(64),
        ))
        .with_mode(ReplayMode::Differential);

    assert_eq!(workload.replay_mode, ReplayMode::Differential);
}

#[test]
fn test_replay_best_effort_mode() {
    let workload = ReplayWorkload::new("test-best-effort")
        .add_query(ReplayQuery::new(
            "q1",
            "SELECT 1",
            0,
            "0".repeat(64),
        ))
        .with_mode(ReplayMode::BestEffort);

    assert_eq!(workload.replay_mode, ReplayMode::BestEffort);
}

#[test]
fn test_replay_query_with_cache_behavior() {
    use qlever_replay_verifier::CacheDecision;

    let query = ReplayQuery::new("q1", "SELECT 1", 0, "0".repeat(64))
        .with_cache_behavior(vec![CacheDecision::Miss, CacheDecision::Admit])
        .with_latency(100.0);

    assert_eq!(query.expected_cache_behavior.len(), 2);
    assert_eq!(query.expected_latency_ms, 100.0);
}

#[test]
fn test_replay_workload_multiple_queries() {
    let mut workload = ReplayWorkload::new("multi-query-test");

    for i in 0..10 {
        let query = ReplayQuery::new(
            format!("q{}", i),
            format!("SELECT {} WHERE {{...}}", i),
            i as u32,
            "0".repeat(64),
        );
        workload = workload.add_query(query);
    }

    assert_eq!(workload.len(), 10);
    for (i, query) in workload.query_pack.iter().enumerate() {
        assert_eq!(query.query_id, format!("q{}", i));
        assert_eq!(query.execution_order, i as u32);
    }
}

#[test]
fn test_replay_query_serialization() {
    let query = ReplayQuery::new(
        "q1",
        "SELECT ?x WHERE { ?x rdf:type ?t }",
        0,
        "a".repeat(64),
    )
    .with_latency(250.5);

    let json = serde_json::to_string(&query).unwrap();
    let deserialized: ReplayQuery = serde_json::from_str(&json).unwrap();

    assert_eq!(deserialized.query_id, "q1");
    assert_eq!(deserialized.execution_order, 0);
    assert_eq!(deserialized.expected_latency_ms, 250.5);
}

#[test]
fn test_replay_workload_cbor_serialization() {
    let query1 = ReplayQuery::new("q1", "QUERY1", 0, "0".repeat(64));
    let query2 = ReplayQuery::new("q2", "QUERY2", 1, "1".repeat(64));

    let workload = ReplayWorkload::new("test-cbor")
        .add_query(query1)
        .add_query(query2)
        .with_mode(ReplayMode::Differential);

    // Serialize to CBOR
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&workload, &mut cbor_bytes).unwrap();

    // Deserialize back
    let deserialized: ReplayWorkload = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    assert_eq!(deserialized.workload_id, "test-cbor");
    assert_eq!(deserialized.len(), 2);
    assert_eq!(deserialized.replay_mode, ReplayMode::Differential);
    assert_eq!(deserialized.query_pack[0].query_id, "q1");
    assert_eq!(deserialized.query_pack[1].query_id, "q2");
}

#[test]
fn test_replay_workload_round_trip_preserves_all_fields() {
    use qlever_replay_verifier::ReplayState;

    let query = ReplayQuery::new("q1", "SELECT * FROM test", 0, "f".repeat(64))
        .with_latency(150.25);

    let mut state = ReplayState::default();
    state.cache_size_bytes = 1024 * 1024;
    state.cache_entries = 100;
    state.hit_rate_pct = 85.5;

    let workload = ReplayWorkload::new("preservation-test")
        .add_query(query)
        .with_expected_state(state)
        .with_mode(ReplayMode::Strict);

    // Serialize and deserialize
    let mut cbor_bytes = Vec::new();
    ciborium::into_writer(&workload, &mut cbor_bytes).unwrap();
    let deserialized: ReplayWorkload = ciborium::from_reader(&cbor_bytes[..]).unwrap();

    // Verify all fields preserved
    assert_eq!(deserialized.workload_id, "preservation-test");
    assert_eq!(deserialized.query_pack[0].query_id, "q1");
    assert_eq!(deserialized.query_pack[0].expected_latency_ms, 150.25);
    assert_eq!(deserialized.expected_state.cache_size_bytes, 1024 * 1024);
    assert_eq!(deserialized.expected_state.cache_entries, 100);
    assert_eq!(deserialized.expected_state.hit_rate_pct, 85.5);
    assert_eq!(deserialized.replay_mode, ReplayMode::Strict);
}

#[test]
fn test_replay_workload_execution_order() {
    let mut workload = ReplayWorkload::new("order-test");

    for i in 0..5 {
        let query = ReplayQuery::new(
            format!("q_{}", i),
            format!("Query number {}", i),
            i as u32,
            format!("{:0>64}", i),
        );
        workload = workload.add_query(query);
    }

    // Verify queries are in order
    for (i, query) in workload.query_pack.iter().enumerate() {
        assert_eq!(query.execution_order, i as u32);
        assert_eq!(query.query_id, format!("q_{}", i));
    }
}

#[test]
fn test_replay_workload_is_empty() {
    let empty = ReplayWorkload::new("empty");
    assert!(empty.is_empty());

    let with_query = ReplayWorkload::new("not-empty").add_query(ReplayQuery::new(
        "q1",
        "SELECT 1",
        0,
        "0".repeat(64),
    ));
    assert!(!with_query.is_empty());
}

#[test]
fn test_replay_mode_default_is_strict() {
    let workload = ReplayWorkload::new("default-mode");
    assert_eq!(workload.replay_mode, ReplayMode::Strict);
}

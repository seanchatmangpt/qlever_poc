//! EPIC 11 Determinism Tests (Invariant B)
//!
//! Validation that digest computation is deterministic across multiple runs.
//! Reference spec: Part I, Invariant B1 & B2

use qlever_digest_verifier::{compute_digest, verify_determinism, verify_digest, DeterminismDigest};

/// Test 1: Same input produces same digest (100 consecutive runs)
#[test]
fn test_same_input_produces_same_digest_100_runs() {
    let query_id = "q_determinism_test_1";
    let result_bytes = b"SELECT ?x WHERE { ?x a <http://example.org/Person> }";
    let cache_log = b"[{\"decision\":\"MISS\",\"timestamp\":1234567890}]";

    let baseline = compute_digest(query_id, result_bytes, cache_log);

    for run in 1..=99 {
        let current = compute_digest(query_id, result_bytes, cache_log);

        // All hashes must match exactly
        assert_eq!(
            baseline.result_hash, current.result_hash,
            "Result hash diverged at run {}: baseline {:?} vs {:?}",
            run + 1,
            baseline.result_hash,
            current.result_hash
        );

        assert_eq!(
            baseline.cache_log_hash, current.cache_log_hash,
            "Cache log hash diverged at run {}: baseline {:?} vs {:?}",
            run + 1,
            baseline.cache_log_hash,
            current.cache_log_hash
        );

        assert_eq!(
            baseline.combined_digest, current.combined_digest,
            "Combined digest diverged at run {}: baseline {:?} vs {:?}",
            run + 1,
            baseline.combined_digest,
            current.combined_digest
        );
    }

    // Final verification: baseline matches itself
    let final_run = compute_digest(query_id, result_bytes, cache_log);
    let verification = verify_digest(&baseline, &final_run);
    assert!(verification.is_ok(), "Final verification failed after 100 runs");
}

/// Test 2: Multiple runs with different data produce consistent digests
#[test]
fn test_multiple_runs_produce_consistent_digests() {
    let test_cases: Vec<(&str, &[u8], &[u8])> = vec![
        ("q_test_2a", b"short", b"log1"),
        ("q_test_2b", b"medium length result string", b"longer log entry"),
        (
            "q_test_2c",
            b"very long result string with lots of data that would be typical for a SPARQL query result",
            b"[{\"decision\":\"HIT\",\"cache_tier\":\"bytes\",\"timestamp\":1234567890,\"query_id\":\"q_test_2c\"}]"
        ),
    ];

    for (query_id, result, log) in test_cases {
        let digest1 = compute_digest(query_id, result, log);
        let digest2 = compute_digest(query_id, result, log);

        assert_eq!(digest1.result_hash, digest2.result_hash);
        assert_eq!(digest1.cache_log_hash, digest2.cache_log_hash);
        assert_eq!(digest1.combined_digest, digest2.combined_digest);
    }
}

/// Test 3: Verify determinism across 100 runs using verify_determinism function
#[test]
fn test_verify_determinism_100_runs() {
    let query_id = "q_determinism_verify";
    let result_bytes = b"test result data";
    let cache_log = b"[{\"decision\":\"ADMIT\"}]";

    let result = verify_determinism(query_id, || (result_bytes.to_vec(), cache_log.to_vec()), 100);

    assert!(
        result.is_ok(),
        "Determinism verification failed: {:?}",
        result.err()
    );

    let digest = result.unwrap();
    assert_eq!(digest.query_id, query_id);
}

/// Test 4: Empty inputs produce valid digests
#[test]
fn test_empty_inputs_produce_valid_digests() {
    let query_id = "q_empty_test";
    let empty_result = b"";
    let empty_log = b"";

    let digest1 = compute_digest(query_id, empty_result, empty_log);
    let digest2 = compute_digest(query_id, empty_result, empty_log);

    assert_eq!(digest1.result_hash, digest2.result_hash);
    assert_eq!(digest1.cache_log_hash, digest2.cache_log_hash);
    assert_eq!(digest1.combined_digest, digest2.combined_digest);

    let verification = verify_digest(&digest1, &digest2);
    assert!(verification.is_ok());
}

/// Test 5: Large result sets maintain determinism
#[test]
fn test_large_results_maintain_determinism() {
    let query_id = "q_large_test";
    // Simulate large SPARQL result (10KB)
    let mut large_result = Vec::new();
    for i in 0..1000 {
        large_result.extend_from_slice(format!("?x_{}: <http://example.org/{}>", i, i).as_bytes());
    }

    // Simulate large cache log (5KB)
    let mut large_log = Vec::new();
    large_log.extend_from_slice(b"[");
    for i in 0..100 {
        large_log.extend_from_slice(
            format!(
                r#"{{"decision":"HIT","index":{},"timestamp":{}}}"#,
                i,
                1234567890 + i as u64
            )
            .as_bytes(),
        );
        if i < 99 {
            large_log.extend_from_slice(b",");
        }
    }
    large_log.extend_from_slice(b"]");

    let digest1 = compute_digest(query_id, &large_result, &large_log);
    let digest2 = compute_digest(query_id, &large_result, &large_log);

    assert_eq!(digest1.combined_digest, digest2.combined_digest);
}

/// Test 6: Different result bytes produce different digests
#[test]
fn test_different_results_produce_different_digests() {
    let query_id = "q_diff_test";
    let result_a = b"result version A";
    let result_b = b"result version B";
    let cache_log = b"[{\"decision\":\"MISS\"}]";

    let digest_a = compute_digest(query_id, result_a, cache_log);
    let digest_b = compute_digest(query_id, result_b, cache_log);

    assert_ne!(digest_a.result_hash, digest_b.result_hash);
    assert_ne!(digest_a.combined_digest, digest_b.combined_digest);
    // cache_log_hash should be the same since cache log is identical
    assert_eq!(digest_a.cache_log_hash, digest_b.cache_log_hash);
}

/// Test 7: Different cache logs produce different digests
#[test]
fn test_different_cache_logs_produce_different_digests() {
    let query_id = "q_cache_log_test";
    let result = b"same result";
    let log_hit = b"[{\"decision\":\"HIT\"}]";
    let log_miss = b"[{\"decision\":\"MISS\"}]";

    let digest_hit = compute_digest(query_id, result, log_hit);
    let digest_miss = compute_digest(query_id, result, log_miss);

    assert_eq!(digest_hit.result_hash, digest_miss.result_hash);
    assert_ne!(digest_hit.cache_log_hash, digest_miss.cache_log_hash);
    assert_ne!(digest_hit.combined_digest, digest_miss.combined_digest);
}

/// Test 8: Combined digest depends on both result and cache log
#[test]
fn test_combined_digest_depends_on_both_inputs() {
    let query_id = "q_combined_test";
    let result = b"test result";
    let log = b"test log";

    let baseline = compute_digest(query_id, result, log);

    // Change only result
    let changed_result = compute_digest(query_id, b"different result", log);
    assert_ne!(baseline.combined_digest, changed_result.combined_digest);

    // Change only log
    let changed_log = compute_digest(query_id, result, b"different log");
    assert_ne!(baseline.combined_digest, changed_log.combined_digest);

    // Both same
    let same = compute_digest(query_id, result, log);
    assert_eq!(baseline.combined_digest, same.combined_digest);
}

/// Test 9: Query ID is preserved in digest
#[test]
fn test_query_id_preserved_in_digest() {
    let result = b"result";
    let log = b"log";

    let digest1 = compute_digest("q_query_id_1", result, log);
    let digest2 = compute_digest("q_query_id_2", result, log);

    // Query IDs should be different
    assert_eq!(digest1.query_id, "q_query_id_1");
    assert_eq!(digest2.query_id, "q_query_id_2");

    // But hashes should be same (query_id is not part of hash computation)
    assert_eq!(digest1.result_hash, digest2.result_hash);
    assert_eq!(digest1.cache_log_hash, digest2.cache_log_hash);
    assert_eq!(digest1.combined_digest, digest2.combined_digest);
}

/// Test 10: Digest structure serialization round-trip
#[test]
fn test_digest_serialization_roundtrip() {
    let original = compute_digest("q_serialize_test", b"data", b"log");

    // Serialize to JSON
    let json = serde_json::to_string(&original).expect("Serialization failed");

    // Deserialize back
    let deserialized: DeterminismDigest =
        serde_json::from_str(&json).expect("Deserialization failed");

    // Verify all fields match
    assert_eq!(original.query_id, deserialized.query_id);
    assert_eq!(original.result_hash, deserialized.result_hash);
    assert_eq!(original.cache_log_hash, deserialized.cache_log_hash);
    assert_eq!(original.combined_digest, deserialized.combined_digest);
}

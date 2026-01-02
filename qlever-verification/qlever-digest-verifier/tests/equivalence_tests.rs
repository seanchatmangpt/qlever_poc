//! EPIC 11 Equivalence Tests (Invariant B2)
//!
//! Validation of three equivalence rules:
//! 1. Replay equivalence: baseline vs cached results
//! 2. Cross-machine equivalence: same workload on different machines
//! 3. Cache behavior equivalence: decision log ordering and sequences

use qlever_digest_verifier::{
    compute_digest, verify_digest, MachineInfo,
};
use qlever_digest_verifier::equivalence_rules::{
    EquivalenceRule, EquivalenceRuleType, EquivalenceRuleBuilder,
};

/// Test 1: Replay equivalence - baseline and cached execution produce same result hash
#[test]
fn test_replay_equivalence_baseline_vs_cached() {
    let query_id = "q_replay_eq_test_1";
    let result_bytes = b"SELECT ?x WHERE { ?x a <http://example.org/Person> }";
    let cache_log = b"[{\"decision\":\"MISS\",\"reason\":\"not_in_cache\"}]";

    // Baseline execution
    let baseline_digest = compute_digest(query_id, result_bytes, cache_log);

    // Cached execution (same result, same cache log)
    let cached_digest = compute_digest(query_id, result_bytes, cache_log);

    // Verify replay equivalence
    let rule = EquivalenceRule::new(
        EquivalenceRuleType::Replay,
        query_id,
        baseline_digest.clone(),
        cached_digest.clone(),
    );

    let result = rule.validate_replay();
    assert!(result.is_ok(), "Replay equivalence failed");
    assert_eq!(baseline_digest.result_hash, cached_digest.result_hash);
}

/// Test 2: Replay equivalence detects divergence
#[test]
fn test_replay_equivalence_detects_divergence() {
    let query_id = "q_replay_eq_test_2";

    // Baseline: one result
    let baseline_digest = compute_digest(query_id, b"baseline result", b"log");

    // Cached: different result (divergence)
    let cached_digest = compute_digest(query_id, b"different result", b"log");

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::Replay,
        query_id,
        baseline_digest,
        cached_digest,
    );

    let result = rule.validate_replay();
    assert!(
        result.is_err(),
        "Replay equivalence should detect divergence"
    );
}

/// Test 3: Cross-machine equivalence - same workload on different machines
#[test]
fn test_cross_machine_equivalence_same_workload() {
    let query_id = "q_cross_machine_test_1";
    let result_bytes = b"SELECT * FROM graph";
    let cache_log = b"[{\"decision\":\"HIT\",\"cache_tier\":\"bytes\"}]";

    // Machine 1 (x86_64 + Linux)
    let mut digest_m1 = compute_digest(query_id, result_bytes, cache_log);
    digest_m1.machine_fingerprint = MachineInfo {
        cpu_model: "Intel Xeon".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    // Machine 2 (x86_64 + Linux) - same configuration
    let mut digest_m2 = compute_digest(query_id, result_bytes, cache_log);
    digest_m2.machine_fingerprint = MachineInfo {
        cpu_model: "Intel Xeon".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    // Verify cross-machine equivalence
    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CrossMachine,
        query_id,
        digest_m1,
        digest_m2,
    );

    let result = rule.validate_cross_machine();
    assert!(result.is_ok(), "Cross-machine equivalence failed");
}

/// Test 4: Cross-machine equivalence detects divergence
#[test]
fn test_cross_machine_equivalence_detects_divergence() {
    let query_id = "q_cross_machine_test_2";

    // Machine 1: one result
    let digest_m1 = compute_digest(query_id, b"result from M1", b"log");

    // Machine 2: different result (e.g., from SIMD difference)
    let digest_m2 = compute_digest(query_id, b"result from M2", b"log");

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CrossMachine,
        query_id,
        digest_m1,
        digest_m2,
    );

    let result = rule.validate_cross_machine();
    assert!(
        result.is_err(),
        "Cross-machine equivalence should detect divergence"
    );
}

/// Test 5: Cache behavior equivalence - exact cache decision log matching
#[test]
fn test_cache_behavior_equivalence_exact_log_match() {
    let query_id = "q_cache_behavior_test_1";
    let result_bytes = b"query result";

    // Cache log with specific decision sequence
    let cache_log = b"[{\"decision\":\"MISS\"},{\"decision\":\"ADMIT\"}]";

    let digest1 = compute_digest(query_id, result_bytes, cache_log);
    let digest2 = compute_digest(query_id, result_bytes, cache_log);

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CacheBehavior,
        query_id,
        digest1,
        digest2,
    );

    let result = rule.validate_cache_behavior();
    assert!(result.is_ok(), "Cache behavior equivalence failed");
}

/// Test 6: Cache behavior equivalence is order-sensitive
#[test]
fn test_cache_behavior_equivalence_order_matters() {
    let query_id = "q_cache_behavior_test_2";
    let result_bytes = b"query result";

    // First: MISS then ADMIT
    let log1 = b"[{\"decision\":\"MISS\"},{\"decision\":\"ADMIT\"}]";
    let digest1 = compute_digest(query_id, result_bytes, log1);

    // Second: ADMIT then MISS (reversed order)
    let log2 = b"[{\"decision\":\"ADMIT\"},{\"decision\":\"MISS\"}]";
    let digest2 = compute_digest(query_id, result_bytes, log2);

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CacheBehavior,
        query_id,
        digest1,
        digest2,
    );

    let result = rule.validate_cache_behavior();
    assert!(
        result.is_err(),
        "Cache behavior equivalence should detect order difference"
    );
}

/// Test 7: Cache behavior equivalence detects log divergence
#[test]
fn test_cache_behavior_equivalence_detects_log_divergence() {
    let query_id = "q_cache_behavior_test_3";
    let result_bytes = b"query result";

    let log_a = b"[{\"decision\":\"HIT\"}]";
    let log_b = b"[{\"decision\":\"MISS\"}]";

    let digest_a = compute_digest(query_id, result_bytes, log_a);
    let digest_b = compute_digest(query_id, result_bytes, log_b);

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CacheBehavior,
        query_id,
        digest_a,
        digest_b,
    );

    let result = rule.validate_cache_behavior();
    assert!(
        result.is_err(),
        "Cache behavior equivalence should detect log divergence"
    );
}

/// Test 8: Equivalence rule builder pattern
#[test]
fn test_equivalence_rule_builder() {
    let digest1 = compute_digest("q_builder_test", b"result", b"log");
    let digest2 = compute_digest("q_builder_test", b"result", b"log");

    let rule = EquivalenceRuleBuilder::new()
        .with_rule_type(EquivalenceRuleType::Replay)
        .with_query_id("q_builder_test")
        .with_expected(digest1)
        .with_actual(digest2)
        .build()
        .expect("Builder failed");

    assert_eq!(rule.rule_type, EquivalenceRuleType::Replay);
    assert!(rule.validate().is_ok());
}

/// Test 9: All three equivalence rules on same digest pair
#[test]
fn test_all_three_equivalence_rules() {
    let query_id = "q_all_rules_test";
    let digest1 = compute_digest(query_id, b"result", b"log");
    let digest2 = compute_digest(query_id, b"result", b"log");

    // Test replay equivalence
    let replay_rule = EquivalenceRule::new(
        EquivalenceRuleType::Replay,
        query_id,
        digest1.clone(),
        digest2.clone(),
    );
    assert!(replay_rule.validate().is_ok());

    // Test cross-machine equivalence
    let cross_machine_rule = EquivalenceRule::new(
        EquivalenceRuleType::CrossMachine,
        query_id,
        digest1.clone(),
        digest2.clone(),
    );
    assert!(cross_machine_rule.validate().is_ok());

    // Test cache behavior equivalence
    let cache_behavior_rule = EquivalenceRule::new(
        EquivalenceRuleType::CacheBehavior,
        query_id,
        digest1,
        digest2,
    );
    assert!(cache_behavior_rule.validate().is_ok());
}

/// Test 10: Equivalence rule descriptions are non-empty
#[test]
fn test_equivalence_rule_descriptions() {
    let digest = compute_digest("q_desc_test", b"result", b"log");

    let replay_rule = EquivalenceRule::new(
        EquivalenceRuleType::Replay,
        "q_desc_test",
        digest.clone(),
        digest.clone(),
    );
    assert!(!replay_rule.description().is_empty());
    assert!(replay_rule.description().contains("Replay"));

    let cross_machine_rule = EquivalenceRule::new(
        EquivalenceRuleType::CrossMachine,
        "q_desc_test",
        digest.clone(),
        digest.clone(),
    );
    assert!(!cross_machine_rule.description().is_empty());
    assert!(cross_machine_rule.description().contains("Cross-machine"));

    let cache_behavior_rule = EquivalenceRule::new(
        EquivalenceRuleType::CacheBehavior,
        "q_desc_test",
        digest.clone(),
        digest,
    );
    assert!(!cache_behavior_rule.description().is_empty());
    assert!(cache_behavior_rule.description().contains("Cache behavior"));
}

/// Test 11: Machine fingerprint differentiation
#[test]
fn test_machine_fingerprint_differentiation() {
    let query_id = "q_machine_fp_test";
    let result_bytes = b"result";
    let cache_log = b"log";

    // Create two digests with different machine info
    let mut digest_x86 = compute_digest(query_id, result_bytes, cache_log);
    digest_x86.machine_fingerprint = MachineInfo {
        cpu_model: "Intel Xeon".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    let mut digest_arm = compute_digest(query_id, result_bytes, cache_log);
    digest_arm.machine_fingerprint = MachineInfo {
        cpu_model: "Cortex-A72".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "aarch64".to_string(),
    };

    // Machine fingerprints should be different
    assert_ne!(
        digest_x86.machine_fingerprint.machine_id(),
        digest_arm.machine_fingerprint.machine_id()
    );

    // But combined digests should be same (machine info is not part of hash)
    assert_eq!(digest_x86.combined_digest, digest_arm.combined_digest);
}

/// Test 12: Cross-machine equivalence with different OS
#[test]
fn test_cross_machine_equivalence_different_os() {
    let query_id = "q_cross_os_test";
    let result_bytes = b"SELECT ?x WHERE { ?x type Person }";
    let cache_log = b"[{\"decision\":\"HIT\"}]";

    // Linux machine
    let mut digest_linux = compute_digest(query_id, result_bytes, cache_log);
    digest_linux.machine_fingerprint = MachineInfo {
        cpu_model: "Intel i7".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    // macOS machine (same x86_64 arch but different OS)
    let mut digest_macos = compute_digest(query_id, result_bytes, cache_log);
    digest_macos.machine_fingerprint = MachineInfo {
        cpu_model: "Intel i7".to_string(),
        os_name: "macOS".to_string(),
        libc_version: "musl-1.2".to_string(),
        architecture: "x86_64".to_string(),
    };

    // Digests should match (OS not part of hash)
    assert_eq!(digest_linux.combined_digest, digest_macos.combined_digest);

    let rule = EquivalenceRule::new(
        EquivalenceRuleType::CrossMachine,
        query_id,
        digest_linux,
        digest_macos,
    );

    assert!(rule.validate_cross_machine().is_ok());
}

/// Test 13: Machine ID formatting
#[test]
fn test_machine_id_formatting() {
    let machine_info = MachineInfo {
        cpu_model: "Intel Xeon".to_string(),
        os_name: "Linux".to_string(),
        libc_version: "glibc-2.35".to_string(),
        architecture: "x86_64".to_string(),
    };

    let id = machine_info.machine_id();
    assert_eq!(id, "x86_64-Linux-glibc-2.35-Intel Xeon");
}

/// Test 14: Equivalence rule verification integration
#[test]
fn test_equivalence_rule_with_verify_digest() {
    let query_id = "q_integration_test";
    let result_bytes = b"integration test result";
    let cache_log = b"[{\"decision\":\"ADMIT\"}]";

    let digest1 = compute_digest(query_id, result_bytes, cache_log);
    let digest2 = compute_digest(query_id, result_bytes, cache_log);

    // Basic verify_digest
    let verify_result = verify_digest(&digest1, &digest2);
    assert!(verify_result.is_ok());

    // Also verify with equivalence rules
    for rule_type in &[
        EquivalenceRuleType::Replay,
        EquivalenceRuleType::CrossMachine,
        EquivalenceRuleType::CacheBehavior,
    ] {
        let rule = EquivalenceRule::new(*rule_type, query_id, digest1.clone(), digest2.clone());
        assert!(rule.validate().is_ok());
    }
}

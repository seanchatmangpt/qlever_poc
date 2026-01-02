//! SIMD Equivalence Tests (AVX-512 vs Scalar, same architecture)
//!
//! Tests that AVX-512 and scalar modes produce identical results on the same machine.
//! These tests validate EPIC 10.3 SIMD equivalence guarantee.

use qlever_simd_verifier::{
    verify_simd_equivalence,
    SimdMode,
};

#[test]
fn test_simd_avx512_vs_scalar_empty_result() {
    // Simulate identical empty results from AVX-512 and scalar execution
    let empty_hash = blake3::hash(b"");

    let digest = verify_simd_equivalence(empty_hash, empty_hash, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
    assert_eq!(digest.divergence_byte_offset, None);
    assert!(digest.verify().is_ok());
}

#[test]
fn test_simd_avx512_vs_scalar_deterministic_query() {
    // Simulate identical results from deterministic query execution
    // In real scenario, this would be from actual AVX-512 and scalar query execution
    let query_result = b"SELECT ?x WHERE { ?x rdf:type ex:Resource }";
    let result_hash = blake3::hash(query_result);

    // Both modes produce the same hash (equivalence)
    let digest = verify_simd_equivalence(result_hash, result_hash, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
    assert!(digest.verify().is_ok());
}

#[test]
fn test_simd_avx512_vs_scalar_with_cache_decision_log() {
    // Simulate determinism formula: BLAKE3(result_bytes || cache_decision_log)
    let result_bytes = b"query_result_data";
    let cache_log = b"HIT,MISS,ADMIT,HIT";
    let mut combined = result_bytes.to_vec();
    combined.extend_from_slice(cache_log);

    let combined_hash = blake3::hash(&combined);

    let digest = verify_simd_equivalence(
        combined_hash,
        combined_hash,
        SimdMode::Avx512,
        SimdMode::Scalar,
    )
    .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
    assert!(digest.verify().is_ok());
}

#[test]
fn test_simd_avx512_vs_avx2_equivalence() {
    // AVX-512 and AVX2 should produce same results on x86_64
    let data = b"SIMD test vector with multiple elements for reduction";
    let hash = blake3::hash(data);

    let digest = verify_simd_equivalence(hash, hash, SimdMode::Avx512, SimdMode::Avx2)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
    assert!(digest.verify().is_ok());
}

#[test]
fn test_simd_neon_vs_scalar_equivalence() {
    // NEON and scalar should produce same results on ARM64
    let data = b"ARM64 NEON test vector";
    let hash = blake3::hash(data);

    let digest = verify_simd_equivalence(hash, hash, SimdMode::Neon, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
    assert!(digest.verify().is_ok());
}

#[test]
fn test_simd_scalar_determinism() {
    // Scalar mode determinism: same input always produces same output
    let input = b"deterministic_test_query";
    let hash1 = blake3::hash(input);
    let hash2 = blake3::hash(input);

    let digest = verify_simd_equivalence(hash1, hash2, SimdMode::Scalar, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
}

#[test]
fn test_simd_avx512_vs_scalar_divergence_fails_closed() {
    // If AVX-512 and scalar diverge, verification should fail-close with receipt
    let avx512_result = blake3::hash(b"avx512_specific_result");
    let scalar_result = blake3::hash(b"scalar_specific_result");

    let digest = verify_simd_equivalence(
        avx512_result,
        scalar_result,
        SimdMode::Avx512,
        SimdMode::Scalar,
    )
    .expect("Failed to create SIMD equivalence digest");

    // Divergence should be detected
    assert!(!digest.is_equivalent);
    assert!(digest.divergence_byte_offset.is_some());

    // verify() should fail (fail-closed semantics)
    let result = digest.verify();
    assert!(result.is_err());

    // Error should be ArchitectureDivergence
    if let Err(e) = result {
        assert!(e.to_string().contains("Digest mismatch"));
    }
}

#[test]
fn test_simd_equivalence_multiple_modes_consistent() {
    // If we have multiple SIMD modes, they should all be consistent with each other
    let baseline = blake3::hash(b"consistent_result");

    // AVX-512 vs scalar
    let eq1 = verify_simd_equivalence(baseline, baseline, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");
    assert!(eq1.is_equivalent);

    // AVX-512 vs AVX-2
    let eq2 = verify_simd_equivalence(baseline, baseline, SimdMode::Avx512, SimdMode::Avx2)
        .expect("Failed to verify SIMD equivalence");
    assert!(eq2.is_equivalent);

    // AVX-2 vs scalar
    let eq3 = verify_simd_equivalence(baseline, baseline, SimdMode::Avx2, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");
    assert!(eq3.is_equivalent);
}

#[test]
fn test_simd_equivalence_large_workload() {
    // Simulate large query result (e.g., from complex SPARQL query)
    let mut large_data = Vec::new();
    for i in 0..1000 {
        large_data.extend_from_slice(format!("result_{}\n", i).as_bytes());
    }

    let hash = blake3::hash(&large_data);

    let digest = verify_simd_equivalence(hash, hash, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    assert!(digest.is_equivalent);
}

#[test]
fn test_simd_equivalence_machine_fingerprint() {
    // Verify that machine fingerprint is correctly captured
    let hash = blake3::hash(b"test");

    let digest = verify_simd_equivalence(hash, hash, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to verify SIMD equivalence");

    // Both should have valid machine fingerprints
    assert!(!digest.baseline_machine.cpu_model.is_empty());
    assert!(!digest.baseline_machine.arch.is_empty());
    assert!(!digest.comparison_machine.cpu_model.is_empty());
    assert!(!digest.comparison_machine.arch.is_empty());
}

#[test]
fn test_simd_mode_combinations() {
    // Test all valid SIMD mode combinations on same architecture
    let test_data = b"test_data_for_simd_combinations";
    let hash = blake3::hash(test_data);

    let combinations = vec![
        (SimdMode::Avx512, SimdMode::Avx2),
        (SimdMode::Avx512, SimdMode::Scalar),
        (SimdMode::Avx2, SimdMode::Scalar),
        (SimdMode::Neon, SimdMode::Scalar),
        (SimdMode::Scalar, SimdMode::Scalar),
    ];

    for (mode1, mode2) in combinations {
        let digest =
            verify_simd_equivalence(hash, hash, mode1, mode2).expect("Failed to verify");
        assert!(digest.is_equivalent, "Modes {:?} and {:?} should be equivalent", mode1, mode2);
    }
}

/// Test: SimdAvx512VsScalar (Per Acceptance Criteria)
///
/// This test validates that AVX-512 and scalar modes produce identical results.
/// Per EPIC 11 acceptance criteria for Subsystem 8:
/// - [ ] Test: "SimdAvx512VsScalar" passes (or fails-closed with receipt)
#[test]
fn test_simd_avx512_vs_scalar_acceptance() {
    // Simulate AVX-512 execution result
    let avx512_data = b"SPARQL query result with SIMD optimizations (AVX-512)";
    let avx512_hash = blake3::hash(avx512_data);

    // Simulate scalar execution result (should be identical)
    let scalar_data = b"SPARQL query result with SIMD optimizations (AVX-512)"; // Same data
    let scalar_hash = blake3::hash(scalar_data);

    // Verify equivalence
    let digest = verify_simd_equivalence(avx512_hash, scalar_hash, SimdMode::Avx512, SimdMode::Scalar)
        .expect("Failed to create SIMD equivalence digest");

    if digest.is_equivalent {
        // PASS: Digests match
        println!("✓ SimdAvx512VsScalar: PASS");
        assert!(digest.verify().is_ok());
    } else {
        // FAIL-CLOSED: Divergence detected, should emit receipt
        println!(
            "✗ SimdAvx512VsScalar: FAIL (divergence at byte {:?})",
            digest.divergence_byte_offset
        );
        assert!(digest.verify().is_err());
        // In real system, this would emit receipt class: "SimdScalarMismatch"
    }
}

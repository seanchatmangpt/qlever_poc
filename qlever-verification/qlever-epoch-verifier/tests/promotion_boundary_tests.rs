//! Integration tests for promotion/swap boundary enforcement
//!
//! Tests SIMD epoch-mortality and promotion invariants.
//! Implements test category from EPIC 11 Invariant A4.

use qlever_epoch_verifier::{
    CacheKeyWithEpoch, CacheResultMeta, CacheTier, Epoch, EpochError, verify_promotion_boundary,
};

/// Helper to create a test epoch
fn create_epoch(gen_id: u64, seed: u8) -> Epoch {
    let mut epoch_key = [0u8; 32];
    epoch_key[0] = seed;
    Epoch::new(gen_id, epoch_key, gen_id * 100, gen_id * 1_000_000_000)
}

#[test]
fn test_promotion_boundary_enforced() {
    // EPIC 11 A4: No query result from epoch E_{i-1} can be accessible in E_{i+1}
    let source_epoch = create_epoch(1, 1);
    let target_epoch = create_epoch(2, 2);

    // Create non-immortal cache entry
    let cache_key = CacheKeyWithEpoch::new(&source_epoch, [0u8; 32], CacheTier::Bytes);

    let non_immortal_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false, // Explicitly mortal
    };

    // Promotion should be rejected
    let err = verify_promotion_boundary(&source_epoch, &target_epoch, &non_immortal_result)
        .unwrap_err();
    assert!(matches!(err, EpochError::PromotionBoundaryViolation(1, 2)));
}

#[test]
fn test_simd_results_cannot_cross_boundaries() {
    // EPIC 11 A4: SIMD-computed results CANNOT cross epoch boundaries
    let source_epoch = create_epoch(1, 1);
    let target_epoch = create_epoch(2, 2);

    let cache_key = CacheKeyWithEpoch::new(&source_epoch, [0u8; 32], CacheTier::Bytes);

    // Even if marked immortal, SIMD results cannot cross
    let simd_immortal_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: true, // SIMD computed
        is_epoch_immortal: true, // Even with immortal marker
    };

    // Should still be rejected
    let err = verify_promotion_boundary(&source_epoch, &target_epoch, &simd_immortal_result)
        .unwrap_err();
    assert!(matches!(err, EpochError::SimdCrossEpoch));
}

#[test]
fn test_immortal_non_simd_promotion_allowed() {
    let source_epoch = create_epoch(1, 1);
    let target_epoch = create_epoch(2, 2);

    let cache_key = CacheKeyWithEpoch::new(&source_epoch, [0u8; 32], CacheTier::Bytes);

    // Non-SIMD, immortal result should be promotable
    let immortal_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false, // NOT SIMD
        is_epoch_immortal: true, // Explicitly immortal
    };

    // Should succeed
    assert!(verify_promotion_boundary(&source_epoch, &target_epoch, &immortal_result).is_ok());
}

#[test]
fn test_simd_mortality_invariant() {
    // EPIC 11 A4: SIMD results are always epoch-mortal
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);
    let epoch3 = create_epoch(3, 3);

    let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);

    let simd_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: true, // SIMD computed
        is_epoch_immortal: true, // Even if system tries to mark as immortal
    };

    // Should fail crossing epoch1->epoch2
    let err12 = verify_promotion_boundary(&epoch1, &epoch2, &simd_result).unwrap_err();
    assert!(matches!(err12, EpochError::SimdCrossEpoch));

    // Should fail crossing epoch2->epoch3
    let err23 = verify_promotion_boundary(&epoch2, &epoch3, &simd_result).unwrap_err();
    assert!(matches!(err23, EpochError::SimdCrossEpoch));
}

#[test]
fn test_multiple_cache_tiers_promotion() {
    let source_epoch = create_epoch(1, 1);
    let target_epoch = create_epoch(2, 2);

    // Test promotion across all cache tiers
    let tiers = vec![CacheTier::Bytes, CacheTier::Neg, CacheTier::Plan];

    for tier in tiers {
        // Non-immortal should fail
        let non_immortal_key = CacheKeyWithEpoch::new(&source_epoch, [1u8; 32], tier);
        let non_immortal = CacheResultMeta {
            cache_key: non_immortal_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: false,
        };

        let err = verify_promotion_boundary(&source_epoch, &target_epoch, &non_immortal)
            .unwrap_err();
        assert!(matches!(err, EpochError::PromotionBoundaryViolation(1, 2)));

        // Immortal should succeed
        let immortal_key = CacheKeyWithEpoch::new(&source_epoch, [2u8; 32], tier);
        let immortal = CacheResultMeta {
            cache_key: immortal_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: true,
        };

        assert!(verify_promotion_boundary(&source_epoch, &target_epoch, &immortal).is_ok());
    }
}

#[test]
fn test_sequential_epoch_promotion() {
    // Test promotion through multiple epoch boundaries
    let epoch0 = create_epoch(0, 0);
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);
    let epoch3 = create_epoch(3, 3);

    let cache_key = CacheKeyWithEpoch::new(&epoch0, [0u8; 32], CacheTier::Bytes);

    let immortal_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 0,
        is_simd_result: false,
        is_epoch_immortal: true,
    };

    // Immortal non-SIMD should promote through all boundaries
    assert!(verify_promotion_boundary(&epoch0, &epoch1, &immortal_result).is_ok());
    assert!(verify_promotion_boundary(&epoch1, &epoch2, &immortal_result).is_ok());
    assert!(verify_promotion_boundary(&epoch2, &epoch3, &immortal_result).is_ok());
}

#[test]
fn test_promotion_with_different_query_hashes() {
    let source_epoch = create_epoch(1, 1);
    let target_epoch = create_epoch(2, 2);

    // Different query hashes, all immortal
    let query_hashes = vec![[1u8; 32], [2u8; 32], [3u8; 32]];

    for hash in query_hashes {
        let cache_key = CacheKeyWithEpoch::new(&source_epoch, hash, CacheTier::Bytes);

        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: true,
        };

        // All should promote successfully
        assert!(verify_promotion_boundary(&source_epoch, &target_epoch, &result).is_ok());
    }
}

#[test]
fn test_simd_non_promotion_across_range() {
    // Verify SIMD mortality across entire epoch range
    let epoch_start = create_epoch(1, 1);
    let epoch_mid = create_epoch(1000, 1);
    let epoch_end = create_epoch(10000, 1);

    let cache_key = CacheKeyWithEpoch::new(&epoch_start, [0u8; 32], CacheTier::Bytes);

    let simd_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: true,
        is_epoch_immortal: true,
    };

    // Should fail across all ranges
    let err1 = verify_promotion_boundary(&epoch_start, &epoch_mid, &simd_result).unwrap_err();
    assert!(matches!(err1, EpochError::SimdCrossEpoch));

    let err2 = verify_promotion_boundary(&epoch_mid, &epoch_end, &simd_result).unwrap_err();
    assert!(matches!(err2, EpochError::SimdCrossEpoch));
}

#[test]
fn test_boundary_enforcement_matrix() {
    // Matrix test: all combinations of (SIMD, immortal) x (promotion)
    let source = create_epoch(1, 1);
    let target = create_epoch(2, 2);

    let cache_key = CacheKeyWithEpoch::new(&source, [0u8; 32], CacheTier::Bytes);

    // Test all 4 combinations
    let test_cases = vec![
        // (is_simd, is_immortal, should_promote)
        (false, false, false), // Non-SIMD, mortal -> blocks
        (false, true, true),   // Non-SIMD, immortal -> allows
        (true, false, false),  // SIMD, mortal -> blocks
        (true, true, false),   // SIMD, immortal -> still blocks (SIMD always mortal)
    ];

    for (is_simd, is_immortal, should_succeed) in test_cases {
        let result = CacheResultMeta {
            cache_key: cache_key.clone(),
            source_epoch_id: 1,
            is_simd_result: is_simd,
            is_epoch_immortal: is_immortal,
        };

        let promotion_result = verify_promotion_boundary(&source, &target, &result);

        if should_succeed {
            assert!(
                promotion_result.is_ok(),
                "Expected success for (SIMD={}, immortal={}) but got error: {:?}",
                is_simd,
                is_immortal,
                promotion_result
            );
        } else {
            assert!(
                promotion_result.is_err(),
                "Expected error for (SIMD={}, immortal={})",
                is_simd,
                is_immortal
            );
        }
    }
}

#[test]
fn test_boundary_enforcement_receipts() {
    // Verify that boundary violations are properly classified
    let source = create_epoch(1, 1);
    let target = create_epoch(2, 2);

    // Test SIMD violation
    let simd_cache = CacheKeyWithEpoch::new(&source, [0u8; 32], CacheTier::Bytes);
    let simd_result = CacheResultMeta {
        cache_key: simd_cache,
        source_epoch_id: 1,
        is_simd_result: true,
        is_epoch_immortal: true,
    };

    let simd_err = verify_promotion_boundary(&source, &target, &simd_result).unwrap_err();
    assert_eq!(
        simd_err.to_failure_class(),
        qlever_epoch_verifier::FailureClass::PromotionBoundaryViolation
    );

    // Test non-immortal violation
    let mortal_cache = CacheKeyWithEpoch::new(&source, [1u8; 32], CacheTier::Bytes);
    let mortal_result = CacheResultMeta {
        cache_key: mortal_cache,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    let mortal_err = verify_promotion_boundary(&source, &target, &mortal_result).unwrap_err();
    assert_eq!(
        mortal_err.to_failure_class(),
        qlever_epoch_verifier::FailureClass::PromotionBoundaryViolation
    );
}

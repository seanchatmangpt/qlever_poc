//! Integration tests for epoch isolation verification
//!
//! Tests cross-epoch cache contamination detection.
//! Implements test category from EPIC 11 Invariant A.

use qlever_epoch_verifier::{
    CacheKeyWithEpoch, CacheResultMeta, CacheTier, Epoch, EpochError, verify_isolation,
};

/// Helper to create a test epoch
fn create_epoch(gen_id: u64, seed: u8) -> Epoch {
    let mut epoch_key = [0u8; 32];
    epoch_key[0] = seed;
    Epoch::new(gen_id, epoch_key, gen_id * 100, gen_id * 1_000_000_000)
}

#[test]
fn test_epoch_contamination_detected() {
    // Create two distinct epochs
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);

    // Create a cache key bound to epoch 1
    let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);

    // Try to use epoch1's cache key in epoch2 context
    let result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    // Should detect contamination
    let err = verify_isolation(&epoch2, &result).unwrap_err();
    assert!(matches!(err, EpochError::EpochContamination(2, 1)));
}

#[test]
fn test_same_epoch_cache_valid() {
    let epoch = create_epoch(1, 1);
    let cache_key = CacheKeyWithEpoch::new(&epoch, [0u8; 32], CacheTier::Bytes);

    let result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    // Should pass - same epoch
    assert!(verify_isolation(&epoch, &result).is_ok());
}

#[test]
fn test_multiple_cache_tiers_isolation() {
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);

    // Test isolation across all cache tiers
    let tiers = vec![CacheTier::Bytes, CacheTier::Neg, CacheTier::Plan];

    for tier in tiers {
        let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], tier);

        let result = CacheResultMeta {
            cache_key,
            source_epoch_id: 1,
            is_simd_result: false,
            is_epoch_immortal: false,
        };

        // All tiers should be isolated per epoch
        let err = verify_isolation(&epoch2, &result).unwrap_err();
        assert!(matches!(err, EpochError::EpochContamination(2, 1)));
    }
}

#[test]
fn test_different_query_hashes_same_epoch() {
    let epoch = create_epoch(1, 1);

    // Different query hashes in same epoch should not contaminate
    let cache_key1 = CacheKeyWithEpoch::new(&epoch, [1u8; 32], CacheTier::Bytes);
    let cache_key2 = CacheKeyWithEpoch::new(&epoch, [2u8; 32], CacheTier::Bytes);
    let _ = cache_key2; // use it in scope

    let result1 = CacheResultMeta {
        cache_key: cache_key1,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    let result2 = CacheResultMeta {
        cache_key: cache_key2,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    // Both should pass
    assert!(verify_isolation(&epoch, &result1).is_ok());
    assert!(verify_isolation(&epoch, &result2).is_ok());
}

#[test]
fn test_sequential_epoch_progression() {
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);
    let epoch3 = create_epoch(3, 3);

    // Create cache in epoch1
    let cache_key = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);
    let _ = epoch1; // use it in scope

    let result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    // Should fail in epoch2
    let err2 = verify_isolation(&epoch2, &result).unwrap_err();
    assert!(matches!(err2, EpochError::EpochContamination(2, 1)));

    // Should fail in epoch3
    let err3 = verify_isolation(&epoch3, &result).unwrap_err();
    assert!(matches!(err3, EpochError::EpochContamination(3, 1)));
}

#[test]
fn test_simd_result_cross_epoch_blocked() {
    let _epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);

    // Create a cache key bound to epoch2, but source from epoch1
    let cache_key = CacheKeyWithEpoch::new(&epoch2, [0u8; 32], CacheTier::Bytes);

    let simd_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1, // Source from epoch 1
        is_simd_result: true, // SIMD computed
        is_epoch_immortal: false,
    };

    // SIMD results should always be blocked from crossing epochs
    let err = verify_isolation(&epoch2, &simd_result).unwrap_err();
    assert!(matches!(err, EpochError::SimdCrossEpoch));
}

#[test]
fn test_epoch_key_prefix_verification() {
    let epoch1 = create_epoch(1, 1);
    let epoch2 = create_epoch(2, 2);

    // Verify that cache keys maintain correct epoch prefixes
    let cache_key1 = CacheKeyWithEpoch::new(&epoch1, [0u8; 32], CacheTier::Bytes);
    let cache_key2 = CacheKeyWithEpoch::new(&epoch2, [0u8; 32], CacheTier::Bytes);

    // Cache keys should have different prefixes
    assert_ne!(cache_key1.epoch_prefix, cache_key2.epoch_prefix);

    // Prefix should match first 8 bytes of epoch key
    assert_eq!(cache_key1.epoch_prefix, epoch1.epoch_prefix());
    assert_eq!(cache_key2.epoch_prefix, epoch2.epoch_prefix());
}

#[test]
fn test_immortal_cache_in_same_epoch() {
    let epoch = create_epoch(1, 1);
    let cache_key = CacheKeyWithEpoch::new(&epoch, [0u8; 32], CacheTier::Bytes);

    let immortal_result = CacheResultMeta {
        cache_key,
        source_epoch_id: 1,
        is_simd_result: false,
        is_epoch_immortal: true, // Marked as immortal
    };

    // Should pass - same epoch regardless of immortality status
    assert!(verify_isolation(&epoch, &immortal_result).is_ok());
}

#[test]
fn test_epoch_boundaries() {
    let epoch_zero = create_epoch(0, 0);
    let epoch_large = create_epoch(1_000_000, 255);

    // Create caches at opposite ends of epoch range
    let cache_key_zero = CacheKeyWithEpoch::new(&epoch_zero, [0u8; 32], CacheTier::Bytes);
    let _cache_key_large = CacheKeyWithEpoch::new(&epoch_large, [0u8; 32], CacheTier::Bytes);

    // Extreme case: cache from epoch 0 accessed in large epoch
    let result_zero = CacheResultMeta {
        cache_key: cache_key_zero,
        source_epoch_id: 0,
        is_simd_result: false,
        is_epoch_immortal: false,
    };

    // Should still detect contamination across large range
    let err = verify_isolation(&epoch_large, &result_zero).unwrap_err();
    assert!(matches!(err, EpochError::EpochContamination(1_000_000, 0)));
}

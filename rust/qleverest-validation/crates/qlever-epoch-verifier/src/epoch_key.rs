//! Epoch key binding logic
//!
//! Implements the epoch key generation and binding logic
//! as specified in EPIC 11 Invariant A1.

use blake3;

/// Generate an epoch key from manifest and guard config digests
pub fn generate_epoch_key(manifest_digest: &[u8; 32], guard_config_digest: &[u8; 32]) -> [u8; 32] {
    let mut hasher = blake3::Hasher::new();
    hasher.update(manifest_digest);
    hasher.update(guard_config_digest);
    *hasher.finalize().as_bytes()
}

/// Extract epoch prefix from epoch key
pub fn extract_epoch_prefix(epoch_key: &[u8; 32]) -> [u8; 8] {
    let mut prefix = [0u8; 8];
    prefix.copy_from_slice(&epoch_key[0..8]);
    prefix
}

/// Verify that a cache key has the correct epoch prefix
pub fn verify_epoch_prefix(cache_key_prefix: &[u8; 8], epoch_key: &[u8; 32]) -> bool {
    cache_key_prefix == &epoch_key[0..8]
}

/// Generate a cache key with epoch binding
pub fn generate_cache_key(
    epoch_key: &[u8; 32],
    query_hash: &[u8; 32],
    cache_tier: u8,
) -> Vec<u8> {
    let mut key = Vec::with_capacity(41);
    key.extend_from_slice(&epoch_key[0..8]); // Epoch prefix
    key.extend_from_slice(query_hash);        // Query hash
    key.push(cache_tier);                     // Tier ID
    key
}

/// Parse a cache key to extract epoch prefix
pub fn parse_cache_key_epoch_prefix(cache_key: &[u8]) -> Option<[u8; 8]> {
    if cache_key.len() < 8 {
        return None;
    }
    let mut prefix = [0u8; 8];
    prefix.copy_from_slice(&cache_key[0..8]);
    Some(prefix)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_generate_epoch_key() {
        let manifest = [1u8; 32];
        let guard_config = [2u8; 32];

        let key1 = generate_epoch_key(&manifest, &guard_config);
        let key2 = generate_epoch_key(&manifest, &guard_config);

        // Deterministic
        assert_eq!(key1, key2);

        // Different inputs produce different keys
        let different_manifest = [3u8; 32];
        let key3 = generate_epoch_key(&different_manifest, &guard_config);
        assert_ne!(key1, key3);
    }

    #[test]
    fn test_epoch_prefix_extraction() {
        let epoch_key = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32];

        let prefix = extract_epoch_prefix(&epoch_key);
        assert_eq!(prefix, [1, 2, 3, 4, 5, 6, 7, 8]);
    }

    #[test]
    fn test_verify_epoch_prefix() {
        let epoch_key = [1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

        let correct_prefix = [1, 2, 3, 4, 5, 6, 7, 8];
        let wrong_prefix = [8, 7, 6, 5, 4, 3, 2, 1];

        assert!(verify_epoch_prefix(&correct_prefix, &epoch_key));
        assert!(!verify_epoch_prefix(&wrong_prefix, &epoch_key));
    }

    #[test]
    fn test_generate_cache_key() {
        let epoch_key = [1u8; 32];
        let query_hash = [2u8; 32];
        let cache_tier = 0u8;

        let cache_key = generate_cache_key(&epoch_key, &query_hash, cache_tier);

        assert_eq!(cache_key.len(), 41);
        assert_eq!(&cache_key[0..8], &epoch_key[0..8]);
        assert_eq!(&cache_key[8..40], &query_hash[..]);
        assert_eq!(cache_key[40], cache_tier);
    }

    #[test]
    fn test_parse_cache_key() {
        let cache_key = vec![1, 2, 3, 4, 5, 6, 7, 8, 0, 0];

        let prefix = parse_cache_key_epoch_prefix(&cache_key).unwrap();
        assert_eq!(prefix, [1, 2, 3, 4, 5, 6, 7, 8]);
    }

    #[test]
    fn test_parse_invalid_cache_key() {
        let short_key = vec![1, 2, 3];
        assert!(parse_cache_key_epoch_prefix(&short_key).is_none());
    }
}

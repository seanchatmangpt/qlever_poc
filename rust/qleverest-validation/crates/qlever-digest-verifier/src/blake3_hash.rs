//! BLAKE3 hash computation
//!
//! Wrapper around the blake3 crate for consistent hashing across
//! all verification subsystems.

use serde::{Deserialize, Serialize};

/// BLAKE3 hash wrapper (32 bytes / 256 bits)
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Blake3Hash([u8; 32]);

impl Blake3Hash {
    /// Create from raw bytes
    pub fn from_bytes(bytes: [u8; 32]) -> Self {
        Self(bytes)
    }

    /// Get raw bytes
    pub fn as_bytes(&self) -> &[u8; 32] {
        &self.0
    }

    /// Convert to hex string
    pub fn to_hex(&self) -> String {
        hex::encode(&self.0)
    }

    /// Parse from hex string
    pub fn from_hex(s: &str) -> Option<Self> {
        let bytes = hex::decode(s).ok()?;
        if bytes.len() != 32 {
            return None;
        }
        let mut arr = [0u8; 32];
        arr.copy_from_slice(&bytes);
        Some(Self(arr))
    }
}

impl std::fmt::Debug for Blake3Hash {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Blake3Hash({})", &self.to_hex()[..16])
    }
}

impl std::fmt::Display for Blake3Hash {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.to_hex())
    }
}

impl Serialize for Blake3Hash {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_hex())
    }
}

impl<'de> Deserialize<'de> for Blake3Hash {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        Blake3Hash::from_hex(&s).ok_or_else(|| serde::de::Error::custom("invalid BLAKE3 hash hex"))
    }
}

/// Hash bytes using BLAKE3
pub fn hash_bytes(data: &[u8]) -> Blake3Hash {
    let hash = blake3::hash(data);
    Blake3Hash::from_bytes(*hash.as_bytes())
}

/// Hash multiple byte slices concatenated
pub fn hash_concat(slices: &[&[u8]]) -> Blake3Hash {
    let mut hasher = blake3::Hasher::new();
    for slice in slices {
        hasher.update(slice);
    }
    Blake3Hash::from_bytes(*hasher.finalize().as_bytes())
}

/// Incremental hasher for large data
pub struct IncrementalHasher {
    hasher: blake3::Hasher,
}

impl IncrementalHasher {
    pub fn new() -> Self {
        Self {
            hasher: blake3::Hasher::new(),
        }
    }

    pub fn update(&mut self, data: &[u8]) {
        self.hasher.update(data);
    }

    pub fn finalize(self) -> Blake3Hash {
        Blake3Hash::from_bytes(*self.hasher.finalize().as_bytes())
    }
}

impl Default for IncrementalHasher {
    fn default() -> Self {
        Self::new()
    }
}

// Re-export hex for convenience
mod hex {
    const HEX_CHARS: &[u8; 16] = b"0123456789abcdef";

    pub fn encode(data: &[u8]) -> String {
        let mut result = String::with_capacity(data.len() * 2);
        for byte in data {
            result.push(HEX_CHARS[(byte >> 4) as usize] as char);
            result.push(HEX_CHARS[(byte & 0x0f) as usize] as char);
        }
        result
    }

    pub fn decode(s: &str) -> Result<Vec<u8>, ()> {
        if s.len() % 2 != 0 {
            return Err(());
        }
        let mut result = Vec::with_capacity(s.len() / 2);
        for chunk in s.as_bytes().chunks(2) {
            let high = from_hex_char(chunk[0])?;
            let low = from_hex_char(chunk[1])?;
            result.push((high << 4) | low);
        }
        Ok(result)
    }

    fn from_hex_char(c: u8) -> Result<u8, ()> {
        match c {
            b'0'..=b'9' => Ok(c - b'0'),
            b'a'..=b'f' => Ok(c - b'a' + 10),
            b'A'..=b'F' => Ok(c - b'A' + 10),
            _ => Err(()),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_blake3_matches_reference() {
        // Test vector: empty input
        let hash = hash_bytes(b"");
        assert_eq!(
            hash.to_hex(),
            "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"
        );
    }

    #[test]
    fn test_blake3_hello_world() {
        let hash = hash_bytes(b"hello world");
        // Known BLAKE3 hash of "hello world"
        assert_eq!(
            hash.to_hex(),
            "d74981efa70a0c880b8d8c1985d075dbcbf679b99a5f9914e5aaf96b831a9e24"
        );
    }

    #[test]
    fn test_hash_concat() {
        let h1 = hash_concat(&[b"hello", b" ", b"world"]);
        let h2 = hash_bytes(b"hello world");
        assert_eq!(h1, h2);
    }

    #[test]
    fn test_incremental_hasher() {
        let mut hasher = IncrementalHasher::new();
        hasher.update(b"hello");
        hasher.update(b" ");
        hasher.update(b"world");
        let h1 = hasher.finalize();

        let h2 = hash_bytes(b"hello world");
        assert_eq!(h1, h2);
    }

    #[test]
    fn test_hex_roundtrip() {
        let hash = hash_bytes(b"test data");
        let hex_str = hash.to_hex();
        let parsed = Blake3Hash::from_hex(&hex_str).unwrap();
        assert_eq!(hash, parsed);
    }

    #[test]
    fn test_serialization() {
        let hash = hash_bytes(b"serialize me");
        let json = serde_json::to_string(&hash).unwrap();
        let parsed: Blake3Hash = serde_json::from_str(&json).unwrap();
        assert_eq!(hash, parsed);
    }
}

//! Cross-Architecture SIMD Equivalence Verification
//!
//! Validates that the same workload produces identical results on different architectures
//! (x86_64 with AVX-512 vs. ARM64 with NEON) and SIMD modes (AVX-512 vs. scalar).

use crate::{SimdError, SimdMode, SimdResult};
use blake3::Hash;
use serde::{Deserialize, Serialize};
use std::fmt;

/// Result of a cross-architecture comparison
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CrossArchResult {
    /// Architecture of first comparison (e.g., "x86_64")
    pub arch_a: String,
    /// Architecture of second comparison (e.g., "aarch64")
    pub arch_b: String,
    /// SIMD mode used on architecture A
    pub mode_a: SimdMode,
    /// SIMD mode used on architecture B
    pub mode_b: SimdMode,
    /// Digest from architecture A execution (as hex string for serialization)
    pub digest_a: String,
    /// Digest from architecture B execution (as hex string for serialization)
    pub digest_b: String,
    /// Whether digests are equivalent (zero tolerance)
    pub is_equivalent: bool,
    /// Byte offset where first divergence occurs (if not equivalent)
    pub divergence_offset: Option<usize>,
    /// Number of bytes compared
    pub bytes_compared: usize,
}

impl CrossArchResult {
    /// Create new cross-architecture result
    pub fn new(
        arch_a: String,
        arch_b: String,
        mode_a: SimdMode,
        mode_b: SimdMode,
        digest_a: Hash,
        digest_b: Hash,
    ) -> Self {
        let bytes_a = digest_a.as_bytes();
        let bytes_b = digest_b.as_bytes();
        let is_equivalent = bytes_a == bytes_b;
        let divergence_offset = if !is_equivalent {
            // Find first diverging byte between hashes
            bytes_a
                .iter()
                .zip(bytes_b.iter())
                .position(|(a, b)| a != b)
        } else {
            None
        };

        CrossArchResult {
            arch_a,
            arch_b,
            mode_a,
            mode_b,
            digest_a: digest_a.to_hex().to_string(),
            digest_b: digest_b.to_hex().to_string(),
            is_equivalent,
            divergence_offset,
            bytes_compared: bytes_a.len(),
        }
    }

    /// Verify cross-architecture equivalence (fail-closed if mismatch)
    pub fn verify(&self) -> SimdResult<()> {
        if !self.is_equivalent {
            return Err(SimdError::ArchitectureDivergence(format!(
                "Cross-architecture digest mismatch: {}({:?}) {} != {}({:?}) {}, divergence at byte {:?}",
                self.arch_a,
                self.mode_a,
                self.digest_a,
                self.arch_b,
                self.mode_b,
                self.digest_b,
                self.divergence_offset
            )));
        }
        Ok(())
    }

    /// Format result as evidence string for receipt
    pub fn as_evidence(&self) -> String {
        format!(
            "CrossArchComparison {{ arch_a: {}, mode_a: {}, digest_a: {}, arch_b: {}, mode_b: {}, digest_b: {}, equivalent: {}, divergence_byte: {:?} }}",
            self.arch_a,
            self.mode_a.as_str(),
            self.digest_a,
            self.arch_b,
            self.mode_b.as_str(),
            self.digest_b,
            self.is_equivalent,
            self.divergence_offset
        )
    }
}

impl fmt::Display for CrossArchResult {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "CrossArch: {} vs {}? {} (divergence at byte {:?})",
            self.arch_a, self.arch_b, self.is_equivalent, self.divergence_offset
        )
    }
}

/// Cross-architecture comparison state machine
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CrossArchComparison {
    /// Comparisons performed
    pub comparisons: Vec<CrossArchResult>,
    /// Total equivalences verified
    pub equivalence_count: usize,
    /// Total divergences detected
    pub divergence_count: usize,
    /// Whether all comparisons passed (fail-closed semantics)
    pub all_passed: bool,
}

impl CrossArchComparison {
    /// Create new cross-architecture comparison tracker
    pub fn new() -> Self {
        CrossArchComparison {
            comparisons: Vec::new(),
            equivalence_count: 0,
            divergence_count: 0,
            all_passed: true,
        }
    }

    /// Add a cross-architecture comparison result
    pub fn add_result(&mut self, result: CrossArchResult) -> SimdResult<()> {
        if result.is_equivalent {
            self.equivalence_count += 1;
        } else {
            self.divergence_count += 1;
            self.all_passed = false;
        }

        self.comparisons.push(result);
        Ok(())
    }

    /// Verify all comparisons (fail-closed if any mismatch)
    pub fn verify_all(&self) -> SimdResult<()> {
        if !self.all_passed {
            // Find first divergence for error reporting
            if let Some(divergent) = self
                .comparisons
                .iter()
                .find(|c| !c.is_equivalent)
            {
                return Err(SimdError::ArchitectureDivergence(format!(
                    "Cross-architecture verification failed: {} divergences detected. First: {}",
                    self.divergence_count, divergent
                )));
            }
        }
        Ok(())
    }

    /// Get summary statistics
    pub fn summary(&self) -> String {
        format!(
            "CrossArchComparison {{ total: {}, equivalences: {}, divergences: {}, all_passed: {} }}",
            self.comparisons.len(),
            self.equivalence_count,
            self.divergence_count,
            self.all_passed
        )
    }
}

impl Default for CrossArchComparison {
    fn default() -> Self {
        Self::new()
    }
}

impl fmt::Display for CrossArchComparison {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.summary())
    }
}

/// Compare two digests and produce cross-architecture result
pub fn compare_cross_arch(
    arch_a: String,
    arch_b: String,
    mode_a: SimdMode,
    mode_b: SimdMode,
    digest_a: Hash,
    digest_b: Hash,
) -> CrossArchResult {
    CrossArchResult::new(arch_a, arch_b, mode_a, mode_b, digest_a, digest_b)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cross_arch_result_equivalence() {
        let hash = blake3::hash(b"same_result");
        let result = CrossArchResult::new(
            "x86_64".to_string(),
            "aarch64".to_string(),
            SimdMode::Avx512,
            SimdMode::Neon,
            hash,
            hash,
        );

        assert!(result.is_equivalent);
        assert_eq!(result.divergence_offset, None);
        assert!(result.verify().is_ok());
    }

    #[test]
    fn test_cross_arch_result_divergence() {
        let hash_a = blake3::hash(b"result_x86");
        let hash_b = blake3::hash(b"result_arm");
        let result = CrossArchResult::new(
            "x86_64".to_string(),
            "aarch64".to_string(),
            SimdMode::Avx512,
            SimdMode::Neon,
            hash_a,
            hash_b,
        );

        assert!(!result.is_equivalent);
        assert!(result.divergence_offset.is_some());
        assert!(result.verify().is_err());
    }

    #[test]
    fn test_cross_arch_result_as_evidence() {
        let hash = blake3::hash(b"test");
        let result = CrossArchResult::new(
            "x86_64".to_string(),
            "aarch64".to_string(),
            SimdMode::Avx512,
            SimdMode::Neon,
            hash,
            hash,
        );

        let evidence = result.as_evidence();
        assert!(evidence.contains("x86_64"));
        assert!(evidence.contains("aarch64"));
        assert!(evidence.contains("avx512"));
        assert!(evidence.contains("neon"));
    }

    #[test]
    fn test_cross_arch_comparison_tracking() {
        let mut comparison = CrossArchComparison::new();
        let hash1 = blake3::hash(b"result1");
        let hash2 = blake3::hash(b"result1");
        let hash3 = blake3::hash(b"result3");

        let result1 = CrossArchResult::new(
            "x86_64".to_string(),
            "aarch64".to_string(),
            SimdMode::Avx512,
            SimdMode::Neon,
            hash1,
            hash2,
        );
        let result2 = CrossArchResult::new(
            "x86_64".to_string(),
            "aarch64".to_string(),
            SimdMode::Avx2,
            SimdMode::Scalar,
            hash2,
            hash3,
        );

        comparison.add_result(result1).unwrap();
        comparison.add_result(result2).unwrap();

        assert_eq!(comparison.comparisons.len(), 2);
        assert_eq!(comparison.equivalence_count, 1);
        assert_eq!(comparison.divergence_count, 1);
        assert!(!comparison.all_passed);
        assert!(comparison.verify_all().is_err());
    }

    #[test]
    fn test_cross_arch_comparison_all_passing() {
        let mut comparison = CrossArchComparison::new();
        let hash = blake3::hash(b"same_result");

        for i in 0..3 {
            let result = CrossArchResult::new(
                "x86_64".to_string(),
                "aarch64".to_string(),
                if i == 0 {
                    SimdMode::Avx512
                } else {
                    SimdMode::Scalar
                },
                SimdMode::Neon,
                hash,
                hash,
            );
            comparison.add_result(result).unwrap();
        }

        assert_eq!(comparison.comparisons.len(), 3);
        assert_eq!(comparison.equivalence_count, 3);
        assert_eq!(comparison.divergence_count, 0);
        assert!(comparison.all_passed);
        assert!(comparison.verify_all().is_ok());
    }
}

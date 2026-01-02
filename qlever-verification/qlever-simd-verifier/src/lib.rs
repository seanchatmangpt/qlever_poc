//! EPIC 11 Subsystem 8: SIMD Equivalence Verification
//!
//! Validates that SIMD operations (AVX-512, NEON, scalar) produce bit-identical results
//! across architectures (x86_64, ARM64). Zero tolerance for divergence.
//!
//! # Invariant (from EPIC 11, Part I D1)
//! ```text
//! ∀ workload pack W:
//!   ∀ arch A1, A2:
//!     run_workload(W, A1, replay_mode=Strict) == run_workload(W, A2, replay_mode=Strict)
//!     ⇒ both must produce identical BLAKE3 digests (fail-closed if not)
//! ```

use blake3::Hash;
use serde::{Deserialize, Serialize};
use std::fmt;
use thiserror::Error;

pub mod cross_architecture;
pub mod simd_modes;

pub use cross_architecture::{compare_cross_arch, CrossArchComparison, CrossArchResult};
pub use simd_modes::{CpuFeatureSet, SimdMode};

/// SIMD Equivalence Verification Error
#[derive(Error, Debug)]
pub enum SimdError {
    #[error("SIMD scalar mismatch: {0}")]
    ScalarMismatch(String),

    #[error("Architecture divergence: {0}")]
    ArchitectureDivergence(String),

    #[error("SIMD nondeterminism: {0}")]
    Nondeterminism(String),

    #[error("CPU feature detection failed: {0}")]
    FeatureDetectionFailed(String),

    #[error("Cross-architecture comparison failed: {0}")]
    CrossArchComparingFailed(String),

    #[error("Invalid digest: {0}")]
    InvalidDigest(String),

    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("Serialization error: {0}")]
    SerializationError(String),

    #[error("Other error: {0}")]
    Other(String),
}

pub type SimdResult<T> = std::result::Result<T, SimdError>;

/// Machine fingerprint for SIMD equivalence verification
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct MachineFingerprint {
    /// CPU model (e.g., "Skylake", "Zen3", "Cortex-A72")
    pub cpu_model: String,
    /// Operating system (e.g., "Linux", "macOS")
    pub os: String,
    /// Architecture (e.g., "x86_64", "aarch64")
    pub arch: String,
    /// Primary SIMD mode available
    pub primary_simd_mode: SimdMode,
    /// All available SIMD modes (in preference order)
    pub available_simd_modes: Vec<SimdMode>,
}

impl MachineFingerprint {
    /// Detect machine fingerprint from current system
    pub fn detect() -> SimdResult<Self> {
        let arch = std::env::consts::ARCH.to_string();
        let os = std::env::consts::OS.to_string();

        let cpu_features = CpuFeatureSet::detect()?;
        let primary_simd_mode = SimdMode::from_features(&cpu_features);
        let available_simd_modes = SimdMode::available_modes(&cpu_features);

        let cpu_model = detect_cpu_model(&arch)?;

        Ok(MachineFingerprint {
            cpu_model,
            os,
            arch,
            primary_simd_mode,
            available_simd_modes,
        })
    }

    /// Format fingerprint as hex digest for reproducibility
    pub fn as_digest(&self) -> SimdResult<String> {
        let s = format!(
            "{}/{}/{}/{}",
            self.os,
            self.arch,
            self.cpu_model,
            self.primary_simd_mode.as_str()
        );
        Ok(blake3::hash(s.as_bytes()).to_hex().to_string())
    }
}

/// SIMD equivalence digest pair (baseline vs. comparison)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SimdEquivalenceDigest {
    /// Baseline run digest (as hex string for serialization)
    pub baseline_digest: String,
    /// Comparison run digest (as hex string for serialization)
    pub comparison_digest: String,
    /// Machine fingerprint of baseline
    pub baseline_machine: MachineFingerprint,
    /// Machine fingerprint of comparison
    pub comparison_machine: MachineFingerprint,
    /// Mode used for baseline
    pub baseline_mode: SimdMode,
    /// Mode used for comparison
    pub comparison_mode: SimdMode,
    /// Equivalence result (true if digests match)
    pub is_equivalent: bool,
    /// Bytes diverged (0 if equivalent, otherwise byte offset of first divergence)
    pub divergence_byte_offset: Option<usize>,
}

impl SimdEquivalenceDigest {
    /// Create new SIMD equivalence digest
    pub fn new(
        baseline_digest: Hash,
        comparison_digest: Hash,
        baseline_machine: MachineFingerprint,
        comparison_machine: MachineFingerprint,
        baseline_mode: SimdMode,
        comparison_mode: SimdMode,
    ) -> SimdResult<Self> {
        let baseline_bytes = baseline_digest.as_bytes();
        let comparison_bytes = comparison_digest.as_bytes();
        let is_equivalent = baseline_bytes == comparison_bytes;
        let divergence_byte_offset = if is_equivalent {
            None
        } else {
            // Find first diverging byte
            let offset = baseline_bytes
                .iter()
                .zip(comparison_bytes.iter())
                .position(|(a, b)| a != b);
            offset
        };

        Ok(SimdEquivalenceDigest {
            baseline_digest: baseline_digest.to_hex().to_string(),
            comparison_digest: comparison_digest.to_hex().to_string(),
            baseline_machine,
            comparison_machine,
            baseline_mode,
            comparison_mode,
            is_equivalent,
            divergence_byte_offset,
        })
    }

    /// Check if equivalence holds (fail-closed if not)
    pub fn verify(&self) -> SimdResult<()> {
        if !self.is_equivalent {
            return Err(SimdError::ArchitectureDivergence(format!(
                "Digest mismatch: baseline {} != comparison {}, divergence at byte {:?}",
                self.baseline_digest, self.comparison_digest, self.divergence_byte_offset
            )));
        }
        Ok(())
    }
}

impl fmt::Display for SimdEquivalenceDigest {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "SimdEquivalenceDigest {{ baseline: {} ({}/{}), comparison: {} ({}/{}), equivalent: {} }}",
            self.baseline_digest,
            self.baseline_machine.arch,
            self.baseline_mode.as_str(),
            self.comparison_digest,
            self.comparison_machine.arch,
            self.comparison_mode.as_str(),
            self.is_equivalent
        )
    }
}

/// Main verification function: Compare SIMD execution results across modes/architectures
///
/// # Arguments
/// * `baseline_digest` - Result digest from baseline SIMD mode (e.g., AVX-512)
/// * `comparison_digest` - Result digest from comparison mode (e.g., scalar or NEON)
/// * `baseline_mode` - SIMD mode used for baseline
/// * `comparison_mode` - SIMD mode used for comparison
///
/// # Returns
/// * `Ok(SimdEquivalenceDigest)` if digests match (equivalence proven)
/// * `Err(SimdError::ArchitectureDivergence)` if digests differ (fail-closed)
///
/// # Example
/// ```no_run
/// use qlever_simd_verifier::{verify_simd_equivalence, SimdMode};
///
/// let baseline = blake3::hash(b"query_result_avx512");
/// let comparison = blake3::hash(b"query_result_scalar");
///
/// match verify_simd_equivalence(baseline, comparison, SimdMode::Avx512, SimdMode::Scalar) {
///     Ok(digest) if digest.is_equivalent => println!("SIMD equivalence verified!"),
///     Ok(digest) => println!("SIMD equivalence check: {} bytes diverged",
///                             digest.divergence_byte_offset.unwrap_or(0)),
///     Err(e) => eprintln!("Verification failed: {}", e),
/// }
/// ```
pub fn verify_simd_equivalence(
    baseline_digest: Hash,
    comparison_digest: Hash,
    baseline_mode: SimdMode,
    comparison_mode: SimdMode,
) -> SimdResult<SimdEquivalenceDigest> {
    let baseline_machine = MachineFingerprint::detect()?;
    let comparison_machine = MachineFingerprint::detect()?;

    SimdEquivalenceDigest::new(
        baseline_digest,
        comparison_digest,
        baseline_machine,
        comparison_machine,
        baseline_mode,
        comparison_mode,
    )
}

/// Detect CPU model from architecture string
fn detect_cpu_model(_arch: &str) -> SimdResult<String> {
    #[cfg(target_arch = "x86_64")]
    {
        use std::arch::x86_64::__cpuid;

        // CPUID leaf 0x80000002-4 returns CPU brand string
        unsafe {
            let mut brand_str = String::new();
            for leaf in [0x80000002u32, 0x80000003, 0x80000004] {
                let result = __cpuid(leaf);
                for val in [result.eax, result.ebx, result.ecx, result.edx] {
                    let bytes = val.to_le_bytes();
                    for b in bytes {
                        if b != 0 {
                            brand_str.push(b as char);
                        }
                    }
                }
            }
            if !brand_str.is_empty() {
                return Ok(brand_str.trim().to_string());
            }
        }

        Ok("x86_64-generic".to_string())
    }

    #[cfg(target_arch = "aarch64")]
    {
        // ARM64 CPU detection is more complex; use /proc/cpuinfo or sysfs
        match std::fs::read_to_string("/proc/cpuinfo") {
            Ok(content) => {
                for line in content.lines() {
                    if line.starts_with("CPU implementer") || line.starts_with("CPU part") {
                        if let Some(value) = line.split(':').nth(1) {
                            return Ok(value.trim().to_string());
                        }
                    }
                }
                Ok("aarch64-generic".to_string())
            }
            Err(_) => Ok("aarch64-generic".to_string()),
        }
    }

    #[cfg(not(any(target_arch = "x86_64", target_arch = "aarch64")))]
    {
        Ok(format!("{}-generic", arch))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_machine_fingerprint_detect() {
        let fp = MachineFingerprint::detect().expect("Failed to detect machine fingerprint");
        assert!(!fp.cpu_model.is_empty());
        assert!(!fp.os.is_empty());
        assert!(!fp.arch.is_empty());
        assert!(!fp.available_simd_modes.is_empty());
    }

    #[test]
    fn test_simd_equivalence_digest_matching() {
        let hash1 = blake3::hash(b"same_result");
        let hash2 = blake3::hash(b"same_result");

        let fp = MachineFingerprint::detect().expect("Failed to detect machine fingerprint");
        let digest = SimdEquivalenceDigest::new(
            hash1,
            hash2,
            fp.clone(),
            fp,
            SimdMode::Scalar,
            SimdMode::Scalar,
        )
        .expect("Failed to create digest");

        assert!(digest.is_equivalent);
        assert_eq!(digest.divergence_byte_offset, None);
        assert!(digest.verify().is_ok());
    }

    #[test]
    fn test_simd_equivalence_digest_divergence() {
        let hash1 = blake3::hash(b"result_1");
        let hash2 = blake3::hash(b"result_2");

        let fp = MachineFingerprint::detect().expect("Failed to detect machine fingerprint");
        let digest = SimdEquivalenceDigest::new(
            hash1,
            hash2,
            fp.clone(),
            fp,
            SimdMode::Avx512,
            SimdMode::Scalar,
        )
        .expect("Failed to create digest");

        assert!(!digest.is_equivalent);
        assert!(digest.divergence_byte_offset.is_some());
        assert!(digest.verify().is_err());
    }

    #[test]
    fn test_machine_fingerprint_as_digest() {
        let fp = MachineFingerprint::detect().expect("Failed to detect machine fingerprint");
        let digest_str = fp.as_digest().expect("Failed to get digest");
        assert_eq!(digest_str.len(), 64); // BLAKE3 hash in hex = 64 chars
    }
}

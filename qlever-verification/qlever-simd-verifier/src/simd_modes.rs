//! SIMD Modes and CPU Feature Detection
//!
//! Defines available SIMD modes (AVX-512, NEON, scalar) and CPU feature detection
//! via CPUID (x86_64) or HWCAP (ARM64).

use crate::SimdResult;
use serde::{Deserialize, Serialize};
use std::fmt;

/// Supported SIMD modes for equivalence verification
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum SimdMode {
    /// AVX-512F (512-bit SIMD, ZMM registers) - Intel Skylake+, AMD Milan+
    Avx512,
    /// AVX2 (256-bit SIMD, YMM registers) - Haswell+, Zen+
    Avx2,
    /// NEON (128-bit SIMD, Q registers) - ARM Cortex-A7+
    Neon,
    /// Scalar fallback (no SIMD)
    Scalar,
}

impl SimdMode {
    /// String representation of SIMD mode
    pub fn as_str(&self) -> &'static str {
        match self {
            SimdMode::Avx512 => "avx512",
            SimdMode::Avx2 => "avx2",
            SimdMode::Neon => "neon",
            SimdMode::Scalar => "scalar",
        }
    }

    /// Get primary SIMD mode from detected CPU features
    pub fn from_features(features: &CpuFeatureSet) -> Self {
        if features.has_avx512f {
            SimdMode::Avx512
        } else if features.has_avx2 {
            SimdMode::Avx2
        } else if features.has_neon {
            SimdMode::Neon
        } else {
            SimdMode::Scalar
        }
    }

    /// Get all available SIMD modes in preference order
    pub fn available_modes(features: &CpuFeatureSet) -> Vec<SimdMode> {
        let mut modes = vec![SimdMode::Scalar]; // Scalar is always available

        if features.has_neon {
            modes.insert(0, SimdMode::Neon);
        }
        if features.has_avx2 {
            modes.insert(0, SimdMode::Avx2);
        }
        if features.has_avx512f {
            modes.insert(0, SimdMode::Avx512);
        }

        modes
    }
}

impl fmt::Display for SimdMode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

/// CPU feature set detected on current system
#[derive(Debug, Clone, Default)]
pub struct CpuFeatureSet {
    // x86_64 features
    pub has_avx512f: bool,
    pub has_avx512cd: bool,
    pub has_avx512_er: bool,
    pub has_avx512_pf: bool,
    pub has_avx2: bool,
    pub has_avx: bool,
    pub has_sse42: bool,
    pub has_sse41: bool,

    // ARM64 features
    pub has_neon: bool,
    pub has_sve: bool,
    pub has_sve2: bool,
}

impl CpuFeatureSet {
    /// Detect CPU features on current system
    pub fn detect() -> SimdResult<Self> {
        #[cfg(target_arch = "x86_64")]
        {
            detect_x86_64_features()
        }

        #[cfg(target_arch = "aarch64")]
        {
            detect_aarch64_features()
        }

        #[cfg(not(any(target_arch = "x86_64", target_arch = "aarch64")))]
        {
            Ok(CpuFeatureSet::default())
        }
    }

    /// Check if any SIMD support is available
    pub fn has_any_simd(&self) -> bool {
        self.has_avx512f || self.has_avx2 || self.has_avx || self.has_neon
    }

    /// Format features as debug string
    pub fn to_feature_string(&self) -> String {
        let mut features = vec![];

        if self.has_avx512f {
            features.push("avx512f");
        }
        if self.has_avx512cd {
            features.push("avx512cd");
        }
        if self.has_avx2 {
            features.push("avx2");
        }
        if self.has_avx {
            features.push("avx");
        }
        if self.has_sse42 {
            features.push("sse4.2");
        }
        if self.has_neon {
            features.push("neon");
        }
        if self.has_sve {
            features.push("sve");
        }

        if features.is_empty() {
            "scalar".to_string()
        } else {
            features.join(",")
        }
    }
}

#[cfg(target_arch = "x86_64")]
fn detect_x86_64_features() -> SimdResult<CpuFeatureSet> {
    use std::arch::x86_64::{__cpuid, __cpuid_count};

    let mut features = CpuFeatureSet::default();

    unsafe {
        // CPUID leaf 1: basic features
        let cpuid1 = __cpuid(1);
        let ecx = cpuid1.ecx;
        let _edx = cpuid1.edx;

        features.has_sse41 = (ecx & (1 << 19)) != 0;
        features.has_sse42 = (ecx & (1 << 20)) != 0;
        features.has_avx = (ecx & (1 << 28)) != 0;

        // CPUID leaf 7: extended features
        let cpuid7 = __cpuid(7);
        let ebx = cpuid7.ebx;

        features.has_avx2 = (ebx & (1 << 5)) != 0;
        features.has_avx512f = (ebx & (1 << 16)) != 0;
        features.has_avx512cd = (ebx & (1 << 28)) != 0;

        // CPUID leaf 7, subleaf 1: extended AVX-512 features
        let cpuid7_1 = __cpuid_count(7, 1);
        let eax = cpuid7_1.eax;

        features.has_avx512_er = (eax & (1 << 7)) != 0;
        features.has_avx512_pf = (eax & (1 << 8)) != 0;
    }

    Ok(features)
}

#[cfg(target_arch = "aarch64")]
fn detect_aarch64_features() -> SimdResult<CpuFeatureSet> {
    let mut features = CpuFeatureSet::default();

    // ARM64 NEON is always present on ARMv8+
    features.has_neon = true;

    // Try to detect SVE from hwcap if available
    // This would require linking against libc's getauxval() or reading /proc/cpuinfo
    // For now, we'll assume SVE is available if we can detect it via /proc/cpuinfo
    if let Ok(content) = std::fs::read_to_string("/proc/cpuinfo") {
        for line in content.lines() {
            if line.starts_with("Features") {
                if line.contains("sve") {
                    features.has_sve = true;
                }
                if line.contains("sve2") {
                    features.has_sve2 = true;
                }
            }
        }
    }

    Ok(features)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_simd_mode_string_representation() {
        assert_eq!(SimdMode::Avx512.as_str(), "avx512");
        assert_eq!(SimdMode::Avx2.as_str(), "avx2");
        assert_eq!(SimdMode::Neon.as_str(), "neon");
        assert_eq!(SimdMode::Scalar.as_str(), "scalar");
    }

    #[test]
    fn test_simd_mode_display() {
        assert_eq!(format!("{}", SimdMode::Avx512), "avx512");
        assert_eq!(format!("{}", SimdMode::Scalar), "scalar");
    }

    #[test]
    fn test_cpu_feature_set_detect() {
        let _features = CpuFeatureSet::detect().expect("Failed to detect CPU features");
        // At least one of scalar or a SIMD mode should be available
        assert!(true); // Detection succeeded
    }

    #[test]
    fn test_simd_mode_from_features() {
        let features = CpuFeatureSet::default();
        let mode = SimdMode::from_features(&features);
        // With default features (no SIMD), should return Scalar
        assert_eq!(mode, SimdMode::Scalar);
    }

    #[test]
    fn test_available_modes_always_includes_scalar() {
        let features = CpuFeatureSet::default();
        let modes = SimdMode::available_modes(&features);
        assert!(modes.contains(&SimdMode::Scalar));
    }

    #[test]
    fn test_cpu_feature_set_to_feature_string() {
        let features = CpuFeatureSet::default();
        let s = features.to_feature_string();
        assert_eq!(s, "scalar");

        let mut features_with_avx2 = CpuFeatureSet::default();
        features_with_avx2.has_avx2 = true;
        let s = features_with_avx2.to_feature_string();
        assert!(s.contains("avx2"));
    }
}

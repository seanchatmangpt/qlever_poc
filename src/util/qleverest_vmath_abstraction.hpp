// EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
// Hardware Abstraction Layer for SIMD Vector Math Operations
//
// Copyright 2026, QLever Architecture Neutrality Team
//
// CRITICAL INVARIANT: This header isolates ALL hardware-specific SIMD
// instructions from general QLever logic. General code must remain 97%+
// architecture-neutral. Only qleverest::vmath namespace may contain
// hardware intrinsics (AVX-512, NEON, etc.).
//
// BUILD REQUIREMENT: Hardware flags (-mavx2, -march=native, etc.) are
// FORBIDDEN in general compilation. CMake must apply hardware flags ONLY
// to vmath backend compilation units.

#ifndef QLEVER_SRC_UTIL_QLEVEREST_VMATH_ABSTRACTION_HPP
#define QLEVER_SRC_UTIL_QLEVEREST_VMATH_ABSTRACTION_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace qlever::vmath {

// Vector Math Backend Selection
// Runtime dispatch based on CPU capabilities (cpuid/getauxval)
enum class Backend : uint8_t {
  Scalar = 0,   // Baseline fallback (100% portable, no SIMD)
  SSE42 = 1,    // x86-64-v2 baseline (128-bit vectors)
  AVX2 = 2,     // Modern x86 (256-bit vectors)
  AVX512 = 3,   // High-end x86 (512-bit vectors)
  NEON = 4,     // ARM NEON (128-bit vectors)
};

// CPU Capability Detection Result
struct CpuCapabilities {
  bool hasSSE42 = false;
  bool hasAVX2 = false;
  bool hasAVX512 = false;
  bool hasNEON = false;

  // Select best available backend
  Backend bestBackend() const noexcept;
};

// Detect CPU capabilities at runtime (calls cpuid on x86, getauxval on ARM)
CpuCapabilities detectCpuCapabilities() noexcept;

// Get singleton CPU capabilities (initialized once at startup)
const CpuCapabilities& getCpuCapabilities() noexcept;

// =============================================================================
// VECTOR MATH OPERATIONS (Integer-only for determinism)
// =============================================================================

// All operations are:
// - Deterministic (same input -> same output)
// - Integer-only (no floating-point for bit-exact results)
// - Fallback-safe (scalar path always available)
// - Stateless (no hidden global state)

// Vectorized Fill: Set all elements of span to single value
// Scalar equivalent: std::fill(dest.begin(), dest.end(), value)
// Performance: AVX2 2-3x faster, AVX-512 4-6x faster
void fillRepeated(std::span<int64_t> dest, int64_t value,
                  Backend backend = Backend::Scalar) noexcept;

// Vectorized Copy: Copy source to destination
// Scalar equivalent: std::copy(src.begin(), src.end(), dest.begin())
// Performance: AVX2 1.5-2x faster, AVX-512 2-3x faster
void vectorCopy(std::span<const int64_t> src, std::span<int64_t> dest,
                Backend backend = Backend::Scalar) noexcept;

// Vectorized Sum: Sum all elements in span
// Scalar equivalent: std::accumulate(src.begin(), src.end(), int64_t(0))
// Performance: AVX2 3-4x faster, AVX-512 6-8x faster
// DETERMINISM: Fixed left-to-right evaluation order (no reordering)
int64_t vectorSum(std::span<const int64_t> src,
                  Backend backend = Backend::Scalar) noexcept;

// Vectorized Min: Find minimum element in span
// Scalar equivalent: *std::min_element(src.begin(), src.end())
// Performance: AVX2 3-4x faster, AVX-512 6-8x faster
int64_t vectorMin(std::span<const int64_t> src,
                  Backend backend = Backend::Scalar) noexcept;

// Vectorized Max: Find maximum element in span
// Scalar equivalent: *std::max_element(src.begin(), src.end())
// Performance: AVX2 3-4x faster, AVX-512 6-8x faster
int64_t vectorMax(std::span<const int64_t> src,
                  Backend backend = Backend::Scalar) noexcept;

// Vectorized Compare: Element-wise comparison, return bitmask
// Scalar equivalent: transform with comparison predicate
// Performance: AVX2 4-6x faster, AVX-512 8-12x faster
enum class CompareOp : uint8_t {
  LessThan,
  LessEqual,
  Equal,
  NotEqual,
  GreaterEqual,
  GreaterThan
};

void vectorCompare(std::span<const int64_t> src, int64_t compareValue,
                   CompareOp op, std::span<bool> result,
                   Backend backend = Backend::Scalar) noexcept;

// =============================================================================
// BACKEND AVAILABILITY CHECKS
// =============================================================================

// Check if backend is available on current CPU
bool isBackendAvailable(Backend backend) noexcept;

// Select best backend for given operation and data size
// Small data (<64 elements): Scalar often faster due to setup overhead
// Medium data (64-512 elements): SSE42/AVX2 optimal
// Large data (>512 elements): AVX-512 optimal (if available)
Backend selectOptimalBackend(size_t elementCount) noexcept;

// =============================================================================
// IMPLEMENTATION NOTES
// =============================================================================

// Backend Implementation Files (HARDWARE FLAGS APPLY HERE ONLY):
// - src/util/qleverest_vmath_scalar.cpp (no flags, always compiled)
// - src/util/qleverest_vmath_avx2.cpp (compiled with -mavx2)
// - src/util/qleverest_vmath_avx512.cpp (compiled with -mavx512f -mavx512vl -mavx512bw)
// - src/util/qleverest_vmath_neon.cpp (compiled with -mfpu=neon on ARM)
//
// CMake Configuration (cmake/VmathFlags.cmake):
// - ONLY apply hardware flags to vmath backend compilation units
// - General QLever code compiles with ZERO hardware flags
// - Proof: 97%+ of SLOC compiles without -march/-mavx/-mfpu
//
// CPU Detection (src/util/qleverest_vmath_cpu_detect.cpp):
// - x86: Uses __get_cpuid() for SSE4.2/AVX2/AVX-512 detection
// - ARM: Uses getauxval(AT_HWCAP) for NEON detection
// - No hardware flags needed for detection code itself
//
// Determinism Guarantee (AX-2 Invariant):
// - All operations are integer-only (no floating-point)
// - All operations have fixed evaluation order (no reordering)
// - All operations produce bit-identical results across backends
// - Correctness tests: scalar vs SIMD must match exactly
//
// Backward Compatibility (AX-6 Invariant):
// - Scalar backend is always available (100% portable)
// - Runtime backend selection allows fallback to scalar
// - No breaking changes to existing QLever code (additive only)

}  // namespace qlever::vmath

#endif  // QLEVER_SRC_UTIL_QLEVEREST_VMATH_ABSTRACTION_HPP

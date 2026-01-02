// EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
// CPU Capability Detection (Architecture-Neutral)
//
// Copyright 2026, QLever Architecture Neutrality Team
//
// CRITICAL: This file compiles WITHOUT hardware-specific flags.
// Uses standard CPUID/HWCAP interfaces for capability detection.

#include "util/qleverest_vmath_abstraction.hpp"

#ifdef __x86_64__
#include <cpuid.h>
#endif

#ifdef __aarch64__
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif

namespace qlever::vmath {

CpuCapabilities detectCpuCapabilities() noexcept {
  CpuCapabilities caps;

#ifdef __x86_64__
  // x86-64 CPU detection via CPUID instruction
  unsigned int eax, ebx, ecx, edx;

  // CPUID function 1: Basic CPU features
  if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
    // SSE4.2: ECX bit 20
    caps.hasSSE42 = (ecx & (1u << 20)) != 0;
  }

  // CPUID function 7, subleaf 0: Extended features
  if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
    // AVX2: EBX bit 5
    caps.hasAVX2 = (ebx & (1u << 5)) != 0;

    // AVX-512F: EBX bit 16
    // AVX-512VL: EBX bit 31
    // AVX-512BW: EBX bit 30
    // Require all three for QLever's int64 operations
    bool hasAVX512F = (ebx & (1u << 16)) != 0;
    bool hasAVX512VL = (ebx & (1u << 31)) != 0;
    bool hasAVX512BW = (ebx & (1u << 30)) != 0;
    caps.hasAVX512 = hasAVX512F && hasAVX512VL && hasAVX512BW;
  }
#endif  // __x86_64__

#ifdef __aarch64__
  // ARM CPU detection via getauxval(AT_HWCAP)
  unsigned long hwcap = getauxval(AT_HWCAP);

  // NEON is mandatory on AArch64, but verify explicitly
  caps.hasNEON = (hwcap & HWCAP_ASIMD) != 0;
#endif  // __aarch64__

  return caps;
}

Backend CpuCapabilities::bestBackend() const noexcept {
  // Select highest-performance available backend
  if (hasAVX512) {
    return Backend::AVX512;
  }
  if (hasAVX2) {
    return Backend::AVX2;
  }
  if (hasSSE42) {
    return Backend::SSE42;
  }
  if (hasNEON) {
    return Backend::NEON;
  }
  return Backend::Scalar;
}

const CpuCapabilities& getCpuCapabilities() noexcept {
  // Thread-safe singleton initialization (C++11 static storage duration)
  static const CpuCapabilities capabilities = detectCpuCapabilities();
  return capabilities;
}

bool isBackendAvailable(Backend backend) noexcept {
  const auto& caps = getCpuCapabilities();

  switch (backend) {
    case Backend::Scalar:
      return true;  // Always available
    case Backend::SSE42:
      return caps.hasSSE42;
    case Backend::AVX2:
      return caps.hasAVX2;
    case Backend::AVX512:
      return caps.hasAVX512;
    case Backend::NEON:
      return caps.hasNEON;
    default:
      return false;
  }
}

Backend selectOptimalBackend(size_t elementCount) noexcept {
  const auto& caps = getCpuCapabilities();

  // Small data (<64 elements): Scalar faster due to setup overhead
  if (elementCount < 64) {
    return Backend::Scalar;
  }

  // Large data (>=512 elements): Use widest available SIMD
  if (elementCount >= 512) {
    if (caps.hasAVX512) {
      return Backend::AVX512;
    }
    if (caps.hasNEON) {
      return Backend::NEON;
    }
  }

  // Medium data (64-511 elements): Use AVX2 or NEON
  if (caps.hasAVX2) {
    return Backend::AVX2;
  }
  if (caps.hasNEON) {
    return Backend::NEON;
  }

  // Small SIMD (SSE4.2) for any size >= 64
  if (caps.hasSSE42) {
    return Backend::SSE42;
  }

  // No SIMD available: Scalar fallback
  return Backend::Scalar;
}

}  // namespace qlever::vmath

// EPIC 14.0: Unified Formalism Pipeline — Configuration and Feature Detection
// Agent 10 Deliverable — SIMD Backend Detection and Capability Query
//
// Copyright 2026, QLever Unified Formalism Team
//
// SPECIFICATION CLOSURE: This header provides compile-time and runtime
// configuration for the unified formalism pipeline, including SIMD backend
// detection, feature queries, and build-time configuration.
//
// AUTHORITY: EPIC 14.0 Formalism Delta Discovery + BB80/20 Convergence Protocol
// AGENT: Agent 10 (CMake Integration)
// STATUS: MEASUREMENT PHASE — API definition only, no implementation yet

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCONFIG_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCONFIG_H

#include <string>
#include <vector>

#include "util/CpuFeatureDetection.h"

namespace qlever::unified_formalism {

// =============================================================================
// COMPILE-TIME CONFIGURATION
// =============================================================================

// Unified formalism enabled flag (set by CMake)
#ifndef QLEVER_UNIFIED_FORMALISM_ENABLED
#define QLEVER_UNIFIED_FORMALISM_ENABLED 0
#endif

// Available SIMD backends (set by CMake as string literal)
#ifndef QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS
#define QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS "SCALAR"
#endif

// =============================================================================
// RUNTIME CONFIGURATION
// =============================================================================

/// Configuration structure for unified formalism pipeline
struct UnifiedFormalismConfig {
  /// SIMD backend preference (default: auto-detect)
  /// Override to force specific backend for testing
  CpuBackend preferredBackend = CpuBackend::Scalar;

  /// Enable SIMD optimization (default: true)
  /// Set to false to force scalar fallback (for testing/debugging)
  bool enableSIMD = true;

  /// Minimum data size for SIMD (default: 64 elements)
  /// Below this threshold, scalar code is faster due to overhead
  size_t simdThreshold = 64;

  /// Maximum data size for single-threaded processing (default: 1MB)
  /// Above this threshold, consider parallelization
  size_t parallelThreshold = 1024 * 1024;

  /// Enable debug logging (default: false)
  /// Prints backend selection and performance metrics
  bool enableDebugLog = false;
};

// =============================================================================
// SIMD CAPABILITY DETECTION
// =============================================================================

/// Query available SIMD backends on current platform
/// Returns list of backends that are both:
/// 1. Supported by hardware (runtime detection)
/// 2. Compiled into binary (compile-time feature flags)
///
/// Example:
///   auto backends = getAvailableSIMDBackends();
///   for (auto backend : backends) {
///     std::cout << "Available: " << toString(backend) << std::endl;
///   }
inline std::vector<CpuBackend> getAvailableSIMDBackends() {
  // Delegate to existing CPU feature detection
  // (Agent 4 Part 1 API, implemented by Agent 9)
  return getAvailableBackends();
}

/// Select optimal SIMD backend for given data size
/// Considers both hardware capabilities and data size heuristics
///
/// Heuristics:
/// - dataSize < 64: Scalar (SIMD overhead dominates)
/// - dataSize < 512: SSE4.2/AVX2/NEON (optimal for medium data)
/// - dataSize >= 512: AVX-512 (worth the setup overhead)
///
/// Args:
///   dataSize: Number of elements to process
///
/// Returns:
///   Optimal backend for given data size
///
/// Example:
///   auto backend = selectOptimalBackend(1000);
///   // Returns AVX2 on Haswell+, NEON on ARM64, Scalar on old CPUs
inline CpuBackend selectOptimalBackend(size_t dataSize) {
  // Delegate to vmath abstraction layer
  return vmath::selectOptimalBackend(dataSize);
}

/// Check if specific backend is available for given data size
/// Combines hardware availability with size heuristics
///
/// Args:
///   backend: Backend to check (e.g., CpuBackend::AVX2)
///   dataSize: Number of elements to process
///
/// Returns:
///   true if backend is both available and optimal for data size
///
/// Example:
///   if (isBackendSupportedFor(CpuBackend::AVX2, 1000)) {
///     // Use AVX2 codepath
///   }
inline bool isBackendSupportedFor(CpuBackend backend, size_t dataSize) {
  // Delegate to existing API (Agent 4 Part 1)
  return isSupportedFor(backend, dataSize);
}

// =============================================================================
// BACKEND INFORMATION
// =============================================================================

/// Get human-readable name for backend
/// Args:
///   backend: Backend enum value
///
/// Returns:
///   String representation (e.g., "AVX2", "NEON", "Scalar")
inline std::string toString(CpuBackend backend) {
  switch (backend) {
    case CpuBackend::Scalar:
      return "Scalar";
    case CpuBackend::SSE42:
      return "SSE4.2";
    case CpuBackend::AVX2:
      return "AVX2";
    case CpuBackend::AVX512:
      return "AVX-512";
    case CpuBackend::NEON:
      return "NEON";
    default:
      return "Unknown";
  }
}

/// Get SIMD width (elements per operation) for backend
/// Args:
///   backend: Backend enum value
///
/// Returns:
///   Number of elements processed per SIMD instruction
///   (assumes 64-bit elements; adjust for other types)
inline size_t getSIMDWidth(CpuBackend backend) {
  switch (backend) {
    case CpuBackend::Scalar:
      return 1;  // No SIMD (1 element per operation)
    case CpuBackend::SSE42:
      return 2;  // 128-bit SIMD / 64-bit element = 2
    case CpuBackend::AVX2:
      return 4;  // 256-bit SIMD / 64-bit element = 4
    case CpuBackend::AVX512:
      return 8;  // 512-bit SIMD / 64-bit element = 8
    case CpuBackend::NEON:
      return 2;  // 128-bit SIMD / 64-bit element = 2
    default:
      return 1;
  }
}

// =============================================================================
// COMPILE-TIME FEATURE QUERIES
// =============================================================================

/// Check if unified formalism is enabled at compile time
constexpr bool isUnifiedFormalismEnabled() {
  return QLEVER_UNIFIED_FORMALISM_ENABLED;
}

/// Get compile-time SIMD backends string (from CMake)
constexpr const char* getCompiledSIMDBackends() {
  return QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS;
}

// =============================================================================
// RUNTIME CAPABILITY SUMMARY
// =============================================================================

/// Runtime capability report (for diagnostics and logging)
struct CapabilitySummary {
  /// List of available backends
  std::vector<CpuBackend> availableBackends;

  /// Current architecture (x86-64, ARM64, etc.)
  Architecture architecture;

  /// Recommended backend for typical workloads
  CpuBackend recommendedBackend;

  /// Compile-time backends (from CMake)
  std::string compiledBackends;

  /// Runtime-detected backends (from CPUID/getauxval)
  std::string runtimeBackends;
};

/// Get full capability summary for diagnostics
/// Returns all available information about SIMD support
///
/// Example:
///   auto summary = getCapabilitySummary();
///   std::cout << "Architecture: " << toString(summary.architecture) << "\n";
///   std::cout << "Recommended: " << toString(summary.recommendedBackend) <<
///   "\n";
inline CapabilitySummary getCapabilitySummary() {
  CapabilitySummary summary;

  // Get available backends
  summary.availableBackends = getAvailableSIMDBackends();

  // Get architecture
  summary.architecture = getCurrentArchitecture();

  // Select recommended backend (for medium-sized workloads)
  summary.recommendedBackend = selectOptimalBackend(256);

  // Compile-time backends (from CMake)
  summary.compiledBackends = getCompiledSIMDBackends();

  // Runtime-detected backends (build string)
  summary.runtimeBackends = "";
  for (size_t i = 0; i < summary.availableBackends.size(); ++i) {
    if (i > 0) summary.runtimeBackends += ";";
    summary.runtimeBackends += toString(summary.availableBackends[i]);
  }

  return summary;
}

// =============================================================================
// ARCHITECTURE INFORMATION
// =============================================================================

/// Get human-readable name for architecture
inline std::string toString(Architecture arch) {
  switch (arch) {
    case Architecture::x86_64:
      return "x86-64";
    case Architecture::ARM64:
      return "ARM64";
    default:
      return "Unknown";
  }
}

// =============================================================================
// USAGE EXAMPLES
// =============================================================================
//
// Example 1: Auto-detect optimal backend
//
//   void processData(const std::vector<int64_t>& data) {
//     auto backend = selectOptimalBackend(data.size());
//     // Use backend for SIMD dispatch
//   }
//
// Example 2: Check specific backend availability
//
//   if (isBackendSupportedFor(CpuBackend::AVX2, data.size())) {
//     // Use AVX2 codepath
//   } else {
//     // Fall back to scalar
//   }
//
// Example 3: Print capability summary
//
//   auto summary = getCapabilitySummary();
//   std::cout << "Architecture: " << toString(summary.architecture) << "\n";
//   std::cout << "Available backends:\n";
//   for (auto backend : summary.availableBackends) {
//     std::cout << "  - " << toString(backend)
//               << " (width: " << getSIMDWidth(backend) << ")\n";
//   }
//
// Example 4: Force specific backend for testing
//
//   UnifiedFormalismConfig config;
//   config.preferredBackend = CpuBackend::Scalar;  // Disable SIMD
//   config.enableSIMD = false;
//
// =============================================================================

}  // namespace qlever::unified_formalism

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMCONFIG_H

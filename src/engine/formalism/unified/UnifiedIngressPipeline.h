// EPIC 14.0: Unified Ingress Pipeline for All Formalisms
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 2 - Unified Ingress Architecture
//
// DESIGN PRINCIPLES:
// - Single entry point for JSON-LD ingress across SHACL, ShEx, N3, Datalog
// - All JSON parsing routes through simdjson (via SimdJsonIngressWrapper)
// - Deterministic digest computation (IngressDigest + SHA256)
// - Guard enforcement at ingress boundary (IngressGuardConfig)
// - Format-specific normalization delegated to dialect handlers
// - Zero heap allocation on hot path (stack-only operation)
// - Fail-closed: any guard violation → immediate error return
//
// ARCHITECTURE:
//   UnifiedIngressPipeline (facade)
//     ↓
//   SimdJsonIngressWrapper (SIMD-accelerated parsing)
//     ↓
//   JsonLdIngressNormalizer (dialect-specific normalization)
//     ↓
//   IngressDigest (deterministic SHA256 binding)

#ifndef QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIED_INGRESS_PIPELINE_H
#define QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIED_INGRESS_PIPELINE_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "engine/ingress/ErrorCodes.h"
#include "engine/ingress/IngressDigest.h"
#include "engine/ingress/IngressResult.h"
#include "engine/ingress/JsonLdIngressNormalizer.h"
#include "engine/ingress/SimdJsonIngressWrapper.h"
#include "global/Epoch.h"

namespace qlever::formalism {

// ============================================================================
// UNIFIED INGRESS CONFIGURATION
// ============================================================================

// Ingress optimization flags (SIMD techniques applied)
enum class IngressOptimization : uint32_t {
  None = 0u,
  SimdParsing = 1u << 0,         // SIMD-accelerated JSON parsing
  SimdValidation = 1u << 1,      // SIMD structural validation
  ParallelNormalization = 1u << 2,  // Parallel field normalization (future)
  CacheDigest = 1u << 3,          // Cache digest computation results
};

// Unified ingress metrics (observability, not diagnostics)
struct UnifiedIngressMetrics {
  // Parsing phase
  uint64_t json_parse_time_ns = 0;
  uint64_t json_bytes_parsed = 0;
  uint32_t simd_techniques_applied = 0;  // Bitmask of SimdTechnique

  // Normalization phase
  uint64_t normalization_time_ns = 0;
  uint64_t canonical_json_bytes = 0;

  // Digest phase
  uint64_t digest_compute_time_ns = 0;

  // Total
  uint64_t total_ingress_time_ns = 0;

  // Guard checks
  bool max_size_checked = false;
  bool max_depth_checked = false;
  bool timeout_checked = false;
};

// Unified ingress result (extends IngressResult with formalism metadata)
struct UnifiedIngressResult {
  // Base ingress result (error code + digest)
  ingress::IngressResult base;

  // Formalism-specific metadata
  ingress::RuleLanguageDialect dialect;

  // Normalized output (canonical JSON-LD)
  std::string normalized_json_ld;

  // Metrics (cold-path observability)
  UnifiedIngressMetrics metrics;

  // Optimization flags applied
  uint32_t optimizations_applied = 0;  // Bitmask of IngressOptimization

  // Constructor
  UnifiedIngressResult() = default;
  explicit UnifiedIngressResult(ingress::IngressErrorCode ec) : base(ec) {}

  // Convenience accessors
  ingress::IngressErrorCode error() const noexcept { return base.error; }
  bool ok() const noexcept { return base.error == ingress::IngressErrorCode::OK; }
  const std::string& digest() const noexcept { return base.digest_sha256; }
};

// ============================================================================
// UNIFIED INGRESS PIPELINE (Facade)
// ============================================================================

// Unified ingress pipeline for all formalisms (SHACL, ShEx, N3, Datalog)
//
// GUARANTEES:
// - All JSON-LD inputs parsed via simdjson (SIMD acceleration)
// - Deterministic normalization (same input → same digest)
// - Epoch-bound digest (includes EpochId + GuardConfig)
// - Guard enforcement (max size, max depth, timeout)
// - Fail-closed (any violation → error, no partial results)
// - Hot-path silence (no logging in parsing loop)
// - Thread-safe (stateless facade, no shared mutable state)
//
// USAGE PATTERN:
//   UnifiedIngressPipeline pipeline;
//   auto token = epochManager.getIngressCapabilityToken();
//   ingress::IngressGuardConfig guards;
//   guards.max_input_size_bytes = 50 * 1024 * 1024;  // 50MB
//
//   auto result = pipeline.ingest(
//       json_ld_input,
//       ingress::RuleLanguageDialect::SHACL,
//       token,
//       guards);
//
//   if (!result.ok()) {
//     // Handle error (fail-closed)
//     return;
//   }
//
//   // Use result.normalized_json_ld for downstream processing
//   // Use result.digest() for caching/determinism
//
class UnifiedIngressPipeline {
 public:
  UnifiedIngressPipeline() = default;
  ~UnifiedIngressPipeline() = default;

  // Deleted copy/move (stateless facade)
  UnifiedIngressPipeline(const UnifiedIngressPipeline&) = delete;
  UnifiedIngressPipeline(UnifiedIngressPipeline&&) = delete;
  UnifiedIngressPipeline& operator=(const UnifiedIngressPipeline&) = delete;
  UnifiedIngressPipeline& operator=(UnifiedIngressPipeline&&) = delete;

  // ========================================================================
  // PRIMARY INGRESS API: Unified entry point for all formalisms
  // ========================================================================

  // Ingest JSON-LD for any supported formalism
  //
  // PRECONDITIONS:
  // - json_ld_input must be valid JSON-LD format (other dialects rejected)
  // - token must be valid for current epoch (checked internally)
  // - guards must be initialized with valid bounds
  // - dialect must be one of: SHACL, SHEX, N3, DATALOG
  //
  // POSTCONDITIONS:
  // - If error != OK: ingestion failed, normalized_json_ld undefined
  // - If error == OK: normalized_json_ld contains canonical JSON-LD
  // - Digest computed from (canonical JSON + epoch + dialect + guard hash)
  // - Metrics populated with timing and optimization information
  //
  // INGRESS FLOW:
  // 1. Format detection (reject non-JSON-LD at boundary)
  // 2. Guard pre-check (size, depth limits)
  // 3. SIMD parsing (via SimdJsonIngressWrapper)
  // 4. Dialect-specific normalization (via JsonLdIngressNormalizer)
  // 5. Digest computation (SHA256 with epoch binding)
  // 6. Guard post-check (timeout, resource exhaustion)
  //
  // OPTIMIZATION POINTS:
  // - SIMD acceleration for JSON parsing (simdjson)
  // - Zero-copy normalization where possible
  // - Stack-only operation (no heap allocation in hot path)
  // - Early rejection for invalid formats
  // - Lazy digest computation (only if all guards pass)
  //
  // Thread-safe: Yes (stateless facade)
  // Hot-path silence: Yes (no logging in parsing loop)
  // Exceptions: Never throws (returns error code)
  //
  UnifiedIngressResult ingest(
      std::string_view json_ld_input,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards) noexcept;

  // Convenience overload: ingest with default guards
  UnifiedIngressResult ingest(
      std::string_view json_ld_input,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token) noexcept {
    ingress::IngressGuardConfig default_guards;
    return ingest(json_ld_input, dialect, token, default_guards);
  }

  // ========================================================================
  // FAST PATH API: Pre-validated JSON-LD (skip format detection)
  // ========================================================================

  // Ingest pre-validated JSON-LD (skip dialect detection)
  //
  // PRECONDITIONS:
  // - Caller MUST guarantee json_ld_input is valid JSON-LD
  // - Use only when format validation already performed upstream
  //
  // PERFORMANCE:
  // - Skips dialect detection (saves ~100ns per call)
  // - Otherwise identical to ingest()
  //
  // WARNING: Incorrect usage (passing non-JSON-LD) will cause parse errors
  //
  UnifiedIngressResult ingestPrevalidated(
      std::string_view json_ld_input,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards) noexcept;

  // ========================================================================
  // BATCH INGRESS API: Multiple documents in single call
  // ========================================================================

  // Ingest multiple JSON-LD documents in parallel (future optimization)
  //
  // PRECONDITIONS:
  // - All inputs must be same dialect
  // - All inputs validated independently
  // - Guards enforced per-document
  //
  // PERFORMANCE:
  // - Parallel SIMD parsing across documents
  // - Shared digest computation (batch SHA256)
  //
  // NOTE: Not yet implemented (returns UNIMPLEMENTED error)
  //
  std::vector<UnifiedIngressResult> ingestBatch(
      const std::vector<std::string_view>& json_ld_inputs,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards) noexcept;

  // ========================================================================
  // DIAGNOSTIC API: Explain ingress decisions (cold-path only)
  // ========================================================================

  // Explain why ingress failed (cold-path diagnostic tool)
  //
  // PURPOSE:
  // - Debugging failed ingress operations
  // - Understanding guard violations
  // - Analyzing SIMD technique selection
  //
  // USAGE:
  //   auto result = pipeline.ingest(...);
  //   if (!result.ok()) {
  //     auto explanation = pipeline.explainFailure(result);
  //     LOG(ERROR) << explanation;
  //   }
  //
  // NOTE: Cold-path only (allocates strings, logs, etc.)
  //
  std::string explainFailure(const UnifiedIngressResult& result) const noexcept;

  // ========================================================================
  // DETERMINISM VERIFICATION: Test suite support
  // ========================================================================

  // Verify ingress determinism (test suite only)
  //
  // GUARANTEES:
  // - Same input → same digest (across iterations)
  // - Same input → same normalized output (bit-identical)
  // - Same metrics (within measurement noise)
  //
  // USAGE:
  //   bool is_deterministic = pipeline.verifyDeterminism(
  //       json_ld_input, dialect, token, guards, 1000);
  //   ASSERT_TRUE(is_deterministic);
  //
  // NOTE: Cold-path only (runs multiple iterations)
  //
  bool verifyDeterminism(
      std::string_view json_ld_input,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards,
      int iterations = 100) const noexcept;

 private:
  // ========================================================================
  // INTERNAL PIPELINE STAGES
  // ========================================================================

  // Stage 1: Format detection and rejection
  UnifiedIngressResult detectAndReject(
      std::string_view input) const noexcept;

  // Stage 2: SIMD-accelerated JSON parsing
  UnifiedIngressResult simdParse(
      std::string_view json_input,
      const ingress::IngressGuardConfig& guards) const noexcept;

  // Stage 3: Dialect-specific normalization
  UnifiedIngressResult normalizeForDialect(
      std::string_view parsed_json,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards) const noexcept;

  // Stage 4: Digest computation with epoch binding
  UnifiedIngressResult computeEpochBoundDigest(
      std::string_view canonical_json,
      ingress::RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const ingress::IngressGuardConfig& guards) const noexcept;

  // ========================================================================
  // OPTIMIZATION UTILITIES
  // ========================================================================

  // Select SIMD optimization strategy based on input characteristics
  uint32_t selectSimdOptimizations(
      std::string_view input,
      const ingress::IngressGuardConfig& guards) const noexcept;

  // Check if input is eligible for fast path (pre-validated JSON-LD)
  bool isFastPathEligible(
      std::string_view input) const noexcept;

  // ========================================================================
  // COMPONENT INSTANCES (stateless, stack-allocated)
  // ========================================================================

  // NOTE: Components are instantiated on-demand (stack allocation)
  // No member variables (facade is stateless)
};

// ============================================================================
// UTILITY FUNCTIONS: Dialect metadata (cold-path only)
// ============================================================================

// Get expected JSON-LD context for dialect
constexpr std::string_view getExpectedContext(
    ingress::RuleLanguageDialect dialect) noexcept {
  switch (dialect) {
    case ingress::RuleLanguageDialect::SHACL:
      return "http://www.w3.org/ns/shacl#";
    case ingress::RuleLanguageDialect::SHEX:
      return "http://www.w3.org/ns/shex#";
    case ingress::RuleLanguageDialect::N3:
      return "http://www.w3.org/2000/10/swap/log#";
    case ingress::RuleLanguageDialect::DATALOG:
      return "http://qlever.cs.uni-freiburg.de/datalog#";
    default:
      return "UNKNOWN";
  }
}

// Get human-readable dialect name (delegates to JsonLdIngressNormalizer)
constexpr std::string_view getDialectName(
    ingress::RuleLanguageDialect dialect) noexcept {
  return ingress::dialectName(dialect);
}

// Get default guard configuration for dialect
inline ingress::IngressGuardConfig getDefaultGuards(
    ingress::RuleLanguageDialect dialect) noexcept {
  ingress::IngressGuardConfig guards;

  // Dialect-specific guard tuning
  switch (dialect) {
    case ingress::RuleLanguageDialect::SHACL:
      // SHACL shapes: typically small (<1MB)
      guards.max_input_size_bytes = 10 * 1024 * 1024;  // 10MB
      guards.max_nesting_depth = 50;
      guards.timeout_ms = 5000;  // 5 seconds
      break;

    case ingress::RuleLanguageDialect::SHEX:
      // ShEx schemas: typically small (<1MB)
      guards.max_input_size_bytes = 10 * 1024 * 1024;  // 10MB
      guards.max_nesting_depth = 50;
      guards.timeout_ms = 5000;
      break;

    case ingress::RuleLanguageDialect::N3:
      // N3 rules: can be large (10MB+)
      guards.max_input_size_bytes = 100 * 1024 * 1024;  // 100MB
      guards.max_nesting_depth = 100;
      guards.timeout_ms = 30000;  // 30 seconds
      break;

    case ingress::RuleLanguageDialect::DATALOG:
      // Datalog programs: typically tiny (<100KB)
      guards.max_input_size_bytes = 1 * 1024 * 1024;  // 1MB
      guards.max_nesting_depth = 20;
      guards.timeout_ms = 1000;  // 1 second
      break;

    default:
      // Conservative defaults
      guards.max_input_size_bytes = 100 * 1024 * 1024;
      guards.max_nesting_depth = 100;
      guards.timeout_ms = 30000;
      break;
  }

  return guards;
}

}  // namespace qlever::formalism

#endif  // QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIED_INGRESS_PIPELINE_H

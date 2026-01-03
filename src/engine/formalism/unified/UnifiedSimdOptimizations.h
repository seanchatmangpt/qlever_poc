// EPIC 14.0 - AGENT 8: Unified SIMD Optimization Layer
// Copyright 2026, QLever Formalism Team
//
// SIMD optimization layer for unified formalism pipeline
// - Vectorized constraint checking (batch evaluate constraints via SIMD)
// - Vectorized pattern matching (SIMD regex, string operations)
// - Vectorized IdTable operations (batch joins, filtering)
// - Fast path + fallback (SIMD when applicable; scalar when needed)
//
// DESIGN PRINCIPLES:
// - Zero modification to existing SimdJsonIngressWrapper
// - Deterministic (integer-only SIMD, bit-identical results)
// - Graceful fallback to scalar operations
// - Batch-oriented API (process multiple items per call)

#ifndef QLEVER_ENGINE_FORMALISM_UNIFIED_SIMD_OPTIMIZATIONS_H
#define QLEVER_ENGINE_FORMALISM_UNIFIED_SIMD_OPTIMIZATIONS_H

#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <vector>
#include <span>
#include <optional>

#include "global/Id.h"
#include "engine/idTable/IdTable.h"

// SIMD intrinsics headers (conditionally included)
#ifdef __x86_64__
#ifdef __SSE4_2__
#include <smmintrin.h>  // SSE4.2
#endif
#ifdef __AVX2__
#include <immintrin.h>  // AVX2
#endif
#ifdef __AVX512F__
#include <immintrin.h>  // AVX-512
#endif
#endif

namespace qlever::formalism::simd {

// ============================================================================
// 1. SIMD CAPABILITY DETECTION
// ============================================================================

/// SIMD capability flags (runtime detection)
enum class SimdCapability : uint32_t {
  None = 0,
  SSE42 = 1 << 0,   // 128-bit vectors (x86-64-v2 baseline)
  AVX2 = 1 << 1,    // 256-bit vectors (modern CPUs)
  AVX512 = 1 << 2,  // 512-bit vectors (high-end CPUs)
};

/// Runtime SIMD feature detection
struct SimdFeatures {
  bool hasSSE42 = false;
  bool hasAVX2 = false;
  bool hasAVX512 = false;

  /// Detect SIMD capabilities at runtime via CPUID
  static SimdFeatures detect() noexcept;

  /// Select best available SIMD technique
  SimdCapability bestCapability() const noexcept {
    if (hasAVX512) return SimdCapability::AVX512;
    if (hasAVX2) return SimdCapability::AVX2;
    if (hasSSE42) return SimdCapability::SSE42;
    return SimdCapability::None;
  }
};

/// Global singleton for SIMD features (initialized once at startup)
const SimdFeatures& getSimdFeatures() noexcept;

// ============================================================================
// 2. VECTORIZED CONSTRAINT CHECKING
// ============================================================================

/// Batch constraint evaluation result
struct ConstraintBatchResult {
  std::vector<bool> results;      // Per-element pass/fail
  size_t passCount = 0;           // Count of passing elements
  size_t failCount = 0;           // Count of failing elements
  bool usedSimd = false;          // Whether SIMD was used
};

/// Constraint type enumeration
enum class ConstraintType {
  MinLength,      // String length >= N
  MaxLength,      // String length <= N
  Pattern,        // Regex match
  InList,         // Value in allowed set
  MinInclusive,   // Numeric >= N
  MaxInclusive,   // Numeric <= N
  MinExclusive,   // Numeric > N
  MaxExclusive,   // Numeric < N
  NodeKind,       // IRI/BlankNode/Literal check
};

/// SIMD-optimized constraint evaluator
class SimdConstraintEvaluator {
 public:
  /// Batch evaluate min length constraint on multiple strings
  /// SIMD: Vectorized string length computation (SSE4.2+)
  /// Fallback: Scalar strlen() loop
  static ConstraintBatchResult evaluateMinLengthBatch(
      std::span<const std::string_view> values,
      size_t minLength) noexcept;

  /// Batch evaluate max length constraint on multiple strings
  /// SIMD: Vectorized string length computation (SSE4.2+)
  /// Fallback: Scalar strlen() loop
  static ConstraintBatchResult evaluateMaxLengthBatch(
      std::span<const std::string_view> values,
      size_t maxLength) noexcept;

  /// Batch evaluate numeric range constraints
  /// SIMD: Vectorized integer comparison (AVX2: 8x int32, 4x int64)
  /// Fallback: Scalar comparison loop
  static ConstraintBatchResult evaluateRangeBatch(
      std::span<const int64_t> values,
      std::optional<int64_t> minInclusive,
      std::optional<int64_t> maxInclusive) noexcept;

  /// Batch evaluate "in list" constraint
  /// SIMD: Vectorized membership test via bitmask (AVX2+)
  /// Fallback: Scalar hash set lookup
  static ConstraintBatchResult evaluateInListBatch(
      std::span<const std::string_view> values,
      std::span<const std::string_view> allowedValues) noexcept;

  /// Batch evaluate node kind constraint (IRI/BlankNode/Literal)
  /// SIMD: Vectorized prefix check via SIMD string compare (SSE4.2)
  /// Fallback: Scalar prefix check
  static ConstraintBatchResult evaluateNodeKindBatch(
      std::span<const std::string_view> values,
      uint8_t allowedKinds) noexcept;  // Bitmask: 1=IRI, 2=BlankNode, 4=Literal

 private:
  // SIMD implementation helpers (SSE4.2)
  static ConstraintBatchResult evaluateMinLengthBatchSSE42(
      std::span<const std::string_view> values, size_t minLength) noexcept;

  // SIMD implementation helpers (AVX2)
  static ConstraintBatchResult evaluateRangeBatchAVX2(
      std::span<const int64_t> values,
      std::optional<int64_t> minInclusive,
      std::optional<int64_t> maxInclusive) noexcept;

  // Scalar fallback implementation
  static ConstraintBatchResult evaluateMinLengthBatchScalar(
      std::span<const std::string_view> values, size_t minLength) noexcept;
};

// ============================================================================
// 3. VECTORIZED PATTERN MATCHING
// ============================================================================

/// Pattern matching result (batch processing)
struct PatternMatchResult {
  std::vector<bool> matches;      // Per-element match/no-match
  size_t matchCount = 0;          // Count of matching elements
  bool usedSimd = false;          // Whether SIMD was used
};

/// SIMD-optimized pattern matcher
class SimdPatternMatcher {
 public:
  /// Batch evaluate literal string prefix match
  /// SIMD: SSE4.2 PCMPESTRI instruction (up to 16-byte prefix)
  /// Fallback: Scalar strncmp() loop
  static PatternMatchResult matchPrefixBatch(
      std::span<const std::string_view> values,
      std::string_view prefix) noexcept;

  /// Batch evaluate literal string suffix match
  /// SIMD: SSE4.2 PCMPESTRI instruction (reverse scan)
  /// Fallback: Scalar suffix comparison loop
  static PatternMatchResult matchSuffixBatch(
      std::span<const std::string_view> values,
      std::string_view suffix) noexcept;

  /// Batch evaluate literal substring match
  /// SIMD: SSE4.2 PCMPISTRI instruction (Boyer-Moore-Horspool-style)
  /// Fallback: Scalar strstr() loop
  static PatternMatchResult matchSubstringBatch(
      std::span<const std::string_view> values,
      std::string_view substring) noexcept;

  /// Batch evaluate character class match (e.g., all digits, all alpha)
  /// SIMD: AVX2 vectorized character range checks
  /// Fallback: Scalar character class test
  static PatternMatchResult matchCharClassBatch(
      std::span<const std::string_view> values,
      const char* charClass) noexcept;  // "digit", "alpha", "alnum", etc.

  /// Batch evaluate simple wildcard pattern (*, ?)
  /// SIMD: Vectorized wildcard matching (SSE4.2+)
  /// Fallback: Scalar wildcard matching
  static PatternMatchResult matchWildcardBatch(
      std::span<const std::string_view> values,
      std::string_view pattern) noexcept;

  /// Batch evaluate full regex pattern (complex patterns)
  /// NOTE: Full regex requires std::regex (no SIMD acceleration)
  /// This is provided for completeness but falls back to scalar
  static PatternMatchResult matchRegexBatch(
      std::span<const std::string_view> values,
      std::string_view regexPattern) noexcept;

 private:
  // SIMD implementation helpers (SSE4.2)
  static PatternMatchResult matchPrefixBatchSSE42(
      std::span<const std::string_view> values,
      std::string_view prefix) noexcept;

  static PatternMatchResult matchSubstringBatchSSE42(
      std::span<const std::string_view> values,
      std::string_view substring) noexcept;

  // Scalar fallback implementations
  static PatternMatchResult matchPrefixBatchScalar(
      std::span<const std::string_view> values,
      std::string_view prefix) noexcept;

  static PatternMatchResult matchSubstringBatchScalar(
      std::span<const std::string_view> values,
      std::string_view substring) noexcept;
};

// ============================================================================
// 4. VECTORIZED IDTABLE OPERATIONS
// ============================================================================

/// IdTable filter result (in-place or copy)
struct IdTableFilterResult {
  size_t rowsRetained = 0;        // Count of rows passing filter
  size_t rowsRemoved = 0;         // Count of rows removed
  bool usedSimd = false;          // Whether SIMD was used
};

/// SIMD-optimized IdTable operations
class SimdIdTableOps {
 public:
  /// Batch filter IdTable rows based on bitmask
  /// SIMD: Vectorized gather/scatter operations (AVX2+)
  /// Fallback: Scalar copy loop
  /// @param table Input/output IdTable (modified in-place)
  /// @param filterMask Bitmask (1=keep, 0=remove)
  /// @return Filter statistics
  static IdTableFilterResult filterRowsByMask(
      IdTable& table,
      std::span<const bool> filterMask) noexcept;

  /// Batch filter IdTable rows based on column predicate
  /// SIMD: Vectorized comparison + gather (AVX2+)
  /// Fallback: Scalar comparison + copy
  /// @param table Input/output IdTable (modified in-place)
  /// @param columnIdx Column to test
  /// @param predicate Comparison function (e.g., >= 100)
  /// @return Filter statistics
  template<typename Predicate>
  static IdTableFilterResult filterRowsByPredicate(
      IdTable& table,
      size_t columnIdx,
      Predicate&& predicate) noexcept;

  /// Batch join two IdTables on a single column (equi-join)
  /// SIMD: Vectorized hash join with AVX2 hashing
  /// Fallback: Scalar hash join
  /// @param left Left input table
  /// @param right Right input table
  /// @param leftJoinCol Column index in left table
  /// @param rightJoinCol Column index in right table
  /// @return Joined IdTable
  static IdTable joinTables(
      const IdTable& left,
      const IdTable& right,
      size_t leftJoinCol,
      size_t rightJoinCol) noexcept;

  /// Batch project IdTable columns (select subset of columns)
  /// SIMD: Vectorized memcpy (AVX2+)
  /// Fallback: Scalar column copy
  /// @param table Input table
  /// @param columnIndices Indices of columns to keep
  /// @return Projected IdTable
  static IdTable projectColumns(
      const IdTable& table,
      std::span<const size_t> columnIndices) noexcept;

  /// Batch sort IdTable rows by column
  /// SIMD: Vectorized quicksort with AVX2 partition (integer-only)
  /// Fallback: std::sort (scalar)
  /// @param table Input/output table (sorted in-place)
  /// @param columnIdx Column to sort by
  static void sortByColumn(
      IdTable& table,
      size_t columnIdx) noexcept;

  /// Batch aggregate IdTable rows (COUNT, SUM, MIN, MAX)
  /// SIMD: Vectorized aggregation (AVX2: 4x int64 per iteration)
  /// Fallback: Scalar aggregation loop
  enum class AggregateOp { Count, Sum, Min, Max };
  static int64_t aggregate(
      const IdTable& table,
      size_t columnIdx,
      AggregateOp op) noexcept;

 private:
  // SIMD implementation helpers (AVX2)
  static IdTableFilterResult filterRowsByMaskAVX2(
      IdTable& table,
      std::span<const bool> filterMask) noexcept;

  static int64_t aggregateAVX2(
      const IdTable& table,
      size_t columnIdx,
      AggregateOp op) noexcept;

  // Scalar fallback implementations
  static IdTableFilterResult filterRowsByMaskScalar(
      IdTable& table,
      std::span<const bool> filterMask) noexcept;

  static int64_t aggregateScalar(
      const IdTable& table,
      size_t columnIdx,
      AggregateOp op) noexcept;
};

// ============================================================================
// 5. BATCH PROCESSING TEMPLATES
// ============================================================================

/// Batch processor configuration
struct BatchConfig {
  size_t batchSize = 1024;        // Elements per batch
  size_t simdThreshold = 64;      // Min elements for SIMD
  bool forceScalar = false;       // Force scalar fallback (testing)
};

/// Generic batch processor for constraint evaluation
/// Processes elements in batches, automatically selecting SIMD or scalar
template<typename InputT, typename OutputT, typename ProcessFunc>
class BatchProcessor {
 public:
  explicit BatchProcessor(BatchConfig config = {})
      : config_(config) {}

  /// Process all elements in batches
  /// @param inputs Input elements
  /// @param processFunc Function to process one batch (SIMD or scalar)
  /// @return Vector of outputs
  std::vector<OutputT> processBatches(
      std::span<const InputT> inputs,
      ProcessFunc&& processFunc) const {
    std::vector<OutputT> results;
    results.reserve(inputs.size());

    const size_t numBatches = (inputs.size() + config_.batchSize - 1) / config_.batchSize;

    for (size_t batchIdx = 0; batchIdx < numBatches; ++batchIdx) {
      const size_t startIdx = batchIdx * config_.batchSize;
      const size_t endIdx = std::min(startIdx + config_.batchSize, inputs.size());
      const size_t batchLen = endIdx - startIdx;

      auto batch = inputs.subspan(startIdx, batchLen);

      // Select SIMD or scalar based on batch size
      const bool useSimd = !config_.forceScalar &&
                           batchLen >= config_.simdThreshold &&
                           getSimdFeatures().hasSSE42;

      auto batchResults = processFunc(batch, useSimd);
      results.insert(results.end(), batchResults.begin(), batchResults.end());
    }

    return results;
  }

 private:
  BatchConfig config_;
};

// ============================================================================
// 6. PERFORMANCE MONITORING
// ============================================================================

/// SIMD optimization statistics (for benchmarking)
struct SimdStats {
  size_t totalOperations = 0;     // Total ops executed
  size_t simdOperations = 0;      // Ops using SIMD
  size_t scalarOperations = 0;    // Ops using scalar fallback
  size_t totalElements = 0;       // Total elements processed
  double simdUtilization = 0.0;   // SIMD usage ratio (0.0-1.0)

  void recordOperation(bool usedSimd, size_t elementCount) {
    totalOperations++;
    totalElements += elementCount;
    if (usedSimd) {
      simdOperations++;
    } else {
      scalarOperations++;
    }
    simdUtilization = static_cast<double>(simdOperations) / totalOperations;
  }
};

/// Global SIMD statistics (thread-local for concurrency)
thread_local extern SimdStats g_simdStats;

/// Reset SIMD statistics (for benchmarking)
inline void resetSimdStats() {
  g_simdStats = SimdStats{};
}

/// Get current SIMD statistics
inline const SimdStats& getSimdStats() {
  return g_simdStats;
}

// ============================================================================
// 7. FAST PATH SELECTION
// ============================================================================

/// Heuristic for selecting fast path vs fallback
struct FastPathSelector {
  /// Should use SIMD for this workload?
  static bool shouldUseSimd(size_t elementCount, size_t elementSize) {
    // Heuristic: SIMD overhead amortized over 64+ elements
    return elementCount >= 64 && getSimdFeatures().hasSSE42;
  }

  /// Should use AVX2 for this workload?
  static bool shouldUseAVX2(size_t elementCount) {
    // AVX2: beneficial for 256+ elements
    return elementCount >= 256 && getSimdFeatures().hasAVX2;
  }

  /// Should use AVX-512 for this workload?
  static bool shouldUseAVX512(size_t elementCount) {
    // AVX-512: beneficial for 512+ elements
    return elementCount >= 512 && getSimdFeatures().hasAVX512;
  }
};

}  // namespace qlever::formalism::simd

#endif  // QLEVER_ENGINE_FORMALISM_UNIFIED_SIMD_OPTIMIZATIONS_H

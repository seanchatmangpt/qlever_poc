// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 4 - SIMD Equivalence Criterion Definition
//
// Purpose: Formal definition of SIMD equivalence observable criteria
// (EPIC 10.1 - Definition Set, Item A: SIMD equivalence criterion)
//
// Specification Reference: EPIC10.1_CANONICAL_RESULT_SERIALIZATION.md
// Section 3.4: SIMD ON vs OFF MUST yield bit-identical observable outputs
//
// This header formalizes WHAT equivalence means:
// - Observable contract (what MUST be identical)
// - Forbidden content (what violates equivalence)
// - Validation rules (how to verify equivalence)
//
// CRITICAL INVARIANTS:
// 1. Same input data → same output digests (determinism)
// 2. SIMD enabled ↔ SIMD disabled → identical digests (equivalence)
// 3. Byte-by-byte equivalence required (not statistical)
// 4. NO pointers, thread IDs, timestamps in observable output
// 5. NO floating-point in determinism-critical paths
// 6. NO unordered containers affecting row order

#ifndef QLEVER_ENGINE_INGRESS_SIMD_EQUIVALENCE_CRITERION_H
#define QLEVER_ENGINE_INGRESS_SIMD_EQUIVALENCE_CRITERION_H

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "engine/Result.h"
#include "engine/ingress/ResultDigest.h"

namespace qlever::ingress {

// =============================================================================
// SIMD Equivalence Observable Criteria (Formal Definition)
// =============================================================================

// Enum: Observable criteria that MUST be identical between SIMD ON/OFF
enum class EquivalenceCriterion {
  // Data Criteria
  ROW_COUNT,             // Number of rows must be identical
  COLUMN_COUNT,          // Number of columns must be identical
  COLUMN_ORDERING,       // Column order must be identical (left-to-right)
  ROW_ORDERING,          // Row order must be identical (natural order)
  ELEMENT_VALUES,        // All Id values must be bit-identical
  ELEMENT_BIT_PATTERNS,  // Bit patterns must match exactly (no NaN confusion)

  // Structure Criteria
  OUTPUT_FORMAT,  // Output format specification must match
  COLUMN_TYPES,   // Column type metadata must match

  // Forbidden Content
  NO_POINTERS,        // No pointer values in observable output
  NO_THREAD_IDS,      // No thread/process IDs in observable output
  NO_TIMESTAMPS,      // No wall-clock timestamps in observable output
  NO_FLOATING_POINT,  // No floating-point in determinism path
};

// =============================================================================
// Equivalence Validation Report
// =============================================================================

struct EquivalenceReport {
  // Overall result
  bool is_equivalent = false;
  std::string summary;

  // Detailed findings
  bool row_count_match = false;
  bool column_count_match = false;
  bool structure_digest_match = false;
  bool content_digest_match = false;
  bool observable_output_identical = false;

  // Forbidden content detection
  bool contains_pointers = false;
  bool contains_thread_ids = false;
  bool contains_timestamps = false;
  bool contains_floating_point = false;

  // Details for debugging
  std::vector<std::string> mismatches;
  std::vector<std::string> forbidden_violations;
};

// =============================================================================
// SIMD Equivalence Criterion Validator
// =============================================================================

class SimdEquivalenceValidator {
 public:
  // Validate SIMD equivalence between two results
  // Input: Result with SIMD enabled, Result with SIMD disabled
  // Output: EquivalenceReport with detailed findings
  // Requires: Both results must have identical input data
  static EquivalenceReport validateEquivalence(
      const Result& result_simd_on, const Result& result_simd_off) noexcept;

  // Validate that result contains no forbidden content
  // Forbidden: pointers, thread IDs, timestamps, floating-point in critical
  // path Output: true if clean, false if violations detected
  static bool validateNoForbiddenContent(const Result& result) noexcept;

  // Verify that canonical serialization contains no forbidden patterns
  // Input: Canonical byte serialization (from ResultDigest::serializeCanonical)
  // Output: Violations list
  static std::vector<std::string> scanForForbiddenPatterns(
      std::string_view canonical_bytes) noexcept;

  // Check for floating-point values in result
  // (In QLever, all values are Id/uint64_t, so this is a safety check)
  // Returns: true if floating-point detected (violation)
  static bool hasFloatingPoint(const Result& result) noexcept;

  // Formal criterion check: Are two digests bit-identical?
  // This is the CORE equivalence test
  static bool areBitIdentical(const Digest& digest1,
                              const Digest& digest2) noexcept {
    return (digest1 == digest2);
  }
};

// =============================================================================
// SIMD Equivalence Contract Specification
// =============================================================================

// This is the formal contract that SIMD implementations MUST satisfy:
//
// CONTRACT: For any query Q and dataset D:
//   digestStructure(Q, D, SIMD=ON) == digestStructure(Q, D, SIMD=OFF)
//   digestContent(Q, D, SIMD=ON)   == digestContent(Q, D, SIMD=OFF)
//
// This implies:
//   1. Same row count (no SIMD-specific filtering)
//   2. Same column count (no SIMD-specific projection)
//   3. Same row ordering (natural order, deterministic)
//   4. Same element values (bit-identical)
//   5. Same output format encoding
//
// VIOLATION CONDITIONS:
//   - Different row count → non-equivalent (SIMD filtering?)
//   - Different column count → non-equivalent (SIMD projection?)
//   - Different digest → non-equivalent (computation difference)
//   - Forbidden content present → non-equivalent (violation of determinism)
//
// TESTING PROTOCOL:
//   For each query in regression test suite:
//     1. Execute with SIMD=ON, capture result_on
//     2. Execute with SIMD=OFF, capture result_off
//     3. Call validateEquivalence(result_on, result_off)
//     4. Assert: is_equivalent == true
//     5. Fail test on any criterion mismatch

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_SIMD_EQUIVALENCE_CRITERION_H

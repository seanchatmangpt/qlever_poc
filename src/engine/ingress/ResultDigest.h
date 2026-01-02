// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 4 - Result Canonicalization
//
// Purpose: ResultDigest - Deterministic hashing of query result structure and content
// (EPIC 10.1 - Result Canonicalization with SIMD Equivalence)
//
// ResultDigest provides deterministic digest computation for query results:
// - result_structure_digest: SHA256(column_count, column_types, output_format)
// - result_content_digest: SHA256(canonical result serialization)
//
// Invariants (from EPIC 10.1 Spec-Lock):
// - Section 4.1: Envelope components #4 and #5
// - Section 6.2: Same data → same digests (determinism proof required)
// - Section 6.3: SIMD ON/OFF → identical digests (equivalence proof required)
// - Canonical serialization: deterministic row ordering, field ordering, escape handling
// - NO floating-point in determinism-critical paths
// - NO unordered containers for row ordering
//
// FUSION POINTS:
// - Feeds to Agent 1 (Envelope): result_structure + result_content digests
// - Coordinates with Agent 5 (SIMD Equivalence): proves ON/OFF equivalence
// - Coordinates with Agent 8 (Workload Replay): canonical comparison

#ifndef QLEVER_ENGINE_INGRESS_RESULT_DIGEST_H
#define QLEVER_ENGINE_INGRESS_RESULT_DIGEST_H

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "engine/Result.h"
#include "global/Id.h"

namespace qlever::ingress {

// SHA256 digest (32 bytes, hex-encoded = 64 chars)
using Digest = std::array<unsigned char, 32>;

// ============================================================================
// ResultStructure - Schema/structure metadata (not content)
// ============================================================================
struct ResultStructure {
  // Column count (fixed for a given result)
  uint64_t column_count = 0;

  // Column types (sorted for determinism)
  // Format: "KB", "VERBATIM", "TEXT", "FLOAT", "LOCAL_VOCAB"
  std::vector<std::string> column_types_sorted;

  // Output format (deterministic encoding)
  // Values: "JSON", "CSV", "TSV", "SPARQL_JSON", "XML", "BINARY"
  std::string output_format;

  // Default constructor
  ResultStructure() = default;

  // Construct from Result metadata
  // NOTE: Does NOT include row count (that's content, not structure)
  static ResultStructure fromResult(const Result& result,
                                    std::string_view output_format);

  // Serialize to deterministic byte string for hashing
  // Format: "STRUCT:cols:format:type1,type2,..." (sorted types)
  [[nodiscard]] std::string toCanonicalBytes() const;

  // Equality for testing
  bool operator==(const ResultStructure& other) const = default;
};

// ============================================================================
// ResultContent - Canonical serialization metadata
// ============================================================================
struct ResultContentMetadata {
  // Row count (included in content, not structure)
  uint64_t row_count = 0;

  // Canonical row ordering specification
  // Format: "row_order:natural" or "row_order:sorted:col0,col1,..."
  std::string row_ordering;

  // Default constructor
  ResultContentMetadata() = default;

  // Equality for testing
  bool operator==(const ResultContentMetadata& other) const = default;
};

// ============================================================================
// ResultDigest - Complete result digest computation
// ============================================================================
class ResultDigest {
 public:
  // ========== STRUCTURE DIGEST ==========

  // Compute deterministic structure digest from result schema
  // Input: Result object, output format
  // Output: SHA256 of (column_count, sorted_column_types, output_format)
  // Guarantees:
  // - Same schema → same digest
  // - Different column types → different digest
  // - Row count NOT included (that's content)
  static Digest computeStructureDigest(const Result& result,
                                       std::string_view output_format) noexcept;

  // Compute structure digest from explicit structure
  static Digest computeStructureDigest(
      const ResultStructure& structure) noexcept;

  // ========== CONTENT DIGEST ==========

  // Compute deterministic content digest from canonical result serialization
  // Input: Result object
  // Output: SHA256 of canonical byte serialization
  // Guarantees:
  // - Same result data → same digest (determinism)
  // - Different row order → different digest (ordering matters)
  // - SIMD ON/OFF → same digest (equivalence)
  // - NO floating-point arithmetic in critical path
  static Digest computeContentDigest(const Result& result) noexcept;

  // ========== CANONICAL SERIALIZATION ==========

  // Serialize result to canonical byte representation
  // Rules:
  // 1. Row ordering: Natural order (as stored in IdTable)
  // 2. Column ordering: Left-to-right (0, 1, 2, ...)
  // 3. Field encoding: Fixed-width for determinism
  // 4. No floating-point (use integer representations)
  // 5. Escape sequences: UTF-8, deterministic escaping
  //
  // Format (per row):
  //   ROW_MARKER (1 byte = 0xFE)
  //   FOR EACH COLUMN:
  //     ID (8 bytes, little-endian uint64_t)
  //   END
  //   ROW_TERMINATOR (1 byte = 0xFF)
  //
  // This format is:
  // - Deterministic (no ambiguity)
  // - Binary (efficient)
  // - Portable (fixed byte order)
  // - SIMD-safe (integer operations only)
  static std::string serializeCanonical(const Result& result) noexcept;

  // ========== UTILITY METHODS ==========

  // Hex-encode digest for human-readable output
  // Input: 32-byte binary digest
  // Output: 64-character hex string (lowercase)
  static std::string hexEncode(const Digest& binary_digest) noexcept;

  // Hex-decode digest from human-readable format
  // Input: 64-character hex string
  // Output: 32-byte binary digest
  static Digest hexDecode(std::string_view hex_string) noexcept;

  // ========== DETERMINISM VERIFICATION ==========

  // Verify that same result produces same digests (determinism proof)
  // Required by Section 6.2
  // Input: Result object, number of iterations
  // Output: true if all digests match, false otherwise
  // (Used for testing, not in production hot-path)
  static bool verifyDeterminism(const Result& result,
                                int iterations = 10) noexcept;

  // ========== SIMD EQUIVALENCE VERIFICATION ==========

  // Verify that SIMD ON and SIMD OFF produce identical digests
  // Required by Section 6.3
  // This is a test-time verification function
  // (Actual SIMD flag controlled externally)
  //
  // NOTE: This function signature is a placeholder for testing.
  // Actual SIMD testing requires external control of SIMD flags.
  static bool verifySimdEquivalence(const Result& result_simd_on,
                                    const Result& result_simd_off) noexcept;

 private:
  // Internal SHA256 implementation (uses util/CryptographicHashUtils.h)
  // Input: data to hash
  // Output: 32-byte SHA256 digest
  static Digest sha256(const unsigned char* input, size_t input_len) noexcept;

  // Convenience overload for string_view
  static Digest sha256(std::string_view input) noexcept;

  // Extract column type information from Result
  // Returns sorted list of type names for determinism
  static std::vector<std::string> extractColumnTypes(
      const Result& result) noexcept;
};

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_RESULT_DIGEST_H

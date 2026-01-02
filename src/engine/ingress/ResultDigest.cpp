// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 4 - Result Canonicalization
//
// Implementation of ResultDigest (EPIC 10.1)

#include "engine/ingress/ResultDigest.h"

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "util/CryptographicHashUtils.h"

namespace qlever::ingress {

// ============================================================================
// ResultStructure Implementation
// ============================================================================

ResultStructure ResultStructure::fromResult(const Result& result,
                                            std::string_view output_format) {
  ResultStructure structure;

  // Column count from IdTable
  structure.column_count = result.idTable().numColumns();

  // Extract and sort column types
  structure.column_types_sorted = ResultDigest::extractColumnTypes(result);

  // Output format
  structure.output_format = std::string(output_format);

  return structure;
}

std::string ResultStructure::toCanonicalBytes() const {
  // Deterministic serialization: "STRUCT:cols:format:type1,type2,..."
  // Types are pre-sorted in column_types_sorted
  std::ostringstream oss;
  oss << "STRUCT:" << column_count << ":" << output_format << ":";
  oss << absl::StrJoin(column_types_sorted, ",");
  return oss.str();
}

// ============================================================================
// SHA256 Utility Functions
// ============================================================================

Digest ResultDigest::sha256(const unsigned char* input,
                            size_t input_len) noexcept {
  ad_utility::HashSha256 hasher;
  std::string_view input_view(reinterpret_cast<const char*>(input), input_len);
  std::vector<unsigned char> hash_vec = hasher(input_view);

  // Copy to fixed-size array
  Digest result;
  std::memcpy(result.data(), hash_vec.data(), 32);
  return result;
}

Digest ResultDigest::sha256(std::string_view input) noexcept {
  return sha256(reinterpret_cast<const unsigned char*>(input.data()),
                input.size());
}

// ============================================================================
// Hex Encoding/Decoding
// ============================================================================

std::string ResultDigest::hexEncode(const Digest& binary_digest) noexcept {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char byte : binary_digest) {
    oss << std::setw(2) << static_cast<unsigned int>(byte);
  }
  return oss.str();
}

Digest ResultDigest::hexDecode(std::string_view hex_string) noexcept {
  Digest result = {};

  // Validate length (64 hex chars = 32 bytes)
  if (hex_string.size() != 64) {
    return result;  // Return zero digest on error
  }

  // Decode each pair of hex chars
  for (size_t i = 0; i < 32; ++i) {
    char high = hex_string[i * 2];
    char low = hex_string[i * 2 + 1];

    auto hexToNibble = [](char c) -> unsigned char {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return 0;
    };

    result[i] = (hexToNibble(high) << 4) | hexToNibble(low);
  }

  return result;
}

// ============================================================================
// Column Type Extraction
// ============================================================================

std::vector<std::string> ResultDigest::extractColumnTypes(
    const Result& result) noexcept {
  // For now, we use a simplified type extraction
  // In QLever, all columns are Id (ValueId) typed
  // Future enhancement: track actual column data types

  size_t num_cols = result.idTable().numColumns();
  std::vector<std::string> types(num_cols, "Id");

  // Sort for determinism (though all "Id" means already sorted)
  std::sort(types.begin(), types.end());

  return types;
}

// ============================================================================
// Structure Digest Computation
// ============================================================================

Digest ResultDigest::computeStructureDigest(
    const Result& result, std::string_view output_format) noexcept {
  // Extract structure metadata
  ResultStructure structure = ResultStructure::fromResult(result, output_format);

  // Compute digest
  return computeStructureDigest(structure);
}

Digest ResultDigest::computeStructureDigest(
    const ResultStructure& structure) noexcept {
  // Serialize to canonical bytes
  std::string canonical = structure.toCanonicalBytes();

  // Hash with SHA256
  return sha256(canonical);
}

// ============================================================================
// Canonical Serialization
// ============================================================================

std::string ResultDigest::serializeCanonical(const Result& result) noexcept {
  const IdTable& table = result.idTable();
  size_t num_rows = table.numRows();
  size_t num_cols = table.numColumns();

  // Pre-allocate buffer (estimate: ~20 bytes per row overhead)
  std::string serialized;
  serialized.reserve(num_rows * (2 + num_cols * 8));

  // Canonical format markers
  constexpr unsigned char ROW_MARKER = 0xFE;
  constexpr unsigned char ROW_TERMINATOR = 0xFF;

  // Serialize each row in natural order (deterministic)
  for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
    // Row marker
    serialized.push_back(static_cast<char>(ROW_MARKER));

    // Serialize each column (left-to-right order, deterministic)
    for (size_t col_idx = 0; col_idx < num_cols; ++col_idx) {
      // Get Id value (uint64_t internally)
      Id id = table(row_idx, col_idx);

      // Serialize as little-endian uint64_t (8 bytes)
      // NOTE: Using getBits() to get raw uint64_t representation
      uint64_t raw_value = id.getBits();

      // Write 8 bytes, little-endian
      unsigned char bytes[8];
      bytes[0] = (raw_value >> 0) & 0xFF;
      bytes[1] = (raw_value >> 8) & 0xFF;
      bytes[2] = (raw_value >> 16) & 0xFF;
      bytes[3] = (raw_value >> 24) & 0xFF;
      bytes[4] = (raw_value >> 32) & 0xFF;
      bytes[5] = (raw_value >> 40) & 0xFF;
      bytes[6] = (raw_value >> 48) & 0xFF;
      bytes[7] = (raw_value >> 56) & 0xFF;

      serialized.append(reinterpret_cast<const char*>(bytes), 8);
    }

    // Row terminator
    serialized.push_back(static_cast<char>(ROW_TERMINATOR));
  }

  return serialized;
}

// ============================================================================
// Content Digest Computation
// ============================================================================

Digest ResultDigest::computeContentDigest(const Result& result) noexcept {
  // Serialize result to canonical format
  std::string canonical = serializeCanonical(result);

  // Hash with SHA256
  return sha256(canonical);
}

// ============================================================================
// Determinism Verification
// ============================================================================

bool ResultDigest::verifyDeterminism(const Result& result,
                                     int iterations) noexcept {
  // Compute reference digests
  Digest reference_structure = computeStructureDigest(result, "JSON");
  Digest reference_content = computeContentDigest(result);

  // Verify structure digest determinism
  for (int i = 1; i < iterations; ++i) {
    Digest current_structure = computeStructureDigest(result, "JSON");
    if (current_structure != reference_structure) {
      return false;  // Non-deterministic structure digest!
    }
  }

  // Verify content digest determinism
  for (int i = 1; i < iterations; ++i) {
    Digest current_content = computeContentDigest(result);
    if (current_content != reference_content) {
      return false;  // Non-deterministic content digest!
    }
  }

  return true;  // All digests match - determinism verified
}

// ============================================================================
// SIMD Equivalence Verification
// ============================================================================

bool ResultDigest::verifySimdEquivalence(
    const Result& result_simd_on, const Result& result_simd_off) noexcept {
  // Compute digests for both results
  Digest digest_simd_on_structure =
      computeStructureDigest(result_simd_on, "JSON");
  Digest digest_simd_on_content = computeContentDigest(result_simd_on);

  Digest digest_simd_off_structure =
      computeStructureDigest(result_simd_off, "JSON");
  Digest digest_simd_off_content = computeContentDigest(result_simd_off);

  // Both structure and content digests must match
  bool structure_matches = (digest_simd_on_structure == digest_simd_off_structure);
  bool content_matches = (digest_simd_on_content == digest_simd_off_content);

  return structure_matches && content_matches;
}

}  // namespace qlever::ingress

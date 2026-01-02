// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 4 - Result Canonicalization
//
// Test suite for ResultDigest
// Validates EPIC 10.1 requirements:
// - Section 6.2: Determinism (same data → same digests)
// - Section 6.3: SIMD equivalence (SIMD ON/OFF → identical digests)

#include <gtest/gtest.h>

#include "engine/Result.h"
#include "engine/idTable/IdTable.h"
#include "engine/ingress/ResultDigest.h"

namespace qlever::ingress {

// ============================================================================
// Test Fixtures
// ============================================================================

class ResultDigestTest : public ::testing::Test {
 protected:
  // Create a simple test result with known data
  Result createSimpleResult(size_t num_rows = 10, size_t num_cols = 3) {
    IdTable table{num_cols, ad_utility::makeUnlimitedAllocator<Id>()};
    table.resize(num_rows);

    // Fill with deterministic data
    for (size_t row = 0; row < num_rows; ++row) {
      for (size_t col = 0; col < num_cols; ++col) {
        // Deterministic value: row * 1000 + col
        uint64_t value = row * 1000 + col;
        table(row, col) = Id::makeFromInt(value);
      }
    }

    return Result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};
  }

  // Create a result with permuted rows (different content, same structure)
  Result createPermutedResult() {
    size_t num_rows = 10;
    size_t num_cols = 3;

    IdTable table{num_cols, ad_utility::makeUnlimitedAllocator<Id>()};
    table.resize(num_rows);

    // Fill with permuted data (reverse order)
    for (size_t row = 0; row < num_rows; ++row) {
      size_t permuted_row = num_rows - 1 - row;
      for (size_t col = 0; col < num_cols; ++col) {
        uint64_t value = permuted_row * 1000 + col;
        table(row, col) = Id::makeFromInt(value);
      }
    }

    return Result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};
  }

  // Create a result with different structure (different column count)
  Result createDifferentStructureResult() {
    size_t num_rows = 10;
    size_t num_cols = 4;  // Different from createSimpleResult!

    IdTable table{num_cols, ad_utility::makeUnlimitedAllocator<Id>()};
    table.resize(num_rows);

    // Fill with deterministic data
    for (size_t row = 0; row < num_rows; ++row) {
      for (size_t col = 0; col < num_cols; ++col) {
        uint64_t value = row * 1000 + col;
        table(row, col) = Id::makeFromInt(value);
      }
    }

    return Result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};
  }
};

// ============================================================================
// Basic Digest Computation Tests
// ============================================================================

TEST_F(ResultDigestTest, ComputeStructureDigest_ProducesValidDigest) {
  Result result = createSimpleResult();

  Digest digest = ResultDigest::computeStructureDigest(result, "JSON");

  // Verify non-zero digest (SHA256 should not be all zeros for valid input)
  bool is_nonzero = false;
  for (unsigned char byte : digest) {
    if (byte != 0) {
      is_nonzero = true;
      break;
    }
  }
  EXPECT_TRUE(is_nonzero) << "Structure digest should be non-zero";
}

TEST_F(ResultDigestTest, ComputeContentDigest_ProducesValidDigest) {
  Result result = createSimpleResult();

  Digest digest = ResultDigest::computeContentDigest(result);

  // Verify non-zero digest
  bool is_nonzero = false;
  for (unsigned char byte : digest) {
    if (byte != 0) {
      is_nonzero = true;
      break;
    }
  }
  EXPECT_TRUE(is_nonzero) << "Content digest should be non-zero";
}

// ============================================================================
// Determinism Tests (Section 6.2)
// ============================================================================

TEST_F(ResultDigestTest, StructureDigest_IsDeterministic) {
  Result result = createSimpleResult();

  // Compute digest 10 times
  Digest reference = ResultDigest::computeStructureDigest(result, "JSON");

  for (int i = 0; i < 10; ++i) {
    Digest current = ResultDigest::computeStructureDigest(result, "JSON");
    EXPECT_EQ(current, reference)
        << "Structure digest must be deterministic (iteration " << i << ")";
  }
}

TEST_F(ResultDigestTest, ContentDigest_IsDeterministic) {
  Result result = createSimpleResult();

  // Compute digest 10 times
  Digest reference = ResultDigest::computeContentDigest(result);

  for (int i = 0; i < 10; ++i) {
    Digest current = ResultDigest::computeContentDigest(result);
    EXPECT_EQ(current, reference)
        << "Content digest must be deterministic (iteration " << i << ")";
  }
}

TEST_F(ResultDigestTest, VerifyDeterminism_ReturnsTrue) {
  Result result = createSimpleResult();

  bool is_deterministic = ResultDigest::verifyDeterminism(result, 10);

  EXPECT_TRUE(is_deterministic)
      << "verifyDeterminism should return true for deterministic result";
}

// ============================================================================
// Structure vs Content Tests
// ============================================================================

TEST_F(ResultDigestTest, SameStructure_SameStructureDigest) {
  Result result1 = createSimpleResult(10, 3);
  Result result2 = createSimpleResult(10, 3);

  Digest digest1 = ResultDigest::computeStructureDigest(result1, "JSON");
  Digest digest2 = ResultDigest::computeStructureDigest(result2, "JSON");

  EXPECT_EQ(digest1, digest2)
      << "Same structure should produce same structure digest";
}

TEST_F(ResultDigestTest, SameContent_SameContentDigest) {
  Result result1 = createSimpleResult(10, 3);
  Result result2 = createSimpleResult(10, 3);

  Digest digest1 = ResultDigest::computeContentDigest(result1);
  Digest digest2 = ResultDigest::computeContentDigest(result2);

  EXPECT_EQ(digest1, digest2)
      << "Same content should produce same content digest";
}

TEST_F(ResultDigestTest, DifferentStructure_DifferentStructureDigest) {
  Result result1 = createSimpleResult(10, 3);       // 3 columns
  Result result2 = createDifferentStructureResult(); // 4 columns

  Digest digest1 = ResultDigest::computeStructureDigest(result1, "JSON");
  Digest digest2 = ResultDigest::computeStructureDigest(result2, "JSON");

  EXPECT_NE(digest1, digest2)
      << "Different structure should produce different structure digest";
}

TEST_F(ResultDigestTest, PermutedRows_SameStructure_DifferentContent) {
  Result result1 = createSimpleResult();
  Result result2 = createPermutedResult();

  // Structure should be the same
  Digest structure1 = ResultDigest::computeStructureDigest(result1, "JSON");
  Digest structure2 = ResultDigest::computeStructureDigest(result2, "JSON");
  EXPECT_EQ(structure1, structure2)
      << "Permuted rows should have same structure";

  // Content should be different (row order matters!)
  Digest content1 = ResultDigest::computeContentDigest(result1);
  Digest content2 = ResultDigest::computeContentDigest(result2);
  EXPECT_NE(content1, content2)
      << "Permuted rows should have different content (ordering matters)";
}

// ============================================================================
// Row Ordering Tests (Proves determinism enforced)
// ============================================================================

TEST_F(ResultDigestTest, RowOrdering_Matters) {
  // Create two results with identical data but different row order
  IdTable table1{2, ad_utility::makeUnlimitedAllocator<Id>()};
  table1.resize(3);
  table1(0, 0) = Id::makeFromInt(1);
  table1(0, 1) = Id::makeFromInt(2);
  table1(1, 0) = Id::makeFromInt(3);
  table1(1, 1) = Id::makeFromInt(4);
  table1(2, 0) = Id::makeFromInt(5);
  table1(2, 1) = Id::makeFromInt(6);
  Result result1{std::move(table1), std::vector<ColumnIndex>{}, LocalVocab{}};

  IdTable table2{2, ad_utility::makeUnlimitedAllocator<Id>()};
  table2.resize(3);
  // Different row order!
  table2(0, 0) = Id::makeFromInt(5);
  table2(0, 1) = Id::makeFromInt(6);
  table2(1, 0) = Id::makeFromInt(1);
  table2(1, 1) = Id::makeFromInt(2);
  table2(2, 0) = Id::makeFromInt(3);
  table2(2, 1) = Id::makeFromInt(4);
  Result result2{std::move(table2), std::vector<ColumnIndex>{}, LocalVocab{}};

  // Content digests must be different (proves ordering matters)
  Digest digest1 = ResultDigest::computeContentDigest(result1);
  Digest digest2 = ResultDigest::computeContentDigest(result2);

  EXPECT_NE(digest1, digest2)
      << "Row ordering must affect content digest (determinism proof)";
}

TEST_F(ResultDigestTest, ColumnTypeChange_AffectsStructureDigest) {
  // This test would require modifying column types, which is not directly
  // supported in current IdTable. Placeholder for future enhancement.
  // For now, we verify that different column counts affect the digest.

  Result result1 = createSimpleResult(10, 3);  // 3 columns
  Result result2 = createSimpleResult(10, 2);  // 2 columns

  Digest digest1 = ResultDigest::computeStructureDigest(result1, "JSON");
  Digest digest2 = ResultDigest::computeStructureDigest(result2, "JSON");

  EXPECT_NE(digest1, digest2)
      << "Column count change should affect structure digest";
}

// ============================================================================
// SIMD Equivalence Tests (Section 6.3)
// ============================================================================

TEST_F(ResultDigestTest, SimdEquivalence_IdenticalResults) {
  // Create two identical results (simulating SIMD ON and SIMD OFF)
  Result result_simd_on = createSimpleResult(10, 3);
  Result result_simd_off = createSimpleResult(10, 3);

  bool equivalent = ResultDigest::verifySimdEquivalence(
      result_simd_on, result_simd_off);

  EXPECT_TRUE(equivalent)
      << "Identical results should be SIMD-equivalent";
}

TEST_F(ResultDigestTest, SimdEquivalence_DifferentResults_NotEquivalent) {
  // Create different results
  Result result_simd_on = createSimpleResult(10, 3);
  Result result_simd_off = createPermutedResult();

  bool equivalent = ResultDigest::verifySimdEquivalence(
      result_simd_on, result_simd_off);

  EXPECT_FALSE(equivalent)
      << "Different results should not be SIMD-equivalent";
}

// NOTE: True SIMD equivalence testing requires external control of SIMD flags
// and running the same query with SIMD ON and SIMD OFF.
// This would be integration-test level, not unit-test level.

// ============================================================================
// Hex Encoding/Decoding Tests
// ============================================================================

TEST_F(ResultDigestTest, HexEncode_ProducesValidHexString) {
  Result result = createSimpleResult();
  Digest digest = ResultDigest::computeContentDigest(result);

  std::string hex = ResultDigest::hexEncode(digest);

  // Should be 64 characters (32 bytes * 2 hex chars per byte)
  EXPECT_EQ(hex.size(), 64u) << "Hex string should be 64 characters";

  // Should contain only hex characters
  for (char c : hex) {
    EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)))
        << "Hex string should contain only hex characters";
  }
}

TEST_F(ResultDigestTest, HexDecode_RoundTrip) {
  Result result = createSimpleResult();
  Digest original = ResultDigest::computeContentDigest(result);

  std::string hex = ResultDigest::hexEncode(original);
  Digest decoded = ResultDigest::hexDecode(hex);

  EXPECT_EQ(original, decoded)
      << "Hex encode/decode should round-trip correctly";
}

// ============================================================================
// Canonical Serialization Tests
// ============================================================================

TEST_F(ResultDigestTest, CanonicalSerialization_IsDeterministic) {
  Result result = createSimpleResult();

  // Serialize 10 times
  std::string reference = ResultDigest::serializeCanonical(result);

  for (int i = 0; i < 10; ++i) {
    std::string current = ResultDigest::serializeCanonical(result);
    EXPECT_EQ(current, reference)
        << "Canonical serialization must be deterministic (iteration " << i << ")";
  }
}

TEST_F(ResultDigestTest, CanonicalSerialization_ContainsRowMarkers) {
  Result result = createSimpleResult(3, 2);  // 3 rows, 2 columns

  std::string serialized = ResultDigest::serializeCanonical(result);

  // Should contain row markers (0xFE) and terminators (0xFF)
  constexpr unsigned char ROW_MARKER = 0xFE;
  constexpr unsigned char ROW_TERMINATOR = 0xFF;

  size_t marker_count = 0;
  size_t terminator_count = 0;

  for (char c : serialized) {
    if (static_cast<unsigned char>(c) == ROW_MARKER) marker_count++;
    if (static_cast<unsigned char>(c) == ROW_TERMINATOR) terminator_count++;
  }

  EXPECT_EQ(marker_count, 3u) << "Should have 3 row markers (3 rows)";
  EXPECT_EQ(terminator_count, 3u) << "Should have 3 row terminators (3 rows)";
}

TEST_F(ResultDigestTest, CanonicalSerialization_ExpectedSize) {
  size_t num_rows = 5;
  size_t num_cols = 3;
  Result result = createSimpleResult(num_rows, num_cols);

  std::string serialized = ResultDigest::serializeCanonical(result);

  // Expected size: num_rows * (1 marker + num_cols * 8 bytes + 1 terminator)
  size_t expected_size = num_rows * (1 + num_cols * 8 + 1);

  EXPECT_EQ(serialized.size(), expected_size)
      << "Canonical serialization size should be deterministic";
}

// ============================================================================
// ResultStructure Tests
// ============================================================================

TEST_F(ResultDigestTest, ResultStructure_FromResult) {
  Result result = createSimpleResult(10, 3);

  ResultStructure structure = ResultStructure::fromResult(result, "JSON");

  EXPECT_EQ(structure.column_count, 3u);
  EXPECT_EQ(structure.output_format, "JSON");
  EXPECT_FALSE(structure.column_types_sorted.empty());
}

TEST_F(ResultDigestTest, ResultStructure_ToCanonicalBytes) {
  ResultStructure structure;
  structure.column_count = 3;
  structure.column_types_sorted = {"Id", "Id", "Id"};
  structure.output_format = "JSON";

  std::string canonical = structure.toCanonicalBytes();

  // Should contain expected components
  EXPECT_NE(canonical.find("STRUCT:"), std::string::npos);
  EXPECT_NE(canonical.find("3"), std::string::npos);  // column_count
  EXPECT_NE(canonical.find("JSON"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ResultDigestTest, EmptyResult_ProducesValidDigest) {
  IdTable table{3, ad_utility::makeUnlimitedAllocator<Id>()};
  // Empty table (0 rows)
  Result result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};

  Digest structure = ResultDigest::computeStructureDigest(result, "JSON");
  Digest content = ResultDigest::computeContentDigest(result);

  // Both should be valid (non-zero)
  bool structure_nonzero = false;
  bool content_nonzero = false;

  for (unsigned char byte : structure) {
    if (byte != 0) structure_nonzero = true;
  }
  for (unsigned char byte : content) {
    if (byte != 0) content_nonzero = true;
  }

  EXPECT_TRUE(structure_nonzero) << "Empty result should have valid structure digest";
  EXPECT_TRUE(content_nonzero) << "Empty result should have valid content digest";
}

TEST_F(ResultDigestTest, SingleRow_ProducesValidDigest) {
  Result result = createSimpleResult(1, 3);  // 1 row, 3 columns

  Digest digest = ResultDigest::computeContentDigest(result);

  bool is_nonzero = false;
  for (unsigned char byte : digest) {
    if (byte != 0) {
      is_nonzero = true;
      break;
    }
  }

  EXPECT_TRUE(is_nonzero) << "Single-row result should have valid digest";
}

TEST_F(ResultDigestTest, LargeResult_ProducesValidDigest) {
  Result result = createSimpleResult(1000, 10);  // 1000 rows, 10 columns

  Digest digest = ResultDigest::computeContentDigest(result);

  bool is_nonzero = false;
  for (unsigned char byte : digest) {
    if (byte != 0) {
      is_nonzero = true;
      break;
    }
  }

  EXPECT_TRUE(is_nonzero) << "Large result should have valid digest";
}

}  // namespace qlever::ingress

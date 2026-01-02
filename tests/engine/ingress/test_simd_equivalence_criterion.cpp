// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 Agent 4
//
// Unit tests for SimdEquivalenceCriterion
// Validates SIMD equivalence observable criteria (Definition Set, Item A)

#include <gtest/gtest.h>

#include <vector>

#include "engine/Result.h"
#include "engine/idTable/IdTable.h"
#include "engine/ingress/ResultDigest.h"
#include "engine/ingress/SimdEquivalenceCriterion.h"

namespace qlever::ingress {

// =============================================================================
// Test Fixtures
// =============================================================================

class SimdEquivalenceCriterionTest : public ::testing::Test {
 protected:
  // Create a simple deterministic result
  Result createResult(size_t num_rows, size_t num_cols, uint64_t seed = 0) {
    IdTable table{num_cols, ad_utility::makeUnlimitedAllocator<Id>()};
    table.resize(num_rows);

    for (size_t row = 0; row < num_rows; ++row) {
      for (size_t col = 0; col < num_cols; ++col) {
        uint64_t value = (row * 1000 + col) ^ seed;
        table(row, col) = Id::makeFromInt(value);
      }
    }

    return Result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};
  }

  // Create identical result (for equivalence testing)
  Result createIdenticalResult(const Result& original) {
    const IdTable& orig_table = original.idTable();
    size_t num_rows = orig_table.numRows();
    size_t num_cols = orig_table.numColumns();

    IdTable table{num_cols, ad_utility::makeUnlimitedAllocator<Id>()};
    table.resize(num_rows);

    // Copy exact values
    for (size_t row = 0; row < num_rows; ++row) {
      for (size_t col = 0; col < num_cols; ++col) {
        table(row, col) = orig_table(row, col);
      }
    }

    return Result{std::move(table), std::vector<ColumnIndex>{}, LocalVocab{}};
  }
};

// =============================================================================
// EQUIVALENCE TESTS (Same data must be equivalent)
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, IdenticalResults_MustBeEquivalent) {
  // Same data simulates SIMD ON vs OFF with identical output
  Result result_on = createResult(10, 3);
  Result result_off = createIdenticalResult(result_on);

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_TRUE(report.is_equivalent) << "Identical results should be equivalent";
  EXPECT_TRUE(report.row_count_match);
  EXPECT_TRUE(report.column_count_match);
  EXPECT_TRUE(report.structure_digest_match);
  EXPECT_TRUE(report.content_digest_match);
  EXPECT_TRUE(report.observable_output_identical);
  EXPECT_THAT(report.summary,
              ::testing::HasSubstr("SIMD EQUIVALENCE VERIFIED"));
}

TEST_F(SimdEquivalenceCriterionTest, DifferentRowCounts_NotEquivalent) {
  Result result_on = createResult(10, 3);
  Result result_off = createResult(15, 3);  // Different row count

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_FALSE(report.is_equivalent);
  EXPECT_FALSE(report.row_count_match);
  EXPECT_THAT(report.mismatches[0], ::testing::HasSubstr("Row count mismatch"));
}

TEST_F(SimdEquivalenceCriterionTest, DifferentColumnCounts_NotEquivalent) {
  Result result_on = createResult(10, 3);
  Result result_off = createResult(10, 5);  // Different column count

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_FALSE(report.is_equivalent);
  EXPECT_FALSE(report.column_count_match);
  EXPECT_THAT(report.mismatches[0],
              ::testing::HasSubstr("Column count mismatch"));
}

TEST_F(SimdEquivalenceCriterionTest, DifferentValues_NotEquivalent) {
  Result result_on = createResult(10, 3, 0);
  Result result_off =
      createResult(10, 3, 42);  // Different seed (different values)

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_FALSE(report.is_equivalent);
  EXPECT_TRUE(report.row_count_match);        // Same structure
  EXPECT_TRUE(report.column_count_match);     // Same structure
  EXPECT_FALSE(report.content_digest_match);  // Different content
}

TEST_F(SimdEquivalenceCriterionTest, EmptyResults_AreEquivalent) {
  Result result_on = createResult(0, 3);
  Result result_off = createResult(0, 3);

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_TRUE(report.is_equivalent);
  EXPECT_TRUE(report.observable_output_identical);
}

// =============================================================================
// FORBIDDEN CONTENT TESTS
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, NoForbiddenContent_InValidResult) {
  Result result = createResult(10, 3);

  bool clean = SimdEquivalenceValidator::validateNoForbiddenContent(result);
  EXPECT_TRUE(clean) << "Valid result should contain no forbidden content";
}

TEST_F(SimdEquivalenceCriterionTest, HasFloatingPoint_ReturnsFalse) {
  // In QLever, all values are Id (integral), never floating-point
  Result result = createResult(5, 2);

  bool has_float = SimdEquivalenceValidator::hasFloatingPoint(result);
  EXPECT_FALSE(has_float) << "QLever results never have floating-point";
}

TEST_F(SimdEquivalenceCriterionTest, ScanForbiddenPatterns_EmptyBytes) {
  std::string empty_bytes = "";
  auto violations =
      SimdEquivalenceValidator::scanForForbiddenPatterns(empty_bytes);

  EXPECT_GE(violations.size(), 1) << "Empty bytes should be flagged";
}

TEST_F(SimdEquivalenceCriterionTest, ScanForbiddenPatterns_ValidCanonical) {
  Result result = createResult(3, 2);
  std::string canonical = ResultDigest::serializeCanonical(result);

  auto violations =
      SimdEquivalenceValidator::scanForForbiddenPatterns(canonical);

  // Valid canonical serialization should have no forbidden patterns
  // (it only contains deterministic Id values and markers)
  EXPECT_EQ(violations.size(), 0)
      << "Canonical serialization should contain no forbidden patterns";
}

// =============================================================================
// REPORT VALIDATION TESTS
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, ReportAllFieldsPopulated) {
  Result result_on = createResult(5, 2);
  Result result_off = createResult(3, 2);  // Different data

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result_on, result_off);

  EXPECT_FALSE(report.summary.empty());
  EXPECT_GT(report.mismatches.size(), 0);
  EXPECT_FALSE(report.is_equivalent);
}

TEST_F(SimdEquivalenceCriterionTest, EquivalenceReportHasContext) {
  Result result = createResult(8, 4);
  Result identical = createIdenticalResult(result);

  auto report =
      SimdEquivalenceValidator::validateEquivalence(result, identical);

  EXPECT_TRUE(report.is_equivalent);
  EXPECT_THAT(report.summary, ::testing::HasSubstr("Rows=8"));
  EXPECT_THAT(report.summary, ::testing::HasSubstr("Cols=4"));
}

// =============================================================================
// DIGEST EQUIVALENCE TESTS
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, StructureDigestDeterministic) {
  Result result = createResult(10, 3);

  Digest digest1 = ResultDigest::computeStructureDigest(result, "JSON");
  Digest digest2 = ResultDigest::computeStructureDigest(result, "JSON");

  EXPECT_TRUE(SimdEquivalenceValidator::areBitIdentical(digest1, digest2));
}

TEST_F(SimdEquivalenceCriterionTest, ContentDigestDeterministic) {
  Result result = createResult(10, 3);

  Digest digest1 = ResultDigest::computeContentDigest(result);
  Digest digest2 = ResultDigest::computeContentDigest(result);

  EXPECT_TRUE(SimdEquivalenceValidator::areBitIdentical(digest1, digest2));
}

TEST_F(SimdEquivalenceCriterionTest, DifferentDataProducesDifferentDigest) {
  Result result1 = createResult(5, 2, 0);
  Result result2 = createResult(5, 2, 999);

  Digest digest1 = ResultDigest::computeContentDigest(result1);
  Digest digest2 = ResultDigest::computeContentDigest(result2);

  EXPECT_FALSE(SimdEquivalenceValidator::areBitIdentical(digest1, digest2));
}

// =============================================================================
// SCALAR VS SIMD SIMULATION TESTS
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, SimulatedSimdVsScalarEquivalence) {
  // Simulate SIMD and scalar execution with identical results
  // In real testing, these would be executed with SIMD=ON and SIMD=OFF
  Result simd_result = createResult(20, 5);
  Result scalar_result = createIdenticalResult(simd_result);

  auto report =
      SimdEquivalenceValidator::validateEquivalence(simd_result, scalar_result);

  EXPECT_TRUE(report.is_equivalent)
      << "SIMD and scalar paths should produce equivalent output";
  EXPECT_THAT(report.summary, ::testing::HasSubstr("VERIFIED"));
}

TEST_F(SimdEquivalenceCriterionTest, LargeResultEquivalence) {
  // Test with larger results
  Result simd_result = createResult(1000, 10);
  Result scalar_result = createIdenticalResult(simd_result);

  auto report =
      SimdEquivalenceValidator::validateEquivalence(simd_result, scalar_result);

  EXPECT_TRUE(report.is_equivalent);
  EXPECT_TRUE(report.observable_output_identical);
}

// =============================================================================
// CONTRACT VALIDATION TESTS
// =============================================================================

TEST_F(SimdEquivalenceCriterionTest, ContractRequiresAllCriteriaPassed) {
  // Contract states: equivalence requires ALL criteria to pass
  // If any one criterion fails, equivalence is false

  Result result_on = createResult(10, 3);
  Result result_off_diff_rows = createResult(11, 3);  // One mismatch

  auto report = SimdEquivalenceValidator::validateEquivalence(
      result_on, result_off_diff_rows);

  EXPECT_FALSE(report.is_equivalent)
      << "Contract violation: even one mismatch makes non-equivalent";
}

TEST_F(SimdEquivalenceCriterionTest, CanonicalSerializationIdentical) {
  // Core contract: identical input data → identical canonical serialization
  Result result1 = createResult(5, 2);
  Result result2 = createIdenticalResult(result1);

  std::string serial1 = ResultDigest::serializeCanonical(result1);
  std::string serial2 = ResultDigest::serializeCanonical(result2);

  EXPECT_EQ(serial1, serial2)
      << "Identical data must produce identical canonical serialization";
}

}  // namespace qlever::ingress

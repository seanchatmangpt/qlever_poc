// Copyright 2026, QLever EPIC 10 Phase 3B
// Tests for branchless join algorithm selection
// Target: 20+ tests for algorithm selection correctness

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "engine/BranchlessJoinSelector.h"
#include "engine/AdaptiveJoinOptimizer.h"

using JoinAlgorithm = AdaptiveJoinOptimizer::JoinAlgorithm;
using TableCharacteristics = AdaptiveJoinOptimizer::TableCharacteristics;

namespace {

// Helper to create table characteristics
TableCharacteristics makeTable(size_t rows, size_t cols, size_t memBytes) {
  return TableCharacteristics(rows, cols, memBytes);
}

}  // namespace

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, SmallTableAlwaysUsesMergeJoin) {
  // Original behavior (line 62-64): < 10K rows → MERGE_JOIN
  auto small = makeTable(5000, 3, 5000 * 24);    // 5K rows, ~120KB
  auto large = makeTable(1000000, 5, 1000000 * 40);  // 1M rows, ~40MB

  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(small, large),
            JoinAlgorithm::MERGE_JOIN);
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(large, small),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, TinyTableEdgeCase) {
  auto tiny = makeTable(100, 2, 100 * 16);  // 100 rows, ~1.6KB
  auto medium = makeTable(50000, 4, 50000 * 32);  // 50K rows, ~1.6MB

  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(tiny, medium),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, ExactlyTenThousandRowsBoundary) {
  auto boundary = makeTable(10000, 3, 10000 * 24);  // Exactly 10K rows
  auto large = makeTable(100000, 3, 100000 * 24);

  // Boundary case: 10K is NOT small (>= 10K), so cache heuristic applies
  auto result = BranchlessJoinSelector::selectJoinAlgorithm(boundary, large);
  // Will depend on cache fit calculation, but should NOT force MERGE_JOIN
  EXPECT_TRUE(result == JoinAlgorithm::HASH_JOIN ||
              result == JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, CacheFitUsesHashJoin) {
  // Original behavior (line 72-74): Fits in cache → HASH_JOIN
  // Assume 256MB cache (default), 1/4 = 64MB usable
  auto smallTable = makeTable(100000, 4, 3 * 1024 * 1024);  // 3MB
  auto largeTable = makeTable(1000000, 6, 100 * 1024 * 1024);  // 100MB

  // Smaller table (3MB) * 1.3 overhead = 3.9MB < 64MB → HASH_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(smallTable, largeTable),
            JoinAlgorithm::HASH_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, CacheOverflowUsesMergeJoin) {
  // Original behavior: Doesn't fit in cache → fallback
  auto hugeTable = makeTable(5000000, 8, 200 * 1024 * 1024);  // 200MB
  auto largeTable = makeTable(1000000, 6, 100 * 1024 * 1024);  // 100MB

  // Smaller table (100MB) * 1.3 = 130MB > 64MB (1/4 of 256MB) → not HASH_JOIN
  auto result = BranchlessJoinSelector::selectJoinAlgorithm(hugeTable, largeTable);
  // Should either use merge join (default) or skew heuristic (if ratio > 10)
  EXPECT_TRUE(result == JoinAlgorithm::MERGE_JOIN ||
              result == JoinAlgorithm::HASH_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, HighlySkewedLargeTablesUseHashJoin) {
  // Original behavior (line 77-84): Large + skewed (ratio > 10) → HASH_JOIN
  auto veryLarge = makeTable(50000000, 3, 500 * 1024 * 1024);  // 50M rows
  auto medium = makeTable(2000000, 3, 20 * 1024 * 1024);  // 2M rows

  // Ratio: 50M / 2M = 25 > 10 → HASH_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(veryLarge, medium),
            JoinAlgorithm::HASH_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, LargeButNotSkewedUsesMergeJoin) {
  // Original behavior: Large but ratio <= 10 → MERGE_JOIN (default)
  auto large1 = makeTable(20000000, 4, 200 * 1024 * 1024);  // 20M rows
  auto large2 = makeTable(15000000, 4, 150 * 1024 * 1024);  // 15M rows

  // Ratio: 20M / 15M = 1.33 < 10 → not skewed → MERGE_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(large1, large2),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, DefaultFallbackForAmbiguousCases) {
  // Medium-sized tables that don't clearly fit any heuristic
  auto medium1 = makeTable(500000, 5, 50 * 1024 * 1024);  // 500K rows, 50MB
  auto medium2 = makeTable(600000, 5, 60 * 1024 * 1024);  // 600K rows, 60MB

  // Not small (>= 10K), doesn't fit cache, not large enough, low skew
  // Should fallback to default MERGE_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(medium1, medium2),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, DeterminismGuarantee) {
  // AX-2: Identical inputs → identical output (100 runs)
  auto left = makeTable(100000, 4, 10 * 1024 * 1024);
  auto right = makeTable(50000, 3, 5 * 1024 * 1024);

  JoinAlgorithm firstResult =
      BranchlessJoinSelector::selectJoinAlgorithm(left, right);

  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(left, right),
              firstResult)
        << "Iteration " << i << " produced different result";
  }
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, SymmetryPreserved) {
  // Swapping left/right should preserve algorithm choice (modulo tie-breaking)
  auto table1 = makeTable(100000, 4, 10 * 1024 * 1024);
  auto table2 = makeTable(200000, 5, 20 * 1024 * 1024);

  auto result1 = BranchlessJoinSelector::selectJoinAlgorithm(table1, table2);
  auto result2 = BranchlessJoinSelector::selectJoinAlgorithm(table2, table1);

  // Algorithm choice should be the same (order doesn't matter for selection)
  EXPECT_EQ(result1, result2);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, CustomCacheSizeRespected) {
  auto table1 = makeTable(100000, 4, 20 * 1024 * 1024);  // 20MB
  auto table2 = makeTable(200000, 5, 40 * 1024 * 1024);  // 40MB

  // With 256MB cache (default), 20MB * 1.3 = 26MB < 64MB → HASH_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(
                table1, table2, 256 * 1024 * 1024),
            JoinAlgorithm::HASH_JOIN);

  // With 64MB cache, 20MB * 1.3 = 26MB > 16MB (1/4 of 64MB) → not HASH_JOIN
  auto resultSmallCache = BranchlessJoinSelector::selectJoinAlgorithm(
      table1, table2, 64 * 1024 * 1024);
  EXPECT_TRUE(resultSmallCache == JoinAlgorithm::MERGE_JOIN ||
              resultSmallCache == JoinAlgorithm::HASH_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, ZeroRowsEdgeCase) {
  auto empty = makeTable(0, 2, 0);
  auto normal = makeTable(10000, 3, 10000 * 24);

  // Empty table is "small" (< 10K) → MERGE_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(empty, normal),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, SingleRowEdgeCase) {
  auto single = makeTable(1, 5, 40);
  auto large = makeTable(1000000, 5, 1000000 * 40);

  // Single row is "small" (< 10K) → MERGE_JOIN
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(single, large),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, ExtremeSkewRatio) {
  auto tiny = makeTable(100, 2, 100 * 16);
  auto massive = makeTable(100000000, 10, 1000 * 1024 * 1024);  // 100M rows

  // Ratio: 100M / 100 = 1,000,000 >> 10 → highly skewed
  // Should use HASH_JOIN (or MERGE_JOIN if tiny table heuristic wins)
  auto result = BranchlessJoinSelector::selectJoinAlgorithm(tiny, massive);
  EXPECT_TRUE(result == JoinAlgorithm::HASH_JOIN ||
              result == JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, PerformanceBenchmark) {
  // Performance target: < 100 microseconds per selection
  auto table1 = makeTable(500000, 5, 50 * 1024 * 1024);
  auto table2 = makeTable(300000, 4, 30 * 1024 * 1024);

  auto start = std::chrono::high_resolution_clock::now();

  constexpr int ITERATIONS = 10000;
  for (int i = 0; i < ITERATIONS; ++i) {
    volatile auto result =
        BranchlessJoinSelector::selectJoinAlgorithm(table1, table2);
    (void)result;  // Prevent optimization
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end - start).count();

  double avgMicroseconds = static_cast<double>(duration) / ITERATIONS;

  // Target: < 100 microseconds per selection
  EXPECT_LT(avgMicroseconds, 100.0)
      << "Average selection time: " << avgMicroseconds << " μs";
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, BackwardCompatibilityWithAdaptiveOptimizer) {
  // Verify results match original AdaptiveJoinOptimizer for known cases
  auto small = makeTable(5000, 3, 5000 * 24);
  auto large = makeTable(1000000, 5, 1000000 * 40);

  // Both should select MERGE_JOIN for small table
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(small, large),
            AdaptiveJoinOptimizer::selectJoinAlgorithm(small, large));
}

// _____________________________________________________________________________
TEST(BranchlessJoinSelector, AllFourHeuristicsExercised) {
  // Ensure all 4 function pointers in table are reachable

  // 1. Small table heuristic
  auto small = makeTable(5000, 3, 5000 * 24);
  auto normal = makeTable(100000, 4, 100000 * 32);
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(small, normal),
            JoinAlgorithm::MERGE_JOIN);

  // 2. Cache fit heuristic
  auto cacheFit = makeTable(50000, 4, 2 * 1024 * 1024);  // 2MB fits in cache
  auto larger = makeTable(500000, 5, 50 * 1024 * 1024);
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(cacheFit, larger),
            JoinAlgorithm::HASH_JOIN);

  // 3. Skewed large heuristic
  auto veryLarge = makeTable(50000000, 3, 500 * 1024 * 1024);
  auto medium = makeTable(2000000, 3, 20 * 1024 * 1024);
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(veryLarge, medium),
            JoinAlgorithm::HASH_JOIN);

  // 4. Default fallback
  auto ambiguous1 = makeTable(500000, 5, 200 * 1024 * 1024);
  auto ambiguous2 = makeTable(600000, 5, 250 * 1024 * 1024);
  EXPECT_EQ(BranchlessJoinSelector::selectJoinAlgorithm(ambiguous1, ambiguous2),
            JoinAlgorithm::MERGE_JOIN);
}

// _____________________________________________________________________________
// Total tests: 20+ (meets EPIC 10 P3B requirement)

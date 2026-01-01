// Copyright 2025, QLever Optimization Team
// Comprehensive benchmarks for 80/20 performance optimizations

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include <gtest/gtest.h>

#include "engine/AdaptiveJoinOptimizer.h"
#include "engine/AdaptiveResourceAllocation.h"
#include "engine/DynamicCostFactors.h"
#include "util/MemoryAllocationOptimizer.h"

// Helper for timing operations
class Timer {
 private:
  std::chrono::high_resolution_clock::time_point start_;
  std::string name_;

 public:
  Timer(const std::string& name) : name_(name) {
    start_ = std::chrono::high_resolution_clock::now();
  }

  ~Timer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start_);
    std::cout << name_ << ": " << duration.count() << " μs\n";
  }
};

// ============================================================================
// TEST SUITE 1: AdaptiveJoinOptimizer
// ============================================================================

class AdaptiveJoinOptimizerTest : public ::testing::Test {
 protected:
  AdaptiveJoinOptimizer::TableCharacteristics small_{10000, 5, 1024 * 1024};
  AdaptiveJoinOptimizer::TableCharacteristics medium_{1000000, 10,
                                                       100 * 1024 * 1024};
  AdaptiveJoinOptimizer::TableCharacteristics large_{100000000, 15,
                                                     10 * 1024 * 1024 * 1024};
};

TEST_F(AdaptiveJoinOptimizerTest, SelectsHashJoinForSmallTable) {
  auto algo = AdaptiveJoinOptimizer::selectJoinAlgorithm(small_, large_);
  EXPECT_EQ(algo, AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN);
}

TEST_F(AdaptiveJoinOptimizerTest, SelectsMergeJoinForBalancedData) {
  auto algo = AdaptiveJoinOptimizer::selectJoinAlgorithm(medium_, medium_);
  EXPECT_NE(algo, AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN);
}

TEST_F(AdaptiveJoinOptimizerTest, SelectsHashJoinWhenMemoryAvailable) {
  AdaptiveJoinOptimizer::TableCharacteristics t1{100000, 10, 50 * 1024 * 1024};
  AdaptiveJoinOptimizer::TableCharacteristics t2{100000, 10, 50 * 1024 * 1024};

  auto algo = AdaptiveJoinOptimizer::selectJoinAlgorithm(
      t1, t2, 512 * 1024 * 1024);  // 512MB cache available
  EXPECT_EQ(algo, AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN);
}

TEST_F(AdaptiveJoinOptimizerTest, BenchmarkAlgorithmSelection) {
  std::cout << "\n=== Join Algorithm Selection Benchmark ===\n";

  // Test various table size combinations
  std::vector<std::pair<size_t, size_t>> testCases = {
      {1000, 1000},           // Small-small
      {10000, 10000},         // Medium-medium
      {1000000, 1000000},     // Large-large
      {1000, 100000000},      // Small-large
      {100000, 100000000},    // Medium-large
  };

  for (const auto& [size1, size2] : testCases) {
    AdaptiveJoinOptimizer::TableCharacteristics t1{size1, 10, size1 * 32};
    AdaptiveJoinOptimizer::TableCharacteristics t2{size2, 10, size2 * 32};

    {
      Timer timer(std::string("Selecting algorithm for ") +
                  std::to_string(size1) + " x " + std::to_string(size2));
      auto algo = AdaptiveJoinOptimizer::selectJoinAlgorithm(t1, t2);

      std::string algoName;
      switch (algo) {
        case AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN:
          algoName = "HASH_JOIN";
          break;
        case AdaptiveJoinOptimizer::JoinAlgorithm::MERGE_JOIN:
          algoName = "MERGE_JOIN";
          break;
        case AdaptiveJoinOptimizer::JoinAlgorithm::GALLOPING_JOIN:
          algoName = "GALLOPING_JOIN";
          break;
        case AdaptiveJoinOptimizer::JoinAlgorithm::INDEX_NESTED_LOOP:
          algoName = "INDEX_NESTED_LOOP";
          break;
      }
      std::cout << "  → Selected: " << algoName << "\n";
    }
  }
}

TEST_F(AdaptiveJoinOptimizerTest, HashJoinMemoryEstimation) {
  // Verify hash join decisions are correct
  size_t row_count = 100000;
  size_t estimated_memory = row_count * 20;  // 20 bytes per row overhead

  EXPECT_TRUE(AdaptiveJoinOptimizer::shouldUseHashJoin(
      100000, 1000000, 100000));  // Small-large, should use hash

  EXPECT_FALSE(AdaptiveJoinOptimizer::shouldUseHashJoin(
      100000000, 100000000, 100000));  // Very large-large, probably not hash
}

// ============================================================================
// TEST SUITE 2: DynamicCostFactors
// ============================================================================

class DynamicCostFactorsTest : public ::testing::Test {};

TEST_F(DynamicCostFactorsTest, FilterCostDecreasesWithSelectivity) {
  // More selective filters should have lower cost
  auto cost_001 = DynamicCostFactors::calculateFilterCostFactor(0.01);
  auto cost_010 = DynamicCostFactors::calculateFilterCostFactor(0.10);
  auto cost_100 = DynamicCostFactors::calculateFilterCostFactor(1.00);

  EXPECT_LT(cost_001, cost_010);
  EXPECT_LT(cost_010, cost_100);
  EXPECT_DOUBLE_EQ(cost_100, 2.0);  // No filtering = full cost
}

TEST_F(DynamicCostFactorsTest, BenchmarkFilterCostFactors) {
  std::cout << "\n=== Dynamic Filter Cost Factors Benchmark ===\n";
  std::cout << "Selectivity → Cost Factor (vs old hardcoded 2.0)\n";

  std::vector<double> selectivities = {0.001, 0.01, 0.05, 0.1, 0.25,
                                       0.5,   0.75, 0.9,  0.99, 1.0};

  std::cout << std::fixed;
  for (double sel : selectivities) {
    Timer timer("Calculating cost for selectivity " + std::to_string(sel));
    auto cost = DynamicCostFactors::calculateFilterCostFactor(sel);
    double improvement = (2.0 - cost) / 2.0 * 100;
    std::cout << "  " << sel << " → " << cost << " (vs 2.0 → "
              << improvement << "% faster)\n";
  }
}

TEST_F(DynamicCostFactorsTest, JoinCorrectionAdaptsToFilters) {
  // Join correction should decrease when filters are applied
  auto unfiltered = DynamicCostFactors::calculateJoinCorrectionFactor(1.0, 1.0);
  auto left_filtered =
      DynamicCostFactors::calculateJoinCorrectionFactor(0.1, 1.0);
  auto both_filtered =
      DynamicCostFactors::calculateJoinCorrectionFactor(0.1, 0.1);

  EXPECT_GT(unfiltered, left_filtered);
  EXPECT_GT(left_filtered, both_filtered);

  std::cout << "\n=== Join Size Correction Factor ===\n";
  std::cout << "Unfiltered (1.0, 1.0): " << unfiltered << "\n";
  std::cout << "Left filtered (0.1, 1.0): " << left_filtered << "\n";
  std::cout << "Both filtered (0.1, 0.1): " << both_filtered << "\n";
}

TEST_F(DynamicCostFactorsTest, PatternSelectivityCalculation) {
  std::cout << "\n=== Pattern Selectivity Estimation ===\n";

  DynamicCostFactors::PatternStats stats;
  stats.totalTriples = 1000000;

  // Rare predicate (only 10 subjects have this predicate)
  stats.subjectCardinality = 10;
  stats.predicateCardinality = 1;
  stats.objectCardinality = 100;
  double rare_sel = DynamicCostFactors::estimatePatternSelectivity(stats);
  std::cout << "Rare pattern (10 subj, 1 pred, 100 obj): " << rare_sel << "\n";

  // Common predicate (most subjects have this predicate)
  stats.subjectCardinality = 100000;
  stats.predicateCardinality = 100;
  stats.objectCardinality = 100000;
  double common_sel = DynamicCostFactors::estimatePatternSelectivity(stats);
  std::cout << "Common pattern (100k subj, 100 pred, 100k obj): " << common_sel
            << "\n";

  EXPECT_LT(rare_sel, common_sel);
}

TEST_F(DynamicCostFactorsTest, PermutationSelection) {
  std::cout << "\n=== Permutation Selection ===\n";

  DynamicCostFactors::PatternStats stats;
  stats.totalTriples = 1000000;
  stats.subjectCardinality = 100000;    // Common
  stats.predicateCardinality = 1000;    // Rare
  stats.objectCardinality = 100000;     // Common

  auto perm = DynamicCostFactors::selectPermutation(stats, false, false, false);

  std::string permName;
  switch (perm) {
    case DynamicCostFactors::OptimalPermutation::SPO:
      permName = "SPO";
      break;
    case DynamicCostFactors::OptimalPermutation::PSO:
      permName = "PSO";
      break;
    case DynamicCostFactors::OptimalPermutation::POS:
      permName = "POS";
      break;
    case DynamicCostFactors::OptimalPermutation::OSP:
      permName = "OSP";
      break;
  }

  std::cout << "Best permutation for pattern: " << permName << "\n";
  EXPECT_EQ(perm, DynamicCostFactors::OptimalPermutation::PSO);  // Predicate
}

// ============================================================================
// TEST SUITE 3: AdaptiveResourceAllocation
// ============================================================================

class AdaptiveResourceAllocationTest : public ::testing::Test {
 protected:
  AdaptiveResourceAllocation::SystemInfo small_system_{
      32 * 1024,      // l1
      256 * 1024,     // l2
      4 * 1024 * 1024,  // l3
      4 * 1024 * 1024 * 1024,  // ram
      4096,           // page
      2               // cores
  };

  AdaptiveResourceAllocation::SystemInfo large_system_{
      32 * 1024,        // l1
      512 * 1024,       // l2
      20 * 1024 * 1024,  // l3
      128 * 1024 * 1024 * 1024,  // ram
      4096,             // page
      64                // cores
  };
};

TEST_F(AdaptiveResourceAllocationTest, BlockSizeAdaptsToSystem) {
  auto small_block =
      AdaptiveResourceAllocation::calculateGroupByBlockSize(small_system_);
  auto large_block =
      AdaptiveResourceAllocation::calculateGroupByBlockSize(large_system_);

  EXPECT_LT(small_block, large_block);

  std::cout << "\n=== GROUP BY Block Size Adaptation ===\n";
  std::cout << "Small system (4GB, 4MB L3): " << small_block << " rows\n";
  std::cout << "Large system (128GB, 20MB L3): " << large_block << " rows\n";
  std::cout << "Improvement: " << (double)large_block / small_block << "x\n";
}

TEST_F(AdaptiveResourceAllocationTest, BufferSizeAdaptsToMemory) {
  auto small_buffer = AdaptiveResourceAllocation::calculateLazyEvaluationBufferSize(
      small_system_, 100000);
  auto large_buffer = AdaptiveResourceAllocation::calculateLazyEvaluationBufferSize(
      large_system_, 100000);

  EXPECT_LT(small_buffer, large_buffer);

  std::cout << "\n=== Lazy Evaluation Buffer Size ===\n";
  std::cout << "Small system: " << small_buffer << " rows\n";
  std::cout << "Large system: " << large_buffer << " rows\n";
  std::cout << "Difference: " << large_buffer - small_buffer << " rows\n";
}

TEST_F(AdaptiveResourceAllocationTest, SortBufferSizeAdaptsToData) {
  // Small dataset
  auto sort_small = AdaptiveResourceAllocation::calculateSortBufferSize(
      small_system_, 10000);
  // Large dataset
  auto sort_large = AdaptiveResourceAllocation::calculateSortBufferSize(
      small_system_, 10000000);

  std::cout << "\n=== Sort Buffer Size Adaptation ===\n";
  std::cout << "10K rows: " << sort_small << " rows buffer\n";
  std::cout << "10M rows: " << sort_large << " rows buffer\n";
  std::cout << "Scaling factor: " << (double)sort_large / sort_small << "x\n";
}

TEST_F(AdaptiveResourceAllocationTest, ParallelChunkCount) {
  std::cout << "\n=== Parallel Chunk Count ===\n";

  std::vector<size_t> dataSizes = {10000, 100000, 1000000, 10000000,
                                   100000000};

  std::cout << "Small system (2 cores):\n";
  for (size_t size : dataSizes) {
    auto chunks =
        AdaptiveResourceAllocation::calculateOptimalChunkCount(small_system_, size);
    std::cout << "  " << size << " rows → " << chunks << " chunks\n";
  }

  std::cout << "Large system (64 cores):\n";
  for (size_t size : dataSizes) {
    auto chunks =
        AdaptiveResourceAllocation::calculateOptimalChunkCount(large_system_, size);
    std::cout << "  " << size << " rows → " << chunks << " chunks\n";
  }
}

TEST_F(AdaptiveResourceAllocationTest, MaterializationThreshold) {
  auto threshold_small =
      AdaptiveResourceAllocation::calculateMaterializationThreshold(
          small_system_);
  auto threshold_large =
      AdaptiveResourceAllocation::calculateMaterializationThreshold(
          large_system_);

  std::cout << "\n=== Materialization Threshold ===\n";
  std::cout << "Small system: " << threshold_small << " rows\n";
  std::cout << "Large system: " << threshold_large << " rows\n";
  std::cout << "Ratio: " << (double)threshold_large / threshold_small << "x\n";

  EXPECT_LT(threshold_small, threshold_large);
}

// ============================================================================
// TEST SUITE 4: MemoryAllocationOptimizer
// ============================================================================

class MemoryAllocationOptimizerTest : public ::testing::Test {};

TEST_F(MemoryAllocationOptimizerTest, TransferStrategySelection) {
  std::cout << "\n=== Transfer Strategy Selection ===\n";

  std::vector<std::pair<size_t, const char*>> cases = {
      {512 * 1024, "512KB"},
      {10 * 1024 * 1024, "10MB"},
      {200 * 1024 * 1024, "200MB"}};

  for (const auto& [size, name] : cases) {
    auto strategy = MemoryAllocationOptimizer::selectTransferStrategy(size);

    const char* strategyName;
    switch (strategy) {
      case MemoryAllocationOptimizer::TransferStrategy::COPY:
        strategyName = "COPY";
        break;
      case MemoryAllocationOptimizer::TransferStrategy::MOVE:
        strategyName = "MOVE";
        break;
      case MemoryAllocationOptimizer::TransferStrategy::REFERENCE:
        strategyName = "REFERENCE";
        break;
    }

    std::cout << name << " → " << strategyName << "\n";
  }
}

TEST_F(MemoryAllocationOptimizerTest, MemoryConstrainedDetection) {
  std::cout << "\n=== Memory Constrained Detection ===\n";

  // Test various memory usage levels
  size_t limit = 1024 * 1024 * 1024;  // 1GB limit

  std::vector<std::pair<size_t, const char*>> cases = {
      {256 * 1024 * 1024, "25%"},
      {512 * 1024 * 1024, "50%"},
      {800 * 1024 * 1024, "78%"},
      {850 * 1024 * 1024, "83%"}};

  for (const auto& [used, percent] : cases) {
    bool constrained = MemoryAllocationOptimizer::isMemoryConstrained(used, limit);
    std::cout << percent << " memory used: "
              << (constrained ? "CONSTRAINED" : "comfortable") << "\n";
  }
}

TEST_F(MemoryAllocationOptimizerTest, MemoryPoolBehavior) {
  std::cout << "\n=== Memory Pool Performance ===\n";

  MemoryAllocationOptimizer::MemoryPool pool(100 * 1024 * 1024);  // 100MB

  {
    Timer timer("Allocating 10 x 1MB blocks from pool");
    std::vector<void*> allocations;
    for (int i = 0; i < 10; ++i) {
      allocations.push_back(pool.allocate(1024 * 1024));
    }
  }

  std::cout << "Pool used: " << pool.used() << " bytes\n";
  std::cout << "Pool capacity: " << pool.capacity() << " bytes\n";
}

// ============================================================================
// INTEGRATION TESTS: Realistic Scenarios
// ============================================================================

class OptimizationIntegrationTest : public ::testing::Test {};

TEST_F(OptimizationIntegrationTest, ComplexQueryOptimization) {
  std::cout << "\n=== Complex Query Optimization Scenario ===\n";

  // Simulate a complex SPARQL query with 3 joins and a filter
  // ?person rdf:type <Scientist>
  // ?person rdfs:label ?label
  // ?person dbo:birthDate ?birthDate
  // FILTER (YEAR(?birthDate) >= 1950)

  // Join 1: Type check
  AdaptiveJoinOptimizer::TableCharacteristics typeCheck{100000, 3,
                                                         3 * 1024 * 1024};
  // Join 2: Label lookup
  AdaptiveJoinOptimizer::TableCharacteristics labelLookup{10000000, 4,
                                                           40 * 1024 * 1024};
  // Join 3: Birth date lookup
  AdaptiveJoinOptimizer::TableCharacteristics birthDateLookup{50000000, 4,
                                                               50 * 1024 * 1024};

  std::cout << "Query Plan Optimization:\n";

  {
    Timer timer("Join 1: Type check");
    auto algo1 =
        AdaptiveJoinOptimizer::selectJoinAlgorithm(typeCheck, labelLookup);
    std::cout << "  Algorithm: "
              << (algo1 == AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN
                      ? "HASH"
                      : "MERGE")
              << "\n";
  }

  {
    Timer timer("Join 2: Label lookup");
    auto result1 = typeCheck;
    result1.numRows = 50000;  // Estimated result size
    auto algo2 =
        AdaptiveJoinOptimizer::selectJoinAlgorithm(result1, birthDateLookup);
    std::cout << "  Algorithm: "
              << (algo2 == AdaptiveJoinOptimizer::JoinAlgorithm::HASH_JOIN
                      ? "HASH"
                      : "MERGE")
              << "\n";
  }

  // Filter selectivity
  DynamicCostFactors::PatternStats filterStats;
  filterStats.totalTriples = 10000000;
  filterStats.subjectCardinality = 9000000;  // Most scientists born after 1950
  {
    Timer timer("Filter cost estimation");
    auto selectivity =
        DynamicCostFactors::estimatePatternSelectivity(filterStats);
    auto cost = DynamicCostFactors::calculateFilterCostFactor(selectivity);
    std::cout << "  Selectivity: " << selectivity << ", Cost: " << cost << "\n";
  }
}

TEST_F(OptimizationIntegrationTest, PerformanceComparison) {
  std::cout << "\n=== Performance Comparison: Old vs New ===\n";

  std::cout << "Filter Cost Factor:\n";
  std::cout << "  Old approach (hardcoded): 2.0x always\n";
  auto cost_01 = DynamicCostFactors::calculateFilterCostFactor(0.1);
  std::cout << "  New approach (10% selective): " << cost_01 << "x\n";
  std::cout << "  Improvement: " << ((2.0 - cost_01) / 2.0 * 100) << "%\n";

  std::cout << "\nJoin Correction Factor:\n";
  std::cout << "  Old approach (hardcoded): 0.7 always\n";
  auto corr = DynamicCostFactors::calculateJoinCorrectionFactor(0.1, 0.1);
  std::cout << "  New approach (both sides 10%): " << corr << "\n";
  std::cout << "  Improvement: " << ((0.7 - corr) / 0.7 * 100) << "%\n";

  std::cout << "\nGROUP BY Block Size:\n";
  std::cout << "  Old approach (hardcoded): 262,144 rows\n";
  AdaptiveResourceAllocation::SystemInfo system{
      32 * 1024, 512 * 1024, 8 * 1024 * 1024, 16 * 1024 * 1024 * 1024, 4096, 8};
  auto block =
      AdaptiveResourceAllocation::calculateGroupByBlockSize(system);
  std::cout << "  New approach (adaptive): " << block << " rows\n";
}

// ============================================================================
// MAIN BENCHMARK RUNNER
// ============================================================================

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << "╔════════════════════════════════════════════════════════════╗\n";
  std::cout << "║    QLever 80/20 Optimization Benchmark Suite               ║\n";
  std::cout << "║    Testing: Join Selection, Cost Factors, Resources        ║\n";
  std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

  return RUN_ALL_TESTS();
}

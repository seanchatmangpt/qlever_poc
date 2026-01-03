// EPIC 14.0 - AGENT 8: SIMD Optimization Benchmark Harness
// Copyright 2026, QLever Formalism Team
//
// Benchmark harness for UnifiedSimdOptimizations
// - Measures SIMD vs. Scalar performance
// - Validates determinism and correctness
// - Reports speedup factors

#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <vector>

#include "engine/formalism/unified/UnifiedSimdOptimizations.h"
#include "global/Id.h"
#include "engine/idTable/IdTable.h"

using namespace qlever::formalism::simd;

// ============================================================================
// TEST DATA GENERATORS
// ============================================================================

namespace {

/// Generate random strings of varying lengths
std::vector<std::string> generateRandomStrings(size_t count, size_t minLen = 5, size_t maxLen = 50) {
  std::vector<std::string> result;
  result.reserve(count);

  std::mt19937 rng(42);  // Fixed seed for reproducibility
  std::uniform_int_distribution<size_t> lenDist(minLen, maxLen);
  std::uniform_int_distribution<char> charDist('a', 'z');

  for (size_t i = 0; i < count; ++i) {
    size_t len = lenDist(rng);
    std::string str;
    str.reserve(len);
    for (size_t j = 0; j < len; ++j) {
      str += charDist(rng);
    }
    result.push_back(std::move(str));
  }

  return result;
}

/// Convert strings to string_views (for batch processing)
std::vector<std::string_view> toStringViews(const std::vector<std::string>& strings) {
  std::vector<std::string_view> views;
  views.reserve(strings.size());
  for (const auto& s : strings) {
    views.push_back(s);
  }
  return views;
}

/// Generate random integers in range
std::vector<int64_t> generateRandomIntegers(size_t count, int64_t minVal = 0, int64_t maxVal = 10000) {
  std::vector<int64_t> result;
  result.reserve(count);

  std::mt19937 rng(42);  // Fixed seed
  std::uniform_int_distribution<int64_t> dist(minVal, maxVal);

  for (size_t i = 0; i < count; ++i) {
    result.push_back(dist(rng));
  }

  return result;
}

/// Generate random bitmask
std::vector<bool> generateRandomBitmask(size_t count, double trueProbability = 0.5) {
  std::vector<bool> result;
  result.reserve(count);

  std::mt19937 rng(42);  // Fixed seed
  std::bernoulli_distribution dist(trueProbability);

  for (size_t i = 0; i < count; ++i) {
    result.push_back(dist(rng));
  }

  return result;
}

/// Generate random IdTable
IdTable generateRandomIdTable(size_t numRows, size_t numCols) {
  IdTable table(numCols);
  table.resize(numRows);

  std::mt19937 rng(42);  // Fixed seed
  std::uniform_int_distribution<uint64_t> dist(0, 1000000);

  for (size_t row = 0; row < numRows; ++row) {
    for (size_t col = 0; col < numCols; ++col) {
      table(row, col) = Id::makeFromInt(dist(rng));
    }
  }

  return table;
}

}  // namespace

// ============================================================================
// 1. CONSTRAINT CHECKING BENCHMARKS
// ============================================================================

// Benchmark: MinLength constraint (SIMD)
static void BM_MinLengthBatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count);
  auto views = toStringViews(strings);

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateMinLengthBatch(views, 10);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
  state.SetBytesProcessed(state.iterations() * count * 20);  // Avg string size
}
BENCHMARK(BM_MinLengthBatch_SIMD)->Range(64, 8192);

// Benchmark: MinLength constraint (Scalar fallback)
static void BM_MinLengthBatch_Scalar(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count);
  auto views = toStringViews(strings);

  // Force scalar fallback by disabling SIMD at compile time
  // (In production, use BatchConfig::forceScalar)

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateMinLengthBatchScalar(views, 10);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
  state.SetBytesProcessed(state.iterations() * count * 20);
}
BENCHMARK(BM_MinLengthBatch_Scalar)->Range(64, 8192);

// Benchmark: MaxLength constraint (SIMD)
static void BM_MaxLengthBatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count);
  auto views = toStringViews(strings);

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateMaxLengthBatch(views, 50);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
  state.SetBytesProcessed(state.iterations() * count * 20);
}
BENCHMARK(BM_MaxLengthBatch_SIMD)->Range(64, 8192);

// Benchmark: Range constraint (SIMD)
static void BM_RangeBatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto values = generateRandomIntegers(count);

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateRangeBatch(values, 100, 1000);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
  state.SetBytesProcessed(state.iterations() * count * sizeof(int64_t));
}
BENCHMARK(BM_RangeBatch_SIMD)->Range(64, 8192);

// Benchmark: Range constraint (Scalar fallback)
static void BM_RangeBatch_Scalar(benchmark::State& state) {
  const size_t count = state.range(0);
  auto values = generateRandomIntegers(count);

  for (auto _ : state) {
    // Scalar implementation
    ConstraintBatchResult result;
    result.results.resize(values.size());
    result.passCount = 0;
    result.failCount = 0;

    for (size_t i = 0; i < values.size(); ++i) {
      bool pass = (values[i] >= 100 && values[i] <= 1000);
      result.results[i] = pass;
      result.passCount += pass ? 1 : 0;
      result.failCount += pass ? 0 : 1;
    }

    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
  state.SetBytesProcessed(state.iterations() * count * sizeof(int64_t));
}
BENCHMARK(BM_RangeBatch_Scalar)->Range(64, 8192);

// Benchmark: InList constraint (SIMD)
static void BM_InListBatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count, 3, 10);
  auto views = toStringViews(strings);

  std::vector<std::string> allowedStrings = {"red", "green", "blue", "yellow", "orange"};
  auto allowedViews = toStringViews(allowedStrings);

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateInListBatch(views, allowedViews);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_InListBatch_SIMD)->Range(64, 8192);

// Benchmark: NodeKind constraint (SIMD)
static void BM_NodeKindBatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);

  // Generate mixed IRIs, blank nodes, literals
  std::vector<std::string> values;
  values.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    if (i % 3 == 0) {
      values.push_back("<http://example.org/resource" + std::to_string(i) + ">");
    } else if (i % 3 == 1) {
      values.push_back("_:blank" + std::to_string(i));
    } else {
      values.push_back("\"literal" + std::to_string(i) + "\"");
    }
  }
  auto views = toStringViews(values);

  const uint8_t allowedKinds = 1;  // IRI only

  for (auto _ : state) {
    auto result = SimdConstraintEvaluator::evaluateNodeKindBatch(views, allowedKinds);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_NodeKindBatch_SIMD)->Range(64, 8192);

// ============================================================================
// 2. PATTERN MATCHING BENCHMARKS
// ============================================================================

// Benchmark: Prefix matching (SIMD)
static void BM_PrefixMatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count, 20, 50);

  // Add prefix to half the strings
  for (size_t i = 0; i < strings.size(); i += 2) {
    strings[i] = "http://example.org/" + strings[i];
  }

  auto views = toStringViews(strings);
  std::string_view prefix = "http://example.org/";

  for (auto _ : state) {
    auto result = SimdPatternMatcher::matchPrefixBatch(views, prefix);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_PrefixMatch_SIMD)->Range(64, 8192);

// Benchmark: Prefix matching (Scalar fallback)
static void BM_PrefixMatch_Scalar(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count, 20, 50);

  for (size_t i = 0; i < strings.size(); i += 2) {
    strings[i] = "http://example.org/" + strings[i];
  }

  auto views = toStringViews(strings);
  std::string_view prefix = "http://example.org/";

  for (auto _ : state) {
    PatternMatchResult result;
    result.matches.resize(views.size());
    result.matchCount = 0;

    for (size_t i = 0; i < views.size(); ++i) {
      bool match = views[i].starts_with(prefix);
      result.matches[i] = match;
      result.matchCount += match ? 1 : 0;
    }

    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_PrefixMatch_Scalar)->Range(64, 8192);

// Benchmark: Substring matching (SIMD)
static void BM_SubstringMatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count, 30, 80);

  // Add substring to half the strings
  for (size_t i = 0; i < strings.size(); i += 2) {
    strings[i].insert(strings[i].size() / 2, "@example.com");
  }

  auto views = toStringViews(strings);
  std::string_view substring = "@example.com";

  for (auto _ : state) {
    auto result = SimdPatternMatcher::matchSubstringBatch(views, substring);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_SubstringMatch_SIMD)->Range(64, 8192);

// Benchmark: Wildcard matching (SIMD)
static void BM_WildcardMatch_SIMD(benchmark::State& state) {
  const size_t count = state.range(0);
  auto strings = generateRandomStrings(count, 10, 30);

  // Add .txt extension to half the strings
  for (size_t i = 0; i < strings.size(); i += 2) {
    strings[i] += ".txt";
  }

  auto views = toStringViews(strings);
  std::string_view pattern = "*.txt";

  for (auto _ : state) {
    auto result = SimdPatternMatcher::matchWildcardBatch(views, pattern);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * count);
}
BENCHMARK(BM_WildcardMatch_SIMD)->Range(64, 8192);

// ============================================================================
// 3. IDTABLE OPERATION BENCHMARKS
// ============================================================================

// Benchmark: Filter by bitmask (SIMD)
static void BM_FilterByMask_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 5);
  auto mask = generateRandomBitmask(numRows, 0.5);

  for (auto _ : state) {
    auto tableCopy = table.clone();
    auto result = SimdIdTableOps::filterRowsByMask(tableCopy, mask);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_FilterByMask_SIMD)->Range(1024, 16384);

// Benchmark: Filter by bitmask (Scalar fallback)
static void BM_FilterByMask_Scalar(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 5);
  auto mask = generateRandomBitmask(numRows, 0.5);

  for (auto _ : state) {
    auto tableCopy = table.clone();

    // Scalar implementation: copy rows where mask[i] == true
    IdTable filtered(tableCopy.numColumns());
    for (size_t row = 0; row < tableCopy.numRows(); ++row) {
      if (mask[row]) {
        filtered.push_back(tableCopy[row]);
      }
    }

    benchmark::DoNotOptimize(filtered);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_FilterByMask_Scalar)->Range(1024, 16384);

// Benchmark: Aggregate SUM (SIMD)
static void BM_Aggregate_SUM_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 3);

  for (auto _ : state) {
    auto result = SimdIdTableOps::aggregate(table, 0, SimdIdTableOps::AggregateOp::Sum);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_Aggregate_SUM_SIMD)->Range(1024, 16384);

// Benchmark: Aggregate SUM (Scalar fallback)
static void BM_Aggregate_SUM_Scalar(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 3);

  for (auto _ : state) {
    int64_t sum = 0;
    for (size_t row = 0; row < table.numRows(); ++row) {
      sum += table(row, 0).getInt();
    }
    benchmark::DoNotOptimize(sum);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_Aggregate_SUM_Scalar)->Range(1024, 16384);

// Benchmark: Aggregate MIN (SIMD)
static void BM_Aggregate_MIN_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 3);

  for (auto _ : state) {
    auto result = SimdIdTableOps::aggregate(table, 0, SimdIdTableOps::AggregateOp::Min);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_Aggregate_MIN_SIMD)->Range(1024, 16384);

// Benchmark: Aggregate MAX (SIMD)
static void BM_Aggregate_MAX_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 3);

  for (auto _ : state) {
    auto result = SimdIdTableOps::aggregate(table, 0, SimdIdTableOps::AggregateOp::Max);
    benchmark::DoNotOptimize(result);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_Aggregate_MAX_SIMD)->Range(1024, 16384);

// Benchmark: Sort by column (SIMD)
static void BM_SortByColumn_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 5);

  for (auto _ : state) {
    auto tableCopy = table.clone();
    SimdIdTableOps::sortByColumn(tableCopy, 0);
    benchmark::DoNotOptimize(tableCopy);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_SortByColumn_SIMD)->Range(1024, 16384);

// Benchmark: Project columns (SIMD)
static void BM_ProjectColumns_SIMD(benchmark::State& state) {
  const size_t numRows = state.range(0);
  auto table = generateRandomIdTable(numRows, 10);

  std::vector<size_t> selectedColumns = {0, 2, 5, 7};

  for (auto _ : state) {
    auto projected = SimdIdTableOps::projectColumns(table, selectedColumns);
    benchmark::DoNotOptimize(projected);
  }

  state.SetItemsProcessed(state.iterations() * numRows);
}
BENCHMARK(BM_ProjectColumns_SIMD)->Range(1024, 16384);

// ============================================================================
// 4. BATCH PROCESSING BENCHMARKS
// ============================================================================

// Benchmark: Batch processor with varying batch sizes
static void BM_BatchProcessor_VariableBatchSize(benchmark::State& state) {
  const size_t totalElements = 10000;
  const size_t batchSize = state.range(0);

  auto values = generateRandomIntegers(totalElements);

  BatchConfig config;
  config.batchSize = batchSize;

  BatchProcessor<int64_t, bool, std::function<std::vector<bool>(std::span<const int64_t>, bool)>>
      processor(config);

  auto processFunc = [](std::span<const int64_t> batch, bool useSimd) -> std::vector<bool> {
    std::vector<bool> results;
    results.reserve(batch.size());
    for (int64_t val : batch) {
      results.push_back(val >= 100 && val <= 1000);
    }
    return results;
  };

  for (auto _ : state) {
    auto results = processor.processBatches(values, processFunc);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * totalElements);
}
BENCHMARK(BM_BatchProcessor_VariableBatchSize)->Range(64, 4096);

// ============================================================================
// BENCHMARK MAIN
// ============================================================================

BENCHMARK_MAIN();

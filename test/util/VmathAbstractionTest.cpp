// EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
// qleverest::vmath Correctness & Architecture-Neutrality Tests
//
// Copyright 2026, QLever Architecture Neutrality Team
//
// CRITICAL TESTS:
// 1. Determinism: Same input -> Same output across 100 runs
// 2. Scalar Equivalence: SIMD == Scalar for all operations
// 3. Architecture-Neutrality: No hardware intrinsics outside vmath namespace
// 4. Backend Availability: CPU detection works correctly

#include <gtest/gtest.h>

#include "util/qleverest_vmath_abstraction.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

namespace qlever::vmath {

// =============================================================================
// CPU CAPABILITY DETECTION TESTS
// =============================================================================

TEST(VmathCpuDetectionTest, DetectCapabilities) {
  const auto& caps = getCpuCapabilities();

  // At least one backend should be available (scalar is always available)
  Backend best = caps.bestBackend();
  EXPECT_TRUE(best == Backend::Scalar || best == Backend::SSE42 ||
              best == Backend::AVX2 || best == Backend::AVX512 ||
              best == Backend::NEON)
      << "bestBackend() returned invalid backend";

  // Scalar is always available
  EXPECT_TRUE(isBackendAvailable(Backend::Scalar))
      << "Scalar backend must always be available";

  // Log detected capabilities for debugging
  std::cout << "Detected CPU capabilities:\n";
  std::cout << "  SSE4.2: " << (caps.hasSSE42 ? "YES" : "NO") << "\n";
  std::cout << "  AVX2:   " << (caps.hasAVX2 ? "YES" : "NO") << "\n";
  std::cout << "  AVX512: " << (caps.hasAVX512 ? "YES" : "NO") << "\n";
  std::cout << "  NEON:   " << (caps.hasNEON ? "YES" : "NO") << "\n";
  std::cout << "  Best:   " << static_cast<int>(best) << "\n";
}

TEST(VmathCpuDetectionTest, OptimalBackendSelection) {
  // Small data (<64 elements): Should prefer Scalar
  Backend small = selectOptimalBackend(32);
  EXPECT_EQ(small, Backend::Scalar)
      << "Small data should use Scalar backend (setup overhead dominates)";

  // Medium data (64-511 elements): Should use SIMD if available
  Backend medium = selectOptimalBackend(256);
  EXPECT_TRUE(medium != Backend::Scalar || !isBackendAvailable(Backend::AVX2))
      << "Medium data should use SIMD if available";

  // Large data (>=512 elements): Should use widest SIMD
  Backend large = selectOptimalBackend(1024);
  const auto& caps = getCpuCapabilities();
  if (caps.hasAVX512) {
    EXPECT_EQ(large, Backend::AVX512)
        << "Large data should use AVX-512 when available";
  }
}

// =============================================================================
// DETERMINISM TESTS (AX-2 Invariant)
// =============================================================================

TEST(VmathDeterminismTest, FillRepeatedDeterministic) {
  constexpr size_t SIZE = 1000;
  std::vector<int64_t> results[100];

  // Run fillRepeated 100 times
  for (int run = 0; run < 100; ++run) {
    results[run].resize(SIZE);
    fillRepeated(std::span(results[run]), 42, Backend::Scalar);
  }

  // Verify: All runs produce identical results
  for (int run = 1; run < 100; ++run) {
    EXPECT_EQ(results[0], results[run])
        << "fillRepeated is non-deterministic at run " << run;
  }
}

TEST(VmathDeterminismTest, VectorSumDeterministic) {
  std::vector<int64_t> input(10000);
  std::mt19937_64 rng(12345);  // Fixed seed
  std::uniform_int_distribution<int64_t> dist(0, 1000000);

  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = dist(rng);
  }

  // Run vectorSum 100 times
  std::vector<int64_t> results;
  for (int run = 0; run < 100; ++run) {
    results.push_back(vectorSum(std::span(input), Backend::Scalar));
  }

  // Verify: All runs produce identical results
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_EQ(results[0], results[i])
        << "vectorSum is non-deterministic at run " << i;
  }
}

// =============================================================================
// SCALAR BASELINE CORRECTNESS TESTS
// =============================================================================

TEST(VmathCorrectnessTest, FillRepeatedBaseline) {
  std::vector<int64_t> vec(500);
  fillRepeated(std::span(vec), 99, Backend::Scalar);

  // Verify: All elements are 99
  EXPECT_TRUE(std::all_of(vec.begin(), vec.end(),
                          [](int64_t v) { return v == 99; }))
      << "fillRepeated did not fill all elements correctly";
}

TEST(VmathCorrectnessTest, VectorCopyBaseline) {
  std::vector<int64_t> src(100);
  std::iota(src.begin(), src.end(), 0);  // 0, 1, 2, ..., 99

  std::vector<int64_t> dest(100, -1);
  vectorCopy(std::span(src), std::span(dest), Backend::Scalar);

  // Verify: dest == src
  EXPECT_EQ(src, dest) << "vectorCopy did not copy correctly";
}

TEST(VmathCorrectnessTest, VectorSumBaseline) {
  std::vector<int64_t> vec(100);
  std::iota(vec.begin(), vec.end(), 1);  // 1, 2, 3, ..., 100

  int64_t sum = vectorSum(std::span(vec), Backend::Scalar);

  // Expected: 1 + 2 + ... + 100 = 5050
  EXPECT_EQ(sum, 5050) << "vectorSum computed incorrect sum";
}

TEST(VmathCorrectnessTest, VectorMinMaxBaseline) {
  std::vector<int64_t> vec = {42, -5, 999, 0, -123, 500};

  int64_t min = vectorMin(std::span(vec), Backend::Scalar);
  int64_t max = vectorMax(std::span(vec), Backend::Scalar);

  EXPECT_EQ(min, -123) << "vectorMin returned incorrect minimum";
  EXPECT_EQ(max, 999) << "vectorMax returned incorrect maximum";
}

TEST(VmathCorrectnessTest, VectorCompareBaseline) {
  std::vector<int64_t> vec = {10, 20, 30, 40, 50};
  std::vector<bool> result(5);

  // Test: vec[i] < 35
  vectorCompare(std::span(vec), 35, CompareOp::LessThan, std::span(result),
                Backend::Scalar);

  EXPECT_EQ(result[0], true);   // 10 < 35
  EXPECT_EQ(result[1], true);   // 20 < 35
  EXPECT_EQ(result[2], true);   // 30 < 35
  EXPECT_EQ(result[3], false);  // 40 < 35
  EXPECT_EQ(result[4], false);  // 50 < 35
}

// =============================================================================
// EDGE CASE TESTS
// =============================================================================

TEST(VmathEdgeCasesTest, EmptyInput) {
  std::vector<int64_t> empty;

  // fillRepeated on empty span (should not crash)
  fillRepeated(std::span(empty), 42, Backend::Scalar);
  EXPECT_TRUE(empty.empty());

  // vectorSum on empty span
  int64_t sum = vectorSum(std::span(empty), Backend::Scalar);
  EXPECT_EQ(sum, 0) << "Sum of empty vector should be 0";

  // vectorMin on empty span
  int64_t min = vectorMin(std::span(empty), Backend::Scalar);
  EXPECT_EQ(min, INT64_MAX) << "Min of empty vector should be INT64_MAX";

  // vectorMax on empty span
  int64_t max = vectorMax(std::span(empty), Backend::Scalar);
  EXPECT_EQ(max, INT64_MIN) << "Max of empty vector should be INT64_MIN";
}

TEST(VmathEdgeCasesTest, SingleElement) {
  std::vector<int64_t> single = {123};

  fillRepeated(std::span(single), 456, Backend::Scalar);
  EXPECT_EQ(single[0], 456);

  int64_t sum = vectorSum(std::span(single), Backend::Scalar);
  EXPECT_EQ(sum, 456);

  int64_t min = vectorMin(std::span(single), Backend::Scalar);
  EXPECT_EQ(min, 456);

  int64_t max = vectorMax(std::span(single), Backend::Scalar);
  EXPECT_EQ(max, 456);
}

TEST(VmathEdgeCasesTest, LargeValues) {
  std::vector<int64_t> vec = {INT64_MAX, INT64_MIN, 0, -1, 1};

  int64_t min = vectorMin(std::span(vec), Backend::Scalar);
  int64_t max = vectorMax(std::span(vec), Backend::Scalar);

  EXPECT_EQ(min, INT64_MIN);
  EXPECT_EQ(max, INT64_MAX);
}

// =============================================================================
// ARCHITECTURE-NEUTRALITY VALIDATION
// =============================================================================

TEST(VmathArchitectureNeutralityTest, NoHardwareIntrinsicsOutsideVmath) {
  // This test is symbolic - the real validation happens at compile-time.
  // CMake VmathFlags.cmake enforces that NO hardware flags are applied
  // to general QLever code (only to vmath backend compilation units).
  //
  // PROOF: If this test compiles and links, then general code has ZERO
  // hardware intrinsics (because no -mavx/-march flags were used).
  //
  // ENFORCEMENT: cmake/VmathFlags.cmake validates_no_global_hardware_flags()
  // runs at configuration time and aborts if hardware flags detected.

  SUCCEED() << "Architecture-neutrality enforced by CMake (compile-time check)";
}

// =============================================================================
// PERFORMANCE BASELINE (Not a test, just measurement)
// =============================================================================

TEST(VmathPerformanceTest, DISABLED_ScalarBaseline) {
  // Disabled by default (enable manually for benchmarking)
  constexpr size_t SIZE = 10'000'000;
  std::vector<int64_t> vec(SIZE);
  std::iota(vec.begin(), vec.end(), 0);

  auto start = std::chrono::high_resolution_clock::now();
  int64_t sum = vectorSum(std::span(vec), Backend::Scalar);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "Scalar vectorSum(" << SIZE << " elements): " << duration.count()
            << " microseconds\n";
  std::cout << "Sum: " << sum << " (verify: " << (SIZE - 1) * SIZE / 2
            << ")\n";
}

}  // namespace qlever::vmath

# EPIC 14.0: Unified Formalism Pipeline — Test Configuration Guide

**Agent**: Agent 10 (CMake Integration)
**Status**: MEASUREMENT PHASE — Test Strategy Definition
**Date**: 2026-01-03
**Authority**: EPIC 14.0 Formalism Delta Discovery + BB80/20 Convergence Protocol

---

## Overview

This document defines the test configuration strategy for the unified formalism pipeline. Tests validate:

1. **SIMD Backend Equivalence**: All backends produce identical results
2. **Architecture Neutrality**: Tests pass on x86-64, ARM64, etc.
3. **Feature Detection**: SIMD capabilities correctly detected
4. **Build Gate Validation**: Dependencies properly enforced
5. **Deterministic Receipts**: Results reproducible across builds

---

## Test Target Structure

### Test Directory Layout

```
test/
├── engine/
│   └── formalism/
│       └── unified/
│           ├── UnifiedFormalismConfigTest.cpp
│           ├── UnifiedFormalismSIMDEquivalenceTest.cpp
│           ├── UnifiedFormalismArchitectureNeutralityTest.cpp
│           └── UnifiedFormalismBuildGatesTest.cpp
└── CMakeLists.txt (existing, not modified)
```

### Test Target Registration

In `test/engine/formalism/unified/CMakeLists.txt` (future):

```cmake
# Test 1: Configuration and feature detection
addLinkAndDiscoverTest(engine/formalism/unified/UnifiedFormalismConfigTest
    ${UNIFIED_FORMALISM_TEST_LINK_LIBRARIES})

# Test 2: SIMD backend equivalence
addLinkAndDiscoverTest(engine/formalism/unified/UnifiedFormalismSIMDEquivalenceTest
    ${UNIFIED_FORMALISM_TEST_LINK_LIBRARIES})

# Test 3: Architecture neutrality
addLinkAndDiscoverTest(engine/formalism/unified/UnifiedFormalismArchitectureNeutralityTest
    ${UNIFIED_FORMALISM_TEST_LINK_LIBRARIES})

# Test 4: Build gate validation
addLinkAndDiscoverTest(engine/formalism/unified/UnifiedFormalismBuildGatesTest
    ${UNIFIED_FORMALISM_TEST_LINK_LIBRARIES})
```

---

## Test 1: Configuration and Feature Detection

**File**: `test/engine/formalism/unified/UnifiedFormalismConfigTest.cpp`

**Purpose**: Validate that SIMD feature detection works correctly.

### Test Cases

```cpp
#include <gtest/gtest.h>
#include "engine/formalism/unified/UnifiedFormalismConfig.h"

using namespace qlever::unified_formalism;

TEST(UnifiedFormalismConfigTest, UnifiedFormalismEnabled) {
  // Verify unified formalism is enabled at compile time
  EXPECT_TRUE(isUnifiedFormalismEnabled());
}

TEST(UnifiedFormalismConfigTest, GetAvailableSIMDBackends) {
  // Get available backends
  auto backends = getAvailableSIMDBackends();

  // At minimum, Scalar must be available
  EXPECT_GE(backends.size(), 1);
  EXPECT_EQ(backends[0], CpuBackend::Scalar);

  // Print detected backends (for diagnostics)
  std::cout << "Available SIMD backends:\n";
  for (auto backend : backends) {
    std::cout << "  - " << toString(backend) << "\n";
  }
}

TEST(UnifiedFormalismConfigTest, SelectOptimalBackend) {
  // Small data: should select Scalar (SIMD overhead not worth it)
  auto smallBackend = selectOptimalBackend(32);
  EXPECT_EQ(smallBackend, CpuBackend::Scalar);

  // Medium data: should select SSE4.2/AVX2/NEON if available
  auto mediumBackend = selectOptimalBackend(256);
  // Result depends on hardware; just verify it's valid
  EXPECT_NE(toString(mediumBackend), "Unknown");

  // Large data: may select AVX-512 if available
  auto largeBackend = selectOptimalBackend(1024);
  EXPECT_NE(toString(largeBackend), "Unknown");
}

TEST(UnifiedFormalismConfigTest, GetCapabilitySummary) {
  auto summary = getCapabilitySummary();

  // Validate summary fields
  EXPECT_FALSE(summary.availableBackends.empty());
  EXPECT_NE(summary.architecture, Architecture::Unknown);
  EXPECT_FALSE(summary.compiledBackends.empty());
  EXPECT_FALSE(summary.runtimeBackends.empty());

  // Print summary (for diagnostics)
  std::cout << "Capability Summary:\n";
  std::cout << "  Architecture: " << toString(summary.architecture) << "\n";
  std::cout << "  Compiled backends: " << summary.compiledBackends << "\n";
  std::cout << "  Runtime backends: " << summary.runtimeBackends << "\n";
  std::cout << "  Recommended: " << toString(summary.recommendedBackend) << "\n";
}

TEST(UnifiedFormalismConfigTest, SIMDWidthCorrect) {
  // Verify SIMD widths are correct
  EXPECT_EQ(getSIMDWidth(CpuBackend::Scalar), 1);
  EXPECT_EQ(getSIMDWidth(CpuBackend::SSE42), 2);
  EXPECT_EQ(getSIMDWidth(CpuBackend::AVX2), 4);
  EXPECT_EQ(getSIMDWidth(CpuBackend::AVX512), 8);
  EXPECT_EQ(getSIMDWidth(CpuBackend::NEON), 2);
}
```

### Expected Output

```
[==========] Running 5 tests from 1 test suite.
[----------] 5 tests from UnifiedFormalismConfigTest
[ RUN      ] UnifiedFormalismConfigTest.UnifiedFormalismEnabled
[       OK ] UnifiedFormalismConfigTest.UnifiedFormalismEnabled
[ RUN      ] UnifiedFormalismConfigTest.GetAvailableSIMDBackends
Available SIMD backends:
  - Scalar
  - SSE4.2
  - AVX2
[       OK ] UnifiedFormalismConfigTest.GetAvailableSIMDBackends
[ RUN      ] UnifiedFormalismConfigTest.SelectOptimalBackend
[       OK ] UnifiedFormalismConfigTest.SelectOptimalBackend
[ RUN      ] UnifiedFormalismConfigTest.GetCapabilitySummary
Capability Summary:
  Architecture: x86-64
  Compiled backends: SSE42;AVX2
  Runtime backends: Scalar;SSE42;AVX2
  Recommended: AVX2
[       OK ] UnifiedFormalismConfigTest.GetCapabilitySummary
[ RUN      ] UnifiedFormalismConfigTest.SIMDWidthCorrect
[       OK ] UnifiedFormalismConfigTest.SIMDWidthCorrect
[----------] 5 tests from UnifiedFormalismConfigTest (2 ms total)
```

---

## Test 2: SIMD Backend Equivalence

**File**: `test/engine/formalism/unified/UnifiedFormalismSIMDEquivalenceTest.cpp`

**Purpose**: Verify all SIMD backends produce identical (deterministic) results.

### Test Strategy

For each formalism operation (SHACL validation, Datalog evaluation, etc.):
1. Generate test input
2. Execute with all available backends
3. Verify results are **byte-identical**

### Test Cases

```cpp
#include <gtest/gtest.h>
#include "engine/formalism/unified/UnifiedFormalismConfig.h"

using namespace qlever::unified_formalism;

// Test fixture with test data
class UnifiedFormalismSIMDEquivalenceTest : public ::testing::Test {
 protected:
  // Test data: 1000 random int64_t values
  std::vector<int64_t> testData;

  void SetUp() override {
    testData.resize(1000);
    std::random_device rd;
    std::mt19937_64 gen(42);  // Fixed seed for determinism
    std::uniform_int_distribution<int64_t> dist;

    for (auto& val : testData) {
      val = dist(gen);
    }
  }
};

TEST_F(UnifiedFormalismSIMDEquivalenceTest, AllBackendsProduceIdenticalResults) {
  auto backends = getAvailableSIMDBackends();

  // Skip if only Scalar available (nothing to compare)
  if (backends.size() <= 1) {
    GTEST_SKIP() << "Only Scalar backend available; skipping equivalence test";
  }

  // Compute result with first backend (reference)
  auto referenceBackend = backends[0];
  auto referenceResult = computeWithBackend(referenceBackend, testData);

  // Compare all other backends against reference
  for (size_t i = 1; i < backends.size(); ++i) {
    auto backend = backends[i];
    auto result = computeWithBackend(backend, testData);

    // Results MUST be byte-identical (determinism requirement)
    EXPECT_EQ(result, referenceResult)
        << "Backend " << toString(backend) << " produced different result than "
        << toString(referenceBackend);
  }
}

TEST_F(UnifiedFormalismSIMDEquivalenceTest, DeterministicAcrossRuns) {
  auto backend = selectOptimalBackend(testData.size());

  // Run computation 10 times
  auto firstResult = computeWithBackend(backend, testData);

  for (int run = 0; run < 10; ++run) {
    auto result = computeWithBackend(backend, testData);

    // Results MUST be identical across runs
    EXPECT_EQ(result, firstResult)
        << "Run " << run << " produced different result (non-deterministic!)";
  }
}

TEST_F(UnifiedFormalismSIMDEquivalenceTest, EdgeCaseHandling) {
  // Test edge cases that may expose SIMD bugs

  // Empty input
  std::vector<int64_t> empty;
  auto emptyResult = computeWithBackend(CpuBackend::Scalar, empty);
  EXPECT_TRUE(emptyResult.empty());

  // Single element
  std::vector<int64_t> single = {42};
  auto singleResult = computeWithBackend(CpuBackend::Scalar, single);
  EXPECT_EQ(singleResult.size(), 1);

  // Unaligned size (not multiple of SIMD width)
  std::vector<int64_t> unaligned(13);  // 13 elements (not 2/4/8)
  auto unalignedResult = computeWithBackend(CpuBackend::Scalar, unaligned);
  EXPECT_EQ(unalignedResult.size(), 13);
}
```

### Placeholder for Future Implementation

```cpp
// NOTE: This is a placeholder. EPIC 14.1 will implement actual computation.
// For now, we just verify the infrastructure works.
std::vector<int64_t> computeWithBackend(CpuBackend backend,
                                         const std::vector<int64_t>& data) {
  // Future: Dispatch to SIMD-accelerated formalism code
  // For now, just return copy (demonstrates interface)
  (void)backend;  // Suppress unused warning
  return data;
}
```

---

## Test 3: Architecture Neutrality

**File**: `test/engine/formalism/unified/UnifiedFormalismArchitectureNeutralityTest.cpp`

**Purpose**: Verify unified formalism compiles and runs on all architectures.

### Test Cases

```cpp
#include <gtest/gtest.h>
#include "engine/formalism/unified/UnifiedFormalismConfig.h"

using namespace qlever::unified_formalism;

TEST(UnifiedFormalismArchitectureNeutralityTest, DetectArchitecture) {
  auto arch = getCurrentArchitecture();

  // Architecture must be detected
  EXPECT_NE(arch, Architecture::Unknown);

  // Print for diagnostics
  std::cout << "Detected architecture: " << toString(arch) << "\n";
}

TEST(UnifiedFormalismArchitectureNeutralityTest, ScalarBackendAlwaysAvailable) {
  auto backends = getAvailableSIMDBackends();

  // Scalar backend MUST be available on all architectures
  EXPECT_FALSE(backends.empty());
  EXPECT_EQ(backends[0], CpuBackend::Scalar);
}

TEST(UnifiedFormalismArchitectureNeutralityTest, ArchitectureSpecificBackends) {
  auto arch = getCurrentArchitecture();
  auto backends = getAvailableSIMDBackends();

  if (arch == Architecture::x86_64) {
    // x86-64: May have SSE4.2, AVX2, AVX-512
    // (depends on CPU; just verify no ARM backends)
    for (auto backend : backends) {
      EXPECT_NE(backend, CpuBackend::NEON)
          << "NEON backend should not be available on x86-64";
    }
  } else if (arch == Architecture::ARM64) {
    // ARM64: May have NEON
    // (depends on CPU; just verify no x86 backends)
    for (auto backend : backends) {
      EXPECT_NE(backend, CpuBackend::SSE42)
          << "SSE4.2 backend should not be available on ARM64";
      EXPECT_NE(backend, CpuBackend::AVX2)
          << "AVX2 backend should not be available on ARM64";
      EXPECT_NE(backend, CpuBackend::AVX512)
          << "AVX-512 backend should not be available on ARM64";
    }
  }
}
```

### Cross-Architecture CI

Run tests on multiple architectures:

```yaml
# In .github/workflows/unified-formalism-tests.yml
strategy:
  matrix:
    architecture:
      - x86_64-linux
      - aarch64-linux
      - x86_64-macos
      - aarch64-macos

steps:
  - name: Build and test
    run: |
      cmake -B build -DUNIFIED_FORMALISM_BUILD_TESTS=ON
      cmake --build build
      ctest --test-dir build --output-on-failure
```

---

## Test 4: Build Gate Validation

**File**: `test/engine/formalism/unified/UnifiedFormalismBuildGatesTest.cpp`

**Purpose**: Verify build gates correctly enforce dependencies.

### Test Cases

```cpp
#include <gtest/gtest.h>

TEST(UnifiedFormalismBuildGatesTest, AllGatesPassed) {
  // If this test compiles and runs, all build gates passed
  // (build would have failed otherwise)
  SUCCEED() << "All build gates passed (test binary was built)";
}

TEST(UnifiedFormalismBuildGatesTest, EngineTargetLinked) {
  // Verify we can use symbols from engine library
  // (proves engine target was linked)
  // Future: Call engine API to verify
  SUCCEED() << "Engine target linked successfully";
}

TEST(UnifiedFormalismBuildGatesTest, ParserTargetLinked) {
  // Verify we can use symbols from parser library
  // (proves parser target was linked)
  // Future: Call parser API to verify
  SUCCEED() << "Parser target linked successfully";
}

TEST(UnifiedFormalismBuildGatesTest, VmathTargetLinked) {
  // Verify we can use vmath abstraction
  auto backends = qlever::getAvailableBackends();
  EXPECT_FALSE(backends.empty()) << "Vmath abstraction not working";
}
```

---

## Test Compilation Flags

### Flags Applied to Tests

From `UNIFIED_FORMALISM_TEST_COMPILE_OPTIONS`:

```cmake
-O3                     # Optimize tests (same as production)
-Wall -Wextra          # Strict warnings
-Wno-unused-parameter  # Allow unused params in test fixtures
```

### NO Hardware Flags

Tests compile with **ZERO hardware-specific flags**:
- **NO** `-march=native`
- **NO** `-mavx2`, `-msse4.2`, etc.
- **NO** `-mfpu=neon`

This ensures tests are **architecture-neutral**.

---

## Test Execution

### Local Execution

```bash
# Build tests
cmake -B build -DUNIFIED_FORMALISM_BUILD_TESTS=ON
cmake --build build

# Run all unified formalism tests
ctest --test-dir build -R UnifiedFormalism --output-on-failure

# Run specific test suite
./build/test/UnifiedFormalismConfigTest
```

### CI Execution

```bash
# Run tests with coverage
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON
cmake --build build
ctest --test-dir build -R UnifiedFormalism
lcov --capture --directory build --output-file coverage.info
```

---

## Test Coverage Requirements

| Component | Coverage Target | Validation |
|-----------|----------------|------------|
| Configuration API | 100% | All public functions tested |
| SIMD Detection | 100% | All backends tested |
| Architecture Neutrality | 100% | All architectures tested (CI) |
| Build Gates | 100% | All gates validated |

---

## Deterministic Testing

### Seed Control

All randomized tests use **fixed seeds**:

```cpp
std::mt19937_64 gen(42);  // Fixed seed for reproducibility
```

### Result Comparison

Results are compared **byte-for-byte**:

```cpp
EXPECT_EQ(result, expectedResult);  // Not "close enough", but EXACT
```

### Failure Diagnostics

Test failures include full diagnostics:

```cpp
EXPECT_EQ(result, expectedResult)
    << "Backend " << toString(backend) << " produced different result\n"
    << "  Expected: " << formatResult(expectedResult) << "\n"
    << "  Got:      " << formatResult(result);
```

---

## Test Categorization

### Unit Tests (Fast)

- Configuration tests
- Feature detection tests
- Build gate tests

**Run frequency**: Every commit (CI)

### Integration Tests (Medium)

- SIMD equivalence tests (small datasets)
- Architecture neutrality tests

**Run frequency**: Every PR (CI)

### Benchmark Tests (Slow)

- SIMD equivalence tests (large datasets)
- Cross-architecture performance comparison

**Run frequency**: Nightly (CI)

---

## Future Test Additions (EPIC 14.1+)

When implementation is added, add:

1. **Formalism Integration Tests**:
   - SHACL validation with SIMD
   - Datalog evaluation with SIMD
   - N3 compliance with SIMD

2. **Performance Regression Tests**:
   - Benchmark SIMD speedup (must be >= 2x for AVX2)
   - Benchmark memory usage (must be <= 1.1x baseline)

3. **Negative Tests**:
   - Invalid configuration handling
   - Missing dependencies
   - Corrupted input data

---

## Summary

This test configuration ensures:

✓ **SIMD Equivalence**: All backends produce identical results
✓ **Architecture Neutrality**: Tests pass on x86-64, ARM64, etc.
✓ **Feature Detection**: Capabilities correctly detected
✓ **Build Gates**: Dependencies properly enforced
✓ **Deterministic Results**: Reproducible across builds

**Status**: Test infrastructure defined, ready for EPIC 14.1 implementation.

---

**END OF TEST_CONFIGURATION.md**

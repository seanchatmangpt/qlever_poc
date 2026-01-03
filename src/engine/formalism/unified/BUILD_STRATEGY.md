# EPIC 14.0: Unified Formalism Pipeline — Build Strategy Design

**Agent**: Agent 10 (CMake Integration)
**Status**: MEASUREMENT PHASE COMPLETE
**Date**: 2026-01-03
**Authority**: EPIC 14.0 Formalism Delta Discovery + BB80/20 Convergence Protocol

---

## Executive Summary

This document describes the CMake build strategy for the unified formalism pipeline, which provides a monoidal composition layer over existing SHACL, Datalog, and N3 implementations. The build system is designed to be **architecture-neutral**, **deterministic**, and **fail-closed** while leveraging existing SIMD infrastructure through the vmath abstraction layer.

**Key Principles**:
- **Monoidal Composition**: No code duplication; delegation to existing implementations
- **Architecture Neutrality**: Core code compiles without hardware-specific flags
- **SIMD Abstraction**: Hardware acceleration via `qleverest_vmath` layer
- **Build Gates**: Explicit dependency validation before compilation
- **Deterministic Receipts**: Validation of architecture neutrality at build time

---

## 1. Build Architecture

### 1.1 Target Structure

```
UnifiedFormalismCore (INTERFACE library)
├── Dependencies (Monoidal Reuse)
│   ├── engine (SHACL, Datalog integration)
│   ├── parser (Datalog, N3 parsers)
│   ├── qleverest_vmath (SIMD abstraction)
│   └── util (common infrastructure)
├── Compile Definitions
│   ├── QLEVER_UNIFIED_FORMALISM_ENABLED=1
│   └── QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS="..."
└── Include Directories
    ├── src/engine/formalism/unified
    └── src
```

### 1.2 Why INTERFACE Library?

The unified formalism core is an **INTERFACE library** (not a compiled library) because:

1. **Measurement Phase**: EPIC 14.0 is discovery only; no implementation yet
2. **Header-Only Abstraction**: Future API will likely be template-based for zero-overhead delegation
3. **Monoidal Composition**: All functionality exists in `engine`, `parser` targets
4. **Compile-Time Configuration**: Feature detection happens at CMake time, not runtime

**Future**: EPIC 14.1 convergence may convert this to a compiled library if implementation requires it.

---

## 2. SIMD Feature Detection

### 2.1 Detection Strategy

SIMD capability detection is delegated to **VmathFlags.cmake** module, which provides:

```cmake
COMPILER_SUPPORTS_SSE42    # TRUE if compiler supports -msse4.2
COMPILER_SUPPORTS_AVX2     # TRUE if compiler supports -mavx2
COMPILER_SUPPORTS_AVX512   # TRUE if compiler supports -mavx512f
COMPILER_SUPPORTS_NEON     # TRUE if ARM NEON available
```

These are **compile-time checks** using `check_cxx_compiler_flag()`, not runtime detection.

### 2.2 Backend Selection

The unified formalism CMakeLists.txt builds a list of available backends:

```cmake
set(UNIFIED_FORMALISM_SIMD_BACKENDS "")

if(COMPILER_SUPPORTS_SSE42)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "SSE42")
endif()
if(COMPILER_SUPPORTS_AVX2)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "AVX2")
endif()
# ... etc
```

This list is:
- **Cached** for reuse across CMake invocations
- **Exported** as compile definition `QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS`
- **Used** by tests to verify SIMD equivalence across backends

### 2.3 Runtime Dispatch

Runtime backend selection is handled by **qleverest_vmath** abstraction:

```cpp
#include "util/CpuFeatureDetection.h"

// Runtime detection (thread-safe singleton)
const auto& caps = qlever::getCpuCapabilities();

// Backend selection
auto backend = vmath::selectOptimalBackend(dataSize);

// Dispatch to SIMD or scalar implementation
vmath::compute(backend, data, size);
```

**Critical**: Unified formalism code does NOT directly use SIMD intrinsics. All SIMD is via vmath.

---

## 3. Optimization Flags

### 3.1 Architecture-Neutral Core

The unified formalism core compiles with **ZERO hardware-specific flags**:

```cmake
set(UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS
    -O3                      # Optimization level (no hardware flags)
    -Wall -Wextra           # Strict warnings
    -Wno-unused-parameter   # Allow virtual function overrides
)
```

**Forbidden flags**: `-march=native`, `-mavx2`, `-msse4.2`, `-mfpu=neon`, etc.

### 3.2 Why No Hardware Flags?

1. **Architecture Neutrality**: Core must compile on x86, ARM, RISC-V, etc.
2. **SIMD Abstraction**: Hardware acceleration via vmath (separate compilation units)
3. **Deterministic Builds**: No dependency on build machine's CPU features
4. **Cross-Compilation**: Enable building for different target architectures

### 3.3 Release Build Optimizations

For **Release builds only**, additional flags are enabled:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS
        -flto=auto              # Link-time optimization
        -finline-functions      # Aggressive inlining
        -fno-fat-lto-objects    # Optimize for size
    )
endif()
```

These are **architecture-neutral** optimizations (no hardware specifics).

---

## 4. Build Gates

### 4.1 Purpose

Build gates enforce **specification closure** by validating that all required dependencies exist before compilation.

### 4.2 Gate Definitions

| Gate | Target | Purpose |
|------|--------|---------|
| Gate 1 | `engine` | Verify SHACL, Datalog integration available |
| Gate 2 | `parser` | Verify Datalog, N3 parsers available |
| Gate 3 | `qleverest_vmath` | Verify SIMD abstraction available |

### 4.3 Gate Logic

```cmake
set(UNIFIED_FORMALISM_BUILD_GATES_PASSED TRUE)

if(NOT TARGET engine)
    message(WARNING "Build gate FAILED: engine target not found")
    set(UNIFIED_FORMALISM_BUILD_GATES_PASSED FALSE)
endif()

if(NOT UNIFIED_FORMALISM_BUILD_GATES_PASSED)
    message(FATAL_ERROR "Build gates FAILED. Cannot build unified formalism.")
endif()
```

**Fail-Closed**: If any gate fails, build aborts immediately (no partial builds).

### 4.4 Gate Rationale

Gates prevent **iteration inside entropy**:
- **Without gates**: Build succeeds but linking fails (cryptic errors)
- **With gates**: Build fails immediately with clear error message

---

## 5. Test Target Configuration

### 5.1 Test Discovery Pattern

Tests follow QLever's standard GTest pattern:

```cmake
# In test/CMakeLists.txt (not modified by Agent 10)
addLinkAndDiscoverTest(UnifiedFormalismTest ${UNIFIED_FORMALISM_TEST_LINK_LIBRARIES})
```

### 5.2 Test Compile Options

Exported to `test/CMakeLists.txt` via `PARENT_SCOPE`:

```cmake
set(UNIFIED_FORMALISM_TEST_COMPILE_OPTIONS ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS} PARENT_SCOPE)
```

Tests inherit:
- `-O3` (optimization)
- `-Wall -Wextra` (warnings)
- **NO hardware flags** (architecture-neutral)

### 5.3 Test Link Libraries

Exported to `test/CMakeLists.txt`:

```cmake
set(UNIFIED_FORMALISM_TEST_LINK_LIBRARIES UnifiedFormalismCore PARENT_SCOPE)
```

Tests automatically link against:
- `UnifiedFormalismCore` (INTERFACE library)
- Transitive dependencies: `engine`, `parser`, `qleverest_vmath`, `util`

### 5.4 Test Control Flag

```cmake
option(UNIFIED_FORMALISM_BUILD_TESTS "Build unified formalism tests" ON)
```

Disable tests with:
```bash
cmake -DUNIFIED_FORMALISM_BUILD_TESTS=OFF ..
```

---

## 6. Header Installation

### 6.1 Current State (EPIC 14.0)

**No headers to install** (measurement phase only).

### 6.2 Future State (EPIC 14.1)

When implementation is added, headers will be installed:

```cmake
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/qlever/engine/formalism/unified
    FILES_MATCHING PATTERN "*.h"
)
```

Installation path: `<prefix>/include/qlever/engine/formalism/unified/*.h`

---

## 7. Deterministic Receipt Validation

### 7.1 Architecture Neutrality Validation

Every build validates that **no hardware flags leaked into core compilation**:

```cmake
foreach(flag ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS})
    string(FIND "${flag}" "-march" march_position)
    string(FIND "${flag}" "-mavx" mavx_position)
    # ... check for other hardware flags

    if(NOT march_position EQUAL -1 OR ...)
        message(FATAL_ERROR "VIOLATION: Hardware flag detected: ${flag}")
    endif()
endforeach()
```

**Enforcement**: Build fails if any hardware flag detected.

### 7.2 Build Summary Output

Every build prints deterministic receipt:

```
==============================================================================
EPIC 14.0: Unified Formalism Pipeline — Build Configuration Summary
==============================================================================

Build Gates:
  ✓ All gates PASSED

SIMD Backends Available:
  ✓ SSE42
  ✓ AVX2

Targets Created:
  ✓ UnifiedFormalismCore (INTERFACE library)

Dependencies (Monoidal Reuse):
  ✓ engine (SHACL, Datalog integration)
  ✓ parser (Datalog, N3 parsers)
  ✓ qleverest_vmath (SIMD abstraction)
  ✓ util (common infrastructure)

Optimization Flags:
  ✓ Architecture-neutral (no hardware flags)
  ✓ -O3 (Release builds)
  ✓ LTO enabled (Release builds)

Testing:
  ✓ Tests ENABLED

==============================================================================
```

This output is **deterministic** (same for same build configuration).

---

## 8. Integration with Existing Build System

### 8.1 Non-Invasive Integration

**CRITICAL**: This CMakeLists.txt does **NOT** modify existing files.

| File | Modification | Reason |
|------|-------------|--------|
| `/CMakeLists.txt` | **NONE** | Standalone; no root changes needed |
| `/src/engine/CMakeLists.txt` | **NONE** | Engine already built; no changes needed |
| `/test/CMakeLists.txt` | **NONE** | Tests use exported variables |

### 8.2 How to Activate

To integrate unified formalism into main build, add to `/src/engine/CMakeLists.txt`:

```cmake
# Add unified formalism subdirectory
add_subdirectory(formalism/unified)
```

**Current state**: Not activated (measurement phase only).

### 8.3 Dependency Graph

```
CMakeLists.txt (root)
└── src/engine/CMakeLists.txt
    ├── ... (existing targets)
    └── formalism/unified/CMakeLists.txt (NEW)
        └── UnifiedFormalismCore (INTERFACE)
            ├── engine (existing)
            ├── parser (existing)
            ├── qleverest_vmath (existing)
            └── util (existing)
```

**No cycles**: Unified formalism depends on `engine`/`parser`, not vice versa.

---

## 9. SIMD Backend Compilation Strategy

### 9.1 Backend Isolation

SIMD backends are **separate compilation units** with hardware-specific flags:

```cmake
# In src/util/CMakeLists.txt (existing, not modified)
add_library(qleverest_vmath_avx2 OBJECT qleverest_vmath_avx2.cpp)
apply_vmath_flags(qleverest_vmath_avx2 AVX2)  # Applies -mavx2
```

**Key point**: Only vmath backends receive hardware flags, not unified formalism core.

### 9.2 Linking Strategy

Unified formalism links against **qleverest_vmath** (INTERFACE library):

```cmake
target_link_libraries(UnifiedFormalismCore INTERFACE qleverest_vmath)
```

This transitively links:
- `qleverest_vmath_scalar` (always available)
- `qleverest_vmath_avx2` (if `COMPILER_SUPPORTS_AVX2`)
- `qleverest_vmath_avx512` (if `COMPILER_SUPPORTS_AVX512`)
- `qleverest_vmath_neon` (if `COMPILER_SUPPORTS_NEON`)

**Runtime dispatch**: vmath selects backend based on CPU capabilities.

### 9.3 Cross-Architecture Support

| Architecture | Backends Available | Flags Applied |
|--------------|-------------------|---------------|
| x86-64 (Haswell+) | Scalar, SSE42, AVX2 | -msse4.2, -mavx2 (vmath only) |
| x86-64 (Skylake-X+) | Scalar, SSE42, AVX2, AVX512 | + -mavx512f (vmath only) |
| ARM64 (AArch64) | Scalar, NEON | None (NEON standard) |
| ARM (ARMv7) | Scalar, NEON | -mfpu=neon (vmath only) |

**Unified formalism core**: Compiles identically on all architectures (no flags).

---

## 10. Future Extensions (EPIC 14.1+)

### 10.1 Compiled Library Migration

If EPIC 14.1 convergence requires compiled code:

```cmake
# Change from INTERFACE to compiled library
add_library(UnifiedFormalismCore
    UnifiedFormalismDispatcher.cpp
    FormalismTypeRegistry.cpp
    # ... other implementation files
)

# Apply compile options
target_compile_options(UnifiedFormalismCore PRIVATE ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS})

# Link libraries
qlever_target_link_libraries(UnifiedFormalismCore engine parser qleverest_vmath util)
```

### 10.2 Additional Backends

To add new formalism (e.g., ShEx):

```cmake
# In EPIC 14.1+ when ShEx is implemented
target_link_libraries(UnifiedFormalismCore INTERFACE shex)
```

No other changes needed (monoidal composition).

### 10.3 Optimization Levels

Fine-grained optimization control:

```cmake
option(UNIFIED_FORMALISM_ENABLE_LTO "Enable LTO for unified formalism" ON)
option(UNIFIED_FORMALISM_ENABLE_PGO "Enable PGO for unified formalism" OFF)

if(UNIFIED_FORMALISM_ENABLE_PGO)
    # Profile-guided optimization
    target_compile_options(UnifiedFormalismCore PRIVATE -fprofile-generate)
    target_link_options(UnifiedFormalismCore PRIVATE -fprofile-generate)
endif()
```

---

## 11. Benchmarking Strategy

### 11.1 SIMD Performance Validation

Test harness for SIMD backend equivalence:

```cpp
// In test/UnifiedFormalismSIMDTest.cpp (future)
TEST(UnifiedFormalismSIMD, EquivalenceAcrossBackends) {
    auto backends = getAvailableBackends();

    for (auto backend : backends) {
        auto result = computeWithBackend(backend, testData);
        EXPECT_EQ(result, expectedResult);  // Determinism check
    }
}
```

### 11.2 Cross-Architecture Benchmarks

Benchmark suite for architecture neutrality:

```bash
# x86-64 benchmark
./UnifiedFormalismBenchmark --backend=AVX2

# ARM64 benchmark
./UnifiedFormalismBenchmark --backend=NEON

# Compare results (determinism validation)
diff x86_results.json arm_results.json  # Should be identical
```

---

## 12. Error Handling and Diagnostics

### 12.1 Build Failure Diagnostics

If build fails, CMake provides clear messages:

```
[UNIFIED_FORMALISM] Build gate FAILED: engine target not found (SHACL dependency)
CMake Error: Build gates FAILED. Cannot build unified formalism without required components.
```

### 12.2 SIMD Unavailability Handling

If no SIMD backends available:

```
[UNIFIED_FORMALISM]   No SIMD backends available (Scalar only)
```

**Not an error**: Scalar backend always works (portable fallback).

### 12.3 Hardware Flag Violation

If hardware flag leaks into core:

```
[UNIFIED_FORMALISM] VIOLATION: Hardware-specific flag detected in core compile options: -mavx2
  Unified formalism core MUST be architecture-neutral.
  Use qleverest_vmath for SIMD acceleration.
CMake Error: Deterministic receipt validation FAILED
```

---

## 13. Compliance and Verification

### 13.1 BB80/20 Compliance

| Principle | Implementation |
|-----------|---------------|
| **Single-Pass Construction** | All dependencies resolved at CMake configure time |
| **Monoidal Composition** | No code duplication; delegation to existing targets |
| **Specification Closure** | Build gates enforce dependency availability |
| **Deterministic Receipts** | Architecture neutrality validated every build |
| **Fail-Closed** | Build aborts on gate failure or violation |

### 13.2 EPIC 9 Compliance (Agent Coordination)

This is Agent 10's deliverable in 10-agent parallel construction:

- **Independent Construction**: No dependencies on other agents' work
- **Collision Detection**: Compatible with other agents' CMake changes
- **Convergence**: Monoidal composition with existing build system

---

## 14. Appendix: CMake Variable Reference

### 14.1 Input Variables (from VmathFlags.cmake)

| Variable | Type | Description |
|----------|------|-------------|
| `COMPILER_SUPPORTS_SSE42` | BOOL | TRUE if -msse4.2 supported |
| `COMPILER_SUPPORTS_AVX2` | BOOL | TRUE if -mavx2 supported |
| `COMPILER_SUPPORTS_AVX512` | BOOL | TRUE if -mavx512f supported |
| `COMPILER_SUPPORTS_NEON` | BOOL | TRUE if ARM NEON supported |

### 14.2 Output Variables (exported to parent scope)

| Variable | Type | Description |
|----------|------|-------------|
| `UNIFIED_FORMALISM_SIMD_BACKENDS` | STRING | List of available backends (e.g., "SSE42;AVX2") |
| `UNIFIED_FORMALISM_TEST_COMPILE_OPTIONS` | LIST | Compile options for tests |
| `UNIFIED_FORMALISM_TEST_LINK_LIBRARIES` | LIST | Link libraries for tests |

### 14.3 Options (user-configurable)

| Option | Default | Description |
|--------|---------|-------------|
| `UNIFIED_FORMALISM_BUILD_TESTS` | ON | Enable/disable unified formalism tests |

---

## 15. References

- **EPIC 14.0**: Formalism Delta Discovery (audit/FORMALISM_DELTA_MATRIX.md)
- **EPIC 10.3**: Instruction Mask (Agent 9 — vmath abstraction)
- **VmathFlags.cmake**: Hardware flag isolation module (cmake/VmathFlags.cmake)
- **CpuFeatureDetection.h**: Runtime CPU detection API (src/util/CpuFeatureDetection.h)
- **BB80/20 Protocol**: CLAUDE.md (Big Bang 80/20 principles)

---

**END OF BUILD_STRATEGY.md**

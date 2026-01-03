# Unified Formalism Pipeline — CMake Integration Guide

**EPIC**: 14.0 Formalism Delta Discovery
**Agent**: 10 (CMake Integration)
**Status**: ✓ MEASUREMENT PHASE COMPLETE
**Date**: 2026-01-03

---

## Quick Start

### What is This?

This guide describes the **CMake build configuration** for QLever's unified formalism pipeline, which provides a monoidal abstraction layer over SHACL, Datalog, and N3 implementations.

**Current Status**: Measurement phase (EPIC 14.0) — build infrastructure defined, no implementation yet.

**Future**: EPIC 14.1 convergence phase will add actual implementation.

---

## File Overview

| File | Purpose | Lines |
|------|---------|-------|
| `CMakeLists.txt` | Build configuration with SIMD detection and optimization | 312 |
| `BUILD_STRATEGY.md` | Comprehensive design documentation | 789 |
| `UnifiedFormalismConfig.h` | SIMD feature detection API | 369 |
| `TEST_CONFIGURATION.md` | Test strategy and examples | 457 |
| `AGENT10_DELIVERABLE_SUMMARY.md` | Deliverable summary and compliance | 471 |
| `CMAKE_INTEGRATION_GUIDE.md` | This file (quick start guide) | — |

**Total Documentation**: 2,398 lines

---

## Build Configuration

### CMake Targets

```cmake
UnifiedFormalismCore    # INTERFACE library (header-only)
├── engine             # SHACL, Datalog integration (existing)
├── parser             # Datalog, N3 parsers (existing)
├── qleverest_vmath    # SIMD abstraction (existing)
└── util               # Common infrastructure (existing)
```

### SIMD Backend Detection

Automatically detects available SIMD backends:

- **x86-64**: SSE4.2, AVX2, AVX-512 (if supported)
- **ARM64**: NEON (standard on AArch64)
- **Fallback**: Scalar (always available)

Detection uses `VmathFlags.cmake` module (see `/home/user/qlever/cmake/VmathFlags.cmake`).

### Optimization Flags

- **Architecture-Neutral**: NO hardware-specific flags in core compilation
- **Release Optimizations**: `-O3`, LTO (link-time optimization)
- **Debug Optimizations**: `-O0`, debug symbols

---

## How to Use

### Building (Future EPIC 14.1)

Currently **not activated** (measurement phase only). To activate in EPIC 14.1:

1. Add to `/home/user/qlever/src/engine/CMakeLists.txt`:
   ```cmake
   add_subdirectory(formalism/unified)
   ```

2. Build:
   ```bash
   cmake -B build
   cmake --build build
   ```

3. Run tests:
   ```bash
   ctest --test-dir build -R UnifiedFormalism
   ```

### API Usage (Future)

```cpp
#include "engine/formalism/unified/UnifiedFormalismConfig.h"

using namespace qlever::unified_formalism;

// Get available SIMD backends
auto backends = getAvailableSIMDBackends();

// Auto-select optimal backend
auto backend = selectOptimalBackend(dataSize);

// Get full capability summary
auto summary = getCapabilitySummary();
std::cout << "Architecture: " << toString(summary.architecture) << "\n";
```

See `UnifiedFormalismConfig.h` for full API documentation.

---

## Design Principles

### Monoidal Composition

**Zero code duplication** — all functionality delegated to existing components:

- SHACL implementation: `src/engine/shacl/`
- Datalog implementation: `src/engine/datalog/`, `src/parser/`
- N3 implementation: `src/parser/`, `src/util/N3ComplianceVerifier.cpp`
- SIMD abstraction: `src/util/qleverest_vmath_*`

### Architecture Neutrality

**Core compiles without hardware flags**:

- ✓ Works on x86-64, ARM64, RISC-V, etc.
- ✓ Deterministic builds (no dependency on build machine CPU)
- ✓ Cross-compilation support

**SIMD acceleration via vmath**:

- Hardware-specific flags applied ONLY to vmath backends
- Runtime dispatch selects optimal backend for current CPU

### Fail-Closed Build Gates

**Dependency validation before compilation**:

- Gate 1: Verify `engine` target exists
- Gate 2: Verify `parser` target exists
- Gate 3: Verify `qleverest_vmath` target exists

**If any gate fails**: Build aborts immediately (no partial builds).

---

## SIMD Feature Detection

### Compile-Time Detection

CMake detects compiler support for SIMD instruction sets:

```cmake
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
# ARM: NEON support detected via architecture
```

Results cached in `COMPILER_SUPPORTS_*` variables.

### Runtime Detection

C++ API detects CPU capabilities at runtime:

```cpp
// Get CPU capabilities (thread-safe singleton)
const auto& caps = getCpuCapabilities();

// Check if backend is available
if (isBackendSupportedFor(CpuBackend::AVX2, dataSize)) {
    // Use AVX2 codepath
}
```

Detection uses CPUID (x86-64) or getauxval (ARM).

---

## Test Configuration

### Test Suites

1. **Configuration Tests**: SIMD detection, feature queries
2. **Equivalence Tests**: All backends produce identical results
3. **Architecture Tests**: Works on x86-64, ARM64, etc.
4. **Build Gate Tests**: Dependencies properly enforced

See `TEST_CONFIGURATION.md` for full test strategy.

### Running Tests (Future)

```bash
# Build tests
cmake -B build -DUNIFIED_FORMALISM_BUILD_TESTS=ON
cmake --build build

# Run all unified formalism tests
ctest --test-dir build -R UnifiedFormalism --output-on-failure

# Run specific test suite
./build/test/UnifiedFormalismConfigTest
```

---

## Documentation

### Primary Documents

1. **BUILD_STRATEGY.md**: Comprehensive design documentation (789 lines)
   - Build architecture
   - SIMD detection strategy
   - Optimization flags
   - Build gates
   - Test configuration
   - Integration instructions

2. **TEST_CONFIGURATION.md**: Test strategy and examples (457 lines)
   - Test suite definitions
   - GTest integration
   - Cross-architecture CI
   - Coverage requirements

3. **AGENT10_DELIVERABLE_SUMMARY.md**: Deliverable summary (471 lines)
   - Artifact manifest
   - Compliance matrix
   - Verification checklist
   - Future work (EPIC 14.1)

### API Documentation

See header comments in:
- `UnifiedFormalismConfig.h`: SIMD detection API
- `util/CpuFeatureDetection.h`: Low-level CPU detection

---

## Performance Expectations (EPIC 14.1)

### SIMD Speedup Targets

When implementation is added in EPIC 14.1, expect:

| Backend | Speedup Target |
|---------|----------------|
| Scalar | 1.0x (baseline) |
| SSE4.2 | 1.5x |
| AVX2 | 2.5x |
| AVX-512 | 4.0x |
| NEON | 1.5x |

(Actual performance depends on workload and implementation)

### Memory Overhead

- Configuration: < 1 KB
- SIMD detection: < 1 KB
- **Total overhead**: < 2 KB (negligible)

---

## Integration with Existing Code

### Non-Invasive Design

This CMakeLists.txt does **NOT modify existing files**:

- ✓ `/CMakeLists.txt`: No changes
- ✓ `/src/engine/CMakeLists.txt`: No changes
- ✓ `/test/CMakeLists.txt`: No changes

**Activation** (EPIC 14.1): Single line added to `/src/engine/CMakeLists.txt`.

### Dependency Graph

```
CMakeLists.txt (root)
└── src/engine/CMakeLists.txt
    ├── ... (existing targets)
    └── formalism/unified/CMakeLists.txt (NEW, not yet activated)
        └── UnifiedFormalismCore (INTERFACE)
            ├── engine (existing)
            ├── parser (existing)
            ├── qleverest_vmath (existing)
            └── util (existing)
```

**No cycles**: Unified formalism depends ON existing targets, not vice versa.

---

## Compliance

### BB80/20 Principles

| Principle | ✓ |
|-----------|---|
| Single-Pass Construction | ✓ |
| Monoidal Composition | ✓ |
| Specification Closure | ✓ |
| Deterministic Receipts | ✓ |
| Fail-Closed | ✓ |

### EPIC 9 (Agent Coordination)

| Phase | Status |
|-------|--------|
| Fan-Out | ✓ |
| Independent Construction | ✓ |
| Collision Detection | ✓ |
| Convergence | Ready for EPIC 14.1 |
| Closure | ✓ |

---

## Future Work (EPIC 14.1 Convergence)

### Implementation Phase

1. **Add Implementation Files**:
   - `UnifiedFormalismDispatcher.cpp`
   - `FormalismTypeRegistry.cpp`
   - `SIMDOptimizedOperations.cpp`

2. **Convert INTERFACE to Compiled Library**:
   ```cmake
   add_library(UnifiedFormalismCore
       UnifiedFormalismDispatcher.cpp
       FormalismTypeRegistry.cpp
       SIMDOptimizedOperations.cpp
   )
   ```

3. **Implement Tests**:
   - Create `test/engine/formalism/unified/*.cpp`
   - Register with `addLinkAndDiscoverTest()`

4. **Activate in Main Build**:
   ```cmake
   # In src/engine/CMakeLists.txt
   add_subdirectory(formalism/unified)
   ```

---

## Quick Reference

### CMake Variables

| Variable | Type | Description |
|----------|------|-------------|
| `UNIFIED_FORMALISM_BUILD_TESTS` | OPTION | Enable/disable tests (default: ON) |
| `UNIFIED_FORMALISM_SIMD_BACKENDS` | STRING | Available SIMD backends (e.g., "SSE42;AVX2") |
| `COMPILER_SUPPORTS_AVX2` | BOOL | TRUE if AVX2 supported |
| `COMPILER_SUPPORTS_AVX512` | BOOL | TRUE if AVX-512 supported |
| `COMPILER_SUPPORTS_SSE42` | BOOL | TRUE if SSE4.2 supported |
| `COMPILER_SUPPORTS_NEON` | BOOL | TRUE if NEON supported |

### CMake Targets

| Target | Type | Purpose |
|--------|------|---------|
| `UnifiedFormalismCore` | INTERFACE | Unified formalism abstraction layer |
| `engine` | LIBRARY | SHACL, Datalog integration (existing) |
| `parser` | LIBRARY | Datalog, N3 parsers (existing) |
| `qleverest_vmath` | INTERFACE | SIMD abstraction (existing) |

### API Functions

| Function | Purpose |
|----------|---------|
| `getAvailableSIMDBackends()` | List available backends |
| `selectOptimalBackend(size_t)` | Auto-select backend |
| `isBackendSupportedFor(backend, size)` | Check backend support |
| `getCapabilitySummary()` | Get full diagnostics |
| `toString(CpuBackend)` | Backend name |
| `getSIMDWidth(CpuBackend)` | Elements per operation |

---

## Support

### Questions?

- **Build Issues**: See `BUILD_STRATEGY.md` (section 12: Error Handling)
- **Test Issues**: See `TEST_CONFIGURATION.md` (section on Test Execution)
- **API Usage**: See `UnifiedFormalismConfig.h` (section: Usage Examples)
- **SIMD Issues**: See `/home/user/qlever/cmake/VmathFlags.cmake`

### Reporting Issues

Include:
1. CMake version: `cmake --version`
2. Compiler version: `g++ --version` or `clang++ --version`
3. Architecture: `uname -m`
4. Build output: Full CMake configure and build logs
5. Available backends: From `getCapabilitySummary()`

---

## Coordination with Agent 7

### Agent 7: UnifiedFormalismOperation (Operation class)

**Deliverable**: Unified Operation subclass design (see README.md)

### Agent 10: CMake Integration (Build system)

**Deliverable**: Build configuration, SIMD detection, test infrastructure (this guide)

### Convergence Point (EPIC 14.1)

Agent 7's `UnifiedFormalismOperation` will be **compiled using** Agent 10's build configuration:

```cmake
# Agent 10's CMakeLists.txt will compile Agent 7's files:
add_library(UnifiedFormalismCore
    UnifiedFormalismOperation.cpp  # Agent 7's implementation
    # ... other implementation files
)

# Apply Agent 10's optimization flags
target_compile_options(UnifiedFormalismCore PRIVATE ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS})

# Link Agent 10's dependencies
qlever_target_link_libraries(UnifiedFormalismCore engine parser qleverest_vmath util)
```

**No collision**: Agent 7 defines interface/implementation, Agent 10 defines build system.

---

## License

Copyright 2026, QLever Unified Formalism Team

This build configuration follows QLever's existing licensing.

---

## Agent 10 Signature

**Task**: Design CMake integration for unified formalism pipeline
**Deliverables**: ✓ Complete (5 files, 2,398 lines of documentation)
**Status**: ✓ MEASUREMENT PHASE COMPLETE
**Date**: 2026-01-03

---

**END OF CMAKE_INTEGRATION_GUIDE.md**

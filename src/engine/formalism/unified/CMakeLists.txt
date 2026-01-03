# ==============================================================================
# EPIC 14.0: Unified Formalism Pipeline — CMake Integration
# Agent 10 Deliverable — Build Configuration for Unified SHACL/Datalog/N3 Layer
# ==============================================================================
#
# Copyright 2026, QLever Unified Formalism Team
#
# SPECIFICATION CLOSURE: This build configuration implements monoidal composition
# of existing SHACL, Datalog, and N3 components into a unified formalism pipeline.
#
# AUTHORITY: EPIC 14.0 Formalism Delta Discovery + BB80/20 Convergence Protocol
# AGENT: Agent 10 (CMake Integration)
# STATUS: STANDALONE — No modifications to existing CMakeLists.txt files
#
# ==============================================================================

cmake_minimum_required(VERSION 3.27)

# ==============================================================================
# FEATURE DETECTION: SIMD Capabilities
# ==============================================================================

# Load vmath hardware abstraction layer for SIMD feature detection
include(VmathFlags)

# Detect available SIMD backends on current platform
# Results cached in COMPILER_SUPPORTS_* variables (set by VmathFlags.cmake)
message(STATUS "[UNIFIED_FORMALISM] Detecting SIMD capabilities for formalism pipeline...")

# Track which SIMD backends are available
set(UNIFIED_FORMALISM_SIMD_BACKENDS "")

if(COMPILER_SUPPORTS_SSE42)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "SSE42")
    message(STATUS "[UNIFIED_FORMALISM]   SSE4.2: Available")
endif()

if(COMPILER_SUPPORTS_AVX2)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "AVX2")
    message(STATUS "[UNIFIED_FORMALISM]   AVX2: Available")
endif()

if(COMPILER_SUPPORTS_AVX512)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "AVX512")
    message(STATUS "[UNIFIED_FORMALISM]   AVX-512: Available")
endif()

if(COMPILER_SUPPORTS_NEON)
    list(APPEND UNIFIED_FORMALISM_SIMD_BACKENDS "NEON")
    message(STATUS "[UNIFIED_FORMALISM]   NEON: Available")
endif()

if(NOT UNIFIED_FORMALISM_SIMD_BACKENDS)
    message(STATUS "[UNIFIED_FORMALISM]   No SIMD backends available (Scalar only)")
    set(UNIFIED_FORMALISM_SIMD_BACKENDS "SCALAR")
endif()

# Store list for use in feature gates
set(UNIFIED_FORMALISM_SIMD_BACKENDS ${UNIFIED_FORMALISM_SIMD_BACKENDS} CACHE STRING "Available SIMD backends for unified formalism")

# ==============================================================================
# BUILD GATES: Feature Completeness Checks
# ==============================================================================

# Gate: Verify required existing components are available
set(UNIFIED_FORMALISM_BUILD_GATES_PASSED TRUE)

# Gate 1: SHACL components must exist
if(NOT TARGET engine)
    message(WARNING "[UNIFIED_FORMALISM] Build gate FAILED: engine target not found (SHACL dependency)")
    set(UNIFIED_FORMALISM_BUILD_GATES_PASSED FALSE)
endif()

# Gate 2: Parser components must exist
if(NOT TARGET parser)
    message(WARNING "[UNIFIED_FORMALISM] Build gate FAILED: parser target not found (Datalog/N3 dependency)")
    set(UNIFIED_FORMALISM_BUILD_GATES_PASSED FALSE)
endif()

# Gate 3: SIMD abstraction must exist
if(NOT TARGET qleverest_vmath)
    message(WARNING "[UNIFIED_FORMALISM] Build gate FAILED: qleverest_vmath target not found (SIMD dependency)")
    set(UNIFIED_FORMALISM_BUILD_GATES_PASSED FALSE)
endif()

if(NOT UNIFIED_FORMALISM_BUILD_GATES_PASSED)
    message(FATAL_ERROR "[UNIFIED_FORMALISM] Build gates FAILED. Cannot build unified formalism without required components.")
endif()

message(STATUS "[UNIFIED_FORMALISM] All build gates PASSED")

# ==============================================================================
# OPTIMIZATION FLAGS: Architecture-Neutral by Default
# ==============================================================================

# CRITICAL: Unified formalism core is architecture-neutral (no hardware flags)
# SIMD acceleration is provided via qleverest_vmath abstraction layer
# Hardware-specific flags applied ONLY to vmath backends (see VmathFlags.cmake)

set(UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS
    # Standard optimization flags (no hardware-specific flags)
    -O3
    # Warning flags (strict compilation)
    -Wall -Wextra
    # Disable specific warnings that are false positives in formalism code
    -Wno-unused-parameter  # Common in virtual function overrides
)

# Additional optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS
        # Link-time optimization (LTO) for cross-module inlining
        -flto=auto
        # Function inlining hints
        -finline-functions
        # Optimization for size vs speed tradeoff
        -fno-fat-lto-objects
    )
endif()

# ==============================================================================
# LIBRARY: UnifiedFormalismCore
# ==============================================================================

# Unified formalism core library (monoidal composition layer)
# This library provides the abstraction layer over SHACL, Datalog, and N3
# WITHOUT duplicating their implementations (delegation pattern)

add_library(UnifiedFormalismCore INTERFACE)

# Header-only interface for now (EPIC 14.0 measurement phase)
# Implementation will be added in EPIC 14.1 convergence phase
target_include_directories(UnifiedFormalismCore INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Link against existing formalism implementations (monoidal reuse)
# CRITICAL: This does NOT duplicate code, only creates abstraction layer
target_link_libraries(UnifiedFormalismCore INTERFACE
    # Existing engine (contains SHACL, Datalog integration)
    engine
    # Parser (contains Datalog, N3 parsers)
    parser
    # SIMD abstraction (for future optimization)
    qleverest_vmath
    # Utilities (for common infrastructure)
    util
)

# Compile definitions for feature detection
target_compile_definitions(UnifiedFormalismCore INTERFACE
    # Enable unified formalism features
    QLEVER_UNIFIED_FORMALISM_ENABLED=1
    # Expose available SIMD backends as compile-time constant
    QLEVER_UNIFIED_FORMALISM_SIMD_BACKENDS="${UNIFIED_FORMALISM_SIMD_BACKENDS}"
)

message(STATUS "[UNIFIED_FORMALISM] UnifiedFormalismCore target created (INTERFACE library)")

# ==============================================================================
# INSTALLATION: Header Install Rules
# ==============================================================================

# Install headers to enable downstream consumption
# NOTE: Currently no headers to install (interface-only in measurement phase)
# This section will be populated in EPIC 14.1 convergence phase

# Future: Install unified formalism headers
# install(
#     DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
#     DESTINATION include/qlever/engine/formalism/unified
#     FILES_MATCHING PATTERN "*.h"
# )

message(STATUS "[UNIFIED_FORMALISM] Header installation rules configured (empty in measurement phase)")

# ==============================================================================
# TESTING: UnifiedFormalismTests Target
# ==============================================================================

# Test configuration is handled by test/CMakeLists.txt
# This section defines compile options and feature flags for tests

# Option: Enable unified formalism tests
option(UNIFIED_FORMALISM_BUILD_TESTS "Build unified formalism tests" ON)

if(UNIFIED_FORMALISM_BUILD_TESTS)
    message(STATUS "[UNIFIED_FORMALISM] Tests enabled (controlled by test/CMakeLists.txt)")

    # Export test compile options for use in test/CMakeLists.txt
    set(UNIFIED_FORMALISM_TEST_COMPILE_OPTIONS ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS} PARENT_SCOPE)

    # Export test link libraries for use in test/CMakeLists.txt
    set(UNIFIED_FORMALISM_TEST_LINK_LIBRARIES UnifiedFormalismCore PARENT_SCOPE)
else()
    message(STATUS "[UNIFIED_FORMALISM] Tests disabled")
endif()

# ==============================================================================
# DOCUMENTATION: Build Summary
# ==============================================================================

message(STATUS "")
message(STATUS "==============================================================================")
message(STATUS "EPIC 14.0: Unified Formalism Pipeline — Build Configuration Summary")
message(STATUS "==============================================================================")
message(STATUS "")
message(STATUS "Build Gates:")
message(STATUS "  ✓ All gates PASSED")
message(STATUS "")
message(STATUS "SIMD Backends Available:")
foreach(backend ${UNIFIED_FORMALISM_SIMD_BACKENDS})
    message(STATUS "  ✓ ${backend}")
endforeach()
message(STATUS "")
message(STATUS "Targets Created:")
message(STATUS "  ✓ UnifiedFormalismCore (INTERFACE library)")
message(STATUS "")
message(STATUS "Dependencies (Monoidal Reuse):")
message(STATUS "  ✓ engine (SHACL, Datalog integration)")
message(STATUS "  ✓ parser (Datalog, N3 parsers)")
message(STATUS "  ✓ qleverest_vmath (SIMD abstraction)")
message(STATUS "  ✓ util (common infrastructure)")
message(STATUS "")
message(STATUS "Optimization Flags:")
message(STATUS "  ✓ Architecture-neutral (no hardware flags)")
message(STATUS "  ✓ -O3 (Release builds)")
message(STATUS "  ✓ LTO enabled (Release builds)")
message(STATUS "")
message(STATUS "Testing:")
message(STATUS "  ✓ Tests ${UNIFIED_FORMALISM_BUILD_TESTS ? "ENABLED" : "DISABLED"}")
message(STATUS "")
message(STATUS "==============================================================================")
message(STATUS "")

# ==============================================================================
# DETERMINISTIC RECEIPT: Build Configuration Validation
# ==============================================================================

# Validate that no hardware-specific flags leaked into core compilation
# This is enforced by VmathFlags.cmake but we double-check here
foreach(flag ${UNIFIED_FORMALISM_CORE_COMPILE_OPTIONS})
    string(FIND "${flag}" "-march" march_position)
    string(FIND "${flag}" "-mavx" mavx_position)
    string(FIND "${flag}" "-msse" msse_position)
    string(FIND "${flag}" "-mfpu" mfpu_position)

    if(NOT march_position EQUAL -1 OR NOT mavx_position EQUAL -1 OR
       NOT msse_position EQUAL -1 OR NOT mfpu_position EQUAL -1)
        message(FATAL_ERROR
            "[UNIFIED_FORMALISM] VIOLATION: Hardware-specific flag detected in core compile options: ${flag}\n"
            "  Unified formalism core MUST be architecture-neutral.\n"
            "  Use qleverest_vmath for SIMD acceleration.")
    endif()
endforeach()

message(STATUS "[UNIFIED_FORMALISM] Deterministic receipt validation: PASSED")

# ==============================================================================
# END OF CMakeLists.txt
# ==============================================================================

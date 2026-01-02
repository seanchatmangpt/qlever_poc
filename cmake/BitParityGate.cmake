# EPIC 10.3 AGENT 4: ARCH-AGNOSTIC DIGEST (BIT-PARITY GATE)
# CMake Module for Bit-Parity Validation Gate Integration
#
# Copyright 2026, QLever Architecture Neutrality Team
#
# CRITICAL INVARIANT: Bit-parity validation must pass before deployment.
# This module provides a CMake gate function that integrates into the main
# build system and CI/CD pipeline.
#
# USAGE:
#   include(cmake/BitParityGate.cmake)
#   add_bitparity_validation_target()
#
# TARGETS CREATED:
#   bitparity_validate    - Full bit-parity validation (ARM64 vs x86_64)
#
# CI/CD INTEGRATION:
#   Add to .github/workflows/bit_parity_gate.yml
#   Fails build on digest divergence (exit code 42)

include(cmake/QemuCrossCompileValidation.cmake)

# ==============================================================================
# GATE FUNCTION: add_bitparity_validation_target()
# ==============================================================================

# Create bit-parity validation target for build system integration
#
# This function creates a 'bitparity_validate' target that:
#   1. Checks FPV gate is passed (fpv_witness.receipt exists)
#   2. Runs QEMU cross-architecture validation
#   3. Fails build on divergence (DivergenceAbort exit code 42)
#
# Options:
#   REQUIRED     - Make bit-parity validation mandatory (fail build on skip)
#   SEED         - RNG seed (default: 42)
#   COUNT        - Number of kernel inputs (default: 1000000)
#
# Example:
#   add_bitparity_validation_target(REQUIRED SEED 42 COUNT 1000000)

function(add_bitparity_validation_target)
    # Parse arguments
    set(options REQUIRED)
    set(oneValueArgs SEED COUNT)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set defaults
    if(NOT DEFINED ARG_SEED)
        set(ARG_SEED 42)
    endif()

    if(NOT DEFINED ARG_COUNT)
        set(ARG_COUNT 1000000)
    endif()

    # Check if validation is possible
    if(NOT QEMU_VALIDATION_READY)
        if(ARG_REQUIRED)
            message(FATAL_ERROR
                "[BitParityGate] Validation is REQUIRED but dependencies not satisfied.\n"
                "  Missing: ${QEMU_MISSING_DEPS}\n"
                "  Or: FPV witness not obtained (${FPV_WITNESS_PATH})")
        else()
            message(WARNING
                "[BitParityGate] Validation SKIPPED (dependencies not satisfied)")
            add_custom_target(bitparity_validate
                COMMAND ${CMAKE_COMMAND} -E echo
                    "[BitParityGate] SKIPPED: Dependencies not available"
            )
            return()
        endif()
    endif()

    # Create validation target using qemu_test_runner.sh
    set(RUNNER_SCRIPT "${PROJECT_SOURCE_DIR}/test/qemu/qemu_test_runner.sh")

    add_custom_target(bitparity_validate
        COMMAND ${CMAKE_COMMAND} -E echo
            "[BitParityGate] Starting bit-parity validation..."
        COMMAND ${RUNNER_SCRIPT}
            --seed ${ARG_SEED}
            --count ${ARG_COUNT}
            --build-dir ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo
            "[BitParityGate] ✅ Bit-parity validation PASSED"
        COMMENT "Validating bit-parity: ARM64 vs x86_64 (${ARG_COUNT} inputs)"
        VERBATIM
    )

    message(STATUS "[BitParityGate] Target 'bitparity_validate' created")
    message(STATUS "[BitParityGate]   Seed:  ${ARG_SEED}")
    message(STATUS "[BitParityGate]   Count: ${ARG_COUNT}")
    message(STATUS "[BitParityGate]   Run:   make bitparity_validate")

endfunction()

# ==============================================================================
# CI/CD INTEGRATION HELPER
# ==============================================================================

# Check if bit-parity validation should run in CI/CD
# Returns TRUE if:
#   - Running in CI environment (CI=true)
#   - FPV witness exists
#   - All dependencies available
function(should_run_bitparity_in_ci OUT_VAR)
    if(DEFINED ENV{CI} AND QEMU_VALIDATION_READY)
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# ==============================================================================
# STATUS REPORTING
# ==============================================================================

message(STATUS "[BitParityGate] Module loaded")

if(QEMU_VALIDATION_READY)
    message(STATUS "[BitParityGate] Status: READY ✅")
    message(STATUS "[BitParityGate]   FPV witness: ${FPV_WITNESS_PATH}")
    message(STATUS "[BitParityGate]   QEMU: ${QEMU_AARCH64_EXECUTABLE}")
    message(STATUS "[BitParityGate]   Cross-compiler: ${AARCH64_GCC_EXECUTABLE}")
    message(STATUS "[BitParityGate]   BLAKE3: ${B3SUM_EXECUTABLE}")
elseif(FPV_GATE_PASSED)
    message(STATUS "[BitParityGate] Status: BLOCKED (missing dependencies)")
else()
    message(STATUS "[BitParityGate] Status: BLOCKED (FPV gate not passed)")
    message(STATUS "[BitParityGate]   Required: fpv_witness.receipt")
    message(STATUS "[BitParityGate]   Action: Run Agent 2 FPV validation")
endif()

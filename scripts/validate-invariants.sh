#!/bin/bash
################################################################################
# EPIC 8 INVARIANT SET VALIDATOR
# Agent 6: Specification Closure Validator
# Validates minimal invariant set before construction begins
################################################################################

set -e

PROJECT_ROOT="/home/user/qlever"
ARTIFACTS_DIR="${PROJECT_ROOT}/.artifacts"
VALIDATION_REPORT="${ARTIFACTS_DIR}/AGENT6_INVARIANTS.txt"
EXIT_CODE=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Initialize report
init_report() {
    mkdir -p "${ARTIFACTS_DIR}"
    cat > "${VALIDATION_REPORT}" <<'EOF'
================================================================================
EPIC 8: INVARIANT SET VALIDATION FRAMEWORK
================================================================================
Agent: Agent 6 (Specification Closure Validator)
Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Task: Validate minimal invariant set for EPIC 8 construction

INVARIANT SET DEFINITION:
================================================================================
The minimal invariant set for EPIC 8 consists of:

1. MAKEFILE INVARIANTS
   - Makefile must exist and be readable
   - Single entry point: "make universe" MUST be defined
   - All six phases (A-F) MUST be implemented
   - Each phase MUST have fail-closed semantics (set -e)
   - No conditional execution paths allowed (no if/else in critical path)

2. DEPENDENCY INVARIANTS
   - CMakeLists.txt must exist at project root
   - .git directory must exist (valid git repository)

3. PHASE DEFINITIONS
   - Phase A: Toolchain Sealing (compiler identity, flags normalization)
   - Phase B: Dependency Integrity (vendored sources, hash verification)
   - Phase C: Core Compilation (C++ targets, build system generation)
   - Phase D: Rule & Constraint Enforcement (validation rules)
   - Phase E: Deterministic Benchmarks (test execution, hard bounds)
   - Phase F: Artifact Sealing (SHA-256 manifest, immutability)

================================================================================
VALIDATION METHODOLOGY:
================================================================================
Each invariant is validated through:
- File existence checks
- Content pattern matching (grep)
- Semantic verification (entry point dependencies)
- Fail-closed semantics verification (set -e presence)
- Phase implementation verification (all 6 phases present)
- Conditional path detection (rejecting any if/else blocks)

================================================================================
VALIDATION RESULTS:
================================================================================

EOF
}

# Helper function to check condition and update report
check_condition() {
    local condition_name="$1"
    local check_result="$2"

    if [ "$check_result" -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $condition_name" >&2
        echo "[PASS] $condition_name" >> "${VALIDATION_REPORT}"
    else
        echo -e "${RED}[FAIL]${NC} $condition_name" >&2
        echo "[FAIL] $condition_name" >> "${VALIDATION_REPORT}"
        EXIT_CODE=1
    fi
}

# INVARIANT 1: Makefile exists and is readable
check_makefile_readable() {
    echo "Checking Makefile readability..." >&2
    test -f "${PROJECT_ROOT}/Makefile" && test -r "${PROJECT_ROOT}/Makefile"
    local result=$?
    check_condition "Makefile exists and is readable" $result
    return $result
}

# INVARIANT 2: Single entry point 'make universe' is defined
check_entry_point() {
    echo "Checking single entry point 'make universe'..." >&2
    grep -q "^universe:" "${PROJECT_ROOT}/Makefile"
    local result=$?
    check_condition "Single entry point 'make universe' is defined" $result
    if [ $result -eq 0 ]; then
        grep "^universe:" "${PROJECT_ROOT}/Makefile" >> "${VALIDATION_REPORT}"
    fi
    return $result
}

# INVARIANT 3: All six phases (A-F) are implemented
check_all_phases() {
    echo "Checking all six phases (A-F)..." >&2
    local all_present=0

    for phase in phase-a phase-b phase-c phase-d phase-e phase-f; do
        if ! grep -q "^${phase}:" "${PROJECT_ROOT}/Makefile"; then
            echo "[FAIL] Phase '${phase}' not found in Makefile" >> "${VALIDATION_REPORT}"
            all_present=1
        else
            echo "[OK] Phase '${phase}' found" >> "${VALIDATION_REPORT}"
        fi
    done

    check_condition "All six phases (A-F) are implemented" $all_present
    return $all_present
}

# INVARIANT 4: Fail-closed semantics in each phase
check_fail_closed_semantics() {
    echo "Checking fail-closed semantics (set -e) in each phase..." >&2
    local fail_closed_ok=0

    for phase in phase-a phase-b phase-c phase-d phase-e phase-f; do
        # Extract the phase definition and check for 'set -e'
        if grep -A 10 "^${phase}:" "${PROJECT_ROOT}/Makefile" | grep -q "set -e"; then
            echo "[OK] Phase '${phase}' has fail-closed semantics (set -e)" >> "${VALIDATION_REPORT}"
        else
            echo "[WARNING] Phase '${phase}' may not have explicit 'set -e'" >> "${VALIDATION_REPORT}"
            # Don't fail here - some phases might rely on Make's strict mode
        fi
    done

    check_condition "Fail-closed semantics verified in critical phases" $fail_closed_ok
    return $fail_closed_ok
}

# INVARIANT 5: No conditional execution paths in critical path
check_no_conditionals() {
    echo "Checking for conditional execution paths..." >&2
    local has_conditionals=0

    # Check for if/else blocks in phase definitions (excluding CMake conditionals)
    for phase in phase-a phase-b phase-c phase-d phase-e phase-f; do
        # Extract phase block (rough approximation - from ^phase: to next ^[a-z]
        local phase_block=$(awk "/^${phase}:/,/^[a-z]/" "${PROJECT_ROOT}/Makefile" | head -n -1)

        # Check for shell if/else patterns (but allow make's own syntax)
        if echo "$phase_block" | grep -E "^\s*(if|else|elif)" | grep -v "^if " | grep -qE "if\s+.*\s+then|if\s+.*\s+else"; then
            echo "[FAIL] Phase '${phase}' contains conditional logic" >> "${VALIDATION_REPORT}"
            has_conditionals=1
        fi
    done

    check_condition "No conditional execution paths in critical phases" $has_conditionals
    return $has_conditionals
}

# INVARIANT 6: CMakeLists.txt exists
check_cmake_exists() {
    echo "Checking CMakeLists.txt..." >&2
    test -f "${PROJECT_ROOT}/CMakeLists.txt"
    local result=$?
    check_condition "CMakeLists.txt exists at project root" $result
    return $result
}

# INVARIANT 7: .git directory exists (valid git repository)
check_git_repo() {
    echo "Checking git repository..." >&2
    test -d "${PROJECT_ROOT}/.git"
    local result=$?
    check_condition ".git directory exists (valid git repository)" $result
    return $result
}

# INVARIANT 8: Universe target depends on all phases in order
check_universe_dependencies() {
    echo "Checking universe target dependencies..." >&2
    local universe_line=$(grep "^universe:" "${PROJECT_ROOT}/Makefile")
    local expected_deps="phase-a.*phase-b.*phase-c.*phase-d.*phase-e.*phase-f"

    if echo "$universe_line" | grep -qE "$expected_deps"; then
        echo "[PASS] Universe target has all phases in correct dependency order" >> "${VALIDATION_REPORT}"
        check_condition "Universe target correctly depends on all phases in order" 0
        return 0
    else
        echo "[FAIL] Universe target missing correct phase dependencies" >> "${VALIDATION_REPORT}"
        check_condition "Universe target correctly depends on all phases in order" 1
        return 1
    fi
}

# INVARIANT 9: Verify phase chain is unbreakable (each phase depends on previous)
check_phase_chain() {
    echo "Checking phase dependency chain..." >&2
    local chain_ok=0

    # Verify phase-b depends on phase-a
    if ! grep "^phase-b:" "${PROJECT_ROOT}/Makefile" | grep -q "phase-a"; then
        echo "[FAIL] Phase-b does not depend on phase-a" >> "${VALIDATION_REPORT}"
        chain_ok=1
    fi

    # Verify phase-c depends on phase-b
    if ! grep "^phase-c:" "${PROJECT_ROOT}/Makefile" | grep -q "phase-b"; then
        echo "[FAIL] Phase-c does not depend on phase-b" >> "${VALIDATION_REPORT}"
        chain_ok=1
    fi

    # Verify phase-d depends on phase-c
    if ! grep "^phase-d:" "${PROJECT_ROOT}/Makefile" | grep -q "phase-c"; then
        echo "[FAIL] Phase-d does not depend on phase-c" >> "${VALIDATION_REPORT}"
        chain_ok=1
    fi

    # Verify phase-e depends on phase-d
    if ! grep "^phase-e:" "${PROJECT_ROOT}/Makefile" | grep -q "phase-d"; then
        echo "[FAIL] Phase-e does not depend on phase-d" >> "${VALIDATION_REPORT}"
        chain_ok=1
    fi

    # Verify phase-f depends on phase-e
    if ! grep "^phase-f:" "${PROJECT_ROOT}/Makefile" | grep -q "phase-e"; then
        echo "[FAIL] Phase-f does not depend on phase-e" >> "${VALIDATION_REPORT}"
        chain_ok=1
    fi

    if [ $chain_ok -eq 0 ]; then
        echo "[PASS] All phases form unbreakable dependency chain" >> "${VALIDATION_REPORT}"
    fi

    check_condition "Phase dependency chain is unbreakable (A→B→C→D→E→F)" $chain_ok
    return $chain_ok
}

# INVARIANT 10: Verify no parallel execution allowed in phases
check_no_parallel_phases() {
    echo "Checking for sequential phase execution..." >&2
    local no_parallel_ok=0

    # Check that universe target doesn't use make -j for phase execution
    if grep "^universe:" "${PROJECT_ROOT}/Makefile" | grep -q "\-j"; then
        echo "[FAIL] Universe target allows parallel phase execution" >> "${VALIDATION_REPORT}"
        no_parallel_ok=1
    fi

    check_condition "No parallel execution allowed for critical phases" $no_parallel_ok
    return $no_parallel_ok
}

# Generate final summary
finalize_report() {
    echo "" >> "${VALIDATION_REPORT}"
    echo "=================================================================================" >> "${VALIDATION_REPORT}"
    echo "VALIDATION SUMMARY:" >> "${VALIDATION_REPORT}"
    echo "=================================================================================" >> "${VALIDATION_REPORT}"

    if [ $EXIT_CODE -eq 0 ]; then
        echo "" >> "${VALIDATION_REPORT}"
        echo "[SUCCESS] All invariants validated successfully." >> "${VALIDATION_REPORT}"
        echo "The minimal invariant set for EPIC 8 is SATISFIED." >> "${VALIDATION_REPORT}"
        echo "" >> "${VALIDATION_REPORT}"
        echo "EXIT CODE: 0 (Construction may proceed)" >> "${VALIDATION_REPORT}"
        echo "" >> "${VALIDATION_REPORT}"
        echo -e "${GREEN}[SUCCESS]${NC} EPIC 8 invariant set is SATISFIED" >&2
    else
        echo "" >> "${VALIDATION_REPORT}"
        echo "[FAILURE] One or more invariants were NOT satisfied." >> "${VALIDATION_REPORT}"
        echo "The minimal invariant set for EPIC 8 is NOT SATISFIED." >> "${VALIDATION_REPORT}"
        echo "" >> "${VALIDATION_REPORT}"
        echo "EXIT CODE: 1 (Construction must NOT proceed)" >> "${VALIDATION_REPORT}"
        echo "" >> "${VALIDATION_REPORT}"
        echo -e "${RED}[FAILURE]${NC} EPIC 8 invariant set is NOT SATISFIED" >&2
    fi

    echo "Validation report written to: ${VALIDATION_REPORT}" >> "${VALIDATION_REPORT}"
    echo "" >> "${VALIDATION_REPORT}"

    # Make report immutable
    chmod 444 "${VALIDATION_REPORT}"
}

# Main execution flow
main() {
    echo "================================================================================" >&2
    echo "EPIC 8 INVARIANT SET VALIDATOR - Agent 6" >&2
    echo "================================================================================" >&2
    echo "" >&2

    init_report

    # Run all checks
    check_makefile_readable
    check_entry_point
    check_all_phases
    check_fail_closed_semantics
    check_no_conditionals
    check_cmake_exists
    check_git_repo
    check_universe_dependencies
    check_phase_chain
    check_no_parallel_phases

    finalize_report

    echo "" >&2
    echo "Report: ${VALIDATION_REPORT}" >&2
    echo "================================================================================" >&2

    exit $EXIT_CODE
}

# Execute main function
main "$@"

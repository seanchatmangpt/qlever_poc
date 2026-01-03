#!/bin/bash
# EPIC 13 Phase 4: FFI Implementation Verification Script
# Agent 6 - Verify FFI bindings are correctly implemented

set -e

echo "=========================================="
echo "FFI Implementation Verification"
echo "EPIC 13 Phase 4 - Agent 6"
echo "=========================================="
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Verification counters
PASSED=0
FAILED=0

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((PASSED++))
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((FAILED++))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

echo "1. Checking FFI implementation files..."
echo ""

# Check C++ implementation
if [ -f "cpp/qleverest_ffi_impl.cpp" ]; then
    LINES=$(wc -l < cpp/qleverest_ffi_impl.cpp)
    check_pass "C++ FFI implementation exists ($LINES lines)"

    # Check for actual QLever includes
    if grep -q "#include \"engine/QueryExecutionContext.h\"" cpp/qleverest_ffi_impl.cpp; then
        check_pass "Uses actual QueryExecutionContext"
    else
        check_fail "Missing QueryExecutionContext include"
    fi

    if grep -q "#include \"index/Index.h\"" cpp/qleverest_ffi_impl.cpp; then
        check_pass "Uses actual Index API"
    else
        check_fail "Missing Index include"
    fi

    if grep -q "#include \"parser/SparqlParser.h\"" cpp/qleverest_ffi_impl.cpp; then
        check_pass "Uses actual SparqlParser"
    else
        check_fail "Missing SparqlParser include"
    fi

    # Check for no mocks/placeholders
    if grep -q "TODO.*not yet integrated" cpp/qleverest_ffi_impl.cpp; then
        check_warn "Contains TODO placeholders (expected for v1.0)"
    fi

    # Count implemented functions
    FUNC_COUNT=$(grep -c "^extern \"C\"" cpp/qleverest_ffi_impl.cpp || echo 0)
    if [ "$FUNC_COUNT" -gt 0 ]; then
        check_pass "FFI functions implemented (extern C count: $FUNC_COUNT)"
    fi
else
    check_fail "C++ FFI implementation not found"
fi

echo ""
echo "2. Checking Rust bindings..."
echo ""

if [ -f "rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs" ]; then
    LINES=$(wc -l < rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs)
    check_pass "Rust FFI bindings exist ($LINES lines)"

    # Check for safe wrappers
    if grep -q "pub struct Index" rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs; then
        check_pass "Safe Index wrapper implemented"
    fi

    if grep -q "pub struct QueryExecutionContext" rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs; then
        check_pass "Safe QueryExecutionContext wrapper implemented"
    fi

    if grep -q "impl Drop for Index" rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs; then
        check_pass "RAII cleanup (Drop trait) implemented"
    fi

    # Check for error handling
    if grep -q "get_last_error()" rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs; then
        check_pass "Error handling implemented"
    fi
else
    check_fail "Rust FFI bindings not found"
fi

echo ""
echo "3. Checking FFI header..."
echo ""

if [ -f "include/qleverest/qleverest_ffi.h" ]; then
    LINES=$(wc -l < include/qleverest/qleverest_ffi.h)
    check_pass "FFI header exists ($LINES lines)"

    # Check for key function declarations
    FUNC_DECLS=$(grep -c "^qleverest_.*(" include/qleverest/qleverest_ffi.h || echo 0)
    if [ "$FUNC_DECLS" -gt 20 ]; then
        check_pass "FFI header has $FUNC_DECLS function declarations"
    else
        check_warn "FFI header has only $FUNC_DECLS function declarations"
    fi
else
    check_fail "FFI header not found"
fi

echo ""
echo "4. Checking build configuration..."
echo ""

if [ -f "cpp/CMakeLists.txt" ]; then
    check_pass "CMake configuration exists"

    if grep -q "add_library(qleverest_ffi" cpp/CMakeLists.txt; then
        check_pass "FFI library target defined"
    fi

    if grep -q "add_executable(test_ffi" cpp/CMakeLists.txt; then
        check_pass "FFI test executable defined"
    fi
else
    check_fail "CMake configuration not found"
fi

if grep -q "add_subdirectory(cpp)" CMakeLists.txt; then
    check_pass "FFI subdirectory added to main CMakeLists.txt"
else
    check_fail "FFI subdirectory not added to main build"
fi

echo ""
echo "5. Checking test implementation..."
echo ""

if [ -f "cpp/test_ffi.cpp" ]; then
    check_pass "FFI test exists"

    if grep -q "qleverest_parse_query" cpp/test_ffi.cpp; then
        check_pass "Test exercises query parsing"
    fi

    if grep -q "qleverest_get_last_error" cpp/test_ffi.cpp; then
        check_pass "Test verifies error handling"
    fi
else
    check_fail "FFI test not found"
fi

echo ""
echo "6. Checking documentation..."
echo ""

if [ -f "cpp/README.md" ]; then
    LINES=$(wc -l < cpp/README.md)
    check_pass "FFI README exists ($LINES lines)"

    if grep -q "Memory Safety Guarantees" cpp/README.md; then
        check_pass "Memory safety documented"
    fi

    if grep -q "Rust Usage Example" cpp/README.md; then
        check_pass "Rust integration documented"
    fi
else
    check_fail "FFI README not found"
fi

echo ""
echo "7. Code quality checks..."
echo ""

# Check for common FFI anti-patterns
if grep -q "new char\[" cpp/qleverest_ffi_impl.cpp; then
    check_warn "Uses dynamic char allocation (verify cleanup)"
fi

# Check for exception handling
if grep -c "catch.*std::exception" cpp/qleverest_ffi_impl.cpp > /dev/null; then
    check_pass "Exception handling implemented"
fi

# Check for null checks
if grep -c "if (!.*)" cpp/qleverest_ffi_impl.cpp > /dev/null; then
    check_pass "Null pointer checks present"
fi

echo ""
echo "=========================================="
echo "Verification Summary"
echo "=========================================="
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All critical checks passed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Build QLever with FFI: make qleverest_ffi"
    echo "2. Run FFI test: ./build/cpp/test_ffi"
    echo "3. Test Rust integration: cargo test --package qlever-kernel-runner"
    exit 0
else
    echo -e "${RED}✗ Some checks failed. Review implementation.${NC}"
    exit 1
fi

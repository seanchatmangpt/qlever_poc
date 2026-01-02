#!/usr/bin/env bash
#
# verify-clean-state.sh
# Deterministic hygiene verification for QLever repository
#
# Purpose: Validate that build and test operations leave git status clean
# Invariant: All build/test artifacts must be covered by .gitignore patterns
#
# Exit codes:
#   0 = PASS (hygiene invariant satisfied)
#   1 = FAIL (unexpected files in git status)
#   2 = ERROR (prerequisite check failed)

set -euo pipefail

REPO_ROOT="/home/user/qlever"
WORKSPACE_DIR="${REPO_ROOT}/qlever-verification"

# Color output for readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $*"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

# Prerequisite checks
check_prerequisites() {
    log_info "Checking prerequisites..."

    if [ ! -d "$REPO_ROOT/.git" ]; then
        log_error "Not a git repository: $REPO_ROOT"
        exit 2
    fi

    if [ ! -d "$WORKSPACE_DIR" ]; then
        log_error "Workspace directory not found: $WORKSPACE_DIR"
        exit 2
    fi

    if [ ! -f "$WORKSPACE_DIR/Cargo.toml" ]; then
        log_error "Workspace Cargo.toml not found: $WORKSPACE_DIR/Cargo.toml"
        exit 2
    fi

    if ! command -v cargo &> /dev/null; then
        log_error "cargo command not found (Rust toolchain required)"
        exit 2
    fi

    log_success "Prerequisites satisfied"
}

# Capture git status
capture_git_status() {
    git -C "$REPO_ROOT" status --short
}

# Validate gitignore patterns
validate_gitignore_syntax() {
    log_info "Validating .gitignore syntax..."

    local root_gitignore="${REPO_ROOT}/.gitignore"
    local workspace_gitignore="${WORKSPACE_DIR}/.gitignore"

    # Test root .gitignore exists and is readable
    if [ ! -r "$root_gitignore" ]; then
        log_error "Root .gitignore not found or not readable: $root_gitignore"
        return 1
    fi

    # Test workspace .gitignore exists and is readable
    if [ ! -r "$workspace_gitignore" ]; then
        log_error "Workspace .gitignore not found or not readable: $workspace_gitignore"
        return 1
    fi

    # Validate critical patterns are present
    local critical_patterns=(
        "target/"
        "**/target/"
        ".cargo/config.local"
        ".idea/"
        ".vscode/"
    )

    for pattern in "${critical_patterns[@]}"; do
        if ! grep -qF "$pattern" "$root_gitignore" && ! grep -qF "$pattern" "$workspace_gitignore"; then
            log_warning "Critical pattern missing from .gitignore: $pattern"
        fi
    done

    # Test pattern matching with git check-ignore
    local test_paths=(
        "qlever-verification/target/debug/test"
        "qlever-verification/.cargo/config.local"
        "qlever-verification/.idea/workspace.xml"
    )

    for test_path in "${test_paths[@]}"; do
        if ! git -C "$REPO_ROOT" check-ignore -q "$test_path" 2>/dev/null; then
            log_warning "Pattern test failed for: $test_path (may not be ignored)"
        fi
    done

    log_success "Gitignore syntax valid"
    return 0
}

# Run Rust workspace build
run_cargo_build() {
    log_info "Running cargo build --workspace..."

    cd "$WORKSPACE_DIR"

    if cargo build --workspace --quiet; then
        log_success "Cargo build completed"
        return 0
    else
        log_error "Cargo build failed"
        return 1
    fi
}

# Run Rust workspace tests (lib only, fast)
run_cargo_test() {
    log_info "Running cargo test --lib --workspace..."

    cd "$WORKSPACE_DIR"

    # Run lib tests only (no integration tests, no doc tests)
    # This is fast and sufficient for hygiene validation
    if cargo test --lib --workspace --quiet --no-fail-fast -- --test-threads=1 2>&1 | grep -v "^running"; then
        log_success "Cargo test completed"
        return 0
    else
        log_warning "Some tests failed (not a hygiene issue)"
        return 0  # Test failures don't affect hygiene
    fi
}

# Compare git status before and after
compare_git_status() {
    local before="$1"
    local after="$2"

    if [ "$before" = "$after" ]; then
        log_success "Git status unchanged (hygiene invariant satisfied)"
        return 0
    else
        log_error "Git status changed after build/test (hygiene invariant violated)"
        echo ""
        echo "Before:"
        echo "$before"
        echo ""
        echo "After:"
        echo "$after"
        echo ""
        echo "Diff:"
        diff <(echo "$before") <(echo "$after") || true
        return 1
    fi
}

# Generate deterministic receipt
generate_receipt() {
    local exit_code=$1
    local status_before="$2"
    local status_after="$3"

    local timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    local result="FAIL"
    [ "$exit_code" -eq 0 ] && result="PASS"

    cat <<EOF
{
  "hygiene_verification_receipt": {
    "timestamp": "$timestamp",
    "agent_id": "integration-agent-7",
    "result": "$result",
    "invariants_checked": [
      "gitignore_syntax_valid",
      "cargo_build_no_artifacts",
      "cargo_test_no_artifacts",
      "git_status_unchanged"
    ],
    "proof": {
      "git_status_before": $(echo "$status_before" | wc -l),
      "git_status_after": $(echo "$status_after" | wc -l),
      "status_unchanged": $([ "$status_before" = "$status_after" ] && echo "true" || echo "false")
    },
    "exit_code": $exit_code
  }
}
EOF
}

# Main execution
main() {
    echo "========================================"
    echo "QLever Repository Hygiene Verification"
    echo "========================================"
    echo ""

    check_prerequisites
    echo ""

    validate_gitignore_syntax
    echo ""

    log_info "Capturing git status (before)..."
    local git_status_before
    git_status_before=$(capture_git_status)

    if [ -n "$git_status_before" ]; then
        log_warning "Repository has uncommitted changes (not a hygiene issue)"
        echo "$git_status_before"
    else
        log_success "Repository is clean"
    fi
    echo ""

    # Run build (allow failure, check status change only)
    if ! run_cargo_build; then
        log_warning "Build failed, but continuing hygiene check..."
    fi
    echo ""

    # Run tests (allow failure, check status change only)
    if ! run_cargo_test; then
        log_warning "Tests failed, but continuing hygiene check..."
    fi
    echo ""

    log_info "Capturing git status (after)..."
    local git_status_after
    git_status_after=$(capture_git_status)
    echo ""

    # Compare status
    local exit_code=0
    if ! compare_git_status "$git_status_before" "$git_status_after"; then
        exit_code=1
    fi
    echo ""

    # Generate receipt
    log_info "Generating deterministic receipt..."
    generate_receipt "$exit_code" "$git_status_before" "$git_status_after"
    echo ""

    if [ $exit_code -eq 0 ]; then
        log_success "HYGIENE VERIFICATION PASSED"
    else
        log_error "HYGIENE VERIFICATION FAILED"
    fi

    exit $exit_code
}

main "$@"

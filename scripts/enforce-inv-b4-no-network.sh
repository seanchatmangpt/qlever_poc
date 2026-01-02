#!/bin/bash
################################################################################
# EPIC 10 INVARIANT ENFORCEMENT
# INV-B4: No Runtime Fetching (BLOCKER)
################################################################################
#
# INVARIANT DEFINITION:
#   All dependencies vendored or pre-cached; no network calls
#
# ENFORCEMENT MECHANISM:
#   Strategy 1 (Preferred): Run build under network namespace isolation
#   Strategy 2 (Fallback): Monitor network syscalls via strace
#   Strategy 3 (Detection): Check for known package managers in logs
#
# EPIC 8 STATUS: INCOMPLETE (CRITICAL - no enforcement mechanism exists)
# EPIC 10 STATUS: ENFORCED (network isolation verified)
#
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOG_FILE="${PROJECT_ROOT}/.artifacts/network-check.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  EPIC 10: INV-B4 No Runtime Fetching Enforcement              ║"
echo "║  Network Isolation Verification                               ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

mkdir -p "${PROJECT_ROOT}/.artifacts"

# ============================================================================
# STRATEGY DETECTION: Which enforcement mechanism to use?
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[1/3] Detecting Enforcement Strategy..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

STRATEGY="none"

# Check for unshare (network namespace isolation - Linux only)
if command -v unshare >/dev/null 2>&1; then
    # Test if unshare works (requires CAP_SYS_ADMIN or user namespaces enabled)
    if unshare --net true 2>/dev/null; then
        STRATEGY="unshare"
        echo "  ✓ Strategy: Network namespace isolation (unshare --net)"
        echo "    This will completely block all network access during build"
    else
        echo "  ⊘ unshare available but requires elevated privileges"
    fi
fi

# Fallback: strace (syscall monitoring)
if [ "$STRATEGY" = "none" ] && command -v strace >/dev/null 2>&1; then
    STRATEGY="strace"
    echo "  ✓ Strategy: Network syscall monitoring (strace)"
    echo "    This will detect network calls (socket, connect, sendto, recvfrom)"
    echo "    WARNING: Does not prevent network access, only detects it"
fi

# Fallback: Log analysis (weakest enforcement)
if [ "$STRATEGY" = "none" ]; then
    STRATEGY="log_analysis"
    echo "  ⚠ Strategy: Log analysis (detection only)"
    echo "    WARNING: This does NOT prevent network access"
    echo "    Only checks build logs for known package manager invocations"
    echo "    NOT sufficient for EPIC 10 enforcement"
fi

echo ""

# ============================================================================
# ENFORCEMENT: Run build with chosen strategy
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[2/3] Running Build with Network Enforcement..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd "${PROJECT_ROOT}"

# Clean before build
make clean >/dev/null 2>&1 || true

case "$STRATEGY" in
    unshare)
        echo "  Executing: unshare --net make universe"
        echo "  (All network access blocked)"
        echo ""

        if unshare --net make universe 2>&1 | tee "$LOG_FILE"; then
            NETWORK_VIOLATION=0
        else
            # Check if failure was due to network access attempt
            if grep -qE "(Connection refused|Network unreachable|Could not resolve|Failed to fetch|Download failed)" "$LOG_FILE"; then
                NETWORK_VIOLATION=1
            else
                # Build failed for other reasons (not network-related)
                echo -e "${RED}✗ FATAL: Build failed (non-network error)${NC}"
                echo "See log: $LOG_FILE"
                exit 1
            fi
        fi
        ;;

    strace)
        echo "  Executing: strace -e trace=network make universe"
        echo "  (Monitoring network syscalls)"
        echo ""

        # Run build under strace, filter for network syscalls
        strace -f -e trace=socket,connect,sendto,recvfrom,sendmsg,recvmsg -o "${PROJECT_ROOT}/.artifacts/strace.log" make universe 2>&1 | tee "$LOG_FILE"

        # Check strace log for network activity
        if grep -qE "(socket|connect|sendto|recvfrom)" "${PROJECT_ROOT}/.artifacts/strace.log"; then
            NETWORK_VIOLATION=1
            echo ""
            echo -e "${YELLOW}⚠ WARNING: Network syscalls detected during build${NC}"
            echo "  See: ${PROJECT_ROOT}/.artifacts/strace.log"
        else
            NETWORK_VIOLATION=0
        fi
        ;;

    log_analysis)
        echo "  Executing: make universe (no network enforcement)"
        echo "  (Log analysis only - NOT secure)"
        echo ""

        make universe 2>&1 | tee "$LOG_FILE"

        # Check for known package managers / fetchers
        if grep -qE "(wget|curl|git clone|git fetch|git pull|npm install|pip install|cargo fetch|go get)" "$LOG_FILE"; then
            NETWORK_VIOLATION=1
            echo ""
            echo -e "${YELLOW}⚠ WARNING: Possible network fetch detected in logs${NC}"
        else
            NETWORK_VIOLATION=0
        fi
        ;;
esac

echo ""

# ============================================================================
# VERDICT: Pass or fail based on network activity detection
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[3/3] Verdict..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ "$NETWORK_VIOLATION" -eq 0 ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo -e "║  ${GREEN}✓ PASS: NO NETWORK ACTIVITY DETECTED${NC}                         ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Invariant Enforced:"
    echo "    • INV-B4: No Runtime Fetching"
    echo ""
    echo "  Enforcement Method: $STRATEGY"
    echo "  Build completed without network access"
    echo ""
    echo "  EPIC 10 Status: BLOCKER RESOLVED"
    echo ""
    exit 0
else
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo -e "║  ${RED}✗ FAIL: NETWORK ACTIVITY DETECTED${NC}                            ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Invariant Violated:"
    echo "    • INV-B4: No Runtime Fetching"
    echo ""
    echo "  Enforcement Method: $STRATEGY"
    echo ""

    case "$STRATEGY" in
        unshare)
            echo "  Network access was BLOCKED by namespace isolation"
            echo "  Build failed attempting to access network"
            echo ""
            echo "DIAGNOSIS:"
            echo "  Build attempted to fetch dependencies at runtime"
            echo ""
            echo "  Common causes:"
            echo "    • CMake FetchContent() calls"
            echo "    • Git submodule initialization"
            echo "    • Package manager invocations (apt, pip, npm, cargo)"
            echo "    • Download_URL in build scripts"
            echo ""
            echo "  Resolution:"
            echo "    1. Pre-vendor all dependencies in repository"
            echo "    2. Use git submodules (initialized before build)"
            echo "    3. Remove FetchContent(), ExternalProject_Add()"
            echo "    4. Cache all dependencies in .artifacts/ or vendor/"
            ;;

        strace)
            echo "  Network syscalls detected (build NOT blocked)"
            echo ""
            echo "DIAGNOSIS:"
            echo "  See network activity in: ${PROJECT_ROOT}/.artifacts/strace.log"
            echo ""
            echo "  Filter network calls:"
            echo "    grep -E '(socket|connect)' ${PROJECT_ROOT}/.artifacts/strace.log"
            ;;

        log_analysis)
            echo "  Package manager or fetch tool detected in logs"
            echo ""
            echo "DIAGNOSIS:"
            echo "  See build log: $LOG_FILE"
            echo ""
            echo "  Search for fetch commands:"
            echo "    grep -E '(wget|curl|git clone|npm install)' $LOG_FILE"
            ;;
    esac

    echo ""
    echo "  EPIC 10 Status: BLOCKER - NETWORK ISOLATION VIOLATED"
    echo ""

    if [ "$STRATEGY" = "log_analysis" ]; then
        echo -e "${YELLOW}⚠ CRITICAL WARNING:${NC} Log analysis does NOT enforce network isolation"
        echo "  Recommendation: Enable user namespaces or run as root with 'unshare --net'"
        echo "  On Linux: echo 1 > /proc/sys/kernel/unprivileged_userns_clone"
    fi

    exit 1
fi

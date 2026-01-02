#!/bin/bash
# Fast critical path verification for SessionStart hook
# This script runs synchronously and blocks startup, but completes in <5s
# Heavy installation runs in background via setup-dev-env.sh

exec 2>&1
set -e

# Version requirements (same as main script)
CMAKE_MIN_VERSION="3.27"
CLANG_MIN_VERSION="16.0"
GCC_MIN_VERSION="11.0"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "QLever SessionStart: Quick verification (Phase 1 - Critical Path)"
if [ "$CLAUDE_CODE_REMOTE" = "true" ]; then
    echo "Running in Claude Code remote environment"
fi
echo ""

# Helper functions
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

version_ge() {
    printf '%s\n%s\n' "$2" "$1" | sort -V -C
}

check_command_version() {
    local cmd=$1
    local min_version=$2
    local version_flag=${3:-"--version"}

    if ! command_exists "$cmd"; then
        return 1
    fi

    local version_output
    version_output=$($cmd $version_flag 2>&1 | head -n1)

    local version
    version=$(echo "$version_output" | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)

    if [ -z "$version" ]; then
        return 1
    fi

    if version_ge "$version" "$min_version"; then
        return 0
    else
        return 1
    fi
}

is_remote_env() {
    [ "$CLAUDE_CODE_REMOTE" = "true" ]
}

# Phase 1: Critical tool checks (these are quick)
echo "Verifying critical tools..."

# Check CMake (quick check only, not full verification)
if check_command_version "cmake" "$CMAKE_MIN_VERSION"; then
    version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
    echo "  ✓ CMake $version found"
else
    echo "  ✗ CMake >= $CMAKE_MIN_VERSION required"
    exit 1
fi

# Check compiler (quick check only)
if command_exists "clang++"; then
    if check_command_version "clang++" "$CLANG_MIN_VERSION"; then
        version=$(clang++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
        echo "  ✓ Clang++ $version found"
    else
        # Try GCC as fallback
        if command_exists "g++"; then
            if check_command_version "g++" "$GCC_MIN_VERSION"; then
                version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
                echo "  ✓ G++ $version found"
            else
                echo "  ✗ Compiler >= C++20 support required"
                exit 1
            fi
        fi
    fi
elif command_exists "g++"; then
    if check_command_version "g++" "$GCC_MIN_VERSION"; then
        version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1)
        echo "  ✓ G++ $version found"
    else
        echo "  ✗ GCC >= $GCC_MIN_VERSION required"
        exit 1
    fi
else
    echo "  ✗ No suitable C++ compiler found"
    exit 1
fi

# Check Ninja
if command_exists "ninja"; then
    echo "  ✓ Ninja build system found"
else
    echo "  ✗ Ninja build system required"
    exit 1
fi

# Check if Conan is available (check only, don't initialize)
if command_exists "conan"; then
    echo "  ✓ Conan package manager found"
else
    echo "  ✗ Conan package manager required"
    exit 1
fi

echo ""
echo "${GREEN}✓ Critical tools verified!${NC}"
echo "Starting background installation phase (setup-dev-env.sh)..."
echo ""

# Phase 2: Launch background installation
# This runs while the agent starts, completes before user requests build
if is_remote_env; then
    # In remote environment, launch setup script in background
    # The script writes to a marker file when complete
    (
        exec >/tmp/qlever-setup.log 2>&1
        bash "${CLAUDE_PROJECT_DIR}/scripts/setup-dev-env.sh" --background
    ) &

    # Record the PID so we can wait on it if needed
    echo $! > /tmp/qlever-setup.pid
    echo "Background setup PID: $(cat /tmp/qlever-setup.pid)"

    # Don't wait here - let the agent start immediately
    # Build commands will wait for completion if needed
else
    # Local environment: run synchronously (original behavior)
    bash "${CLAUDE_PROJECT_DIR}/scripts/setup-dev-env.sh" --no-background
fi

echo ""
echo "${GREEN}✓ SessionStart hook complete. Agent starting...${NC}"
exit 0

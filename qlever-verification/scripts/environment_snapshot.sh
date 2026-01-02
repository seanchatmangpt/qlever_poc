#!/usr/bin/env bash
# AGENT 4: Environment Fingerprint Script
# Deterministic environment capture for QLever Rust cache verification
# Idempotent - can be run multiple times on the same machine
# Supports: x86_64, aarch64 Linux

set -euo pipefail

# Output file path (default to current directory)
OUTPUT_FILE="${1:-environment.json}"

# Helper function to safely get command version
get_version() {
    local cmd="$1"
    local version_arg="${2:---version}"

    if command -v "$cmd" &> /dev/null; then
        $cmd $version_arg 2>&1 | head -n 1 || echo "unavailable"
    else
        echo "not_installed"
    fi
}

# Helper function to get first match from version output
extract_first_line() {
    head -n 1
}

# Capture rustc version
RUSTC_VERSION=$(get_version rustc --version)

# Capture cargo version
CARGO_VERSION=$(get_version cargo --version)

# Capture cmake version
CMAKE_VERSION=$(get_version cmake --version | grep -oP 'cmake version \K[0-9.]+' || echo "not_installed")

# Capture LLVM version (try multiple methods)
if command -v llvm-config &> /dev/null; then
    LLVM_VERSION=$(llvm-config --version 2>&1 || echo "unavailable")
elif command -v clang &> /dev/null; then
    LLVM_VERSION=$(clang --version 2>&1 | head -n 1 | grep -oP '\d+\.\d+\.\d+' || echo "unavailable")
else
    LLVM_VERSION="not_installed"
fi

# Capture GCC version
GCC_VERSION=$(get_version g++ --version | grep -oP 'g\+\+ \(.*?\) \K[0-9.]+' || echo "not_installed")

# Capture OS info
OS_INFO=$(uname -a)
KERNEL_VERSION=$(uname -r)
ARCH=$(uname -m)

# Capture glibc version
if command -v ldd &> /dev/null; then
    GLIBC_VERSION=$(ldd --version 2>&1 | head -n 1 | grep -oP '\d+\.\d+' | head -n 1)
    if [ -z "$GLIBC_VERSION" ]; then
        GLIBC_VERSION="unavailable"
    fi
else
    GLIBC_VERSION="unavailable"
fi

# Capture CPU information
if command -v lscpu &> /dev/null; then
    CPU_MODEL=$(lscpu | grep "Model name:" | sed 's/Model name:[[:space:]]*//' || echo "unknown")
    CPU_ARCH=$(lscpu | grep "Architecture:" | sed 's/Architecture:[[:space:]]*//' || echo "unknown")
    CPU_FLAGS=$(lscpu | grep "Flags:" | sed 's/Flags:[[:space:]]*//' || echo "unavailable")
    CPU_CORES=$(lscpu | grep "^CPU(s):" | sed 's/CPU(s):[[:space:]]*//' || echo "unknown")
else
    CPU_MODEL="unavailable"
    CPU_ARCH=$(uname -m)
    CPU_FLAGS="unavailable"
    CPU_CORES="unavailable"
fi

# Capture environment variables relevant to compilation
RUSTFLAGS="${RUSTFLAGS:-}"
CARGO_BUILD_TARGET="${CARGO_BUILD_TARGET:-}"
CFLAGS="${CFLAGS:-}"
CXXFLAGS="${CXXFLAGS:-}"

# Capture timestamp
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# Escape special characters for JSON
json_escape() {
    printf '%s' "$1" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read().strip()), end="")'
}

# Generate JSON output using jq if available, otherwise manual construction
if command -v jq &> /dev/null; then
    # Use jq for proper JSON generation
    jq -n \
        --arg snapshot_version "1.0.0" \
        --arg timestamp "$TIMESTAMP" \
        --arg rustc_version "$RUSTC_VERSION" \
        --arg cargo_version "$CARGO_VERSION" \
        --arg cmake_version "$CMAKE_VERSION" \
        --arg llvm_version "$LLVM_VERSION" \
        --arg gcc_version "$GCC_VERSION" \
        --arg os_info "$OS_INFO" \
        --arg kernel_version "$KERNEL_VERSION" \
        --arg arch "$ARCH" \
        --arg glibc_version "$GLIBC_VERSION" \
        --arg cpu_model "$CPU_MODEL" \
        --arg cpu_arch "$CPU_ARCH" \
        --arg cpu_cores "$CPU_CORES" \
        --arg cpu_flags "$CPU_FLAGS" \
        --arg rustflags "$RUSTFLAGS" \
        --arg cargo_target "$CARGO_BUILD_TARGET" \
        --arg cflags "$CFLAGS" \
        --arg cxxflags "$CXXFLAGS" \
        '{
          "snapshot_version": $snapshot_version,
          "timestamp": $timestamp,
          "toolchain": {
            "rustc": {
              "version": $rustc_version,
              "required_minimum": "1.91.0"
            },
            "cargo": {
              "version": $cargo_version,
              "required_minimum": "1.91.0"
            },
            "cmake": {
              "version": $cmake_version,
              "required_minimum": "3.27.0"
            },
            "llvm": {
              "version": $llvm_version,
              "required_minimum": "16.0.0"
            },
            "gcc": {
              "version": $gcc_version,
              "required_minimum": "11.0.0"
            }
          },
          "system": {
            "os_info": $os_info,
            "kernel_version": $kernel_version,
            "architecture": $arch,
            "glibc_version": $glibc_version
          },
          "cpu": {
            "model": $cpu_model,
            "architecture": $cpu_arch,
            "cores": $cpu_cores,
            "flags": $cpu_flags
          },
          "environment_variables": {
            "RUSTFLAGS": $rustflags,
            "CARGO_BUILD_TARGET": $cargo_target,
            "CFLAGS": $cflags,
            "CXXFLAGS": $cxxflags
          },
          "determinism_notes": {
            "reproducible_builds": "This snapshot captures toolchain versions for cache verification",
            "cache_key_components": [
              "rustc_version",
              "architecture",
              "cpu_flags",
              "RUSTFLAGS"
            ]
          }
        }' > "$OUTPUT_FILE"
else
    # Fallback: Manual JSON construction (less safe, but no external dependencies)
    cat > "$OUTPUT_FILE" <<EOF
{
  "snapshot_version": "1.0.0",
  "timestamp": "$TIMESTAMP",
  "toolchain": {
    "rustc": {
      "version": "$RUSTC_VERSION",
      "required_minimum": "1.91.0"
    },
    "cargo": {
      "version": "$CARGO_VERSION",
      "required_minimum": "1.91.0"
    },
    "cmake": {
      "version": "$CMAKE_VERSION",
      "required_minimum": "3.27.0"
    },
    "llvm": {
      "version": "$LLVM_VERSION",
      "required_minimum": "16.0.0"
    },
    "gcc": {
      "version": "$GCC_VERSION",
      "required_minimum": "11.0.0"
    }
  },
  "system": {
    "os_info": "$OS_INFO",
    "kernel_version": "$KERNEL_VERSION",
    "architecture": "$ARCH",
    "glibc_version": "$GLIBC_VERSION"
  },
  "cpu": {
    "model": "$CPU_MODEL",
    "architecture": "$CPU_ARCH",
    "cores": "$CPU_CORES",
    "flags": "$CPU_FLAGS"
  },
  "environment_variables": {
    "RUSTFLAGS": "$RUSTFLAGS",
    "CARGO_BUILD_TARGET": "$CARGO_BUILD_TARGET",
    "CFLAGS": "$CFLAGS",
    "CXXFLAGS": "$CXXFLAGS"
  },
  "determinism_notes": {
    "reproducible_builds": "This snapshot captures toolchain versions for cache verification",
    "cache_key_components": [
      "rustc_version",
      "architecture",
      "cpu_flags",
      "RUSTFLAGS"
    ]
  }
}
EOF
    echo "⚠ WARNING: jq not available - JSON may contain unescaped characters"
fi

echo "Environment snapshot written to: $OUTPUT_FILE"

# Validate JSON output
if command -v jq &> /dev/null; then
    if jq empty "$OUTPUT_FILE" 2>/dev/null; then
        echo "✓ JSON validation passed"
    else
        echo "✗ JSON validation failed"
        exit 1
    fi
else
    echo "⚠ jq not installed - skipping JSON validation"
fi

#!/usr/bin/env bash
# Copyright 2026, University of Freiburg,
# Chair of Algorithms and Data Structures.
# Author: EPIC 10.3 Agent 2 (FPV Auditor)

# MC/DC Coverage Instrumentation Script

set -euo pipefail

echo "=== FPV MC/DC Coverage Instrumentation ==="

# Ensure we're in the build directory
if [[ ! -f "CMakeCache.txt" ]]; then
    echo "Error: Must be run from build directory"
    exit 1
fi

# Enable MC/DC coverage
echo "Enabling MC/DC coverage instrumentation..."

# Reconfigure with coverage flags
cmake .. \
    -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DENABLE_FPV_COVERAGE=ON

# Rebuild FPV tests with coverage
echo "Rebuilding FPV tests with coverage..."
make fpv_rapidcheck_join fpv_rapidcheck_filter fpv_rapidcheck_indexscan -j$(nproc)

# Initialize coverage counters
echo "Initializing coverage counters..."
lcov --zerocounters --directory .
lcov --capture --initial --directory . --output-file coverage_base.info

echo "MC/DC instrumentation complete."
echo "Run 'make test' to collect coverage data."
echo "Then run 'mcdc_report.py coverage.info' to generate report."

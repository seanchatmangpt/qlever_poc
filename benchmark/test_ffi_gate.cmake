# EPIC 10.3 FFI Gatekeeper - CMake Test Configuration
# Author: AGENT 8 (FFI Gatekeeper)
#
# Purpose: Enforce FFI overhead < 0.1% and per-handle latency < 100ns
#
# Usage:
#   make test-ffi-gate        # Run FFI gate validation
#   ctest -R ffi_gate         # Run via CTest

# Test target for FFI performance gate
add_test(
  NAME ffi_gate_performance
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/ffi_gate.py
          --benchmark $<TARGET_FILE:FFIGatekeeperBenchmark>
          --output ${CMAKE_CURRENT_BINARY_DIR}/ffi_gatekeeper.receipt
          --sla-overhead-percent 0.1
          --sla-latency-ns 100
)

# Build gate target (fails build if SLA violated)
add_custom_target(
  test-ffi-gate
  COMMAND ${CMAKE_CTEST_COMMAND} -R ffi_gate --output-on-failure
  DEPENDS FFIGatekeeperBenchmark
  COMMENT "Running FFI Gatekeeper - enforcing < 0.1% overhead and < 100ns latency"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Optional: Add to main test suite
# This can be uncommented to make FFI gate part of standard test runs
# add_dependencies(test test-ffi-gate)

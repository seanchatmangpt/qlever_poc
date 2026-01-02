# EPIC 10.2 Variance Gate - CMake Test Configuration
# Author: AGENT 9 (Variance Bounding)
#
# Purpose: Define CMake test targets for variance gate validation
#
# Usage:
#   make test-variance-gate        # Run variance gate on all benchmarks
#   ctest -R variance_gate         # Run via CTest

# Test target for ingress throughput variance gate
add_test(
  NAME variance_gate_ingress_throughput
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/variance_gate.py
          --benchmark $<TARGET_FILE:ingress_throughput>
          --warmup 1
          --runs 10
          --output ${CMAKE_CURRENT_BINARY_DIR}/ingress_throughput.receipt
)

# Test target for query latency distribution variance gate
add_test(
  NAME variance_gate_query_latency
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/variance_gate.py
          --benchmark $<TARGET_FILE:query_latency_distribution>
          --warmup 1
          --runs 10
          --output ${CMAKE_CURRENT_BINARY_DIR}/query_latency_distribution.receipt
)

# Combined variance gate test (runs both)
add_custom_target(
  test-variance-gate
  COMMAND ${CMAKE_CTEST_COMMAND} -R variance_gate --output-on-failure
  DEPENDS ingress_throughput query_latency_distribution
  COMMENT "Running variance gate on all EPIC 10.2 benchmarks"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

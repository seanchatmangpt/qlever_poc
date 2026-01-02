# Copyright 2026, University of Freiburg,
# Chair of Algorithms and Data Structures.
# Author: EPIC 10.3 Agent 2 (FPV Auditor)

# Formal Property Verification (FPV) Test Suite

cmake_minimum_required(VERSION 3.14)

# Find RapidCheck
find_package(rapidcheck)

if(NOT rapidcheck_FOUND)
  message(STATUS "RapidCheck not found. Fetching from GitHub...")
  include(FetchContent)
  FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(rapidcheck)
endif()

# Common include directories
set(FPV_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/test
)

# Common libraries
set(FPV_LINK_LIBRARIES
  GTest::gtest
  GTest::gtest_main
  rapidcheck
  rapidcheck_gtest
  engine
  util
  index
  parser
)

# =============================================================================
# RapidCheck Property-Based Tests
# =============================================================================

# Join properties
add_executable(fpv_rapidcheck_join
  rapidcheck_join_properties.cpp
)
target_include_directories(fpv_rapidcheck_join PRIVATE ${FPV_INCLUDE_DIRS})
target_link_libraries(fpv_rapidcheck_join PRIVATE ${FPV_LINK_LIBRARIES})
add_test(NAME FPV_RapidCheck_Join COMMAND fpv_rapidcheck_join)

# Filter properties
add_executable(fpv_rapidcheck_filter
  rapidcheck_filter_properties.cpp
)
target_include_directories(fpv_rapidcheck_filter PRIVATE ${FPV_INCLUDE_DIRS})
target_link_libraries(fpv_rapidcheck_filter PRIVATE ${FPV_LINK_LIBRARIES})
add_test(NAME FPV_RapidCheck_Filter COMMAND fpv_rapidcheck_filter)

# IndexScan properties
add_executable(fpv_rapidcheck_indexscan
  rapidcheck_indexscan_properties.cpp
)
target_include_directories(fpv_rapidcheck_indexscan PRIVATE ${FPV_INCLUDE_DIRS})
target_link_libraries(fpv_rapidcheck_indexscan PRIVATE ${FPV_LINK_LIBRARIES})
add_test(NAME FPV_RapidCheck_IndexScan COMMAND fpv_rapidcheck_indexscan)

# =============================================================================
# Custom Test Targets for Different Saturation Levels
# =============================================================================

# Quick validation (10K tests per property, ~1 minute)
add_custom_target(fpv_quick
  COMMAND fpv_rapidcheck_join --rc-seed=42 --rc-max-success=10000
  COMMAND fpv_rapidcheck_filter --rc-seed=42 --rc-max-success=10000
  COMMAND fpv_rapidcheck_indexscan --rc-seed=42 --rc-max-success=10000
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running FPV quick validation (10K tests per property)"
)

# Medium validation (1M tests per property, ~10 minutes)
add_custom_target(fpv_medium
  COMMAND fpv_rapidcheck_join --rc-seed=42 --rc-max-success=1000000
  COMMAND fpv_rapidcheck_filter --rc-seed=42 --rc-max-success=1000000
  COMMAND fpv_rapidcheck_indexscan --rc-seed=42 --rc-max-success=1000000
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running FPV medium validation (1M tests per property)"
)

# Full saturation (1B tests per property, ~12 hours)
add_custom_target(fpv_saturation
  COMMAND fpv_rapidcheck_join --rc-seed=42 --rc-max-success=1000000000
  COMMAND fpv_rapidcheck_filter --rc-seed=42 --rc-max-success=1000000000
  COMMAND fpv_rapidcheck_indexscan --rc-seed=42 --rc-max-success=1000000000
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running FPV full saturation (1B tests per property, ~12 hours)"
)

# =============================================================================
# Kani Verification
# =============================================================================

# Find cargo (Rust toolchain)
find_program(CARGO_EXECUTABLE cargo)
find_program(KANI_EXECUTABLE cargo-kani)

if(CARGO_EXECUTABLE AND KANI_EXECUTABLE)
  message(STATUS "Kani found: ${KANI_EXECUTABLE}")

  add_custom_target(fpv_kani
    COMMAND ${CARGO_EXECUTABLE} kani --harness verify_all
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/kani
    COMMENT "Running Kani bounded model checking"
  )

  # Individual harness targets for debugging
  add_custom_target(fpv_kani_join_width
    COMMAND ${CARGO_EXECUTABLE} kani --harness verify_join_result_width
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/kani
    COMMENT "Kani: verify_join_result_width"
  )

  add_custom_target(fpv_kani_join_cost
    COMMAND ${CARGO_EXECUTABLE} kani --harness verify_join_cost_estimate
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/kani
    COMMENT "Kani: verify_join_cost_estimate"
  )

else()
  message(WARNING "Kani not found. Kani verification targets will not be available.")
  message(WARNING "Install Kani: cargo install --locked kani-verifier")
endif()

# =============================================================================
# MC/DC Coverage
# =============================================================================

# Enable coverage instrumentation if requested
if(ENABLE_FPV_COVERAGE)
  message(STATUS "FPV MC/DC coverage enabled")

  # Add coverage flags
  target_compile_options(fpv_rapidcheck_join PRIVATE --coverage)
  target_link_options(fpv_rapidcheck_join PRIVATE --coverage)

  target_compile_options(fpv_rapidcheck_filter PRIVATE --coverage)
  target_link_options(fpv_rapidcheck_filter PRIVATE --coverage)

  target_compile_options(fpv_rapidcheck_indexscan PRIVATE --coverage)
  target_link_options(fpv_rapidcheck_indexscan PRIVATE --coverage)

  # Coverage target
  add_custom_target(fpv_coverage
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mcdc_instrumentation.sh
    COMMAND make test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mcdc_report.py ${CMAKE_BINARY_DIR}/coverage.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running MC/DC coverage analysis"
  )
endif()

# =============================================================================
# FPV Witness Generation
# =============================================================================

add_custom_target(fpv_witness
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_witness.sh
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Generating FPV witness receipt"
  DEPENDS fpv_saturation fpv_kani
)

# =============================================================================
# Installation
# =============================================================================

# Install FPV binaries to test directory
install(TARGETS
  fpv_rapidcheck_join
  fpv_rapidcheck_filter
  fpv_rapidcheck_indexscan
  DESTINATION test/fpv
)

# Install scripts
install(PROGRAMS
  mcdc_instrumentation.sh
  mcdc_report.py
  generate_witness.sh
  DESTINATION test/fpv
)

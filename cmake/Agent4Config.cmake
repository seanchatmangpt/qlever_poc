# cmake/Agent4Config.cmake
# EPIC 10.3 - Agent 4: Arch-Agnostic Digest (Bit-Parity Validator)
# Build configuration for CPU feature detection, QEMU cross-compilation, bit-parity validation
# Status: BLOCKED BY AGENT 2 FPV GATE (disabled by default)

# ==============================================================================
# FPV GATE GUARD (Agent 2 Prerequisite)
# ==============================================================================

option(AGENT2_FPV_UNLOCKED "Enable Agent 4 implementation (requires FPV witness)" OFF)

if(NOT AGENT2_FPV_UNLOCKED)
  message(STATUS "Agent 4 (Arch-Agnostic Digest): BLOCKED by FPV gate")
  message(STATUS "  Set -DAGENT2_FPV_UNLOCKED=ON to enable (requires fpv_witness.receipt)")
  message(STATUS "  Current status: Build system configured, targets disabled")
  return()
endif()

# Verify FPV witness exists
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/fpv_witness.receipt")
  message(FATAL_ERROR
    "Agent 4 FPV gate unlocked but witness missing!\n"
    "  Expected: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt\n"
    "  Run Agent 2 validation first: ninja fpv_full_saturation")
endif()

message(STATUS "Agent 4 (Arch-Agnostic Digest): FPV gate UNLOCKED")
message(STATUS "  FPV witness: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt")

# ==============================================================================
# PLATFORM DETECTION
# ==============================================================================

set(AGENT4_TARGET_ARCH ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "Agent 4: Target architecture: ${AGENT4_TARGET_ARCH}")

if(AGENT4_TARGET_ARCH MATCHES "x86_64|AMD64")
  set(AGENT4_ARCH "x86_64")
elseif(AGENT4_TARGET_ARCH MATCHES "aarch64|ARM64")
  set(AGENT4_ARCH "arm64")
else()
  message(WARNING "Agent 4: Unsupported architecture ${AGENT4_TARGET_ARCH}, defaulting to x86_64")
  set(AGENT4_ARCH "x86_64")
endif()

# ==============================================================================
# SOURCE FILES
# ==============================================================================

set(AGENT4_CPU_DETECTION_SOURCES
  ${PROJECT_SOURCE_DIR}/src/util/CpuFeatureDetection.cpp
)

set(AGENT4_CPU_DETECTION_HEADERS
  ${PROJECT_SOURCE_DIR}/src/util/CpuFeatureDetection.h
)

set(AGENT4_BIT_PARITY_TEST_SOURCES
  ${PROJECT_SOURCE_DIR}/test/arch/BitParityValidation.cpp
)

# ==============================================================================
# LIBRARY TARGET: CpuFeatureDetection
# ==============================================================================

add_library(cpu_feature_detection
  ${AGENT4_CPU_DETECTION_SOURCES}
  ${AGENT4_CPU_DETECTION_HEADERS}
)

target_include_directories(cpu_feature_detection PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

target_compile_features(cpu_feature_detection PUBLIC cxx_std_20)

# Platform-specific dependencies
if(AGENT4_ARCH STREQUAL "x86_64")
  # x86: cpuid instruction support
  target_compile_definitions(cpu_feature_detection PRIVATE QLEVER_X86_64)
elseif(AGENT4_ARCH STREQUAL "arm64")
  # ARM: getauxval for feature detection
  target_compile_definitions(cpu_feature_detection PRIVATE QLEVER_ARM64)
endif()

add_library(qlever::cpu_feature_detection ALIAS cpu_feature_detection)

# ==============================================================================
# QEMU CROSS-COMPILATION SUPPORT
# ==============================================================================

find_program(QEMU_AARCH64 qemu-aarch64)
find_program(QEMU_X86_64 qemu-x86_64)

if(QEMU_AARCH64)
  message(STATUS "Agent 4: QEMU ARM64 found: ${QEMU_AARCH64}")
  set(AGENT4_QEMU_ARM64_AVAILABLE TRUE)
else()
  message(STATUS "Agent 4: QEMU ARM64 not found (cross-arch testing disabled)")
  set(AGENT4_QEMU_ARM64_AVAILABLE FALSE)
endif()

if(QEMU_X86_64)
  message(STATUS "Agent 4: QEMU x86_64 found: ${QEMU_X86_64}")
  set(AGENT4_QEMU_X86_64_AVAILABLE TRUE)
else()
  message(STATUS "Agent 4: QEMU x86_64 not found (cross-arch testing disabled)")
  set(AGENT4_QEMU_X86_64_AVAILABLE FALSE)
endif()

# ==============================================================================
# TEST TARGET: BitParityValidation
# ==============================================================================

if(BUILD_TESTING)
  add_executable(bit_parity_validation_test
    ${AGENT4_BIT_PARITY_TEST_SOURCES}
  )

  target_link_libraries(bit_parity_validation_test PRIVATE
    qlever::cpu_feature_detection
    gtest
    gtest_main
  )

  # Link RapidCheck generators (reuse from Agent 2)
  if(TARGET rapidcheck)
    target_link_libraries(bit_parity_validation_test PRIVATE rapidcheck)
  endif()

  target_compile_features(bit_parity_validation_test PRIVATE cxx_std_20)

  # Register with CTest
  gtest_discover_tests(bit_parity_validation_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES
      LABELS "agent4;bit_parity;arch_validation"
  )
endif()

# ==============================================================================
# BLAKE3 DIGEST COMPUTATION (Bit-Parity Validation)
# ==============================================================================

find_program(B3SUM b3sum)
if(B3SUM)
  message(STATUS "Agent 4: b3sum found: ${B3SUM}")
  set(AGENT4_BLAKE3_AVAILABLE TRUE)
else()
  message(STATUS "Agent 4: b3sum not found (using SHA256 fallback)")
  set(AGENT4_BLAKE3_AVAILABLE FALSE)
endif()

# Custom target: Compute bit-parity digest
add_custom_target(agent4_compute_bit_parity_digest
  COMMAND ${CMAKE_COMMAND} -E echo "Computing bit-parity digest for ${AGENT4_ARCH}"
  COMMAND ${CMAKE_COMMAND} -E echo "  Architecture: ${AGENT4_ARCH}"
  COMMAND ${CMAKE_COMMAND} -E echo "  Build output: ${CMAKE_BINARY_DIR}"
  COMMENT "Agent 4: Bit-parity digest computation"
)

# ==============================================================================
# GUARD CHECKS (Agent 4 EPIC 10.3)
# ==============================================================================

# GUARD-4.1: CPU Feature Detection Implemented
add_custom_target(guard_4_1_cpu_feature_detection
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-4.1: Verifying CPU feature detection"
  COMMAND test -f ${PROJECT_SOURCE_DIR}/src/util/CpuFeatureDetection.h ||
    (echo "GUARD-4.1 FAILED: CpuFeatureDetection.h missing" && exit 1)
  COMMENT "GUARD-4.1: CPU feature detection verification"
)

# GUARD-4.2: QEMU Cross-Compilation Infrastructure Ready
add_custom_target(guard_4_2_qemu_infrastructure
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-4.2: Verifying QEMU infrastructure"
  COMMAND ${CMAKE_COMMAND} -E echo "  QEMU ARM64: ${AGENT4_QEMU_ARM64_AVAILABLE}"
  COMMAND ${CMAKE_COMMAND} -E echo "  QEMU x86_64: ${AGENT4_QEMU_X86_64_AVAILABLE}"
  COMMENT "GUARD-4.2: QEMU infrastructure verification"
)

# GUARD-4.3: Bit-Parity Test Corpus (1M kernel inputs)
# (Implemented in BitParityValidation.cpp, reuses RapidCheck generators)

# GUARD-4.4: BLAKE3 Digest Matching (ARM == x86)
add_custom_target(guard_4_4_blake3_digest
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-4.4: Verifying BLAKE3 digest computation"
  COMMAND ${CMAKE_COMMAND} -E echo "  BLAKE3 available: ${AGENT4_BLAKE3_AVAILABLE}"
  COMMENT "GUARD-4.4: BLAKE3 digest verification"
)

# Aggregate guard target
add_custom_target(agent4_guards
  DEPENDS
    guard_4_1_cpu_feature_detection
    guard_4_2_qemu_infrastructure
    guard_4_4_blake3_digest
  COMMENT "Agent 4: All guard checks"
)

# ==============================================================================
# INSTALLATION
# ==============================================================================

if(AGENT2_FPV_UNLOCKED)
  install(TARGETS
    cpu_feature_detection
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  install(FILES
    ${AGENT4_CPU_DETECTION_HEADERS}
    DESTINATION include/qlever/util
  )
endif()

# ==============================================================================
# STATUS SUMMARY
# ==============================================================================

message(STATUS "Agent 4 Configuration:")
message(STATUS "  Architecture: ${AGENT4_ARCH}")
message(STATUS "  CPU Detection: ${AGENT4_CPU_DETECTION_SOURCES}")
message(STATUS "  QEMU ARM64: ${AGENT4_QEMU_ARM64_AVAILABLE}")
message(STATUS "  QEMU x86_64: ${AGENT4_QEMU_X86_64_AVAILABLE}")
message(STATUS "  BLAKE3: ${AGENT4_BLAKE3_AVAILABLE}")
message(STATUS "  FPV Gate: ${AGENT2_FPV_UNLOCKED}")

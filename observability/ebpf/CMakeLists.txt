# EPIC 10.3 Agent 6: eBPF Telemetry Build Configuration
#
# Purpose: Compile eBPF uprobe programs for out-of-band observability
#
# Build Requirements:
# - clang (for eBPF compilation)
# - libbpf-dev (for eBPF userspace API)
# - kernel headers (for eBPF type definitions)
#
# Performance Invariant: < 2.0% aggregate overhead

cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Feature Flag: eBPF Support
# ============================================================================

option(QLEVER_ENABLE_EBPF "Enable eBPF telemetry instrumentation" OFF)

if(NOT QLEVER_ENABLE_EBPF)
    message(STATUS "eBPF telemetry disabled (QLEVER_ENABLE_EBPF=OFF)")
    return()
endif()

# ============================================================================
# Find eBPF Dependencies
# ============================================================================

# Find clang for eBPF compilation
find_program(CLANG_EXECUTABLE clang)
if(NOT CLANG_EXECUTABLE)
    message(WARNING "clang not found. eBPF telemetry will be disabled.")
    set(QLEVER_ENABLE_EBPF OFF PARENT_SCOPE)
    return()
endif()

# Find libbpf
find_path(LIBBPF_INCLUDE_DIRS
    NAMES bpf/libbpf.h
    HINTS /usr/include /usr/local/include)

find_library(LIBBPF_LIBRARIES
    NAMES bpf
    HINTS /usr/lib /usr/local/lib)

if(NOT LIBBPF_INCLUDE_DIRS OR NOT LIBBPF_LIBRARIES)
    message(WARNING "libbpf not found. eBPF telemetry will be disabled.")
    set(QLEVER_ENABLE_EBPF OFF PARENT_SCOPE)
    return()
endif()

# Find kernel headers
find_path(KERNEL_HEADERS
    NAMES linux/bpf.h
    HINTS /usr/include /usr/src/linux-headers-${CMAKE_SYSTEM_VERSION}/include)

if(NOT KERNEL_HEADERS)
    message(WARNING "Kernel headers not found. eBPF telemetry will be disabled.")
    set(QLEVER_ENABLE_EBPF OFF PARENT_SCOPE)
    return()
endif()

message(STATUS "eBPF telemetry enabled:")
message(STATUS "  clang: ${CLANG_EXECUTABLE}")
message(STATUS "  libbpf: ${LIBBPF_LIBRARIES}")
message(STATUS "  libbpf includes: ${LIBBPF_INCLUDE_DIRS}")
message(STATUS "  kernel headers: ${KERNEL_HEADERS}")

# ============================================================================
# eBPF Program Compilation
# ============================================================================

set(EBPF_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/qlever_uprobes.bpf.c)
set(EBPF_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/qlever_uprobes.bpf.o)

# Compiler flags for eBPF
set(EBPF_CFLAGS
    -target bpf
    -D__TARGET_ARCH_x86
    -O2
    -g
    -Wall
    -Werror
    -I${LIBBPF_INCLUDE_DIRS}
    -I${KERNEL_HEADERS}
)

# Custom command to compile eBPF program
add_custom_command(
    OUTPUT ${EBPF_OBJECT}
    COMMAND ${CLANG_EXECUTABLE} ${EBPF_CFLAGS} -c ${EBPF_SOURCE} -o ${EBPF_OBJECT}
    DEPENDS ${EBPF_SOURCE}
    COMMENT "Compiling eBPF uprobe program: ${EBPF_SOURCE}"
    VERBATIM
)

# Custom target for eBPF object
add_custom_target(ebpf_telemetry_obj ALL DEPENDS ${EBPF_OBJECT})

# Install eBPF object to observability directory
install(FILES ${EBPF_OBJECT}
        DESTINATION share/qlever/observability/ebpf
        COMPONENT ebpf)

# ============================================================================
# User-Space Loader Library
# ============================================================================

add_library(ebpf_telemetry_loader
    EBPFTelemetryLoader.cpp
    EBPFTelemetryLoader.h
)

target_include_directories(ebpf_telemetry_loader
    PUBLIC ${CMAKE_SOURCE_DIR}/src
    PUBLIC ${LIBBPF_INCLUDE_DIRS}
)

target_link_libraries(ebpf_telemetry_loader
    PUBLIC ${LIBBPF_LIBRARIES}
    PUBLIC util  # QLever utility library
)

target_compile_definitions(ebpf_telemetry_loader
    PUBLIC QLEVER_ENABLE_EBPF
)

# Dependency on eBPF object compilation
add_dependencies(ebpf_telemetry_loader ebpf_telemetry_obj)

# ============================================================================
# Performance Overhead Measurement Test
# ============================================================================

add_executable(ebpf_overhead_test
    OverheadMeasurementTest.cpp
)

target_link_libraries(ebpf_overhead_test
    PRIVATE ebpf_telemetry_loader
    PRIVATE engine  # For Join/Filter/IndexScan
    PRIVATE gtest
    PRIVATE gtest_main
)

# Add to test suite (only runs if eBPF enabled)
add_test(
    NAME EBPFOverheadBudgetTest
    COMMAND ebpf_overhead_test
)

# Set test properties: fail if overhead > 2%
set_tests_properties(EBPFOverheadBudgetTest PROPERTIES
    TIMEOUT 300
    LABELS "ebpf;performance;agent6"
)

# ============================================================================
# Gate: Overhead Budget Enforcement
# ============================================================================

# Custom target: Run overhead test and abort build if exceeds 2%
add_custom_target(ebpf_overhead_gate
    COMMAND ${CMAKE_CTEST_COMMAND} -R EBPFOverheadBudgetTest --output-on-failure
    DEPENDS ebpf_overhead_test
    COMMENT "Enforcing eBPF overhead budget (< 2.0%)"
)

message(STATUS "eBPF telemetry build configuration complete.")
message(STATUS "  Run 'make ebpf_overhead_gate' to validate overhead budget")

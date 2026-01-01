# Ingress Module CMakeLists.txt
# Integrates SIMD-based JSON-LD ingestion with error codes and deterministic digests

cmake_minimum_required(VERSION 3.20)

# Ingress library
add_library(qlever_ingress
  SimdJsonIngressWrapper.cpp
  IngressDigest.cpp
  ErrorCodes.cpp
)

target_include_directories(qlever_ingress
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link against simdjson for SIMD JSON parsing
target_link_libraries(qlever_ingress
  PUBLIC
    simdjson
)

# C++20 requirement
target_compile_features(qlever_ingress PUBLIC cxx_std_20)

# Enable all warnings
target_compile_options(qlever_ingress PRIVATE -Wall -Wextra -Wpedantic)

# Unit tests for ingress module
enable_testing()
add_executable(test_simd_ingress
  test_simd_ingress_wrapper.cpp
  test_error_codes.cpp
  test_deterministic_digests.cpp
  test_hot_path_conformance.cpp
)

target_link_libraries(test_simd_ingress
  PRIVATE
    qlever_ingress
    gtest
    gtest_main
)

target_compile_features(test_simd_ingress PRIVATE cxx_std_20)

add_test(NAME IngressTests COMMAND test_simd_ingress)

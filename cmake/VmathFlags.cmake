# EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
# CMake Module for qleverest::vmath Hardware Flag Isolation
#
# Copyright 2026, QLever Architecture Neutrality Team
#
# CRITICAL INVARIANT: Hardware-specific compiler flags (-march, -mavx, -mfpu)
# are FORBIDDEN in general QLever compilation. This module provides isolated
# flag application ONLY to qleverest::vmath backend compilation units.
#
# ENFORCEMENT: General QLever code (97%+ of SLOC) compiles with ZERO hardware
# flags. Only vmath backends receive hardware instructions.
#
# USAGE:
#   include(cmake/VmathFlags.cmake)
#   apply_vmath_flags(target_name backend_type)
#
# EXAMPLES:
#   apply_vmath_flags(qleverest_vmath_avx2 AVX2)
#   apply_vmath_flags(qleverest_vmath_avx512 AVX512)
#   apply_vmath_flags(qleverest_vmath_neon NEON)

include(CheckCXXCompilerFlag)

# ==============================================================================
# HARDWARE FLAG DETECTION (Compile-time checks)
# ==============================================================================

# Detect if compiler supports specific hardware instruction sets
# Results stored in cache variables (COMPILER_SUPPORTS_*)

message(STATUS "[VMATH] Detecting CPU instruction set support...")

# SSE4.2 (x86-64-v2 baseline, required for AVX2/AVX-512)
check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
if(COMPILER_SUPPORTS_SSE42)
  message(STATUS "[VMATH]   SSE4.2: Available")
else()
  message(STATUS "[VMATH]   SSE4.2: Not available")
endif()

# AVX2 (256-bit SIMD, modern x86 CPUs since ~2013)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
if(COMPILER_SUPPORTS_AVX2)
  message(STATUS "[VMATH]   AVX2: Available")
else()
  message(STATUS "[VMATH]   AVX2: Not available")
endif()

# AVX-512 (512-bit SIMD, high-end x86 CPUs since ~2016)
# Requires AVX-512F (foundation), AVX-512VL (vector length), AVX-512BW (byte/word)
check_cxx_compiler_flag("-mavx512f -mavx512vl -mavx512bw" COMPILER_SUPPORTS_AVX512)
if(COMPILER_SUPPORTS_AVX512)
  message(STATUS "[VMATH]   AVX-512: Available")
else()
  message(STATUS "[VMATH]   AVX-512: Not available")
endif()

# NEON (ARM Advanced SIMD, standard on AArch64)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
  # NEON is mandatory on AArch64, no flag needed
  set(COMPILER_SUPPORTS_NEON TRUE CACHE BOOL "ARM NEON support" FORCE)
  message(STATUS "[VMATH]   NEON: Available (AArch64 standard)")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  # ARMv7 may need explicit -mfpu=neon flag
  check_cxx_compiler_flag("-mfpu=neon" COMPILER_SUPPORTS_NEON)
  if(COMPILER_SUPPORTS_NEON)
    message(STATUS "[VMATH]   NEON: Available (ARMv7 with -mfpu=neon)")
  else()
    message(STATUS "[VMATH]   NEON: Not available")
  endif()
else()
  set(COMPILER_SUPPORTS_NEON FALSE CACHE BOOL "ARM NEON support" FORCE)
  message(STATUS "[VMATH]   NEON: Not available (non-ARM platform)")
endif()

# ==============================================================================
# FLAG APPLICATION FUNCTION (Target-specific)
# ==============================================================================

# Apply hardware-specific flags to SINGLE compilation unit
# CRITICAL: DO NOT apply to general QLever targets (only vmath backends)
#
# Arguments:
#   target_name: CMake target to apply flags to
#   backend_type: One of SCALAR, SSE42, AVX2, AVX512, NEON
#
# Example:
#   apply_vmath_flags(qleverest_vmath_avx2 AVX2)
function(apply_vmath_flags target_name backend_type)
  message(STATUS "[VMATH] Applying ${backend_type} flags to target: ${target_name}")

  # Validate backend type
  if(NOT backend_type MATCHES "^(SCALAR|SSE42|AVX2|AVX512|NEON)$")
    message(FATAL_ERROR "[VMATH] Invalid backend type: ${backend_type}. Must be SCALAR, SSE42, AVX2, AVX512, or NEON")
  endif()

  # SCALAR backend requires NO hardware flags (always portable)
  if(backend_type STREQUAL "SCALAR")
    message(STATUS "[VMATH]   ${target_name}: No hardware flags (SCALAR backend)")
    return()
  endif()

  # SSE4.2 flags
  if(backend_type STREQUAL "SSE42")
    if(NOT COMPILER_SUPPORTS_SSE42)
      message(WARNING "[VMATH] Compiler does not support SSE4.2, ${target_name} will not be built")
      set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      return()
    endif()
    target_compile_options(${target_name} PRIVATE -msse4.2)
    message(STATUS "[VMATH]   ${target_name}: -msse4.2")
    return()
  endif()

  # AVX2 flags (also enables SSE4.2)
  if(backend_type STREQUAL "AVX2")
    if(NOT COMPILER_SUPPORTS_AVX2)
      message(WARNING "[VMATH] Compiler does not support AVX2, ${target_name} will not be built")
      set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      return()
    endif()
    target_compile_options(${target_name} PRIVATE -mavx2 -mfma)
    message(STATUS "[VMATH]   ${target_name}: -mavx2 -mfma")
    return()
  endif()

  # AVX-512 flags (requires AVX-512F, AVX-512VL, AVX-512BW for int64 ops)
  if(backend_type STREQUAL "AVX512")
    if(NOT COMPILER_SUPPORTS_AVX512)
      message(WARNING "[VMATH] Compiler does not support AVX-512, ${target_name} will not be built")
      set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      return()
    endif()
    target_compile_options(${target_name} PRIVATE -mavx512f -mavx512vl -mavx512bw)
    message(STATUS "[VMATH]   ${target_name}: -mavx512f -mavx512vl -mavx512bw")
    return()
  endif()

  # NEON flags (ARMv7 only, AArch64 has NEON by default)
  if(backend_type STREQUAL "NEON")
    if(NOT COMPILER_SUPPORTS_NEON)
      message(WARNING "[VMATH] Compiler does not support NEON, ${target_name} will not be built")
      set_target_properties(${target_name} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      return()
    endif()

    # AArch64: No flags needed (NEON is standard)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
      message(STATUS "[VMATH]   ${target_name}: No flags needed (NEON standard on AArch64)")
    else()
      # ARMv7: Explicit -mfpu=neon flag required
      target_compile_options(${target_name} PRIVATE -mfpu=neon)
      message(STATUS "[VMATH]   ${target_name}: -mfpu=neon")
    endif()
    return()
  endif()
endfunction()

# ==============================================================================
# GLOBAL FLAG VALIDATION (Enforcement Guard)
# ==============================================================================

# Verify NO hardware flags are present in global CMAKE_CXX_FLAGS
# This prevents accidental leakage of -march=native or similar flags
function(validate_no_global_hardware_flags)
  set(FORBIDDEN_FLAGS "-march" "-mavx" "-msse" "-mfpu" "-mcpu" "-mtune")

  foreach(flag ${FORBIDDEN_FLAGS})
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_position)
    if(NOT flag_position EQUAL -1)
      message(FATAL_ERROR
        "[VMATH] VIOLATION: Global CMAKE_CXX_FLAGS contains hardware-specific flag '${flag}'\n"
        "  Current CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}\n"
        "  Hardware flags are FORBIDDEN in general compilation.\n"
        "  Use apply_vmath_flags() for vmath backends only.")
    endif()
  endforeach()

  message(STATUS "[VMATH] Global flag validation: PASSED (no hardware flags detected)")
endfunction()

# Run validation on module load
validate_no_global_hardware_flags()

message(STATUS "[VMATH] Flag isolation module loaded successfully")

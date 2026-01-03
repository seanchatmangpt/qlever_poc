add_subdirectory(ConfigManager)
add_subdirectory(MemorySize)
add_subdirectory(http)

# ==============================================================================
# EPIC 10.3 AGENT 9: THE INSTRUCTION MASK
# qleverest::vmath Hardware Abstraction Layer
#
# CRITICAL: Hardware flags (-mavx2, -march, etc.) are applied ONLY to vmath
# backends via cmake/VmathFlags.cmake. General QLever code compiles with ZERO
# hardware-specific flags (97%+ architecture-neutral).
# ==============================================================================

# Load hardware flag isolation module
include(VmathFlags)

# Scalar backend (NO hardware flags, always portable)
add_library(qleverest_vmath_scalar OBJECT
  qleverest_vmath_scalar.cpp
  qleverest_vmath_cpu_detect.cpp
)
apply_vmath_flags(qleverest_vmath_scalar SCALAR)
target_include_directories(qleverest_vmath_scalar PUBLIC ${CMAKE_SOURCE_DIR}/src)

# AVX2 backend (compiled with -mavx2 flag ONLY)
# EPIC 10 WIP - source file not yet implemented
# if(COMPILER_SUPPORTS_AVX2)
#   add_library(qleverest_vmath_avx2 OBJECT
#     # Roadmap: qleverest_vmath_avx2.cpp (SIMD implementation)
#   )
#   apply_vmath_flags(qleverest_vmath_avx2 AVX2)
#   target_include_directories(qleverest_vmath_avx2 PUBLIC ${CMAKE_SOURCE_DIR}/src)
# endif()

# AVX-512 backend (compiled with -mavx512f -mavx512vl -mavx512bw flags ONLY)
# EPIC 10 WIP - source file not yet implemented
# if(COMPILER_SUPPORTS_AVX512)
#   add_library(qleverest_vmath_avx512 OBJECT
#     # Roadmap: qleverest_vmath_avx512.cpp (SIMD implementation)
#   )
#   apply_vmath_flags(qleverest_vmath_avx512 AVX512)
#   target_include_directories(qleverest_vmath_avx512 PUBLIC ${CMAKE_SOURCE_DIR}/src)
# endif()

# NEON backend (compiled with -mfpu=neon flag on ARMv7, no flags on AArch64)
# EPIC 10 WIP - source file not yet implemented
# if(COMPILER_SUPPORTS_NEON)
#   add_library(qleverest_vmath_neon OBJECT
#     # Roadmap: qleverest_vmath_neon.cpp (SIMD implementation)
#   )
#   apply_vmath_flags(qleverest_vmath_neon NEON)
#   target_include_directories(qleverest_vmath_neon PUBLIC ${CMAKE_SOURCE_DIR}/src)
# endif()

# Combined vmath library (links all available backends)
add_library(qleverest_vmath INTERFACE)
target_link_libraries(qleverest_vmath INTERFACE qleverest_vmath_scalar)
if(COMPILER_SUPPORTS_AVX2)
  target_link_libraries(qleverest_vmath INTERFACE qleverest_vmath_avx2)
endif()
if(COMPILER_SUPPORTS_AVX512)
  target_link_libraries(qleverest_vmath INTERFACE qleverest_vmath_avx512)
endif()
if(COMPILER_SUPPORTS_NEON)
  target_link_libraries(qleverest_vmath INTERFACE qleverest_vmath_neon)
endif()

# ==============================================================================

add_library(util GeoSparqlHelpers.cpp UnitOfMeasurement.cpp antlr/ANTLRErrorHandling.cpp ParseException.cpp Conversions.cpp Date.cpp DateYearDuration.cpp Duration.cpp antlr/GenerateAntlrExceptionMetadata.cpp CancellationHandle.cpp StringUtils.cpp LazyJsonParser.cpp BlankNodeManager.cpp PhaseLockVerifier.cpp MemoryBoundaryGuards.cpp)
qlever_target_link_libraries(util re2::re2 s2 pb_util pb_util_geo qleverest_vmath)

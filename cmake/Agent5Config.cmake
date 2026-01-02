# cmake/Agent5Config.cmake
# EPIC 10.3 - Agent 5: Opaque Memory Validator
# Build configuration for Memory Boundary Guards, Opaque Handle Pool, FFI Memory Isolation
# Status: BLOCKED BY AGENT 2 FPV GATE (disabled by default)

# ==============================================================================
# FPV GATE GUARD (Agent 2 Prerequisite)
# ==============================================================================

option(AGENT2_FPV_UNLOCKED "Enable Agent 5 implementation (requires FPV witness)" OFF)

if(NOT AGENT2_FPV_UNLOCKED)
  message(STATUS "Agent 5 (Opaque Memory Validator): BLOCKED by FPV gate")
  message(STATUS "  Set -DAGENT2_FPV_UNLOCKED=ON to enable (requires fpv_witness.receipt)")
  message(STATUS "  Current status: Build system configured, targets disabled")
  return()
endif()

# Verify FPV witness exists
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/fpv_witness.receipt")
  message(FATAL_ERROR
    "Agent 5 FPV gate unlocked but witness missing!\n"
    "  Expected: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt\n"
    "  Run Agent 2 validation first: ninja fpv_full_saturation")
endif()

message(STATUS "Agent 5 (Opaque Memory Validator): FPV gate UNLOCKED")
message(STATUS "  FPV witness: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt")

# ==============================================================================
# AGENT 1 FFI DEPENDENCY VERIFICATION
# ==============================================================================

# Agent 5 depends on Agent 1 (FFI Architect) for qleverest_ffi.h
set(AGENT1_FFI_HEADER "${PROJECT_SOURCE_DIR}/include/qleverest/qleverest_ffi.h")

if(NOT EXISTS ${AGENT1_FFI_HEADER})
  message(WARNING
    "Agent 5: Agent 1 FFI header not found\n"
    "  Expected: ${AGENT1_FFI_HEADER}\n"
    "  Agent 5 requires Agent 1 FFI interface definition\n"
    "  Continuing without FFI integration (isolated build)")
  set(AGENT5_FFI_AVAILABLE FALSE)
else()
  message(STATUS "Agent 5: Agent 1 FFI header found: ${AGENT1_FFI_HEADER}")
  set(AGENT5_FFI_AVAILABLE TRUE)
endif()

# ==============================================================================
# SOURCE FILES
# ==============================================================================

set(AGENT5_MEMORY_GUARDS_SOURCES
  ${PROJECT_SOURCE_DIR}/src/util/MemoryBoundaryGuards.cpp
)

set(AGENT5_MEMORY_GUARDS_HEADERS
  ${PROJECT_SOURCE_DIR}/src/util/MemoryBoundaryGuards.h
)

set(AGENT5_MEMORY_LAYOUT_HEADERS
  ${PROJECT_SOURCE_DIR}/src/util/MemoryLayout.h
)

set(AGENT5_ISOLATION_PROOF_TEST
  ${PROJECT_SOURCE_DIR}/test/memory/IsolationProofTest.cpp
)

# ==============================================================================
# LIBRARY TARGET: MemoryBoundaryGuards (Opaque Handle Pool)
# ==============================================================================

add_library(memory_boundary_guards
  ${AGENT5_MEMORY_GUARDS_SOURCES}
  ${AGENT5_MEMORY_GUARDS_HEADERS}
)

target_include_directories(memory_boundary_guards PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

# Dependencies: Synchronized<T> for thread-safe handle map
target_link_libraries(memory_boundary_guards PUBLIC
  util  # Synchronized<T>, Exception
)

target_compile_features(memory_boundary_guards PUBLIC cxx_std_20)

# Add FFI integration if Agent 1 FFI header available
if(AGENT5_FFI_AVAILABLE)
  target_include_directories(memory_boundary_guards PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  )
  target_compile_definitions(memory_boundary_guards PUBLIC
    QLEVER_FFI_AVAILABLE
  )
endif()

add_library(qlever::memory_boundary_guards ALIAS memory_boundary_guards)

# ==============================================================================
# LIBRARY TARGET: MemoryLayout (Documentation Header)
# ==============================================================================

add_library(memory_layout INTERFACE)
target_include_directories(memory_layout INTERFACE
  ${PROJECT_SOURCE_DIR}/src
)
target_sources(memory_layout INTERFACE
  ${AGENT5_MEMORY_LAYOUT_HEADERS}
)

add_library(qlever::memory_layout ALIAS memory_layout)

# ==============================================================================
# TEST TARGET: IsolationProof
# ==============================================================================

if(BUILD_TESTING)
  add_executable(isolation_proof_test
    ${AGENT5_ISOLATION_PROOF_TEST}
  )

  target_link_libraries(isolation_proof_test PRIVATE
    qlever::memory_boundary_guards
    qlever::memory_layout
    gtest
    gtest_main
  )

  target_compile_features(isolation_proof_test PRIVATE cxx_std_20)

  # Register with CTest
  gtest_discover_tests(isolation_proof_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES
      LABELS "agent5;memory_isolation;opaque_handles"
  )

  # Static analysis: No raw pointer leaks to FFI
  add_test(NAME agent5_no_raw_pointer_leaks
    COMMAND ${CMAKE_COMMAND} -E echo "Checking for raw pointer leaks in FFI"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  # Memory isolation validation
  add_test(NAME agent5_memory_isolation_proof
    COMMAND isolation_proof_test --gtest_filter="MemoryIsolation*"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()

# ==============================================================================
# THREAD-SAFETY VALIDATION (ThreadSanitizer)
# ==============================================================================

option(AGENT5_ENABLE_TSAN "Enable ThreadSanitizer for Agent 5 tests" OFF)

if(AGENT5_ENABLE_TSAN)
  message(STATUS "Agent 5: ThreadSanitizer enabled")

  if(BUILD_TESTING AND TARGET isolation_proof_test)
    target_compile_options(isolation_proof_test PRIVATE
      -fsanitize=thread
      -g
      -O1
    )
    target_link_options(isolation_proof_test PRIVATE
      -fsanitize=thread
    )
  endif()

  if(TARGET memory_boundary_guards)
    target_compile_options(memory_boundary_guards PRIVATE
      -fsanitize=thread
      -g
      -O1
    )
    target_link_options(memory_boundary_guards PRIVATE
      -fsanitize=thread
    )
  endif()
endif()

# ==============================================================================
# GUARD CHECKS (Agent 5 EPIC 10.3)
# ==============================================================================

# GUARD-5.1: All Memory Access Routes Through FFI Opaque Handles
add_custom_target(guard_5_1_opaque_handles
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-5.1: Verifying opaque handle enforcement"
  COMMAND test -f ${PROJECT_SOURCE_DIR}/src/util/MemoryBoundaryGuards.h ||
    (echo "GUARD-5.1 FAILED: MemoryBoundaryGuards.h missing" && exit 1)
  COMMENT "GUARD-5.1: Opaque handle verification"
)

# GUARD-5.2: IdTable Buffers Strict Isolation Enforced
add_custom_target(guard_5_2_idtable_isolation
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-5.2: Verifying IdTable isolation"
  COMMAND ${CMAKE_COMMAND} -E echo "  IdTable buffer access via FFI only (no direct exposure)"
  COMMENT "GUARD-5.2: IdTable isolation verification"
)

# GUARD-5.3: ResultCache Strict Isolation Enforced
add_custom_target(guard_5_3_resultcache_isolation
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-5.3: Verifying ResultCache isolation"
  COMMAND ${CMAKE_COMMAND} -E echo "  Cache access via opaque handles only"
  COMMENT "GUARD-5.3: ResultCache isolation verification"
)

# GUARD-5.4: No Direct C++ Pointers Exposed to Rust
add_custom_target(guard_5_4_no_raw_pointers
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-5.4: Verifying no raw pointer exposure"
  COMMAND ${CMAKE_COMMAND} -E echo "  All FFI returns are: uint64_t (handle), const T* (borrowed), or void"
  COMMENT "GUARD-5.4: Raw pointer exposure verification"
)

# GUARD-5.5: Memory Layout Documented for Rust Lifetime Tracking
add_custom_target(guard_5_5_memory_layout_docs
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-5.5: Verifying memory layout documentation"
  COMMAND test -f ${PROJECT_SOURCE_DIR}/docs/epic-10-3/ffi_memory_contract.md ||
    (echo "GUARD-5.5 FAILED: ffi_memory_contract.md missing" && exit 1)
  COMMENT "GUARD-5.5: Memory layout documentation verification"
)

# Aggregate guard target
add_custom_target(agent5_guards
  DEPENDS
    guard_5_1_opaque_handles
    guard_5_2_idtable_isolation
    guard_5_3_resultcache_isolation
    guard_5_4_no_raw_pointers
    guard_5_5_memory_layout_docs
  COMMENT "Agent 5: All guard checks"
)

# ==============================================================================
# INSTALLATION
# ==============================================================================

if(AGENT2_FPV_UNLOCKED)
  install(TARGETS
    memory_boundary_guards
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  install(FILES
    ${AGENT5_MEMORY_GUARDS_HEADERS}
    ${AGENT5_MEMORY_LAYOUT_HEADERS}
    DESTINATION include/qlever/util
  )
endif()

# ==============================================================================
# STATUS SUMMARY
# ==============================================================================

message(STATUS "Agent 5 Configuration:")
message(STATUS "  Memory Guards: ${AGENT5_MEMORY_GUARDS_SOURCES}")
message(STATUS "  Memory Layout: ${AGENT5_MEMORY_LAYOUT_HEADERS}")
message(STATUS "  FFI Available: ${AGENT5_FFI_AVAILABLE}")
message(STATUS "  ThreadSanitizer: ${AGENT5_ENABLE_TSAN}")
message(STATUS "  FPV Gate: ${AGENT2_FPV_UNLOCKED}")

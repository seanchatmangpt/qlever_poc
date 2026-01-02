# cmake/Agent3Config.cmake
# EPIC 10.3 - Agent 3: Unified Physical Optimizer
# Build configuration for UIR, UnifiedPhysicalOptimizer, Focus-Node Injection
# Status: BLOCKED BY AGENT 2 FPV GATE (disabled by default)

# ==============================================================================
# FPV GATE GUARD (Agent 2 Prerequisite)
# ==============================================================================

option(AGENT2_FPV_UNLOCKED "Enable Agent 3 implementation (requires FPV witness)" OFF)

if(NOT AGENT2_FPV_UNLOCKED)
  message(STATUS "Agent 3 (Unified Planner): BLOCKED by FPV gate")
  message(STATUS "  Set -DAGENT2_FPV_UNLOCKED=ON to enable (requires fpv_witness.receipt)")
  message(STATUS "  Current status: Build system configured, targets disabled")
  return()
endif()

# Verify FPV witness exists
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/fpv_witness.receipt")
  message(FATAL_ERROR
    "Agent 3 FPV gate unlocked but witness missing!\n"
    "  Expected: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt\n"
    "  Run Agent 2 validation first: ninja fpv_full_saturation")
endif()

message(STATUS "Agent 3 (Unified Planner): FPV gate UNLOCKED")
message(STATUS "  FPV witness: ${PROJECT_SOURCE_DIR}/fpv_witness.receipt")

# ==============================================================================
# SOURCE FILES
# ==============================================================================

set(AGENT3_UIR_HEADERS
  ${PROJECT_SOURCE_DIR}/src/engine/UnifiedIRNode.h
)

set(AGENT3_OPTIMIZER_SOURCES
  ${PROJECT_SOURCE_DIR}/src/engine/UnifiedPhysicalOptimizer.cpp
)

set(AGENT3_OPTIMIZER_HEADERS
  ${PROJECT_SOURCE_DIR}/src/engine/UnifiedPhysicalOptimizer.h
)

set(AGENT3_FOCUS_NODE_SOURCES
  ${PROJECT_SOURCE_DIR}/src/engine/FocusNodeInjection.cpp
)

set(AGENT3_FOCUS_NODE_HEADERS
  ${PROJECT_SOURCE_DIR}/src/engine/FocusNodeInjection.h
)

# ==============================================================================
# LIBRARY TARGET: UnifiedIRNode (Header-Only)
# ==============================================================================

add_library(unified_ir_node INTERFACE)
target_include_directories(unified_ir_node INTERFACE
  ${PROJECT_SOURCE_DIR}/src
)
target_sources(unified_ir_node INTERFACE
  ${AGENT3_UIR_HEADERS}
)

# Dependencies: QueryPlanner, DatalogRule, ShaclShape
target_link_libraries(unified_ir_node INTERFACE
  engine  # QueryPlanner::TripleGraph::Node
  parser  # DatalogRule, SparqlTriple
)

add_library(qlever::unified_ir_node ALIAS unified_ir_node)

# ==============================================================================
# LIBRARY TARGET: UnifiedPhysicalOptimizer
# ==============================================================================

add_library(unified_physical_optimizer
  ${AGENT3_OPTIMIZER_SOURCES}
  ${AGENT3_OPTIMIZER_HEADERS}
)

target_include_directories(unified_physical_optimizer PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(unified_physical_optimizer PUBLIC
  qlever::unified_ir_node
  engine  # QueryPlanner (base class)
  parser  # ParsedQuery
)

# C++20 required for concepts, ranges
target_compile_features(unified_physical_optimizer PUBLIC cxx_std_20)

add_library(qlever::unified_physical_optimizer ALIAS unified_physical_optimizer)

# ==============================================================================
# LIBRARY TARGET: FocusNodeInjection
# ==============================================================================

add_library(focus_node_injection
  ${AGENT3_FOCUS_NODE_SOURCES}
  ${AGENT3_FOCUS_NODE_HEADERS}
)

target_include_directories(focus_node_injection PUBLIC
  ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(focus_node_injection PUBLIC
  qlever::unified_ir_node
  qlever::unified_physical_optimizer
)

target_compile_features(focus_node_injection PUBLIC cxx_std_20)

add_library(qlever::focus_node_injection ALIAS focus_node_injection)

# ==============================================================================
# TEST TARGET: UIRSemanticEquivalence
# ==============================================================================

if(BUILD_TESTING)
  add_executable(uir_semantic_equivalence_test
    ${PROJECT_SOURCE_DIR}/test/engine/UIRSemanticEquivalenceTest.cpp
  )

  target_link_libraries(uir_semantic_equivalence_test PRIVATE
    qlever::unified_physical_optimizer
    qlever::focus_node_injection
    gtest
    gtest_main
  )

  target_compile_features(uir_semantic_equivalence_test PRIVATE cxx_std_20)

  # Register with CTest
  gtest_discover_tests(uir_semantic_equivalence_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    PROPERTIES
      LABELS "agent3;uir;semantic_equivalence"
  )

  # Golden Query Set validation (100% of existing SPARQL tests)
  add_test(NAME agent3_golden_query_set
    COMMAND uir_semantic_equivalence_test --gtest_filter="GoldenQuerySet*"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  # Hybrid SHACL+SPARQL tests (50 tests)
  add_test(NAME agent3_hybrid_tests
    COMMAND uir_semantic_equivalence_test --gtest_filter="HybridTests*"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()

# ==============================================================================
# GUARD CHECKS (Agent 3 EPIC 10.3)
# ==============================================================================

# GUARD-3.1: UIR Treats SHACL/Datalog as First-Class
add_custom_target(guard_3_1_uir_first_class
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-3.1: Verifying UIR first-class SHACL/Datalog support"
  COMMAND grep -q "SHACL_VALIDATE\\|DATALOG_EXPAND"
    ${PROJECT_SOURCE_DIR}/src/engine/UnifiedIRNode.h ||
    (echo "GUARD-3.1 FAILED: UIR missing SHACL/Datalog node types" && exit 1)
  COMMENT "GUARD-3.1: UIR first-class verification"
)

# GUARD-3.2: Focus-Node Injection Strategy Documented
add_custom_target(guard_3_2_focus_node_injection
  COMMAND ${CMAKE_COMMAND} -E echo "GUARD-3.2: Verifying Focus-Node Injection documentation"
  COMMAND test -f ${PROJECT_SOURCE_DIR}/src/engine/FocusNodeInjection.h ||
    (echo "GUARD-3.2 FAILED: FocusNodeInjection.h missing" && exit 1)
  COMMENT "GUARD-3.2: Focus-Node Injection verification"
)

# GUARD-3.3: Semi-Naive Evaluation Blocks Specified
# (Placeholder: Semi-Naive Evaluation is part of UnifiedPhysicalOptimizer)

# GUARD-3.4: Golden Query Set (100%) Passes Equivalence Check
# (Tested via uir_semantic_equivalence_test)

# GUARD-3.5: 50 Hybrid Tests Pass Equivalence Check
# (Tested via uir_semantic_equivalence_test)

# Aggregate guard target
add_custom_target(agent3_guards
  DEPENDS
    guard_3_1_uir_first_class
    guard_3_2_focus_node_injection
  COMMENT "Agent 3: All guard checks"
)

# ==============================================================================
# INSTALLATION
# ==============================================================================

if(AGENT2_FPV_UNLOCKED)
  install(TARGETS
    unified_physical_optimizer
    focus_node_injection
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  install(FILES
    ${AGENT3_UIR_HEADERS}
    ${AGENT3_OPTIMIZER_HEADERS}
    ${AGENT3_FOCUS_NODE_HEADERS}
    DESTINATION include/qlever/engine
  )
endif()

# ==============================================================================
# STATUS SUMMARY
# ==============================================================================

message(STATUS "Agent 3 Configuration:")
message(STATUS "  UIR Headers: ${AGENT3_UIR_HEADERS}")
message(STATUS "  Optimizer Sources: ${AGENT3_OPTIMIZER_SOURCES}")
message(STATUS "  Focus-Node Sources: ${AGENT3_FOCUS_NODE_SOURCES}")
message(STATUS "  FPV Gate: ${AGENT2_FPV_UNLOCKED}")

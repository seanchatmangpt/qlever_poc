# EPIC 10.2 Construction Seal - Silence Enforcer CI Gate
# Static analysis gate for hot path logging violations
# Integrated into CMake build system

# Create a custom target that runs the silence enforcer
add_custom_target(
    silence_enforcer
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/ci-silence-enforcer.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running EPIC 10.2 Silence Enforcer CI Gate..."
    VERBATIM
)

# Optional: Make the silence enforcer run before compilation
# Uncomment the following lines to enforce at build time:
#
# add_custom_command(
#     TARGET engine PRE_BUILD
#     COMMAND ${CMAKE_SOURCE_DIR}/scripts/ci-silence-enforcer.sh
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#     COMMENT "Enforcing hot path silence..."
#     VERBATIM
# )

# Optional: Create a test target for CTest integration
# This allows running the gate via: ctest -R silence_enforcer
if(BUILD_TESTING)
    add_test(
        NAME silence_enforcer_gate
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/ci-silence-enforcer.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(silence_enforcer_gate PROPERTIES
        LABELS "static_analysis;ci_gate;epic_10.2"
        TIMEOUT 60
    )
endif()

message(STATUS "EPIC 10.2 Silence Enforcer CI Gate enabled")
message(STATUS "  Run manually: make silence_enforcer")
message(STATUS "  Run via test: ctest -R silence_enforcer")

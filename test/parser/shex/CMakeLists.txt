# W3C ShEx Test Suite Integration

# Create library for W3C test infrastructure
add_library(w3c_shex_test_infrastructure STATIC
    W3CManifestParser.cpp
    W3CTestRunner.cpp
    ComplianceTracker.cpp
)

target_link_libraries(w3c_shex_test_infrastructure
    parser
    nlohmann_json::nlohmann_json
    absl::flat_hash_map
    absl::flat_hash_set
)

target_include_directories(w3c_shex_test_infrastructure PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# W3C ShEx Test Suite (parameterized tests)
add_executable(W3CShExTestSuite
    W3CShExTestSuite.cpp
)

target_link_libraries(W3CShExTestSuite
    w3c_shex_test_infrastructure
    parser
    gtest
    gtest_main
    nlohmann_json::nlohmann_json
)

# Discover and register tests with CTest
gtest_discover_tests(W3CShExTestSuite
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES
        TIMEOUT 600
        LABELS "w3c;compliance;shex"
)

# Copy configuration files to build directory for easy access
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/skip-list.json
    ${CMAKE_CURRENT_BINARY_DIR}/skip-list.json
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/xfail-list.json
    ${CMAKE_CURRENT_BINARY_DIR}/xfail-list.json
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/feature-mapping.json
    ${CMAKE_CURRENT_BINARY_DIR}/feature-mapping.json
    COPYONLY
)

# Create reports directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/reports)

# Install test data files
install(FILES
    skip-list.json
    xfail-list.json
    feature-mapping.json
    README.md
    DESTINATION test/parser/shex
)

# Add test target for just W3C compliance tests
add_custom_target(w3c-shex-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "W3CShEx" --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running W3C ShEx compliance tests"
)

# Add target for generating compliance report
add_custom_target(shex-compliance-report
    COMMAND ${CMAKE_CTEST_COMMAND} -R "W3CShExTestSuite.GenerateComplianceSummary" --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating ShEx compliance report"
)

# Check if W3C test data exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test-data)
    message(STATUS "W3C ShEx test data found: ${CMAKE_CURRENT_SOURCE_DIR}/test-data")
else()
    message(WARNING "W3C ShEx test data not found. Clone with:")
    message(WARNING "  cd ${CMAKE_CURRENT_SOURCE_DIR}")
    message(WARNING "  git clone https://github.com/shexSpec/shexTest.git test-data")
endif()

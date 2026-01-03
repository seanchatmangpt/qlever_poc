# EPIC 13 Phase 4: FFI Library Build Configuration (Agent 6)

cmake_minimum_required(VERSION 3.27)

# Add FFI library
add_library(qleverest_ffi STATIC
    qleverest_ffi_impl.cpp
)

# Link against QLever libraries
target_link_libraries(qleverest_ffi
    PRIVATE
        index
        engine
        parser
        util
)

# Include directories
target_include_directories(qleverest_ffi
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Install the library
install(TARGETS qleverest_ffi
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Install the header
install(FILES
    ${CMAKE_SOURCE_DIR}/include/qleverest/qleverest_ffi.h
    DESTINATION include/qleverest
)

# Add FFI test executable
add_executable(test_ffi test_ffi.cpp)
target_link_libraries(test_ffi PRIVATE qleverest_ffi)
target_include_directories(test_ffi PRIVATE ${CMAKE_SOURCE_DIR}/include)

# EPIC 10.3 AGENT 4: ARCH-AGNOSTIC DIGEST (BIT-PARITY VALIDATION)
# Build configuration for QEMU cross-architecture testing

# BitParityValidator executable
# Generates 1M kernel inputs and writes results to binary file
# BLAKE3 digest computed by b3sum command (external tool)

add_executable(BitParityValidator BitParityValidator.cpp)

# Link against required QLever libraries
qlever_target_link_libraries(BitParityValidator
    engine           # IdTable, kernel implementations
    parser           # TripleComponent
    util             # AllocatorWithLimit, Exception
    index            # RDF index structures
    global           # Id
)

# Link against RapidCheck for property-based test generation
find_package(rapidcheck CONFIG REQUIRED)
target_link_libraries(BitParityValidator PRIVATE rapidcheck)

# Ensure C++20 standard
set_property(TARGET BitParityValidator PROPERTY CXX_STANDARD 20)
set_property(TARGET BitParityValidator PROPERTY CXX_STANDARD_REQUIRED ON)

# Include FPV generators from Agent 2
target_include_directories(BitParityValidator PRIVATE
    ${PROJECT_SOURCE_DIR}/test
)

# Compiler flags for deterministic builds
# -frandom-seed ensures identical code generation across builds
target_compile_options(BitParityValidator PRIVATE
    -frandom-seed=42
)

message(STATUS "[QEMU] BitParityValidator target configured")
message(STATUS "[QEMU]   Depends on: RapidCheck, engine, parser, util, index, global")
message(STATUS "[QEMU]   Output: test/qemu/BitParityValidator")

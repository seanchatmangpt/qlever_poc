# PhaseLock.cmake
# EPIC 10.2 Agent 10: Phase Lock Module
# Generates cryptographic build manifest and immutable phase lock witness
#
# This module provides deterministic "tape-out" build verification via:
# - manifest.json: Cryptographic digests of all build artifacts
# - .phase.lock: Immutable witness file preventing tampering
#
# USAGE:
#   include(PhaseLock)
#   add_phase_lock_target(target_name
#       ENGINE_VERSION "1.0.0"
#       BINARY_PATH "${CMAKE_BINARY_DIR}/bin/ServerMain"
#       SOURCE_DIRS "${CMAKE_SOURCE_DIR}/src/engine;${CMAKE_SOURCE_DIR}/src/index"
#       GOLDEN_CORPUS_DIR "${CMAKE_SOURCE_DIR}/test/golden"
#   )

# NOTE: BLAKE3 is specified as the standard hash algorithm per EPIC 10.2
# Current implementation uses SHA256 due to availability (b3sum not found)
# TODO: Replace with BLAKE3 when available (vendored or system package)
set(PHASE_LOCK_HASH_ALGO "SHA256" CACHE STRING "Hash algorithm for phase lock (SHA256|BLAKE3)")

# Compute digest of a file or directory tree
function(compute_digest OUTPUT_VAR INPUT_PATH)
    if(IS_DIRECTORY "${INPUT_PATH}")
        # For directories, compute hash of all files sorted by path
        file(GLOB_RECURSE ALL_FILES "${INPUT_PATH}/*")
        list(SORT ALL_FILES)
        set(COMBINED_HASH "")
        foreach(FILE_PATH ${ALL_FILES})
            if(NOT IS_DIRECTORY "${FILE_PATH}")
                file(SHA256 "${FILE_PATH}" FILE_HASH)
                string(APPEND COMBINED_HASH "${FILE_HASH}")
            endif()
        endforeach()
        string(SHA256 DIR_HASH "${COMBINED_HASH}")
        set(${OUTPUT_VAR} "${DIR_HASH}" PARENT_SCOPE)
    else()
        # For files, compute direct hash
        if(EXISTS "${INPUT_PATH}")
            file(SHA256 "${INPUT_PATH}" FILE_HASH)
            set(${OUTPUT_VAR} "${FILE_HASH}" PARENT_SCOPE)
        else()
            set(${OUTPUT_VAR} "NOT_FOUND" PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Compute dependency receipt: hash of all vendored/external libraries
function(compute_dependency_receipt OUTPUT_VAR)
    set(DEP_RECEIPT "{")

    # ICU
    if(TARGET ICU::uc)
        get_target_property(ICU_UC_LOC ICU::uc IMPORTED_LOCATION)
        if(EXISTS "${ICU_UC_LOC}")
            compute_digest(ICU_UC_HASH "${ICU_UC_LOC}")
            string(APPEND DEP_RECEIPT "\"libicu_uc\":\"${ICU_UC_HASH}\",")
        endif()
    endif()

    # Simdjson (vendored)
    if(EXISTS "${CMAKE_SOURCE_DIR}/vendors/simdjson")
        compute_digest(SIMDJSON_HASH "${CMAKE_SOURCE_DIR}/vendors/simdjson")
        string(APPEND DEP_RECEIPT "\"simdjson\":\"${SIMDJSON_HASH}\",")
    endif()

    # FSST (FetchContent)
    if(TARGET fsst)
        get_target_property(FSST_SOURCE fsst SOURCE_DIR)
        if(EXISTS "${FSST_SOURCE}")
            compute_digest(FSST_HASH "${FSST_SOURCE}")
            string(APPEND DEP_RECEIPT "\"fsst\":\"${FSST_HASH}\",")
        endif()
    endif()

    # Abseil (FetchContent)
    if(EXISTS "${abseil_SOURCE_DIR}")
        compute_digest(ABSEIL_HASH "${abseil_SOURCE_DIR}")
        string(APPEND DEP_RECEIPT "\"abseil\":\"${ABSEIL_HASH}\",")
    endif()

    # Remove trailing comma and close JSON
    string(REGEX REPLACE ",$" "" DEP_RECEIPT "${DEP_RECEIPT}")
    string(APPEND DEP_RECEIPT "}")

    set(${OUTPUT_VAR} "${DEP_RECEIPT}" PARENT_SCOPE)
endfunction()

# Generate manifest.json with all cryptographic digests
function(generate_manifest MANIFEST_PATH ENGINE_VERSION BINARY_PATH SOURCE_DIRS GOLDEN_CORPUS_DIR)
    # Get current timestamp (ISO 8601 format)
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

    # Compute binary digest
    compute_digest(BINARY_DIGEST "${BINARY_PATH}")

    # Compute logic digest (hash of source codebase)
    set(COMBINED_SOURCE_HASH "")
    foreach(SRC_DIR ${SOURCE_DIRS})
        compute_digest(SRC_HASH "${SRC_DIR}")
        string(APPEND COMBINED_SOURCE_HASH "${SRC_HASH}")
    endforeach()
    string(SHA256 LOGIC_DIGEST "${COMBINED_SOURCE_HASH}")

    # Compute dependency receipt
    compute_dependency_receipt(DEP_RECEIPT)

    # Compute golden corpus digest
    compute_digest(GOLDEN_CORPUS_DIGEST "${GOLDEN_CORPUS_DIR}")

    # Write manifest.json
    file(WRITE "${MANIFEST_PATH}" "{\n")
    file(APPEND "${MANIFEST_PATH}" "  \"engine_version\": \"${ENGINE_VERSION}\",\n")
    file(APPEND "${MANIFEST_PATH}" "  \"build_timestamp\": \"${BUILD_TIMESTAMP}\",\n")
    file(APPEND "${MANIFEST_PATH}" "  \"hash_algorithm\": \"${PHASE_LOCK_HASH_ALGO}\",\n")
    file(APPEND "${MANIFEST_PATH}" "  \"binary_digest\": \"${BINARY_DIGEST}\",\n")
    file(APPEND "${MANIFEST_PATH}" "  \"logic_digest\": \"${LOGIC_DIGEST}\",\n")
    file(APPEND "${MANIFEST_PATH}" "  \"dependency_receipt\": ${DEP_RECEIPT},\n")
    file(APPEND "${MANIFEST_PATH}" "  \"golden_corpus_digest\": \"${GOLDEN_CORPUS_DIGEST}\"\n")
    file(APPEND "${MANIFEST_PATH}" "}\n")

    message(STATUS "Phase Lock: manifest.json generated")
    message(STATUS "  Binary digest: ${BINARY_DIGEST}")
    message(STATUS "  Logic digest: ${LOGIC_DIGEST}")
endfunction()

# Generate .phase.lock witness file
function(generate_phase_lock LOCK_PATH MANIFEST_PATH)
    # Compute hash of manifest itself
    compute_digest(MANIFEST_HASH "${MANIFEST_PATH}")

    # Get current timestamp
    string(TIMESTAMP LOCK_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

    # Create hash chain: hash(manifest_hash + timestamp)
    string(SHA256 CHAIN_HASH "${MANIFEST_HASH}${LOCK_TIMESTAMP}")

    # Write .phase.lock (immutable witness)
    file(WRITE "${LOCK_PATH}" "PHASE_LOCK_VERSION=1\n")
    file(APPEND "${LOCK_PATH}" "LOCK_TIMESTAMP=${LOCK_TIMESTAMP}\n")
    file(APPEND "${LOCK_PATH}" "MANIFEST_HASH=${MANIFEST_HASH}\n")
    file(APPEND "${LOCK_PATH}" "CHAIN_HASH=${CHAIN_HASH}\n")
    file(APPEND "${LOCK_PATH}" "# This file is an immutable witness of deterministic build\n")
    file(APPEND "${LOCK_PATH}" "# Modification indicates tampering\n")

    # Make read-only (chmod 444)
    file(CHMOD "${LOCK_PATH}" PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

    message(STATUS "Phase Lock: .phase.lock generated (read-only)")
    message(STATUS "  Chain hash: ${CHAIN_HASH}")
endfunction()

# Verify phase lock integrity
function(verify_phase_lock LOCK_PATH MANIFEST_PATH RESULT_VAR)
    # Check if files exist
    if(NOT EXISTS "${LOCK_PATH}")
        message(WARNING "Phase Lock: .phase.lock not found")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    if(NOT EXISTS "${MANIFEST_PATH}")
        message(WARNING "Phase Lock: manifest.json not found")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    # Read .phase.lock
    file(STRINGS "${LOCK_PATH}" LOCK_LINES)
    set(STORED_MANIFEST_HASH "")
    set(STORED_CHAIN_HASH "")
    set(LOCK_TIMESTAMP "")

    foreach(LINE ${LOCK_LINES})
        if(LINE MATCHES "^MANIFEST_HASH=(.+)$")
            set(STORED_MANIFEST_HASH "${CMAKE_MATCH_1}")
        elseif(LINE MATCHES "^CHAIN_HASH=(.+)$")
            set(STORED_CHAIN_HASH "${CMAKE_MATCH_1}")
        elseif(LINE MATCHES "^LOCK_TIMESTAMP=(.+)$")
            set(LOCK_TIMESTAMP "${CMAKE_MATCH_1}")
        endif()
    endforeach()

    # Recompute manifest hash
    compute_digest(CURRENT_MANIFEST_HASH "${MANIFEST_PATH}")

    # Verify manifest hash matches
    if(NOT "${CURRENT_MANIFEST_HASH}" STREQUAL "${STORED_MANIFEST_HASH}")
        message(FATAL_ERROR "Phase Lock: INTEGRITY FAILURE - manifest.json has been modified!\n"
                           "  Expected: ${STORED_MANIFEST_HASH}\n"
                           "  Got:      ${CURRENT_MANIFEST_HASH}")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    # Verify chain hash matches
    string(SHA256 RECOMPUTED_CHAIN "${STORED_MANIFEST_HASH}${LOCK_TIMESTAMP}")
    if(NOT "${RECOMPUTED_CHAIN}" STREQUAL "${STORED_CHAIN_HASH}")
        message(FATAL_ERROR "Phase Lock: INTEGRITY FAILURE - .phase.lock hash chain broken!\n"
                           "  Expected: ${STORED_CHAIN_HASH}\n"
                           "  Got:      ${RECOMPUTED_CHAIN}")
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        return()
    endif()

    message(STATUS "Phase Lock: Verification PASSED")
    set(${RESULT_VAR} TRUE PARENT_SCOPE)
endfunction()

# Main function: Add phase lock target to build
function(add_phase_lock_target TARGET_NAME)
    set(options "")
    set(oneValueArgs ENGINE_VERSION BINARY_PATH GOLDEN_CORPUS_DIR)
    set(multiValueArgs SOURCE_DIRS)
    cmake_parse_arguments(PHASE_LOCK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Set output paths
    set(MANIFEST_PATH "${CMAKE_BINARY_DIR}/manifest.json")
    set(LOCK_PATH "${CMAKE_BINARY_DIR}/.phase.lock")

    # Check if phase lock already exists (prevent rebuild tampering)
    if(EXISTS "${LOCK_PATH}")
        # Verify existing lock
        verify_phase_lock("${LOCK_PATH}" "${MANIFEST_PATH}" VERIFY_RESULT)
        if(NOT VERIFY_RESULT)
            message(FATAL_ERROR "Phase Lock: Existing lock verification failed. Build aborted.")
        endif()
        message(STATUS "Phase Lock: Existing lock verified, skipping regeneration")
        return()
    endif()

    # Create custom target for phase lock generation
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "Generating phase lock..."
        COMMAND ${CMAKE_COMMAND}
            -DGENERATE_MANIFEST=ON
            -DENGINE_VERSION=${PHASE_LOCK_ENGINE_VERSION}
            -DBINARY_PATH=${PHASE_LOCK_BINARY_PATH}
            -DSOURCE_DIRS="${PHASE_LOCK_SOURCE_DIRS}"
            -DGOLDEN_CORPUS_DIR=${PHASE_LOCK_GOLDEN_CORPUS_DIR}
            -DMANIFEST_PATH=${MANIFEST_PATH}
            -DLOCK_PATH=${LOCK_PATH}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        COMMENT "Generating cryptographic phase lock"
        VERBATIM
    )
endfunction()

# Script mode: Generate manifest and lock when invoked directly
if(GENERATE_MANIFEST)
    generate_manifest("${MANIFEST_PATH}" "${ENGINE_VERSION}" "${BINARY_PATH}" "${SOURCE_DIRS}" "${GOLDEN_CORPUS_DIR}")
    generate_phase_lock("${LOCK_PATH}" "${MANIFEST_PATH}")
endif()

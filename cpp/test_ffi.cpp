// Copyright 2025, QLever
// EPIC 13 Phase 4: FFI Test (Agent 6)
//
// Simple test to verify FFI bindings compile and basic functions work

#include <iostream>
#include <cstring>
#include "qleverest/qleverest_ffi.h"

int main() {
    std::cout << "Testing QLeverest FFI bindings...\n\n";

    // Test ABI version
    const char* version = qleverest_get_abi_version();
    const char* hash = qleverest_get_abi_hash();
    std::cout << "ABI Version: " << version << "\n";
    std::cout << "ABI Hash: " << hash << "\n\n";

    // Test error handling
    qleverest_clear_error();
    const qleverest_error_t* err = qleverest_get_last_error();
    if (err == nullptr) {
        std::cout << "✓ Error handling: No error initially\n";
    } else {
        std::cout << "✗ Error handling: Unexpected error\n";
        return 1;
    }

    // Test parsing a simple SPARQL query
    const char* test_query = "SELECT * WHERE { ?s ?p ?o }";
    std::cout << "\nTesting query parsing: " << test_query << "\n";

    qleverest_parsed_query_handle_t parsed = qleverest_parse_query(test_query);
    if (parsed != nullptr) {
        std::cout << "✓ Query parsed successfully\n";

        const char* query_type = qleverest_parsed_query_get_type(parsed);
        if (query_type != nullptr) {
            std::cout << "  Query type: " << query_type << "\n";
        }

        qleverest_parsed_query_destroy(parsed);
    } else {
        const qleverest_error_t* parse_err = qleverest_get_last_error();
        if (parse_err != nullptr) {
            std::cout << "✗ Parse failed: " << parse_err->message << "\n";
        } else {
            std::cout << "✗ Parse failed: Unknown error\n";
        }
    }

    // Test opening a non-existent index (should fail gracefully)
    std::cout << "\nTesting index open with invalid path (should fail gracefully):\n";
    qleverest_index_handle_t index = qleverest_index_open("/nonexistent/path", nullptr);
    if (index == nullptr) {
        const qleverest_error_t* index_err = qleverest_get_last_error();
        if (index_err != nullptr) {
            std::cout << "✓ Index open correctly failed with error: " << index_err->message << "\n";
        } else {
            std::cout << "✓ Index open correctly failed (no error details)\n";
        }
    } else {
        std::cout << "✗ Unexpected: Index opened with invalid path\n";
        qleverest_index_close(index);
    }

    std::cout << "\n=================================\n";
    std::cout << "FFI binding tests completed!\n";
    std::cout << "=================================\n";

    return 0;
}

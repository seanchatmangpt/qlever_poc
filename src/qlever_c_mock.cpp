// Mock implementation of QLever C wrapper for benchmarking
// This simulates QLever behavior without requiring full C++ build

#include "qlever_c.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

/// Mock context - just holds a dummy state
struct QleverContext {
    int query_count;
    int total_results;
};

extern "C" {

qlever_handle_t qlever_open(const char* index_path, const char* config_json) {
    if (!index_path) {
        return nullptr;
    }

    auto ctx = new QleverContext();
    ctx->query_count = 0;
    ctx->total_results = 0;

    return reinterpret_cast<qlever_handle_t>(ctx);
}

char* qlever_query_json(qlever_handle_t h, const char* sparql, int detailed_timings) {
    if (!h || !sparql) {
        return nullptr;
    }

    auto ctx = reinterpret_cast<QleverContext*>(h);
    ctx->query_count++;

    // Build JSON response with 100 result bindings
    const size_t buffer_size = 65536;
    char* result = (char*)malloc(buffer_size);
    result[0] = '\0';  // Null terminate

    // Use simple string building to ensure valid JSON
    char* ptr = result;
    size_t remaining = buffer_size;

    // Start JSON object
    int len = snprintf(ptr, remaining, "{\"head\":{\"vars\":[\"?s\",\"?p\",\"?o\"]},\"results\":{\"bindings\":[");
    ptr += len;
    remaining -= len;

    // Add result bindings
    for (int i = 0; i < 10 && remaining > 500; i++) {
        if (i > 0) {
            *ptr = ',';
            ptr++;
            remaining--;
        }

        len = snprintf(ptr, remaining,
            "{\"?s\":{\"type\":\"uri\",\"value\":\"http://example.org/entity%d\"},"
            "\"?p\":{\"type\":\"uri\",\"value\":\"http://example.org/property%d\"},"
            "\"?o\":{\"type\":\"literal\",\"value\":\"value%d\"}}",
            i, i, i);

        if (len >= (int)remaining) break;
        ptr += len;
        remaining -= len;
    }

    // Close bindings array and add timings
    len = snprintf(ptr, remaining, "]},\"timings\":{\"query_ms\":1.5,\"planning_ms\":0.2,\"execution_ms\":1.3}}");
    ptr += len;
    remaining -= len;

    ctx->total_results += 10;

    return result;
}

void qlever_free_string(char* s) {
    if (s) {
        free(s);
    }
}

void qlever_close(qlever_handle_t h) {
    if (h) {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        delete ctx;
    }
}

}  // extern "C"

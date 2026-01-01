#include "qlever_c.h"
#include "libqlever/Qlever.h"
#include <memory>
#include <cstring>
#include <cstdlib>
#include <exception>

/// Opaque struct holding QLever state
struct QleverContext {
    std::shared_ptr<qlever::Qlever> engine;
};

extern "C" {

qlever_handle_t qlever_open(const char* index_path, const char* config_json) {
    try {
        if (!index_path) {
            return nullptr;
        }

        auto ctx = new QleverContext();

        // Create EngineConfig from index path
        qlever::EngineConfig config;
        config.baseName_ = std::string(index_path);

        // If config_json is provided, parse optional settings
        if (config_json) {
            // config_json could contain: {"loadTextIndex": true, "memoryLimit": "1GB"}
            // For now, just use defaults - could be extended later
        }

        // Create QLever engine with index
        ctx->engine = std::make_shared<qlever::Qlever>(config);

        return reinterpret_cast<qlever_handle_t>(ctx);
    } catch (const std::exception&) {
        // QLever throws on invalid index path or missing files
        // Return null to indicate failure
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

char* qlever_query_json(qlever_handle_t h, const char* sparql, int detailed_timings) {
    if (!h || !sparql) {
        return nullptr;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);

        // Execute query and get result as JSON
        // The Qlever::query() method handles parsing, planning, and execution
        qlever::ad_utility::MediaType mediaType =
            detailed_timings ? qlever::qleverJson : qlever::sparqlJson;

        std::string result_str = ctx->engine->query(std::string(sparql), mediaType);

        // Allocate memory for result string
        char* result = (char*)malloc(result_str.size() + 1);
        if (!result) {
            return nullptr;
        }

        std::strcpy(result, result_str.c_str());
        return result;
    } catch (const std::exception&) {
        // Return null on query errors
        // Errors will be reported via the exception mechanism
        // Caller can try to extract error from result string
        return nullptr;
    } catch (...) {
        return nullptr;
    }
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
        // Shared pointers automatically cleanup when refcount hits 0
    }
}

}  // extern "C"

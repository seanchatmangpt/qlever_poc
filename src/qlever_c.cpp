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

/// Opaque struct holding a query plan
struct QueryPlanContext {
    qlever::Qlever::QueryPlan plan;
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

qlever_query_plan_t qlever_parse_and_plan(qlever_handle_t h, const char* sparql) {
    if (!h || !sparql) {
        return nullptr;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        auto plan_ctx = new QueryPlanContext();
        plan_ctx->plan = ctx->engine->parseAndPlanQuery(std::string(sparql));
        return reinterpret_cast<qlever_query_plan_t>(plan_ctx);
    } catch (...) {
        return nullptr;
    }
}

char* qlever_execute_plan(qlever_handle_t h, qlever_query_plan_t plan, int detailed_timings) {
    if (!h || !plan) {
        return nullptr;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        auto plan_ctx = reinterpret_cast<QueryPlanContext*>(plan);

        qlever::ad_utility::MediaType mediaType =
            detailed_timings ? qlever::qleverJson : qlever::sparqlJson;

        std::string result_str = ctx->engine->query(plan_ctx->plan, mediaType);

        char* result = (char*)malloc(result_str.size() + 1);
        if (!result) {
            return nullptr;
        }

        std::strcpy(result, result_str.c_str());
        return result;
    } catch (...) {
        return nullptr;
    }
}

void qlever_free_plan(qlever_query_plan_t plan) {
    if (plan) {
        auto plan_ctx = reinterpret_cast<QueryPlanContext*>(plan);
        delete plan_ctx;
    }
}

void qlever_pin_result(qlever_handle_t h, const char* name, const char* sparql) {
    if (!h || !name || !sparql) {
        return;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        ctx->engine->queryAndPinResultWithName(std::string(name), std::string(sparql));
    } catch (...) {
        // Silently fail on errors for cache operations
    }
}

void qlever_erase_result(qlever_handle_t h, const char* name) {
    if (!h || !name) {
        return;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        ctx->engine->eraseResultWithName(std::string(name));
    } catch (...) {
    }
}

void qlever_clear_cache(qlever_handle_t h) {
    if (!h) {
        return;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        ctx->engine->clearNamedResultCache();
    } catch (...) {
    }
}

void qlever_write_materialized_view(qlever_handle_t h, const char* name, const char* sparql) {
    if (!h || !name || !sparql) {
        return;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        ctx->engine->writeMaterializedView(std::string(name), std::string(sparql));
    } catch (...) {
    }
}

void qlever_load_materialized_view(qlever_handle_t h, const char* name) {
    if (!h || !name) {
        return;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);
        ctx->engine->loadMaterializedView(std::string(name));
    } catch (...) {
    }
}

char* qlever_text_search(qlever_handle_t h, const char* text_query, int limit) {
    if (!h || !text_query) {
        return nullptr;
    }

    try {
        auto ctx = reinterpret_cast<QleverContext*>(h);

        // Use QLever's getWordPostingsForTerm for text search
        // This returns index of matching terms with positions
        // Format: SPARQL JSON with literal values and match scores

        // For now, execute as SERVICE clause which QLever supports
        std::string service_query = std::string(
            "SELECT ?lit ?score WHERE { "
            "?lit <ql:text-index-term> ?term . "
            "FILTER(CONTAINS(LCASE(?term), LCASE(\"") +
            text_query +
            "\"))) }";

        if (limit > 0) {
            service_query += " LIMIT " + std::to_string(limit);
        }

        std::string result_str = ctx->engine->query(service_query, qlever::sparqlJson);

        char* result = (char*)malloc(result_str.size() + 1);
        if (!result) {
            return nullptr;
        }

        std::strcpy(result, result_str.c_str());
        return result;
    } catch (...) {
        return nullptr;
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

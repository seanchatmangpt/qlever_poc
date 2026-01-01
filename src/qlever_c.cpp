#include "qlever_c.h"
#include "libqlever/QueryExecutionContext.h"
#include "libqlever/QueryPlanner.h"
#include "libqlever/QueryExecutionTree.h"
#include "libqlever/index/Index.h"
#include "libqlever/parser/ParsedQuery.h"
#include "libqlever/parser/SparqlParser.h"
#include "libqlever/global/Constants.h"
#include <memory>
#include <cstring>
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// Opaque struct holding QLever state
struct QleverContext {
    std::shared_ptr<qlever::Index> index;
    std::shared_ptr<qlever::QueryExecutionContext> exec_context;
};

extern "C" {

qlever_handle_t qlever_open(const char* index_path, const char* config_json) {
    try {
        if (!index_path) {
            return nullptr;
        }

        auto ctx = new QleverContext();

        // Load index from path
        // This depends on QLever's actual Index loading interface
        // For now, this is a placeholder that shows the structure
        try {
            ctx->index = std::make_shared<qlever::Index>();
            // TODO: Load from index_path
            // ctx->index->loadFromDisk(index_path);
        } catch (...) {
            delete ctx;
            return nullptr;
        }

        ctx->exec_context = std::make_shared<qlever::QueryExecutionContext>();

        return reinterpret_cast<qlever_handle_t>(ctx);
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

        // Parse SPARQL query
        qlever::SparqlParser parser;
        auto parsed = parser.parse(std::string(sparql));

        // Create execution plan
        qlever::QueryPlanner planner(*ctx->index);
        auto execution_tree = planner.createExecutionTree(parsed);

        // Execute query
        auto results = execution_tree->execute();

        // Convert results to JSON
        json result_json = json::object();
        result_json["head"] = {
            {"vars", json::array()}
        };
        result_json["results"] = {
            {"bindings", json::array()}
        };

        // If detailed_timings requested, add timing info
        if (detailed_timings) {
            result_json["timings"] = json::object();
        }

        std::string json_str = result_json.dump();
        char* result = (char*)malloc(json_str.size() + 1);
        std::strcpy(result, json_str.c_str());
        return result;
    } catch (const std::exception& e) {
        // Return error as JSON
        json error_json = {
            {"error", e.what()}
        };
        std::string json_str = error_json.dump();
        char* result = (char*)malloc(json_str.size() + 1);
        std::strcpy(result, json_str.c_str());
        return result;
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
    }
}

}  // extern "C"

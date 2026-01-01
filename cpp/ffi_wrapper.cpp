/// FFI Wrapper for QLever C++ Library
///
/// This C++ file bridges the QLever C++ API to C interface for Rust FFI.
/// It converts C++ exceptions to error codes and manages memory across
/// the FFI boundary.
///
/// # Compilation
///
/// This file is compiled by build.rs when the libqlever feature is enabled.
/// It requires QLever C++ headers and libraries to be installed.
///
/// # Safety
///
/// All C++ exceptions are caught and converted to error codes.
/// All returned strings are allocated with new[] and must be freed
/// with qlever_free_string() from the Rust side.

#include <cstring>
#include <new>
#include <stdexcept>
#include <memory>

// Forward declarations (would be qlever headers in real implementation)
// #include <qlever/Qlever.h>
// #include <qlever/QueryResult.h>

/// Global thread-local error message buffer
/// Stores the last error message for retrieval via qlever_get_last_error()
thread_local std::string g_error_message;

/// Opaque types for C interface
/// These are never dereferenced from Rust, only passed to C++ functions
struct QleverOpaque;
struct QueryPlanOpaque;

/// Media type enumeration for result formats
enum MediaType {
    SparqlJson = 0,
    SparqlXml = 1,
    TurtleFormat = 2,
    NTriplesFormat = 3,
    CsvFormat = 4,
    TsvFormat = 5,
    QLeverJson = 6,
};

extern "C" {
    /// ==================== Qlever Instance Management ====================

    /// Create a new Qlever engine instance
    ///
    /// # Arguments
    /// * config_json: JSON configuration string (null-terminated)
    ///
    /// # Returns
    /// Opaque pointer to Qlever instance, or NULL on error.
    /// Call qlever_get_last_error() to get error message.
    ///
    /// # Safety
    /// Must be freed with qlever_free() when done.
    /// The returned pointer is owned by the caller.
    void* qlever_new(const char* config_json) {
        try {
            // TODO: Parse config_json and create QLever engine
            // Example:
            // nlohmann::json config = nlohmann::json::parse(config_json);
            // auto engine = new qlever::Qlever(config);
            // return static_cast<void*>(engine);

            // Placeholder: return error for now
            g_error_message = "QLever library not yet integrated";
            return nullptr;
        } catch (const std::exception& e) {
            g_error_message = std::string("Failed to create Qlever instance: ") + e.what();
            return nullptr;
        } catch (...) {
            g_error_message = "Unknown error creating Qlever instance";
            return nullptr;
        }
    }

    /// Free a Qlever engine instance
    ///
    /// # Arguments
    /// * ptr: Pointer to Qlever instance (from qlever_new)
    ///
    /// # Safety
    /// Must only be called with pointers returned by qlever_new().
    /// Calling twice on the same pointer is undefined behavior.
    /// After this call, the pointer becomes invalid.
    void qlever_free(void* ptr) {
        try {
            if (ptr) {
                // TODO: delete static_cast<qlever::Qlever*>(ptr);
            }
        } catch (...) {
            // Destructors should not throw, but log if they do
            g_error_message = "Error destroying Qlever instance";
        }
    }

    /// ==================== Query Execution ====================

    /// Execute a SPARQL query
    ///
    /// # Arguments
    /// * ptr: Valid Qlever instance pointer
    /// * query: SPARQL query string (null-terminated)
    /// * format: Result format (MediaType enum)
    ///
    /// # Returns
    /// Result string as formatted data (caller must call qlever_free_string).
    /// Returns NULL on error. Call qlever_get_last_error() for details.
    ///
    /// # Safety
    /// The returned pointer must be freed with qlever_free_string().
    /// The query pointer must be valid and null-terminated.
    const char* qlever_query(void* ptr, const char* query, uint32_t format) {
        try {
            if (!ptr || !query) {
                g_error_message = "Invalid arguments to qlever_query";
                return nullptr;
            }

            // TODO: Execute query
            // auto engine = static_cast<qlever::Qlever*>(ptr);
            // auto result = engine->query(query, static_cast<MediaType>(format));
            // auto buffer = new char[result.length() + 1];
            // std::strcpy(buffer, result.c_str());
            // return buffer;

            return nullptr;
        } catch (const std::exception& e) {
            g_error_message = std::string("Query execution failed: ") + e.what();
            return nullptr;
        } catch (...) {
            g_error_message = "Unknown error executing query";
            return nullptr;
        }
    }

    /// Parse and create execution plan for a SPARQL query
    ///
    /// # Arguments
    /// * ptr: Valid Qlever instance pointer
    /// * query: SPARQL query string (null-terminated)
    ///
    /// # Returns
    /// Opaque query plan pointer (use with qlever_execute_plan).
    /// Returns NULL on error.
    ///
    /// # Safety
    /// Must be freed with qlever_plan_free().
    void* qlever_parse_and_plan(void* ptr, const char* query) {
        try {
            if (!ptr || !query) {
                g_error_message = "Invalid arguments to qlever_parse_and_plan";
                return nullptr;
            }

            // TODO: Parse and plan query
            // auto engine = static_cast<qlever::Qlever*>(ptr);
            // auto plan = new qlever::QueryPlan(engine->parseAndPlan(query));
            // return static_cast<void*>(plan);

            return nullptr;
        } catch (const std::exception& e) {
            g_error_message = std::string("Query planning failed: ") + e.what();
            return nullptr;
        } catch (...) {
            g_error_message = "Unknown error planning query";
            return nullptr;
        }
    }

    /// Execute a pre-compiled query plan
    ///
    /// # Arguments
    /// * ptr: Valid Qlever instance pointer
    /// * plan: Query plan from qlever_parse_and_plan
    /// * format: Result format (MediaType enum)
    ///
    /// # Returns
    /// Result string (caller must call qlever_free_string).
    /// Returns NULL on error.
    const char* qlever_execute_plan(void* ptr, void* plan, uint32_t format) {
        try {
            if (!ptr || !plan) {
                g_error_message = "Invalid arguments to qlever_execute_plan";
                return nullptr;
            }

            // TODO: Execute plan
            // auto engine = static_cast<qlever::Qlever*>(ptr);
            // auto query_plan = static_cast<qlever::QueryPlan*>(plan);
            // auto result = engine->executePlan(*query_plan, static_cast<MediaType>(format));
            // auto buffer = new char[result.length() + 1];
            // std::strcpy(buffer, result.c_str());
            // return buffer;

            return nullptr;
        } catch (const std::exception& e) {
            g_error_message = std::string("Plan execution failed: ") + e.what();
            return nullptr;
        } catch (...) {
            g_error_message = "Unknown error executing plan";
            return nullptr;
        }
    }

    /// Free a query plan
    void qlever_plan_free(void* plan) {
        try {
            if (plan) {
                // TODO: delete static_cast<qlever::QueryPlan*>(plan);
            }
        } catch (...) {
            g_error_message = "Error destroying query plan";
        }
    }

    /// ==================== Memory Management ====================

    /// Free a string allocated by C++
    ///
    /// # Arguments
    /// * ptr: Pointer to string allocated by qlever_* functions
    ///
    /// # Safety
    /// Must only be called with pointers returned by qlever_* functions.
    /// Calling twice on the same pointer is undefined behavior.
    void qlever_free_string(const char* ptr) {
        try {
            if (ptr) {
                delete[] ptr;
            }
        } catch (...) {
            // Deleting already-freed memory or invalid pointers is a critical error
            g_error_message = "Error freeing string";
        }
    }

    /// Get the last error message
    ///
    /// # Returns
    /// Null-terminated error string. Valid only until next FFI call.
    /// Do not free this string.
    const char* qlever_get_last_error() {
        return g_error_message.c_str();
    }

    // ==================== Additional Functions ====================
    // TODO: Add remaining functions from ffi/bindings.rs:
    // - qlever_query_and_pin()
    // - qlever_get_pinned_result()
    // - qlever_clear_pinned()
    // - qlever_write_materialized_view()
    // - qlever_load_materialized_view()
    // - qlever_list_materialized_views()
    // - qlever_server_stats()
    // - qlever_cache_stats()
    // - qlever_index_stats()
}

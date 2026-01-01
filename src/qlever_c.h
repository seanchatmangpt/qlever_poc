#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* qlever_handle_t;
typedef void* qlever_query_plan_t;

/// Opens a QLever index from disk.
/// \param index_path: Path to the QLever index directory
/// \param config_json: Optional JSON configuration string (can be NULL for defaults)
/// \return Opaque handle, or NULL on failure
qlever_handle_t qlever_open(const char* index_path, const char* config_json);

/// Executes a SPARQL query and returns results as JSON.
/// \param h: Handle from qlever_open
/// \param sparql: SPARQL query string
/// \param detailed_timings: If non-zero, include timing information in result
/// \return Malloc'ed null-terminated UTF-8 JSON string, or NULL on error.
///         Must be freed with qlever_free_string().
char* qlever_query_json(qlever_handle_t h, const char* sparql, int detailed_timings);

/// Parses and plans a SPARQL query separately from execution.
/// \param h: Handle from qlever_open
/// \param sparql: SPARQL query string
/// \return Opaque query plan handle, or NULL on parse/planning error
qlever_query_plan_t qlever_parse_and_plan(qlever_handle_t h, const char* sparql);

/// Executes a previously parsed and planned query.
/// \param h: Handle from qlever_open
/// \param plan: Query plan from qlever_parse_and_plan
/// \param detailed_timings: If non-zero, include timing information
/// \return Malloc'ed JSON result, or NULL on execution error
char* qlever_execute_plan(qlever_handle_t h, qlever_query_plan_t plan, int detailed_timings);

/// Frees a query plan handle.
void qlever_free_plan(qlever_query_plan_t plan);

/// Caches and pins a query result with a name for reuse in SERVICE clauses.
/// \param h: Handle from qlever_open
/// \param name: Name to identify the cached result (used in SERVICE ql:cached-result-with-name-<name>)
/// \param sparql: SPARQL query to execute and cache
void qlever_pin_result(qlever_handle_t h, const char* name, const char* sparql);

/// Clears a cached result by name.
void qlever_erase_result(qlever_handle_t h, const char* name);

/// Clears all cached results.
void qlever_clear_cache(qlever_handle_t h);

/// Writes a materialized view to disk.
/// \param h: Handle from qlever_open
/// \param name: Name of the materialized view
/// \param sparql: SPARQL query defining the view
void qlever_write_materialized_view(qlever_handle_t h, const char* name, const char* sparql);

/// Loads a previously written materialized view into memory.
/// \param h: Handle from qlever_open
/// \param name: Name of the materialized view to load
void qlever_load_materialized_view(qlever_handle_t h, const char* name);

/// Full-text text search over indexed literals.
/// \param h: Handle from qlever_open
/// \param text_query: Text search query
/// \param limit: Maximum number of results (0 for no limit)
/// \return Malloc'ed JSON result with matching literals and scores, or NULL on error
char* qlever_text_search(qlever_handle_t h, const char* text_query, int limit);

/// Frees a string returned by qlever_query_json, qlever_execute_plan, or qlever_text_search.
void qlever_free_string(char* s);

/// Closes the QLever handle and frees resources.
void qlever_close(qlever_handle_t h);

#ifdef __cplusplus
}
#endif

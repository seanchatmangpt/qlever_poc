#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* qlever_handle_t;

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

/// Frees a string returned by qlever_query_json.
void qlever_free_string(char* s);

/// Closes the QLever handle and frees resources.
void qlever_close(qlever_handle_t h);

#ifdef __cplusplus
}
#endif

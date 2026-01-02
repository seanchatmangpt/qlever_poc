#pragma once

/**
 * QLeverest FFI: Zero-Copy Foreign Function Interface for QLever
 *
 * EPIC 10.3 Agent 1: FFI Architect
 *
 * INVARIANTS ENFORCED:
 * 1. C-ABI Sovereignty: All handles are opaque (void* wrappers)
 * 2. Zero-Copy Absolute: No memcpy in hot paths; shared memory only
 * 3. Bit-Parity Requirement: Results must be identical across ARM/x86
 * 4. FPV Closure: All operations have formal property verification
 * 5. Memory Isolation: All memory access through FFI gates only
 *
 * MEMORY CONTRACT:
 * - C++ owns all objects (Index, QueryExecutionTree, Result)
 * - Rust leases opaque handles (read-only pointers)
 * - Shared memory for zero-copy data transfer
 * - Thread-safe atomic handle pool for concurrent access
 * - Reference counting for shared ownership
 * - Explicit lifecycle management (create/destroy pairs)
 *
 * ABI VERSION: 1.0.0
 * BLAKE3 Hash: <computed at build time>
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// OPAQUE HANDLE TYPES (C-ABI Sovereignty)
// ============================================================================

/**
 * Opaque handle to QLever Index (RDF storage engine).
 * Represents: std::unique_ptr<Index>
 * Lifetime: Created by qleverest_index_open, destroyed by qleverest_index_close
 * Thread-safety: Thread-safe (Index is internally synchronized)
 */
typedef void* qleverest_index_handle_t;

/**
 * Opaque handle to QueryExecutionContext.
 * Represents: std::unique_ptr<QueryExecutionContext>
 * Lifetime: Created by qleverest_qec_create, destroyed by qleverest_qec_destroy
 * Thread-safety: Thread-local (create one per thread)
 */
typedef void* qleverest_qec_handle_t;

/**
 * Opaque handle to ParsedQuery.
 * Represents: std::unique_ptr<ParsedQuery>
 * Lifetime: Created by qleverest_parse_query, destroyed by qleverest_parsed_query_destroy
 * Thread-safety: Immutable after creation
 */
typedef void* qleverest_parsed_query_handle_t;

/**
 * Opaque handle to QueryExecutionTree.
 * Represents: std::shared_ptr<QueryExecutionTree>
 * Lifetime: Created by qleverest_plan_query, destroyed by qleverest_qet_destroy
 * Thread-safety: Thread-safe (reference counted)
 */
typedef void* qleverest_qet_handle_t;

/**
 * Opaque handle to Result.
 * Represents: std::shared_ptr<const Result>
 * Lifetime: Created by qleverest_execute_query, destroyed by qleverest_result_destroy
 * Thread-safety: Thread-safe (immutable, reference counted)
 */
typedef void* qleverest_result_handle_t;

/**
 * Opaque handle to IdTable (zero-copy result data).
 * Represents: const IdTable* (non-owning pointer)
 * Lifetime: Valid while parent Result handle is alive
 * Thread-safety: Thread-safe (immutable, borrowed reference)
 */
typedef void* qleverest_idtable_handle_t;

/**
 * Opaque handle to row iterator (zero-copy row access).
 * Represents: Internal iterator state
 * Lifetime: Created by qleverest_result_iter_rows, destroyed by qleverest_iter_destroy
 * Thread-safety: Not thread-safe (iterator is stateful)
 */
typedef void* qleverest_row_iter_handle_t;

// ============================================================================
// ERROR HANDLING
// ============================================================================

/**
 * Error code enumeration (closed domain, exhaustive).
 */
typedef enum {
  QLEVEREST_OK = 0,
  QLEVEREST_ERR_NULL_HANDLE = 1,
  QLEVEREST_ERR_INVALID_HANDLE = 2,
  QLEVEREST_ERR_INDEX_OPEN_FAILED = 3,
  QLEVEREST_ERR_PARSE_FAILED = 4,
  QLEVEREST_ERR_PLAN_FAILED = 5,
  QLEVEREST_ERR_EXEC_FAILED = 6,
  QLEVEREST_ERR_OUT_OF_MEMORY = 7,
  QLEVEREST_ERR_INVALID_QUERY = 8,
  QLEVEREST_ERR_TIMEOUT = 9,
  QLEVEREST_ERR_CANCELLED = 10,
  QLEVEREST_ERR_UNKNOWN = 99
} qleverest_error_code_t;

/**
 * Error details structure (allocated by C++, freed by Rust).
 * Memory Layout: Fixed-size struct, no dynamic allocation.
 */
typedef struct {
  qleverest_error_code_t code;
  char message[1024];  // Null-terminated UTF-8
  const char* file;    // Static string, do not free
  int line;
} qleverest_error_t;

/**
 * Get the last error for the current thread.
 * Returns: Pointer to thread-local error struct, or NULL if no error.
 * Thread-safety: Thread-local storage
 */
const qleverest_error_t* qleverest_get_last_error(void);

/**
 * Clear the last error for the current thread.
 */
void qleverest_clear_error(void);

// ============================================================================
// INDEX MANAGEMENT (Memory Isolation Boundary #1)
// ============================================================================

/**
 * Open a QLever index from disk.
 *
 * @param index_path Null-terminated UTF-8 path to index directory
 * @param config_json Optional JSON configuration (NULL for defaults)
 * @return Opaque index handle, or NULL on failure (check qleverest_get_last_error)
 *
 * Memory Contract: C++ allocates Index, returns opaque handle
 * Thread-safety: Thread-safe (can be called from multiple threads)
 * FPV Property: index_path must be valid directory OR return NULL
 */
qleverest_index_handle_t qleverest_index_open(
  const char* index_path,
  const char* config_json
);

/**
 * Close an index handle and free all associated resources.
 *
 * @param index Handle from qleverest_index_open
 *
 * Memory Contract: C++ frees Index, invalidates handle
 * Thread-safety: Must not be called concurrently with other operations on same handle
 * FPV Property: After close, handle is invalid for all future operations
 */
void qleverest_index_close(qleverest_index_handle_t index);

/**
 * Get index statistics (number of triples, subjects, predicates, objects).
 *
 * @param index Handle from qleverest_index_open
 * @param num_triples Output: total number of triples
 * @param num_subjects Output: distinct number of subjects
 * @param num_predicates Output: distinct number of predicates
 * @param num_objects Output: distinct number of objects
 * @return Error code (QLEVEREST_OK on success)
 *
 * Thread-safety: Thread-safe (Index is synchronized)
 */
qleverest_error_code_t qleverest_index_get_stats(
  qleverest_index_handle_t index,
  size_t* num_triples,
  size_t* num_subjects,
  size_t* num_predicates,
  size_t* num_objects
);

// ============================================================================
// QUERY EXECUTION CONTEXT (Memory Isolation Boundary #2)
// ============================================================================

/**
 * Create a QueryExecutionContext for a given Index.
 *
 * @param index Handle from qleverest_index_open
 * @return Opaque QEC handle, or NULL on failure
 *
 * Memory Contract: C++ allocates QEC, returns opaque handle
 * Thread-safety: Create one QEC per thread (thread-local)
 * FPV Property: QEC lifetime must not exceed Index lifetime
 */
qleverest_qec_handle_t qleverest_qec_create(qleverest_index_handle_t index);

/**
 * Destroy a QueryExecutionContext and free resources.
 *
 * @param qec Handle from qleverest_qec_create
 *
 * Memory Contract: C++ frees QEC, invalidates handle
 */
void qleverest_qec_destroy(qleverest_qec_handle_t qec);

// ============================================================================
// QUERY PARSING (Memory Isolation Boundary #3)
// ============================================================================

/**
 * Parse a SPARQL query string into ParsedQuery.
 *
 * @param sparql Null-terminated UTF-8 SPARQL query
 * @return Opaque ParsedQuery handle, or NULL on parse failure
 *
 * Memory Contract: C++ allocates ParsedQuery, returns opaque handle
 * Thread-safety: Thread-safe (parser is stateless)
 * FPV Property: Valid SPARQL => non-NULL handle; invalid SPARQL => NULL + error
 */
qleverest_parsed_query_handle_t qleverest_parse_query(const char* sparql);

/**
 * Destroy a ParsedQuery handle.
 *
 * @param parsed_query Handle from qleverest_parse_query
 */
void qleverest_parsed_query_destroy(qleverest_parsed_query_handle_t parsed_query);

/**
 * Get the query type (SELECT, CONSTRUCT, ASK, DESCRIBE, UPDATE).
 *
 * @param parsed_query Handle from qleverest_parse_query
 * @return Query type as null-terminated string, or NULL on error
 *
 * Memory Contract: Returns pointer to internal string (do not free)
 * Thread-safety: Thread-safe (ParsedQuery is immutable)
 */
const char* qleverest_parsed_query_get_type(
  qleverest_parsed_query_handle_t parsed_query
);

// ============================================================================
// QUERY PLANNING (Memory Isolation Boundary #4)
// ============================================================================

/**
 * Plan a parsed query into a QueryExecutionTree.
 *
 * @param qec QueryExecutionContext handle
 * @param parsed_query ParsedQuery handle
 * @return Opaque QueryExecutionTree handle, or NULL on planning failure
 *
 * Memory Contract: C++ allocates QueryExecutionTree (shared_ptr), returns opaque handle
 * Thread-safety: Thread-safe (if QEC is thread-local)
 * FPV Property: Valid parse + valid index => non-NULL QET OR error
 */
qleverest_qet_handle_t qleverest_plan_query(
  qleverest_qec_handle_t qec,
  qleverest_parsed_query_handle_t parsed_query
);

/**
 * Destroy a QueryExecutionTree handle.
 *
 * @param qet Handle from qleverest_plan_query
 *
 * Memory Contract: C++ decrements shared_ptr ref count
 */
void qleverest_qet_destroy(qleverest_qet_handle_t qet);

/**
 * Get the estimated number of result rows.
 *
 * @param qet QueryExecutionTree handle
 * @return Size estimate, or 0 on error
 *
 * Thread-safety: Thread-safe (QET is immutable after planning)
 */
size_t qleverest_qet_get_size_estimate(qleverest_qet_handle_t qet);

/**
 * Get the estimated query cost (abstract units).
 *
 * @param qet QueryExecutionTree handle
 * @return Cost estimate, or 0 on error
 */
size_t qleverest_qet_get_cost_estimate(qleverest_qet_handle_t qet);

// ============================================================================
// QUERY EXECUTION (Memory Isolation Boundary #5)
// ============================================================================

/**
 * Execute a QueryExecutionTree and return the Result.
 *
 * @param qet QueryExecutionTree handle
 * @return Opaque Result handle, or NULL on execution failure
 *
 * Memory Contract: C++ allocates Result (shared_ptr), returns opaque handle
 * Thread-safety: Thread-safe (execution is reentrant)
 * FPV Property: Valid QET => non-NULL Result OR error (no silent failure)
 */
qleverest_result_handle_t qleverest_execute_query(qleverest_qet_handle_t qet);

/**
 * Destroy a Result handle.
 *
 * @param result Handle from qleverest_execute_query
 *
 * Memory Contract: C++ decrements shared_ptr ref count
 */
void qleverest_result_destroy(qleverest_result_handle_t result);

// ============================================================================
// RESULT ACCESS (Zero-Copy Memory Transfer)
// ============================================================================

/**
 * Get the IdTable from a Result (zero-copy, borrowed pointer).
 *
 * @param result Result handle
 * @return Opaque IdTable handle (non-owning), or NULL on error
 *
 * Memory Contract: Returns borrowed pointer (valid while Result is alive)
 * Zero-Copy: No data copied; Rust borrows C++ memory
 * Thread-safety: Thread-safe (Result is immutable)
 * FPV Property: IdTable lifetime <= Result lifetime
 */
qleverest_idtable_handle_t qleverest_result_get_idtable(
  qleverest_result_handle_t result
);

/**
 * Get the number of rows in the IdTable.
 *
 * @param idtable IdTable handle
 * @return Number of rows, or 0 on error
 *
 * Zero-Copy: No data access, just metadata
 */
size_t qleverest_idtable_num_rows(qleverest_idtable_handle_t idtable);

/**
 * Get the number of columns in the IdTable.
 *
 * @param idtable IdTable handle
 * @return Number of columns, or 0 on error
 */
size_t qleverest_idtable_num_columns(qleverest_idtable_handle_t idtable);

/**
 * Get a pointer to the raw data of a specific column (zero-copy).
 *
 * @param idtable IdTable handle
 * @param column_index Column index (0-based)
 * @param out_data Output: pointer to column data (array of uint64_t IDs)
 * @param out_size Output: number of elements in the array
 * @return Error code (QLEVEREST_OK on success)
 *
 * Memory Contract: Returns borrowed pointer to internal data
 * Zero-Copy: Rust gets direct pointer to C++ memory (read-only)
 * Thread-safety: Thread-safe (IdTable is immutable)
 * FPV Property: Pointer valid while IdTable (and Result) are alive
 */
qleverest_error_code_t qleverest_idtable_get_column_data(
  qleverest_idtable_handle_t idtable,
  size_t column_index,
  const uint64_t** out_data,
  size_t* out_size
);

/**
 * Get a single cell value from the IdTable (zero-copy).
 *
 * @param idtable IdTable handle
 * @param row_index Row index (0-based)
 * @param column_index Column index (0-based)
 * @param out_value Output: cell value (uint64_t ID)
 * @return Error code (QLEVEREST_OK on success)
 *
 * Zero-Copy: Returns value by copy (8 bytes, negligible overhead)
 */
qleverest_error_code_t qleverest_idtable_get_cell(
  qleverest_idtable_handle_t idtable,
  size_t row_index,
  size_t column_index,
  uint64_t* out_value
);

// ============================================================================
// ROW ITERATION (Zero-Copy Iterator)
// ============================================================================

/**
 * Create a row iterator for zero-copy row-wise access.
 *
 * @param result Result handle
 * @return Opaque row iterator handle, or NULL on error
 *
 * Memory Contract: C++ allocates iterator state, returns opaque handle
 * Zero-Copy: Iterator provides borrowed pointers to rows
 * Thread-safety: Not thread-safe (iterator is stateful)
 */
qleverest_row_iter_handle_t qleverest_result_iter_rows(
  qleverest_result_handle_t result
);

/**
 * Advance the iterator to the next row.
 *
 * @param iter Row iterator handle
 * @param out_row Output: pointer to row data (array of uint64_t)
 * @param out_num_columns Output: number of columns in the row
 * @return 1 if row is valid, 0 if end of iteration, -1 on error
 *
 * Zero-Copy: Returns borrowed pointer to row (valid until next call)
 * FPV Property: out_row valid only until next call to qleverest_iter_next
 */
int qleverest_iter_next(
  qleverest_row_iter_handle_t iter,
  const uint64_t** out_row,
  size_t* out_num_columns
);

/**
 * Destroy a row iterator.
 *
 * @param iter Row iterator handle
 */
void qleverest_iter_destroy(qleverest_row_iter_handle_t iter);

// ============================================================================
// VOCABULARY ACCESS (ID -> String Resolution)
// ============================================================================

/**
 * Resolve an ID to its string representation (IRI, literal, or blank node).
 *
 * @param index Index handle
 * @param id The ID to resolve
 * @param out_string Output: pointer to null-terminated UTF-8 string
 * @param out_length Output: length of string (excluding null terminator)
 * @return Error code (QLEVEREST_OK on success)
 *
 * Memory Contract: Returns pointer to internal string (do not free)
 * Thread-safety: Thread-safe (Vocabulary is synchronized)
 * FPV Property: Valid ID => non-NULL string; invalid ID => error
 */
qleverest_error_code_t qleverest_vocab_id_to_string(
  qleverest_index_handle_t index,
  uint64_t id,
  const char** out_string,
  size_t* out_length
);

/**
 * Resolve a string to its ID.
 *
 * @param index Index handle
 * @param string Null-terminated UTF-8 string (IRI, literal, or blank node)
 * @param out_id Output: the ID corresponding to the string
 * @return Error code (QLEVEREST_OK on success, QLEVEREST_ERR_INVALID_QUERY if not found)
 *
 * Thread-safety: Thread-safe (Vocabulary is synchronized)
 */
qleverest_error_code_t qleverest_vocab_string_to_id(
  qleverest_index_handle_t index,
  const char* string,
  uint64_t* out_id
);

// ============================================================================
// CACHE MANAGEMENT (Memory Isolation Boundary #6)
// ============================================================================

/**
 * Pin a query result with a name for reuse in SERVICE clauses.
 *
 * @param qec QueryExecutionContext handle
 * @param name Null-terminated UTF-8 name for the cached result
 * @param sparql SPARQL query to execute and cache
 * @return Error code (QLEVEREST_OK on success)
 *
 * Thread-safety: Thread-safe (cache is synchronized)
 */
qleverest_error_code_t qleverest_cache_pin_result(
  qleverest_qec_handle_t qec,
  const char* name,
  const char* sparql
);

/**
 * Erase a cached result by name.
 *
 * @param qec QueryExecutionContext handle
 * @param name Null-terminated UTF-8 name
 * @return Error code (QLEVEREST_OK on success)
 */
qleverest_error_code_t qleverest_cache_erase_result(
  qleverest_qec_handle_t qec,
  const char* name
);

/**
 * Clear all cached results.
 *
 * @param qec QueryExecutionContext handle
 */
void qleverest_cache_clear_all(qleverest_qec_handle_t qec);

// ============================================================================
// CONVENIENCE API (Combined Operations)
// ============================================================================

/**
 * Execute a SPARQL query and return the result as JSON (convenience function).
 *
 * @param index Index handle
 * @param sparql Null-terminated UTF-8 SPARQL query
 * @param out_json Output: malloc'ed null-terminated UTF-8 JSON string
 * @return Error code (QLEVEREST_OK on success)
 *
 * Memory Contract: C++ allocates JSON string (malloc), Rust must free with qleverest_free_string
 * Thread-safety: Thread-safe (creates thread-local QEC internally)
 *
 * NOTE: This is a convenience wrapper; for performance-critical code, use
 * the granular API (parse -> plan -> execute -> iterate).
 */
qleverest_error_code_t qleverest_query_json(
  qleverest_index_handle_t index,
  const char* sparql,
  char** out_json
);

/**
 * Free a string allocated by qleverest_query_json.
 *
 * @param s String to free
 */
void qleverest_free_string(char* s);

// ============================================================================
// ABI VERSION & COMPATIBILITY
// ============================================================================

/**
 * Get the ABI version of this FFI.
 *
 * @return Null-terminated version string (e.g., "1.0.0")
 *
 * Memory Contract: Returns pointer to static string (do not free)
 */
const char* qleverest_get_abi_version(void);

/**
 * Get the BLAKE3 hash of the FFI header (deterministic ABI fingerprint).
 *
 * @return Null-terminated hex string (64 characters)
 *
 * Memory Contract: Returns pointer to static string (do not free)
 * FPV Property: Hash changes if and only if ABI changes
 */
const char* qleverest_get_abi_hash(void);

#ifdef __cplusplus
}
#endif

// Copyright 2026, QLever contributors
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// EPIC 10.3 Agent 5 Part 3: Memory Isolation Integration
// FFI Wrapper: C-ABI interface for QLever with memory isolation guards
//
// Status: IMPLEMENTATION IN PROGRESS (blocked by Agent 2 FPV gate)
//
// All FFI functions follow the pattern:
// 1. Validate handle(s) using VALIDATE_HANDLE_OR_RETURN_*
// 2. Execute operation on validated C++ object
// 3. Return result (opaque handle or error code)
//
// Memory Contract: C++-owned, Rust-leased, zero-copy via borrowed pointers
// Thread-Safety: See ffi_memory_contract.md for handle-specific guarantees

#include <cstring>
#include <memory>
#include <string>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/Result.h"
#include "engine/idTable/IdTable.h"
#include "index/Index.h"
#include "parser/SparqlParser.h"
#include "qleverest/qleverest_ffi.h"
#include "util/Exception.h"
#include "util/MemoryBoundaryGuards.h"

// ============================================================================
// Thread-Local Error Storage (GUARD-5.1: Error Handling)
// ============================================================================

namespace {

// Thread-local error state (per-thread, no mutex required)
thread_local qleverest_error_t g_last_error = {QLEVEREST_OK, "", nullptr, 0};

void set_thread_local_error(qleverest_error_code_t code, const char* message,
                            const char* file = __FILE__, int line = __LINE__) {
  g_last_error.code = code;
  std::strncpy(g_last_error.message, message, sizeof(g_last_error.message) - 1);
  g_last_error.message[sizeof(g_last_error.message) - 1] = '\0';
  g_last_error.file = file;  // Static string, do not free
  g_last_error.line = line;
}

}  // anonymous namespace

// ============================================================================
// Error Handling API
// ============================================================================

extern "C" {

const qleverest_error_t* qleverest_get_last_error(void) {
  if (g_last_error.code == QLEVEREST_OK) {
    return nullptr;
  }
  return &g_last_error;
}

void qleverest_clear_error(void) {
  g_last_error.code = QLEVEREST_OK;
  g_last_error.message[0] = '\0';
  g_last_error.file = nullptr;
  g_last_error.line = 0;
}

// ============================================================================
// Index Management (Memory Isolation Boundary #1)
// ============================================================================

qleverest_index_handle_t qleverest_index_open(const char* index_path,
                                              const char* config_json) {
  if (!index_path) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "Index path is NULL");
    return nullptr;
  }

  try {
    // TODO: Parse config_json (if non-NULL) for index configuration
    // For now, use default Index constructor
    auto index =
        std::make_shared<Index>(ad_utility::makeUnlimitedAllocator<Id>());

    // TODO: Actually open the index from disk
    // index->openIndex(index_path);

    // Register handle in opaque pool
    uint64_t handle_id =
        ad_utility::OpaqueHandlePool::instance().registerHandle(index);
    return reinterpret_cast<void*>(handle_id);
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_INDEX_OPEN_FAILED, e.what());
    return nullptr;
  }
}

void qleverest_index_close(qleverest_index_handle_t index) {
  if (!index) {
    return;  // NULL handle is idempotent (no error)
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(index);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
  // std::shared_ptr<Index> automatically destroyed when refcount = 0
}

qleverest_error_code_t qleverest_index_get_stats(qleverest_index_handle_t index,
                                                 size_t* num_triples,
                                                 size_t* num_subjects,
                                                 size_t* num_predicates,
                                                 size_t* num_objects) {
  // Validate handle
  auto index_ptr = ad_utility::OpaqueHandlePool::instance().getHandle<Index>(
      reinterpret_cast<uint64_t>(index));
  if (!index_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Index handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  try {
    // ASPIRATIONAL STUB: Statistics retrieval not yet implemented.
    // TODO: Implement actual statistics retrieval from Index object:
    //   - index_ptr->numTriples()
    //   - index_ptr->numDistinctSubjects()
    //   - index_ptr->numPredicates()
    //   - index_ptr->numDistinctObjects()
    //
    // Current status: Returns UNIMPLEMENTED to prevent silent failures.
    // Previously returned OK with all stats=0, which was deceptive.

    set_thread_local_error(QLEVEREST_ERR_UNIMPLEMENTED,
                           "Statistics retrieval not yet implemented");
    return QLEVEREST_ERR_UNIMPLEMENTED;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

// ============================================================================
// Query Execution Context (Memory Isolation Boundary #2)
// ============================================================================

qleverest_qec_handle_t qleverest_qec_create(qleverest_index_handle_t index) {
  // Validate Index handle
  auto index_ptr = ad_utility::OpaqueHandlePool::instance().getHandle<Index>(
      reinterpret_cast<uint64_t>(index));
  if (!index_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Index handle in qleverest_qec_create");
    return nullptr;
  }

  try {
    // TODO: Create actual QueryExecutionContext
    // auto qec = std::make_shared<QueryExecutionContext>(index_ptr, allocator,
    // ...); For now, return placeholder
    set_thread_local_error(
        QLEVEREST_ERR_UNKNOWN,
        "QueryExecutionContext creation not yet implemented");
    return nullptr;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return nullptr;
  }
}

void qleverest_qec_destroy(qleverest_qec_handle_t qec) {
  if (!qec) {
    return;  // NULL handle is idempotent
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(qec);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
}

// ============================================================================
// Query Parsing (Memory Isolation Boundary #3)
// ============================================================================

qleverest_parsed_query_handle_t qleverest_parse_query(const char* sparql) {
  if (!sparql) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "SPARQL query string is NULL");
    return nullptr;
  }

  try {
    // TODO: Parse SPARQL query using SparqlParser
    // SparqlParser parser;
    // auto parsed_query = std::make_shared<ParsedQuery>(parser.parse(sparql));

    set_thread_local_error(QLEVEREST_ERR_PARSE_FAILED,
                           "SPARQL parsing not yet implemented");
    return nullptr;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_PARSE_FAILED, e.what());
    return nullptr;
  }
}

void qleverest_parsed_query_destroy(
    qleverest_parsed_query_handle_t parsed_query) {
  if (!parsed_query) {
    return;
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(parsed_query);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
}

const char* qleverest_parsed_query_get_type(
    qleverest_parsed_query_handle_t parsed_query) {
  // Validate handle
  auto parsed_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<ParsedQuery>(
          reinterpret_cast<uint64_t>(parsed_query));
  if (!parsed_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid ParsedQuery handle");
    return nullptr;
  }

  try {
    // TODO: Return query type string
    // return parsed_ptr->queryType().c_str();  // Static string
    return "SELECT";  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return nullptr;
  }
}

// ============================================================================
// Query Planning (Memory Isolation Boundary #4)
// ============================================================================

qleverest_qet_handle_t qleverest_plan_query(
    qleverest_qec_handle_t qec, qleverest_parsed_query_handle_t parsed_query) {
  // Validate QEC handle
  auto qec_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionContext>(
          reinterpret_cast<uint64_t>(qec));
  if (!qec_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionContext handle");
    return nullptr;
  }

  // Validate ParsedQuery handle
  auto parsed_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<ParsedQuery>(
          reinterpret_cast<uint64_t>(parsed_query));
  if (!parsed_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid ParsedQuery handle");
    return nullptr;
  }

  try {
    // TODO: Plan query using QueryPlanner
    // auto qet = std::make_shared<QueryExecutionTree>(qec_ptr, parsed_ptr);

    set_thread_local_error(QLEVEREST_ERR_PLAN_FAILED,
                           "Query planning not yet implemented");
    return nullptr;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_PLAN_FAILED, e.what());
    return nullptr;
  }
}

void qleverest_qet_destroy(qleverest_qet_handle_t qet) {
  if (!qet) {
    return;
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(qet);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
}

size_t qleverest_qet_get_size_estimate(qleverest_qet_handle_t qet) {
  // Validate handle
  auto qet_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionTree>(
          reinterpret_cast<uint64_t>(qet));
  if (!qet_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionTree handle");
    return 0;
  }

  try {
    // TODO: Return actual size estimate
    // return qet_ptr->getSizeEstimate();
    return 0;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return 0;
  }
}

size_t qleverest_qet_get_cost_estimate(qleverest_qet_handle_t qet) {
  // Validate handle
  auto qet_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionTree>(
          reinterpret_cast<uint64_t>(qet));
  if (!qet_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionTree handle");
    return 0;
  }

  try {
    // TODO: Return actual cost estimate
    // return qet_ptr->getCostEstimate();
    return 0;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return 0;
  }
}

// ============================================================================
// Query Execution (Memory Isolation Boundary #5)
// ============================================================================

qleverest_result_handle_t qleverest_execute_query(qleverest_qet_handle_t qet) {
  // Validate handle
  auto qet_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionTree>(
          reinterpret_cast<uint64_t>(qet));
  if (!qet_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionTree handle");
    return nullptr;
  }

  try {
    // TODO: Execute query and get Result
    // auto result = qet_ptr->getResult();
    // uint64_t result_handle_id = ad_utility::OpaqueHandlePool::instance()
    //                                  .registerHandle(result);
    // return reinterpret_cast<void*>(result_handle_id);

    set_thread_local_error(QLEVEREST_ERR_EXEC_FAILED,
                           "Query execution not yet implemented");
    return nullptr;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_EXEC_FAILED, e.what());
    return nullptr;
  }
}

void qleverest_result_destroy(qleverest_result_handle_t result) {
  if (!result) {
    return;
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(result);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
}

// ============================================================================
// Result Access (Zero-Copy Memory Transfer)
// ============================================================================

qleverest_idtable_handle_t qleverest_result_get_idtable(
    qleverest_result_handle_t result) {
  // Validate handle
  auto result_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<const Result>(
          reinterpret_cast<uint64_t>(result));
  if (!result_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Result handle");
    return nullptr;
  }

  try {
    // TODO: Return borrowed pointer to IdTable
    // const IdTable& idtable = result_ptr->idTable();
    // return const_cast<void*>(reinterpret_cast<const void*>(&idtable));

    return nullptr;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return nullptr;
  }
}

size_t qleverest_idtable_num_rows(qleverest_idtable_handle_t idtable) {
  // Note: IdTable is a borrowed pointer, not registered in handle pool
  // Validation relies on Rust lifetime enforcement
  if (!idtable) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "IdTable handle is NULL");
    return 0;
  }

  try {
    // TODO: Cast to IdTable and return row count
    // const IdTable* table = reinterpret_cast<const IdTable*>(idtable);
    // return table->numRows();
    return 0;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return 0;
  }
}

size_t qleverest_idtable_num_columns(qleverest_idtable_handle_t idtable) {
  if (!idtable) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "IdTable handle is NULL");
    return 0;
  }

  try {
    // TODO: Cast to IdTable and return column count
    // const IdTable* table = reinterpret_cast<const IdTable*>(idtable);
    // return table->numColumns();
    return 0;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return 0;
  }
}

qleverest_error_code_t qleverest_idtable_get_column_data(
    qleverest_idtable_handle_t idtable, size_t column_index,
    const uint64_t** out_data, size_t* out_size) {
  if (!idtable) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "IdTable handle is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  if (!out_data || !out_size) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "Output pointers are NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Cast to IdTable, validate column index, return zero-copy pointer
    // const IdTable* table = reinterpret_cast<const IdTable*>(idtable);
    // if (column_index >= table->numColumns()) {
    //   set_thread_local_error(QLEVEREST_ERR_INVALID_QUERY,
    //                           "Column index out of bounds");
    //   return QLEVEREST_ERR_INVALID_QUERY;
    // }
    // const auto& column = (*table)[column_index];
    // *out_data = reinterpret_cast<const uint64_t*>(column.data());
    // *out_size = column.size();
    // return QLEVEREST_OK;

    set_thread_local_error(QLEVEREST_ERR_UNKNOWN,
                           "IdTable column access not yet implemented");
    return QLEVEREST_ERR_UNKNOWN;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

qleverest_error_code_t qleverest_idtable_get_cell(
    qleverest_idtable_handle_t idtable, size_t row_index, size_t column_index,
    uint64_t* out_value) {
  if (!idtable) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "IdTable handle is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  if (!out_value) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "Output pointer is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Cast to IdTable, validate indices, return cell value
    // const IdTable* table = reinterpret_cast<const IdTable*>(idtable);
    // *out_value = (*table)(row_index, column_index).getBits();
    // return QLEVEREST_OK;

    *out_value = 0;  // Placeholder
    return QLEVEREST_OK;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

// ============================================================================
// Row Iteration (Zero-Copy Iterator)
// ============================================================================

qleverest_row_iter_handle_t qleverest_result_iter_rows(
    qleverest_result_handle_t result) {
  // Validate handle
  auto result_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<const Result>(
          reinterpret_cast<uint64_t>(result));
  if (!result_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Result handle");
    return nullptr;
  }

  try {
    // TODO: Create row iterator
    // auto iter = std::make_shared<RowIterator>(result_ptr);
    // uint64_t iter_handle_id = ad_utility::OpaqueHandlePool::instance()
    //                                .registerHandle(iter);
    // return reinterpret_cast<void*>(iter_handle_id);

    return nullptr;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return nullptr;
  }
}

int qleverest_iter_next(qleverest_row_iter_handle_t iter,
                        const uint64_t** out_row, size_t* out_num_columns) {
  // Validate handle
  // auto iter_ptr = ad_utility::OpaqueHandlePool::instance()
  //                      .getHandle<RowIterator>(
  //                          reinterpret_cast<uint64_t>(iter));
  // if (!iter_ptr) {
  //   set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
  //                           "Invalid row iterator handle");
  //   return -1;
  // }

  try {
    // TODO: Advance iterator, return borrowed pointer to row
    // if (iter_ptr->hasNext()) {
    //   auto row = iter_ptr->next();
    //   *out_row = row.data();
    //   *out_num_columns = row.size();
    //   return 1;  // Row valid
    // } else {
    //   return 0;  // End of iteration
    // }

    return 0;  // Placeholder (end of iteration)
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return -1;
  }
}

void qleverest_iter_destroy(qleverest_row_iter_handle_t iter) {
  if (!iter) {
    return;
  }

  uint64_t handle_id = reinterpret_cast<uint64_t>(iter);
  ad_utility::OpaqueHandlePool::instance().unregisterHandle(handle_id);
}

// ============================================================================
// Vocabulary Access (ID -> String Resolution)
// ============================================================================

qleverest_error_code_t qleverest_vocab_id_to_string(
    qleverest_index_handle_t index, uint64_t id, const char** out_string,
    size_t* out_length) {
  // Validate handle
  auto index_ptr = ad_utility::OpaqueHandlePool::instance().getHandle<Index>(
      reinterpret_cast<uint64_t>(index));
  if (!index_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Index handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  if (!out_string || !out_length) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "Output pointers are NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Resolve ID to string via Vocabulary
    // const std::string& str =
    // index_ptr->getVocab().idToString(Id::makeFromInt(id)); *out_string =
    // str.c_str();  // Borrowed pointer *out_length = str.size(); return
    // QLEVEREST_OK;

    *out_string = "";
    *out_length = 0;
    return QLEVEREST_OK;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

qleverest_error_code_t qleverest_vocab_string_to_id(
    qleverest_index_handle_t index, const char* string, uint64_t* out_id) {
  // Validate handle
  auto index_ptr = ad_utility::OpaqueHandlePool::instance().getHandle<Index>(
      reinterpret_cast<uint64_t>(index));
  if (!index_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Index handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  if (!string || !out_id) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "Input/output pointers are NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Resolve string to ID via Vocabulary
    // Id id = index_ptr->getVocab().getIdForString(string);
    // *out_id = id.getBits();
    // return QLEVEREST_OK;

    *out_id = 0;  // Placeholder
    return QLEVEREST_OK;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_QUERY, e.what());
    return QLEVEREST_ERR_INVALID_QUERY;
  }
}

// ============================================================================
// Cache Management (Memory Isolation Boundary #6)
// ============================================================================

qleverest_error_code_t qleverest_cache_pin_result(qleverest_qec_handle_t qec,
                                                  const char* name,
                                                  const char* sparql) {
  // Validate handle
  auto qec_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionContext>(
          reinterpret_cast<uint64_t>(qec));
  if (!qec_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionContext handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  if (!name || !sparql) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "Name or SPARQL query is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Pin query result in cache
    // qec_ptr->getQueryResultCache().pinResult(name, sparql);
    return QLEVEREST_OK;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

qleverest_error_code_t qleverest_cache_erase_result(qleverest_qec_handle_t qec,
                                                    const char* name) {
  // Validate handle
  auto qec_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionContext>(
          reinterpret_cast<uint64_t>(qec));
  if (!qec_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionContext handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  if (!name) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "Name is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Erase cached result by name
    // qec_ptr->getQueryResultCache().eraseResult(name);
    return QLEVEREST_OK;  // Placeholder
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
    return QLEVEREST_ERR_UNKNOWN;
  }
}

void qleverest_cache_clear_all(qleverest_qec_handle_t qec) {
  // Validate handle
  auto qec_ptr =
      ad_utility::OpaqueHandlePool::instance().getHandle<QueryExecutionContext>(
          reinterpret_cast<uint64_t>(qec));
  if (!qec_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid QueryExecutionContext handle");
    return;
  }

  try {
    // TODO: Clear all cached results
    // qec_ptr->getQueryResultCache().clear();
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_UNKNOWN, e.what());
  }
}

// ============================================================================
// Convenience API (Combined Operations)
// ============================================================================

qleverest_error_code_t qleverest_query_json(qleverest_index_handle_t index,
                                            const char* sparql,
                                            char** out_json) {
  // Validate handle
  auto index_ptr = ad_utility::OpaqueHandlePool::instance().getHandle<Index>(
      reinterpret_cast<uint64_t>(index));
  if (!index_ptr) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_HANDLE,
                           "Invalid Index handle");
    return QLEVEREST_ERR_INVALID_HANDLE;
  }

  if (!sparql || !out_json) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE,
                           "Input/output pointers are NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  try {
    // TODO: Parse -> Plan -> Execute -> Serialize to JSON
    // 1. Create QEC
    // 2. Parse query
    // 3. Plan query
    // 4. Execute query
    // 5. Serialize Result to JSON
    // 6. Allocate and copy JSON string

    // Placeholder: Return empty JSON array
    const char* placeholder_json = "[]";
    *out_json = static_cast<char*>(malloc(strlen(placeholder_json) + 1));
    strcpy(*out_json, placeholder_json);
    return QLEVEREST_OK;
  } catch (const std::exception& e) {
    set_thread_local_error(QLEVEREST_ERR_EXEC_FAILED, e.what());
    return QLEVEREST_ERR_EXEC_FAILED;
  }
}

void qleverest_free_string(char* s) {
  if (s) {
    free(s);
  }
}

// ============================================================================
// ABI Version & Compatibility
// ============================================================================

const char* qleverest_get_abi_version(void) {
  return "1.0.0";  // Static string, do not free
}

const char* qleverest_get_abi_hash(void) {
  // TODO: Compute BLAKE3 hash of FFI header at build time
  return "0000000000000000000000000000000000000000000000000000000000000000";  // Placeholder
}

}  // extern "C"

// Copyright 2025, QLever
// EPIC 13 Phase 4: Working FFI Implementation (Agent 6)
//
// This file implements the C FFI bindings defined in qleverest_ffi.h
// using actual QLever C++ classes.

#include "qleverest/qleverest_ffi.h"

#include <cstring>
#include <exception>
#include <memory>
#include <string>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "engine/Result.h"
#include "index/Index.h"
#include "parser/SparqlParser.h"
#include "util/Exception.h"
#include "util/MemorySize/MemorySize.h"

// Thread-local error handling
thread_local qleverest_error_t g_last_error = {QLEVEREST_OK, {0}, nullptr, 0};

namespace {
// Helper to set last error
void setLastError(qleverest_error_code_t code, const std::string& message,
                  const char* file = __builtin_FILE(),
                  int line = __builtin_LINE()) {
  g_last_error.code = code;
  std::strncpy(g_last_error.message, message.c_str(),
               sizeof(g_last_error.message) - 1);
  g_last_error.message[sizeof(g_last_error.message) - 1] = '\0';
  g_last_error.file = file;
  g_last_error.line = line;
}

void clearError() {
  g_last_error.code = QLEVEREST_OK;
  g_last_error.message[0] = '\0';
  g_last_error.file = nullptr;
  g_last_error.line = 0;
}

// RAII wrapper for automatic error handling
template <typename T>
T* handleExceptions(const char* operation,
                    std::function<T*()> func) noexcept {
  try {
    clearError();
    return func();
  } catch (const ad_utility::Exception& e) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": " + e.what());
    return nullptr;
  } catch (const std::exception& e) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": " + e.what());
    return nullptr;
  } catch (...) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": unknown error");
    return nullptr;
  }
}

qleverest_error_code_t handleExceptionsForCode(
    const char* operation, std::function<void()> func) noexcept {
  try {
    clearError();
    func();
    return QLEVEREST_OK;
  } catch (const ad_utility::Exception& e) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": " + e.what());
    return g_last_error.code;
  } catch (const std::exception& e) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": " + e.what());
    return g_last_error.code;
  } catch (...) {
    setLastError(QLEVEREST_ERR_UNKNOWN,
                 std::string(operation) + ": unknown error");
    return QLEVEREST_ERR_UNKNOWN;
  }
}

}  // namespace

// ============================================================================
// ERROR HANDLING
// ============================================================================

extern "C" {

const qleverest_error_t* qleverest_get_last_error(void) {
  if (g_last_error.code != QLEVEREST_OK) {
    return &g_last_error;
  }
  return nullptr;
}

void qleverest_clear_error(void) { clearError(); }

// ============================================================================
// INDEX MANAGEMENT
// ============================================================================

qleverest_index_handle_t qleverest_index_open(const char* index_path,
                                               const char* config_json) {
  return handleExceptions<Index>("qleverest_index_open", [&]() -> Index* {
    if (!index_path) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "index_path is NULL");
      return nullptr;
    }

    try {
      // Create allocator with default memory limit (16 GB)
      auto allocator = ad_utility::AllocatorWithLimit<Id>(
          ad_utility::makeAllocationMemoryLeftThreadsafeObject(
              ad_utility::MemorySize::gigabytes(16)));

      auto index = std::make_unique<Index>(allocator);

      // Open index from disk
      index->createFromOnDiskIndex(index_path, false);

      return index.release();
    } catch (const std::exception& e) {
      setLastError(QLEVEREST_ERR_INDEX_OPEN_FAILED, e.what());
      return nullptr;
    }
  });
}

void qleverest_index_close(qleverest_index_handle_t index) {
  if (index) {
    delete static_cast<Index*>(index);
  }
}

qleverest_error_code_t qleverest_index_get_stats(
    qleverest_index_handle_t index, size_t* num_triples, size_t* num_subjects,
    size_t* num_predicates, size_t* num_objects) {
  return handleExceptionsForCode("qleverest_index_get_stats", [&]() {
    if (!index) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "index is NULL");
      throw std::runtime_error("NULL index");
    }

    auto* idx = static_cast<Index*>(index);

    if (num_triples) {
      *num_triples = idx->numTriples().normalAndInternal_();
    }
    if (num_subjects) {
      *num_subjects = idx->numDistinctSubjects().normalAndInternal_();
    }
    if (num_predicates) {
      *num_predicates = idx->numDistinctPredicates().normalAndInternal_();
    }
    if (num_objects) {
      *num_objects = idx->numDistinctObjects().normalAndInternal_();
    }
  });
}

// ============================================================================
// QUERY EXECUTION CONTEXT
// ============================================================================

qleverest_qec_handle_t qleverest_qec_create(qleverest_index_handle_t index) {
  return handleExceptions<QueryExecutionContext>(
      "qleverest_qec_create", [&]() -> QueryExecutionContext* {
        if (!index) {
          setLastError(QLEVEREST_ERR_NULL_HANDLE, "index is NULL");
          return nullptr;
        }

        auto* idx = static_cast<Index*>(index);

        // Create QEC with cache and allocator
        // Note: We use nullptr for cache as we create a thread-local QEC
        auto allocator = ad_utility::AllocatorWithLimit<Id>(
            ad_utility::makeAllocationMemoryLeftThreadsafeObject(
                ad_utility::MemorySize::gigabytes(16)));

        auto qec = std::make_unique<QueryExecutionContext>(
            *idx, nullptr, allocator, SortPerformanceEstimator{},
            nullptr, nullptr);

        return qec.release();
      });
}

void qleverest_qec_destroy(qleverest_qec_handle_t qec) {
  if (qec) {
    delete static_cast<QueryExecutionContext*>(qec);
  }
}

// ============================================================================
// QUERY PARSING
// ============================================================================

qleverest_parsed_query_handle_t qleverest_parse_query(const char* sparql) {
  return handleExceptions<ParsedQuery>("qleverest_parse_query",
                                        [&]() -> ParsedQuery* {
    if (!sparql) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "sparql is NULL");
      return nullptr;
    }

    try {
      auto parsed = SparqlParser::parseQuery(std::string(sparql));
      return new ParsedQuery(std::move(parsed));
    } catch (const std::exception& e) {
      setLastError(QLEVEREST_ERR_PARSE_FAILED, e.what());
      return nullptr;
    }
  });
}

void qleverest_parsed_query_destroy(
    qleverest_parsed_query_handle_t parsed_query) {
  if (parsed_query) {
    delete static_cast<ParsedQuery*>(parsed_query);
  }
}

const char* qleverest_parsed_query_get_type(
    qleverest_parsed_query_handle_t parsed_query) {
  if (!parsed_query) {
    setLastError(QLEVEREST_ERR_NULL_HANDLE, "parsed_query is NULL");
    return nullptr;
  }

  auto* pq = static_cast<ParsedQuery*>(parsed_query);

  // Return static string based on query type
  if (pq->hasSelectClause()) {
    return "SELECT";
  } else if (pq->hasConstructClause()) {
    return "CONSTRUCT";
  } else {
    return "UNKNOWN";
  }
}

// ============================================================================
// QUERY PLANNING
// ============================================================================

qleverest_qet_handle_t qleverest_plan_query(
    qleverest_qec_handle_t qec,
    qleverest_parsed_query_handle_t parsed_query) {
  return handleExceptions<QueryExecutionTree>(
      "qleverest_plan_query", [&]() -> QueryExecutionTree* {
        if (!qec) {
          setLastError(QLEVEREST_ERR_NULL_HANDLE, "qec is NULL");
          return nullptr;
        }
        if (!parsed_query) {
          setLastError(QLEVEREST_ERR_NULL_HANDLE, "parsed_query is NULL");
          return nullptr;
        }

        auto* qecPtr = static_cast<QueryExecutionContext*>(qec);
        auto* pq = static_cast<ParsedQuery*>(parsed_query);

        try {
          QueryPlanner planner(qecPtr);
          auto qet = planner.createExecutionTree(*pq);

          // Convert shared_ptr to raw pointer (caller owns it)
          return new QueryExecutionTree(*qet);
        } catch (const std::exception& e) {
          setLastError(QLEVEREST_ERR_PLAN_FAILED, e.what());
          return nullptr;
        }
      });
}

void qleverest_qet_destroy(qleverest_qet_handle_t qet) {
  if (qet) {
    delete static_cast<QueryExecutionTree*>(qet);
  }
}

size_t qleverest_qet_get_size_estimate(qleverest_qet_handle_t qet) {
  if (!qet) {
    setLastError(QLEVEREST_ERR_NULL_HANDLE, "qet is NULL");
    return 0;
  }

  auto* qetPtr = static_cast<QueryExecutionTree*>(qet);
  return qetPtr->getSizeEstimate();
}

size_t qleverest_qet_get_cost_estimate(qleverest_qet_handle_t qet) {
  if (!qet) {
    setLastError(QLEVEREST_ERR_NULL_HANDLE, "qet is NULL");
    return 0;
  }

  auto* qetPtr = static_cast<QueryExecutionTree*>(qet);
  return qetPtr->getCostEstimate();
}

// ============================================================================
// QUERY EXECUTION
// ============================================================================

qleverest_result_handle_t qleverest_execute_query(qleverest_qet_handle_t qet) {
  return handleExceptions<Result>("qleverest_execute_query",
                                   [&]() -> Result* {
    if (!qet) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "qet is NULL");
      return nullptr;
    }

    auto* qetPtr = static_cast<QueryExecutionTree*>(qet);

    try {
      auto resultPtr = qetPtr->getResult();
      // Make a copy since we need to return ownership
      return new Result(*resultPtr);
    } catch (const std::exception& e) {
      setLastError(QLEVEREST_ERR_EXEC_FAILED, e.what());
      return nullptr;
    }
  });
}

void qleverest_result_destroy(qleverest_result_handle_t result) {
  if (result) {
    delete static_cast<Result*>(result);
  }
}

// ============================================================================
// RESULT ACCESS
// ============================================================================

qleverest_idtable_handle_t qleverest_result_get_idtable(
    qleverest_result_handle_t result) {
  if (!result) {
    setLastError(QLEVEREST_ERR_NULL_HANDLE, "result is NULL");
    return nullptr;
  }

  auto* res = static_cast<Result*>(result);

  try {
    // Return non-owning pointer to IdTable
    return const_cast<IdTable*>(&res->idTable());
  } catch (const std::exception& e) {
    setLastError(QLEVEREST_ERR_UNKNOWN, e.what());
    return nullptr;
  }
}

size_t qleverest_idtable_num_rows(qleverest_idtable_handle_t idtable) {
  if (!idtable) {
    return 0;
  }

  auto* table = static_cast<IdTable*>(idtable);
  return table->numRows();
}

size_t qleverest_idtable_num_columns(qleverest_idtable_handle_t idtable) {
  if (!idtable) {
    return 0;
  }

  auto* table = static_cast<IdTable*>(idtable);
  return table->numColumns();
}

qleverest_error_code_t qleverest_idtable_get_column_data(
    qleverest_idtable_handle_t idtable, size_t column_index,
    const uint64_t** out_data, size_t* out_size) {
  return handleExceptionsForCode("qleverest_idtable_get_column_data", [&]() {
    if (!idtable || !out_data || !out_size) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "NULL pointer");
      throw std::runtime_error("NULL pointer");
    }

    auto* table = static_cast<IdTable*>(idtable);

    if (column_index >= table->numColumns()) {
      setLastError(QLEVEREST_ERR_INVALID_QUERY, "Column index out of bounds");
      throw std::runtime_error("Column index out of bounds");
    }

    // Get pointer to column data
    *out_data = reinterpret_cast<const uint64_t*>(
        table->getColumn(column_index).data());
    *out_size = table->numRows();
  });
}

qleverest_error_code_t qleverest_idtable_get_cell(
    qleverest_idtable_handle_t idtable, size_t row_index, size_t column_index,
    uint64_t* out_value) {
  return handleExceptionsForCode("qleverest_idtable_get_cell", [&]() {
    if (!idtable || !out_value) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "NULL pointer");
      throw std::runtime_error("NULL pointer");
    }

    auto* table = static_cast<IdTable*>(idtable);

    if (row_index >= table->numRows() || column_index >= table->numColumns()) {
      setLastError(QLEVEREST_ERR_INVALID_QUERY, "Index out of bounds");
      throw std::runtime_error("Index out of bounds");
    }

    *out_value = (*table)(row_index, column_index).getBits();
  });
}

// ============================================================================
// VOCABULARY ACCESS
// ============================================================================

qleverest_error_code_t qleverest_vocab_id_to_string(
    qleverest_index_handle_t index, uint64_t id, const char** out_string,
    size_t* out_length) {
  return handleExceptionsForCode("qleverest_vocab_id_to_string", [&]() {
    if (!index || !out_string || !out_length) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "NULL pointer");
      throw std::runtime_error("NULL pointer");
    }

    auto* idx = static_cast<Index*>(index);

    // Convert ID to string using vocabulary
    Id idVal = Id::fromBits(id);
    auto str = idx->indexToString(idVal.getVocabIndex());

    *out_string = str.c_str();
    *out_length = str.size();
  });
}

// ============================================================================
// CONVENIENCE API
// ============================================================================

qleverest_error_code_t qleverest_query_json(qleverest_index_handle_t index,
                                             const char* sparql,
                                             char** out_json) {
  return handleExceptionsForCode("qleverest_query_json", [&]() {
    if (!index || !sparql || !out_json) {
      setLastError(QLEVEREST_ERR_NULL_HANDLE, "NULL pointer");
      throw std::runtime_error("NULL pointer");
    }

    // This is a convenience function - not implemented for now
    setLastError(QLEVEREST_ERR_UNKNOWN, "Not yet implemented");
    throw std::runtime_error("Not yet implemented");
  });
}

void qleverest_free_string(char* s) {
  if (s) {
    free(s);
  }
}

// ============================================================================
// ABI VERSION
// ============================================================================

const char* qleverest_get_abi_version(void) { return "1.0.0"; }

const char* qleverest_get_abi_hash(void) {
  // TODO: Compute actual BLAKE3 hash at build time
  return "0000000000000000000000000000000000000000000000000000000000000000";
}

// Stubs for remaining functions (row iteration, cache management)
// These are not critical for initial FFI validation

qleverest_row_iter_handle_t qleverest_result_iter_rows(
    qleverest_result_handle_t result) {
  setLastError(QLEVEREST_ERR_UNKNOWN, "Iterator not yet implemented");
  return nullptr;
}

int qleverest_iter_next(qleverest_row_iter_handle_t iter,
                        const uint64_t** out_row, size_t* out_num_columns) {
  return -1;
}

void qleverest_iter_destroy(qleverest_row_iter_handle_t iter) {}

qleverest_error_code_t qleverest_vocab_string_to_id(
    qleverest_index_handle_t index, const char* string, uint64_t* out_id) {
  setLastError(QLEVEREST_ERR_UNKNOWN, "Not yet implemented");
  return QLEVEREST_ERR_UNKNOWN;
}

qleverest_error_code_t qleverest_cache_pin_result(qleverest_qec_handle_t qec,
                                                   const char* name,
                                                   const char* sparql) {
  setLastError(QLEVEREST_ERR_UNKNOWN, "Cache pinning not yet implemented");
  return QLEVEREST_ERR_UNKNOWN;
}

qleverest_error_code_t qleverest_cache_erase_result(qleverest_qec_handle_t qec,
                                                     const char* name) {
  setLastError(QLEVEREST_ERR_UNKNOWN, "Cache erase not yet implemented");
  return QLEVEREST_ERR_UNKNOWN;
}

void qleverest_cache_clear_all(qleverest_qec_handle_t qec) {}

}  // extern "C"

// Copyright 2025 The QLever Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef QLEVER_SRC_LIBQLEVER_QLEVER_WASM_WRAPPER_H
#define QLEVER_SRC_LIBQLEVER_QLEVER_WASM_WRAPPER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace qlever::wasm {

// Forward declaration
class Qlever;

// High-level WASM-friendly wrapper around Qlever
// This class provides a simplified C++ interface that can be easily exposed to
// JavaScript via Emscripten and wasm-bindgen.
class QleverWasmWrapper {
 private:
  std::unique_ptr<Qlever> qlever_;
  std::string lastErrorMessage_;
  bool isInitialized_;

 public:
  QleverWasmWrapper();
  ~QleverWasmWrapper();

  // Initialize the wrapper with an index basename
  // The index must have been previously built at:
  //   <indexBasename>.PSO, <indexBasename>.POS, etc.
  bool init(const std::string& indexBasename);

  // Execute a SPARQL query and return results as JSON string
  // Supports: SELECT, CONSTRUCT, DESCRIBE, ASK queries
  // Returns JSON result in SPARQL JSON Results Format
  std::string query(const std::string& sparqlQuery);

  // Execute a SPARQL query with specific media type
  // mediaType can be: "application/sparql-results+json", "text/turtle", etc.
  std::string queryWithFormat(const std::string& sparqlQuery,
                              const std::string& mediaType);

  // Get the last error message (useful for debugging)
  std::string getLastError() const;

  // Check if the wrapper is initialized
  bool isInitialized() const;

  // Get QLever index statistics (number of triples, subjects, etc.)
  std::string getStats() const;

  // Clear all data from the index
  void clear();

 private:
  // Helper to set error message and return empty result
  std::string setError(const std::string& errorMessage);
};

}  // namespace qlever::wasm

#endif  // QLEVER_SRC_LIBQLEVER_QLEVER_WASM_WRAPPER_H

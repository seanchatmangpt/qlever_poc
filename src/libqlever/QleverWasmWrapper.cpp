// Copyright 2025 The QLever Authors
// SPDX-License-Identifier: Apache-2.0

#include "libqlever/QleverWasmWrapper.h"

#include "libqlever/Qlever.h"
#include "util/Exception.h"
#include "util/http/MediaTypes.h"

namespace qlever::wasm {

QleverWasmWrapper::QleverWasmWrapper()
    : qlever_(nullptr), isInitialized_(false) {}

QleverWasmWrapper::~QleverWasmWrapper() = default;

bool QleverWasmWrapper::init(const std::string& indexBasename) {
  try {
    EngineConfig config;
    config.baseName_ = indexBasename;
    config.memoryLimit_ = ad_utility::MemorySize::gigabytes(4);  // WASM limit
    qlever_ = std::make_unique<Qlever>(config);
    isInitialized_ = true;
    lastErrorMessage_.clear();
    return true;
  } catch (const std::exception& e) {
    lastErrorMessage_ = std::string("Failed to initialize QLever: ") + e.what();
    isInitialized_ = false;
    return false;
  }
}

std::string QleverWasmWrapper::query(const std::string& sparqlQuery) {
  return queryWithFormat(sparqlQuery, "application/sparql-results+json");
}

std::string QleverWasmWrapper::queryWithFormat(const std::string& sparqlQuery,
                                               const std::string& mediaType) {
  if (!isInitialized_) {
    return setError("QLever not initialized. Call init() first.");
  }

  try {
    // Convert media type string to enum
    ad_utility::MediaType mediaTypeEnum =
        ad_utility::MediaType::fromString(mediaType);
    if (!mediaTypeEnum) {
      return setError("Unsupported media type: " + mediaType);
    }

    // Execute query and return result
    std::string result = qlever_->query(sparqlQuery, mediaTypeEnum.value());
    lastErrorMessage_.clear();
    return result;
  } catch (const std::exception& e) {
    return setError("Query execution failed: " + std::string(e.what()));
  } catch (...) {
    return setError("Unknown error during query execution");
  }
}

std::string QleverWasmWrapper::getLastError() const {
  return lastErrorMessage_;
}

bool QleverWasmWrapper::isInitialized() const { return isInitialized_; }

std::string QleverWasmWrapper::getStats() const {
  if (!isInitialized_) {
    return "QLever not initialized";
  }

  try {
    // Execute a COUNT query to get statistics
    std::string statsQuery = "SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }";
    return qlever_->query(statsQuery);
  } catch (const std::exception& e) {
    return std::string("Error getting stats: ") + e.what();
  }
}

void QleverWasmWrapper::clear() {
  if (isInitialized_) {
    // Note: Full clear would require a SPARQL UPDATE DELETE
    // For WASM, we might just reinitialize the index
    try {
      // This is a placeholder - actual implementation depends on
      // whether QLever supports in-memory clearing
      qlever_.reset();
      isInitialized_ = false;
      lastErrorMessage_.clear();
    } catch (const std::exception& e) {
      lastErrorMessage_ = std::string("Clear failed: ") + e.what();
    }
  }
}

std::string QleverWasmWrapper::setError(const std::string& errorMessage) {
  lastErrorMessage_ = errorMessage;
  // Return JSON error response
  return R"({"error":")" + errorMessage + R"("})";
}

}  // namespace qlever::wasm

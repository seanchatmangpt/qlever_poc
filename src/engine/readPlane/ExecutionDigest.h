// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: AI Agent Implementation
//
// Purpose: ExecutionDigest - Deterministic hash representing a query execution
// (EPIC 4 - Deterministic Execution Envelope)
//
// ExecutionDigest provides a portable, deterministic representation of a query
// execution that can be compared across machines and epochs. Two executions
// are "the same" IFF their digest_hash values are identical.
//
// Invariants (from EPIC4_SHARED_INVARIANTS.md):
// - Epoch-independent: Same query, different epochs -> same digest if
// execution identical
// - Deterministic: No wall-clock times, no pointers, no architecture-specific
// values
// - Portable: Can be serialized to JSON-LD and compared across machines
// - Fail-closed: Computation errors abort, never silent fallback

#ifndef QLEVER_SRC_ENGINE_READPLANE_EXECUTIONDIGEST_H
#define QLEVER_SRC_ENGINE_READPLANE_EXECUTIONDIGEST_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "engine/queryCanonical/QueryFingerprint.h"
#include "util/json.h"

namespace readPlane {

// Forward declarations for types used in compute()
struct ResourceMetrics;
struct ResultMetadata;
struct PlanInfo;

// ResourceMetrics - Captures cache and memory behavior during execution.
// All values must be deterministic (no wall-clock times).
struct ResourceMetrics {
  // Cache tier utilization counts
  uint64_t bytes_cache_hits = 0;
  uint64_t plan_cache_hits = 0;
  uint64_t negative_cache_hits = 0;

  // Memory behavior (page-level, not byte-level for determinism)
  uint64_t memory_pages_accessed = 0;

  // Result serialization metadata
  uint64_t result_bytes_written = 0;

  // Default constructor
  ResourceMetrics() = default;

  // Serialize to deterministic byte string for hashing
  [[nodiscard]] std::string toCanonicalBytes() const;

  // Equality for testing
  bool operator==(const ResourceMetrics& other) const = default;
};

// ResultMetadata - Captures result structure without including result content.
// This allows comparing execution behavior without storing actual query
// results.
struct ResultMetadata {
  // Result shape (structure, not content)
  uint64_t column_count = 0;
  uint64_t row_count = 0;

  // Column type information (deterministic encoding)
  std::vector<std::string> column_types;

  // Serialization format used
  std::string output_format;  // "JSON", "CSV", "TSV", "SPARQL_JSON", etc.

  // Default constructor
  ResultMetadata() = default;

  // Serialize to deterministic byte string for hashing
  [[nodiscard]] std::string toCanonicalBytes() const;

  // Equality for testing
  bool operator==(const ResultMetadata& other) const = default;
};

// PlanInfo - Captures QueryExecutionTree structure for plan hash computation.
// Must be serialized deterministically (sorted keys, no pointers).
struct PlanInfo {
  // Canonical cache key from QueryExecutionTree (already deterministic)
  std::string plan_cache_key;

  // Operation descriptor chain (root to leaves, depth-first)
  std::vector<std::string> operation_descriptors;

  // Plan cost estimate (deterministic, from query planner)
  uint64_t cost_estimate = 0;

  // Plan size estimate (deterministic, from query planner)
  uint64_t size_estimate = 0;

  // Default constructor
  PlanInfo() = default;

  // Serialize to deterministic byte string for hashing
  [[nodiscard]] std::string toCanonicalBytes() const;

  // Equality for testing
  bool operator==(const PlanInfo& other) const = default;
};

// ExecutionDigest - Complete deterministic identifier for a query execution.
//
// This structure captures all aspects of execution that affect correctness
// and reproducibility. The digest is independent of wall-clock time and
// machine-specific values.
//
// EPIC 4 Requirements (from EPIC4_SHARED_INVARIANTS.md section 2.4):
// - query_fingerprint_sha256: SHA256 of QueryFingerprint (from EPIC 3)
// - plan_hash: SHA256 of compiled QueryExecutionTree structure
// - resource_signature: Hash of memory/cache/serialization behavior
// - result_length_hash: SHA256(result.size())
// - result_shape_hash: SHA256(result structure metadata)
// - digest_hash: SHA256 of all above
struct ExecutionDigest {
  // ========== COMPONENT HASHES (all SHA256 hex strings, 64 chars) ==========

  // Query identity from EPIC 3 (fingerprint serialized and hashed)
  std::string query_fingerprint_sha256;

  // Plan structure hash (deterministic plan serialization)
  std::string plan_hash;

  // Resource envelope hash (memory + cache + serialization behavior)
  std::string resource_signature;

  // Result size hash (SHA256 of result.size() as string)
  std::string result_length_hash;

  // Result structure hash (column count, types, format - NOT content)
  std::string result_shape_hash;

  // ========== FINAL DIGEST ==========

  // SHA256(query_fingerprint_sha256 || plan_hash || resource_signature ||
  //        result_length_hash || result_shape_hash)
  // Order matters for determinism.
  std::string digest_hash;

  // ========== METADATA (not included in digest_hash) ==========

  // Format version for forward compatibility
  static constexpr const char* FORMAT_VERSION = "1.0";

  // ========== CONSTRUCTORS ==========

  // Default constructor (empty digest)
  ExecutionDigest() = default;

  // ========== FACTORY METHODS ==========

  // Compute ExecutionDigest from query context and execution results.
  // This is the primary entry point for creating digests.
  //
  // Parameters:
  // - fingerprint: QueryFingerprint from EPIC 3 (must be valid)
  // - plan: PlanInfo capturing QueryExecutionTree structure
  // - resources: ResourceMetrics capturing cache/memory behavior
  // - result: ResultMetadata capturing result structure (not content)
  // - result_size: Size of the result (for result_length_hash)
  //
  // Returns: Fully computed ExecutionDigest
  //
  // Throws: std::runtime_error if any input is invalid (fail-closed)
  [[nodiscard]] static ExecutionDigest compute(
      const queryCanonical::QueryFingerprint& fingerprint, const PlanInfo& plan,
      const ResourceMetrics& resources, const ResultMetadata& result,
      uint64_t result_size);

  // ========== COMPARISON METHODS ==========

  // Two digests match IFF digest_hash is identical.
  // Component hashes are for debugging only.
  [[nodiscard]] bool matches(const ExecutionDigest& other) const {
    return digest_hash == other.digest_hash;
  }

  // Equality operator (same as matches)
  bool operator==(const ExecutionDigest& other) const {
    return matches(other);
  }

  bool operator!=(const ExecutionDigest& other) const {
    return !matches(other);
  }

  // ========== SERIALIZATION ==========

  // Serialize to deterministic JSON-LD format.
  // Output is sorted by field names for reproducibility.
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;

  // Deserialize from JSON-LD format.
  // Throws std::runtime_error if format is invalid.
  [[nodiscard]] static ExecutionDigest fromJsonLD(
      const nlohmann::ordered_json& json);

  // ========== VALIDATION ==========

  // Check if all hash fields are populated and valid.
  // Returns false if any field is empty or malformed.
  [[nodiscard]] bool isValid() const;

  // Verify that digest_hash is correctly computed from components.
  // Used for integrity checking after deserialization.
  [[nodiscard]] bool verifyIntegrity() const;

  // ========== DEBUG ==========

  // Human-readable string representation (for logging only).
  [[nodiscard]] std::string toString() const;

  // ========== INTERNAL HELPERS ==========

 private:
  // Compute SHA256 hash and return as 64-char hex string.
  [[nodiscard]] static std::string sha256Hex(std::string_view input);

  // Compute the final digest_hash from component hashes.
  [[nodiscard]] static std::string computeFinalDigest(
      const std::string& query_fp, const std::string& plan,
      const std::string& resource, const std::string& result_len,
      const std::string& result_shape);
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_EXECUTIONDIGEST_H

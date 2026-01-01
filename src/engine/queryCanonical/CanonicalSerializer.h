// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALSERIALIZER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALSERIALIZER_H

#include <cstdint>
#include <string>
#include <vector>

#include "parser/ParsedQuery.h"

namespace queryCanonical {

// Operator type enumeration for canonical serialization.
// Each operator in the SPARQL algebra is assigned a unique byte value
// to ensure deterministic serialization of query shapes.
enum class OperatorType : uint8_t {
  // Query types
  SELECT = 0x01,
  CONSTRUCT = 0x02,
  ASK = 0x03,
  DESCRIBE = 0x04,
  UPDATE = 0x05,

  // Basic graph patterns
  BASIC_GRAPH_PATTERN = 0x10,
  TRIPLE_PATTERN = 0x11,

  // Solution modifiers
  DISTINCT = 0x20,
  REDUCED = 0x21,
  ORDER_BY = 0x22,
  LIMIT = 0x23,
  OFFSET = 0x24,

  // Graph patterns
  GROUP_GRAPH_PATTERN = 0x30,
  OPTIONAL = 0x31,
  UNION = 0x32,
  MINUS = 0x33,
  FILTER = 0x34,
  BIND = 0x35,
  VALUES = 0x36,
  GRAPH = 0x37,

  // Subqueries and special operations
  SUBQUERY = 0x40,
  SERVICE = 0x41,
  TRANS_PATH = 0x42,

  // Aggregation and grouping
  GROUP_BY = 0x50,
  HAVING = 0x51,

  // QLever-specific extensions
  PATH_QUERY = 0x60,
  SPATIAL_QUERY = 0x61,
  TEXT_SEARCH_QUERY = 0x62,
  MATERIALIZED_VIEW_QUERY = 0x63,
  NAMED_CACHED_RESULT = 0x64,
  LOAD = 0x65,

  // Sentinel value for unknown operators
  UNKNOWN = 0xFF
};

// Feature flags for query characteristics (bitfield).
// These flags capture query-level properties that affect query shape.
enum class FeatureFlags : uint16_t {
  NONE = 0x0000,
  DISTINCT = 0x0001,
  REDUCED = 0x0002,
  HAS_ORDER_BY = 0x0004,
  HAS_LIMIT = 0x0008,
  HAS_OFFSET = 0x0010,
  HAS_GROUP_BY = 0x0020,
  HAS_HAVING = 0x0040,
  HAS_SUBQUERY = 0x0080,
  HAS_OPTIONAL = 0x0100,
  HAS_UNION = 0x0200,
  HAS_MINUS = 0x0400,
  HAS_SERVICE = 0x0800,
  HAS_FILTER = 0x1000,
  HAS_BIND = 0x2000,
  HAS_VALUES = 0x4000,
  IS_CONSTRUCT = 0x8000
};

// Bitwise OR operator for combining feature flags
inline FeatureFlags operator|(FeatureFlags a, FeatureFlags b) {
  return static_cast<FeatureFlags>(static_cast<uint16_t>(a) |
                                   static_cast<uint16_t>(b));
}

// Bitwise OR-assignment operator for feature flags
inline FeatureFlags& operator|=(FeatureFlags& a, FeatureFlags b) {
  a = a | b;
  return a;
}

// CanonicalSerializer converts a normalized ParsedQuery into a deterministic
// byte representation (QSHAPE format) suitable for hashing.
//
// The serialization process:
// 1. Writes version header ("QSHAPE\x01")
// 2. Encodes feature flags (DISTINCT, REDUCED, etc.)
// 3. Performs depth-first traversal of operator tree
// 4. For each operator: writes OpType byte + operator-specific data
// 5. Appends constants table (already normalized)
//
// Design invariants:
// - Deterministic: same ParsedQuery → identical bytes across runs
// - Versioned: format version in header prevents cache invalidation surprises
// - No floating point: only integers and strings for consistency
// - Platform-independent: fixed endianness (little-endian)
class CanonicalSerializer {
 public:
  // Construct a CanonicalSerializer for a normalized ParsedQuery.
  // The ParsedQuery must have been normalized by prior EPIC 2 steps:
  // - IRIs expanded (no prefixes)
  // - Variables renamed to canonical names (?v0, ?v1, ...)
  // - Constants extracted and replaced with placeholders
  // - Triple patterns reordered deterministically
  explicit CanonicalSerializer(const ParsedQuery& query);

  // Serialize the query to canonical byte form (QSHAPE v1 format).
  // Returns a byte vector suitable for SHA-256 hashing to produce qshape_sha256.
  //
  // Format structure:
  // - 8 bytes: "QSHAPE\x01" (version header)
  // - 2 bytes: feature flags (little-endian uint16)
  // - Variable length: operator tree (depth-first traversal)
  // - Variable length: constants table
  std::vector<uint8_t> serializeToCanonicalForm() const;

  // Serialize the query to a human-readable debug string (QSHAPE format).
  // This is optional and used for debugging/logging purposes.
  // The format is similar to SPARQL algebra but includes operator types and
  // placeholder information.
  std::string serializeToDebugString() const;

 private:
  const ParsedQuery& query_;

  // Compute feature flags from the query structure
  FeatureFlags computeFeatureFlags() const;

  // Serialize the operator tree in depth-first order
  void serializeOperatorTree(std::vector<uint8_t>& buffer) const;

  // Serialize the query header (SELECT, CONSTRUCT, ASK, etc.)
  void serializeQueryHeader(std::vector<uint8_t>& buffer) const;

  // Serialize a graph pattern and its children recursively
  void serializeGraphPattern(const parsedQuery::GraphPattern& pattern,
                             std::vector<uint8_t>& buffer) const;

  // Serialize a specific graph pattern operation
  void serializeGraphPatternOperation(
      const parsedQuery::GraphPatternOperation& op,
      std::vector<uint8_t>& buffer) const;

  // Serialize solution modifiers (ORDER BY, LIMIT, OFFSET, GROUP BY, HAVING)
  void serializeSolutionModifiers(std::vector<uint8_t>& buffer) const;

  // Helper to write a string in deterministic format (length-prefixed)
  static void writeString(std::vector<uint8_t>& buffer, const std::string& str);

  // Helper to write a variable in deterministic format
  static void writeVariable(std::vector<uint8_t>& buffer, const Variable& var);

  // Helper to write a uint8_t value
  static void writeUInt8(std::vector<uint8_t>& buffer, uint8_t value);

  // Helper to write a uint16_t value (little-endian)
  static void writeUInt16(std::vector<uint8_t>& buffer, uint16_t value);

  // Helper to write a uint32_t value (little-endian)
  static void writeUInt32(std::vector<uint8_t>& buffer, uint32_t value);

  // Helper to write a uint64_t value (little-endian)
  static void writeUInt64(std::vector<uint8_t>& buffer, uint64_t value);

  // Debug string builders (mirror the binary serialization structure)
  std::string debugOperatorTree() const;
  std::string debugGraphPattern(const parsedQuery::GraphPattern& pattern,
                                int indent = 0) const;
  std::string debugGraphPatternOperation(
      const parsedQuery::GraphPatternOperation& op, int indent = 0) const;
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALSERIALIZER_H

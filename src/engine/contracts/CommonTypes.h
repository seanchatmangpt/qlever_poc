// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_CONTRACTS_COMMONTYPES_H
#define QLEVER_SRC_ENGINE_CONTRACTS_COMMONTYPES_H

#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>

#include "global/Id.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Literal.h"

namespace qlever::contracts {

// Execution outcome for validation/rules/N3 services
enum class Outcome {
  OK,      // Operation succeeded without violations
  GUARDED, // Operation succeeded but guard condition triggered
  ERROR    // Operation failed with error
};

// Severity levels for violations and diagnostics
enum class Severity {
  INFO,     // Informational message
  WARNING,  // Warning that doesn't prevent execution
  ERROR,    // Error that prevents correct execution
  CRITICAL  // Critical error requiring immediate attention
};

// String representation for Outcome
constexpr std::string_view outcomeToString(Outcome outcome) {
  switch (outcome) {
    case Outcome::OK:
      return "OK";
    case Outcome::GUARDED:
      return "GUARDED";
    case Outcome::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

// String representation for Severity
constexpr std::string_view severityToString(Severity severity) {
  switch (severity) {
    case Severity::INFO:
      return "INFO";
    case Severity::WARNING:
      return "WARNING";
    case Severity::ERROR:
      return "ERROR";
    case Severity::CRITICAL:
      return "CRITICAL";
    default:
      return "UNKNOWN";
  }
}

// Bounded string with compile-time size limit
// Prevents excessive memory allocation for violation messages
template <size_t MaxSize>
  requires(MaxSize > 0 && MaxSize <= 4096)
class BoundedString {
 private:
  std::string value_;

 public:
  static constexpr size_t max_size = MaxSize;

  BoundedString() = default;

  explicit BoundedString(std::string_view sv) {
    if (sv.size() > MaxSize) {
      value_ = std::string(sv.substr(0, MaxSize - 3)) + "...";
    } else {
      value_ = sv;
    }
  }

  explicit BoundedString(const std::string& s) : BoundedString(std::string_view(s)) {}

  const std::string& value() const& { return value_; }
  std::string value() && { return std::move(value_); }

  size_t size() const { return value_.size(); }
  bool empty() const { return value_.empty(); }

  operator std::string_view() const { return value_; }
};

// Common bounded string types
using ShortMessage = BoundedString<256>;
using MediumMessage = BoundedString<1024>;
using LongMessage = BoundedString<4096>;

// Node type classification for rendering
enum class NodeType {
  IRI,
  LITERAL,
  BLANK_NODE,
  VARIABLE,
  UNKNOWN
};

// Helper to determine node type from ValueId
// This is a simplified version - actual implementation would need
// access to the index to resolve IDs properly
inline NodeType classifyNodeById(Id id) {
  // This is a placeholder implementation
  // Real implementation would use ValueId's type information
  return NodeType::UNKNOWN;
}

// Helper to render a node ID as a string (IRI, Literal, or Blank node)
// Returns a bounded string representation suitable for violation messages
inline ShortMessage renderNodeId(Id id, const std::string& vocabularyLookup = "") {
  // Placeholder implementation - would need actual vocabulary access
  if (!vocabularyLookup.empty()) {
    return ShortMessage(vocabularyLookup);
  }
  return ShortMessage("_:node" + std::to_string(id.getBits()));
}

// Helper to render an IRI as a short string
inline ShortMessage renderIri(const ad_utility::triple_component::Iri& iri) {
  auto content = iri.getContent();
  return ShortMessage(std::string(content.data(), content.size()));
}

// Helper to render a Literal as a short string
inline ShortMessage renderLiteral(const ad_utility::triple_component::Literal& literal) {
  auto content = literal.getContent();
  return ShortMessage(std::string(content.data(), content.size()));
}

// Timestamp helper for violation tracking
// Returns milliseconds since epoch
inline uint64_t currentTimestampMs() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

}  // namespace qlever::contracts

#endif  // QLEVER_SRC_ENGINE_CONTRACTS_COMMONTYPES_H

// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTPLACEHOLDER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTPLACEHOLDER_H

#include <cstdint>
#include <string>

namespace queryCanonical {

// Categorization of constant types in SPARQL queries for placeholder
// substitution. This classification enables semantic-preserving query shape
// canonicalization where constants of the same type can be interchanged.
enum class PlaceholderType {
  IRI,              // IRIs and URIs
  PLAIN_LITERAL,    // Untyped literals without language tags
  LANG_LITERAL,     // Literals with language tags (e.g., "hello"@en)
  NUMERIC_LITERAL,  // Numeric datatypes (int, float, double, decimal, etc.)
  TEMPORAL_LITERAL,  // Temporal datatypes (date, dateTime, gYear, etc.)
  BOOLEAN_LITERAL,   // xsd:boolean values
  REGEX_PATTERN      // Regular expression patterns in FILTER/STRSTARTS
};

// Represents a placeholder for a constant that has been extracted from a query.
// During canonicalization, constants are replaced with placeholders to produce
// a query shape independent of specific values.
struct Placeholder {
  // The semantic category of this placeholder
  PlaceholderType placeholderType;

  // Position of the original constant in the extracted parameters table
  // (used for reconstruction and SHA-256 computation)
  uint32_t index;

  // Original value preserved for debugging and error messages
  std::string originalValue;

  // Default constructor for easier usage in containers
  Placeholder() = default;

  // Constructor with all fields
  Placeholder(PlaceholderType type, uint32_t idx, std::string value)
      : placeholderType(type), index(idx), originalValue(std::move(value)) {}
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTPLACEHOLDER_H

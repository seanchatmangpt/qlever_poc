#ifndef PARSER_SHEX_H
#define PARSER_SHEX_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include "absl/container/flat_hash_set.h"
#include "absl/container/flat_hash_map.h"
#include "../rdfTypes/Iri.h"
#include "../rdfTypes/Literal.h"

namespace shex {

// ============================================================================
// Core ShEx Data Types (80/20: Essential constraints only)
// ============================================================================

enum class ValueType { IRI, LITERAL, BNODE };

enum class Cardinality { EXACTLY_ONE, ZERO_OR_ONE, ZERO_OR_MORE, ONE_OR_MORE };

struct ValueSetConstraint {
  std::optional<ValueType> valueType;
  absl::flat_hash_set<std::string> allowedIris;
  std::optional<std::string> datatypeRestriction;

  ValueSetConstraint() = default;
  bool validate(const std::string& value, ValueType type) const;
};

struct PropertyShape {
  std::string predicate;
  ValueSetConstraint valueConstraint;
  Cardinality cardinality = Cardinality::EXACTLY_ONE;
  bool inverse = false;
  std::optional<std::string> nodeKind;

  PropertyShape(const std::string& pred) : predicate(pred) {}
  bool validate(const std::string& value, ValueType type) const;
};

struct Shape {
  std::string id;
  std::vector<PropertyShape> properties;
  bool closed = false;

  Shape(const std::string& shapeId) : id(shapeId) {}
  void addProperty(const PropertyShape& prop) { properties.push_back(prop); }

  struct ValidationResult {
    bool isValid;
    std::vector<std::string> errors;
  };

  ValidationResult validate(const std::map<std::string,
                            std::vector<std::pair<std::string, ValueType>>>& nodeData) const;
};

// ============================================================================
// ShEx Schema (Collection of Shapes)
// ============================================================================

class ShExSchema {
 public:
  void addShape(const Shape& shape);
  const Shape* getShape(const std::string& shapeId) const;
  bool hasShape(const std::string& shapeId) const;
  const absl::flat_hash_map<std::string, Shape>& getShapes() const {
    return shapes_;
  }

 private:
  absl::flat_hash_map<std::string, Shape> shapes_;
};

// ============================================================================
// ShEx Parser (80/20: Parse basic ShEx syntax)
// ============================================================================

class ShExParser {
 public:
  ShExParser() = default;

  // Parse ShEx from string format
  // Simple format: shape shape_id { prop pred cardinality constraint; ... }
  std::optional<ShExSchema> parse(const std::string& input);

  std::string getLastError() const { return lastError_; }

 private:
  std::string lastError_;

  std::optional<Shape> parseShape(const std::string& input, size_t& pos);
  std::optional<PropertyShape> parseProperty(const std::string& input, size_t& pos);
  std::optional<Cardinality> parseCardinality(const std::string& input);
  std::optional<ValueSetConstraint> parseValueConstraint(const std::string& input);

  void skipWhitespace(const std::string& input, size_t& pos);
  std::string readWord(const std::string& input, size_t& pos);
  void setError(const std::string& msg) { lastError_ = msg; }
};

// ============================================================================
// Validator for RDF Conformance to ShEx
// ============================================================================

class ShExValidator {
 public:
  explicit ShExValidator(const ShExSchema& schema) : schema_(schema) {}

  struct ValidationReport {
    bool conforms;
    std::map<std::string, std::vector<std::string>> nodeErrors;
    std::vector<std::string> schemaErrors;
  };

  // Validate a node (identified by IRI) against its target shape
  ValidationReport validateNode(const std::string& nodeIri,
                               const std::string& targetShapeId,
                               const std::map<std::string,
                                 std::vector<std::pair<std::string, ValueType>>>& data);

  // Validate all nodes in a dataset
  ValidationReport validateDataset(
      const std::map<std::string,
        std::map<std::string, std::vector<std::pair<std::string, ValueType>>>>& dataset,
      const std::map<std::string, std::string>& nodeToShapeMapping);

 private:
  const ShExSchema& schema_;
};

}  // namespace shex

#endif  // PARSER_SHEX_H

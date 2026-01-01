#ifndef PARSER_SHAPE_SERIALIZER_H
#define PARSER_SHAPE_SERIALIZER_H

#include <string>
#include <memory>
#include <optional>
#include <map>
#include <vector>
#include <fstream>
#include <stdexcept>

#include "ShEx.h"
#include "../util/json.h"

namespace shex {

// ============================================================================
// Serialization Exceptions
// ============================================================================

class SerializationException : public std::runtime_error {
 public:
  explicit SerializationException(const std::string& msg)
      : std::runtime_error(msg) {}
};

class DeserializationException : public std::runtime_error {
 public:
  DeserializationException(const std::string& msg, size_t line = 0,
                          size_t column = 0)
      : std::runtime_error(formatMessage(msg, line, column)),
        line_(line),
        column_(column) {}

  size_t getLine() const { return line_; }
  size_t getColumn() const { return column_; }

 private:
  size_t line_;
  size_t column_;

  static std::string formatMessage(const std::string& msg, size_t line,
                                   size_t column) {
    if (line == 0 && column == 0) {
      return msg;
    }
    return msg + " at line " + std::to_string(line) + ", column " +
           std::to_string(column);
  }
};

// ============================================================================
// Serialization Format Enumeration
// ============================================================================

enum class SerializationFormat {
  JSON_LD,   // W3C ShEx JSON-LD format
  TURTLE,    // ShExC compact Turtle-like syntax
  AUTO       // Auto-detect from file extension or content
};

// ============================================================================
// Abstract Base Serializer
// ============================================================================

class ShapeSerializer {
 public:
  virtual ~ShapeSerializer() = default;

  // Serialize a single shape to string
  virtual std::string serializeShape(const Shape& shape) const = 0;

  // Serialize entire schema to string
  virtual std::string serializeSchema(const ShExSchema& schema) const = 0;

  // Deserialize schema from string
  virtual std::optional<ShExSchema> deserializeSchema(
      const std::string& input) = 0;

  // Serialize schema to file
  virtual void serializeToFile(const ShExSchema& schema,
                              const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
      throw SerializationException("Failed to open file for writing: " +
                                   filename);
    }
    file << serializeSchema(schema);
    if (!file) {
      throw SerializationException("Failed to write to file: " + filename);
    }
  }

  // Deserialize schema from file
  virtual std::optional<ShExSchema> deserializeFromFile(
      const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
      throw DeserializationException("Failed to open file for reading: " +
                                     filename);
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return deserializeSchema(content);
  }

  // Get format identifier
  virtual SerializationFormat getFormat() const = 0;

  // Get last error message
  virtual std::string getLastError() const { return lastError_; }

 protected:
  mutable std::string lastError_;

  void setError(const std::string& msg) const { lastError_ = msg; }
};

// ============================================================================
// JSON-LD Serializer (W3C ShEx JSON-LD Format)
// ============================================================================

class JsonLdSerializer : public ShapeSerializer {
 public:
  JsonLdSerializer() = default;

  std::string serializeShape(const Shape& shape) const override;
  std::string serializeSchema(const ShExSchema& schema) const override;
  std::optional<ShExSchema> deserializeSchema(
      const std::string& input) override;

  SerializationFormat getFormat() const override {
    return SerializationFormat::JSON_LD;
  }

  // Round-trip validation helper
  bool validateRoundTrip(const ShExSchema& original) const;

 private:
  nlohmann::json shapeToJson(const Shape& shape) const;
  nlohmann::json propertyShapeToJson(const PropertyShape& prop) const;
  nlohmann::json valueConstraintToJson(
      const ValueSetConstraint& constraint) const;
  std::string cardinalityToString(Cardinality card) const;

  std::optional<Shape> jsonToShape(const nlohmann::json& j);
  std::optional<PropertyShape> jsonToPropertyShape(const nlohmann::json& j);
  std::optional<ValueSetConstraint> jsonToValueConstraint(
      const nlohmann::json& j);
  std::optional<Cardinality> stringToCardinality(const std::string& str);
};

// ============================================================================
// Turtle/ShExC Serializer (Compact Turtle-like Syntax)
// ============================================================================

class TurtleSerializer : public ShapeSerializer {
 public:
  TurtleSerializer() = default;

  std::string serializeShape(const Shape& shape) const override;
  std::string serializeSchema(const ShExSchema& schema) const override;
  std::optional<ShExSchema> deserializeSchema(
      const std::string& input) override;

  SerializationFormat getFormat() const override {
    return SerializationFormat::TURTLE;
  }

  // Prefix management
  void addPrefix(const std::string& prefix, const std::string& iri);
  void clearPrefixes();
  std::string abbreviateIri(const std::string& iri) const;
  std::string expandIri(const std::string& abbreviated) const;

  // Pretty-printing options
  void setIndentation(size_t spaces) { indentation_ = spaces; }
  void setCompactMode(bool compact) { compactMode_ = compact; }

 private:
  std::map<std::string, std::string> prefixes_;
  std::map<std::string, std::string> reversePrefixes_;  // IRI -> prefix
  size_t indentation_ = 2;
  bool compactMode_ = false;

  std::string serializePropertyShape(const PropertyShape& prop) const;
  std::string serializeValueConstraint(
      const ValueSetConstraint& constraint) const;
  std::string serializeCardinality(Cardinality card) const;

  std::optional<PropertyShape> parsePropertyLine(const std::string& line);
  std::optional<Cardinality> parseCardinalitySymbol(const std::string& symbol);

  std::string indent(size_t level) const;
  void initializeCommonPrefixes();
};

// ============================================================================
// Serializer Factory (Format Detection & Creation)
// ============================================================================

class ShapeSerializerFactory {
 public:
  // Create serializer for specific format
  static std::unique_ptr<ShapeSerializer> createSerializer(
      SerializationFormat format);

  // Detect format from file extension
  static SerializationFormat detectFormatFromExtension(
      const std::string& filename);

  // Detect format from content analysis
  static SerializationFormat detectFormatFromContent(
      const std::string& content);

  // Auto-detect and deserialize
  static std::optional<ShExSchema> autoDeserialize(
      const std::string& content);

  // Auto-detect from file and deserialize
  static std::optional<ShExSchema> autoDeserializeFromFile(
      const std::string& filename);

 private:
  static bool looksLikeJson(const std::string& content);
  static bool looksLikeTurtle(const std::string& content);
};

// ============================================================================
// Utility Functions
// ============================================================================

// Round-trip test helper
bool testRoundTrip(const ShExSchema& schema, SerializationFormat format);

// Cross-format conversion
std::optional<ShExSchema> convertFormat(const std::string& input,
                                       SerializationFormat from,
                                       SerializationFormat to);

}  // namespace shex

#endif  // PARSER_SHAPE_SERIALIZER_H

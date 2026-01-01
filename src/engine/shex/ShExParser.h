#ifndef QLEVER_ENGINE_SHEX_SHEXPARSER_H
#define QLEVER_ENGINE_SHEX_SHEXPARSER_H

#include <optional>
#include <string>
#include <vector>
#include "ShExTypes.h"
#include "ShExConformance.h"

namespace shex {

// Parser for minimal ShEx compact syntax
// Supports: CLOSED, EXTRA, shape definitions, triple constraints
// Rejects: Regex constraints, inheritance, negation (with UNSUPPORTED error)
class ShExParser {
 public:
  ShExParser() = default;

  // Parse ShEx schema from compact syntax string
  // Returns ShapeMap on success, or throws exception with ErrorCode on failure
  ShapeMap parseSchema(const std::string& schemaText);

  // Parse a single shape definition
  // Returns nullptr if parsing fails
  std::shared_ptr<ShapeExpr> parseShape(const std::string& shapeText);

  // Get last parse error (if any)
  std::optional<ErrorCode> getLastError() const { return lastError_; }

  // Get last error message
  std::string getLastErrorMessage() const { return lastErrorMessage_; }

  // Check if schema uses only supported features
  bool validateFeatures(const std::string& schemaText);

 private:
  // Internal parsing state
  size_t pos_ = 0;
  std::string text_;
  std::optional<ErrorCode> lastError_;
  std::string lastErrorMessage_;

  // Parsing helper methods
  void skipWhitespace();
  void skipComments();
  bool match(const std::string& str);
  bool matchChar(char c);
  std::string parseIdentifier();
  std::string parseIRI();
  std::string parsePrefix();
  std::optional<int> parseInt();

  // Parse shape components
  bool parseShapeDefinition(ShapeMap& shapeMap);
  std::shared_ptr<ShapeExpr> parseShapeBody(const std::string& shapeId);
  std::optional<TripleConstraint> parseTripleConstraint();
  std::optional<ValueType> parseValueType();
  std::optional<ShExNodeKind> parseNodeKind();

  // Parse modifiers
  bool parseClosedModifier(ShapeExpr& shape);
  bool parseExtraModifier(ShapeExpr& shape);

  // Feature detection
  bool containsUnsupportedFeature(const std::string& text);
  void setError(ErrorCode code, const std::string& message);

  // Prefix handling
  std::unordered_map<std::string, std::string> prefixes_;
  std::string expandPrefix(const std::string& prefixedName);

  // Current character
  char current() const;
  bool isAtEnd() const;
  void advance();
};

}  // namespace shex

#endif  // QLEVER_ENGINE_SHEX_SHEXPARSER_H

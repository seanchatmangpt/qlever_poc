#include "ShExParser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace shex {

// Parse ShEx schema from compact syntax
ShapeMap ShExParser::parseSchema(const std::string& schemaText) {
  text_ = schemaText;
  pos_ = 0;
  lastError_ = std::nullopt;
  lastErrorMessage_.clear();
  prefixes_.clear();

  ShapeMap shapeMap;

  // Check for unsupported features first
  if (containsUnsupportedFeature(schemaText)) {
    throw std::runtime_error(
        "Schema contains unsupported ShEx features: " + lastErrorMessage_);
  }

  // Parse prefix declarations
  skipWhitespace();
  while (match("PREFIX") || match("prefix")) {
    parsePrefix();
    skipWhitespace();
  }

  // Parse shape definitions
  while (!isAtEnd()) {
    skipWhitespace();
    skipComments();
    if (isAtEnd()) break;

    if (!parseShapeDefinition(shapeMap)) {
      if (lastError_.has_value()) {
        throw std::runtime_error("Parse error: " + lastErrorMessage_);
      }
      break;
    }
  }

  return shapeMap;
}

// Parse a single shape definition
std::shared_ptr<ShapeExpr> ShExParser::parseShape(
    const std::string& shapeText) {
  text_ = shapeText;
  pos_ = 0;
  lastError_ = std::nullopt;
  lastErrorMessage_.clear();

  skipWhitespace();
  std::string shapeId = parseIRI();
  if (shapeId.empty()) {
    setError(ErrorCode::PARSE_ERROR, "Expected shape IRI");
    return nullptr;
  }

  return parseShapeBody(shapeId);
}

// Skip whitespace
void ShExParser::skipWhitespace() {
  while (!isAtEnd() && std::isspace(current())) {
    advance();
  }
}

// Skip comments (# to end of line)
void ShExParser::skipComments() {
  while (match("#")) {
    while (!isAtEnd() && current() != '\n') {
      advance();
    }
    if (!isAtEnd()) advance();  // Skip newline
    skipWhitespace();
  }
}

// Match a string at current position
bool ShExParser::match(const std::string& str) {
  if (pos_ + str.length() > text_.length()) {
    return false;
  }

  if (text_.substr(pos_, str.length()) == str) {
    pos_ += str.length();
    return true;
  }
  return false;
}

// Match a single character
bool ShExParser::matchChar(char c) {
  if (current() == c) {
    advance();
    return true;
  }
  return false;
}

// Parse identifier (alphanumeric + underscore)
std::string ShExParser::parseIdentifier() {
  skipWhitespace();
  std::string id;

  if (!std::isalpha(current()) && current() != '_') {
    return "";
  }

  while (!isAtEnd() &&
         (std::isalnum(current()) || current() == '_' || current() == '-')) {
    id += current();
    advance();
  }

  return id;
}

// Parse IRI (simplified: <...> or prefix:name)
std::string ShExParser::parseIRI() {
  skipWhitespace();

  // Full IRI: <...>
  if (matchChar('<')) {
    std::string iri;
    while (!isAtEnd() && current() != '>') {
      iri += current();
      advance();
    }
    if (!matchChar('>')) {
      setError(ErrorCode::PARSE_ERROR, "Unclosed IRI");
      return "";
    }
    return iri;
  }

  // Prefixed name: prefix:name
  std::string prefix = parseIdentifier();
  if (matchChar(':')) {
    std::string localName = parseIdentifier();
    return expandPrefix(prefix + ":" + localName);
  }

  return prefix;
}

// Parse prefix declaration
std::string ShExParser::parsePrefix() {
  skipWhitespace();
  std::string prefixName = parseIdentifier();
  skipWhitespace();

  if (!matchChar(':')) {
    setError(ErrorCode::PARSE_ERROR, "Expected ':' after prefix name");
    return "";
  }

  skipWhitespace();
  std::string iri = parseIRI();
  prefixes_[prefixName + ":"] = iri;

  return prefixName;
}

// Parse integer
std::optional<int> ShExParser::parseInt() {
  skipWhitespace();
  std::string numStr;

  while (!isAtEnd() && std::isdigit(current())) {
    numStr += current();
    advance();
  }

  if (numStr.empty()) {
    return std::nullopt;
  }

  return std::stoi(numStr);
}

// Parse shape definition
bool ShExParser::parseShapeDefinition(ShapeMap& shapeMap) {
  skipWhitespace();

  // Parse shape IRI
  std::string shapeId = parseIRI();
  if (shapeId.empty()) {
    return false;
  }

  skipWhitespace();

  // Expect '{'
  if (!matchChar('{')) {
    setError(ErrorCode::PARSE_ERROR,
             "Expected '{' after shape ID: " + shapeId);
    return false;
  }

  auto shape = parseShapeBody(shapeId);
  if (!shape) {
    return false;
  }

  shapeMap.addShape(shape);
  return true;
}

// Parse shape body
std::shared_ptr<ShapeExpr> ShExParser::parseShapeBody(
    const std::string& shapeId) {
  auto shape = std::make_shared<ShapeExpr>(shapeId);

  skipWhitespace();

  // Parse CLOSED modifier
  parseClosedModifier(*shape);
  skipWhitespace();

  // Parse EXTRA modifier
  parseExtraModifier(*shape);
  skipWhitespace();

  // Parse triple constraints
  while (!isAtEnd() && current() != '}') {
    auto tc = parseTripleConstraint();
    if (!tc.has_value()) {
      break;
    }

    shape->addTripleConstraint(tc.value());
    skipWhitespace();

    // Optional semicolon or comma separator
    if (matchChar(';') || matchChar(',')) {
      skipWhitespace();
    }
  }

  // Expect '}'
  if (!matchChar('}')) {
    setError(ErrorCode::PARSE_ERROR, "Expected '}' at end of shape body");
    return nullptr;
  }

  return shape;
}

// Parse triple constraint
std::optional<TripleConstraint> ShExParser::parseTripleConstraint() {
  skipWhitespace();

  // Parse predicate IRI
  std::string predicate = parseIRI();
  if (predicate.empty()) {
    return std::nullopt;
  }

  TripleConstraint tc(predicate);
  skipWhitespace();

  // Parse value constraints in brackets [...]
  if (matchChar('[')) {
    skipWhitespace();

    // Parse valueType or nodeKind
    while (!isAtEnd() && current() != ']') {
      skipWhitespace();

      // Check for datatype constraint
      auto valueType = parseValueType();
      if (valueType.has_value()) {
        tc.valueType = valueType;
        skipWhitespace();
        continue;
      }

      // Check for node kind constraint
      auto nodeKind = parseNodeKind();
      if (nodeKind.has_value()) {
        tc.nodeKind = nodeKind;
        skipWhitespace();
        continue;
      }

      // Unknown constraint - skip
      advance();
    }

    if (!matchChar(']')) {
      setError(ErrorCode::PARSE_ERROR, "Expected ']' after value constraints");
      return std::nullopt;
    }
  }

  skipWhitespace();

  // Parse cardinality modifiers
  if (matchChar('*')) {
    // Zero or more
    tc.minCount = 0;
    tc.maxCount = -1;  // Unbounded
  } else if (matchChar('+')) {
    // One or more
    tc.minCount = 1;
    tc.maxCount = -1;  // Unbounded
  } else if (matchChar('?')) {
    // Zero or one
    tc.minCount = 0;
    tc.maxCount = 1;
  } else if (matchChar('{')) {
    // Explicit cardinality {min,max}
    auto min = parseInt();
    if (min.has_value()) {
      tc.minCount = min.value();
    }

    skipWhitespace();
    if (matchChar(',')) {
      skipWhitespace();
      auto max = parseInt();
      if (max.has_value()) {
        tc.maxCount = max.value();
      } else {
        tc.maxCount = -1;  // Unbounded
      }
    } else {
      tc.maxCount = tc.minCount;  // Exact count
    }

    skipWhitespace();
    if (!matchChar('}')) {
      setError(ErrorCode::PARSE_ERROR, "Expected '}' after cardinality");
      return std::nullopt;
    }
  }

  return tc;
}

// Parse value type (datatype constraint)
std::optional<ValueType> ShExParser::parseValueType() {
  skipWhitespace();

  // Check for common XSD datatypes
  std::vector<std::string> datatypes = {
      "xsd:string", "xsd:integer", "xsd:decimal", "xsd:boolean",
      "xsd:date",   "xsd:dateTime", "xsd:anyURI"};

  for (const auto& dt : datatypes) {
    if (match(dt)) {
      return ValueType(expandPrefix(dt));
    }
  }

  return std::nullopt;
}

// Parse node kind constraint
std::optional<ShExNodeKind> ShExParser::parseNodeKind() {
  skipWhitespace();

  if (match("IRI")) {
    return ShExNodeKind::IRI;
  } else if (match("BNODE")) {
    return ShExNodeKind::BlankNode;
  } else if (match("LITERAL")) {
    return ShExNodeKind::Literal;
  } else if (match("NONLITERAL")) {
    return ShExNodeKind::NonLiteral;
  }

  return std::nullopt;
}

// Parse CLOSED modifier
bool ShExParser::parseClosedModifier(ShapeExpr& shape) {
  skipWhitespace();

  if (match("CLOSED")) {
    shape.setClosed(true);
    return true;
  }

  return false;
}

// Parse EXTRA modifier
bool ShExParser::parseExtraModifier(ShapeExpr& shape) {
  skipWhitespace();

  if (match("EXTRA")) {
    skipWhitespace();

    // Parse list of extra properties
    while (!isAtEnd() && current() != '{' && current() != '}') {
      std::string prop = parseIRI();
      if (prop.empty()) {
        break;
      }
      shape.addExtraProperty(prop);
      skipWhitespace();

      // Optional comma separator
      matchChar(',');
      skipWhitespace();
    }

    return true;
  }

  return false;
}

// Check if schema contains unsupported features
bool ShExParser::containsUnsupportedFeature(const std::string& text) {
  // Check for regex patterns (/)
  if (text.find('/') != std::string::npos &&
      text.find("//") == std::string::npos) {  // Ignore comments
    setError(ErrorCode::UNSUPPORTED_SHEX_FEATURE,
             "Regex constraints are not supported");
    return true;
  }

  // Check for negation (NOT, !)
  if (text.find("NOT") != std::string::npos ||
      text.find(" ! ") != std::string::npos) {
    setError(ErrorCode::UNSUPPORTED_SHEX_FEATURE,
             "Negation is not supported");
    return true;
  }

  // Check for inheritance (EXTENDS, &)
  if (text.find("EXTENDS") != std::string::npos ||
      text.find(" & ") != std::string::npos) {
    setError(ErrorCode::UNSUPPORTED_SHEX_FEATURE,
             "Inheritance is not supported");
    return true;
  }

  // Check for semantic actions (%code%)
  if (text.find('%') != std::string::npos) {
    setError(ErrorCode::UNSUPPORTED_SHEX_FEATURE,
             "Semantic actions are not supported");
    return true;
  }

  return false;
}

// Expand prefixed name to full IRI
std::string ShExParser::expandPrefix(const std::string& prefixedName) {
  for (const auto& [prefix, iri] : prefixes_) {
    if (prefixedName.rfind(prefix, 0) == 0) {
      return iri + prefixedName.substr(prefix.length());
    }
  }
  return prefixedName;
}

// Set error state
void ShExParser::setError(ErrorCode code, const std::string& message) {
  lastError_ = code;
  lastErrorMessage_ = message;
}

// Get current character
char ShExParser::current() const {
  return isAtEnd() ? '\0' : text_[pos_];
}

// Check if at end of input
bool ShExParser::isAtEnd() const { return pos_ >= text_.length(); }

// Advance to next character
void ShExParser::advance() {
  if (!isAtEnd()) {
    pos_++;
  }
}

// Validate features
bool ShExParser::validateFeatures(const std::string& schemaText) {
  return !containsUnsupportedFeature(schemaText);
}

}  // namespace shex

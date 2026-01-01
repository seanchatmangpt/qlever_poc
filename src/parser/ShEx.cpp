#include "ShEx.h"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace shex {

// ============================================================================
// ValueSetConstraint Implementation
// ============================================================================

bool ValueSetConstraint::validate(const std::string& value, ValueType type) const {
  // If valueType is specified, check it first
  if (valueType.has_value() && valueType.value() != type) {
    return false;
  }

  // If specific IRIs are allowed, check against them
  if (!allowedIris.empty()) {
    return allowedIris.contains(value);
  }

  // If datatype restriction is specified, validate
  if (datatypeRestriction.has_value()) {
    // Simple check: if it's a literal, we could validate the datatype
    // For 80/20: just ensure it's not an IRI
    return type == ValueType::LITERAL;
  }

  // Default: allow everything
  return true;
}

// ============================================================================
// PropertyShape Implementation
// ============================================================================

bool PropertyShape::validate(const std::string& value, ValueType type) const {
  return valueConstraint.validate(value, type);
}

// ============================================================================
// Shape Implementation
// ============================================================================

Shape::ValidationResult Shape::validate(
    const std::map<std::string,
      std::vector<std::pair<std::string, ValueType>>>& nodeData) const {
  ValidationResult result{true, {}};

  // Check each property in the shape
  for (const auto& prop : properties) {
    auto it = nodeData.find(prop.predicate);

    // Check cardinality
    size_t count = (it != nodeData.end()) ? it->second.size() : 0;

    switch (prop.cardinality) {
      case Cardinality::EXACTLY_ONE:
        if (count != 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear exactly once (found " +
                                std::to_string(count) + ")");
        }
        break;
      case Cardinality::ZERO_OR_ONE:
        if (count > 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at most once (found " +
                                std::to_string(count) + ")");
        }
        break;
      case Cardinality::ZERO_OR_MORE:
        // Always valid
        break;
      case Cardinality::ONE_OR_MORE:
        if (count < 1) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " must appear at least once");
        }
        break;
    }

    // Validate each value against constraints
    if (it != nodeData.end()) {
      for (const auto& [value, type] : it->second) {
        if (!prop.validate(value, type)) {
          result.isValid = false;
          result.errors.push_back("Property " + prop.predicate +
                                " value '" + value + "' does not match constraints");
        }
      }
    }
  }

  return result;
}

// ============================================================================
// ShExSchema Implementation
// ============================================================================

void ShExSchema::addShape(const Shape& shape) { shapes_[shape.id] = shape; }

const Shape* ShExSchema::getShape(const std::string& shapeId) const {
  auto it = shapes_.find(shapeId);
  if (it != shapes_.end()) {
    return &it->second;
  }
  return nullptr;
}

bool ShExSchema::hasShape(const std::string& shapeId) const {
  return shapes_.count(shapeId) > 0;
}

// ============================================================================
// ShExParser Implementation
// ============================================================================

void ShExParser::skipWhitespace(const std::string& input, size_t& pos) {
  while (pos < input.length() && std::isspace(input[pos])) {
    ++pos;
  }
}

std::string ShExParser::readWord(const std::string& input, size_t& pos) {
  skipWhitespace(input, pos);
  std::string word;
  while (pos < input.length() && (std::isalnum(input[pos]) || input[pos] == '_' ||
                                  input[pos] == ':' || input[pos] == '/' ||
                                  input[pos] == '#' || input[pos] == '-' ||
                                  input[pos] == '.')) {
    word += input[pos];
    ++pos;
  }
  return word;
}

std::optional<Cardinality> ShExParser::parseCardinality(const std::string& input) {
  if (input == "?" || input == "*") {
    return Cardinality::ZERO_OR_ONE;
  }
  if (input == "*") {
    return Cardinality::ZERO_OR_MORE;
  }
  if (input == "+") {
    return Cardinality::ONE_OR_MORE;
  }
  if (input.empty()) {
    return Cardinality::EXACTLY_ONE;
  }
  return std::nullopt;
}

std::optional<ValueSetConstraint> ShExParser::parseValueConstraint(
    const std::string& input) {
  // Simple constraint parsing
  // Format: IRI, LITERAL, BNODE, or specific IRI values
  ValueSetConstraint constraint;

  if (input == "IRI") {
    constraint.valueType = ValueType::IRI;
  } else if (input == "LITERAL") {
    constraint.valueType = ValueType::LITERAL;
  } else if (input == "BNODE") {
    constraint.valueType = ValueType::BNODE;
  } else if (input.substr(0, 8) == "DATATYPE") {
    // DATATYPE<type>
    constraint.datatypeRestriction = input;
  } else if (input[0] == '<' && input[input.length() - 1] == '>') {
    // Specific IRI
    constraint.allowedIris.insert(input.substr(1, input.length() - 2));
  } else {
    // Default to accepting anything
    return constraint;
  }

  return constraint;
}

std::optional<PropertyShape> ShExParser::parseProperty(const std::string& input,
                                                       size_t& pos) {
  skipWhitespace(input, pos);

  // Read predicate
  std::string predicate = readWord(input, pos);
  if (predicate.empty()) {
    setError("Expected predicate");
    return std::nullopt;
  }

  PropertyShape prop(predicate);

  skipWhitespace(input, pos);

  // Read value constraint (optional)
  if (pos < input.length() && input[pos] != ';' && input[pos] != '}') {
    std::string constraint = readWord(input, pos);
    if (auto vc = parseValueConstraint(constraint)) {
      prop.valueConstraint = vc.value();
    }
  }

  skipWhitespace(input, pos);

  // Read cardinality (optional)
  if (pos < input.length() && (input[pos] == '?' || input[pos] == '*' ||
                               input[pos] == '+')) {
    std::string card(1, input[pos]);
    ++pos;
    if (auto c = parseCardinality(card)) {
      prop.cardinality = c.value();
    }
  }

  return prop;
}

std::optional<Shape> ShExParser::parseShape(const std::string& input, size_t& pos) {
  skipWhitespace(input, pos);

  // Read shape keyword
  std::string keyword = readWord(input, pos);
  if (keyword != "shape") {
    setError("Expected 'shape' keyword");
    return std::nullopt;
  }

  // Read shape ID
  std::string shapeId = readWord(input, pos);
  if (shapeId.empty()) {
    setError("Expected shape ID");
    return std::nullopt;
  }

  Shape shape(shapeId);

  skipWhitespace(input, pos);

  // Expect opening brace
  if (pos >= input.length() || input[pos] != '{') {
    setError("Expected '{'");
    return std::nullopt;
  }
  ++pos;

  // Parse properties until closing brace
  while (pos < input.length()) {
    skipWhitespace(input, pos);

    if (input[pos] == '}') {
      ++pos;
      break;
    }

    if (auto prop = parseProperty(input, pos)) {
      shape.addProperty(prop.value());
    } else {
      return std::nullopt;
    }

    skipWhitespace(input, pos);

    // Expect semicolon
    if (pos < input.length() && input[pos] == ';') {
      ++pos;
    }
  }

  return shape;
}

std::optional<ShExSchema> ShExParser::parse(const std::string& input) {
  ShExSchema schema;
  size_t pos = 0;

  while (pos < input.length()) {
    skipWhitespace(input, pos);

    if (pos >= input.length()) break;

    if (auto shape = parseShape(input, pos)) {
      schema.addShape(shape.value());
    } else {
      return std::nullopt;
    }
  }

  return schema;
}

// ============================================================================
// ShExValidator Implementation
// ============================================================================

ShExValidator::ValidationReport ShExValidator::validateNode(
    const std::string& nodeIri, const std::string& targetShapeId,
    const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>&
        data) {
  ValidationReport report{true, {}, {}};

  const Shape* shape = schema_.getShape(targetShapeId);
  if (!shape) {
    report.schemaErrors.push_back("Shape '" + targetShapeId + "' not found");
    report.conforms = false;
    return report;
  }

  auto result = shape->validate(data);
  report.conforms = result.isValid;
  report.nodeErrors[nodeIri] = result.errors;

  return report;
}

ShExValidator::ValidationReport ShExValidator::validateDataset(
    const std::map<std::string,
      std::map<std::string,
        std::vector<std::pair<std::string, ValueType>>>>& dataset,
    const std::map<std::string, std::string>& nodeToShapeMapping) {
  ValidationReport report{true, {}, {}};

  for (const auto& [nodeIri, properties] : dataset) {
    auto shapeIt = nodeToShapeMapping.find(nodeIri);
    if (shapeIt == nodeToShapeMapping.end()) {
      report.schemaErrors.push_back("No shape mapping for node: " + nodeIri);
      continue;
    }

    auto nodeReport = validateNode(nodeIri, shapeIt->second, properties);
    if (!nodeReport.conforms) {
      report.conforms = false;
      report.nodeErrors.insert(nodeReport.nodeErrors.begin(),
                              nodeReport.nodeErrors.end());
    }
  }

  return report;
}

}  // namespace shex

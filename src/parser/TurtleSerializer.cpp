#include "ShapeSerializer.h"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace shex {

// ============================================================================
// Turtle/ShExC Serializer Implementation
// ============================================================================

TurtleSerializer::TurtleSerializer() { initializeCommonPrefixes(); }

void TurtleSerializer::initializeCommonPrefixes() {
  // Initialize common RDF/RDFS/OWL/XSD prefixes
  addPrefix("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
  addPrefix("rdfs", "http://www.w3.org/2000/01/rdf-schema#");
  addPrefix("xsd", "http://www.w3.org/2001/XMLSchema#");
  addPrefix("owl", "http://www.w3.org/2002/07/owl#");
  addPrefix("foaf", "http://xmlns.com/foaf/0.1/");
  addPrefix("dc", "http://purl.org/dc/elements/1.1/");
  addPrefix("dct", "http://purl.org/dc/terms/");
  addPrefix("skos", "http://www.w3.org/2004/02/skos/core#");
  addPrefix("schema", "http://schema.org/");
}

void TurtleSerializer::addPrefix(const std::string& prefix,
                                const std::string& iri) {
  prefixes_[prefix] = iri;
  reversePrefixes_[iri] = prefix;
}

void TurtleSerializer::clearPrefixes() {
  prefixes_.clear();
  reversePrefixes_.clear();
}

std::string TurtleSerializer::abbreviateIri(const std::string& iri) const {
  // Check if it's already abbreviated (contains :)
  if (iri.find(':') != std::string::npos &&
      iri.find("http://") != 0 && iri.find("https://") != 0) {
    return iri;
  }

  // Try to find a matching prefix
  for (const auto& [prefix, prefixIri] : prefixes_) {
    if (iri.rfind(prefixIri, 0) == 0) {  // starts_with
      std::string localPart = iri.substr(prefixIri.length());
      return prefix + ":" + localPart;
    }
  }

  // If no prefix matches, return as angle-bracketed IRI
  return "<" + iri + ">";
}

std::string TurtleSerializer::expandIri(const std::string& abbreviated) const {
  // If it's angle-bracketed, remove brackets
  if (abbreviated.size() >= 2 && abbreviated[0] == '<' &&
      abbreviated[abbreviated.size() - 1] == '>') {
    return abbreviated.substr(1, abbreviated.size() - 2);
  }

  // If it contains :, expand the prefix
  size_t colonPos = abbreviated.find(':');
  if (colonPos != std::string::npos) {
    std::string prefix = abbreviated.substr(0, colonPos);
    std::string localPart = abbreviated.substr(colonPos + 1);

    auto it = prefixes_.find(prefix);
    if (it != prefixes_.end()) {
      return it->second + localPart;
    }
  }

  // Return as-is if can't expand
  return abbreviated;
}

std::string TurtleSerializer::indent(size_t level) const {
  if (compactMode_) {
    return "";
  }
  return std::string(level * indentation_, ' ');
}

std::string TurtleSerializer::serializeCardinality(Cardinality card) const {
  switch (card) {
    case Cardinality::EXACTLY_ONE:
      return "";  // Default, no symbol needed
    case Cardinality::ZERO_OR_ONE:
      return "?";
    case Cardinality::ZERO_OR_MORE:
      return "*";
    case Cardinality::ONE_OR_MORE:
      return "+";
    default:
      return "";
  }
}

std::string TurtleSerializer::serializeValueConstraint(
    const ValueSetConstraint& constraint) const {
  std::vector<std::string> parts;

  if (constraint.valueType.has_value()) {
    switch (constraint.valueType.value()) {
      case ValueType::IRI:
        parts.push_back("IRI");
        break;
      case ValueType::LITERAL:
        parts.push_back("LITERAL");
        break;
      case ValueType::BNODE:
        parts.push_back("BNODE");
        break;
    }
  }

  if (!constraint.allowedIris.empty()) {
    std::vector<std::string> values;
    for (const auto& iri : constraint.allowedIris) {
      values.push_back(abbreviateIri(iri));
    }
    if (values.size() == 1) {
      parts.push_back(values[0]);
    } else {
      parts.push_back("[ " + std::accumulate(
                               std::next(values.begin()), values.end(),
                               values[0],
                               [](const std::string& a, const std::string& b) {
                                 return a + " " + b;
                               }) +
                      " ]");
    }
  }

  if (constraint.datatypeRestriction.has_value()) {
    parts.push_back(abbreviateIri(constraint.datatypeRestriction.value()));
  }

  if (parts.empty()) {
    return ".";  // No constraint
  }

  return std::accumulate(std::next(parts.begin()), parts.end(), parts[0],
                        [](const std::string& a, const std::string& b) {
                          return a + " " + b;
                        });
}

std::string TurtleSerializer::serializePropertyShape(
    const PropertyShape& prop) const {
  std::ostringstream oss;

  if (prop.inverse) {
    oss << "^";
  }

  oss << abbreviateIri(prop.predicate);

  // Add value constraint if meaningful
  std::string constraint = serializeValueConstraint(prop.valueConstraint);
  if (constraint != ".") {
    oss << " " << constraint;
  }

  // Add cardinality
  std::string card = serializeCardinality(prop.cardinality);
  if (!card.empty()) {
    oss << " " << card;
  }

  return oss.str();
}

std::string TurtleSerializer::serializeShape(const Shape& shape) const {
  std::ostringstream oss;

  oss << abbreviateIri(shape.id);

  if (shape.closed) {
    oss << " CLOSED";
  }

  oss << " {\n";

  for (size_t i = 0; i < shape.properties.size(); ++i) {
    oss << indent(1) << serializePropertyShape(shape.properties[i]);
    if (i < shape.properties.size() - 1) {
      oss << " ;";
    }
    oss << "\n";
  }

  oss << "}";

  return oss.str();
}

std::string TurtleSerializer::serializeSchema(const ShExSchema& schema) const {
  std::ostringstream oss;

  // Write prefixes
  if (!compactMode_) {
    for (const auto& [prefix, iri] : prefixes_) {
      oss << "PREFIX " << prefix << ": <" << iri << ">\n";
    }
    if (!prefixes_.empty()) {
      oss << "\n";
    }
  }

  // Write shapes
  size_t count = 0;
  for (const auto& [id, shape] : schema.getShapes()) {
    oss << serializeShape(shape);
    if (count < schema.getShapes().size() - 1) {
      oss << "\n\n";
    }
    ++count;
  }

  return oss.str();
}

std::optional<Cardinality> TurtleSerializer::parseCardinalitySymbol(
    const std::string& symbol) {
  if (symbol.empty() || symbol == "1") {
    return Cardinality::EXACTLY_ONE;
  }
  if (symbol == "?") {
    return Cardinality::ZERO_OR_ONE;
  }
  if (symbol == "*") {
    return Cardinality::ZERO_OR_MORE;
  }
  if (symbol == "+") {
    return Cardinality::ONE_OR_MORE;
  }
  return std::nullopt;
}

std::optional<PropertyShape> TurtleSerializer::parsePropertyLine(
    const std::string& line) {
  // Simple parser for property lines
  // Format: [^]predicate [constraint] [cardinality] [;]

  std::string trimmed = line;
  // Trim whitespace
  trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
  trimmed.erase(trimmed.find_last_not_of(" \t\r\n;") + 1);

  if (trimmed.empty()) {
    return std::nullopt;
  }

  std::istringstream iss(trimmed);
  std::string token;
  std::vector<std::string> tokens;

  while (iss >> token) {
    tokens.push_back(token);
  }

  if (tokens.empty()) {
    return std::nullopt;
  }

  size_t idx = 0;
  bool inverse = false;

  // Check for inverse
  if (tokens[idx][0] == '^') {
    inverse = true;
    tokens[idx] = tokens[idx].substr(1);
  }

  std::string predicate = expandIri(tokens[idx]);
  PropertyShape prop(predicate);
  prop.inverse = inverse;
  ++idx;

  // Parse value constraint and cardinality
  while (idx < tokens.size()) {
    const std::string& tok = tokens[idx];

    if (tok == "?" || tok == "*" || tok == "+") {
      auto card = parseCardinalitySymbol(tok);
      if (card.has_value()) {
        prop.cardinality = card.value();
      }
    } else if (tok == "IRI") {
      prop.valueConstraint.valueType = ValueType::IRI;
    } else if (tok == "LITERAL") {
      prop.valueConstraint.valueType = ValueType::LITERAL;
    } else if (tok == "BNODE") {
      prop.valueConstraint.valueType = ValueType::BNODE;
    } else if (tok[0] == '<' || tok.find(':') != std::string::npos) {
      // IRI constraint
      prop.valueConstraint.allowedIris.insert(expandIri(tok));
    }

    ++idx;
  }

  return prop;
}

std::optional<ShExSchema> TurtleSerializer::deserializeSchema(
    const std::string& input) {
  try {
    ShExSchema schema;
    std::istringstream stream(input);
    std::string line;

    // First pass: parse prefixes
    while (std::getline(stream, line)) {
      // Trim
      line.erase(0, line.find_first_not_of(" \t\r\n"));

      if (line.empty() || line[0] == '#') {
        continue;  // Skip empty lines and comments
      }

      if (line.rfind("PREFIX", 0) == 0) {  // starts_with
        // Parse PREFIX declaration
        // Format: PREFIX prefix: <iri>
        std::istringstream iss(line);
        std::string prefix_kw, prefix, iri;
        iss >> prefix_kw >> prefix;

        // Remove trailing colon from prefix
        if (!prefix.empty() && prefix.back() == ':') {
          prefix.pop_back();
        }

        // Read IRI (between < and >)
        iss >> iri;
        if (iri.size() >= 2 && iri[0] == '<' && iri.back() == '>') {
          iri = iri.substr(1, iri.size() - 2);
          addPrefix(prefix, iri);
        }
        continue;
      }

      // If not a prefix, we've reached shape definitions
      break;
    }

    // Second pass: parse shapes
    stream.clear();
    stream.seekg(0);

    std::optional<Shape> currentShape;
    bool inShape = false;

    while (std::getline(stream, line)) {
      // Trim
      line.erase(0, line.find_first_not_of(" \t\r\n"));

      if (line.empty() || line[0] == '#' || line.rfind("PREFIX", 0) == 0) {
        continue;
      }

      // Check for shape start
      if (line.find('{') != std::string::npos && !inShape) {
        // Extract shape ID and flags
        size_t bracePos = line.find('{');
        std::string header = line.substr(0, bracePos);

        std::istringstream iss(header);
        std::string shapeId;
        iss >> shapeId;

        currentShape = Shape(expandIri(shapeId));
        inShape = true;

        // Check for CLOSED
        if (header.find("CLOSED") != std::string::npos) {
          currentShape->closed = true;
        }

        // Check if there's content on same line after {
        size_t afterBrace = line.find_first_not_of(" \t", bracePos + 1);
        if (afterBrace != std::string::npos && line[afterBrace] != '}') {
          std::string propLine = line.substr(afterBrace);
          auto prop = parsePropertyLine(propLine);
          if (prop.has_value()) {
            currentShape->addProperty(prop.value());
          }
        }

        continue;
      }

      // Check for shape end
      if (line.find('}') != std::string::npos && inShape) {
        if (currentShape.has_value()) {
          schema.addShape(currentShape.value());
        }
        currentShape = std::nullopt;
        inShape = false;
        continue;
      }

      // Parse property line
      if (inShape && currentShape.has_value()) {
        auto prop = parsePropertyLine(line);
        if (prop.has_value()) {
          currentShape->addProperty(prop.value());
        }
      }
    }

    // Handle case where shape wasn't closed
    if (inShape && currentShape.has_value()) {
      schema.addShape(currentShape.value());
    }

    return schema;
  } catch (const std::exception& e) {
    setError(std::string("Turtle deserialization error: ") + e.what());
    return std::nullopt;
  }
}

}  // namespace shex

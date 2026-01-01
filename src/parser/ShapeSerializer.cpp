#include "ShapeSerializer.h"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace shex {

// ============================================================================
// JSON-LD Serializer Implementation
// ============================================================================

std::string JsonLdSerializer::cardinalityToString(Cardinality card) const {
  switch (card) {
    case Cardinality::EXACTLY_ONE:
      return "exactly_one";
    case Cardinality::ZERO_OR_ONE:
      return "zero_or_one";
    case Cardinality::ZERO_OR_MORE:
      return "zero_or_more";
    case Cardinality::ONE_OR_MORE:
      return "one_or_more";
    default:
      return "exactly_one";
  }
}

std::optional<Cardinality> JsonLdSerializer::stringToCardinality(
    const std::string& str) {
  if (str == "exactly_one")
    return Cardinality::EXACTLY_ONE;
  if (str == "zero_or_one")
    return Cardinality::ZERO_OR_ONE;
  if (str == "zero_or_more")
    return Cardinality::ZERO_OR_MORE;
  if (str == "one_or_more")
    return Cardinality::ONE_OR_MORE;
  return std::nullopt;
}

nlohmann::json JsonLdSerializer::valueConstraintToJson(
    const ValueSetConstraint& constraint) const {
  nlohmann::json j = nlohmann::json::object();

  if (constraint.valueType.has_value()) {
    switch (constraint.valueType.value()) {
      case ValueType::IRI:
        j["nodeKind"] = "iri";
        break;
      case ValueType::LITERAL:
        j["nodeKind"] = "literal";
        break;
      case ValueType::BNODE:
        j["nodeKind"] = "bnode";
        break;
    }
  }

  if (!constraint.allowedIris.empty()) {
    nlohmann::json values = nlohmann::json::array();
    for (const auto& iri : constraint.allowedIris) {
      values.push_back(iri);
    }
    j["values"] = values;
  }

  if (constraint.datatypeRestriction.has_value()) {
    j["datatype"] = constraint.datatypeRestriction.value();
  }

  return j;
}

nlohmann::json JsonLdSerializer::propertyShapeToJson(
    const PropertyShape& prop) const {
  nlohmann::json j = nlohmann::json::object();

  j["type"] = "TripleConstraint";
  j["predicate"] = prop.predicate;
  j["cardinality"] = cardinalityToString(prop.cardinality);

  if (prop.inverse) {
    j["inverse"] = true;
  }

  // Add value constraint if it has meaningful content
  auto valueExpr = valueConstraintToJson(prop.valueConstraint);
  if (!valueExpr.empty()) {
    j["valueExpr"] = valueExpr;
  }

  if (prop.nodeKind.has_value()) {
    j["nodeKind"] = prop.nodeKind.value();
  }

  return j;
}

nlohmann::json JsonLdSerializer::shapeToJson(const Shape& shape) const {
  nlohmann::json j = nlohmann::json::object();

  j["type"] = "Shape";
  j["id"] = shape.id;

  if (shape.closed) {
    j["closed"] = true;
  }

  if (!shape.properties.empty()) {
    if (shape.properties.size() == 1) {
      j["expression"] = propertyShapeToJson(shape.properties[0]);
    } else {
      nlohmann::json expressions = nlohmann::json::array();
      for (const auto& prop : shape.properties) {
        expressions.push_back(propertyShapeToJson(prop));
      }
      nlohmann::json eachOf = nlohmann::json::object();
      eachOf["type"] = "EachOf";
      eachOf["expressions"] = expressions;
      j["expression"] = eachOf;
    }
  }

  return j;
}

std::string JsonLdSerializer::serializeShape(const Shape& shape) const {
  try {
    auto j = shapeToJson(shape);
    return j.dump(2);  // Pretty print with 2-space indentation
  } catch (const std::exception& e) {
    setError(std::string("JSON-LD serialization error: ") + e.what());
    throw SerializationException(getLastError());
  }
}

std::string JsonLdSerializer::serializeSchema(const ShExSchema& schema) const {
  try {
    nlohmann::json j = nlohmann::json::object();

    j["@context"] = "http://www.w3.org/ns/shex.jsonld";
    j["type"] = "Schema";

    nlohmann::json shapes = nlohmann::json::array();
    for (const auto& [id, shape] : schema.getShapes()) {
      shapes.push_back(shapeToJson(shape));
    }
    j["shapes"] = shapes;

    return j.dump(2);  // Pretty print with 2-space indentation
  } catch (const std::exception& e) {
    setError(std::string("JSON-LD schema serialization error: ") + e.what());
    throw SerializationException(getLastError());
  }
}

std::optional<ValueSetConstraint> JsonLdSerializer::jsonToValueConstraint(
    const nlohmann::json& j) {
  ValueSetConstraint constraint;

  if (j.contains("nodeKind")) {
    std::string kind = j["nodeKind"].get<std::string>();
    if (kind == "iri") {
      constraint.valueType = ValueType::IRI;
    } else if (kind == "literal") {
      constraint.valueType = ValueType::LITERAL;
    } else if (kind == "bnode") {
      constraint.valueType = ValueType::BNODE;
    }
  }

  if (j.contains("values") && j["values"].is_array()) {
    for (const auto& value : j["values"]) {
      constraint.allowedIris.insert(value.get<std::string>());
    }
  }

  if (j.contains("datatype")) {
    constraint.datatypeRestriction = j["datatype"].get<std::string>();
  }

  return constraint;
}

std::optional<PropertyShape> JsonLdSerializer::jsonToPropertyShape(
    const nlohmann::json& j) {
  try {
    if (!j.contains("predicate")) {
      setError("PropertyShape missing required 'predicate' field");
      return std::nullopt;
    }

    PropertyShape prop(j["predicate"].get<std::string>());

    if (j.contains("cardinality")) {
      auto card = stringToCardinality(j["cardinality"].get<std::string>());
      if (card.has_value()) {
        prop.cardinality = card.value();
      }
    }

    if (j.contains("inverse")) {
      prop.inverse = j["inverse"].get<bool>();
    }

    if (j.contains("valueExpr")) {
      auto constraint = jsonToValueConstraint(j["valueExpr"]);
      if (constraint.has_value()) {
        prop.valueConstraint = constraint.value();
      }
    }

    if (j.contains("nodeKind")) {
      prop.nodeKind = j["nodeKind"].get<std::string>();
    }

    return prop;
  } catch (const nlohmann::json::exception& e) {
    setError(std::string("Error parsing PropertyShape: ") + e.what());
    return std::nullopt;
  }
}

std::optional<Shape> JsonLdSerializer::jsonToShape(const nlohmann::json& j) {
  try {
    if (!j.contains("id")) {
      setError("Shape missing required 'id' field");
      return std::nullopt;
    }

    Shape shape(j["id"].get<std::string>());

    if (j.contains("closed")) {
      shape.closed = j["closed"].get<bool>();
    }

    if (j.contains("expression")) {
      const auto& expr = j["expression"];

      if (expr.contains("type")) {
        std::string type = expr["type"].get<std::string>();

        if (type == "TripleConstraint") {
          // Single property
          auto prop = jsonToPropertyShape(expr);
          if (prop.has_value()) {
            shape.addProperty(prop.value());
          }
        } else if (type == "EachOf" && expr.contains("expressions")) {
          // Multiple properties
          for (const auto& propJson : expr["expressions"]) {
            auto prop = jsonToPropertyShape(propJson);
            if (prop.has_value()) {
              shape.addProperty(prop.value());
            }
          }
        }
      }
    }

    return shape;
  } catch (const nlohmann::json::exception& e) {
    setError(std::string("Error parsing Shape: ") + e.what());
    return std::nullopt;
  }
}

std::optional<ShExSchema> JsonLdSerializer::deserializeSchema(
    const std::string& input) {
  try {
    nlohmann::json j = nlohmann::json::parse(input);

    if (!j.contains("type") || j["type"].get<std::string>() != "Schema") {
      setError("Invalid ShEx JSON-LD: missing or incorrect 'type' field");
      return std::nullopt;
    }

    ShExSchema schema;

    if (j.contains("shapes") && j["shapes"].is_array()) {
      for (const auto& shapeJson : j["shapes"]) {
        auto shape = jsonToShape(shapeJson);
        if (shape.has_value()) {
          schema.addShape(shape.value());
        } else {
          return std::nullopt;  // Error already set
        }
      }
    }

    return schema;
  } catch (const nlohmann::json::parse_error& e) {
    setError(std::string("JSON parse error: ") + e.what());
    throw DeserializationException(getLastError(), e.byte, 0);
  } catch (const std::exception& e) {
    setError(std::string("Deserialization error: ") + e.what());
    return std::nullopt;
  }
}

bool JsonLdSerializer::validateRoundTrip(const ShExSchema& original) const {
  try {
    // Serialize
    std::string serialized = serializeSchema(original);

    // Deserialize
    auto deserializedOpt =
        const_cast<JsonLdSerializer*>(this)->deserializeSchema(serialized);
    if (!deserializedOpt.has_value()) {
      return false;
    }

    const auto& deserialized = deserializedOpt.value();

    // Compare shapes
    const auto& origShapes = original.getShapes();
    const auto& deserShapes = deserialized.getShapes();

    if (origShapes.size() != deserShapes.size()) {
      return false;
    }

    for (const auto& [id, origShape] : origShapes) {
      if (!deserialized.hasShape(id)) {
        return false;
      }

      const Shape* deserShape = deserialized.getShape(id);
      if (!deserShape) {
        return false;
      }

      // Compare basic properties
      if (origShape.closed != deserShape->closed) {
        return false;
      }

      if (origShape.properties.size() != deserShape->properties.size()) {
        return false;
      }

      // Compare properties
      for (size_t i = 0; i < origShape.properties.size(); ++i) {
        const auto& origProp = origShape.properties[i];
        const auto& deserProp = deserShape->properties[i];

        if (origProp.predicate != deserProp.predicate ||
            origProp.cardinality != deserProp.cardinality ||
            origProp.inverse != deserProp.inverse) {
          return false;
        }
      }
    }

    return true;
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace shex

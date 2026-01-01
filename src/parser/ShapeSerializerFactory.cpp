#include "ShapeSerializer.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace shex {

// ============================================================================
// ShapeSerializerFactory Implementation
// ============================================================================

std::unique_ptr<ShapeSerializer> ShapeSerializerFactory::createSerializer(
    SerializationFormat format) {
  switch (format) {
    case SerializationFormat::JSON_LD:
      return std::make_unique<JsonLdSerializer>();
    case SerializationFormat::TURTLE:
      return std::make_unique<TurtleSerializer>();
    case SerializationFormat::AUTO:
      // Default to JSON-LD for auto
      return std::make_unique<JsonLdSerializer>();
    default:
      throw std::invalid_argument("Unknown serialization format");
  }
}

SerializationFormat ShapeSerializerFactory::detectFormatFromExtension(
    const std::string& filename) {
  // Extract file extension
  size_t dotPos = filename.find_last_of('.');
  if (dotPos == std::string::npos) {
    return SerializationFormat::AUTO;
  }

  std::string ext = filename.substr(dotPos + 1);

  // Convert to lowercase for comparison
  std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return std::tolower(c); });

  if (ext == "json" || ext == "jsonld") {
    return SerializationFormat::JSON_LD;
  }

  if (ext == "ttl" || ext == "turtle" || ext == "shex" || ext == "shexc") {
    return SerializationFormat::TURTLE;
  }

  return SerializationFormat::AUTO;
}

bool ShapeSerializerFactory::looksLikeJson(const std::string& content) {
  // Trim leading whitespace
  size_t start = 0;
  while (start < content.length() &&
         std::isspace(static_cast<unsigned char>(content[start]))) {
    ++start;
  }

  if (start >= content.length()) {
    return false;
  }

  // JSON objects start with {, arrays with [
  char firstChar = content[start];
  if (firstChar != '{' && firstChar != '[') {
    return false;
  }

  // Additional validation: try to find key indicators
  // Look for "@context" or "type": "Schema" which are ShEx JSON-LD markers
  return content.find("\"@context\"") != std::string::npos ||
         content.find("\"type\"") != std::string::npos ||
         content.find("\"shapes\"") != std::string::npos;
}

bool ShapeSerializerFactory::looksLikeTurtle(const std::string& content) {
  // Trim leading whitespace
  std::string trimmed = content;
  trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));

  if (trimmed.empty()) {
    return false;
  }

  // Turtle/ShExC indicators:
  // - PREFIX declarations
  // - Shape definitions with { }
  // - Property declarations with predicates

  // Check for PREFIX
  if (trimmed.rfind("PREFIX", 0) == 0) {  // starts_with
    return true;
  }

  // Check for common prefixes used without PREFIX declaration
  if (trimmed.find(':') != std::string::npos) {
    // Has prefix-style notation
    return true;
  }

  // Check for shape braces
  if (trimmed.find('{') != std::string::npos &&
      trimmed.find('}') != std::string::npos) {
    return true;
  }

  // Check for angle-bracketed IRIs
  if (trimmed.find('<') != std::string::npos &&
      trimmed.find('>') != std::string::npos) {
    return true;
  }

  return false;
}

SerializationFormat ShapeSerializerFactory::detectFormatFromContent(
    const std::string& content) {
  if (content.empty()) {
    return SerializationFormat::AUTO;
  }

  // Try JSON first (more deterministic)
  if (looksLikeJson(content)) {
    return SerializationFormat::JSON_LD;
  }

  // Try Turtle
  if (looksLikeTurtle(content)) {
    return SerializationFormat::TURTLE;
  }

  // Default to AUTO if can't determine
  return SerializationFormat::AUTO;
}

std::optional<ShExSchema> ShapeSerializerFactory::autoDeserialize(
    const std::string& content) {
  // Detect format
  SerializationFormat format = detectFormatFromContent(content);

  // If still auto, try JSON first, then Turtle
  if (format == SerializationFormat::AUTO) {
    // Try JSON-LD
    try {
      auto jsonSerializer = std::make_unique<JsonLdSerializer>();
      auto result = jsonSerializer->deserializeSchema(content);
      if (result.has_value()) {
        return result;
      }
    } catch (...) {
      // Ignore and try next format
    }

    // Try Turtle
    try {
      auto turtleSerializer = std::make_unique<TurtleSerializer>();
      auto result = turtleSerializer->deserializeSchema(content);
      if (result.has_value()) {
        return result;
      }
    } catch (...) {
      // Ignore
    }

    return std::nullopt;
  }

  // Use detected format
  auto serializer = createSerializer(format);
  return serializer->deserializeSchema(content);
}

std::optional<ShExSchema> ShapeSerializerFactory::autoDeserializeFromFile(
    const std::string& filename) {
  // First try to detect from extension
  SerializationFormat format = detectFormatFromExtension(filename);

  if (format != SerializationFormat::AUTO) {
    auto serializer = createSerializer(format);
    return serializer->deserializeFromFile(filename);
  }

  // Read file content and detect format
  std::ifstream file(filename);
  if (!file) {
    throw DeserializationException("Failed to open file: " + filename);
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  return autoDeserialize(content);
}

// ============================================================================
// Utility Functions
// ============================================================================

bool testRoundTrip(const ShExSchema& schema, SerializationFormat format) {
  try {
    auto serializer = ShapeSerializerFactory::createSerializer(format);

    // Serialize
    std::string serialized = serializer->serializeSchema(schema);

    // Deserialize
    auto deserializedOpt = serializer->deserializeSchema(serialized);
    if (!deserializedOpt.has_value()) {
      return false;
    }

    const auto& deserialized = deserializedOpt.value();

    // Compare shape counts
    if (schema.getShapes().size() != deserialized.getShapes().size()) {
      return false;
    }

    // Compare each shape
    for (const auto& [id, origShape] : schema.getShapes()) {
      if (!deserialized.hasShape(id)) {
        return false;
      }

      const Shape* deserShape = deserialized.getShape(id);
      if (!deserShape) {
        return false;
      }

      // Basic validation
      if (origShape.properties.size() != deserShape->properties.size()) {
        return false;
      }
    }

    return true;
  } catch (const std::exception&) {
    return false;
  }
}

std::optional<ShExSchema> convertFormat(const std::string& input,
                                       SerializationFormat from,
                                       SerializationFormat to) {
  try {
    // Deserialize from source format
    auto fromSerializer = ShapeSerializerFactory::createSerializer(from);
    auto schemaOpt = fromSerializer->deserializeSchema(input);

    if (!schemaOpt.has_value()) {
      return std::nullopt;
    }

    // This function returns schema, not string
    // The caller can serialize to desired format if needed
    return schemaOpt;

  } catch (const std::exception&) {
    return std::nullopt;
  }
}

}  // namespace shex

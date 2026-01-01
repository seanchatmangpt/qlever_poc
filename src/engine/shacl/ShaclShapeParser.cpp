#include "ShaclShapeParser.h"

namespace shacl {

std::string ShaclShapeParser::trim(const std::string& str) {
  const auto start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) return "";
  const auto end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

std::vector<std::string> ShaclShapeParser::split(const std::string& str,
                                                 char delimiter) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    if (!token.empty()) {
      tokens.push_back(trim(token));
    }
  }
  return tokens;
}

std::string ShaclShapeParser::extractValue(const std::string& line,
                                           const std::string& prefix) {
  auto pos = line.find(prefix);
  if (pos == std::string::npos) return "";

  pos += prefix.length();
  auto start = line.find_first_not_of(" \t", pos);
  if (start == std::string::npos) return "";

  // Handle quoted strings
  if (line[start] == '"') {
    auto end = line.find('"', start + 1);
    if (end != std::string::npos) {
      return line.substr(start + 1, end - start - 1);
    }
  }

  // Handle IRIs
  if (line[start] == '<') {
    auto end = line.find('>', start);
    if (end != std::string::npos) {
      return line.substr(start + 1, end - start - 1);
    }
  }

  // Handle plain values
  auto end = line.find(';', start);
  if (end == std::string::npos) {
    end = line.find('.', start);
  }
  if (end == std::string::npos) {
    end = line.length();
  }

  return trim(line.substr(start, end - start));
}

std::vector<NodeShape> ShaclShapeParser::parseShapes(
    const std::string& shaclTurtle) {
  std::vector<NodeShape> shapes;

  // Split by shape definitions (simplified)
  auto lines = split(shaclTurtle, '\n');
  std::string currentShapeId;
  std::string currentShapeContent;

  for (const auto& line : lines) {
    const auto trimmed = trim(line);

    // Skip empty lines and comments
    if (trimmed.empty() || trimmed[0] == '#') continue;

    // Detect shape start
    if (trimmed.find("a sh:NodeShape") != std::string::npos ||
        trimmed.find("rdf:type sh:NodeShape") != std::string::npos) {
      if (!currentShapeId.empty() && !currentShapeContent.empty()) {
        shapes.push_back(parseShape(currentShapeId, currentShapeContent));
      }
      // Extract shape IRI from previous lines
      currentShapeId = trim(trimmed.substr(0, trimmed.find("a")));
      currentShapeContent = trimmed;
    } else if (!currentShapeId.empty()) {
      currentShapeContent += " " + trimmed;
    }
  }

  // Parse last shape
  if (!currentShapeId.empty() && !currentShapeContent.empty()) {
    shapes.push_back(parseShape(currentShapeId, currentShapeContent));
  }

  return shapes;
}

NodeShape ShaclShapeParser::parseShape(const std::string& shapeId,
                                       const std::string& shapeDef) {
  NodeShape shape;
  shape.shapeId = shapeId;

  // Parse targetClass
  auto targetClassValue = extractValue(shapeDef, "sh:targetClass");
  if (!targetClassValue.empty()) {
    shape.targetClasses.push_back(targetClassValue);
  }

  // Parse targetNode
  auto targetNodeValue = extractValue(shapeDef, "sh:targetNode");
  if (!targetNodeValue.empty()) {
    shape.targetNodes.push_back(targetNodeValue);
  }

  // Parse property shapes
  parsePropertyShapes(shapeDef, shape);

  // Parse node-level constraints
  parseNodeConstraints(shapeDef, shape);

  return shape;
}

void ShaclShapeParser::parsePropertyShapes(const std::string& shapeContent,
                                           NodeShape& shape) {
  // Find property shape blocks
  auto pos = shapeContent.find("sh:property");
  while (pos != std::string::npos) {
    // Extract property content (simplified - finds balanced brackets)
    auto propStart = shapeContent.find('[', pos);
    auto propEnd = shapeContent.find(']', propStart);

    if (propStart != std::string::npos && propEnd != std::string::npos) {
      auto propContent =
          shapeContent.substr(propStart + 1, propEnd - propStart - 1);
      shape.propertyShapes.push_back(parsePropertyShape(propContent));
    }

    pos = shapeContent.find("sh:property", propEnd);
  }
}

PropertyShape ShaclShapeParser::parsePropertyShape(
    const std::string& propContent) {
  PropertyShape prop;

  // Extract path
  auto pathValue = extractValue(propContent, "sh:path");
  prop.path = pathValue;

  // Extract constraints within this property
  auto minCountValue = extractValue(propContent, "sh:minCount");
  if (!minCountValue.empty()) {
    try {
      auto count = std::stoi(minCountValue);
      if (count >= 1) prop.required = true;

      ShaclConstraint constraint(ConstraintType::MinCount);
      constraint.value = count;
      constraint.message = "Minimum count violated: expected at least " +
                           minCountValue + " values for property " + prop.path;
      prop.constraints.push_back(constraint);
    } catch (...) {
      // Invalid value, skip
    }
  }

  auto maxCountValue = extractValue(propContent, "sh:maxCount");
  if (!maxCountValue.empty()) {
    try {
      auto count = std::stoi(maxCountValue);
      ShaclConstraint constraint(ConstraintType::MaxCount);
      constraint.value = count;
      constraint.message = "Maximum count violated: expected at most " +
                           maxCountValue + " values for property " + prop.path;
      prop.constraints.push_back(constraint);
    } catch (...) {
      // Invalid value, skip
    }
  }

  // Extract datatype constraint
  auto datatypeValue = extractValue(propContent, "sh:datatype");
  if (!datatypeValue.empty()) {
    ShaclConstraint constraint(ConstraintType::Datatype);
    constraint.value = datatypeValue;
    constraint.message = "Datatype constraint violated for property " + prop.path;
    prop.constraints.push_back(constraint);
  }

  // Extract pattern constraint
  auto patternValue = extractValue(propContent, "sh:pattern");
  if (!patternValue.empty()) {
    ShaclConstraint constraint(ConstraintType::Pattern);
    constraint.value = patternValue;
    constraint.message = "Pattern constraint violated for property " + prop.path;
    prop.constraints.push_back(constraint);
  }

  // Extract min/max length
  auto minLengthValue = extractValue(propContent, "sh:minLength");
  if (!minLengthValue.empty()) {
    try {
      auto length = std::stoi(minLengthValue);
      ShaclConstraint constraint(ConstraintType::MinLength);
      constraint.value = length;
      constraint.message = "Minimum length constraint violated for property " + prop.path;
      prop.constraints.push_back(constraint);
    } catch (...) {
    }
  }

  auto maxLengthValue = extractValue(propContent, "sh:maxLength");
  if (!maxLengthValue.empty()) {
    try {
      auto length = std::stoi(maxLengthValue);
      ShaclConstraint constraint(ConstraintType::MaxLength);
      constraint.value = length;
      constraint.message = "Maximum length constraint violated for property " + prop.path;
      prop.constraints.push_back(constraint);
    } catch (...) {
    }
  }

  return prop;
}

void ShaclShapeParser::parseNodeConstraints(const std::string& shapeContent,
                                            NodeShape& shape) {
  // Parse sh:closed
  if (shapeContent.find("sh:closed true") != std::string::npos) {
    shape.closed = true;
  }
}

}  // namespace shacl

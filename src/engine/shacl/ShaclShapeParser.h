#ifndef QLEVER_ENGINE_SHACL_SHACLSHAPEPARSER_H
#define QLEVER_ENGINE_SHACL_SHACLSHAPEPARSER_H

#include "ShaclShape.h"
#include <map>
#include <sstream>
#include <regex>

namespace shacl {

// Simple SHACL shape parser for Turtle format (80/20 implementation)
// Handles the most common SHACL patterns used in practice
class ShaclShapeParser {
 public:
  // Parse SHACL shapes from Turtle format
  std::vector<NodeShape> parseShapes(const std::string& shaclTurtle);

  // Parse a single shape definition
  NodeShape parseShape(const std::string& shapeId, const std::string& shapeDef);

 private:
  // Helper methods for parsing specific constraints
  void parsePropertyShapes(const std::string& shapeContent, NodeShape& shape);
  PropertyShape parsePropertyShape(const std::string& propContent);

  void parseNodeConstraints(const std::string& shapeContent, NodeShape& shape);

  void parseConstraint(const std::string& line, ShaclConstraint& constraint);

  // String utilities
  std::string trim(const std::string& str);
  std::string extractValue(const std::string& line, const std::string& prefix);
  std::vector<std::string> split(const std::string& str, char delimiter);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLSHAPEPARSER_H

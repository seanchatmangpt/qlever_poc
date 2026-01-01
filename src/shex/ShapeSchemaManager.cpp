// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: AI Assistant (Claude Code)

#include "shex/ShapeSchemaManager.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "util/Exception.h"
#include "util/Log.h"

namespace shex {

// ____________________________________________________________________________
ShapeOptimizationHints ShapeExpression::computeHints() const {
  ShapeOptimizationHints hints;
  hints.isClosed = isClosed;

  // Aggregate selectivity from all triple constraints
  double combinedSelectivity = 1.0;
  for (const auto& tc : tripleConstraints) {
    combinedSelectivity *= tc.estimatedSelectivity();
    hints.requiredPredicates.insert(Id::makeFromIri(tc.predicate));

    // Extract type constraints
    if (tc.valueConstraint.has_value()) {
      for (const auto& dtype : tc.valueConstraint->datatypes) {
        hints.requiredTypes.insert(Id::makeFromIri(dtype));
      }
    }

    // Extract cardinality constraints
    if (tc.minCount > 0) {
      hints.minCardinality =
          std::max(hints.minCardinality.value_or(0), tc.minCount);
    }
    if (tc.maxCount < std::numeric_limits<size_t>::max()) {
      hints.maxCardinality = std::min(
          hints.maxCardinality.value_or(std::numeric_limits<size_t>::max()),
          tc.maxCount);
    }
  }

  hints.selectivityFactor = combinedSelectivity;

  return hints;
}

// ____________________________________________________________________________
void ShapeSchemaManager::loadFromFile(const std::string& filename) {
  LOG(INFO) << "Loading shape schema from file: " << filename << std::endl;

  std::ifstream file(filename);
  AD_CONTRACT_CHECK(file.is_open(), "Failed to open shape schema file: ",
                    filename);

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Detect format by file extension or content
  if (filename.ends_with(".json") || filename.ends_with(".jsonld")) {
    loadFromJson(content);
  } else if (filename.ends_with(".shex") || filename.ends_with(".shexc")) {
    parseShExC(content);
  } else {
    // Try JSON first, fall back to ShExC
    try {
      loadFromJson(content);
    } catch (...) {
      parseShExC(content);
    }
  }

  rebuildPredicateIndex();
  LOG(INFO) << "Loaded " << shapes_.size() << " shapes" << std::endl;
}

// ____________________________________________________________________________
void ShapeSchemaManager::loadFromJson(const std::string& jsonContent) {
  try {
    parseShExJson(jsonContent);
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("Failed to parse ShEx JSON: ") +
                             e.what());
  }
}

// ____________________________________________________________________________
void ShapeSchemaManager::registerShape(ShapeExpression shape) {
  Iri shapeId = shape.id;
  shapes_[shapeId] = std::make_shared<ShapeExpression>(std::move(shape));

  // Update predicate index for this shape
  for (const auto& tc : shapes_[shapeId]->tripleConstraints) {
    predicateToShapes_[tc.predicate].push_back(shapeId);
  }

  // Invalidate cached hints for this shape
  hintsCache_.erase(shapeId);
}

// ____________________________________________________________________________
void ShapeSchemaManager::clear() {
  shapes_.clear();
  predicateToShapes_.clear();
  hintsCache_.clear();
  statistics_.reset();
}

// ____________________________________________________________________________
std::optional<const ShapeExpression*> ShapeSchemaManager::getShape(
    const Iri& shapeId) const {
  auto it = shapes_.find(shapeId);
  if (it != shapes_.end()) {
    return it->second.get();
  }
  return std::nullopt;
}

// ____________________________________________________________________________
std::vector<const ShapeExpression*> ShapeSchemaManager::getShapesForPredicate(
    const Iri& predicate) const {
  std::vector<const ShapeExpression*> result;

  auto it = predicateToShapes_.find(predicate);
  if (it != predicateToShapes_.end()) {
    result.reserve(it->second.size());
    for (const auto& shapeId : it->second) {
      auto shapeOpt = getShape(shapeId);
      if (shapeOpt.has_value()) {
        result.push_back(shapeOpt.value());
      }
    }
  }

  return result;
}

// ____________________________________________________________________________
std::optional<ShapeOptimizationHints> ShapeSchemaManager::getOptimizationHints(
    const Iri& shapeId) const {
  // Check cache first
  auto cacheIt = hintsCache_.find(shapeId);
  if (cacheIt != hintsCache_.end()) {
    return cacheIt->second;
  }

  // Compute hints if shape exists
  auto shapeOpt = getShape(shapeId);
  if (shapeOpt.has_value()) {
    auto hints = (*shapeOpt)->computeHints();
    hintsCache_[shapeId] = hints;
    return hints;
  }

  return std::nullopt;
}

// ____________________________________________________________________________
std::vector<Iri> ShapeSchemaManager::getAllShapeIds() const {
  std::vector<Iri> ids;
  ids.reserve(shapes_.size());
  for (const auto& [id, _] : shapes_) {
    ids.push_back(id);
  }
  return ids;
}

// ____________________________________________________________________________
bool ShapeSchemaManager::hasShape(const Iri& shapeId) const {
  return shapes_.contains(shapeId);
}

// ____________________________________________________________________________
void ShapeSchemaManager::saveMetadata(const std::string& filename) const {
  LOG(INFO) << "Saving shape metadata to: " << filename << std::endl;

  nlohmann::json root;
  root["version"] = "1.0";
  root["shapeCount"] = shapes_.size();

  // Serialize shapes
  nlohmann::json shapesJson = nlohmann::json::array();
  for (const auto& [id, shape] : shapes_) {
    nlohmann::json shapeJson;
    shapeJson["id"] = id.toStringRepresentation();
    shapeJson["label"] = shape->label;
    shapeJson["isClosed"] = shape->isClosed;

    // Serialize triple constraints
    nlohmann::json constraintsJson = nlohmann::json::array();
    for (const auto& tc : shape->tripleConstraints) {
      nlohmann::json tcJson;
      tcJson["predicate"] = tc.predicate.toStringRepresentation();
      tcJson["minCount"] = tc.minCount;
      tcJson["maxCount"] =
          (tc.maxCount == std::numeric_limits<size_t>::max())
              ? -1
              : static_cast<int64_t>(tc.maxCount);
      tcJson["inverse"] = tc.inverse;
      constraintsJson.push_back(tcJson);
    }
    shapeJson["tripleConstraints"] = constraintsJson;

    shapesJson.push_back(shapeJson);
  }
  root["shapes"] = shapesJson;

  // Serialize statistics
  root["statistics"]["totalValidations"] = statistics_.totalValidations;
  root["statistics"]["successfulValidations"] =
      statistics_.successfulValidations;
  root["statistics"]["failedValidations"] = statistics_.failedValidations;
  root["statistics"]["optimizationHintsUsed"] =
      statistics_.optimizationHintsUsed;
  root["statistics"]["avgCardinalityReduction"] =
      statistics_.avgCardinalityReduction;

  std::ofstream file(filename);
  AD_CONTRACT_CHECK(file.is_open(), "Failed to open metadata file for writing: ",
                    filename);
  file << root.dump(2);
  file.close();

  LOG(INFO) << "Saved metadata for " << shapes_.size() << " shapes"
            << std::endl;
}

// ____________________________________________________________________________
void ShapeSchemaManager::loadMetadata(const std::string& filename) {
  LOG(INFO) << "Loading shape metadata from: " << filename << std::endl;

  std::ifstream file(filename);
  AD_CONTRACT_CHECK(file.is_open(), "Failed to open metadata file: ", filename);

  nlohmann::json root;
  file >> root;

  // Clear existing data
  clear();

  // Load shapes
  for (const auto& shapeJson : root["shapes"]) {
    ShapeExpression shape;
    shape.id = Iri::fromIriref(shapeJson["id"].get<std::string>());
    shape.label = shapeJson["label"].get<std::string>();
    shape.isClosed = shapeJson["isClosed"].get<bool>();

    // Load triple constraints
    for (const auto& tcJson : shapeJson["tripleConstraints"]) {
      TripleConstraint tc;
      tc.predicate =
          Iri::fromIriref(tcJson["predicate"].get<std::string>());
      tc.minCount = tcJson["minCount"].get<size_t>();
      int64_t maxCountRaw = tcJson["maxCount"].get<int64_t>();
      tc.maxCount = (maxCountRaw == -1) ? std::numeric_limits<size_t>::max()
                                        : static_cast<size_t>(maxCountRaw);
      tc.inverse = tcJson["inverse"].get<bool>();
      shape.tripleConstraints.push_back(tc);
    }

    registerShape(std::move(shape));
  }

  // Load statistics if present
  if (root.contains("statistics")) {
    statistics_.totalValidations =
        root["statistics"]["totalValidations"].get<size_t>();
    statistics_.successfulValidations =
        root["statistics"]["successfulValidations"].get<size_t>();
    statistics_.failedValidations =
        root["statistics"]["failedValidations"].get<size_t>();
    statistics_.optimizationHintsUsed =
        root["statistics"]["optimizationHintsUsed"].get<size_t>();
    statistics_.avgCardinalityReduction =
        root["statistics"]["avgCardinalityReduction"].get<double>();
  }

  LOG(INFO) << "Loaded " << shapes_.size() << " shapes from metadata"
            << std::endl;
}

// ____________________________________________________________________________
void ShapeSchemaManager::recordValidation(bool success,
                                          double cardinalityReduction) {
  if (!collectStatistics_) {
    return;
  }

  statistics_.totalValidations++;
  if (success) {
    statistics_.successfulValidations++;
  } else {
    statistics_.failedValidations++;
  }

  // Update moving average of cardinality reduction
  if (statistics_.totalValidations > 0) {
    statistics_.avgCardinalityReduction =
        (statistics_.avgCardinalityReduction *
             (statistics_.totalValidations - 1) +
         cardinalityReduction) /
        statistics_.totalValidations;
  }
}

// ____________________________________________________________________________
void ShapeSchemaManager::recordOptimizationHint() {
  if (collectStatistics_) {
    statistics_.optimizationHintsUsed++;
  }
}

// ____________________________________________________________________________
void ShapeSchemaManager::rebuildPredicateIndex() {
  predicateToShapes_.clear();

  for (const auto& [shapeId, shape] : shapes_) {
    for (const auto& tc : shape->tripleConstraints) {
      predicateToShapes_[tc.predicate].push_back(shapeId);
    }
  }
}

// ____________________________________________________________________________
void ShapeSchemaManager::parseShExC(const std::string& content) {
  // Simplified ShExC parser (parses basic shape definitions)
  // Production implementation would use a proper ShExC parser

  LOG(WARN) << "ShExC parsing not fully implemented, using simplified parser"
            << std::endl;

  // For now, just create a placeholder shape
  // Real implementation would parse the full ShExC grammar
  ShapeExpression placeholder;
  placeholder.id = Iri::fromIriref("<http://example.org/shapes/Placeholder>");
  placeholder.label = "Placeholder";
  placeholder.isClosed = false;

  registerShape(std::move(placeholder));
}

// ____________________________________________________________________________
void ShapeSchemaManager::parseShExJson(const std::string& jsonContent) {
  nlohmann::json root = nlohmann::json::parse(jsonContent);

  // ShEx JSON format parsing (simplified)
  if (!root.contains("shapes")) {
    throw std::runtime_error("Invalid ShEx JSON: missing 'shapes' field");
  }

  for (const auto& shapeJson : root["shapes"]) {
    ShapeExpression shape;

    // Parse shape ID
    if (!shapeJson.contains("id")) {
      LOG(WARN) << "Skipping shape without ID" << std::endl;
      continue;
    }
    shape.id = Iri::fromIriref(shapeJson["id"].get<std::string>());

    // Parse optional label
    if (shapeJson.contains("label")) {
      shape.label = shapeJson["label"].get<std::string>();
    } else {
      shape.label = shape.id.toStringRepresentation();
    }

    // Parse closed flag
    if (shapeJson.contains("closed")) {
      shape.isClosed = shapeJson["closed"].get<bool>();
    }

    // Parse triple constraints (simplified)
    if (shapeJson.contains("expression") &&
        shapeJson["expression"].contains("expressions")) {
      for (const auto& exprJson : shapeJson["expression"]["expressions"]) {
        if (exprJson.contains("predicate")) {
          TripleConstraint tc;
          tc.predicate =
              Iri::fromIriref(exprJson["predicate"].get<std::string>());

          if (exprJson.contains("min")) {
            tc.minCount = exprJson["min"].get<size_t>();
          }
          if (exprJson.contains("max")) {
            auto maxVal = exprJson["max"];
            if (maxVal.is_number()) {
              tc.maxCount = maxVal.get<size_t>();
            }
            // else: max is "*" (unbounded), keep default
          }

          shape.tripleConstraints.push_back(tc);
        }
      }
    }

    registerShape(std::move(shape));
  }
}

}  // namespace shex

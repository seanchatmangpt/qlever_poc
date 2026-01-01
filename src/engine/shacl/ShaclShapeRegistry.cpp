#include "ShaclShapeRegistry.h"

namespace shacl {

void ShaclShapeRegistry::registerShape(const NodeShape& shape) {
  shapes_[shape.shapeId] = shape;
}

const NodeShape* ShaclShapeRegistry::getShape(
    const std::string& shapeId) const {
  auto it = shapes_.find(shapeId);
  if (it != shapes_.end()) {
    return &it->second;
  }
  return nullptr;
}

std::vector<const NodeShape*> ShaclShapeRegistry::getShapesForClass(
    const std::string& classIri) const {
  std::vector<const NodeShape*> results;
  for (const auto& [_, shape] : shapes_) {
    if (shape.isTargetClass(classIri)) {
      results.push_back(&shape);
    }
  }
  return results;
}

std::vector<const NodeShape*> ShaclShapeRegistry::getShapesForNode(
    const std::string& nodeIri) const {
  std::vector<const NodeShape*> results;
  for (const auto& [_, shape] : shapes_) {
    if (shape.isTargetNode(nodeIri)) {
      results.push_back(&shape);
    }
  }
  return results;
}

std::vector<const NodeShape*> ShaclShapeRegistry::getAllShapes() const {
  std::vector<const NodeShape*> results;
  for (const auto& [_, shape] : shapes_) {
    results.push_back(&shape);
  }
  return results;
}

void ShaclShapeRegistry::clear() {
  shapes_.clear();
}

bool ShaclShapeRegistry::hasShape(const std::string& shapeId) const {
  return shapes_.find(shapeId) != shapes_.end();
}

}  // namespace shacl

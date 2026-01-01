#include "ShaclShapeRegistry.h"
#include "ShapeComposition.h"

namespace shacl {

void ShaclShapeRegistry::registerShape(const NodeShape& shape) {
  shapes_[shape.shapeId] = shape;
}

void ShaclShapeRegistry::registerComposableShape(
    const ComposableNodeShape& shape) {
  composableShapes_[shape.shapeId] = shape;
}

const NodeShape* ShaclShapeRegistry::getShape(
    const std::string& shapeId) const {
  auto it = shapes_.find(shapeId);
  if (it != shapes_.end()) {
    return &it->second;
  }
  return nullptr;
}

const ComposableNodeShape* ShaclShapeRegistry::getComposableShape(
    const std::string& shapeId) const {
  auto it = composableShapes_.find(shapeId);
  if (it != composableShapes_.end()) {
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

std::vector<const ComposableNodeShape*>
ShaclShapeRegistry::getAllComposableShapes() const {
  std::vector<const ComposableNodeShape*> results;
  for (const auto& [_, shape] : composableShapes_) {
    results.push_back(&shape);
  }
  return results;
}

void ShaclShapeRegistry::clear() {
  shapes_.clear();
  composableShapes_.clear();
  compositionEngine_.reset();
}

bool ShaclShapeRegistry::hasShape(const std::string& shapeId) const {
  return shapes_.find(shapeId) != shapes_.end();
}

bool ShaclShapeRegistry::hasComposableShape(
    const std::string& shapeId) const {
  return composableShapes_.find(shapeId) != composableShapes_.end();
}

void ShaclShapeRegistry::resolveComposableShapes() {
  initializeCompositionEngine();
  compositionEngine_->resolveAllShapes();
}

const ShapeDependencyGraph* ShaclShapeRegistry::getDependencyGraph() const {
  if (compositionEngine_) {
    return &compositionEngine_->getDependencyGraph();
  }
  return nullptr;
}

std::vector<std::string> ShaclShapeRegistry::validateComposition() const {
  if (compositionEngine_) {
    return compositionEngine_->validateComposition();
  }
  return {};
}

void ShaclShapeRegistry::initializeCompositionEngine() {
  if (!compositionEngine_) {
    compositionEngine_ = std::make_unique<ShapeCompositionEngine>(this);
  }
}

}  // namespace shacl

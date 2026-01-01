#ifndef QLEVER_ENGINE_SHACL_SHACLSHAPEREGISTRY_H
#define QLEVER_ENGINE_SHACL_SHACLSHAPEREGISTRY_H

#include "ShaclShape.h"
#include <unordered_map>
#include <memory>
#include <vector>

namespace shacl {

// Registry for managing SHACL shapes
class ShaclShapeRegistry {
 public:
  // Register a shape
  void registerShape(const NodeShape& shape);

  // Retrieve a shape by ID
  const NodeShape* getShape(const std::string& shapeId) const;

  // Get all shapes targeting a specific class
  std::vector<const NodeShape*> getShapesForClass(
      const std::string& classIri) const;

  // Get all shapes targeting a specific node
  std::vector<const NodeShape*> getShapesForNode(
      const std::string& nodeIri) const;

  // Get all registered shapes
  std::vector<const NodeShape*> getAllShapes() const;

  // Clear all shapes
  void clear();

  // Check if a shape is registered
  bool hasShape(const std::string& shapeId) const;

  // Get shape count
  size_t size() const { return shapes_.size(); }

  // Enable/disable shape validation
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

 private:
  std::unordered_map<std::string, NodeShape> shapes_;
  bool enabled_ = true;
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLSHAPEREGISTRY_H

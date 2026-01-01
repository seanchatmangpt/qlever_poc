#ifndef QLEVER_ENGINE_SHACL_SHAPECOMPOSITION_H
#define QLEVER_ENGINE_SHACL_SHAPECOMPOSITION_H

#include "ShaclShape.h"
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shacl {

// Forward declaration
class ShaclShapeRegistry;

// Represents a parameter that can be passed between shapes
struct ShapeParameter {
  std::string name;         // Parameter name (e.g., "minAge")
  std::string defaultValue; // Default value if not provided
  bool required = false;    // Whether parameter is required

  ShapeParameter() = default;
  ShapeParameter(const std::string& n, const std::string& def = "",
                 bool req = false)
      : name(n), defaultValue(def), required(req) {}
};

// Represents a reference to another shape with parameter bindings
struct ShapeReference {
  std::string shapeId;    // ID of the referenced shape
  std::unordered_map<std::string, std::string>
      parameterBindings; // Parameter name -> value

  ShapeReference() = default;
  explicit ShapeReference(const std::string& id) : shapeId(id) {}

  void bindParameter(const std::string& name, const std::string& value) {
    parameterBindings[name] = value;
  }
};

// Extended NodeShape with composition support
class ComposableNodeShape : public NodeShape {
 public:
  // Shape inheritance (non-standard sh:extends)
  std::vector<ShapeReference> extends; // Parent shapes

  // Shape composition (standard sh:node)
  std::vector<ShapeReference> nodeReferences; // Referenced shapes

  // Parameters this shape accepts
  std::vector<ShapeParameter> parameters;

  // Track if this shape has been resolved (all dependencies processed)
  bool resolved = false;

  // Add parent shape
  ComposableNodeShape& addExtends(const ShapeReference& parent) {
    extends.push_back(parent);
    return *this;
  }

  // Add node reference
  ComposableNodeShape& addNodeReference(const ShapeReference& ref) {
    nodeReferences.push_back(ref);
    return *this;
  }

  // Add parameter
  ComposableNodeShape& addParameter(const ShapeParameter& param) {
    parameters.push_back(param);
    return *this;
  }

  // Get all shape dependencies (for dependency graph)
  std::vector<std::string> getDependencies() const {
    std::vector<std::string> deps;
    for (const auto& ext : extends) {
      deps.push_back(ext.shapeId);
    }
    for (const auto& ref : nodeReferences) {
      deps.push_back(ref.shapeId);
    }
    return deps;
  }
};

// Manages shape dependency graph and resolution order
class ShapeDependencyGraph {
 public:
  // Add a shape and its dependencies
  void addShape(const std::string& shapeId,
                const std::vector<std::string>& dependencies);

  // Compute topological sort (resolution order)
  // Returns empty vector if cycle detected
  std::vector<std::string> getResolutionOrder() const;

  // Check if there are circular dependencies
  bool hasCycle() const;

  // Get direct dependencies of a shape
  std::vector<std::string> getDependencies(const std::string& shapeId) const;

  // Get all shapes that depend on this shape
  std::vector<std::string> getDependents(const std::string& shapeId) const;

  // Clear the graph
  void clear();

  // Get all shape IDs in the graph
  std::vector<std::string> getAllShapeIds() const;

 private:
  std::unordered_map<std::string, std::vector<std::string>>
      dependencies_; // shapeId -> list of dependencies
  std::unordered_map<std::string, std::vector<std::string>>
      dependents_; // shapeId -> list of shapes that depend on it

  // Helper for topological sort using DFS
  bool topologicalSortUtil(const std::string& shapeId,
                           std::unordered_set<std::string>& visited,
                           std::unordered_set<std::string>& recursionStack,
                           std::vector<std::string>& result) const;
};

// Handles merging of constraints from multiple shapes
class ConstraintMerger {
 public:
  enum class MergeStrategy {
    Override,      // Child overrides parent
    Accumulate,    // Combine all constraints
    MostRestrictive // Use most restrictive constraint
  };

  // Merge constraints from parent and child shapes
  static std::vector<ShaclConstraint>
  mergeConstraints(const std::vector<ShaclConstraint>& parentConstraints,
                   const std::vector<ShaclConstraint>& childConstraints,
                   MergeStrategy strategy = MergeStrategy::MostRestrictive);

  // Merge property shapes (by path)
  static std::vector<PropertyShape>
  mergePropertyShapes(const std::vector<PropertyShape>& parentShapes,
                      const std::vector<PropertyShape>& childShapes,
                      MergeStrategy strategy = MergeStrategy::MostRestrictive);

  // Check if two constraints conflict
  static bool
  hasConflict(const ShaclConstraint& c1, const ShaclConstraint& c2);

  // Resolve conflict between two constraints
  static ShaclConstraint
  resolveConflict(const ShaclConstraint& c1, const ShaclConstraint& c2,
                  MergeStrategy strategy = MergeStrategy::MostRestrictive);

 private:
  // Get the more restrictive of two constraints
  static ShaclConstraint getMostRestrictive(const ShaclConstraint& c1,
                                            const ShaclConstraint& c2);

  // Check if constraint c1 is more restrictive than c2
  static bool isMoreRestrictive(const ShaclConstraint& c1,
                                 const ShaclConstraint& c2);
};

// Main composition engine that resolves shape hierarchies
class ShapeCompositionEngine {
 public:
  explicit ShapeCompositionEngine(ShaclShapeRegistry* registry)
      : registry_(registry) {}

  // Resolve all composable shapes in the registry
  void resolveAllShapes();

  // Resolve a single composable shape
  NodeShape resolveShape(const ComposableNodeShape& shape);

  // Apply parameter bindings to a shape
  NodeShape applyParameterBindings(
      const NodeShape& shape,
      const std::unordered_map<std::string, std::string>& bindings);

  // Get the dependency graph
  const ShapeDependencyGraph& getDependencyGraph() const {
    return dependencyGraph_;
  }

  // Set merge strategy
  void setMergeStrategy(ConstraintMerger::MergeStrategy strategy) {
    mergeStrategy_ = strategy;
  }

  // Check for composition errors (cycles, missing dependencies, etc.)
  std::vector<std::string> validateComposition() const;

 private:
  ShaclShapeRegistry* registry_;
  ShapeDependencyGraph dependencyGraph_;
  ConstraintMerger::MergeStrategy mergeStrategy_ =
      ConstraintMerger::MergeStrategy::MostRestrictive;

  // Cache of resolved shapes to avoid redundant resolution
  mutable std::unordered_map<std::string, NodeShape> resolvedCache_;

  // Resolve shape inheritance (sh:extends)
  NodeShape resolveInheritance(const ComposableNodeShape& shape);

  // Apply node references (sh:node)
  NodeShape applyNodeReferences(const NodeShape& shape,
                                const std::vector<ShapeReference>& refs);

  // Merge two node shapes
  NodeShape mergeNodeShapes(const NodeShape& parent, const NodeShape& child);

  // Build dependency graph from composable shapes
  void buildDependencyGraph();

  // Substitute parameters in constraint values
  std::string substituteParameters(
      const std::string& value,
      const std::unordered_map<std::string, std::string>& bindings);

  // Substitute parameters in a constraint
  ShaclConstraint substituteConstraintParameters(
      const ShaclConstraint& constraint,
      const std::unordered_map<std::string, std::string>& bindings);
};

} // namespace shacl

#endif // QLEVER_ENGINE_SHACL_SHAPECOMPOSITION_H

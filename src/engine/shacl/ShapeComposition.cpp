#include "ShapeComposition.h"
#include "ShaclShapeRegistry.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace shacl {

// ============================================================================
// ShapeDependencyGraph Implementation
// ============================================================================

void ShapeDependencyGraph::addShape(
    const std::string& shapeId, const std::vector<std::string>& dependencies) {
  dependencies_[shapeId] = dependencies;

  // Build reverse dependency map
  for (const auto& dep : dependencies) {
    dependents_[dep].push_back(shapeId);
  }
}

std::vector<std::string> ShapeDependencyGraph::getResolutionOrder() const {
  std::vector<std::string> result;
  std::unordered_set<std::string> visited;
  std::unordered_set<std::string> recursionStack;

  // Process all shapes
  for (const auto& [shapeId, _] : dependencies_) {
    if (visited.find(shapeId) == visited.end()) {
      if (!topologicalSortUtil(shapeId, visited, recursionStack, result)) {
        // Cycle detected
        return {};
      }
    }
  }

  // Reverse to get correct order (dependencies first)
  std::reverse(result.begin(), result.end());
  return result;
}

bool ShapeDependencyGraph::hasCycle() const {
  return getResolutionOrder().empty() && !dependencies_.empty();
}

std::vector<std::string> ShapeDependencyGraph::getDependencies(
    const std::string& shapeId) const {
  auto it = dependencies_.find(shapeId);
  if (it != dependencies_.end()) {
    return it->second;
  }
  return {};
}

std::vector<std::string> ShapeDependencyGraph::getDependents(
    const std::string& shapeId) const {
  auto it = dependents_.find(shapeId);
  if (it != dependents_.end()) {
    return it->second;
  }
  return {};
}

void ShapeDependencyGraph::clear() {
  dependencies_.clear();
  dependents_.clear();
}

std::vector<std::string> ShapeDependencyGraph::getAllShapeIds() const {
  std::vector<std::string> ids;
  for (const auto& [shapeId, _] : dependencies_) {
    ids.push_back(shapeId);
  }
  return ids;
}

bool ShapeDependencyGraph::topologicalSortUtil(
    const std::string& shapeId, std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recursionStack,
    std::vector<std::string>& result) const {

  visited.insert(shapeId);
  recursionStack.insert(shapeId);

  // Visit all dependencies
  auto it = dependencies_.find(shapeId);
  if (it != dependencies_.end()) {
    for (const auto& dep : it->second) {
      if (recursionStack.find(dep) != recursionStack.end()) {
        // Cycle detected
        return false;
      }
      if (visited.find(dep) == visited.end()) {
        if (!topologicalSortUtil(dep, visited, recursionStack, result)) {
          return false;
        }
      }
    }
  }

  recursionStack.erase(shapeId);
  result.push_back(shapeId);
  return true;
}

// ============================================================================
// ConstraintMerger Implementation
// ============================================================================

std::vector<ShaclConstraint> ConstraintMerger::mergeConstraints(
    const std::vector<ShaclConstraint>& parentConstraints,
    const std::vector<ShaclConstraint>& childConstraints,
    MergeStrategy strategy) {

  std::vector<ShaclConstraint> merged;
  std::unordered_set<ConstraintType> processedTypes;

  // Process child constraints first
  for (const auto& childConstr : childConstraints) {
    processedTypes.insert(childConstr.type);

    // Find matching parent constraint
    auto parentIt = std::find_if(
        parentConstraints.begin(), parentConstraints.end(),
        [&](const ShaclConstraint& c) { return c.type == childConstr.type; });

    if (parentIt != parentConstraints.end()) {
      // Conflict exists - resolve it
      merged.push_back(resolveConflict(*parentIt, childConstr, strategy));
    } else {
      // No conflict - add child constraint
      merged.push_back(childConstr);
    }
  }

  // Add parent constraints that weren't overridden
  if (strategy == MergeStrategy::Accumulate ||
      strategy == MergeStrategy::MostRestrictive) {
    for (const auto& parentConstr : parentConstraints) {
      if (processedTypes.find(parentConstr.type) == processedTypes.end()) {
        merged.push_back(parentConstr);
      }
    }
  }

  return merged;
}

std::vector<PropertyShape> ConstraintMerger::mergePropertyShapes(
    const std::vector<PropertyShape>& parentShapes,
    const std::vector<PropertyShape>& childShapes, MergeStrategy strategy) {

  std::vector<PropertyShape> merged;
  std::unordered_set<std::string> processedPaths;

  // Process child shapes first
  for (const auto& childShape : childShapes) {
    processedPaths.insert(childShape.path);

    // Find matching parent shape
    auto parentIt = std::find_if(
        parentShapes.begin(), parentShapes.end(),
        [&](const PropertyShape& ps) { return ps.path == childShape.path; });

    if (parentIt != parentShapes.end()) {
      // Merge constraints for this property
      PropertyShape mergedShape;
      mergedShape.path = childShape.path;
      mergedShape.required = childShape.required || parentIt->required;
      mergedShape.constraints =
          mergeConstraints(parentIt->constraints, childShape.constraints,
                           strategy);
      merged.push_back(mergedShape);
    } else {
      // No parent shape for this path
      merged.push_back(childShape);
    }
  }

  // Add parent shapes that weren't overridden
  if (strategy == MergeStrategy::Accumulate ||
      strategy == MergeStrategy::MostRestrictive) {
    for (const auto& parentShape : parentShapes) {
      if (processedPaths.find(parentShape.path) == processedPaths.end()) {
        merged.push_back(parentShape);
      }
    }
  }

  return merged;
}

bool ConstraintMerger::hasConflict(const ShaclConstraint& c1,
                                   const ShaclConstraint& c2) {
  // Constraints of same type always potentially conflict
  if (c1.type == c2.type) {
    return c1.value != c2.value;
  }
  return false;
}

ShaclConstraint ConstraintMerger::resolveConflict(const ShaclConstraint& c1,
                                                  const ShaclConstraint& c2,
                                                  MergeStrategy strategy) {
  switch (strategy) {
  case MergeStrategy::Override:
    // c2 (child) overrides c1 (parent)
    return c2;

  case MergeStrategy::MostRestrictive:
    return getMostRestrictive(c1, c2);

  case MergeStrategy::Accumulate:
    // For accumulate, prefer child (similar to override)
    // In a full implementation, might create multiple constraints
    return c2;

  default:
    return c2;
  }
}

ShaclConstraint ConstraintMerger::getMostRestrictive(
    const ShaclConstraint& c1, const ShaclConstraint& c2) {

  if (c1.type != c2.type) {
    // Different types - return c2 (child)
    return c2;
  }

  switch (c1.type) {
  case ConstraintType::MinCount:
  case ConstraintType::MinLength:
  case ConstraintType::MinInclusive:
    // Higher minimum is more restrictive
    return std::get<int>(c1.value) > std::get<int>(c2.value) ? c1 : c2;

  case ConstraintType::MaxCount:
  case ConstraintType::MaxLength:
  case ConstraintType::MaxInclusive:
    // Lower maximum is more restrictive
    return std::get<int>(c1.value) < std::get<int>(c2.value) ? c1 : c2;

  case ConstraintType::Pattern:
    // More specific pattern is more restrictive (hard to determine - use child)
    return c2;

  case ConstraintType::In:
    // Smaller list is more restrictive
    return std::get<std::vector<std::string>>(c1.value).size() <
                   std::get<std::vector<std::string>>(c2.value).size()
               ? c1
               : c2;

  default:
    // For other types, prefer child
    return c2;
  }
}

bool ConstraintMerger::isMoreRestrictive(const ShaclConstraint& c1,
                                         const ShaclConstraint& c2) {
  auto mostRestrictive = getMostRestrictive(c1, c2);
  return mostRestrictive.value == c1.value;
}

// ============================================================================
// ShapeCompositionEngine Implementation
// ============================================================================

void ShapeCompositionEngine::resolveAllShapes() {
  buildDependencyGraph();

  // Check for cycles
  if (dependencyGraph_.hasCycle()) {
    throw std::runtime_error(
        "Circular dependency detected in shape composition");
  }

  // Get resolution order
  auto resolutionOrder = dependencyGraph_.getResolutionOrder();

  // Resolve shapes in order (dependencies first)
  for (const auto& shapeId : resolutionOrder) {
    auto shape = registry_->getShape(shapeId);
    if (shape) {
      // Try to cast to ComposableNodeShape
      // In practice, registry would store ComposableNodeShape
      // For now, we assume it's composable if it has dependencies
      ComposableNodeShape composable;
      composable.shapeId = shape->shapeId;
      composable.targetClasses = shape->targetClasses;
      composable.targetNodes = shape->targetNodes;
      composable.propertyShapes = shape->propertyShapes;
      composable.nodeConstraints = shape->nodeConstraints;
      composable.closed = shape->closed;

      auto resolved = resolveShape(composable);
      resolvedCache_[shapeId] = resolved;

      // Update registry with resolved shape
      registry_->registerShape(resolved);
    }
  }
}

NodeShape ShapeCompositionEngine::resolveShape(
    const ComposableNodeShape& shape) {

  // Check cache first
  auto it = resolvedCache_.find(shape.shapeId);
  if (it != resolvedCache_.end()) {
    return it->second;
  }

  // Resolve inheritance first
  NodeShape resolved = resolveInheritance(shape);

  // Apply node references
  resolved = applyNodeReferences(resolved, shape.nodeReferences);

  // Cache result
  resolvedCache_[shape.shapeId] = resolved;

  return resolved;
}

NodeShape ShapeCompositionEngine::applyParameterBindings(
    const NodeShape& shape,
    const std::unordered_map<std::string, std::string>& bindings) {

  NodeShape result = shape;

  // Substitute parameters in constraints
  for (auto& constraint : result.nodeConstraints) {
    constraint = substituteConstraintParameters(constraint, bindings);
  }

  // Substitute in property shapes
  for (auto& propShape : result.propertyShapes) {
    for (auto& constraint : propShape.constraints) {
      constraint = substituteConstraintParameters(constraint, bindings);
    }
  }

  return result;
}

std::vector<std::string>
ShapeCompositionEngine::validateComposition() const {
  std::vector<std::string> errors;

  // Check for cycles
  if (dependencyGraph_.hasCycle()) {
    errors.push_back("Circular dependency detected in shape hierarchy");
  }

  // Check for missing dependencies
  auto allShapeIds = dependencyGraph_.getAllShapeIds();
  for (const auto& shapeId : allShapeIds) {
    auto deps = dependencyGraph_.getDependencies(shapeId);
    for (const auto& dep : deps) {
      if (!registry_->hasShape(dep)) {
        errors.push_back("Shape '" + shapeId + "' depends on missing shape '" +
                         dep + "'");
      }
    }
  }

  return errors;
}

NodeShape ShapeCompositionEngine::resolveInheritance(
    const ComposableNodeShape& shape) {

  NodeShape result;
  result.shapeId = shape.shapeId;
  result.closed = shape.closed;

  // Start with base shape properties
  result.targetClasses = shape.targetClasses;
  result.targetNodes = shape.targetNodes;
  result.propertyShapes = shape.propertyShapes;
  result.nodeConstraints = shape.nodeConstraints;

  // Process parent shapes in order
  for (const auto& parentRef : shape.extends) {
    auto parentShape = registry_->getShape(parentRef.shapeId);
    if (!parentShape) {
      throw std::runtime_error("Parent shape not found: " + parentRef.shapeId);
    }

    // Apply parameter bindings to parent
    auto boundParent =
        applyParameterBindings(*parentShape, parentRef.parameterBindings);

    // Merge with current result
    result = mergeNodeShapes(boundParent, result);
  }

  return result;
}

NodeShape ShapeCompositionEngine::applyNodeReferences(
    const NodeShape& shape, const std::vector<ShapeReference>& refs) {

  NodeShape result = shape;

  // Apply each node reference
  for (const auto& ref : refs) {
    auto refShape = registry_->getShape(ref.shapeId);
    if (!refShape) {
      throw std::runtime_error("Referenced shape not found: " + ref.shapeId);
    }

    // Apply parameter bindings
    auto boundRef = applyParameterBindings(*refShape, ref.parameterBindings);

    // Merge constraints (node references accumulate)
    result.nodeConstraints = ConstraintMerger::mergeConstraints(
        result.nodeConstraints, boundRef.nodeConstraints, mergeStrategy_);

    result.propertyShapes = ConstraintMerger::mergePropertyShapes(
        result.propertyShapes, boundRef.propertyShapes, mergeStrategy_);
  }

  return result;
}

NodeShape ShapeCompositionEngine::mergeNodeShapes(const NodeShape& parent,
                                                  const NodeShape& child) {
  NodeShape merged;

  // Child properties take precedence
  merged.shapeId = child.shapeId;
  merged.closed = child.closed;

  // Merge target classes and nodes (union)
  merged.targetClasses = parent.targetClasses;
  merged.targetClasses.insert(merged.targetClasses.end(),
                              child.targetClasses.begin(),
                              child.targetClasses.end());

  merged.targetNodes = parent.targetNodes;
  merged.targetNodes.insert(merged.targetNodes.end(), child.targetNodes.begin(),
                            child.targetNodes.end());

  // Merge constraints
  merged.nodeConstraints = ConstraintMerger::mergeConstraints(
      parent.nodeConstraints, child.nodeConstraints, mergeStrategy_);

  merged.propertyShapes = ConstraintMerger::mergePropertyShapes(
      parent.propertyShapes, child.propertyShapes, mergeStrategy_);

  return merged;
}

void ShapeCompositionEngine::buildDependencyGraph() {
  dependencyGraph_.clear();

  // Get all shapes from registry
  auto allShapes = registry_->getAllShapes();

  for (const auto* shape : allShapes) {
    // Try to get dependencies (would need to extend registry to track these)
    // For now, we assume shapes are stored as ComposableNodeShape
    // In practice, registry would need to be modified
    std::vector<std::string> deps;
    // deps would come from shape->getDependencies()

    dependencyGraph_.addShape(shape->shapeId, deps);
  }
}

std::string ShapeCompositionEngine::substituteParameters(
    const std::string& value,
    const std::unordered_map<std::string, std::string>& bindings) {

  std::string result = value;

  // Simple parameter substitution: ${paramName}
  for (const auto& [name, val] : bindings) {
    std::string placeholder = "${" + name + "}";
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
      result.replace(pos, placeholder.length(), val);
      pos += val.length();
    }
  }

  return result;
}

ShaclConstraint ShapeCompositionEngine::substituteConstraintParameters(
    const ShaclConstraint& constraint,
    const std::unordered_map<std::string, std::string>& bindings) {

  ShaclConstraint result = constraint;

  // Substitute in string values
  if (std::holds_alternative<std::string>(result.value)) {
    std::string val = std::get<std::string>(result.value);
    result.value = substituteParameters(val, bindings);
  } else if (std::holds_alternative<std::vector<std::string>>(result.value)) {
    auto vals = std::get<std::vector<std::string>>(result.value);
    for (auto& val : vals) {
      val = substituteParameters(val, bindings);
    }
    result.value = vals;
  }

  // Substitute in message
  result.message = substituteParameters(result.message, bindings);

  return result;
}

} // namespace shacl

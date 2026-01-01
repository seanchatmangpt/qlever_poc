// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#ifndef QLEVER_SRC_SHEX_SHAPEINDEX_H
#define QLEVER_SRC_SHEX_SHAPEINDEX_H

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "parser/ShEx.h"
#include "util/Synchronized.h"

namespace shex {

// ============================================================================
// Predicate-to-Shapes Index
// Maps predicates to shapes that constrain them for fast shape filtering
// ============================================================================

class PredicateToShapesIndex {
 public:
  // Build index from schema
  void buildIndex(const ShExSchema& schema) {
    auto lock = index_.wlock();
    lock->clear();

    for (const auto& [shapeId, shape] : schema.getShapes()) {
      for (const auto& prop : shape.properties) {
        (*lock)[prop.predicate].insert(shapeId);
      }
    }
  }

  // Get shapes that constrain a given predicate
  absl::flat_hash_set<std::string> getShapesForPredicate(
      const std::string& predicate) const {
    auto lock = index_.rlock();
    auto it = lock->find(predicate);
    return it != lock->end() ? it->second : absl::flat_hash_set<std::string>{};
  }

  // Get all indexed predicates
  std::vector<std::string> getAllPredicates() const {
    auto lock = index_.rlock();
    std::vector<std::string> predicates;
    predicates.reserve(lock->size());
    for (const auto& [pred, _] : *lock) {
      predicates.push_back(pred);
    }
    return predicates;
  }

  size_t size() const { return index_.rlock()->size(); }

 private:
  ad_utility::Synchronized<
      absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>>
      index_;
};

// ============================================================================
// Type-to-Shapes Index
// Maps value types to shapes for efficient type-based filtering
// ============================================================================

class TypeToShapesIndex {
 public:
  void buildIndex(const ShExSchema& schema) {
    auto lock = index_.wlock();
    lock->clear();

    for (const auto& [shapeId, shape] : schema.getShapes()) {
      for (const auto& prop : shape.properties) {
        if (prop.valueConstraint.valueType.has_value()) {
          ValueType vt = prop.valueConstraint.valueType.value();
          (*lock)[vt].insert(shapeId);
        }
      }
    }
  }

  absl::flat_hash_set<std::string> getShapesForType(ValueType type) const {
    auto lock = index_.rlock();
    auto it = lock->find(type);
    return it != lock->end() ? it->second : absl::flat_hash_set<std::string>{};
  }

 private:
  ad_utility::Synchronized<
      absl::flat_hash_map<ValueType, absl::flat_hash_set<std::string>>>
      index_;
};

// ============================================================================
// Shape Inheritance Graph
// Tracks EXTENDS relationships for dependency resolution
// ============================================================================

class ShapeInheritanceGraph {
 public:
  void buildGraph(const ShExSchema& schema) {
    auto lock = graph_.wlock();
    lock->childToParent.clear();
    lock->parentToChildren.clear();

    for (const auto& [shapeId, shape] : schema.getShapes()) {
      if (shape.getExtends().has_value()) {
        const std::string& parentId = shape.getExtends().value();
        lock->childToParent[shapeId] = parentId;
        lock->parentToChildren[parentId].insert(shapeId);
      }
    }
  }

  // Get parent shape (if any)
  std::optional<std::string> getParent(const std::string& shapeId) const {
    auto lock = graph_.rlock();
    auto it = lock->childToParent.find(shapeId);
    return it != lock->childToParent.end()
               ? std::optional<std::string>(it->second)
               : std::nullopt;
  }

  // Get all child shapes
  absl::flat_hash_set<std::string> getChildren(
      const std::string& shapeId) const {
    auto lock = graph_.rlock();
    auto it = lock->parentToChildren.find(shapeId);
    return it != lock->parentToChildren.end()
               ? it->second
               : absl::flat_hash_set<std::string>{};
  }

  // Get full inheritance chain (bottom-up)
  std::vector<std::string> getInheritanceChain(
      const std::string& shapeId) const {
    std::vector<std::string> chain;
    std::optional<std::string> current = shapeId;

    absl::flat_hash_set<std::string> visited;  // Cycle detection
    while (current.has_value()) {
      if (visited.contains(current.value())) {
        break;  // Cycle detected
      }
      chain.push_back(current.value());
      visited.insert(current.value());
      current = getParent(current.value());
    }

    return chain;
  }

 private:
  struct Graph {
    absl::flat_hash_map<std::string, std::string> childToParent;
    absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>
        parentToChildren;
  };

  ad_utility::Synchronized<Graph> graph_;
};

// ============================================================================
// Compiled Shape Metadata Cache
// Pre-computes and caches expensive shape analysis
// ============================================================================

struct CompiledShapeMetadata {
  std::string shapeId;
  size_t propertyCount;
  bool isClosed;
  bool hasInheritance;
  absl::flat_hash_set<std::string> requiredPredicates;  // Cardinality >= 1
  absl::flat_hash_set<std::string> optionalPredicates;
  absl::flat_hash_set<std::string> extraPredicates;
  size_t estimatedMemoryBytes;

  CompiledShapeMetadata() = default;

  static CompiledShapeMetadata compile(const Shape& shape) {
    CompiledShapeMetadata meta;
    meta.shapeId = shape.id;
    meta.propertyCount = shape.properties.size();
    meta.isClosed = shape.closed;
    meta.hasInheritance = shape.getExtends().has_value();

    for (const auto& prop : shape.properties) {
      if (prop.cardinality == Cardinality::EXACTLY_ONE ||
          prop.cardinality == Cardinality::ONE_OR_MORE) {
        meta.requiredPredicates.insert(prop.predicate);
      } else {
        meta.optionalPredicates.insert(prop.predicate);
      }
    }

    meta.extraPredicates = shape.getExtraPredicates();

    // Estimate memory usage
    meta.estimatedMemoryBytes =
        sizeof(CompiledShapeMetadata) +
        meta.shapeId.size() +
        (meta.requiredPredicates.size() + meta.optionalPredicates.size() +
         meta.extraPredicates.size()) *
            sizeof(std::string);

    return meta;
  }
};

class ShapeMetadataCache {
 public:
  void buildCache(const ShExSchema& schema) {
    auto lock = cache_.wlock();
    lock->clear();

    for (const auto& [shapeId, shape] : schema.getShapes()) {
      (*lock)[shapeId] = CompiledShapeMetadata::compile(shape);
    }
  }

  std::optional<CompiledShapeMetadata> get(const std::string& shapeId) const {
    auto lock = cache_.rlock();
    auto it = lock->find(shapeId);
    return it != lock->end() ? std::optional<CompiledShapeMetadata>(it->second)
                             : std::nullopt;
  }

  size_t size() const { return cache_.rlock()->size(); }

 private:
  ad_utility::Synchronized<
      absl::flat_hash_map<std::string, CompiledShapeMetadata>>
      cache_;
};

// ============================================================================
// Unified Shape Index Manager
// Combines all indexes for efficient query processing
// ============================================================================

class ShapeIndexManager {
 public:
  void buildAllIndexes(const ShExSchema& schema) {
    predicateIndex_.buildIndex(schema);
    typeIndex_.buildIndex(schema);
    inheritanceGraph_.buildGraph(schema);
    metadataCache_.buildCache(schema);
  }

  const PredicateToShapesIndex& predicateIndex() const {
    return predicateIndex_;
  }
  const TypeToShapesIndex& typeIndex() const { return typeIndex_; }
  const ShapeInheritanceGraph& inheritanceGraph() const {
    return inheritanceGraph_;
  }
  const ShapeMetadataCache& metadataCache() const { return metadataCache_; }

  // Query: Get candidate shapes for a node based on its predicates
  absl::flat_hash_set<std::string> getCandidateShapes(
      const std::vector<std::string>& nodePredicates) const {
    absl::flat_hash_set<std::string> candidates;

    for (const auto& pred : nodePredicates) {
      auto shapes = predicateIndex_.getShapesForPredicate(pred);
      candidates.insert(shapes.begin(), shapes.end());
    }

    return candidates;
  }

  // Statistics
  struct IndexStats {
    size_t predicateIndexSize;
    size_t metadataCacheSize;
    size_t totalShapes;
    size_t shapesWithInheritance;
  };

  IndexStats stats() const {
    IndexStats s;
    s.predicateIndexSize = predicateIndex_.size();
    s.metadataCacheSize = metadataCache_.size();
    s.totalShapes = metadataCache_.size();

    // Count shapes with inheritance
    s.shapesWithInheritance = 0;
    auto lock = metadataCache_.cache_.rlock();
    for (const auto& [_, meta] : *lock) {
      if (meta.hasInheritance) {
        ++s.shapesWithInheritance;
      }
    }

    return s;
  }

 private:
  PredicateToShapesIndex predicateIndex_;
  TypeToShapesIndex typeIndex_;
  ShapeInheritanceGraph inheritanceGraph_;
  ShapeMetadataCache metadataCache_;
};

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_SHAPEINDEX_H

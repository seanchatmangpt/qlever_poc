#ifndef PARSER_SHEX_SHAPE_REFERENCE_H
#define PARSER_SHEX_SHAPE_REFERENCE_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>

#include "absl/container/flat_hash_set.h"
#include "absl/container/flat_hash_map.h"

namespace shex {

// Forward declarations
struct ValueSetConstraint;

// ============================================================================
// Shape Reference Data Structure
// ============================================================================

/**
 * Represents a reference to another shape using @ShapeName syntax.
 * Enables recursive and hierarchical shape validation.
 */
struct ShapeReference {
  std::string targetShapeId;  // The ID of the referenced shape
  bool isRecursive = false;   // Set to true if this reference is part of a cycle

  ShapeReference() = default;
  explicit ShapeReference(const std::string& shapeId)
      : targetShapeId(shapeId) {}

  bool operator==(const ShapeReference& other) const {
    return targetShapeId == other.targetShapeId;
  }
};

// ============================================================================
// Shape Constraint Variant
// ============================================================================

/**
 * A property constraint can be either:
 * - ValueSetConstraint: Traditional value-based constraint (IRI, LITERAL, etc.)
 * - ShapeReference: Reference to another shape for recursive validation
 */
using ShapeConstraint = std::variant<ValueSetConstraint, ShapeReference>;

// ============================================================================
// Validation Context with Cycle Detection
// ============================================================================

/**
 * Tracks the current validation path to detect and prevent infinite recursion.
 * Uses a stack-based approach to maintain the chain of shape validations.
 */
class ValidationContext {
 public:
  ValidationContext() = default;

  /**
   * Checks if we're currently validating the given shape (cycle detection).
   * @param shapeId The shape ID to check
   * @return true if this shape is already in the validation stack
   */
  bool isInValidationPath(const std::string& shapeId) const {
    return validationStack_.contains(shapeId);
  }

  /**
   * Enters a shape validation context.
   * @param shapeId The shape being validated
   * @return true if entry was successful (no cycle), false if cycle detected
   */
  bool enterShape(const std::string& shapeId) {
    if (isInValidationPath(shapeId)) {
      return false;  // Cycle detected
    }
    validationStack_.insert(shapeId);
    pathOrder_.push_back(shapeId);
    return true;
  }

  /**
   * Exits a shape validation context.
   * @param shapeId The shape being exited
   */
  void exitShape(const std::string& shapeId) {
    validationStack_.erase(shapeId);
    if (!pathOrder_.empty() && pathOrder_.back() == shapeId) {
      pathOrder_.pop_back();
    }
  }

  /**
   * Gets the current validation depth (for debugging and limits).
   */
  size_t getDepth() const { return validationStack_.size(); }

  /**
   * Gets the current validation path (for error reporting).
   */
  std::vector<std::string> getPath() const { return pathOrder_; }

  /**
   * Clears the validation context (for starting fresh validation).
   */
  void clear() {
    validationStack_.clear();
    pathOrder_.clear();
  }

  /**
   * Maximum recursion depth allowed (prevents stack overflow).
   * Can be adjusted based on system constraints.
   */
  static constexpr size_t MAX_RECURSION_DEPTH = 1000;

  /**
   * Checks if we've exceeded the maximum recursion depth.
   */
  bool isDepthExceeded() const {
    return getDepth() > MAX_RECURSION_DEPTH;
  }

 private:
  absl::flat_hash_set<std::string> validationStack_;  // For O(1) lookup
  std::vector<std::string> pathOrder_;                // For ordered path tracking
};

// ============================================================================
// Validation Guard (RAII Helper)
// ============================================================================

/**
 * RAII helper to automatically enter/exit shape validation context.
 * Ensures proper cleanup even in case of exceptions.
 */
class ValidationGuard {
 public:
  ValidationGuard(ValidationContext& context, const std::string& shapeId)
      : context_(context), shapeId_(shapeId), entered_(false) {
    entered_ = context_.enterShape(shapeId_);
  }

  ~ValidationGuard() {
    if (entered_) {
      context_.exitShape(shapeId_);
    }
  }

  // Non-copyable, non-movable
  ValidationGuard(const ValidationGuard&) = delete;
  ValidationGuard& operator=(const ValidationGuard&) = delete;
  ValidationGuard(ValidationGuard&&) = delete;
  ValidationGuard& operator=(ValidationGuard&&) = delete;

  /**
   * Returns true if the shape was successfully entered (no cycle).
   */
  bool isValid() const { return entered_; }

 private:
  ValidationContext& context_;
  std::string shapeId_;
  bool entered_;
};

// ============================================================================
// Shape Dependency Graph for Cycle Detection
// ============================================================================

/**
 * Analyzes shape dependencies to detect cycles using DFS.
 * Time Complexity: O(V + E) where V = shapes, E = references
 */
class ShapeDependencyGraph {
 public:
  /**
   * Adds a dependency edge from one shape to another.
   */
  void addDependency(const std::string& fromShape, const std::string& toShape) {
    dependencies_[fromShape].insert(toShape);
  }

  /**
   * Detects all cycles in the shape dependency graph.
   * @return A map of shape IDs that are part of cycles (with cycle paths)
   */
  absl::flat_hash_map<std::string, std::vector<std::string>> detectCycles() {
    absl::flat_hash_map<std::string, std::vector<std::string>> cycleInfo;
    absl::flat_hash_set<std::string> visited;
    absl::flat_hash_set<std::string> recursionStack;
    std::vector<std::string> currentPath;

    for (const auto& [shapeId, _] : dependencies_) {
      if (!visited.contains(shapeId)) {
        dfsCycleDetection(shapeId, visited, recursionStack, currentPath,
                         cycleInfo);
      }
    }

    return cycleInfo;
  }

  /**
   * Checks if a specific shape is part of any cycle.
   */
  bool isShapeRecursive(const std::string& shapeId) {
    auto cycles = detectCycles();
    return cycles.contains(shapeId);
  }

  /**
   * Gets all shapes that a given shape depends on (direct dependencies).
   */
  const absl::flat_hash_set<std::string>* getDependencies(
      const std::string& shapeId) const {
    auto it = dependencies_.find(shapeId);
    return (it != dependencies_.end()) ? &it->second : nullptr;
  }

  /**
   * Clears all dependency information.
   */
  void clear() { dependencies_.clear(); }

 private:
  /**
   * DFS-based cycle detection algorithm.
   * Detects cycles and records the cycle path for each shape involved.
   */
  void dfsCycleDetection(
      const std::string& shapeId,
      absl::flat_hash_set<std::string>& visited,
      absl::flat_hash_set<std::string>& recursionStack,
      std::vector<std::string>& currentPath,
      absl::flat_hash_map<std::string, std::vector<std::string>>& cycleInfo) {

    visited.insert(shapeId);
    recursionStack.insert(shapeId);
    currentPath.push_back(shapeId);

    // Explore all dependencies
    auto it = dependencies_.find(shapeId);
    if (it != dependencies_.end()) {
      for (const auto& dependentShape : it->second) {
        if (!visited.contains(dependentShape)) {
          // Continue DFS
          dfsCycleDetection(dependentShape, visited, recursionStack,
                           currentPath, cycleInfo);
        } else if (recursionStack.contains(dependentShape)) {
          // Cycle detected! Record the cycle path
          auto cycleStart = std::find(currentPath.begin(), currentPath.end(),
                                     dependentShape);
          std::vector<std::string> cyclePath(cycleStart, currentPath.end());
          cyclePath.push_back(dependentShape);  // Complete the cycle

          // Mark all shapes in the cycle
          for (size_t i = 0; i < cyclePath.size() - 1; ++i) {
            cycleInfo[cyclePath[i]] = cyclePath;
          }
        }
      }
    }

    // Backtrack
    currentPath.pop_back();
    recursionStack.erase(shapeId);
  }

  // Map from shape ID to set of shape IDs it references
  absl::flat_hash_map<std::string, absl::flat_hash_set<std::string>>
      dependencies_;
};

}  // namespace shex

#endif  // PARSER_SHEX_SHAPE_REFERENCE_H

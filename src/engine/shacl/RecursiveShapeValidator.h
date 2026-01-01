#ifndef QLEVER_ENGINE_SHACL_RECURSIVESHAPEVALIDATOR_H
#define QLEVER_ENGINE_SHACL_RECURSIVESHAPEVALIDATOR_H

#include "ShaclShape.h"
#include "ShaclShapeRegistry.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shacl {

// Handles recursive SHACL shape validation with circular reference detection
// and memoization to avoid redundant validation.
//
// Supports:
// - sh:node constraints (validating a node with another shape)
// - sh:shape references (recursive validation with another shape)
// - Circular reference detection via validation stack
// - Memoization cache for performance optimization
class RecursiveShapeValidator {
 public:
  // Validation context that tracks the current validation state
  struct ValidationContext {
    // Stack of (nodeId, shapeId) pairs to detect circular references
    std::vector<std::pair<std::string, std::string>> validationStack;

    // Memoization cache: (nodeId, shapeId) -> ValidationResult
    std::unordered_map<std::string, ValidationResult> memoCache;

    // Maximum recursion depth to prevent stack overflow
    size_t maxDepth = 100;

    // Check if currently validating a (node, shape) pair
    bool isInValidationStack(const std::string& nodeId,
                             const std::string& shapeId) const;

    // Add to validation stack
    void pushValidation(const std::string& nodeId,
                        const std::string& shapeId);

    // Remove from validation stack
    void popValidation();

    // Get memoization key
    static std::string getMemoKey(const std::string& nodeId,
                                  const std::string& shapeId);

    // Check if result is memoized
    bool hasMemoized(const std::string& nodeId,
                     const std::string& shapeId) const;

    // Get memoized result
    const ValidationResult& getMemoized(const std::string& nodeId,
                                        const std::string& shapeId) const;

    // Store result in memo cache
    void memoize(const std::string& nodeId, const std::string& shapeId,
                 const ValidationResult& result);
  };

  // RAII guard for validation stack management
  class ValidationGuard {
   public:
    ValidationGuard(ValidationContext& ctx, const std::string& nodeId,
                    const std::string& shapeId)
        : context_(ctx) {
      context_.pushValidation(nodeId, shapeId);
    }

    ~ValidationGuard() { context_.popValidation(); }

    // Prevent copying
    ValidationGuard(const ValidationGuard&) = delete;
    ValidationGuard& operator=(const ValidationGuard&) = delete;

   private:
    ValidationContext& context_;
  };

 public:
  // Constructor
  explicit RecursiveShapeValidator(const ShaclShapeRegistry* registry);

  // Validate a node against a shape with recursive constraint support
  // Returns ValidationResult with any violations found
  ValidationResult validateNodeWithShape(const std::string& nodeId,
                                         const std::string& shapeId,
                                         ValidationContext& context);

  // Validate a node constraint (sh:node)
  // The node itself must conform to the referenced shape
  ValidationResult validateNodeConstraint(const std::string& nodeId,
                                          const std::string& targetShapeId,
                                          ValidationContext& context);

  // Validate a shape reference (sh:shape)
  // Used within property shapes to validate property values
  ValidationResult validateShapeReference(const std::string& valueNodeId,
                                          const std::string& shapeId,
                                          ValidationContext& context);

  // Validate multiple nodes against a shape
  std::vector<ValidationResult> validateNodesWithShape(
      const std::vector<std::string>& nodeIds, const std::string& shapeId);

  // Create a new validation context
  ValidationContext createContext(size_t maxDepth = 100);

  // Clear memoization cache
  void clearCache();

  // Get statistics
  size_t getCacheSize() const { return globalCacheSize_; }
  size_t getValidationCount() const { return validationCount_; }
  size_t getCacheHits() const { return cacheHits_; }

 private:
  // Reference to shape registry (non-owning)
  const ShaclShapeRegistry* registry_;

  // Global statistics
  size_t globalCacheSize_ = 0;
  size_t validationCount_ = 0;
  size_t cacheHits_ = 0;

  // Core validation logic (delegates to ShaclConstraintEvaluator for
  // non-recursive constraints)
  ValidationResult validateNodeWithShapeImpl(const std::string& nodeId,
                                             const NodeShape& shape,
                                             ValidationContext& context);

  // Check for circular reference
  bool wouldCreateCircularReference(const std::string& nodeId,
                                    const std::string& shapeId,
                                    const ValidationContext& context) const;

  // Validate node constraints (non-recursive)
  void validateNodeConstraints(const std::string& nodeId,
                               const NodeShape& shape,
                               ValidationResult& result);

  // Validate property shapes (may trigger recursive validation)
  void validatePropertyShapes(const std::string& nodeId,
                              const NodeShape& shape,
                              ValidationContext& context,
                              ValidationResult& result);

  // Validate a single property shape with recursive support
  void validatePropertyShape(const std::string& nodeId,
                             const PropertyShape& propShape,
                             const std::vector<std::string>& values,
                             ValidationContext& context,
                             ValidationResult& result);

  // Extract property values for a node (placeholder - would integrate with
  // index)
  std::vector<std::string> getPropertyValues(const std::string& nodeId,
                                             const std::string& propertyPath);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_RECURSIVESHAPEVALIDATOR_H

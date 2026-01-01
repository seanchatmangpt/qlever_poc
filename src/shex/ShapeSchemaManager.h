// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: AI Assistant (Claude Code)

#ifndef QLEVER_SRC_SHEX_SHAPESCHEMAMANAGER_H
#define QLEVER_SRC_SHEX_SHAPESCHEMAMANAGER_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "global/Id.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Variable.h"
#include "util/HashMap.h"
#include "util/HashSet.h"

namespace shex {

// Forward declarations
struct ShapeExpression;
struct TripleConstraint;
struct NodeConstraint;

// Configuration for shape validation
struct ValidationConfig {
  enum class Mode {
    STRICT,  // Throw exception on validation failure
    LAX,     // Filter out invalid bindings
    REPORT   // Add validation metadata to results
  };

  Mode mode = Mode::LAX;
  bool enableOptimization = true;
  bool collectStatistics = false;
};

// Statistics for shape validation and optimization
struct ValidationStatistics {
  size_t totalValidations = 0;
  size_t successfulValidations = 0;
  size_t failedValidations = 0;
  size_t optimizationHintsUsed = 0;
  double avgCardinalityReduction = 0.0;

  void reset() {
    totalValidations = 0;
    successfulValidations = 0;
    failedValidations = 0;
    optimizationHintsUsed = 0;
    avgCardinalityReduction = 0.0;
  }
};

// Optimization hints derived from shape constraints
struct ShapeOptimizationHints {
  // Estimated cardinality reduction factor (0.0-1.0)
  double selectivityFactor = 1.0;

  // Type constraints for efficient filtering
  ad_utility::HashSet<Id> requiredTypes;

  // Predicate constraints for index selection
  ad_utility::HashSet<Id> requiredPredicates;

  // Cardinality constraints (min/max occurrences)
  std::optional<size_t> minCardinality;
  std::optional<size_t> maxCardinality;

  // Whether this shape allows additional properties
  bool isClosed = false;

  // Variables that must be bound for this shape to apply
  ad_utility::HashSet<Variable> requiredBindings;
};

// Node constraint (type, datatype, value restrictions)
struct NodeConstraint {
  ad_utility::HashSet<Iri> nodeKinds;  // IRI, Literal, BlankNode
  ad_utility::HashSet<Iri> datatypes;
  ad_utility::HashSet<Id> values;  // Specific allowed values
  std::optional<std::string> pattern;  // Regex pattern for literals
  std::optional<size_t> minLength;
  std::optional<size_t> maxLength;

  bool isEmpty() const {
    return nodeKinds.empty() && datatypes.empty() && values.empty() &&
           !pattern.has_value() && !minLength.has_value() &&
           !maxLength.has_value();
  }
};

// Triple constraint (property shape)
struct TripleConstraint {
  Iri predicate;
  std::optional<NodeConstraint> valueConstraint;
  size_t minCount = 0;
  size_t maxCount = std::numeric_limits<size_t>::max();
  bool inverse = false;  // For inverse property paths

  // Estimated selectivity of this constraint
  double estimatedSelectivity() const {
    // Simple heuristic: tighter constraints = lower selectivity
    double factor = 1.0;
    if (minCount > 0) factor *= 0.5;
    if (maxCount < std::numeric_limits<size_t>::max()) factor *= 0.7;
    if (valueConstraint.has_value() && !valueConstraint->isEmpty()) {
      factor *= 0.6;
    }
    return factor;
  }
};

// Shape expression (defines structure for a set of nodes)
struct ShapeExpression {
  Iri id;
  std::string label;
  bool isClosed = false;
  std::optional<NodeConstraint> nodeConstraint;
  std::vector<TripleConstraint> tripleConstraints;

  // Compute optimization hints from this shape
  ShapeOptimizationHints computeHints() const;
};

/**
 * @brief Manages ShEx shape schemas for validation and query optimization
 *
 * This class provides:
 * - O(1) shape lookup by IRI
 * - O(1) predicate-to-shapes mapping for optimization
 * - Metadata persistence to index files
 * - Shape-driven query optimization hints
 *
 * Thread-safe for concurrent reads after initialization.
 */
class ShapeSchemaManager {
 public:
  ShapeSchemaManager() = default;

  // Load shapes from a ShExC file
  void loadFromFile(const std::string& filename);

  // Load shapes from ShEx JSON
  void loadFromJson(const std::string& jsonContent);

  // Register a single shape programmatically
  void registerShape(ShapeExpression shape);

  // Clear all registered shapes
  void clear();

  // Get shape by IRI (O(1))
  std::optional<const ShapeExpression*> getShape(const Iri& shapeId) const;

  // Get all shapes that reference a given predicate (O(1) amortized)
  std::vector<const ShapeExpression*> getShapesForPredicate(
      const Iri& predicate) const;

  // Get optimization hints for a shape (O(1))
  std::optional<ShapeOptimizationHints> getOptimizationHints(
      const Iri& shapeId) const;

  // Get all registered shape IRIs
  std::vector<Iri> getAllShapeIds() const;

  // Check if a shape is registered
  bool hasShape(const Iri& shapeId) const;

  // Get number of registered shapes
  size_t shapeCount() const { return shapes_.size(); }

  // Persist metadata to a binary file
  void saveMetadata(const std::string& filename) const;

  // Load metadata from a binary file
  void loadMetadata(const std::string& filename);

  // Get validation statistics
  const ValidationStatistics& getStatistics() const { return statistics_; }

  // Reset validation statistics
  void resetStatistics() { statistics_.reset(); }

  // Enable/disable statistics collection
  void setCollectStatistics(bool enable) {
    collectStatistics_ = enable;
    if (!enable) {
      statistics_.reset();
    }
  }

  // Record a validation event (for statistics)
  void recordValidation(bool success, double cardinalityReduction = 0.0);

  // Record an optimization hint usage
  void recordOptimizationHint();

 private:
  // Map from shape IRI to shape expression (O(1) lookup)
  ad_utility::HashMap<Iri, std::shared_ptr<ShapeExpression>> shapes_;

  // Map from predicate IRI to shapes that use it (O(1) amortized lookup)
  ad_utility::HashMap<Iri, std::vector<Iri>> predicateToShapes_;

  // Cached optimization hints (computed lazily)
  mutable ad_utility::HashMap<Iri, ShapeOptimizationHints> hintsCache_;

  // Validation statistics
  ValidationStatistics statistics_;

  // Whether to collect statistics
  bool collectStatistics_ = false;

  // Rebuild predicate-to-shapes index
  void rebuildPredicateIndex();

  // Parse ShExC format (simplified parser)
  void parseShExC(const std::string& content);

  // Parse ShEx JSON format
  void parseShExJson(const std::string& jsonContent);
};

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_SHAPESCHEMAMANAGER_H

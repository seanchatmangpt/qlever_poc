#ifndef PARSER_SHEX_NEGATION_H
#define PARSER_SHEX_NEGATION_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"

namespace shex {

// ============================================================================
// Negation Operators for ShEx Constraints
// ============================================================================

enum class NegationOperator {
  NONE,  // No negation
  NOT    // Negation operator (!)
};

// ============================================================================
// Logical Operators for Constraint Composition
// ============================================================================

enum class LogicalOperator {
  NONE,  // Single constraint
  AND,   // Conjunction
  OR     // Disjunction
};

// ============================================================================
// Forward Declarations
// ============================================================================

struct ConstraintNode;

// ============================================================================
// Constraint Node - Supports Logical Composition with Negation
// ============================================================================

struct ConstraintNode {
  // Type of constraint (for simple constraints)
  enum class Type {
    VALUE_TYPE,      // IRI, LITERAL, BNODE
    VALUE_SET,       // Set of allowed values
    PROPERTY_SHAPE,  // Reference to property shape
    SHAPE_REF,       // Reference to named shape
    LOGICAL          // Logical composition (AND/OR)
  };

  Type type;
  NegationOperator negation = NegationOperator::NONE;
  LogicalOperator logicalOp = LogicalOperator::NONE;

  // For VALUE_TYPE constraints
  std::optional<std::string> valueType;

  // For VALUE_SET constraints
  absl::flat_hash_set<std::string> valueSet;

  // For SHAPE_REF constraints
  std::optional<std::string> shapeRef;

  // For LOGICAL composition (AND/OR)
  std::vector<std::shared_ptr<ConstraintNode>> children;

  // Constructors
  ConstraintNode() : type(Type::VALUE_TYPE) {}
  explicit ConstraintNode(Type t) : type(t) {}

  // Factory methods for creating specific constraint types
  static std::shared_ptr<ConstraintNode> createValueType(
      const std::string& vtype, NegationOperator neg = NegationOperator::NONE) {
    auto node = std::make_shared<ConstraintNode>(Type::VALUE_TYPE);
    node->valueType = vtype;
    node->negation = neg;
    return node;
  }

  static std::shared_ptr<ConstraintNode> createValueSet(
      const absl::flat_hash_set<std::string>& values,
      NegationOperator neg = NegationOperator::NONE) {
    auto node = std::make_shared<ConstraintNode>(Type::VALUE_SET);
    node->valueSet = values;
    node->negation = neg;
    return node;
  }

  static std::shared_ptr<ConstraintNode> createShapeRef(
      const std::string& ref, NegationOperator neg = NegationOperator::NONE) {
    auto node = std::make_shared<ConstraintNode>(Type::SHAPE_REF);
    node->shapeRef = ref;
    node->negation = neg;
    return node;
  }

  static std::shared_ptr<ConstraintNode> createLogical(
      LogicalOperator op,
      const std::vector<std::shared_ptr<ConstraintNode>>& childNodes,
      NegationOperator neg = NegationOperator::NONE) {
    auto node = std::make_shared<ConstraintNode>(Type::LOGICAL);
    node->logicalOp = op;
    node->children = childNodes;
    node->negation = neg;
    return node;
  }

  // Negate this constraint node
  void negate() {
    negation = (negation == NegationOperator::NOT)
        ? NegationOperator::NONE
        : NegationOperator::NOT;
  }

  // Check if this constraint is negated
  bool isNegated() const { return negation == NegationOperator::NOT; }

  // Get constraint description for debugging
  std::string toString() const;
};

// ============================================================================
// De Morgan's Laws Optimization
// ============================================================================

class DeMorganOptimizer {
 public:
  // Apply De Morgan's laws to optimize constraint tree
  // !(A AND B) -> !A OR !B
  // !(A OR B) -> !A AND !B
  static std::shared_ptr<ConstraintNode> optimize(
      std::shared_ptr<ConstraintNode> node);

  // Check if two constraints form a tautology (A OR !A = true)
  static bool isTautology(const std::shared_ptr<ConstraintNode>& node1,
                         const std::shared_ptr<ConstraintNode>& node2);

  // Check if two constraints form a contradiction (A AND !A = false)
  static bool isContradiction(const std::shared_ptr<ConstraintNode>& node1,
                             const std::shared_ptr<ConstraintNode>& node2);

  // Eliminate double negation: !!A -> A
  static std::shared_ptr<ConstraintNode> eliminateDoubleNegation(
      std::shared_ptr<ConstraintNode> node);

  // Apply all optimization passes
  static std::shared_ptr<ConstraintNode> fullyOptimize(
      std::shared_ptr<ConstraintNode> node);

 private:
  // Apply De Morgan's transformation to a negated logical node
  static std::shared_ptr<ConstraintNode> applyDeMorgan(
      std::shared_ptr<ConstraintNode> node);

  // Check if two nodes are semantically equivalent
  static bool areEquivalent(const std::shared_ptr<ConstraintNode>& node1,
                           const std::shared_ptr<ConstraintNode>& node2);

  // Check if node2 is the negation of node1
  static bool isNegationOf(const std::shared_ptr<ConstraintNode>& node1,
                          const std::shared_ptr<ConstraintNode>& node2);

  // Recursively optimize child nodes
  static void optimizeChildren(std::shared_ptr<ConstraintNode> node);
};

// ============================================================================
// Constraint Evaluator with Negation Support
// ============================================================================

class ConstraintEvaluator {
 public:
  // Evaluate a constraint node against a value
  static bool evaluate(const std::shared_ptr<ConstraintNode>& node,
                      const std::string& value,
                      const std::string& valueTypeStr);

  // Evaluate value type constraint
  static bool evaluateValueType(const std::string& constraint,
                               const std::string& actualType,
                               bool negated);

  // Evaluate value set constraint
  static bool evaluateValueSet(const absl::flat_hash_set<std::string>& valueSet,
                              const std::string& value,
                              bool negated);

  // Evaluate logical composition (AND/OR)
  static bool evaluateLogical(const std::shared_ptr<ConstraintNode>& node,
                             const std::string& value,
                             const std::string& valueTypeStr);

 private:
  // Helper to negate boolean result based on negation operator
  static bool applyNegation(bool result, NegationOperator neg) {
    return (neg == NegationOperator::NOT) ? !result : result;
  }
};

// ============================================================================
// Statistics and Performance Tracking
// ============================================================================

struct NegationStats {
  size_t totalNegations = 0;
  size_t deMorganOptimizations = 0;
  size_t doubleNegationEliminations = 0;
  size_t tautologiesDetected = 0;
  size_t contradictionsDetected = 0;
  double optimizationSpeedup = 0.0;  // Ratio: baseline / optimized

  void reset() {
    totalNegations = 0;
    deMorganOptimizations = 0;
    doubleNegationEliminations = 0;
    tautologiesDetected = 0;
    contradictionsDetected = 0;
    optimizationSpeedup = 0.0;
  }

  std::string toString() const;
};

// Global statistics tracker
extern NegationStats g_negationStats;

}  // namespace shex

#endif  // PARSER_SHEX_NEGATION_H

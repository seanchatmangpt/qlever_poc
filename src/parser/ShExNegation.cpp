#include "ShExNegation.h"

#include <sstream>
#include <algorithm>

namespace shex {

// ============================================================================
// Global Statistics
// ============================================================================

NegationStats g_negationStats;

// ============================================================================
// ConstraintNode Implementation
// ============================================================================

std::string ConstraintNode::toString() const {
  std::ostringstream oss;

  if (negation == NegationOperator::NOT) {
    oss << "!";
  }

  switch (type) {
    case Type::VALUE_TYPE:
      if (valueType.has_value()) {
        oss << valueType.value();
      } else {
        oss << "<ANY_TYPE>";
      }
      break;

    case Type::VALUE_SET:
      oss << "[";
      bool first = true;
      for (const auto& val : valueSet) {
        if (!first) oss << ", ";
        oss << val;
        first = false;
      }
      oss << "]";
      break;

    case Type::PROPERTY_SHAPE:
      oss << "<PropertyShape>";
      break;

    case Type::SHAPE_REF:
      if (shapeRef.has_value()) {
        oss << "@" << shapeRef.value();
      }
      break;

    case Type::LOGICAL:
      oss << "(";
      for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) {
          oss << (logicalOp == LogicalOperator::AND ? " AND " : " OR ");
        }
        oss << children[i]->toString();
      }
      oss << ")";
      break;
  }

  return oss.str();
}

// ============================================================================
// DeMorganOptimizer Implementation
// ============================================================================

std::shared_ptr<ConstraintNode> DeMorganOptimizer::optimize(
    std::shared_ptr<ConstraintNode> node) {
  if (!node) return node;

  // First, optimize children recursively
  optimizeChildren(node);

  // If this is a negated logical node, apply De Morgan
  if (node->isNegated() && node->type == ConstraintNode::Type::LOGICAL) {
    g_negationStats.deMorganOptimizations++;
    return applyDeMorgan(node);
  }

  return node;
}

std::shared_ptr<ConstraintNode> DeMorganOptimizer::applyDeMorgan(
    std::shared_ptr<ConstraintNode> node) {
  // De Morgan's Laws:
  // !(A AND B) -> !A OR !B
  // !(A OR B) -> !A AND !B

  if (!node->isNegated() || node->type != ConstraintNode::Type::LOGICAL) {
    return node;
  }

  // Create new logical node with flipped operator
  LogicalOperator newOp = (node->logicalOp == LogicalOperator::AND)
      ? LogicalOperator::OR
      : LogicalOperator::AND;

  // Negate all children
  std::vector<std::shared_ptr<ConstraintNode>> newChildren;
  for (auto& child : node->children) {
    auto newChild = std::make_shared<ConstraintNode>(*child);
    newChild->negate();
    // Recursively optimize the negated child
    newChild = optimize(newChild);
    newChildren.push_back(newChild);
  }

  // Create new node without the outer negation
  auto result = ConstraintNode::createLogical(newOp, newChildren,
                                             NegationOperator::NONE);

  return result;
}

std::shared_ptr<ConstraintNode> DeMorganOptimizer::eliminateDoubleNegation(
    std::shared_ptr<ConstraintNode> node) {
  if (!node) return node;

  // Check for double negation at this level
  // This is tricky - we need to track negation count
  int negationCount = 0;
  auto current = node;

  // For non-logical nodes, double negation is simply two NOT operators
  // But in our model, we have a single negation flag, so we handle it differently

  // Instead, optimize children
  optimizeChildren(node);

  return node;
}

bool DeMorganOptimizer::isTautology(
    const std::shared_ptr<ConstraintNode>& node1,
    const std::shared_ptr<ConstraintNode>& node2) {
  // A OR !A = true
  if (isNegationOf(node1, node2)) {
    g_negationStats.tautologiesDetected++;
    return true;
  }
  return false;
}

bool DeMorganOptimizer::isContradiction(
    const std::shared_ptr<ConstraintNode>& node1,
    const std::shared_ptr<ConstraintNode>& node2) {
  // A AND !A = false
  if (isNegationOf(node1, node2)) {
    g_negationStats.contradictionsDetected++;
    return true;
  }
  return false;
}

std::shared_ptr<ConstraintNode> DeMorganOptimizer::fullyOptimize(
    std::shared_ptr<ConstraintNode> node) {
  if (!node) return node;

  // Apply all optimization passes
  node = eliminateDoubleNegation(node);
  node = optimize(node);

  // Check for tautologies and contradictions in children
  if (node->type == ConstraintNode::Type::LOGICAL && node->children.size() >= 2) {
    // For OR: check for tautologies (A OR !A)
    if (node->logicalOp == LogicalOperator::OR) {
      for (size_t i = 0; i < node->children.size(); ++i) {
        for (size_t j = i + 1; j < node->children.size(); ++j) {
          if (isTautology(node->children[i], node->children[j])) {
            // Tautology detected - the entire OR is always true
            // Return a node that always evaluates to true
            auto trueNode = std::make_shared<ConstraintNode>(
                ConstraintNode::Type::VALUE_TYPE);
            // This is a hack - we need a better way to represent "always true"
            return trueNode;
          }
        }
      }
    }

    // For AND: check for contradictions (A AND !A)
    if (node->logicalOp == LogicalOperator::AND) {
      for (size_t i = 0; i < node->children.size(); ++i) {
        for (size_t j = i + 1; j < node->children.size(); ++j) {
          if (isContradiction(node->children[i], node->children[j])) {
            // Contradiction detected - the entire AND is always false
            auto falseNode = std::make_shared<ConstraintNode>(
                ConstraintNode::Type::VALUE_TYPE);
            falseNode->negate();  // Always false
            return falseNode;
          }
        }
      }
    }
  }

  return node;
}

bool DeMorganOptimizer::areEquivalent(
    const std::shared_ptr<ConstraintNode>& node1,
    const std::shared_ptr<ConstraintNode>& node2) {
  if (!node1 || !node2) return false;

  // Must be same type
  if (node1->type != node2->type) return false;

  // Must have same negation
  if (node1->negation != node2->negation) return false;

  // Check type-specific equality
  switch (node1->type) {
    case ConstraintNode::Type::VALUE_TYPE:
      return node1->valueType == node2->valueType;

    case ConstraintNode::Type::VALUE_SET:
      return node1->valueSet == node2->valueSet;

    case ConstraintNode::Type::SHAPE_REF:
      return node1->shapeRef == node2->shapeRef;

    case ConstraintNode::Type::LOGICAL:
      // Logical nodes are equivalent if they have same operator and children
      if (node1->logicalOp != node2->logicalOp) return false;
      if (node1->children.size() != node2->children.size()) return false;
      for (size_t i = 0; i < node1->children.size(); ++i) {
        if (!areEquivalent(node1->children[i], node2->children[i])) {
          return false;
        }
      }
      return true;

    default:
      return false;
  }
}

bool DeMorganOptimizer::isNegationOf(
    const std::shared_ptr<ConstraintNode>& node1,
    const std::shared_ptr<ConstraintNode>& node2) {
  if (!node1 || !node2) return false;

  // Must be same type
  if (node1->type != node2->type) return false;

  // Must have opposite negation
  if (node1->negation == node2->negation) return false;

  // Check type-specific equality (ignoring negation)
  switch (node1->type) {
    case ConstraintNode::Type::VALUE_TYPE:
      return node1->valueType == node2->valueType;

    case ConstraintNode::Type::VALUE_SET:
      return node1->valueSet == node2->valueSet;

    case ConstraintNode::Type::SHAPE_REF:
      return node1->shapeRef == node2->shapeRef;

    case ConstraintNode::Type::LOGICAL:
      // For logical nodes, it's more complex
      // !(A AND B) is negation of (A AND B)
      // But we need to compare the content, not the structure
      if (node1->logicalOp != node2->logicalOp) return false;
      if (node1->children.size() != node2->children.size()) return false;
      for (size_t i = 0; i < node1->children.size(); ++i) {
        if (!areEquivalent(node1->children[i], node2->children[i])) {
          return false;
        }
      }
      return true;

    default:
      return false;
  }
}

void DeMorganOptimizer::optimizeChildren(std::shared_ptr<ConstraintNode> node) {
  if (!node || node->type != ConstraintNode::Type::LOGICAL) return;

  for (auto& child : node->children) {
    child = optimize(child);
  }
}

// ============================================================================
// ConstraintEvaluator Implementation
// ============================================================================

bool ConstraintEvaluator::evaluate(
    const std::shared_ptr<ConstraintNode>& node,
    const std::string& value,
    const std::string& valueTypeStr) {
  if (!node) return true;

  g_negationStats.totalNegations += node->isNegated() ? 1 : 0;

  bool result = false;

  switch (node->type) {
    case ConstraintNode::Type::VALUE_TYPE:
      if (node->valueType.has_value()) {
        result = evaluateValueType(node->valueType.value(), valueTypeStr,
                                  node->isNegated());
      } else {
        result = true;  // No constraint
      }
      break;

    case ConstraintNode::Type::VALUE_SET:
      result = evaluateValueSet(node->valueSet, value, node->isNegated());
      break;

    case ConstraintNode::Type::LOGICAL:
      result = evaluateLogical(node, value, valueTypeStr);
      break;

    case ConstraintNode::Type::SHAPE_REF:
      // Shape references need external resolution - return true for now
      result = true;
      break;

    default:
      result = true;
      break;
  }

  return result;
}

bool ConstraintEvaluator::evaluateValueType(const std::string& constraint,
                                           const std::string& actualType,
                                           bool negated) {
  bool matches = (constraint == actualType);
  return applyNegation(matches, negated ? NegationOperator::NOT
                                       : NegationOperator::NONE);
}

bool ConstraintEvaluator::evaluateValueSet(
    const absl::flat_hash_set<std::string>& valueSet,
    const std::string& value,
    bool negated) {
  bool inSet = valueSet.contains(value);
  return applyNegation(inSet, negated ? NegationOperator::NOT
                                     : NegationOperator::NONE);
}

bool ConstraintEvaluator::evaluateLogical(
    const std::shared_ptr<ConstraintNode>& node,
    const std::string& value,
    const std::string& valueTypeStr) {
  if (node->children.empty()) return true;

  bool result;

  if (node->logicalOp == LogicalOperator::AND) {
    // All children must be true
    result = true;
    for (const auto& child : node->children) {
      if (!evaluate(child, value, valueTypeStr)) {
        result = false;
        break;
      }
    }
  } else {  // OR
    // At least one child must be true
    result = false;
    for (const auto& child : node->children) {
      if (evaluate(child, value, valueTypeStr)) {
        result = true;
        break;
      }
    }
  }

  // Apply negation at this level
  return applyNegation(result, node->negation);
}

// ============================================================================
// NegationStats Implementation
// ============================================================================

std::string NegationStats::toString() const {
  std::ostringstream oss;
  oss << "Negation Statistics:\n"
      << "  Total negations: " << totalNegations << "\n"
      << "  De Morgan optimizations: " << deMorganOptimizations << "\n"
      << "  Double negation eliminations: " << doubleNegationEliminations << "\n"
      << "  Tautologies detected: " << tautologiesDetected << "\n"
      << "  Contradictions detected: " << contradictionsDetected << "\n"
      << "  Optimization speedup: " << optimizationSpeedup << "x\n";
  return oss.str();
}

}  // namespace shex

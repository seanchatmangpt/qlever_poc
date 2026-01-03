//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 1 - EPIC 14.0 Formalism Convergence

#include "engine/formalism/unified/UnifiedFormalismAST.h"

#include <sstream>
#include <algorithm>

namespace formalism::unified {

// =============================================================================
// DATALOG AST NODE IMPLEMENTATIONS
// =============================================================================

std::string DatalogRuleNode::toString() const {
  std::ostringstream oss;

  // Head: predicate(var1, var2, ...)
  oss << headPredicate_ << "(";
  for (size_t i = 0; i < headVariables_.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << headVariables_[i].toSparql();
  }
  oss << ")";

  // Body: :- pattern1, pattern2, ...
  if (!bodyPatterns_.empty() || !filters_.empty()) {
    oss << " :- ";

    // Body patterns
    for (size_t i = 0; i < bodyPatterns_.size(); ++i) {
      if (i > 0) oss << ", ";
      // Note: SparqlTriple should have its own toString method
      oss << "[triple pattern " << i << "]";
    }

    // Filters
    if (!filters_.empty()) {
      if (!bodyPatterns_.empty()) oss << ", ";
      oss << "[" << filters_.size() << " filter(s)]";
    }
  }

  if (isRecursive_) {
    oss << " [recursive]";
  }

  return oss.str();
}

bool DatalogRuleNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::Datalog) {
    return false;
  }
  if (other.getNodeType() != NodeType::Rule) {
    return false;
  }

  const auto* otherRule = dynamic_cast<const DatalogRuleNode*>(&other);
  if (!otherRule) {
    return false;
  }

  return *this == *otherRule;
}

// =============================================================================
// SHACL AST NODE IMPLEMENTATIONS
// =============================================================================

std::string ShaclConstraintNode::toString() const {
  std::ostringstream oss;

  // Constraint type
  switch (type_) {
    case ShaclConstraintType::MinCount:
      oss << "sh:minCount ";
      if (std::holds_alternative<int>(value_)) {
        oss << std::get<int>(value_);
      }
      break;
    case ShaclConstraintType::MaxCount:
      oss << "sh:maxCount ";
      if (std::holds_alternative<int>(value_)) {
        oss << std::get<int>(value_);
      }
      break;
    case ShaclConstraintType::Datatype:
      oss << "sh:datatype ";
      if (std::holds_alternative<std::string>(value_)) {
        oss << std::get<std::string>(value_);
      }
      break;
    case ShaclConstraintType::NodeKind:
      oss << "sh:nodeKind ";
      if (std::holds_alternative<std::string>(value_)) {
        oss << std::get<std::string>(value_);
      }
      break;
    case ShaclConstraintType::Pattern:
      oss << "sh:pattern ";
      if (std::holds_alternative<std::string>(value_)) {
        oss << "\"" << std::get<std::string>(value_) << "\"";
      }
      break;
    case ShaclConstraintType::MinInclusive:
      oss << "sh:minInclusive ";
      if (std::holds_alternative<double>(value_)) {
        oss << std::get<double>(value_);
      }
      break;
    case ShaclConstraintType::MaxInclusive:
      oss << "sh:maxInclusive ";
      if (std::holds_alternative<double>(value_)) {
        oss << std::get<double>(value_);
      }
      break;
    case ShaclConstraintType::MinExclusive:
      oss << "sh:minExclusive ";
      if (std::holds_alternative<double>(value_)) {
        oss << std::get<double>(value_);
      }
      break;
    case ShaclConstraintType::MaxExclusive:
      oss << "sh:maxExclusive ";
      if (std::holds_alternative<double>(value_)) {
        oss << std::get<double>(value_);
      }
      break;
    case ShaclConstraintType::MinLength:
      oss << "sh:minLength ";
      if (std::holds_alternative<int>(value_)) {
        oss << std::get<int>(value_);
      }
      break;
    case ShaclConstraintType::MaxLength:
      oss << "sh:maxLength ";
      if (std::holds_alternative<int>(value_)) {
        oss << std::get<int>(value_);
      }
      break;
    case ShaclConstraintType::In:
      oss << "sh:in [";
      if (std::holds_alternative<std::vector<std::string>>(value_)) {
        const auto& values = std::get<std::vector<std::string>>(value_);
        for (size_t i = 0; i < values.size(); ++i) {
          if (i > 0) oss << ", ";
          oss << values[i];
        }
      }
      oss << "]";
      break;
    case ShaclConstraintType::Node:
      oss << "sh:node ";
      if (std::holds_alternative<std::string>(value_)) {
        oss << std::get<std::string>(value_);
      }
      break;
    case ShaclConstraintType::Shape:
      oss << "sh:shape ";
      if (std::holds_alternative<std::string>(value_)) {
        oss << std::get<std::string>(value_);
      }
      break;
  }

  // Add severity if not default
  if (severity_ != ShaclSeverity::Violation) {
    oss << " [";
    switch (severity_) {
      case ShaclSeverity::Warning: oss << "warning"; break;
      case ShaclSeverity::Info: oss << "info"; break;
      default: break;
    }
    oss << "]";
  }

  // Add message if present
  if (!message_.empty()) {
    oss << " message: \"" << message_ << "\"";
  }

  return oss.str();
}

bool ShaclConstraintNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::SHACL) {
    return false;
  }
  if (other.getNodeType() != NodeType::Constraint) {
    return false;
  }

  const auto* otherConstraint = dynamic_cast<const ShaclConstraintNode*>(&other);
  if (!otherConstraint) {
    return false;
  }

  return *this == *otherConstraint;
}

std::string ShaclPropertyShapeNode::toString() const {
  std::ostringstream oss;
  oss << "PropertyShape[" << propertyPath_ << "]";
  if (required_) {
    oss << " (required)";
  }
  oss << " with " << constraints_.size() << " constraint(s)";
  return oss.str();
}

bool ShaclPropertyShapeNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::SHACL) {
    return false;
  }
  if (other.getNodeType() != NodeType::PropertyShape) {
    return false;
  }

  const auto* otherShape = dynamic_cast<const ShaclPropertyShapeNode*>(&other);
  if (!otherShape) {
    return false;
  }

  // Compare basic fields
  if (propertyPath_ != otherShape->propertyPath_ ||
      required_ != otherShape->required_ ||
      constraints_.size() != otherShape->constraints_.size()) {
    return false;
  }

  // Deep comparison of constraints
  for (size_t i = 0; i < constraints_.size(); ++i) {
    if (!constraints_[i] || !otherShape->constraints_[i]) {
      return constraints_[i] == otherShape->constraints_[i];
    }
    if (!constraints_[i]->equals(*otherShape->constraints_[i])) {
      return false;
    }
  }

  return true;
}

std::string ShaclNodeShapeNode::toString() const {
  std::ostringstream oss;
  oss << "NodeShape[" << shapeId_ << "]";
  if (closed_) {
    oss << " (closed)";
  }
  oss << " targets: " << targetClasses_.size() << " class(es)";
  oss << ", " << propertyShapes_.size() << " property shape(s)";
  oss << ", " << nodeConstraints_.size() << " node constraint(s)";
  return oss.str();
}

bool ShaclNodeShapeNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::SHACL) {
    return false;
  }
  if (other.getNodeType() != NodeType::NodeShape) {
    return false;
  }

  const auto* otherShape = dynamic_cast<const ShaclNodeShapeNode*>(&other);
  if (!otherShape) {
    return false;
  }

  // Compare basic fields
  if (shapeId_ != otherShape->shapeId_ ||
      closed_ != otherShape->closed_ ||
      targetClasses_ != otherShape->targetClasses_ ||
      propertyShapes_.size() != otherShape->propertyShapes_.size() ||
      nodeConstraints_.size() != otherShape->nodeConstraints_.size()) {
    return false;
  }

  // Deep comparison of property shapes
  for (size_t i = 0; i < propertyShapes_.size(); ++i) {
    if (!propertyShapes_[i] || !otherShape->propertyShapes_[i]) {
      return propertyShapes_[i] == otherShape->propertyShapes_[i];
    }
    if (!propertyShapes_[i]->equals(*otherShape->propertyShapes_[i])) {
      return false;
    }
  }

  // Deep comparison of node constraints
  for (size_t i = 0; i < nodeConstraints_.size(); ++i) {
    if (!nodeConstraints_[i] || !otherShape->nodeConstraints_[i]) {
      return nodeConstraints_[i] == otherShape->nodeConstraints_[i];
    }
    if (!nodeConstraints_[i]->equals(*otherShape->nodeConstraints_[i])) {
      return false;
    }
  }

  return true;
}

// =============================================================================
// N3 AST NODE IMPLEMENTATIONS
// =============================================================================

std::vector<Variable> N3TripleNode::getVariables() const {
  std::vector<Variable> vars;

  // Extract variables from each component
  // Note: TripleComponent has a isVariable() method
  // This is a simplified implementation - actual extraction depends on
  // TripleComponent API

  // TODO: Implement proper variable extraction from TripleComponent
  // when TripleComponent API is fully understood

  return vars;
}

std::string N3TripleNode::toString() const {
  std::ostringstream oss;

  // Note: TripleComponent should have a toString or toSparql method
  // This is a placeholder implementation
  oss << "[subject] [predicate] [object]";

  return oss.str();
}

bool N3TripleNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::N3) {
    return false;
  }
  if (other.getNodeType() != NodeType::Triple) {
    return false;
  }

  const auto* otherTriple = dynamic_cast<const N3TripleNode*>(&other);
  if (!otherTriple) {
    return false;
  }

  return *this == *otherTriple;
}

// =============================================================================
// SHEX AST NODE IMPLEMENTATIONS
// =============================================================================

bool ShExSchemaNode::equals(const ASTNode& other) const {
  if (other.getFormalismType() != FormalismType::ShEx) {
    return false;
  }
  if (other.getNodeType() != NodeType::Schema) {
    return false;
  }

  const auto* otherSchema = dynamic_cast<const ShExSchemaNode*>(&other);
  if (!otherSchema) {
    return false;
  }

  return *this == *otherSchema;
}

}  // namespace formalism::unified

//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 1 - EPIC 14.0 Formalism Convergence

#ifndef QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMAST_H
#define QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMAST_H

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "backports/three_way_comparison.h"
#include "parser/SparqlTriple.h"
#include "parser/data/SparqlFilter.h"
#include "rdfTypes/Variable.h"
#include "util/Serializer/SerializeString.h"
#include "util/Serializer/Serializer.h"

namespace formalism::unified {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class ASTNode;
class DatalogRuleNode;
class ShaclConstraintNode;
class N3TripleNode;
class ShExSchemaNode;

// =============================================================================
// CORE TYPES AND ENUMS
// =============================================================================

/// Formalism type discriminator for unified AST nodes
enum class FormalismType {
  Datalog,   // Datalog rules (head :- body)
  SHACL,     // SHACL shapes and constraints
  N3,        // N3 triples and patterns
  ShEx       // ShEx schemas (future extension)
};

/// Node type within each formalism
enum class NodeType {
  // Datalog node types
  Rule,           // Complete Datalog rule (head :- body)
  RuleHead,       // Head of a rule
  RuleBody,       // Body of a rule (patterns + filters)

  // SHACL node types
  NodeShape,      // Node shape with target and constraints
  PropertyShape,  // Property shape with path and constraints
  Constraint,     // Individual constraint (minCount, datatype, etc.)

  // N3 node types
  Triple,         // RDF triple pattern
  TriplePattern,  // Triple pattern with variables

  // ShEx node types (future)
  Schema,         // ShEx schema
  ShapeExpr       // Shape expression
};

/// Variable binding for AST nodes
/// Immutable by design - follows Datalog's pattern
struct VariableBinding {
  Variable variable;
  std::optional<std::string> bindingValue;  // Optional bound value

  VariableBinding() = default;
  explicit VariableBinding(Variable var,
                          std::optional<std::string> value = std::nullopt)
      : variable(std::move(var)), bindingValue(std::move(value)) {}

  [[nodiscard]] bool isBound() const { return bindingValue.has_value(); }

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(VariableBinding, variable,
                                              bindingValue)

  AD_SERIALIZE_FRIEND_FUNCTION(VariableBinding) {
    serializer | arg.variable_;
    serializer | arg.bindingValue;
  }
};

// =============================================================================
// BASE AST NODE (ABSTRACT)
// =============================================================================

/// Base class for all unified formalism AST nodes
/// Immutable by design - all fields are const-accessible
/// Follows Datalog's architectural pattern identified as best-in-class
class ASTNode {
 public:
  virtual ~ASTNode() = default;

  /// Get the formalism type this node belongs to
  [[nodiscard]] virtual FormalismType getFormalismType() const = 0;

  /// Get the specific node type
  [[nodiscard]] virtual NodeType getNodeType() const = 0;

  /// Get all variables referenced in this node
  [[nodiscard]] virtual std::vector<Variable> getVariables() const = 0;

  /// Convert node to human-readable string representation
  [[nodiscard]] virtual std::string toString() const = 0;

  /// Deep equality comparison (must be implemented by derived classes)
  [[nodiscard]] virtual bool equals(const ASTNode& other) const = 0;

  /// Serialization support (must be implemented by derived classes)
  virtual void serialize(auto& serializer) const = 0;

 protected:
  // Protected constructor - this is an abstract base class
  ASTNode() = default;

  // Disable copy/move - use shared_ptr for node ownership
  ASTNode(const ASTNode&) = delete;
  ASTNode& operator=(const ASTNode&) = delete;
  ASTNode(ASTNode&&) = delete;
  ASTNode& operator=(ASTNode&&) = delete;
};

// =============================================================================
// DATALOG AST NODES
// =============================================================================

/// Datalog rule node: head(Vars) :- body_pattern1, body_pattern2, filters
/// Immutable by design - all accessors return const references
/// Direct reuse of existing DatalogRule structure with unified interface
class DatalogRuleNode : public ASTNode {
 public:
  /// Construct a Datalog rule node
  DatalogRuleNode(std::string headPredicate,
                  std::vector<Variable> headVariables,
                  std::vector<SparqlTriple> bodyPatterns,
                  std::vector<SparqlFilter> filters = {},
                  bool isRecursive = false)
      : headPredicate_(std::move(headPredicate)),
        headVariables_(std::move(headVariables)),
        bodyPatterns_(std::move(bodyPatterns)),
        filters_(std::move(filters)),
        isRecursive_(isRecursive) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::Datalog;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::Rule;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override {
    return headVariables_;
  }

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | headPredicate_;
    serializer | headVariables_;
    serializer | bodyPatterns_;
    serializer | filters_;
    serializer | isRecursive_;
  }

  // Datalog-specific accessors
  [[nodiscard]] const std::string& getHeadPredicate() const {
    return headPredicate_;
  }

  [[nodiscard]] const std::vector<Variable>& getHeadVariables() const {
    return headVariables_;
  }

  [[nodiscard]] const std::vector<SparqlTriple>& getBodyPatterns() const {
    return bodyPatterns_;
  }

  [[nodiscard]] const std::vector<SparqlFilter>& getFilters() const {
    return filters_;
  }

  [[nodiscard]] size_t getArity() const { return headVariables_.size(); }

  [[nodiscard]] bool isRecursive() const { return isRecursive_; }

  // Equality comparison
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(DatalogRuleNode, headPredicate_,
                                              headVariables_, bodyPatterns_,
                                              filters_, isRecursive_)

  // Serialization support
  AD_SERIALIZE_FRIEND_FUNCTION(DatalogRuleNode) {
    serializer | arg.headPredicate_;
    serializer | arg.headVariables_;
    serializer | arg.bodyPatterns_;
    serializer | arg.filters_;
    serializer | arg.isRecursive_;
  }

 private:
  const std::string headPredicate_;
  const std::vector<Variable> headVariables_;
  const std::vector<SparqlTriple> bodyPatterns_;
  const std::vector<SparqlFilter> filters_;
  const bool isRecursive_;
};

// =============================================================================
// SHACL AST NODES
// =============================================================================

/// SHACL constraint value (unified representation)
/// Immutable variant - no post-construction modification
using ShaclConstraintValue = std::variant<
    int,                      // MinCount, MaxCount, MinLength, MaxLength
    double,                   // MinExclusive, MaxExclusive, MinInclusive, MaxInclusive
    std::string,              // Datatype, Pattern, HasValue, NodeKind
    std::vector<std::string>  // In (allowed values list)
>;

/// SHACL constraint types (80/20 coverage)
enum class ShaclConstraintType {
  // Cardinality
  MinCount,
  MaxCount,

  // Value type
  Datatype,
  NodeKind,

  // Value range
  MinInclusive,
  MaxInclusive,
  MinExclusive,
  MaxExclusive,
  MinLength,
  MaxLength,

  // Pattern matching
  Pattern,
  In,

  // Recursive
  Node,
  Shape
};

/// Severity level for SHACL constraints
enum class ShaclSeverity {
  Violation,
  Warning,
  Info
};

/// Individual SHACL constraint node
/// Immutable by design - all fields const after construction
class ShaclConstraintNode : public ASTNode {
 public:
  ShaclConstraintNode(ShaclConstraintType type,
                     ShaclConstraintValue value,
                     ShaclSeverity severity = ShaclSeverity::Violation,
                     std::string message = "")
      : type_(type),
        value_(std::move(value)),
        severity_(severity),
        message_(std::move(message)) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::SHACL;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::Constraint;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override {
    return {};  // SHACL constraints don't contain SPARQL variables directly
  }

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | type_;
    serializer | value_;
    serializer | severity_;
    serializer | message_;
  }

  // SHACL-specific accessors
  [[nodiscard]] ShaclConstraintType getType() const { return type_; }

  [[nodiscard]] const ShaclConstraintValue& getValue() const { return value_; }

  [[nodiscard]] ShaclSeverity getSeverity() const { return severity_; }

  [[nodiscard]] const std::string& getMessage() const { return message_; }

  // Equality comparison
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(ShaclConstraintNode, type_,
                                              value_, severity_, message_)

  // Serialization support
  AD_SERIALIZE_FRIEND_FUNCTION(ShaclConstraintNode) {
    serializer | arg.type_;
    serializer | arg.value_;
    serializer | arg.severity_;
    serializer | arg.message_;
  }

 private:
  const ShaclConstraintType type_;
  const ShaclConstraintValue value_;
  const ShaclSeverity severity_;
  const std::string message_;
};

/// SHACL property shape node
/// Immutable collection of constraints on a property path
class ShaclPropertyShapeNode : public ASTNode {
 public:
  ShaclPropertyShapeNode(std::string propertyPath,
                        std::vector<std::shared_ptr<ShaclConstraintNode>> constraints,
                        bool required = false)
      : propertyPath_(std::move(propertyPath)),
        constraints_(std::move(constraints)),
        required_(required) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::SHACL;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::PropertyShape;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override {
    return {};  // Property shapes reference IRIs, not variables
  }

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | propertyPath_;
    serializer | constraints_;
    serializer | required_;
  }

  // SHACL-specific accessors
  [[nodiscard]] const std::string& getPropertyPath() const {
    return propertyPath_;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<ShaclConstraintNode>>&
      getConstraints() const {
    return constraints_;
  }

  [[nodiscard]] bool isRequired() const { return required_; }

  // Note: Equality and serialization require special handling for shared_ptr

 private:
  const std::string propertyPath_;
  const std::vector<std::shared_ptr<ShaclConstraintNode>> constraints_;
  const bool required_;
};

/// SHACL node shape
/// Immutable collection of property shapes and node-level constraints
class ShaclNodeShapeNode : public ASTNode {
 public:
  ShaclNodeShapeNode(std::string shapeId,
                    std::vector<std::string> targetClasses,
                    std::vector<std::shared_ptr<ShaclPropertyShapeNode>> propertyShapes,
                    std::vector<std::shared_ptr<ShaclConstraintNode>> nodeConstraints = {},
                    bool closed = false)
      : shapeId_(std::move(shapeId)),
        targetClasses_(std::move(targetClasses)),
        propertyShapes_(std::move(propertyShapes)),
        nodeConstraints_(std::move(nodeConstraints)),
        closed_(closed) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::SHACL;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::NodeShape;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override {
    return {};  // Node shapes reference classes and nodes by IRI
  }

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | shapeId_;
    serializer | targetClasses_;
    serializer | propertyShapes_;
    serializer | nodeConstraints_;
    serializer | closed_;
  }

  // SHACL-specific accessors
  [[nodiscard]] const std::string& getShapeId() const { return shapeId_; }

  [[nodiscard]] const std::vector<std::string>& getTargetClasses() const {
    return targetClasses_;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<ShaclPropertyShapeNode>>&
      getPropertyShapes() const {
    return propertyShapes_;
  }

  [[nodiscard]] const std::vector<std::shared_ptr<ShaclConstraintNode>>&
      getNodeConstraints() const {
    return nodeConstraints_;
  }

  [[nodiscard]] bool isClosed() const { return closed_; }

 private:
  const std::string shapeId_;
  const std::vector<std::string> targetClasses_;
  const std::vector<std::shared_ptr<ShaclPropertyShapeNode>> propertyShapes_;
  const std::vector<std::shared_ptr<ShaclConstraintNode>> nodeConstraints_;
  const bool closed_;
};

// =============================================================================
// N3 AST NODES
// =============================================================================

/// N3 triple pattern node
/// Immutable RDF triple with optional variable bindings
class N3TripleNode : public ASTNode {
 public:
  N3TripleNode(TripleComponent subject,
              TripleComponent predicate,
              TripleComponent object)
      : subject_(std::move(subject)),
        predicate_(std::move(predicate)),
        object_(std::move(object)) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::N3;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::Triple;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override;

  [[nodiscard]] std::string toString() const override;

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | subject_;
    serializer | predicate_;
    serializer | object_;
  }

  // N3-specific accessors
  [[nodiscard]] const TripleComponent& getSubject() const { return subject_; }

  [[nodiscard]] const TripleComponent& getPredicate() const {
    return predicate_;
  }

  [[nodiscard]] const TripleComponent& getObject() const { return object_; }

  // Equality comparison
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(N3TripleNode, subject_,
                                              predicate_, object_)

  // Serialization support
  AD_SERIALIZE_FRIEND_FUNCTION(N3TripleNode) {
    serializer | arg.subject_;
    serializer | arg.predicate_;
    serializer | arg.object_;
  }

 private:
  const TripleComponent subject_;
  const TripleComponent predicate_;
  const TripleComponent object_;
};

// =============================================================================
// SHEX AST NODES (FUTURE EXTENSION)
// =============================================================================

/// ShEx schema node (stub for future implementation)
/// Placeholder following the immutable design pattern
class ShExSchemaNode : public ASTNode {
 public:
  explicit ShExSchemaNode(std::string schemaId)
      : schemaId_(std::move(schemaId)) {}

  // ASTNode interface implementation
  [[nodiscard]] FormalismType getFormalismType() const override {
    return FormalismType::ShEx;
  }

  [[nodiscard]] NodeType getNodeType() const override {
    return NodeType::Schema;
  }

  [[nodiscard]] std::vector<Variable> getVariables() const override {
    return {};
  }

  [[nodiscard]] std::string toString() const override {
    return "ShExSchema[" + schemaId_ + "] (stub)";
  }

  [[nodiscard]] bool equals(const ASTNode& other) const override;

  void serialize(auto& serializer) const override {
    serializer | schemaId_;
  }

  [[nodiscard]] const std::string& getSchemaId() const { return schemaId_; }

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(ShExSchemaNode, schemaId_)

  AD_SERIALIZE_FRIEND_FUNCTION(ShExSchemaNode) {
    serializer | arg.schemaId_;
  }

 private:
  const std::string schemaId_;
};

// =============================================================================
// AST UTILITIES
// =============================================================================

/// Type-safe smart pointer for AST nodes
template<typename T>
using ASTNodePtr = std::shared_ptr<const T>;

/// Create an immutable AST node
template<typename T, typename... Args>
[[nodiscard]] ASTNodePtr<T> makeASTNode(Args&&... args) {
  return std::make_shared<const T>(std::forward<Args>(args)...);
}

}  // namespace formalism::unified

#endif  // QLEVER_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMAST_H

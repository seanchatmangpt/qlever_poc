#ifndef PARSER_SHEX_TRIPLE_EXPRESSION_H
#define PARSER_SHEX_TRIPLE_EXPRESSION_H

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace shex {

// Forward declarations
class TripleExpression;
struct NodeConstraint;
struct TripleContext;

// ============================================================================
// Cardinality Types for Triple Expressions
// ============================================================================

/**
 * @brief Represents cardinality constraints for triple expressions
 *
 * min/max semantics follow W3C ShEx specification:
 * - {m,n}: min=m, max=n (specific range)
 * - {m,*}: min=m, max=nullopt (unbounded)
 * - *: min=0, max=nullopt (zero or more)
 * - +: min=1, max=nullopt (one or more)
 * - ?: min=0, max=1 (zero or one)
 * - (no annotation): min=1, max=1 (exactly one)
 */
struct CardinalityConstraint {
  size_t min = 1;
  std::optional<size_t> max = 1;

  CardinalityConstraint() = default;
  CardinalityConstraint(size_t m, std::optional<size_t> mx)
      : min(m), max(mx) {}

  static CardinalityConstraint exactlyOne() { return {1, 1}; }
  static CardinalityConstraint zeroOrOne() { return {0, 1}; }
  static CardinalityConstraint zeroOrMore() { return {0, std::nullopt}; }
  static CardinalityConstraint oneOrMore() { return {1, std::nullopt}; }

  /**
   * Check if count satisfies cardinality constraint
   * @param count Number of occurrences
   * @return true if count is within [min, max]
   */
  bool satisfies(size_t count) const {
    if (count < min) return false;
    if (max.has_value() && count > max.value()) return false;
    return true;
  }

  std::string toString() const {
    if (min == 1 && max.has_value() && max.value() == 1) return "";
    if (min == 0 && max.has_value() && max.value() == 1) return "?";
    if (min == 0 && !max.has_value()) return "*";
    if (min == 1 && !max.has_value()) return "+";
    if (max.has_value())
      return "{" + std::to_string(min) + "," + std::to_string(max.value()) +
             "}";
    return "{" + std::to_string(min) + ",*}";
  }
};

// ============================================================================
// Node Constraint (value-level constraints)
// ============================================================================

enum class NodeKind { IRI, LITERAL, BNODE, NONLITERAL };

/**
 * @brief Constraints on RDF node values (IRIs, literals, blank nodes)
 *
 * Supports:
 * - Node kind restrictions (IRI, Literal, BNode)
 * - Datatype constraints (xsd:string, xsd:integer, etc.)
 * - Value set constraints (enumeration)
 * - Numeric facets (minInclusive, maxInclusive, etc.)
 * - String facets (length, pattern, etc.)
 */
struct NodeConstraint {
  std::optional<NodeKind> nodeKind;
  std::optional<std::string> datatype;
  absl::flat_hash_set<std::string> values;  // Explicit value enumeration

  // Numeric facets
  std::optional<int> minInclusive;
  std::optional<int> maxInclusive;
  std::optional<int> minExclusive;
  std::optional<int> maxExclusive;

  // String facets
  std::optional<size_t> length;
  std::optional<size_t> minLength;
  std::optional<size_t> maxLength;
  std::optional<std::string> pattern;  // Regex pattern

  NodeConstraint() = default;

  /**
   * Validate a node value against this constraint
   * @param value The RDF node value (IRI, literal, or blank node)
   * @param kind The kind of RDF node
   * @return true if value satisfies all constraints
   */
  bool validate(const std::string& value, NodeKind kind) const;

  bool hasConstraints() const {
    return nodeKind.has_value() || datatype.has_value() || !values.empty() ||
           minInclusive.has_value() || maxInclusive.has_value() ||
           minExclusive.has_value() || maxExclusive.has_value() ||
           length.has_value() || minLength.has_value() ||
           maxLength.has_value() || pattern.has_value();
  }
};

// ============================================================================
// Triple Context (validation state and matched triples)
// ============================================================================

/**
 * @brief Encapsulates validation context for triple expressions
 *
 * Tracks:
 * - Subject being validated
 * - Available triples (property -> values)
 * - Matched triples (consumed by validation)
 * - Inverse mode (for InverseProperty)
 */
struct TripleContext {
  std::string subject;  // The focus node being validated

  // Available triples: predicate -> [(object, kind)]
  absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
      triples;

  // Triples matched (consumed) during validation
  absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
      matchedTriples;

  // If true, validation is in inverse mode (subject/object swapped)
  bool inverseMode = false;

  TripleContext() = default;
  TripleContext(const std::string& subj) : subject(subj) {}

  /**
   * Get remaining (unmatched) triples for a predicate
   * @param predicate The property to query
   * @return Vector of (value, kind) pairs not yet matched
   */
  std::vector<std::pair<std::string, NodeKind>> getRemainingTriples(
      const std::string& predicate) const;

  /**
   * Mark a triple as matched (consumed)
   * @param predicate The property
   * @param value The object value
   * @param kind The node kind
   */
  void markMatched(const std::string& predicate, const std::string& value,
                   NodeKind kind);

  /**
   * Get all unmatched triples across all predicates
   * @return Map of predicate -> [(value, kind)]
   */
  absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
  getAllRemainingTriples() const;

  /**
   * Create a copy of this context for nested validation
   * @return New context with same triples but empty matched set
   */
  TripleContext copy() const;
};

// ============================================================================
// Validation Result
// ============================================================================

struct ValidationResult {
  bool isValid = false;
  std::vector<std::string> errors;
  size_t matchedCount = 0;  // Number of triples matched by this expression

  ValidationResult() = default;
  ValidationResult(bool valid) : isValid(valid) {}

  void addError(const std::string& error) {
    errors.push_back(error);
    isValid = false;
  }

  static ValidationResult success(size_t matched = 0) {
    ValidationResult r;
    r.isValid = true;
    r.matchedCount = matched;
    return r;
  }

  static ValidationResult failure(const std::string& error) {
    ValidationResult r;
    r.addError(error);
    return r;
  }
};

// ============================================================================
// Triple Expression Base Class (Polymorphic)
// ============================================================================

/**
 * @brief Abstract base class for all triple expressions
 *
 * W3C ShEx defines four types of triple expressions:
 * 1. TripleConstraint - single property with cardinality
 * 2. EachOf - conjunction (all must match)
 * 3. OneOf - disjunction (exactly one must match)
 * 4. InverseProperty - subject/object swapping
 *
 * All triple expressions support:
 * - Cardinality constraints (min, max)
 * - Virtual properties (not matched against data)
 * - Recursive nesting
 */
class TripleExpression {
 public:
  virtual ~TripleExpression() = default;

  /**
   * Validate this triple expression against a context
   * @param context The validation context (triples, subject, etc.)
   * @return Validation result with errors and matched count
   */
  virtual ValidationResult validate(TripleContext& context) const = 0;

  /**
   * Get the cardinality constraint for this expression
   * @return Cardinality constraint
   */
  virtual CardinalityConstraint getCardinality() const {
    return CardinalityConstraint::exactlyOne();
  }

  /**
   * Check if this is a virtual property (not matched against data)
   * @return true if virtual
   */
  virtual bool isVirtual() const { return false; }

  /**
   * Get a human-readable representation of this expression
   * @return String representation
   */
  virtual std::string toString() const = 0;

  /**
   * Clone this expression (deep copy)
   * @return Unique pointer to cloned expression
   */
  virtual std::unique_ptr<TripleExpression> clone() const = 0;
};

// ============================================================================
// Triple Constraint (leaf expression)
// ============================================================================

/**
 * @brief Represents a single property constraint with cardinality
 *
 * Example: foaf:name xsd:string {1,3}
 * - predicate: foaf:name
 * - valueConstraint: NodeConstraint with datatype xsd:string
 * - cardinality: min=1, max=3
 *
 * Validation algorithm:
 * 1. Get triples with matching predicate from context
 * 2. Validate each value against NodeConstraint
 * 3. Check count against cardinality
 * 4. Mark matched triples in context
 */
class TripleConstraint : public TripleExpression {
 public:
  std::string predicate;
  NodeConstraint valueConstraint;
  CardinalityConstraint cardinality = CardinalityConstraint::exactlyOne();
  bool inverse = false;    // If true, match (object, predicate, subject)
  bool virtual_ = false;   // If true, don't match against data

  TripleConstraint(const std::string& pred) : predicate(pred) {}

  ValidationResult validate(TripleContext& context) const override;

  CardinalityConstraint getCardinality() const override { return cardinality; }

  bool isVirtual() const override { return virtual_; }

  std::string toString() const override {
    std::string s = inverse ? "^" : "";
    s += predicate;
    if (valueConstraint.hasConstraints()) {
      s += " [constraints]";
    }
    s += cardinality.toString();
    if (virtual_) s += " // VIRTUAL";
    return s;
  }

  std::unique_ptr<TripleExpression> clone() const override {
    auto c = std::make_unique<TripleConstraint>(predicate);
    c->valueConstraint = valueConstraint;
    c->cardinality = cardinality;
    c->inverse = inverse;
    c->virtual_ = virtual_;
    return c;
  }
};

// ============================================================================
// EachOf (conjunction - all must match)
// ============================================================================

/**
 * @brief Represents conjunction of triple expressions (all must match)
 *
 * Example: ( foaf:name . ; foaf:email . )
 * Both name and email must be present.
 *
 * Validation algorithm (triple partitioning):
 * 1. Create a partition of available triples for each sub-expression
 * 2. Validate each sub-expression against its partition
 * 3. All sub-expressions must succeed
 * 4. Aggregate matched triples from all sub-expressions
 *
 * Cardinality propagation:
 * - Leaf cardinality: applied to individual properties
 * - Group cardinality: applied to the entire EachOf group
 * - Example: ( foaf:name . ; foaf:email . ){2,3}
 *   The entire group (name+email pair) must appear 2-3 times
 */
class EachOf : public TripleExpression {
 public:
  std::vector<std::unique_ptr<TripleExpression>> expressions;
  CardinalityConstraint cardinality = CardinalityConstraint::exactlyOne();

  EachOf() = default;

  void addExpression(std::unique_ptr<TripleExpression> expr) {
    expressions.push_back(std::move(expr));
  }

  ValidationResult validate(TripleContext& context) const override;

  CardinalityConstraint getCardinality() const override { return cardinality; }

  std::string toString() const override {
    std::string s = "EachOf(";
    for (size_t i = 0; i < expressions.size(); ++i) {
      if (i > 0) s += " ; ";
      s += expressions[i]->toString();
    }
    s += ")" + cardinality.toString();
    return s;
  }

  std::unique_ptr<TripleExpression> clone() const override {
    auto c = std::make_unique<EachOf>();
    for (const auto& expr : expressions) {
      c->addExpression(expr->clone());
    }
    c->cardinality = cardinality;
    return c;
  }

 private:
  /**
   * Validate a single iteration of the EachOf group
   * @param context Validation context
   * @return Result with matched triples for this iteration
   */
  ValidationResult validateSingleIteration(TripleContext& context) const;
};

// ============================================================================
// OneOf (disjunction - exactly one must match)
// ============================================================================

/**
 * @brief Represents disjunction of triple expressions (exactly one must match)
 *
 * Example: ( foaf:name . | foaf:nick . )
 * Either name or nickname must be present (but not both).
 *
 * Validation algorithm (greedy choice selection):
 * 1. Try each sub-expression in order
 * 2. Select the first that successfully validates
 * 3. Consume triples matched by selected expression
 * 4. Fail if no expression matches or multiple match
 *
 * Tie-breaking rule (W3C ShEx):
 * - Select expression that matches the most triples
 * - If tie, select first in declaration order
 *
 * Cardinality propagation:
 * - Applied to the selected expression's matched triples
 * - Example: ( foaf:name . | foaf:nick . ){1,2}
 *   The selected alternative must appear 1-2 times
 */
class OneOf : public TripleExpression {
 public:
  std::vector<std::unique_ptr<TripleExpression>> expressions;
  CardinalityConstraint cardinality = CardinalityConstraint::exactlyOne();

  OneOf() = default;

  void addExpression(std::unique_ptr<TripleExpression> expr) {
    expressions.push_back(std::move(expr));
  }

  ValidationResult validate(TripleContext& context) const override;

  CardinalityConstraint getCardinality() const override { return cardinality; }

  std::string toString() const override {
    std::string s = "OneOf(";
    for (size_t i = 0; i < expressions.size(); ++i) {
      if (i > 0) s += " | ";
      s += expressions[i]->toString();
    }
    s += ")" + cardinality.toString();
    return s;
  }

  std::unique_ptr<TripleExpression> clone() const override {
    auto c = std::make_unique<OneOf>();
    for (const auto& expr : expressions) {
      c->addExpression(expr->clone());
    }
    c->cardinality = cardinality;
    return c;
  }

 private:
  /**
   * Select best matching alternative using greedy strategy
   * @param context Validation context
   * @return Index of best alternative, or nullopt if none match
   */
  std::optional<size_t> selectBestAlternative(TripleContext& context) const;
};

// ============================================================================
// Inverse Property (subject/object swapping)
// ============================================================================

/**
 * @brief Represents inverse property constraint
 *
 * Example: ^foaf:knows @PersonShape
 * Matches triples where the focus node appears as object:
 * (?x, foaf:knows, focusNode)
 *
 * Validation algorithm:
 * 1. Transform context to swap subject/object semantics
 * 2. Validate wrapped expression with transformed context
 * 3. Transform results back to original context
 *
 * Note: InverseProperty is a wrapper that modifies interpretation
 * of the wrapped expression, not a standalone constraint.
 */
class InverseProperty : public TripleExpression {
 public:
  std::unique_ptr<TripleExpression> expression;

  InverseProperty(std::unique_ptr<TripleExpression> expr)
      : expression(std::move(expr)) {}

  ValidationResult validate(TripleContext& context) const override;

  CardinalityConstraint getCardinality() const override {
    return expression->getCardinality();
  }

  bool isVirtual() const override { return expression->isVirtual(); }

  std::string toString() const override {
    return "^(" + expression->toString() + ")";
  }

  std::unique_ptr<TripleExpression> clone() const override {
    return std::make_unique<InverseProperty>(expression->clone());
  }
};

}  // namespace shex

#endif  // PARSER_SHEX_TRIPLE_EXPRESSION_H

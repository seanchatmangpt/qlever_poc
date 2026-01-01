//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#ifndef PARSER_DATALOG_RULE_H
#define PARSER_DATALOG_RULE_H

#include <string>
#include <vector>

#include "backports/three_way_comparison.h"
#include "parser/SparqlTriple.h"
#include "parser/data/SparqlFilter.h"
#include "rdfTypes/Variable.h"
#include "util/Serializer/SerializeString.h"
#include "util/Serializer/Serializer.h"

/// Represents a single Datalog rule of the form:
/// head(Vars) :- body_pattern1, body_pattern2, ..., constraints
///
/// Example:
///   ancestor(?x, ?y) :- parent(?x, ?y).
///   ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
///
/// This class stores:
/// - The rule name (head predicate)
/// - Head variables that appear in the rule head
/// - Body patterns (represented as SPARQL triples)
/// - Optional constraints/filters that must be satisfied
/// - Metadata like arity and recursiveness flag
class DatalogRule {
 public:
  /// Default constructor creates an empty rule
  DatalogRule() = default;

  /// Construct a Datalog rule with all components
  /// @param headPredicate The name of the rule (head predicate name)
  /// @param headVariables Variables that appear in the head
  /// @param bodyPatterns Triple patterns in the rule body
  /// @param filters Optional constraints/filters
  /// @param isRecursive Whether this rule is recursive (references itself)
  DatalogRule(std::string headPredicate, std::vector<Variable> headVariables,
              std::vector<SparqlTriple> bodyPatterns,
              std::vector<SparqlFilter> filters = {}, bool isRecursive = false)
      : headPredicate_(std::move(headPredicate)),
        headVariables_(std::move(headVariables)),
        bodyPatterns_(std::move(bodyPatterns)),
        filters_(std::move(filters)),
        isRecursive_(isRecursive) {}

  /// Get the head predicate name
  [[nodiscard]] const std::string& getHeadPredicate() const {
    return headPredicate_;
  }

  /// Get the head variables
  [[nodiscard]] const std::vector<Variable>& getHeadVariables() const {
    return headVariables_;
  }

  /// Get the body patterns
  [[nodiscard]] const std::vector<SparqlTriple>& getBodyPatterns() const {
    return bodyPatterns_;
  }

  /// Get the filters/constraints
  [[nodiscard]] const std::vector<SparqlFilter>& getFilters() const {
    return filters_;
  }

  /// Get the arity (number of head variables)
  [[nodiscard]] size_t getArity() const { return headVariables_.size(); }

  /// Check if this rule is recursive
  [[nodiscard]] bool isRecursive() const { return isRecursive_; }

  /// Set the recursive flag
  void setRecursive(bool recursive) { isRecursive_ = recursive; }

  /// Convert the rule to a human-readable string representation
  /// Format: headPredicate(var1, var2, ...) :- pattern1, pattern2, ...
  [[nodiscard]] std::string toString() const;

  /// Equality comparison
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(DatalogRule, headPredicate_,
                                              headVariables_, bodyPatterns_,
                                              filters_, isRecursive_)

  /// Serialization support
  AD_SERIALIZE_FRIEND_FUNCTION(DatalogRule) {
    serializer | arg.headPredicate_;
    serializer | arg.headVariables_;
    serializer | arg.bodyPatterns_;
    serializer | arg.filters_;
    serializer | arg.isRecursive_;
  }

 private:
  /// The name of the rule (predicate name in the head)
  std::string headPredicate_;

  /// Variables appearing in the head of the rule
  std::vector<Variable> headVariables_;

  /// Triple patterns forming the body of the rule
  std::vector<SparqlTriple> bodyPatterns_;

  /// Optional filters/constraints that must be satisfied
  std::vector<SparqlFilter> filters_;

  /// Flag indicating whether this rule references itself (recursive rule)
  bool isRecursive_ = false;
};

#endif  // PARSER_DATALOG_RULE_H

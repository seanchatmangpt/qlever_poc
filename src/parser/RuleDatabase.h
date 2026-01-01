//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team)

#ifndef PARSER_RULE_DATABASE_H
#define PARSER_RULE_DATABASE_H

#include <map>
#include <string>
#include <vector>

#include "parser/DatalogRule.h"
#include "util/Synchronized.h"

/// Thread-safe database for storing and retrieving Datalog rules.
/// Rules are indexed by predicate name for efficient O(1) lookup.
///
/// Example usage:
///   RuleDatabase db;
///   db.addRule(DatalogRule("ancestor", {...}, {...}));
///   auto rules = db.getRulesByPredicate("ancestor");
///
/// This class is thread-safe and can be safely used from multiple threads.
/// All public methods acquire appropriate locks internally.
class RuleDatabase {
 public:
  /// Default constructor
  RuleDatabase() = default;

  /// Add a rule to the database
  /// @param rule The Datalog rule to add
  /// Rules are indexed by their head predicate name for fast retrieval
  void addRule(DatalogRule rule);

  /// Retrieve all rules for a specific predicate
  /// @param predicateName The predicate name to look up
  /// @return Vector of all rules with matching head predicate (may be empty)
  [[nodiscard]] std::vector<DatalogRule> getRulesByPredicate(
      const std::string& predicateName) const;

  /// Get all rules in the database
  /// @return Vector of all rules (flattened from all predicates)
  [[nodiscard]] std::vector<DatalogRule> getAllRules() const;

  /// Check if any rules exist for a given predicate
  /// @param predicateName The predicate name to check
  /// @return true if at least one rule exists, false otherwise
  [[nodiscard]] bool hasRuleFor(const std::string& predicateName) const;

  /// Get the total number of rules in the database
  /// @return Total count of all rules across all predicates
  [[nodiscard]] size_t getRuleCount() const;

  /// Get the number of distinct predicates that have rules
  /// @return Number of unique predicate names
  [[nodiscard]] size_t getPredicateCount() const;

  /// Clear all rules from the database
  void clear();

  /// Get all predicate names that have rules defined
  /// @return Vector of all predicate names with at least one rule
  [[nodiscard]] std::vector<std::string> getPredicateNames() const;

 private:
  /// Storage: maps predicate name -> vector of rules for that predicate
  /// Wrapped in Synchronized for thread-safe access
  ad_utility::Synchronized<std::map<std::string, std::vector<DatalogRule>>>
      rules_;
};

#endif  // PARSER_RULE_DATABASE_H

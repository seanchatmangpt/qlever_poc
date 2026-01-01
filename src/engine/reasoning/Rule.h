// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_RULE_H
#define QLEVER_SRC_ENGINE_REASONING_RULE_H

#include <memory>
#include <string>
#include <vector>

#include "engine/idTable/IdTable.h"
#include "parser/SparqlTriple.h"
#include "rdfTypes/RdfTypes.h"

namespace reasoning {

/// Represents a single Datalog/N3 rule.
/// Maps to SPARQL: INSERT { head } WHERE { body }
/// Or in N3: body => head
class Rule {
 public:
  /// Create a rule from body and head SPARQL triple patterns.
  /// body: list of triple patterns in the WHERE clause
  /// head: list of triple patterns in the CONSTRUCT/INSERT clause
  /// ruleId: unique identifier for this rule
  explicit Rule(std::vector<SparqlTriple> body,
                std::vector<SparqlTriple> head, size_t ruleId)
      : body_(std::move(body)), head_(std::move(head)), ruleId_(ruleId) {}

  Rule(const Rule&) = default;
  Rule& operator=(const Rule&) = default;
  Rule(Rule&&) = default;
  Rule& operator=(Rule&&) = default;

  [[nodiscard]] const std::vector<SparqlTriple>& getBody() const {
    return body_;
  }

  [[nodiscard]] const std::vector<SparqlTriple>& getHead() const {
    return head_;
  }

  [[nodiscard]] size_t getRuleId() const { return ruleId_; }

  [[nodiscard]] bool isRecursive() const {
    // A rule is recursive if any predicate in the head also appears in the body.
    std::set<LiteralOrIri> headPredicates;
    for (const auto& triple : head_) {
      if (triple.p_.isIri()) {
        headPredicates.insert(triple.p_);
      }
    }

    for (const auto& triple : body_) {
      if (triple.p_.isIri() && headPredicates.contains(triple.p_)) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::string toString() const {
    std::string result = "Rule " + std::to_string(ruleId_) + ":\n";
    result += "  Body:\n";
    for (const auto& triple : body_) {
      result += "    " + triple.asString() + "\n";
    }
    result += "  Head:\n";
    for (const auto& triple : head_) {
      result += "    " + triple.asString() + "\n";
    }
    return result;
  }

 private:
  std::vector<SparqlTriple> body_;
  std::vector<SparqlTriple> head_;
  size_t ruleId_;
};

/// Manages a collection of Datalog/N3 rules.
/// Responsible for storing and querying rules for reasoning operations.
class RuleDatabase {
 public:
  RuleDatabase() = default;

  /// Add a rule to the database.
  void addRule(std::shared_ptr<Rule> rule) {
    rules_.push_back(std::move(rule));
    ruleCount_++;
  }

  /// Get all rules.
  [[nodiscard]] const std::vector<std::shared_ptr<Rule>>& getRules() const {
    return rules_;
  }

  /// Get rules that derive facts with the given predicate.
  /// Returns rules where the predicate appears in the head.
  [[nodiscard]] std::vector<std::shared_ptr<Rule>> getRulesForPredicate(
      const LiteralOrIri& predicate) const {
    std::vector<std::shared_ptr<Rule>> result;
    for (const auto& rule : rules_) {
      for (const auto& triple : rule->getHead()) {
        if (triple.p_.isIri() && triple.p_ == predicate) {
          result.push_back(rule);
          break;
        }
      }
    }
    return result;
  }

  /// Get recursive rules (rules whose head predicate appears in their body).
  [[nodiscard]] std::vector<std::shared_ptr<Rule>> getRecursiveRules() const {
    std::vector<std::shared_ptr<Rule>> result;
    for (const auto& rule : rules_) {
      if (rule->isRecursive()) {
        result.push_back(rule);
      }
    }
    return result;
  }

  /// Clear all rules.
  void clear() {
    rules_.clear();
    ruleCount_ = 0;
  }

  [[nodiscard]] size_t size() const { return rules_.size(); }

  [[nodiscard]] size_t getTotalRuleCount() const { return ruleCount_; }

 private:
  std::vector<std::shared_ptr<Rule>> rules_;
  size_t ruleCount_ = 0;
};

}  // namespace reasoning

#endif  // QLEVER_SRC_ENGINE_REASONING_RULE_H

// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include "engine/reasoning/QueryReasoningHelper.h"

#include <regex>

#include "engine/reasoning/ReasoningOperation.h"
#include "util/Log.h"

namespace reasoning {

bool QueryReasoningHelper::isReasoningQuery(const ParsedQuery& parsedQuery) {
  return containsN3Rules(parsedQuery._originalString);
}

bool QueryReasoningHelper::containsN3Rules(const std::string& sparqlQuery) {
  return N3Parser::isN3Rule(sparqlQuery);
}

std::shared_ptr<RuleDatabase>
QueryReasoningHelper::extractRulesFromQuery(const ParsedQuery& parsedQuery) {
  return extractRulesFromSparql(parsedQuery._originalString);
}

std::shared_ptr<RuleDatabase>
QueryReasoningHelper::extractRulesFromSparql(const std::string& sparqlQuery) {
  auto ruleDatabase = std::make_shared<RuleDatabase>();

  N3Parser parser;
  auto rules = parser.parseRulesFromSparql(sparqlQuery);

  for (auto& rule : rules) {
    ruleDatabase->addRule(rule);
  }

  LOG(INFO) << "Extracted " << ruleDatabase->size() << " rules from query";

  return ruleDatabase;
}

std::optional<std::shared_ptr<QueryExecutionTree>>
QueryReasoningHelper::createReasoningExecutionTree(
    std::shared_ptr<RuleDatabase> ruleDatabase,
    const std::vector<Variable>& outputVariables) {
  if (!ruleDatabase || ruleDatabase->size() == 0) {
    return std::nullopt;
  }

  try {
    auto reasoningOp = std::make_unique<ReasoningOperation>(
        qec_, std::move(ruleDatabase), outputVariables);

    auto tree = std::make_shared<QueryExecutionTree>(std::move(reasoningOp));
    return tree;

  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to create reasoning execution tree: " << e.what();
    return std::nullopt;
  }
}

size_t QueryReasoningHelper::estimateReasoningCost(
    const RuleDatabase& ruleDatabase) {
  // Cost estimation heuristic:
  // Base cost = 1000 per rule
  // Recursive rules multiply by 100 (fixpoint iteration)

  size_t baseCost = ruleDatabase.size() * 1000;
  size_t recursiveCost = 0;

  for (const auto& rule : ruleDatabase.getRules()) {
    if (rule->isRecursive()) {
      recursiveCost += 100000;  // Recursive rules are more expensive
    }
  }

  return baseCost + recursiveCost;
}

bool QueryReasoningHelper::isOptimizableRule(const Rule& rule) {
  // A rule is optimizable if it's a simple transitive pattern:
  // p(X, Z), p(Z, Y) => p(X, Y)
  // This can be converted to a property path: p+

  if (rule.getBody().size() != 2 || rule.getHead().size() != 1) {
    return false;
  }

  const auto& body1 = rule.getBody()[0];
  const auto& body2 = rule.getBody()[1];
  const auto& head = rule.getHead()[0];

  // Check if both body triples have the same predicate as the head
  if (!body1.p_.isIri() || !body2.p_.isIri() || !head.p_.isIri()) {
    return false;
  }

  if (body1.p_ != head.p_ || body2.p_ != head.p_) {
    return false;
  }

  // Check pattern: p(X, Z), p(Z, Y) => p(X, Y)
  if (body1.s_ == head.s_ && body1.o_ == body2.s_ && body2.o_ == head.o_) {
    return true;
  }

  return false;
}

std::optional<std::string> QueryReasoningHelper::convertToPropertyPath(
    const Rule& rule) {
  if (!isOptimizableRule(rule)) {
    return std::nullopt;
  }

  // Convert p(X, Z), p(Z, Y) => p(X, Y) to p+
  const auto& head = rule.getHead()[0];

  // Construct property path pattern
  std::string subject = head.s_.isVariable()
                            ? head.s_.getVariable().name()
                            : head.s_.getIri().getIri();

  std::string predicate = head.p_.getIri().getIri();

  std::string object = head.o_.isVariable() ? head.o_.getVariable().name()
                                             : head.o_.getIri().getIri();

  // Return SPARQL property path pattern
  return subject + " " + predicate + "+ " + object;
}

std::vector<std::shared_ptr<Rule>>
QueryReasoningHelper::extractRulesFromGraphPattern(
    const parsedQuery::GraphPattern& pattern) {
  // Extract rules from graph pattern
  // This is a simplified implementation
  // A full implementation would recursively traverse the graph pattern
  std::vector<std::shared_ptr<Rule>> rules;

  LOG(DEBUG) << "Extracting rules from graph pattern";

  return rules;
}

}  // namespace reasoning

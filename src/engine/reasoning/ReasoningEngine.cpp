// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include "engine/reasoning/ReasoningEngine.h"

#include <chrono>
#include <set>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "parser/SparqlParser.h"
#include "util/Log.h"

namespace reasoning {

ReasoningEngine::FactSet ReasoningEngine::executeReasoning(size_t maxIterations) {
  LOG(INFO) << "Starting Semi-Naïve reasoning with " << ruleDatabase_->size()
            << " rules";

  iterationStats_.clear();
  derivedFacts_.clear();
  deltaFacts_.clear();

  // Initialize delta with facts from the index
  for (const auto& rule : ruleDatabase_->getRules()) {
    for (const auto& triple : rule->getHead()) {
      // Query initial facts matching the head predicate
      auto facts = queryToFacts(rule->getHead());
      for (const auto& fact : facts) {
        deltaFacts_.insert(fact);
        derivedFacts_.insert(fact);
      }
    }
  }

  size_t iteration = 0;
  bool converged = false;

  while (!converged && (maxIterations == 0 || iteration < maxIterations)) {
    auto iterationStart = std::chrono::steady_clock::now();

    FactSet newFacts;

    // Apply all rules using Semi-Naïve evaluation
    // Only use delta facts from the previous iteration
    for (const auto& rule : ruleDatabase_->getRules()) {
      auto ruleFacts = applyRule(*rule, deltaFacts_);

      for (const auto& fact : ruleFacts) {
        if (derivedFacts_.find(fact) == derivedFacts_.end()) {
          newFacts.insert(fact);
          derivedFacts_.insert(fact);
        }
      }
    }

    // Update statistics
    auto iterationEnd = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(iterationEnd -
                                                               iterationStart);

    IterationStats stats;
    stats.newFactsGenerated = newFacts.size();
    stats.totalFactsInDatabase = derivedFacts_.size();
    stats.iterationTimeMs = duration.count();
    iterationStats_.push_back(stats);

    LOG(INFO) << "Iteration " << iteration << ": " << newFacts.size()
              << " new facts, total: " << derivedFacts_.size()
              << " facts (" << duration.count() << "ms)";

    // Check for convergence
    if (newFacts.empty()) {
      converged = true;
    }

    // For next iteration, delta becomes the new facts
    deltaFacts_ = newFacts;
    iteration++;
  }

  LOG(INFO) << "Reasoning completed after " << iteration << " iterations with "
            << derivedFacts_.size() << " derived facts";

  return derivedFacts_;
}

IdTable ReasoningEngine::executeReasoningAsIdTable(
    const std::vector<Variable>& variables) {
  auto facts = executeReasoning();

  // Convert facts to IdTable
  // Create IdTable with width = variables.size()
  IdTable result{variables.size()};

  for (const auto& fact : facts) {
    std::vector<Id> row = {fact[0], fact[1], fact[2]};
    result.push_back(row);
  }

  return result;
}

ReasoningEngine::FactSet ReasoningEngine::applyRule(const Rule& rule,
                                                     const FactSet& currentFacts) {
  FactSet newFacts;

  // For each triple in the rule body, query the database
  // Then unify with the head and generate new facts

  // This is a simplified implementation.
  // A full implementation would:
  // 1. Execute the rule body as a SPARQL query
  // 2. For each result row, instantiate the rule head
  // 3. Add the instantiated head to new facts

  auto bodyFacts = queryToFacts(rule.getBody());

  // For each fact in the body, try to instantiate the head
  for (const auto& bodyFact : bodyFacts) {
    // Extract variables from body
    std::vector<Variable> bodyVars;
    for (const auto& triple : rule.getBody()) {
      if (triple.s_.isVariable()) {
        bodyVars.push_back(triple.s_.getVariable());
      }
      if (triple.o_.isVariable()) {
        bodyVars.push_back(triple.o_.getVariable());
      }
    }

    // Create substitution mapping
    std::vector<Id> substitution = {bodyFact[0], bodyFact[1], bodyFact[2]};

    // Instantiate head with substitution
    for (const auto& headTriple : rule.getHead()) {
      auto instantiated = instantiateTriple(headTriple, substitution, bodyVars);
      if (instantiated) {
        newFacts.insert(*instantiated);
      }
    }
  }

  return newFacts;
}

std::optional<std::array<Id, 3>> ReasoningEngine::instantiateTriple(
    const SparqlTriple& pattern, const std::vector<Id>& substitution,
    const std::vector<Variable>& variables) const {
  Id subject = pattern.s_.isVariable()
                   ? substitution[0]
                   : Id::makeUndefined();  // Would use actual Id from pattern
  Id predicate = pattern.p_.isVariable()
                     ? substitution[1]
                     : Id::makeUndefined();
  Id object = pattern.o_.isVariable() ? substitution[2]
                                       : Id::makeUndefined();

  if (subject.isUndefined() || predicate.isUndefined() ||
      object.isUndefined()) {
    return std::nullopt;
  }

  return std::array<Id, 3>{subject, predicate, object};
}

std::vector<Id> ReasoningEngine::unifyVariables(
    const IdTable& resultTable, size_t rowIndex,
    const std::vector<Variable>& variables) const {
  std::vector<Id> result;

  for (size_t i = 0; i < variables.size(); ++i) {
    if (rowIndex < resultTable.size()) {
      result.push_back(resultTable.at(rowIndex, i));
    } else {
      result.push_back(Id::makeUndefined());
    }
  }

  return result;
}

ReasoningEngine::FactSet ReasoningEngine::queryToFacts(
    const std::vector<SparqlTriple>& pattern) {
  FactSet facts;

  // Build a SPARQL SELECT query from the pattern
  // Then execute it and convert results to facts
  // This is a simplified implementation

  try {
    // Create a SPARQL query
    std::string sparqlQuery = "SELECT * WHERE { ";
    for (size_t i = 0; i < pattern.size(); ++i) {
      sparqlQuery += pattern[i].asString();
      if (i < pattern.size() - 1) {
        sparqlQuery += " . ";
      }
    }
    sparqlQuery += " }";

    LOG(DEBUG) << "Executing reasoning query: " << sparqlQuery;

    // Parse and execute the query
    // Note: This is a placeholder - actual implementation would use
    // QueryPlanner to build an execution tree
    // For now, we return empty facts

  } catch (const std::exception& e) {
    LOG(WARN) << "Failed to execute reasoning query: " << e.what();
  }

  return facts;
}

}  // namespace reasoning

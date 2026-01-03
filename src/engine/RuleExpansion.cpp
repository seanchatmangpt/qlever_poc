//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 3)

#include "engine/RuleExpansion.h"

#include <set>
#include <sstream>

#include "engine/IndexScan.h"
#include "engine/Join.h"
#include "engine/QueryExecutionTree.h"
#include "parser/SparqlTriple.h"
#include "util/Exception.h"

// _____________________________________________________________________________
RuleExpansion::RuleExpansion(QueryExecutionContext* qec,
                             std::shared_ptr<RuleDatabase> ruleDatabase,
                             std::string rulePredicate,
                             std::vector<TripleComponent> arguments)
    : Operation(qec),
      ruleDatabase_(std::move(ruleDatabase)),
      rulePredicate_(std::move(rulePredicate)),
      arguments_(std::move(arguments)) {
  AD_CONTRACT_CHECK(ruleDatabase_ != nullptr);
  AD_CONTRACT_CHECK(!rulePredicate_.empty());
}

// _____________________________________________________________________________
std::string RuleExpansion::getDescriptor() const {
  std::ostringstream os;
  os << "RuleExpansion " << rulePredicate_ << "(";
  for (size_t i = 0; i < arguments_.size(); ++i) {
    if (i > 0) os << ", ";
    os << arguments_[i].toString();
  }
  os << ")";
  return os.str();
}

// _____________________________________________________________________________
size_t RuleExpansion::getResultWidth() const {
  // Count the number of variables in the arguments
  size_t width = 0;
  for (const auto& arg : arguments_) {
    if (arg.isVariable()) {
      ++width;
    }
  }
  return width;
}

// _____________________________________________________________________________
size_t RuleExpansion::getCostEstimate() {
  // If we have an expanded tree, use its cost estimate
  if (expandedTree_) {
    return expandedTree_->getCostEstimate();
  }

  // Otherwise, provide a conservative estimate
  // Rules can be expensive, especially recursive ones
  return 10000;
}

// _____________________________________________________________________________
uint64_t RuleExpansion::getSizeEstimateBeforeLimit() {
  if (sizeEstimateComputed_) {
    return sizeEstimate_;
  }

  // Get the rules for this predicate
  auto rules = ruleDatabase_->getRulesByPredicate(rulePredicate_);
  if (rules.empty()) {
    sizeEstimate_ = 0;
    sizeEstimateComputed_ = true;
    return sizeEstimate_;
  }

  // For now, use a simple heuristic: estimate based on the first rule
  // In a full implementation, we would combine estimates from all rules
  try {
    auto patterns = expandRuleToPatterns(rules[0]);
    auto tree = buildExecutionTree(patterns, rules[0]);
    sizeEstimate_ = tree->getSizeEstimate();
  } catch (const std::exception&) {
    // If expansion fails, return a conservative estimate
    sizeEstimate_ = 1000;
  }

  sizeEstimateComputed_ = true;
  return sizeEstimate_;
}

// _____________________________________________________________________________
float RuleExpansion::getMultiplicity(size_t col) {
  if (!multiplicities_.empty() && col < multiplicities_.size()) {
    return multiplicities_[col];
  }

  // Lazy initialization of multiplicities
  size_t width = getResultWidth();
  if (multiplicities_.empty()) {
    multiplicities_.resize(width, 1.0f);
  }

  // If we have an expanded tree, get multiplicities from it
  if (expandedTree_) {
    for (size_t i = 0; i < width && i < multiplicities_.size(); ++i) {
      try {
        multiplicities_[i] = expandedTree_->getMultiplicity(i);
      } catch (...) {
        multiplicities_[i] = 1.0f;
      }
    }
  }

  return col < multiplicities_.size() ? multiplicities_[col] : 1.0f;
}

// _____________________________________________________________________________
bool RuleExpansion::knownEmptyResult() {
  // Check if there are any rules for this predicate
  if (!ruleDatabase_->hasRuleFor(rulePredicate_)) {
    return true;
  }

  // If we have an expanded tree, check if it's known empty
  if (expandedTree_) {
    return expandedTree_->knownEmptyResult();
  }

  return false;
}

// _____________________________________________________________________________
std::vector<ColumnIndex> RuleExpansion::resultSortedOn() const {
  // If we have an expanded tree, return its sort order
  if (expandedTree_) {
    return expandedTree_->resultSortedOn();
  }

  // Otherwise, the result is not sorted
  return {};
}

// _____________________________________________________________________________
std::vector<QueryExecutionTree*> RuleExpansion::getChildren() {
  if (expandedTree_) {
    return {expandedTree_.get()};
  }
  return {};
}

// _____________________________________________________________________________
Result RuleExpansion::computeResult(bool requestLaziness) {
  // Get all rules for this predicate
  auto rules = ruleDatabase_->getRulesByPredicate(rulePredicate_);

  if (rules.empty()) {
    AD_THROW("No rules found for predicate: " + rulePredicate_);
  }

  // For now, we only expand the first rule
  // In a full implementation with multiple rules, we would need to UNION them
  const auto& rule = rules[0];

  // Expand the rule to patterns with mapped variables
  auto expandedPatterns = expandRuleToPatterns(rule);

  // Build execution tree from the expanded patterns
  expandedTree_ = buildExecutionTree(expandedPatterns, rule);

  // Execute the tree and get the result
  std::shared_ptr<const Result> subRes =
      expandedTree_->getResult(requestLaziness);

  // Clone the result to return by value
  // RuleExpansion acts as a transparent pass-through for the expanded tree
  IdTable idTable = subRes->idTable().clone();

  return {std::move(idTable), resultSortedOn(), subRes->getSharedLocalVocab()};
}

// _____________________________________________________________________________
std::string RuleExpansion::getCacheKeyImpl() const {
  std::ostringstream os;
  os << "RULE_EXPANSION " << rulePredicate_ << "(";
  for (size_t i = 0; i < arguments_.size(); ++i) {
    if (i > 0) os << ",";
    os << arguments_[i].toRdfLiteral();
  }
  os << ")";

  // EPIC 10.2: Include epoch ID and manifest hash for epoch isolation
  // This prevents cache contamination across different epochs
  os << " epoch=" << _executionContext->getCurrentEpochId();
  std::string manifestHash = _executionContext->getEpochDeterministicKey();
  if (!manifestHash.empty()) {
    os << " manifest=" << manifestHash;
  }

  return os.str();
}

// _____________________________________________________________________________
std::unique_ptr<Operation> RuleExpansion::cloneImpl() const {
  return std::make_unique<RuleExpansion>(_executionContext, ruleDatabase_,
                                         rulePredicate_, arguments_);
}

// _____________________________________________________________________________
VariableToColumnMap RuleExpansion::computeVariableToColumnMap() const {
  VariableToColumnMap result;

  // Map each variable argument to its column index
  size_t colIdx = 0;
  for (const auto& arg : arguments_) {
    if (arg.isVariable()) {
      result[arg.getVariable()] = {
          colIdx, ColumnIndexAndTypeInfo::UndefStatus::AlwaysDefined};
      ++colIdx;
    }
  }

  return result;
}

// _____________________________________________________________________________
std::vector<SparqlTriple> RuleExpansion::expandRuleToPatterns(
    const DatalogRule& rule) {
  // Create variable mapping from head variables to query arguments
  auto mapping = createVariableMapping(rule);

  // Expand each body pattern by applying the mapping
  std::vector<SparqlTriple> expandedPatterns;
  for (const auto& bodyPattern : rule.getBodyPatterns()) {
    // Apply mapping to subject, predicate, and object
    auto mappedSubject = applyMapping(bodyPattern.s_, mapping);
    auto mappedObject = applyMapping(bodyPattern.o_, mapping);

    // Handle predicate - need to check if it's a variable or IRI
    TripleComponent mappedPredicate;
    if (bodyPattern.p_.isVariable()) {
      mappedPredicate = applyMapping(bodyPattern.p_, mapping);
    } else {
      mappedPredicate = bodyPattern.p_;
    }

    // Create the expanded triple
    // Convert to SparqlTriple format
    SparqlTripleSimple simpleTriple(mappedSubject, mappedPredicate,
                                    mappedObject);
    expandedPatterns.push_back(SparqlTriple::fromSimple(simpleTriple));
  }

  return expandedPatterns;
}

// _____________________________________________________________________________
std::map<Variable, TripleComponent> RuleExpansion::createVariableMapping(
    const DatalogRule& rule) {
  std::map<Variable, TripleComponent> mapping;

  // Map head variables to the provided arguments
  const auto& headVars = rule.getHeadVariables();
  if (headVars.size() != arguments_.size()) {
    AD_THROW("Arity mismatch: rule " + rulePredicate_ + " expects " +
             std::to_string(headVars.size()) + " arguments but got " +
             std::to_string(arguments_.size()));
  }

  for (size_t i = 0; i < headVars.size(); ++i) {
    mapping[headVars[i]] = arguments_[i];
  }

  // For body variables not in the head, generate temporary variables
  std::set<Variable> headVarSet(headVars.begin(), headVars.end());
  for (const auto& bodyPattern : rule.getBodyPatterns()) {
    // Check subject
    if (bodyPattern.s_.isVariable()) {
      auto var = bodyPattern.s_.getVariable();
      if (headVarSet.find(var) == headVarSet.end() &&
          mapping.find(var) == mapping.end()) {
        mapping[var] =
            TripleComponent(generateTempVariable("temp", tempVarCounter_++));
      }
    }

    // Check predicate
    if (bodyPattern.p_.isVariable()) {
      auto var = bodyPattern.p_.getVariable();
      if (headVarSet.find(var) == headVarSet.end() &&
          mapping.find(var) == mapping.end()) {
        mapping[var] =
            TripleComponent(generateTempVariable("temp", tempVarCounter_++));
      }
    }

    // Check object
    if (bodyPattern.o_.isVariable()) {
      auto var = bodyPattern.o_.getVariable();
      if (headVarSet.find(var) == headVarSet.end() &&
          mapping.find(var) == mapping.end()) {
        mapping[var] =
            TripleComponent(generateTempVariable("temp", tempVarCounter_++));
      }
    }
  }

  return mapping;
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> RuleExpansion::buildExecutionTree(
    const std::vector<SparqlTriple>& expandedPatterns,
    [[maybe_unused]] const DatalogRule& rule) {
  if (expandedPatterns.empty()) {
    AD_THROW("Cannot build execution tree from empty pattern list");
  }

  // Start with the first pattern as an IndexScan
  auto firstPattern = expandedPatterns[0].getSimple();
  auto tree = std::make_shared<QueryExecutionTree>(_executionContext);
  tree = std::make_shared<QueryExecutionTree>(
      _executionContext,
      std::make_shared<IndexScan>(_executionContext, Permutation::Enum::PSO,
                                  firstPattern));

  // Join with remaining patterns
  for (size_t i = 1; i < expandedPatterns.size(); ++i) {
    auto pattern = expandedPatterns[i].getSimple();
    auto scanTree = std::make_shared<QueryExecutionTree>(
        _executionContext,
        std::make_shared<IndexScan>(_executionContext, Permutation::Enum::PSO,
                                    pattern));

    // Find join columns
    auto joinCols = QueryExecutionTree::getJoinColumns(*tree, *scanTree);

    if (joinCols.empty()) {
      // No common variables - this would be a Cartesian product
      // For now, throw an error as this is likely not intended
      AD_THROW("Pattern " + std::to_string(i) +
               " has no variables in common with previous patterns");
    }

    // Create join operation
    // Use the first join column pair
    auto join = std::make_shared<Join>(_executionContext, tree, scanTree,
                                       joinCols[0][0], joinCols[0][1]);
    tree = std::make_shared<QueryExecutionTree>(_executionContext, join);
  }

  // Future enhancement: Apply filters from rule.getFilters() if any

  return tree;
}

// _____________________________________________________________________________
Variable RuleExpansion::generateTempVariable(const std::string& baseName,
                                             size_t index) {
  return Variable("?" + baseName + "_" + std::to_string(index));
}

// _____________________________________________________________________________
TripleComponent RuleExpansion::applyMapping(
    const TripleComponent& component,
    const std::map<Variable, TripleComponent>& mapping) {
  // If it's a variable and in the mapping, replace it
  if (component.isVariable()) {
    auto var = component.getVariable();
    auto it = mapping.find(var);
    if (it != mapping.end()) {
      return it->second;
    }
  }

  // Otherwise, return unchanged (constants, unmapped variables)
  return component;
}

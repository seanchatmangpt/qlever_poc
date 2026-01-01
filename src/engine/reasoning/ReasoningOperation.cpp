// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include "engine/reasoning/ReasoningOperation.h"

#include "engine/idTable/IdTable.h"
#include "util/Log.h"

ReasoningOperation::ReasoningOperation(
    QueryExecutionContext* qec,
    std::shared_ptr<reasoning::RuleDatabase> ruleDatabase,
    const std::vector<Variable>& outputVariables)
    : Operation(qec),
      ruleDatabase_(std::move(ruleDatabase)),
      outputVariables_(outputVariables) {
  reasoningEngine_ =
      std::make_shared<reasoning::ReasoningEngine>(qec, ruleDatabase_);
}

std::vector<QueryExecutionTree*> ReasoningOperation::getChildren() {
  return {};  // ReasoningOperation has no children
}

std::string ReasoningOperation::getCacheKeyImpl() const {
  std::string key = "ReasoningOperation_" + std::to_string(ruleDatabase_->size());
  for (const auto& var : outputVariables_) {
    key += "_" + var.name();
  }
  return key;
}

std::string ReasoningOperation::getDescriptor() const {
  return "ReasoningOperation(" + std::to_string(ruleDatabase_->size()) +
         " rules)";
}

size_t ReasoningOperation::getResultWidth() const {
  if (!cachedResultWidth_) {
    cachedResultWidth_ = outputVariables_.size();
  }
  return *cachedResultWidth_;
}

size_t ReasoningOperation::getCostEstimate() {
  // Estimate cost as number of rules * average rule size
  // This is a rough heuristic
  return ruleDatabase_->size() * 1000;
}

uint64_t ReasoningOperation::getSizeEstimateBeforeLimit() {
  // Estimate output size based on number of rules and average rule size
  // This is a rough estimate; actual number depends on the knowledge base
  return ruleDatabase_->size() * 100;
}

float ReasoningOperation::getMultiplicity(size_t col) {
  // Conservative estimate: each variable appears about 10 times on average
  return 10.0f;
}

bool ReasoningOperation::knownEmptyResult() {
  return ruleDatabase_->size() == 0;
}

std::vector<ColumnIndex> ReasoningOperation::resultSortedOn() const {
  // Results from reasoning are typically not sorted
  return {};
}

Result ReasoningOperation::computeResult(bool requestLaziness) {
  LOG(INFO) << "Computing reasoning result with " << ruleDatabase_->size()
            << " rules";

  try {
    // Execute the reasoning engine
    auto idTable = reasoningEngine_->executeReasoningAsIdTable(outputVariables_);

    LOG(INFO) << "Reasoning produced " << idTable.size() << " results";

    // Wrap in a Result object
    return Result{idTable, outputVariables_};

  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to execute reasoning: " << e.what();
    throw;
  }
}

std::unique_ptr<Operation> ReasoningOperation::cloneImpl() const {
  return std::make_unique<ReasoningOperation>(getExecutionContext(),
                                              ruleDatabase_, outputVariables_);
}

VariableToColumnMap ReasoningOperation::computeVariableToColumnMap() const {
  VariableToColumnMap map;

  for (size_t i = 0; i < outputVariables_.size(); ++i) {
    map.emplace(outputVariables_[i], i);
  }

  return map;
}

IdTable ReasoningOperation::resultToIdTable(
    const reasoning::ReasoningEngine::FactSet& facts) const {
  IdTable result{outputVariables_.size()};

  for (const auto& fact : facts) {
    std::vector<Id> row = {fact[0], fact[1], fact[2]};
    result.push_back(row);
  }

  return result;
}

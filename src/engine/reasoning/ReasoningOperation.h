// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_REASONINGOPERATION_H
#define QLEVER_SRC_ENGINE_REASONING_REASONINGOPERATION_H

#include <memory>
#include <string>
#include <vector>

#include "engine/Operation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/reasoning/ReasoningEngine.h"
#include "engine/reasoning/Rule.h"
#include "parser/ParsedQuery.h"

/// ReasoningOperation integrates Datalog/N3 reasoning into the SPARQL
/// execution engine. It executes Semi-Naïve evaluation with fixpoint
/// iteration to derive new facts from the knowledge base.
///
/// Usage:
/// 1. Create a ReasoningOperation with a RuleDatabase
/// 2. Call getResult() to execute the reasoning
/// 3. The result contains all derived facts
class ReasoningOperation : public Operation {
 public:
  explicit ReasoningOperation(
      QueryExecutionContext* qec,
      std::shared_ptr<reasoning::RuleDatabase> ruleDatabase,
      const std::vector<Variable>& outputVariables = {});

  ~ReasoningOperation() override = default;

  // Get non-owning pointers to child trees (no children for reasoning op)
  std::vector<QueryExecutionTree*> getChildren() override;

  // Operation interface implementation
  [[nodiscard]] std::string getCacheKeyImpl() const override;
  [[nodiscard]] std::string getDescriptor() const override;
  [[nodiscard]] size_t getResultWidth() const override;
  size_t getCostEstimate() override;
  uint64_t getSizeEstimateBeforeLimit() override;
  float getMultiplicity(size_t col) override;
  bool knownEmptyResult() override;
  [[nodiscard]] std::vector<ColumnIndex> resultSortedOn() const override;

 private:
  std::shared_ptr<reasoning::RuleDatabase> ruleDatabase_;
  std::shared_ptr<reasoning::ReasoningEngine> reasoningEngine_;
  std::vector<Variable> outputVariables_;
  mutable std::optional<size_t> cachedResultWidth_;

  // Execution implementation
  Result computeResult(bool requestLaziness) override;
  std::unique_ptr<Operation> cloneImpl() const override;
  [[nodiscard]] VariableToColumnMap computeVariableToColumnMap()
      const override;

  /// Convert reasoning engine results to an IdTable.
  [[nodiscard]] IdTable resultToIdTable(
      const reasoning::ReasoningEngine::FactSet& facts) const;
};

#endif  // QLEVER_SRC_ENGINE_REASONING_REASONINGOPERATION_H

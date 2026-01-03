// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: EPIC 14.0 Formalism Convergence - Agent 7
//
// Implementation of UnifiedFormalismOperation and formalism-specific executors

#include "engine/formalism/unified/UnifiedFormalismOperation.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/QueryExecutionTree.h"
#include "engine/Result.h"
#include "util/Exception.h"

namespace formalism {

// =============================================================================
// ShaclExecutor Implementation
// =============================================================================

ShaclExecutor::ShaclExecutor(std::string shapeGraphUri,
                             std::shared_ptr<QueryExecutionTree> dataTree)
    : shapeGraphUri_(std::move(shapeGraphUri)),
      dataTree_(std::move(dataTree)) {}

Result ShaclExecutor::execute(
    QueryExecutionContext* qec,
    const ad_utility::SharedCancellationHandle& handle,
    std::chrono::steady_clock::time_point deadline) {
  // TODO(EPIC 14.1): Integrate with existing ShaclValidator
  // For now, stub implementation that throws
  throw std::runtime_error(
      "ShaclExecutor::execute() not yet implemented - awaiting EPIC 14.1 "
      "convergence");
}

uint64_t ShaclExecutor::estimateResultSize() const {
  // TODO(EPIC 14.1): Implement size estimation
  return dataTree_ ? dataTree_->getSizeEstimate() : 0;
}

size_t ShaclExecutor::getResultWidth() const {
  // SHACL validation result: ?focusNode ?constraintComponent ?severity ?message
  return 4;
}

std::vector<ColumnIndex> ShaclExecutor::getResultSortedOn() const {
  // SHACL results sorted by focus node (column 0)
  return {ColumnIndex(0)};
}

std::string ShaclExecutor::getCacheKeyComponent() const {
  std::string dataTreeKey = dataTree_ ? dataTree_->getCacheKey() : "NONE";
  return absl::StrCat("SHACL:", shapeGraphUri_, ":", dataTreeKey);
}

std::string ShaclExecutor::getDescriptor() const {
  return absl::StrCat("SHACL Validation [", shapeGraphUri_, "]");
}

VariableToColumnMap ShaclExecutor::getVariableToColumnMap() const {
  // SHACL standard violation schema
  VariableToColumnMap map;
  map[Variable("?focusNode")] = {ColumnIndex(0), {}};
  map[Variable("?constraintComponent")] = {ColumnIndex(1), {}};
  map[Variable("?severity")] = {ColumnIndex(2), {}};
  map[Variable("?message")] = {ColumnIndex(3), {}};
  return map;
}

size_t ShaclExecutor::getCostEstimate() const {
  // TODO(EPIC 14.1): Implement cost estimation
  // Rough estimate: proportional to data tree size
  return dataTree_ ? dataTree_->getCostEstimate() * 2 : 0;
}

float ShaclExecutor::getMultiplicity(size_t col) const {
  // Conservative estimate: assume high multiplicity for violations
  return 1.0f;
}

bool ShaclExecutor::knownEmptyResult() const {
  // Result is empty only if data tree is empty
  return dataTree_ ? dataTree_->knownEmptyResult() : true;
}

std::unique_ptr<IFormalismExecutor> ShaclExecutor::clone() const {
  return std::make_unique<ShaclExecutor>(shapeGraphUri_, dataTree_);
}

// =============================================================================
// DatalogExecutor Implementation
// =============================================================================

DatalogExecutor::DatalogExecutor(
    std::string ruleSetId,
    std::vector<std::shared_ptr<QueryExecutionTree>> dependentSubtrees)
    : ruleSetId_(std::move(ruleSetId)),
      dependentSubtrees_(std::move(dependentSubtrees)) {}

Result DatalogExecutor::execute(
    QueryExecutionContext* qec,
    const ad_utility::SharedCancellationHandle& handle,
    std::chrono::steady_clock::time_point deadline) {
  // TODO(EPIC 14.1): Integrate with existing FixpointComputation
  throw std::runtime_error(
      "DatalogExecutor::execute() not yet implemented - awaiting EPIC 14.1 "
      "convergence");
}

uint64_t DatalogExecutor::estimateResultSize() const {
  // TODO(EPIC 14.1): Implement fixpoint size estimation
  // Conservative estimate: sum of all dependent subtree sizes
  uint64_t total = 0;
  for (const auto& tree : dependentSubtrees_) {
    if (tree) {
      total += tree->getSizeEstimate();
    }
  }
  return total;
}

size_t DatalogExecutor::getResultWidth() const {
  // TODO(EPIC 14.1): Determine width from rule head
  // For now, assume binary predicate (2 columns)
  return 2;
}

std::vector<ColumnIndex> DatalogExecutor::getResultSortedOn() const {
  // Datalog results sorted by first column (subject)
  return {ColumnIndex(0)};
}

std::string DatalogExecutor::getCacheKeyComponent() const {
  std::vector<std::string> subtreeKeys;
  subtreeKeys.reserve(dependentSubtrees_.size());
  for (const auto& tree : dependentSubtrees_) {
    subtreeKeys.push_back(tree ? tree->getCacheKey() : "NONE");
  }
  return absl::StrCat("DATALOG:", ruleSetId_, ":",
                      absl::StrJoin(subtreeKeys, ":"));
}

std::string DatalogExecutor::getDescriptor() const {
  return absl::StrCat("Datalog Rule Evaluation [", ruleSetId_, "]");
}

VariableToColumnMap DatalogExecutor::getVariableToColumnMap() const {
  // TODO(EPIC 14.1): Determine variables from rule head
  // For now, stub with generic subject/object variables
  VariableToColumnMap map;
  map[Variable("?subject")] = {ColumnIndex(0), {}};
  map[Variable("?object")] = {ColumnIndex(1), {}};
  return map;
}

size_t DatalogExecutor::getCostEstimate() const {
  // TODO(EPIC 14.1): Implement fixpoint cost estimation
  // Conservative estimate: sum of subtree costs * fixpoint iterations
  size_t total = 0;
  for (const auto& tree : dependentSubtrees_) {
    if (tree) {
      total += tree->getCostEstimate();
    }
  }
  return total * 10;  // Assume max 10 fixpoint iterations
}

float DatalogExecutor::getMultiplicity(size_t col) const {
  // Conservative estimate: assume moderate multiplicity
  return 5.0f;
}

bool DatalogExecutor::knownEmptyResult() const {
  // Result is empty if any dependent subtree is empty
  for (const auto& tree : dependentSubtrees_) {
    if (tree && tree->knownEmptyResult()) {
      return true;
    }
  }
  return false;
}

std::unique_ptr<IFormalismExecutor> DatalogExecutor::clone() const {
  return std::make_unique<DatalogExecutor>(ruleSetId_, dependentSubtrees_);
}

// =============================================================================
// N3Executor Implementation
// =============================================================================

N3Executor::N3Executor(std::string patternGraphUri,
                       std::shared_ptr<QueryExecutionTree> sourceTree)
    : patternGraphUri_(std::move(patternGraphUri)),
      sourceTree_(std::move(sourceTree)) {}

Result N3Executor::execute(
    QueryExecutionContext* qec,
    const ad_utility::SharedCancellationHandle& handle,
    std::chrono::steady_clock::time_point deadline) {
  // TODO(EPIC 14.1): Integrate with existing N3ComplianceVerifier
  throw std::runtime_error(
      "N3Executor::execute() not yet implemented - awaiting EPIC 14.1 "
      "convergence");
}

uint64_t N3Executor::estimateResultSize() const {
  // N3 pattern matching: estimate proportional to source tree size
  return sourceTree_ ? sourceTree_->getSizeEstimate() : 0;
}

size_t N3Executor::getResultWidth() const {
  // TODO(EPIC 14.1): Determine width from N3 pattern variables
  // For now, assume source tree width (pattern preserves variables)
  return sourceTree_ ? sourceTree_->getResultWidth() : 0;
}

std::vector<ColumnIndex> N3Executor::getResultSortedOn() const {
  // N3 preserves source tree sort order
  return sourceTree_ ? sourceTree_->resultSortedOn() : std::vector<ColumnIndex>{};
}

std::string N3Executor::getCacheKeyComponent() const {
  std::string sourceTreeKey = sourceTree_ ? sourceTree_->getCacheKey() : "NONE";
  return absl::StrCat("N3:", patternGraphUri_, ":", sourceTreeKey);
}

std::string N3Executor::getDescriptor() const {
  return absl::StrCat("N3 Pattern Matching [", patternGraphUri_, "]");
}

VariableToColumnMap N3Executor::getVariableToColumnMap() const {
  // TODO(EPIC 14.1): Determine variables from N3 pattern
  // For now, inherit from source tree
  return sourceTree_ ? sourceTree_->getVariableColumns()
                     : VariableToColumnMap{};
}

size_t N3Executor::getCostEstimate() const {
  // Pattern matching cost proportional to source tree
  return sourceTree_ ? sourceTree_->getCostEstimate() : 0;
}

float N3Executor::getMultiplicity(size_t col) const {
  // Inherit multiplicity from source tree
  return sourceTree_ ? sourceTree_->getMultiplicity(col) : 1.0f;
}

bool N3Executor::knownEmptyResult() const {
  // Result is empty if source tree is empty
  return sourceTree_ ? sourceTree_->knownEmptyResult() : true;
}

std::unique_ptr<IFormalismExecutor> N3Executor::clone() const {
  return std::make_unique<N3Executor>(patternGraphUri_, sourceTree_);
}

// =============================================================================
// ShExExecutor Implementation
// =============================================================================

ShExExecutor::ShExExecutor(std::string schemaUri,
                           std::shared_ptr<QueryExecutionTree> dataTree)
    : schemaUri_(std::move(schemaUri)), dataTree_(std::move(dataTree)) {}

Result ShExExecutor::execute(
    QueryExecutionContext* qec,
    const ad_utility::SharedCancellationHandle& handle,
    std::chrono::steady_clock::time_point deadline) {
  // TODO(EPIC 14.1): Implement ShEx validation
  // ShEx is currently stub-only in QLever
  throw std::runtime_error(
      "ShExExecutor::execute() not yet implemented - ShEx is stub-only");
}

uint64_t ShExExecutor::estimateResultSize() const {
  return dataTree_ ? dataTree_->getSizeEstimate() : 0;
}

size_t ShExExecutor::getResultWidth() const {
  // ShEx validation result: ?node ?shape ?status ?message
  return 4;
}

std::vector<ColumnIndex> ShExExecutor::getResultSortedOn() const {
  // ShEx results sorted by node (column 0)
  return {ColumnIndex(0)};
}

std::string ShExExecutor::getCacheKeyComponent() const {
  std::string dataTreeKey = dataTree_ ? dataTree_->getCacheKey() : "NONE";
  return absl::StrCat("SHEX:", schemaUri_, ":", dataTreeKey);
}

std::string ShExExecutor::getDescriptor() const {
  return absl::StrCat("ShEx Validation [", schemaUri_, "]");
}

VariableToColumnMap ShExExecutor::getVariableToColumnMap() const {
  // ShEx validation schema (similar to SHACL)
  VariableToColumnMap map;
  map[Variable("?node")] = {ColumnIndex(0), {}};
  map[Variable("?shape")] = {ColumnIndex(1), {}};
  map[Variable("?status")] = {ColumnIndex(2), {}};
  map[Variable("?message")] = {ColumnIndex(3), {}};
  return map;
}

size_t ShExExecutor::getCostEstimate() const {
  // Conservative estimate: proportional to data tree size
  return dataTree_ ? dataTree_->getCostEstimate() * 2 : 0;
}

float ShExExecutor::getMultiplicity(size_t col) const {
  // Conservative estimate: assume high multiplicity
  return 1.0f;
}

bool ShExExecutor::knownEmptyResult() const {
  return dataTree_ ? dataTree_->knownEmptyResult() : true;
}

std::unique_ptr<IFormalismExecutor> ShExExecutor::clone() const {
  return std::make_unique<ShExExecutor>(schemaUri_, dataTree_);
}

}  // namespace formalism

// =============================================================================
// UnifiedFormalismOperation Implementation
// =============================================================================

UnifiedFormalismOperation::UnifiedFormalismOperation(
    QueryExecutionContext* qec,
    std::unique_ptr<formalism::IFormalismExecutor> executor)
    : Operation(qec), executor_(std::move(executor)) {
  AD_CONTRACT_CHECK(executor_ != nullptr);
}

std::shared_ptr<QueryExecutionTree>
UnifiedFormalismOperation::createShaclOperation(
    QueryExecutionContext* qec, std::string shapeGraphUri,
    std::shared_ptr<QueryExecutionTree> dataTree) {
  auto executor = std::make_unique<formalism::ShaclExecutor>(
      std::move(shapeGraphUri), dataTree);
  auto operation =
      std::make_shared<UnifiedFormalismOperation>(qec, std::move(executor));
  operation->childTrees_ = {dataTree};
  return std::make_shared<QueryExecutionTree>(qec, std::move(operation));
}

std::shared_ptr<QueryExecutionTree>
UnifiedFormalismOperation::createDatalogOperation(
    QueryExecutionContext* qec, std::string ruleSetId,
    std::vector<std::shared_ptr<QueryExecutionTree>> dependentSubtrees) {
  auto executor = std::make_unique<formalism::DatalogExecutor>(
      std::move(ruleSetId), dependentSubtrees);
  auto operation =
      std::make_shared<UnifiedFormalismOperation>(qec, std::move(executor));
  operation->childTrees_ = std::move(dependentSubtrees);
  return std::make_shared<QueryExecutionTree>(qec, std::move(operation));
}

std::shared_ptr<QueryExecutionTree>
UnifiedFormalismOperation::createN3Operation(
    QueryExecutionContext* qec, std::string patternGraphUri,
    std::shared_ptr<QueryExecutionTree> sourceTree) {
  auto executor = std::make_unique<formalism::N3Executor>(
      std::move(patternGraphUri), sourceTree);
  auto operation =
      std::make_shared<UnifiedFormalismOperation>(qec, std::move(executor));
  operation->childTrees_ = {sourceTree};
  return std::make_shared<QueryExecutionTree>(qec, std::move(operation));
}

std::shared_ptr<QueryExecutionTree>
UnifiedFormalismOperation::createShExOperation(
    QueryExecutionContext* qec, std::string schemaUri,
    std::shared_ptr<QueryExecutionTree> dataTree) {
  auto executor = std::make_unique<formalism::ShExExecutor>(
      std::move(schemaUri), dataTree);
  auto operation =
      std::make_shared<UnifiedFormalismOperation>(qec, std::move(executor));
  operation->childTrees_ = {dataTree};
  return std::make_shared<QueryExecutionTree>(qec, std::move(operation));
}

std::vector<QueryExecutionTree*> UnifiedFormalismOperation::getChildren() {
  std::vector<QueryExecutionTree*> result;
  result.reserve(childTrees_.size());
  for (auto& tree : childTrees_) {
    result.push_back(tree.get());
  }
  return result;
}

std::string UnifiedFormalismOperation::getCacheKeyImpl() const {
  auto lock = executorMutex_.rlock();
  return executor_->getCacheKeyComponent();
}

std::string UnifiedFormalismOperation::getDescriptor() const {
  auto lock = executorMutex_.rlock();
  return executor_->getDescriptor();
}

size_t UnifiedFormalismOperation::getResultWidth() const {
  auto lock = executorMutex_.rlock();
  return executor_->getResultWidth();
}

std::vector<ColumnIndex> UnifiedFormalismOperation::resultSortedOn() const {
  auto lock = executorMutex_.rlock();
  return executor_->getResultSortedOn();
}

uint64_t UnifiedFormalismOperation::getSizeEstimateBeforeLimit() {
  auto lock = executorMutex_.rlock();
  return executor_->estimateResultSize();
}

size_t UnifiedFormalismOperation::getCostEstimate() {
  auto lock = executorMutex_.rlock();
  return executor_->getCostEstimate();
}

float UnifiedFormalismOperation::getMultiplicity(size_t col) {
  auto lock = executorMutex_.rlock();
  return executor_->getMultiplicity(col);
}

bool UnifiedFormalismOperation::knownEmptyResult() {
  auto lock = executorMutex_.rlock();
  return executor_->knownEmptyResult();
}

std::unique_ptr<Operation> UnifiedFormalismOperation::cloneImpl() const {
  auto lock = executorMutex_.rlock();
  auto clonedExecutor = executor_->clone();
  auto clonedOp = std::make_unique<UnifiedFormalismOperation>(
      _executionContext, std::move(clonedExecutor));
  clonedOp->childTrees_ = childTrees_;  // Shallow copy of shared_ptrs
  return clonedOp;
}

VariableToColumnMap UnifiedFormalismOperation::computeVariableToColumnMap()
    const {
  auto lock = executorMutex_.rlock();
  return executor_->getVariableToColumnMap();
}

Result UnifiedFormalismOperation::computeResult(
    [[maybe_unused]] bool requestLaziness) {
  auto lock = executorMutex_.rlock();
  checkCancellation();
  return executor_->execute(_executionContext, cancellationHandle_, deadline_);
}

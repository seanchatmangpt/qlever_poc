// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: EPIC 14.0 Formalism Convergence - Agent 7
//
// Unified Operation class for all formalisms (SHACL, Datalog, N3, ShEx)
// in QueryExecutionTree. Provides single integration point for formalism
// execution with thread-safe operation and consistent Operation interface.

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMOPERATION_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMOPERATION_H

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "engine/Operation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/Result.h"
#include "engine/VariableToColumnMap.h"
#include "util/CancellationHandle.h"
#include "util/CopyableSynchronization.h"

// Forward declarations for formalism-specific types
namespace formalism {

// Formalism type enumeration
enum class FormalismType {
  SHACL,   // SHACL validation
  DATALOG, // Datalog rule evaluation
  N3,      // N3 pattern matching
  SHEX     // ShEx schema validation
};

// Abstract base interface for formalism-specific executors
// Each formalism implements this interface to provide uniform execution
class IFormalismExecutor {
 public:
  virtual ~IFormalismExecutor() = default;

  // Execute formalism-specific logic and return Result
  virtual Result execute(QueryExecutionContext* qec,
                         const ad_utility::SharedCancellationHandle& handle,
                         std::chrono::steady_clock::time_point deadline) = 0;

  // Get estimated result size before execution
  virtual uint64_t estimateResultSize() const = 0;

  // Get result width (number of columns)
  virtual size_t getResultWidth() const = 0;

  // Get columns by which result will be sorted
  virtual std::vector<ColumnIndex> getResultSortedOn() const = 0;

  // Get formalism-specific cache key component
  virtual std::string getCacheKeyComponent() const = 0;

  // Get human-readable descriptor
  virtual std::string getDescriptor() const = 0;

  // Get variable to column mapping
  virtual VariableToColumnMap getVariableToColumnMap() const = 0;

  // Get cost estimate for execution
  virtual size_t getCostEstimate() const = 0;

  // Get multiplicity estimate for given column
  virtual float getMultiplicity(size_t col) const = 0;

  // Check if result is known to be empty
  virtual bool knownEmptyResult() const = 0;

  // Clone this executor
  virtual std::unique_ptr<IFormalismExecutor> clone() const = 0;
};

// SHACL-specific executor
class ShaclExecutor : public IFormalismExecutor {
 public:
  explicit ShaclExecutor(std::string shapeGraphUri,
                         std::shared_ptr<QueryExecutionTree> dataTree);

  Result execute(QueryExecutionContext* qec,
                 const ad_utility::SharedCancellationHandle& handle,
                 std::chrono::steady_clock::time_point deadline) override;

  uint64_t estimateResultSize() const override;
  size_t getResultWidth() const override;
  std::vector<ColumnIndex> getResultSortedOn() const override;
  std::string getCacheKeyComponent() const override;
  std::string getDescriptor() const override;
  VariableToColumnMap getVariableToColumnMap() const override;
  size_t getCostEstimate() const override;
  float getMultiplicity(size_t col) const override;
  bool knownEmptyResult() const override;
  std::unique_ptr<IFormalismExecutor> clone() const override;

 private:
  std::string shapeGraphUri_;
  std::shared_ptr<QueryExecutionTree> dataTree_;
};

// Datalog-specific executor
class DatalogExecutor : public IFormalismExecutor {
 public:
  explicit DatalogExecutor(std::string ruleSetId,
                           std::vector<std::shared_ptr<QueryExecutionTree>>
                               dependentSubtrees);

  Result execute(QueryExecutionContext* qec,
                 const ad_utility::SharedCancellationHandle& handle,
                 std::chrono::steady_clock::time_point deadline) override;

  uint64_t estimateResultSize() const override;
  size_t getResultWidth() const override;
  std::vector<ColumnIndex> getResultSortedOn() const override;
  std::string getCacheKeyComponent() const override;
  std::string getDescriptor() const override;
  VariableToColumnMap getVariableToColumnMap() const override;
  size_t getCostEstimate() const override;
  float getMultiplicity(size_t col) const override;
  bool knownEmptyResult() const override;
  std::unique_ptr<IFormalismExecutor> clone() const override;

 private:
  std::string ruleSetId_;
  std::vector<std::shared_ptr<QueryExecutionTree>> dependentSubtrees_;
};

// N3-specific executor
class N3Executor : public IFormalismExecutor {
 public:
  explicit N3Executor(std::string patternGraphUri,
                      std::shared_ptr<QueryExecutionTree> sourceTree);

  Result execute(QueryExecutionContext* qec,
                 const ad_utility::SharedCancellationHandle& handle,
                 std::chrono::steady_clock::time_point deadline) override;

  uint64_t estimateResultSize() const override;
  size_t getResultWidth() const override;
  std::vector<ColumnIndex> getResultSortedOn() const override;
  std::string getCacheKeyComponent() const override;
  std::string getDescriptor() const override;
  VariableToColumnMap getVariableToColumnMap() const override;
  size_t getCostEstimate() const override;
  float getMultiplicity(size_t col) const override;
  bool knownEmptyResult() const override;
  std::unique_ptr<IFormalismExecutor> clone() const override;

 private:
  std::string patternGraphUri_;
  std::shared_ptr<QueryExecutionTree> sourceTree_;
};

// ShEx-specific executor
class ShExExecutor : public IFormalismExecutor {
 public:
  explicit ShExExecutor(std::string schemaUri,
                        std::shared_ptr<QueryExecutionTree> dataTree);

  Result execute(QueryExecutionContext* qec,
                 const ad_utility::SharedCancellationHandle& handle,
                 std::chrono::steady_clock::time_point deadline) override;

  uint64_t estimateResultSize() const override;
  size_t getResultWidth() const override;
  std::vector<ColumnIndex> getResultSortedOn() const override;
  std::string getCacheKeyComponent() const override;
  std::string getDescriptor() const override;
  VariableToColumnMap getVariableToColumnMap() const override;
  size_t getCostEstimate() const override;
  float getMultiplicity(size_t col) const override;
  bool knownEmptyResult() const override;
  std::unique_ptr<IFormalismExecutor> clone() const override;

 private:
  std::string schemaUri_;
  std::shared_ptr<QueryExecutionTree> dataTree_;
};

}  // namespace formalism

// Unified Operation for all formalism types
// Delegates to formalism-specific executors while providing uniform Operation
// interface
class UnifiedFormalismOperation : public Operation {
 public:
  // Constructor: accepts QueryExecutionContext and formalism-specific executor
  UnifiedFormalismOperation(
      QueryExecutionContext* qec,
      std::unique_ptr<formalism::IFormalismExecutor> executor);

  // Factory methods for each formalism type
  static std::shared_ptr<QueryExecutionTree> createShaclOperation(
      QueryExecutionContext* qec, std::string shapeGraphUri,
      std::shared_ptr<QueryExecutionTree> dataTree);

  static std::shared_ptr<QueryExecutionTree> createDatalogOperation(
      QueryExecutionContext* qec, std::string ruleSetId,
      std::vector<std::shared_ptr<QueryExecutionTree>> dependentSubtrees);

  static std::shared_ptr<QueryExecutionTree> createN3Operation(
      QueryExecutionContext* qec, std::string patternGraphUri,
      std::shared_ptr<QueryExecutionTree> sourceTree);

  static std::shared_ptr<QueryExecutionTree> createShExOperation(
      QueryExecutionContext* qec, std::string schemaUri,
      std::shared_ptr<QueryExecutionTree> dataTree);

  // Operation interface implementation
  std::vector<QueryExecutionTree*> getChildren() override;

  std::string getCacheKeyImpl() const override;

  std::string getDescriptor() const override;

  size_t getResultWidth() const override;

  std::vector<ColumnIndex> resultSortedOn() const override;

  uint64_t getSizeEstimateBeforeLimit() override;

  size_t getCostEstimate() override;

  float getMultiplicity(size_t col) override;

  bool knownEmptyResult() override;

 private:
  std::unique_ptr<Operation> cloneImpl() const override;

  VariableToColumnMap computeVariableToColumnMap() const override;

  Result computeResult(bool requestLaziness) override;

  // Formalism-specific executor (polymorphic)
  std::unique_ptr<formalism::IFormalismExecutor> executor_;

  // Mutex for thread-safe access to executor state
  mutable ad_utility::CopyableMutex executorMutex_;

  // Child trees (extracted from executor for getChildren())
  std::vector<std::shared_ptr<QueryExecutionTree>> childTrees_;
};

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMOPERATION_H

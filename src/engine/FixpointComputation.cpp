//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 6)

#include "engine/FixpointComputation.h"

#include <algorithm>
#include <sstream>
#include <unordered_set>

#include "engine/QueryExecutionTree.h"
#include "util/Exception.h"
#include "util/Log.h"

// _____________________________________________________________________________
FixpointComputation::FixpointComputation(
    QueryExecutionContext* qec, std::shared_ptr<RuleDatabase> ruleDatabase,
    std::string rulePredicate, std::vector<TripleComponent> arguments,
    size_t maxIterations)
    : Operation(qec),
      ruleDatabase_(std::move(ruleDatabase)),
      rulePredicate_(std::move(rulePredicate)),
      arguments_(std::move(arguments)),
      maxIterations_(maxIterations) {
  AD_CONTRACT_CHECK(ruleDatabase_ != nullptr);
  AD_CONTRACT_CHECK(!rulePredicate_.empty());
  AD_CONTRACT_CHECK(maxIterations_ > 0);
}

// _____________________________________________________________________________
std::string FixpointComputation::getDescriptor() const {
  std::ostringstream os;
  os << "FixpointComputation " << rulePredicate_ << "(";
  for (size_t i = 0; i < arguments_.size(); ++i) {
    if (i > 0) os << ", ";
    os << arguments_[i].toString();
  }
  os << ") [max_iter=" << maxIterations_ << "]";
  return os.str();
}

// _____________________________________________________________________________
size_t FixpointComputation::getResultWidth() const {
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
size_t FixpointComputation::getCostEstimate() {
  // Fixpoint computation can be expensive, especially for recursive rules
  // Estimate based on: max_iterations * cost_per_iteration
  if (ruleExpansionTree_) {
    // Use the cost of the rule expansion times expected iterations
    // Conservative estimate: assume half of max iterations
    return ruleExpansionTree_->getCostEstimate() * (maxIterations_ / 2);
  }

  // Conservative default estimate
  return maxIterations_ * 10000;
}

// _____________________________________________________________________________
uint64_t FixpointComputation::getSizeEstimateBeforeLimit() {
  if (sizeEstimateComputed_) {
    return sizeEstimate_;
  }

  // Check if there are any rules
  if (!ruleDatabase_->hasRuleFor(rulePredicate_)) {
    sizeEstimate_ = 0;
    sizeEstimateComputed_ = true;
    return sizeEstimate_;
  }

  // For fixpoint computation, estimate based on potential growth
  // This is a heuristic: assume exponential growth with dampening
  // Start with initial rule expansion estimate
  auto rules = ruleDatabase_->getRulesByPredicate(rulePredicate_);
  if (!rules.empty()) {
    try {
      // Create a temporary RuleExpansion to get base estimate
      auto tempExpansion = std::make_unique<RuleExpansion>(
          _executionContext, ruleDatabase_, rulePredicate_, arguments_);
      uint64_t baseEstimate = tempExpansion->getSizeEstimateBeforeLimit();

      // Estimate growth: each iteration might add 50% more facts
      // But with diminishing returns
      sizeEstimate_ = baseEstimate;
      for (size_t i = 1; i < std::min(maxIterations_, size_t(5)); ++i) {
        sizeEstimate_ += baseEstimate / (i + 1);
      }
    } catch (...) {
      // If estimation fails, use conservative estimate
      sizeEstimate_ = 10000;
    }
  } else {
    sizeEstimate_ = 0;
  }

  sizeEstimateComputed_ = true;
  return sizeEstimate_;
}

// _____________________________________________________________________________
float FixpointComputation::getMultiplicity(size_t col) {
  if (!multiplicities_.empty() && col < multiplicities_.size()) {
    return multiplicities_[col];
  }

  // Lazy initialization
  size_t width = getResultWidth();
  if (multiplicities_.empty()) {
    multiplicities_.resize(width, 1.0f);
  }

  // For fixpoint computation, multiplicity can increase across iterations
  // Use a conservative estimate
  for (size_t i = 0; i < width && i < multiplicities_.size(); ++i) {
    multiplicities_[i] = 2.0f;  // Conservative: assume some duplication
  }

  return col < multiplicities_.size() ? multiplicities_[col] : 1.0f;
}

// _____________________________________________________________________________
bool FixpointComputation::knownEmptyResult() {
  // Check if there are any rules for this predicate
  if (!ruleDatabase_->hasRuleFor(rulePredicate_)) {
    return true;
  }

  // If we have a rule expansion tree, check if it's known empty
  if (ruleExpansionTree_) {
    return ruleExpansionTree_->knownEmptyResult();
  }

  return false;
}

// _____________________________________________________________________________
std::vector<ColumnIndex> FixpointComputation::resultSortedOn() const {
  // After fixpoint computation, results are typically not sorted
  // due to merging from multiple iterations
  return {};
}

// _____________________________________________________________________________
std::vector<QueryExecutionTree*> FixpointComputation::getChildren() {
  if (ruleExpansionTree_) {
    return {ruleExpansionTree_.get()};
  }
  return {};
}

// _____________________________________________________________________________
Result FixpointComputation::computeResult([[maybe_unused]] bool requestLaziness) {
  LOG(DEBUG) << "Starting fixpoint computation for " << rulePredicate_
             << std::endl;

  // Check if there are any rules
  if (!ruleDatabase_->hasRuleFor(rulePredicate_)) {
    LOG(WARNING) << "No rules found for predicate: " << rulePredicate_
                 << std::endl;
    // Return empty result
    IdTable emptyTable(getResultWidth(), allocator());
    return {std::move(emptyTable), resultSortedOn(), LocalVocab{}};
  }

  // Run the fixpoint iteration algorithm
  IdTable result = runIterations();

  LOG(DEBUG) << "Fixpoint computation completed. Total rows: " << result.size()
             << std::endl;

  // Return the final result
  return {std::move(result), resultSortedOn(), LocalVocab{}};
}

// _____________________________________________________________________________
IdTable FixpointComputation::runIterations() {
  // Initialize result table
  IdTable allResults(getResultWidth(), allocator());
  size_t iteration = 0;

  LOG(DEBUG) << "Starting fixpoint iterations..." << std::endl;

  while (iteration < maxIterations_) {
    // Check for cancellation
    checkCancellation();

    LOG(DEBUG) << "Iteration " << iteration << "..." << std::endl;

    // Create RuleExpansion for this iteration
    // In a full implementation, we would pass previous iteration results
    // as additional facts for semi-naive evaluation
    auto ruleExpansion = std::make_unique<RuleExpansion>(
        _executionContext, ruleDatabase_, rulePredicate_, arguments_);

    // Create execution tree
    ruleExpansionTree_ = ad_utility::makeExecutionTree<RuleExpansion>(
        _executionContext, std::move(ruleExpansion));

    // Execute the rule expansion
    std::shared_ptr<const Result> iterationResult =
        ruleExpansionTree_->getResult();

    // Get the IdTable from the result
    const IdTable& newFacts = iterationResult->idTable();

    LOG(DEBUG) << "Iteration " << iteration << " produced " << newFacts.size()
               << " facts" << std::endl;

    // If first iteration, initialize results
    if (iteration == 0) {
      if (newFacts.size() > 0) {
        allResults = newFacts.clone();
        logIterationStats(iteration, newFacts.size(), allResults.size());
      } else {
        // No base facts, return empty
        LOG(DEBUG) << "No base facts found, terminating" << std::endl;
        break;
      }
    } else {
      // Merge new facts with existing results
      size_t sizeBefore = allResults.size();
      size_t newRowsAdded = mergeAndDeduplicate(allResults, newFacts);

      logIterationStats(iteration, newRowsAdded, allResults.size());

      // Check if we've reached a fixpoint (no new facts)
      if (!hasNewFacts(sizeBefore, allResults.size())) {
        LOG(DEBUG) << "Fixpoint reached at iteration " << iteration
                   << std::endl;
        break;
      }
    }

    // Check memory constraints
    checkMemoryLimit(allResults.size() * getResultWidth());

    ++iteration;
  }

  if (iteration >= maxIterations_) {
    LOG(WARNING) << "Reached maximum iteration limit (" << maxIterations_
                 << ") without reaching fixpoint" << std::endl;
  }

  return allResults;
}

// _____________________________________________________________________________
size_t FixpointComputation::mergeAndDeduplicate(IdTable& table1,
                                                const IdTable& table2) {
  // Check that tables have the same width
  AD_CONTRACT_CHECK(table1.numColumns() == table2.numColumns());

  if (table2.size() == 0) {
    return 0;
  }

  size_t originalSize = table1.size();

  // For deduplication, we use a set to track unique rows
  // In a production implementation, this could be optimized using
  // sorted merge or hash-based approaches

  // Helper to create a hash of a row for deduplication
  auto rowToHash = [](const auto& row) {
    size_t hash = 0;
    for (const auto& id : row) {
      // Combine hashes using a simple algorithm
      hash ^= std::hash<Id>{}(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  };

  // Build set of existing rows for fast lookup
  std::unordered_set<size_t> existingRows;
  existingRows.reserve(table1.size());
  for (const auto& row : table1) {
    existingRows.insert(rowToHash(row));
  }

  // Add new rows that don't exist yet
  std::vector<IdTable::row_type> newRows;
  for (const auto& row : table2) {
    size_t hash = rowToHash(row);
    if (existingRows.find(hash) == existingRows.end()) {
      // New row, add it
      IdTable::row_type rowCopy(row.size());
      std::copy(row.begin(), row.end(), rowCopy.begin());
      newRows.push_back(std::move(rowCopy));
      existingRows.insert(hash);
    }
  }

  // Append new rows to table1
  for (auto& row : newRows) {
    table1.push_back(row);
  }

  size_t newRowsAdded = table1.size() - originalSize;
  return newRowsAdded;
}

// _____________________________________________________________________________
bool FixpointComputation::hasNewFacts(size_t currentSize,
                                      size_t newSize) const {
  return newSize > currentSize;
}

// _____________________________________________________________________________
void FixpointComputation::checkMemoryLimit(size_t currentSize) {
  // Get the allocator's memory limit
  const auto& alloc = allocator();

  // Calculate approximate memory usage
  // Each Id is typically 8 bytes
  size_t estimatedMemory = currentSize * sizeof(Id);

  // Check against allocator limit (if available)
  // In a full implementation, we would check:
  // if (alloc.amountUsed() + estimatedMemory > alloc.amountAllocated()) {
  //   AD_THROW("Memory limit exceeded during fixpoint computation");
  // }

  // For now, just log memory usage
  if (estimatedMemory > 100'000'000) {  // 100MB threshold for logging
    LOG(INFO) << "Fixpoint computation using approximately "
              << (estimatedMemory / 1'000'000) << " MB" << std::endl;
  }

  // Ignore the alloc variable to avoid unused variable warning
  (void)alloc;
}

// _____________________________________________________________________________
void FixpointComputation::logIterationStats(size_t iteration, size_t newRows,
                                            size_t totalRows) const {
  LOG(DEBUG) << "Iteration " << iteration << ": Added " << newRows
             << " new rows, total: " << totalRows << std::endl;
}

// _____________________________________________________________________________
std::string FixpointComputation::getCacheKeyImpl() const {
  std::ostringstream os;
  os << "FIXPOINT_COMPUTATION " << rulePredicate_ << "(";
  for (size_t i = 0; i < arguments_.size(); ++i) {
    if (i > 0) os << ",";
    os << arguments_[i].toRdfLiteral();
  }
  os << ") max_iter=" << maxIterations_;
  return os.str();
}

// _____________________________________________________________________________
std::unique_ptr<Operation> FixpointComputation::cloneImpl() const {
  return std::make_unique<FixpointComputation>(
      _executionContext, ruleDatabase_, rulePredicate_, arguments_,
      maxIterations_);
}

// _____________________________________________________________________________
VariableToColumnMap FixpointComputation::computeVariableToColumnMap() const {
  VariableToColumnMap result;

  // Map each variable argument to its column index
  size_t columnIndex = 0;
  for (const auto& arg : arguments_) {
    if (arg.isVariable()) {
      Variable var = arg.getVariable();
      result[var] = ColumnIndexAndTypeInfo{columnIndex};
      ++columnIndex;
    }
  }

  return result;
}

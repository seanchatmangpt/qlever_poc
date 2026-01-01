// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: AI Assistant (Claude Code)

#include "shex/ShapeValidationOperation.h"

#include "engine/CallFixedSize.h"
#include "engine/QueryExecutionTree.h"
#include "util/Exception.h"
#include "util/Log.h"

namespace shex {

// ____________________________________________________________________________
ShapeValidationOperation::ShapeValidationOperation(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> subtree,
    Variable targetVariable, Iri shapeId, ValidationConfig config)
    : Operation(qec),
      subtree_(std::move(subtree)),
      targetVariable_(std::move(targetVariable)),
      shapeId_(std::move(shapeId)),
      config_(std::move(config)) {
  // Get shape schema manager and retrieve optimization hints
  auto* manager = getShapeSchemaManager();
  if (manager && config_.enableOptimization) {
    auto hintsOpt = manager->getOptimizationHints(shapeId_);
    if (hintsOpt.has_value()) {
      hints_ = hintsOpt.value();
      manager->recordOptimizationHint();
    }

    // Cache the shape expression for faster validation
    auto shapeOpt = manager->getShape(shapeId_);
    if (shapeOpt.has_value()) {
      cachedShape_ = shapeOpt.value();
    }
  }
}

// ____________________________________________________________________________
std::string ShapeValidationOperation::getCacheKeyImpl() const {
  std::ostringstream os;
  os << "SHAPE_VALIDATE " << subtree_->getRootOperation()->getCacheKey()
     << " var:" << targetVariable_.name()
     << " shape:" << shapeId_.toStringRepresentation()
     << " mode:" << static_cast<int>(config_.mode);
  return std::move(os).str();
}

// ____________________________________________________________________________
std::string ShapeValidationOperation::getDescriptor() const {
  std::ostringstream os;
  os << "ShapeValidation(" << targetVariable_.name() << " @ "
     << shapeId_.toStringRepresentation() << ")";
  return std::move(os).str();
}

// ____________________________________________________________________________
size_t ShapeValidationOperation::getResultWidth() const {
  size_t baseWidth = subtree_->getResultWidth();
  // REPORT mode adds an additional validation result column
  if (config_.mode == ValidationConfig::Mode::REPORT) {
    return baseWidth + 1;
  }
  return baseWidth;
}

// ____________________________________________________________________________
size_t ShapeValidationOperation::getCostEstimate() {
  size_t baseCost = subtree_->getCostEstimate();

  // Add validation overhead
  // STRICT mode: minimal overhead (fail-fast)
  // LAX mode: moderate overhead (filter rows)
  // REPORT mode: higher overhead (compute validation for all rows)
  double validationOverhead = 1.05;  // 5% base overhead
  if (config_.mode == ValidationConfig::Mode::LAX) {
    validationOverhead = 1.15;  // 15% overhead for filtering
  } else if (config_.mode == ValidationConfig::Mode::REPORT) {
    validationOverhead = 1.25;  // 25% overhead for reporting
  }

  return static_cast<size_t>(baseCost * validationOverhead);
}

// ____________________________________________________________________________
uint64_t ShapeValidationOperation::getSizeEstimateBeforeLimit() {
  uint64_t baseSize = subtree_->getSizeEstimate();

  // LAX mode filters out invalid rows, reducing result size
  if (config_.mode == ValidationConfig::Mode::LAX && hints_.selectivityFactor < 1.0) {
    return static_cast<uint64_t>(baseSize * hints_.selectivityFactor);
  }

  return baseSize;
}

// ____________________________________________________________________________
float ShapeValidationOperation::getMultiplicity(size_t col) {
  // Pass through to subtree for existing columns
  if (col < subtree_->getResultWidth()) {
    return subtree_->getMultiplicity(col);
  }

  // For REPORT mode's validation column
  if (config_.mode == ValidationConfig::Mode::REPORT &&
      col == subtree_->getResultWidth()) {
    return 1.0f;  // Validation result is unique per row
  }

  return 1.0f;
}

// ____________________________________________________________________________
bool ShapeValidationOperation::knownEmptyResult() {
  return subtree_->knownEmptyResult();
}

// ____________________________________________________________________________
std::vector<ColumnIndex> ShapeValidationOperation::resultSortedOn() const {
  // Preserve sorting from subtree (validation doesn't change order)
  return subtree_->getRootOperation()->getResultSortedOn();
}

// ____________________________________________________________________________
Result ShapeValidationOperation::computeResult(
    [[maybe_unused]] bool requestLaziness) {
  // Get the input result
  auto inputResult = subtree_->getResult();

  // Validate based on mode
  switch (config_.mode) {
    case ValidationConfig::Mode::STRICT:
      return applyStrictValidation(std::move(inputResult));
    case ValidationConfig::Mode::LAX:
      return applyLaxValidation(std::move(inputResult));
    case ValidationConfig::Mode::REPORT:
      return applyReportValidation(std::move(inputResult));
  }

  AD_FAIL();  // Unreachable
}

// ____________________________________________________________________________
VariableToColumnMap ShapeValidationOperation::computeVariableToColumnMap()
    const {
  VariableToColumnMap varMap = subtree_->getVariableColumns();

  // REPORT mode adds a validation result column
  if (config_.mode == ValidationConfig::Mode::REPORT) {
    Variable reportVar =
        Variable{targetVariable_.name() + "_validation_report"};
    varMap[reportVar] = makeAlwaysDefinedColumn(subtree_->getResultWidth());
  }

  return varMap;
}

// ____________________________________________________________________________
std::unique_ptr<Operation> ShapeValidationOperation::cloneImpl() const {
  return std::make_unique<ShapeValidationOperation>(
      getExecutionContext(), subtree_, targetVariable_, shapeId_, config_);
}

// ____________________________________________________________________________
bool ShapeValidationOperation::validateRow(const IdTable& table,
                                           size_t rowIdx) const {
  if (!cachedShape_) {
    // No shape available, consider valid
    return true;
  }

  // Get the variable column map
  const auto& varCols = subtree_->getVariableColumns();
  auto it = varCols.find(targetVariable_);
  if (it == varCols.end()) {
    // Variable not found in result
    return false;
  }

  // Get the node ID to validate
  size_t colIdx = it->second.columnIndex_;
  Id nodeId = table(rowIdx, colIdx);

  // Check if undefined (undefined values are not validated)
  if (nodeId.isUndefined()) {
    return true;
  }

  // Validate node constraint if present
  if (cachedShape_->nodeConstraint.has_value()) {
    if (!validateNodeConstraint(cachedShape_->nodeConstraint.value(),
                                 nodeId)) {
      return false;
    }
  }

  // Validate triple constraints
  return validateTripleConstraints(cachedShape_->tripleConstraints, nodeId);
}

// ____________________________________________________________________________
Result ShapeValidationOperation::applyStrictValidation(Result inputResult) {
  auto* manager = getShapeSchemaManager();

  // Validate all rows, throw on first failure
  const auto& idTable = inputResult->idTable();
  for (size_t i = 0; i < idTable.size(); ++i) {
    bool isValid = validateRow(idTable, i);

    if (manager) {
      manager->recordValidation(isValid);
    }

    if (!isValid) {
      throw std::runtime_error(
          "Shape validation failed (STRICT mode) for " +
          targetVariable_.name() + " at shape " +
          shapeId_.toStringRepresentation());
    }
  }

  return inputResult;
}

// ____________________________________________________________________________
Result ShapeValidationOperation::applyLaxValidation(Result inputResult) {
  auto* manager = getShapeSchemaManager();

  // Filter out invalid rows
  IdTable resultTable = inputResult->idTable().clone();
  IdTable filteredTable(resultTable.numColumns(), allocator());

  size_t originalSize = resultTable.size();
  size_t validCount = 0;

  for (size_t i = 0; i < resultTable.size(); ++i) {
    bool isValid = validateRow(resultTable, i);

    if (manager) {
      manager->recordValidation(isValid);
    }

    if (isValid) {
      filteredTable.push_back(resultTable[i]);
      validCount++;
    }
  }

  // Record cardinality reduction
  if (manager && originalSize > 0) {
    double reduction = static_cast<double>(validCount) / originalSize;
    manager->recordValidation(true, reduction);
  }

  return {std::move(filteredTable), resultSortedOn(),
          inputResult->getSharedLocalVocab()};
}

// ____________________________________________________________________________
Result ShapeValidationOperation::applyReportValidation(Result inputResult) {
  auto* manager = getShapeSchemaManager();

  // Add validation result column
  IdTable resultTable = inputResult->idTable().clone();
  IdTable outputTable(resultTable.numColumns() + 1, allocator());

  for (size_t i = 0; i < resultTable.size(); ++i) {
    bool isValid = validateRow(resultTable, i);

    if (manager) {
      manager->recordValidation(isValid);
    }

    // Copy original row
    for (size_t col = 0; col < resultTable.numColumns(); ++col) {
      outputTable(i, col) = resultTable(i, col);
    }

    // Add validation result (1 = valid, 0 = invalid)
    outputTable(i, resultTable.numColumns()) =
        Id::makeFromBool(isValid);
  }

  return {std::move(outputTable), resultSortedOn(),
          inputResult->getSharedLocalVocab()};
}

// ____________________________________________________________________________
ShapeSchemaManager* ShapeValidationOperation::getShapeSchemaManager() const {
  // In a real implementation, this would be retrieved from QueryExecutionContext
  // For now, return nullptr (will be integrated when modifying
  // QueryExecutionContext)
  return nullptr;
}

// ____________________________________________________________________________
bool ShapeValidationOperation::validateTripleConstraints(
    const std::vector<TripleConstraint>& constraints, Id nodeId) const {
  // For each triple constraint, check cardinality
  for (const auto& tc : constraints) {
    size_t count = countPredicateOccurrences(nodeId, tc.predicate);

    if (count < tc.minCount || count > tc.maxCount) {
      return false;
    }

    // If there's a value constraint, validate it
    // (Simplified: in production, would need to query the index)
    if (tc.valueConstraint.has_value()) {
      // TODO: Implement full value constraint validation
      // This requires querying the index for object values
    }
  }

  return true;
}

// ____________________________________________________________________________
bool ShapeValidationOperation::validateNodeConstraint(
    const NodeConstraint& constraint, Id nodeId) const {
  // Check node kind (IRI, Literal, BlankNode)
  // Simplified: would need to inspect the actual ID type
  if (!constraint.nodeKinds.empty()) {
    // TODO: Implement node kind checking based on Id type
  }

  // Check datatype
  if (!constraint.datatypes.empty()) {
    // TODO: Implement datatype checking for literals
  }

  // Check value restrictions
  if (!constraint.values.empty()) {
    if (!constraint.values.contains(nodeId)) {
      return false;
    }
  }

  // Check pattern (for literals)
  if (constraint.pattern.has_value()) {
    // TODO: Implement regex pattern matching for literal values
  }

  return true;
}

// ____________________________________________________________________________
size_t ShapeValidationOperation::countPredicateOccurrences(
    Id nodeId, const Iri& predicate) const {
  // This requires querying the index
  // Simplified: return a placeholder value
  // Real implementation would use: getIndex().scan(nodeId, predicate, ?)
  return 1;
}

}  // namespace shex

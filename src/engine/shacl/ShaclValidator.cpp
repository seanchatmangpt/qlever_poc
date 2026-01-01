#include "ShaclValidator.h"
#include "engine/idTable/IdTable.h"
#include "engine/Result.h"

namespace shacl {

ShaclValidator::ShaclValidator(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> subtree,
    const ShaclShapeRegistry* shapeRegistry, ColumnIndex resourceColumnIndex,
    std::optional<std::string> targetShapeId)
    : Operation(qec),
      _subtree(std::move(subtree)),
      _shapeRegistry(shapeRegistry),
      _resourceColumnIndex(resourceColumnIndex),
      _targetShapeId(std::move(targetShapeId)) {}

ShaclValidator::ShaclValidator(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> subtree,
    const ShaclShapeRegistry* shapeRegistry, ColumnIndex resourceColumnIndex,
    std::unordered_map<std::string, ColumnIndex> propertyColumns)
    : Operation(qec),
      _subtree(std::move(subtree)),
      _shapeRegistry(shapeRegistry),
      _resourceColumnIndex(resourceColumnIndex),
      _propertyColumns(std::move(propertyColumns)) {}

std::string ShaclValidator::getCacheKeyImpl() const {
  std::string key = _subtree->getCacheKey();
  key += "|SHACL_VALIDATOR";
  if (_targetShapeId) {
    key += "|shape=" + *_targetShapeId;
  }
  key += "|resourceCol=" + std::to_string(_resourceColumnIndex);
  return key;
}

std::string ShaclValidator::getDescriptor() const {
  std::string desc = "ShaclValidator(";
  if (_targetShapeId) {
    desc += "shape=" + *_targetShapeId;
  } else {
    desc += "allShapes";
  }
  desc += ")";
  return desc;
}

size_t ShaclValidator::getResultWidth() const {
  // Result width is same as input (pass through validation info)
  return _subtree->getResultWidth();
}

uint64_t ShaclValidator::getSizeEstimateBeforeLimit() {
  // Validation doesn't change result size, just marks which conform
  return _subtree->getSizeEstimate();
}

size_t ShaclValidator::getCostEstimate() {
  // Add validation cost on top of subtree
  auto subtreeCost = _subtree->getCostEstimate();
  auto validationCost = _subtree->getSizeEstimate();  // Linear in result size
  return subtreeCost + validationCost;
}

std::unique_ptr<Operation> ShaclValidator::cloneImpl() const {
  return std::make_unique<ShaclValidator>(*this);
}

std::vector<const NodeShape*> ShaclValidator::getApplicableShapes(
    const std::string& resourceId) {
  // Get all shapes (or filtered to _targetShapeId)
  auto allShapes = _shapeRegistry->getAllShapes();
  std::vector<const NodeShape*> applicable;

  for (const auto* shape : allShapes) {
    if (_targetShapeId && shape->shapeId != *_targetShapeId) {
      continue;
    }
    // For 80/20: check target node and all shapes are applicable
    applicable.push_back(shape);
  }

  return applicable;
}

bool ShaclValidator::validatePropertyValue(const PropertyShape& propShape,
                                           const std::string& value) {
  for (const auto& constraint : propShape.constraints) {
    if (!ShaclConstraintEvaluator::evaluateConstraint(constraint, value)) {
      return false;
    }
  }
  return true;
}

ValidationResult ShaclValidator::validateResource(const std::string& resourceId,
                                                  const IdTable& inputTable,
                                                  size_t rowIndex) {
  ValidationResult result;
  result.focusNode = resourceId;

  auto applicableShapes = getApplicableShapes(resourceId);

  // For each applicable shape, validate property shapes
  for (const auto* shape : applicableShapes) {
    for (const auto& propShape : shape->propertyShapes) {
      // Get values for this property from the input table
      std::vector<std::string> values;

      auto it = _propertyColumns.find(propShape.path);
      if (it != _propertyColumns.end()) {
        // Property has a column in the result
        auto colIdx = it->second;
        if (colIdx < inputTable.getWidth()) {
          auto val = inputTable.getEntry(rowIndex, colIdx);
          if (val != 0) {  // 0 is typically NULL/undefined
            values.push_back(std::to_string(val));
          }
        }
      }

      // Validate property constraints
      auto propResult = ShaclConstraintEvaluator::evaluatePropertyShape(
          resourceId, propShape, values);
      if (!propResult.conforms) {
        result.conforms = false;
        for (const auto& violation : propResult.violations) {
          result.violations.push_back(violation);
        }
      }
    }
  }

  return result;
}

Result ShaclValidator::computeResult(bool requestLaziness) {
  // Get the input result from subtree
  auto subtreeResult = _subtree->getResult(false);

  if (!subtreeResult) {
    return Result({}, {});
  }

  // Get the IdTable from result
  const auto& inputTable = subtreeResult->getIdTable();
  const auto& varToCol = subtreeResult->getVariableColumns();

  // Create output table (same structure as input)
  IdTable outputTable(inputTable.getWidth());
  std::vector<std::string> resourceIds;

  // Validate each row
  size_t validationErrors = 0;
  for (size_t rowIdx = 0; rowIdx < inputTable.size(); ++rowIdx) {
    // Get resource ID from resource column
    auto resourceId = std::to_string(inputTable.getEntry(rowIdx, _resourceColumnIndex));

    // Validate the resource
    auto validationResult = validateResource(resourceId, inputTable, rowIdx);

    // Only include conforming resources in output (80/20: filtering approach)
    if (validationResult.conforms) {
      // Copy row to output table
      std::vector<IdTableValue> row;
      for (size_t colIdx = 0; colIdx < inputTable.getWidth(); ++colIdx) {
        row.push_back(inputTable.getEntry(rowIdx, colIdx));
      }
      outputTable.push_back(row);
    } else {
      validationErrors++;
    }
  }

  // Create result with validation info
  auto result = Result(outputTable, varToCol);

  // Add warning if there were validation errors
  if (validationErrors > 0) {
    addWarning("SHACL validation: " + std::to_string(validationErrors) +
               " resources did not conform to shapes");
  }

  return result;
}

}  // namespace shacl

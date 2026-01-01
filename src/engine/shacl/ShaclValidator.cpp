#include "ShaclValidator.h"
#include "LogicalShapes.h"
#include "engine/idTable/IdTable.h"
#include "engine/Result.h"
#include "util/TaskQueue.h"
#include <thread>

namespace shacl {

ShaclValidator::ShaclValidator(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> subtree,
    const ShaclShapeRegistry* shapeRegistry, ColumnIndex resourceColumnIndex,
    std::optional<std::string> targetShapeId,
    std::shared_ptr<ShaclValidationCache> cache,
    bool enableParallelValidation,
    size_t parallelThreads)
    : Operation(qec),
      _subtree(std::move(subtree)),
      _shapeRegistry(shapeRegistry),
      _resourceColumnIndex(resourceColumnIndex),
      _targetShapeId(std::move(targetShapeId)),
      _cache(std::move(cache)),
      _enableParallelValidation(enableParallelValidation),
      _parallelThreads(parallelThreads) {
  // Create default cache if not provided
  if (!_cache) {
    _cache = std::make_shared<ShaclValidationCache>();
  }
}

ShaclValidator::ShaclValidator(
    QueryExecutionContext* qec, std::shared_ptr<QueryExecutionTree> subtree,
    const ShaclShapeRegistry* shapeRegistry, ColumnIndex resourceColumnIndex,
    std::unordered_map<std::string, ColumnIndex> propertyColumns,
    std::shared_ptr<ShaclValidationCache> cache,
    bool enableParallelValidation,
    size_t parallelThreads)
    : Operation(qec),
      _subtree(std::move(subtree)),
      _shapeRegistry(shapeRegistry),
      _resourceColumnIndex(resourceColumnIndex),
      _propertyColumns(std::move(propertyColumns)),
      _cache(std::move(cache)),
      _enableParallelValidation(enableParallelValidation),
      _parallelThreads(parallelThreads) {
  // Create default cache if not provided
  if (!_cache) {
    _cache = std::make_shared<ShaclValidationCache>();
  }
}

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

  // Validate against each applicable shape (with caching)
  for (const auto* shape : applicableShapes) {
    auto shapeResult = validateResourceWithShape(resourceId, shape,
                                                 inputTable, rowIndex);
    if (!shapeResult.conforms) {
      result.conforms = false;
      for (const auto& violation : shapeResult.violations) {
        result.violations.push_back(violation);
      }
    }
  }

  return result;
}

ValidationResult ShaclValidator::validateResourceWithShape(
    const std::string& resourceId, const NodeShape* shape,
    const IdTable& inputTable, size_t rowIndex) {

  // Try to get from cache first
  auto cacheKey = makeValidationKey(resourceId, shape->shapeId);

  // Check bloom filter for quick negative lookup
  if (_cache->isBloomFilterEnabled() &&
      !_cache->mightBeCached(resourceId, shape->shapeId)) {
    // Not in cache, compute validation
    auto result = computeValidationForShape(resourceId, shape, inputTable, rowIndex);

    // Cache the result if it conforms
    if (result.conforms) {
      _cache->addToBloomFilter(resourceId, shape->shapeId);
    }

    return result;
  }

  // Use cache with computation fallback
  auto computeValidation = [&]() -> ValidationResult {
    return computeValidationForShape(resourceId, shape, inputTable, rowIndex);
  };

  auto result = _cache->getOrComputeValidationResult(cacheKey, computeValidation);

  // Add to bloom filter on successful validation
  if (result.conforms && _cache->isBloomFilterEnabled()) {
    _cache->addToBloomFilter(resourceId, shape->shapeId);
  }

  return result;
}

ValidationResult ShaclValidator::computeValidationForShape(
    const std::string& resourceId, const NodeShape* shape,
    const IdTable& inputTable, size_t rowIndex) {

  ValidationResult result;
  result.focusNode = resourceId;

  // Get or compile the shape for optimized validation
  const auto& compiledShape = _cache->getOrCompileShape(
      shape->shapeId,
      [shape]() { return CompiledShape::compile(*shape); });

  // Validate logical constraints first (sh:and, sh:or, sh:not, sh:xone)
  if (shape->hasLogicalConstraints()) {
    // Evaluate sh:and constraint
    if (shape->andConstraint) {
      auto andResult = shape->andConstraint->evaluate(
          resourceId, inputTable, rowIndex, _shapeRegistry, _propertyColumns);
      if (!andResult.conforms) {
        result.conforms = false;
        for (const auto& violation : andResult.violations) {
          result.violations.push_back(violation);
        }
      }
    }

    // Evaluate sh:or constraint
    if (shape->orConstraint) {
      auto orResult = shape->orConstraint->evaluate(
          resourceId, inputTable, rowIndex, _shapeRegistry, _propertyColumns);
      if (!orResult.conforms) {
        result.conforms = false;
        for (const auto& violation : orResult.violations) {
          result.violations.push_back(violation);
        }
      }
    }

    // Evaluate sh:not constraint
    if (shape->notConstraint) {
      auto notResult = shape->notConstraint->evaluate(
          resourceId, inputTable, rowIndex, _shapeRegistry, _propertyColumns);
      if (!notResult.conforms) {
        result.conforms = false;
        for (const auto& violation : notResult.violations) {
          result.violations.push_back(violation);
        }
      }
    }

    // Evaluate sh:xone constraint
    if (shape->xoneConstraint) {
      auto xoneResult = shape->xoneConstraint->evaluate(
          resourceId, inputTable, rowIndex, _shapeRegistry, _propertyColumns);
      if (!xoneResult.conforms) {
        result.conforms = false;
        for (const auto& violation : xoneResult.violations) {
          result.violations.push_back(violation);
        }
      }
    }
  }

  // Validate property shapes
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

    // Validate property constraints (with caching)
    auto propResult = ShaclConstraintEvaluator::evaluatePropertyShape(
        resourceId, propShape, values, _cache.get());
    if (!propResult.conforms) {
      result.conforms = false;
      for (const auto& violation : propResult.violations) {
        result.violations.push_back(violation);
      }
    }
  }

  return result;
}

std::vector<ValidationResult> ShaclValidator::validateResourcesParallel(
    const std::vector<std::string>& resourceIds,
    const IdTable& inputTable,
    const std::vector<size_t>& rowIndices) {

  std::vector<ValidationResult> results(resourceIds.size());

  // Determine number of threads
  size_t numThreads = _parallelThreads;
  if (numThreads == 0) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // Fallback
  }

  // Use TaskQueue for parallel validation
  ad_utility::TaskQueue<> taskQueue(numThreads, numThreads);

  for (size_t i = 0; i < resourceIds.size(); ++i) {
    taskQueue.push([&, i]() {
      results[i] = validateResource(resourceIds[i], inputTable, rowIndices[i]);
    });
  }

  taskQueue.finish();
  return results;
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

  // Decide whether to use parallel validation
  const size_t PARALLEL_THRESHOLD = 100;  // Use parallel for >100 rows
  bool useParallel = _enableParallelValidation &&
                     inputTable.size() > PARALLEL_THRESHOLD;

  size_t validationErrors = 0;

  if (useParallel) {
    // Parallel validation path
    std::vector<std::string> resourceIds;
    std::vector<size_t> rowIndices;

    resourceIds.reserve(inputTable.size());
    rowIndices.reserve(inputTable.size());

    for (size_t rowIdx = 0; rowIdx < inputTable.size(); ++rowIdx) {
      auto resourceId = std::to_string(
          inputTable.getEntry(rowIdx, _resourceColumnIndex));
      resourceIds.push_back(resourceId);
      rowIndices.push_back(rowIdx);
    }

    auto validationResults = validateResourcesParallel(
        resourceIds, inputTable, rowIndices);

    // Process results
    for (size_t i = 0; i < validationResults.size(); ++i) {
      if (validationResults[i].conforms) {
        std::vector<IdTableValue> row;
        for (size_t colIdx = 0; colIdx < inputTable.getWidth(); ++colIdx) {
          row.push_back(inputTable.getEntry(rowIndices[i], colIdx));
        }
        outputTable.push_back(row);
      } else {
        validationErrors++;
      }
    }
  } else {
    // Sequential validation path
    for (size_t rowIdx = 0; rowIdx < inputTable.size(); ++rowIdx) {
      // Get resource ID from resource column
      auto resourceId = std::to_string(
          inputTable.getEntry(rowIdx, _resourceColumnIndex));

      // Validate the resource
      auto validationResult = validateResource(resourceId, inputTable, rowIdx);

      // Only include conforming resources in output
      if (validationResult.conforms) {
        std::vector<IdTableValue> row;
        for (size_t colIdx = 0; colIdx < inputTable.getWidth(); ++colIdx) {
          row.push_back(inputTable.getEntry(rowIdx, colIdx));
        }
        outputTable.push_back(row);
      } else {
        validationErrors++;
      }
    }
  }

  // Create result with validation info
  auto result = Result(outputTable, varToCol);

  // Add warning if there were validation errors
  if (validationErrors > 0) {
    addWarning("SHACL validation: " + std::to_string(validationErrors) +
               " resources did not conform to shapes");
  }

  // Add cache statistics as info
  if (_cache) {
    addInfo(getCacheStatistics());
  }

  return result;
}

std::string ShaclValidator::getCacheStatistics() const {
  if (_cache) {
    return _cache->getMetrics().toString();
  }
  return "Cache not available";
}

void ShaclValidator::clearCache() {
  if (_cache) {
    _cache->clearAll();
  }
}

std::shared_ptr<ShaclValidationCache> ShaclValidator::getCache() {
  if (!_cache) {
    _cache = std::make_shared<ShaclValidationCache>();
  }
  return _cache;
}

// Detailed violation reporting implementations
DetailedValidationReport ShaclValidator::validateResourceDetailed(
    const std::string& resourceId, const IdTable& inputTable,
    size_t rowIndex) {
  DetailedValidationReport report;

  auto applicableShapes = getApplicableShapes(resourceId);

  // For each applicable shape, validate property shapes with detailed violations
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

      // Evaluate property shape and collect detailed violations
      auto violations = ShaclConstraintEvaluator::evaluatePropertyShapeDetailed(
          resourceId, propShape, values, shape->shapeId);

      // Add all violations to the report
      for (auto& violation : violations) {
        report.addViolation(std::move(violation));
      }
    }
  }

  return report;
}

DetailedValidationReport ShaclValidator::validateAllResourcesDetailed(
    const IdTable& inputTable) {
  DetailedValidationReport report;

  // Validate each row
  for (size_t rowIdx = 0; rowIdx < inputTable.size(); ++rowIdx) {
    // Get resource ID from resource column
    auto resourceId =
        std::to_string(inputTable.getEntry(rowIdx, _resourceColumnIndex));

    // Validate the resource and collect violations
    auto resourceReport = validateResourceDetailed(resourceId, inputTable, rowIdx);

    // Merge violations into main report
    for (auto& violation : resourceReport.violations) {
      report.addViolation(std::move(violation));
    }
  }

  return report;
}

std::string ShaclValidator::getValidationReport(
    const DetailedValidationReport& report, ViolationFormat format) {
  return ViolationFormatter::format(report, format);
}

}  // namespace shacl

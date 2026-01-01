#ifndef QLEVER_ENGINE_SHACL_SHACLVALIDATOR_H
#define QLEVER_ENGINE_SHACL_SHACLVALIDATOR_H

#include "engine/Operation.h"
#include "engine/QueryExecutionTree.h"
#include "ShaclShape.h"
#include "ShaclShapeRegistry.h"
#include "ShaclConstraintEvaluator.h"
#include <memory>
#include <vector>

namespace shacl {

// SHACL Validator operation - validates RDF resources against SHACL shapes
// This operation takes a subtree producing resources to validate and checks
// them against registered SHACL shapes.
class ShaclValidator : public Operation {
 private:
  // The operation producing the resources to validate
  std::shared_ptr<QueryExecutionTree> _subtree;

  // Reference to the shape registry (non-owning)
  const ShaclShapeRegistry* _shapeRegistry;

  // The column index of the subject/resource being validated
  ColumnIndex _resourceColumnIndex;

  // The shape ID to validate against (if validating specific shape)
  std::optional<std::string> _targetShapeId;

  // Property column mapping (property IRI -> column index)
  std::unordered_map<std::string, ColumnIndex> _propertyColumns;

 public:
  // Constructor: validate resources in subtree against shapes
  ShaclValidator(QueryExecutionContext* qec,
                 std::shared_ptr<QueryExecutionTree> subtree,
                 const ShaclShapeRegistry* shapeRegistry,
                 ColumnIndex resourceColumnIndex = 0,
                 std::optional<std::string> targetShapeId = std::nullopt);

  // Constructor with property column mapping for property shape validation
  ShaclValidator(
      QueryExecutionContext* qec,
      std::shared_ptr<QueryExecutionTree> subtree,
      const ShaclShapeRegistry* shapeRegistry, ColumnIndex resourceColumnIndex,
      std::unordered_map<std::string, ColumnIndex> propertyColumns);

 private:
  std::string getCacheKeyImpl() const override;

 public:
  std::string getDescriptor() const override;

  std::vector<ColumnIndex> resultSortedOn() const override {
    return _subtree->resultSortedOn();
  }

  size_t getResultWidth() const override;

 private:
  uint64_t getSizeEstimateBeforeLimit() override;

 public:
  size_t getCostEstimate() override;

  std::vector<QueryExecutionTree*> getChildren() override {
    return {_subtree.get()};
  }

  bool knownEmptyResult() override { return _subtree->knownEmptyResult(); }

  float getMultiplicity(size_t col) override {
    return _subtree->getMultiplicity(col);
  }

 private:
  std::unique_ptr<Operation> cloneImpl() const override;

  VariableToColumnMap computeVariableToColumnMap() const override {
    return _subtree->getVariableColumns();
  }

  Result computeResult(bool requestLaziness) override;

  // Validate a resource against applicable shapes
  ValidationResult validateResource(const std::string& resourceId,
                                    const IdTable& inputTable,
                                    size_t rowIndex);

  // Validate a single property value against property shape constraints
  bool validatePropertyValue(const PropertyShape& propShape,
                             const std::string& value);

  // Get applicable shapes for a resource
  std::vector<const NodeShape*> getApplicableShapes(
      const std::string& resourceId);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLVALIDATOR_H

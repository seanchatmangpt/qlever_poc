#ifndef QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H
#define QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H

#include "ShaclShape.h"
#include "ShaclViolation.h"
#include "AdvancedConstraints.h"
#include "SparqlBasedConstraint.h"
#include <vector>
#include <string>
#include <regex>
#include <unordered_map>

// Forward declarations
class QueryExecutionContext;

namespace shacl {

// Forward declaration for cache
class ShaclValidationCache;

// Evaluates SHACL constraints against RDF values
class ShaclConstraintEvaluator {
 public:
  // Check if a value satisfies a constraint (with optional caching)
  static bool evaluateConstraint(const ShaclConstraint& constraint,
                                  const std::string& value,
                                  const std::string& datatype = "",
                                  ShaclValidationCache* cache = nullptr);

  // Check if all values in a property satisfy constraints (with optional caching)
  static ValidationResult evaluatePropertyShape(
      const std::string& nodeId, const PropertyShape& propShape,
      const std::vector<std::string>& values,
      ShaclValidationCache* cache = nullptr);

  // Check if all values in a property satisfy constraints (with context and caching)
  static ValidationResult evaluatePropertyShapeWithContext(
      const std::string& nodeId, const PropertyShape& propShape,
      const std::vector<std::string>& values,
      AdvancedConstraintContext* context,
      ShaclValidationCache* cache = nullptr);

  // Evaluate property shape and generate detailed violations
  static std::vector<ShaclViolation> evaluatePropertyShapeDetailed(
      const std::string& nodeId, const PropertyShape& propShape,
      const std::vector<std::string>& values, const std::string& shapeId);

  // Evaluate property shape with context and generate detailed violations
  static std::vector<ShaclViolation> evaluatePropertyShapeDetailedWithContext(
      const std::string& nodeId, const PropertyShape& propShape,
      const std::vector<std::string>& values, const std::string& shapeId,
      AdvancedConstraintContext* context);

  // Check if a value matches a datatype (with optional caching)
  static bool matchesDatatype(const std::string& value,
                              const std::string& datatype,
                              ShaclValidationCache* cache = nullptr);

  // Get the datatype of an RDF value (with optional caching)
  static std::string getDatatype(const std::string& value,
                                 ShaclValidationCache* cache = nullptr);

  // Check if a value is a valid IRI
  static bool isValidIri(const std::string& value);

  // Check if a value is a blank node
  static bool isBlankNode(const std::string& value);

  // Check if a value is a literal
  static bool isLiteral(const std::string& value);

  // Evaluate SPARQL-based constraint
  static bool evaluateSparqlConstraint(
      const std::string& sparqlQuery,
      const std::string& focusNode,
      const std::unordered_map<std::string, std::string>& bindings,
      QueryExecutionContext* context);

  // Evaluate SPARQL-based constraint using SparqlBasedConstraint
  static std::vector<std::string> evaluateSparqlConstraintWithResult(
      const SparqlBasedConstraint& constraint,
      const std::string& focusNode,
      const std::unordered_map<std::string, std::string>& bindings,
      QueryExecutionContext* context);

 private:
  // Constraint evaluation helpers
  static bool evaluateMinCount(int minCount,
                               const std::vector<std::string>& values);
  static bool evaluateMaxCount(int maxCount,
                               const std::vector<std::string>& values);
  static bool evaluateMinLength(int minLength, const std::string& value);
  static bool evaluateMaxLength(int maxLength, const std::string& value);
  static bool evaluatePattern(const std::string& pattern,
                              const std::string& value);
  static bool evaluateInList(const std::vector<std::string>& allowedValues,
                             const std::string& value);
  static bool evaluateMinInclusive(const std::string& minValue,
                                   const std::string& value);
  static bool evaluateMaxInclusive(const std::string& maxValue,
                                   const std::string& value);
  static bool evaluateNodeKind(NodeKind kind, const std::string& value);

  // Advanced constraint evaluators
  static bool evaluateUnique(const UniqueConstraintValue& constraint,
                             const std::vector<std::string>& values,
                             AdvancedConstraintContext* context);
  static bool evaluateDisjointWith(const DisjointWithConstraintValue& constraint,
                                   const std::vector<std::string>& values1,
                                   const std::vector<std::string>& values2);
  static bool evaluateClosed(const ClosedConstraintValue& constraint,
                             const std::unordered_set<std::string>& properties);
  static bool evaluateHasValue(const HasValueConstraintValue& constraint,
                               const std::vector<std::string>& values);
  static bool evaluateMinExclusive(const MinExclusiveConstraintValue& constraint,
                                   const std::string& value);
  static bool evaluateMaxExclusive(const MaxExclusiveConstraintValue& constraint,
                                   const std::string& value);

  // Helper to extract literal string from RDF literal (remove quotes/language
  // tag)
  static std::string extractLiteralValue(const std::string& rdfLiteral);

  // Helper to extract datatype from RDF literal
  static std::string extractLiteralDatatype(const std::string& rdfLiteral);

  // Helper to parse numeric value from string
  static double parseNumericValue(const std::string& value);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H

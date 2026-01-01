#ifndef QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H
#define QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H

#include "ShaclShape.h"
#include <vector>
#include <string>
#include <regex>

namespace shacl {

// Evaluates SHACL constraints against RDF values
class ShaclConstraintEvaluator {
 public:
  // Check if a value satisfies a constraint
  static bool evaluateConstraint(const ShaclConstraint& constraint,
                                  const std::string& value,
                                  const std::string& datatype = "");

  // Check if all values in a property satisfy constraints
  static ValidationResult evaluatePropertyShape(
      const std::string& nodeId, const PropertyShape& propShape,
      const std::vector<std::string>& values);

  // Check if a value matches a datatype
  static bool matchesDatatype(const std::string& value,
                              const std::string& datatype);

  // Get the datatype of an RDF value
  static std::string getDatatype(const std::string& value);

  // Check if a value is a valid IRI
  static bool isValidIri(const std::string& value);

  // Check if a value is a blank node
  static bool isBlankNode(const std::string& value);

  // Check if a value is a literal
  static bool isLiteral(const std::string& value);

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

  // Helper to extract literal string from RDF literal (remove quotes/language
  // tag)
  static std::string extractLiteralValue(const std::string& rdfLiteral);

  // Helper to extract datatype from RDF literal
  static std::string extractLiteralDatatype(const std::string& rdfLiteral);
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SHACLCONSTRAINTEVALUATOR_H

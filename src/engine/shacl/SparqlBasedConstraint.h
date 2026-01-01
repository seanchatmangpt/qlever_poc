#ifndef QLEVER_ENGINE_SHACL_SPARQLBASEDCONSTRAINT_H
#define QLEVER_ENGINE_SHACL_SPARQLBASEDCONSTRAINT_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShaclShape.h"

// Forward declarations
class QueryExecutionContext;
class ParsedQuery;
class QueryExecutionTree;

namespace shacl {

// Represents a SPARQL-based constraint (sh:sparql)
// SHACL allows expressing constraints as SPARQL queries
// The query is executed with bound variables (?this, ?focusNode, ?value)
// and violations are detected based on query results
class SparqlBasedConstraint {
 public:
  // Construct from SPARQL query string
  explicit SparqlBasedConstraint(const std::string& sparqlQuery);

  // Get the SPARQL query string
  const std::string& getSparqlQuery() const { return sparqlQuery_; }

  // Set custom message for violations
  void setMessage(const std::string& message) { message_ = message; }
  const std::string& getMessage() const { return message_; }

  // Set severity level
  void setSeverity(SeverityLevel severity) { severity_ = severity; }
  SeverityLevel getSeverity() const { return severity_; }

  // Check if the constraint is valid (query can be parsed)
  bool isValid() const { return isValid_; }

  // Get validation error if constraint is invalid
  const std::string& getValidationError() const { return validationError_; }

  // Evaluate constraint for a focus node
  // Returns list of violations (empty if conforms)
  // bindings: variable bindings for ?this, ?focusNode, ?value
  std::vector<std::string> evaluate(
      const std::string& focusNode,
      const std::unordered_map<std::string, std::string>& bindings,
      QueryExecutionContext* context) const;

  // Parse and validate the SPARQL query
  // Returns true if query is valid, false otherwise
  bool validateQuery();

  // Variable names that can be bound in SPARQL constraint queries
  static constexpr const char* THIS_VAR = "?this";
  static constexpr const char* FOCUS_NODE_VAR = "?focusNode";
  static constexpr const char* VALUE_VAR = "?value";
  static constexpr const char* PATH_VAR = "?path";

 private:
  std::string sparqlQuery_;
  std::string message_;
  SeverityLevel severity_ = SeverityLevel::Violation;
  bool isValid_ = false;
  std::string validationError_;

  // Parsed query representation (cached for performance)
  mutable std::shared_ptr<ParsedQuery> parsedQuery_;

  // Helper to bind variables in the query
  std::string bindVariables(
      const std::unordered_map<std::string, std::string>& bindings) const;

  // Helper to execute the bound query
  bool executeQuery(const std::string& boundQuery,
                    QueryExecutionContext* context) const;

  // Extract violation messages from query results
  std::vector<std::string> extractViolations(
      const std::string& boundQuery, QueryExecutionContext* context) const;
};

// Extended SHACL constraint that can hold SPARQL-based constraints
struct SparqlConstraint {
  SparqlBasedConstraint sparqlConstraint;
  SeverityLevel severity = SeverityLevel::Violation;
  std::string message;

  explicit SparqlConstraint(const std::string& sparqlQuery)
      : sparqlConstraint(sparqlQuery) {}
};

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_SPARQLBASEDCONSTRAINT_H

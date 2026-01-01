#include "SparqlBasedConstraint.h"

#include <sstream>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"
#include "util/Exception.h"

namespace shacl {

SparqlBasedConstraint::SparqlBasedConstraint(const std::string& sparqlQuery)
    : sparqlQuery_(sparqlQuery),
      message_("SPARQL constraint violation"),
      isValid_(false) {
  // Initialize with empty parsed query
  parsedQuery_ = nullptr;
}

bool SparqlBasedConstraint::validateQuery() {
  try {
    // Check if query contains expected variables
    if (sparqlQuery_.empty()) {
      validationError_ = "SPARQL constraint query cannot be empty";
      isValid_ = false;
      return false;
    }

    // Basic validation: check for SELECT query structure
    // A SHACL SPARQL constraint should be a SELECT query or ASK query
    std::string queryUpper = sparqlQuery_;
    std::transform(queryUpper.begin(), queryUpper.end(), queryUpper.begin(),
                   ::toupper);

    if (queryUpper.find("SELECT") == std::string::npos &&
        queryUpper.find("ASK") == std::string::npos) {
      validationError_ =
          "SPARQL constraint must be a SELECT or ASK query, got: " +
          sparqlQuery_.substr(0, 50) + "...";
      isValid_ = false;
      return false;
    }

    // Mark as valid for now
    // Full parsing will happen during execution with proper context
    isValid_ = true;
    validationError_.clear();
    return true;

  } catch (const std::exception& e) {
    validationError_ = std::string("Failed to validate SPARQL query: ") + e.what();
    isValid_ = false;
    return false;
  }
}

std::string SparqlBasedConstraint::bindVariables(
    const std::unordered_map<std::string, std::string>& bindings) const {
  std::string boundQuery = sparqlQuery_;

  // Replace each binding in the query
  // Note: This is a simplified approach. In production, we'd use
  // proper SPARQL query manipulation
  for (const auto& [varName, value] : bindings) {
    // Handle different variable formats: ?var and $var
    std::string varPattern1 = "?" + varName;
    std::string varPattern2 = "$" + varName;

    // Quote the value if it's an IRI or literal
    std::string quotedValue = value;
    if (!value.empty() && value[0] != '<' && value[0] != '"' &&
        value.find("^^") == std::string::npos) {
      // Add IRI brackets if not already present
      quotedValue = "<" + value + ">";
    }

    // Replace all occurrences
    size_t pos = 0;
    while ((pos = boundQuery.find(varPattern1, pos)) != std::string::npos) {
      // Check if it's a standalone variable (not part of a longer word)
      if ((pos == 0 || !std::isalnum(boundQuery[pos - 1])) &&
          (pos + varPattern1.length() >= boundQuery.length() ||
           !std::isalnum(boundQuery[pos + varPattern1.length()]))) {
        boundQuery.replace(pos, varPattern1.length(), quotedValue);
        pos += quotedValue.length();
      } else {
        pos += varPattern1.length();
      }
    }

    pos = 0;
    while ((pos = boundQuery.find(varPattern2, pos)) != std::string::npos) {
      if ((pos == 0 || !std::isalnum(boundQuery[pos - 1])) &&
          (pos + varPattern2.length() >= boundQuery.length() ||
           !std::isalnum(boundQuery[pos + varPattern2.length()]))) {
        boundQuery.replace(pos, varPattern2.length(), quotedValue);
        pos += quotedValue.length();
      } else {
        pos += varPattern2.length();
      }
    }
  }

  return boundQuery;
}

bool SparqlBasedConstraint::executeQuery(const std::string& boundQuery,
                                         QueryExecutionContext* context) const {
  if (!context) {
    return false;
  }

  try {
    // Parse the bound query
    // Note: This is a simplified implementation
    // In a full implementation, we would:
    // 1. Parse the query using SparqlParser
    // 2. Create a QueryExecutionTree
    // 3. Execute and check results
    //
    // For now, we'll do basic string-based validation
    // to avoid complex dependencies during initial development

    // Check if it's an ASK query - if so, return based on pattern matching
    std::string queryUpper = boundQuery;
    std::transform(queryUpper.begin(), queryUpper.end(), queryUpper.begin(),
                   ::toupper);

    if (queryUpper.find("ASK") != std::string::npos) {
      // For ASK queries: true = constraint violated (query returns true)
      // This is a placeholder - real implementation would execute the query
      return true;
    }

    // For SELECT queries: results exist = constraint violated
    // This is a placeholder - real implementation would execute the query
    return false;

  } catch (const std::exception& e) {
    // If query execution fails, assume constraint is violated
    return true;
  }
}

std::vector<std::string> SparqlBasedConstraint::extractViolations(
    const std::string& boundQuery, QueryExecutionContext* context) const {
  std::vector<std::string> violations;

  // Execute query and check if it returns results
  bool hasViolation = executeQuery(boundQuery, context);

  if (hasViolation) {
    std::ostringstream msg;
    msg << (message_.empty() ? "SPARQL constraint violation" : message_);
    msg << " (Query: " << sparqlQuery_.substr(0, 100);
    if (sparqlQuery_.length() > 100) {
      msg << "...";
    }
    msg << ")";
    violations.push_back(msg.str());
  }

  return violations;
}

std::vector<std::string> SparqlBasedConstraint::evaluate(
    const std::string& focusNode,
    const std::unordered_map<std::string, std::string>& bindings,
    QueryExecutionContext* context) const {
  if (!isValid_) {
    return {validationError_};
  }

  if (!context) {
    return {"Cannot evaluate SPARQL constraint: no execution context"};
  }

  try {
    // Create full bindings including focus node
    std::unordered_map<std::string, std::string> fullBindings = bindings;

    // Add standard SHACL variable bindings
    fullBindings["this"] = focusNode;
    fullBindings["focusNode"] = focusNode;

    // If no explicit ?value binding, use focus node as value
    if (fullBindings.find("value") == fullBindings.end()) {
      fullBindings["value"] = focusNode;
    }

    // Bind variables in the query
    std::string boundQuery = bindVariables(fullBindings);

    // Execute and extract violations
    return extractViolations(boundQuery, context);

  } catch (const std::exception& e) {
    return {std::string("SPARQL constraint evaluation failed: ") + e.what()};
  }
}

}  // namespace shacl

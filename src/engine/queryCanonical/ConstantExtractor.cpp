// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/ConstantExtractor.h"

#include <algorithm>
#include <utility>
#include <variant>

#include "global/Constants.h"
#include "parser/GraphPatternOperation.h"
#include "parser/SparqlTriple.h"
#include "parser/TripleComponent.h"

namespace queryCanonical {

// _____________________________________________________________________________
ConstantExtractor::ConstantExtractor(const ParsedQuery& query)
    : query_(query) {}

// _____________________________________________________________________________
std::vector<std::string> ConstantExtractor::extractConstants() {
  std::vector<std::pair<std::string, PlaceholderType>> constants;

  // Collect constants from the root graph pattern
  collectFromGraphPattern(query_._rootGraphPattern, constants);

  // Sort constants by byte representation for deterministic ordering
  std::sort(constants.begin(), constants.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // Extract just the string values
  std::vector<std::string> result;
  result.reserve(constants.size());
  for (const auto& [value, _] : constants) {
    result.push_back(value);
  }

  return result;
}

// _____________________________________________________________________________
PlaceholderType ConstantExtractor::classifyConstant(
    const ad_utility::triple_component::Iri& iri) {
  return PlaceholderType::IRI;
}

// _____________________________________________________________________________
PlaceholderType ConstantExtractor::classifyConstant(
    const ad_utility::triple_component::Literal& literal) {
  // Check for language tag
  if (literal.hasLanguageTag()) {
    return PlaceholderType::LANG_LITERAL;
  }

  // Check for datatype
  if (literal.hasDatatype()) {
    auto datatype = literal.getDatatype();
    std::string datatypeStr(datatype.begin(), datatype.end());

    // Check for boolean
    if (datatypeStr == XSD_BOOLEAN_TYPE) {
      return PlaceholderType::BOOLEAN_LITERAL;
    }

    // Check for temporal types
    if (isTemporalDatatype(datatypeStr)) {
      return PlaceholderType::TEMPORAL_LITERAL;
    }

    // Check for numeric types
    if (isNumericDatatype(datatypeStr)) {
      return PlaceholderType::NUMERIC_LITERAL;
    }
  }

  // Default to plain literal
  return PlaceholderType::PLAIN_LITERAL;
}

// _____________________________________________________________________________
Placeholder ConstantExtractor::replaceWithPlaceholder(
    const ad_utility::triple_component::Iri& iri, uint32_t index) {
  return Placeholder(classifyConstant(iri), index,
                     iri.toStringRepresentation());
}

// _____________________________________________________________________________
Placeholder ConstantExtractor::replaceWithPlaceholder(
    const ad_utility::triple_component::Literal& literal, uint32_t index) {
  return Placeholder(classifyConstant(literal), index,
                     literal.toStringRepresentation());
}

// _____________________________________________________________________________
bool ConstantExtractor::isNumericDatatype(std::string_view datatype) {
  return datatype == XSD_INT_TYPE || datatype == XSD_INTEGER_TYPE ||
         datatype == XSD_FLOAT_TYPE || datatype == XSD_DOUBLE_TYPE ||
         datatype == XSD_DECIMAL_TYPE || datatype == XSD_LONG_TYPE ||
         datatype == XSD_SHORT_TYPE || datatype == XSD_BYTE_TYPE ||
         datatype == XSD_NON_POSITIVE_INTEGER_TYPE ||
         datatype == XSD_NEGATIVE_INTEGER_TYPE ||
         datatype == XSD_NON_NEGATIVE_INTEGER_TYPE ||
         datatype == XSD_UNSIGNED_LONG_TYPE ||
         datatype == XSD_UNSIGNED_INT_TYPE ||
         datatype == XSD_UNSIGNED_SHORT_TYPE ||
         datatype == XSD_POSITIVE_INTEGER_TYPE;
}

// _____________________________________________________________________________
bool ConstantExtractor::isTemporalDatatype(std::string_view datatype) {
  return datatype == XSD_DATETIME_TYPE || datatype == XSD_DATE_TYPE ||
         datatype == XSD_GYEAR_TYPE || datatype == XSD_GYEARMONTH_TYPE ||
         datatype == XSD_DAYTIME_DURATION_TYPE;
}

// _____________________________________________________________________________
void ConstantExtractor::collectFromGraphPattern(
    const parsedQuery::GraphPattern& pattern,
    std::vector<std::pair<std::string, PlaceholderType>>& constants) {
  // Iterate over all graph pattern operations
  for (const auto& child : pattern._graphPatterns) {
    std::visit(
        [this, &constants](const auto& operation) {
          using T = std::decay_t<decltype(operation)>;

          if constexpr (std::is_same_v<T, parsedQuery::BasicGraphPattern>) {
            // Extract from basic graph pattern triples
            for (const auto& triple : operation._triples) {
              collectFromTriple(triple, constants);
            }
          } else if constexpr (std::is_same_v<T, parsedQuery::GroupGraphPattern>) {
            // Recursively process child graph pattern
            collectFromGraphPattern(operation._child, constants);
          } else if constexpr (std::is_same_v<T, parsedQuery::Optional>) {
            // Recursively process optional child
            collectFromGraphPattern(operation._child, constants);
          } else if constexpr (std::is_same_v<T, parsedQuery::Minus>) {
            // Recursively process minus child
            collectFromGraphPattern(operation._child, constants);
          } else if constexpr (std::is_same_v<T, parsedQuery::Union>) {
            // Recursively process both union children
            collectFromGraphPattern(operation._child1, constants);
            collectFromGraphPattern(operation._child2, constants);
          } else if constexpr (std::is_same_v<T, parsedQuery::Subquery>) {
            // Recursively process subquery
            collectFromGraphPattern(operation.get()._rootGraphPattern, constants);
          } else if constexpr (std::is_same_v<T, parsedQuery::Values>) {
            // Extract constants from VALUES clause
            for (const auto& row : operation._inlineValues._values) {
              for (const auto& value : row) {
                if (value.isIri()) {
                  constants.emplace_back(
                      value.getIri().toStringRepresentation(),
                      PlaceholderType::IRI);
                } else if (value.isLiteral()) {
                  constants.emplace_back(
                      value.getLiteral().toStringRepresentation(),
                      classifyConstant(value.getLiteral()));
                }
              }
            }
          }
          // Note: Other operations like Bind, Filter, etc. would need
          // more complex expression traversal which we skip for now
        },
        child);
  }
}

// _____________________________________________________________________________
void ConstantExtractor::collectFromTriple(
    const SparqlTriple& triple,
    std::vector<std::pair<std::string, PlaceholderType>>& constants) {
  // Helper lambda to extract constants from a TripleComponent
  auto extractFromComponent = [&](const TripleComponent& component) {
    if (component.isIri()) {
      constants.emplace_back(component.getIri().toStringRepresentation(),
                             PlaceholderType::IRI);
    } else if (component.isLiteral()) {
      constants.emplace_back(component.getLiteral().toStringRepresentation(),
                             classifyConstant(component.getLiteral()));
    }
    // Variables are not constants, so we skip them
  };

  // Extract from subject
  extractFromComponent(triple.s_);

  // Extract from predicate (if it's a simple IRI, not a property path or
  // variable)
  if (auto predicateIri = triple.getSimplePredicate()) {
    constants.emplace_back(std::string(predicateIri.value()),
                           PlaceholderType::IRI);
  }

  // Extract from object
  extractFromComponent(triple.o_);
}

}  // namespace queryCanonical

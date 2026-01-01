#include "ShExTripleExpression.h"

#include <algorithm>
#include <regex>
#include <sstream>

namespace shex {

// ============================================================================
// NodeConstraint Implementation
// ============================================================================

bool NodeConstraint::validate(const std::string& value, NodeKind kind) const {
  // Check node kind constraint
  if (nodeKind.has_value() && nodeKind.value() != kind) {
    return false;
  }

  // Check value set constraint (enumeration)
  if (!values.empty()) {
    return values.contains(value);
  }

  // For literals, check additional constraints
  if (kind == NodeKind::LITERAL) {
    // Check datatype
    if (datatype.has_value()) {
      // TODO: In full implementation, extract and compare datatype from value
      // For now, just check it's a literal
      if (kind != NodeKind::LITERAL) return false;
    }

    // Check string length facets
    if (length.has_value() && value.length() != length.value()) {
      return false;
    }
    if (minLength.has_value() && value.length() < minLength.value()) {
      return false;
    }
    if (maxLength.has_value() && value.length() > maxLength.value()) {
      return false;
    }

    // Check pattern (regex)
    if (pattern.has_value()) {
      try {
        std::regex r(pattern.value());
        if (!std::regex_match(value, r)) {
          return false;
        }
      } catch (const std::regex_error&) {
        // Invalid regex pattern - fail conservatively
        return false;
      }
    }

    // Check numeric facets (if value is numeric)
    // For 80/20: simple integer parsing
    if (minInclusive.has_value() || maxInclusive.has_value() ||
        minExclusive.has_value() || maxExclusive.has_value()) {
      try {
        int numValue = std::stoi(value);

        if (minInclusive.has_value() && numValue < minInclusive.value()) {
          return false;
        }
        if (maxInclusive.has_value() && numValue > maxInclusive.value()) {
          return false;
        }
        if (minExclusive.has_value() && numValue <= minExclusive.value()) {
          return false;
        }
        if (maxExclusive.has_value() && numValue >= maxExclusive.value()) {
          return false;
        }
      } catch (const std::invalid_argument&) {
        // Not a valid integer - fail if numeric constraints are present
        return false;
      } catch (const std::out_of_range&) {
        return false;
      }
    }
  }

  return true;
}

// ============================================================================
// TripleContext Implementation
// ============================================================================

std::vector<std::pair<std::string, NodeKind>> TripleContext::getRemainingTriples(
    const std::string& predicate) const {
  std::vector<std::pair<std::string, NodeKind>> remaining;

  auto it = triples.find(predicate);
  if (it == triples.end()) {
    return remaining;
  }

  auto matchedIt = matchedTriples.find(predicate);
  if (matchedIt == matchedTriples.end()) {
    // No triples matched yet - all are remaining
    return it->second;
  }

  // Filter out matched triples
  for (const auto& [value, kind] : it->second) {
    bool isMatched = false;
    for (const auto& [matchedValue, matchedKind] : matchedIt->second) {
      if (value == matchedValue && kind == matchedKind) {
        isMatched = true;
        break;
      }
    }
    if (!isMatched) {
      remaining.push_back({value, kind});
    }
  }

  return remaining;
}

void TripleContext::markMatched(const std::string& predicate,
                                const std::string& value, NodeKind kind) {
  matchedTriples[predicate].push_back({value, kind});
}

absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
TripleContext::getAllRemainingTriples() const {
  absl::flat_hash_map<std::string, std::vector<std::pair<std::string, NodeKind>>>
      result;

  for (const auto& [pred, values] : triples) {
    auto remaining = getRemainingTriples(pred);
    if (!remaining.empty()) {
      result[pred] = remaining;
    }
  }

  return result;
}

TripleContext TripleContext::copy() const {
  TripleContext newContext;
  newContext.subject = subject;
  newContext.triples = triples;
  newContext.inverseMode = inverseMode;
  // Don't copy matchedTriples - start fresh
  return newContext;
}

// ============================================================================
// TripleConstraint Implementation
// ============================================================================

ValidationResult TripleConstraint::validate(TripleContext& context) const {
  // Virtual properties don't match against data
  if (virtual_) {
    return ValidationResult::success(0);
  }

  // Get remaining triples for this predicate
  std::vector<std::pair<std::string, NodeKind>> candidates =
      context.getRemainingTriples(predicate);

  // Validate each candidate against value constraint
  std::vector<std::pair<std::string, NodeKind>> validValues;
  for (const auto& [value, kind] : candidates) {
    if (valueConstraint.hasConstraints()) {
      if (valueConstraint.validate(value, kind)) {
        validValues.push_back({value, kind});
      }
    } else {
      // No constraints - all values valid
      validValues.push_back({value, kind});
    }
  }

  // Check cardinality
  size_t count = validValues.size();
  if (!cardinality.satisfies(count)) {
    std::ostringstream oss;
    oss << "Property " << predicate << " cardinality violation: found "
        << count << ", expected " << cardinality.toString();
    return ValidationResult::failure(oss.str());
  }

  // Mark all valid values as matched
  for (const auto& [value, kind] : validValues) {
    context.markMatched(predicate, value, kind);
  }

  return ValidationResult::success(count);
}

// ============================================================================
// EachOf Implementation
// ============================================================================

ValidationResult EachOf::validateSingleIteration(TripleContext& context) const {
  ValidationResult result = ValidationResult::success(0);

  // Validate each sub-expression sequentially
  // All must succeed for the iteration to succeed
  for (const auto& expr : expressions) {
    auto subResult = expr->validate(context);
    if (!subResult.isValid) {
      result.isValid = false;
      result.errors.insert(result.errors.end(), subResult.errors.begin(),
                          subResult.errors.end());
      return result;
    }
    result.matchedCount += subResult.matchedCount;
  }

  return result;
}

ValidationResult EachOf::validate(TripleContext& context) const {
  // Handle empty EachOf (edge case)
  if (expressions.empty()) {
    return ValidationResult::success(0);
  }

  // For EachOf with group cardinality > 1, we need to validate multiple
  // iterations
  size_t iterations = 0;
  ValidationResult aggregateResult = ValidationResult::success(0);

  // Determine how many iterations to perform based on cardinality
  // We try to match as many complete iterations as possible
  while (true) {
    // Try to validate one complete iteration
    TripleContext iterContext = context.copy();
    iterContext.matchedTriples = context.matchedTriples;  // Preserve previous matches

    ValidationResult iterResult = validateSingleIteration(iterContext);

    if (iterResult.isValid) {
      // Iteration succeeded - commit the matches
      iterations++;
      context.matchedTriples = iterContext.matchedTriples;
      aggregateResult.matchedCount += iterResult.matchedCount;

      // Check if we've reached max iterations
      if (cardinality.max.has_value() && iterations >= cardinality.max.value()) {
        break;
      }

      // Check if there are any remaining triples
      if (context.getAllRemainingTriples().empty()) {
        break;
      }
    } else {
      // Iteration failed - check if we have enough iterations
      break;
    }
  }

  // Check if iteration count satisfies cardinality
  if (!cardinality.satisfies(iterations)) {
    std::ostringstream oss;
    oss << "EachOf group cardinality violation: completed " << iterations
        << " iterations, expected " << cardinality.toString();
    return ValidationResult::failure(oss.str());
  }

  return ValidationResult::success(aggregateResult.matchedCount);
}

// ============================================================================
// OneOf Implementation
// ============================================================================

std::optional<size_t> OneOf::selectBestAlternative(
    TripleContext& context) const {
  std::optional<size_t> bestIndex;
  size_t maxMatched = 0;

  // Try each alternative and find the one that matches the most triples
  for (size_t i = 0; i < expressions.size(); ++i) {
    TripleContext testContext = context.copy();
    testContext.matchedTriples = context.matchedTriples;

    ValidationResult result = expressions[i]->validate(testContext);

    if (result.isValid) {
      if (!bestIndex.has_value() || result.matchedCount > maxMatched) {
        bestIndex = i;
        maxMatched = result.matchedCount;
      }
    }
  }

  return bestIndex;
}

ValidationResult OneOf::validate(TripleContext& context) const {
  // Handle empty OneOf (edge case)
  if (expressions.empty()) {
    return ValidationResult::failure("OneOf has no alternatives");
  }

  // For OneOf with group cardinality, we validate iterations
  size_t iterations = 0;
  ValidationResult aggregateResult = ValidationResult::success(0);

  while (true) {
    // Select best alternative for this iteration
    auto bestIndex = selectBestAlternative(context);

    if (!bestIndex.has_value()) {
      // No alternative matches - check if we have enough iterations
      break;
    }

    // Validate the selected alternative and commit matches
    TripleContext iterContext = context.copy();
    iterContext.matchedTriples = context.matchedTriples;

    ValidationResult iterResult = expressions[bestIndex.value()]->validate(iterContext);

    if (iterResult.isValid) {
      iterations++;
      context.matchedTriples = iterContext.matchedTriples;
      aggregateResult.matchedCount += iterResult.matchedCount;

      // Check if we've reached max iterations
      if (cardinality.max.has_value() && iterations >= cardinality.max.value()) {
        break;
      }

      // Check if there are any remaining triples
      if (context.getAllRemainingTriples().empty()) {
        break;
      }
    } else {
      break;
    }
  }

  // Check if iteration count satisfies cardinality
  if (!cardinality.satisfies(iterations)) {
    std::ostringstream oss;
    oss << "OneOf group cardinality violation: completed " << iterations
        << " iterations, expected " << cardinality.toString();
    return ValidationResult::failure(oss.str());
  }

  return ValidationResult::success(aggregateResult.matchedCount);
}

// ============================================================================
// InverseProperty Implementation
// ============================================================================

ValidationResult InverseProperty::validate(TripleContext& context) const {
  // Transform context for inverse mode
  // In inverse mode, we're looking for triples where the focus node is the object
  // This is a simplified implementation - full implementation would require
  // access to the full RDF graph to find inverse triples

  TripleContext inverseContext = context.copy();
  inverseContext.inverseMode = !context.inverseMode;
  inverseContext.matchedTriples = context.matchedTriples;

  // Validate wrapped expression with inverse context
  ValidationResult result = expression->validate(inverseContext);

  // Copy matched triples back to original context
  context.matchedTriples = inverseContext.matchedTriples;

  return result;
}

}  // namespace shex

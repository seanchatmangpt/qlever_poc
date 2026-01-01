//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant) - EPIC 2 Step E

#include "engine/queryCanonical/TriplePatternNormalizer.h"

#include <algorithm>
#include <sstream>
#include <variant>

#include "parser/PropertyPath.h"

namespace queryCanonical {

// ____________________________________________________________________________
std::vector<SparqlTriple> TriplePatternNormalizer::normalizeTriplePatterns(
    std::vector<SparqlTriple> patterns) const {
  // Use stable_sort to maintain relative order for triples with identical keys
  std::stable_sort(patterns.begin(), patterns.end(), compareTriples);
  return patterns;
}

// ____________________________________________________________________________
std::string TriplePatternNormalizer::normalizeComponent(
    const TripleComponent& component) {
  // Convert the component to a normalized string for deterministic comparison.
  // The string format includes a type prefix to ensure different types sort
  // consistently (e.g., variables always sort before/after other types).

  std::ostringstream result;

  if (component.isVariable()) {
    // Variables: prefix with "0:" to sort before other types
    result << "0:" << component.getVariable().name();
  } else if (component.isIri()) {
    // IRIs: prefix with "1:"
    result << "1:" << component.getIri().toStringRepresentation();
  } else if (component.isLiteral()) {
    // Literals: prefix with "2:"
    result << "2:" << component.getLiteral().toStringRepresentation();
  } else if (component.isString()) {
    // Plain strings: prefix with "3:"
    result << "3:" << component.getString();
  } else if (component.isInt()) {
    // Integers: prefix with "4:" and format consistently
    result << "4:" << component.getInt();
  } else if (component.isDouble()) {
    // Doubles: prefix with "5:" and format consistently
    result << "5:" << component.getDouble();
  } else if (component.isBool()) {
    // Booleans: prefix with "6:"
    result << "6:" << (component.getBool() ? "true" : "false");
  } else if (component.isUndef()) {
    // UNDEF: prefix with "7:"
    result << "7:UNDEF";
  } else if (component.isId()) {
    // IDs: prefix with "8:" and use bits for deterministic ordering
    result << "8:" << component.getId().getBits();
  } else {
    // Fallback: use the generic toString() method if available
    // Prefix with "9:" for unknown types
    result << "9:" << component.toString();
  }

  return result.str();
}

// ____________________________________________________________________________
bool TriplePatternNormalizer::compareTriples(const SparqlTriple& a,
                                             const SparqlTriple& b) {
  // Sort key: (predicate, subject, object)
  // This ordering ensures that triples with the same predicate are grouped
  // together, which is beneficial for query optimization.

  // First compare predicates
  // The predicate in SparqlTriple is of type VarOrPath (variant of Variable or
  // PropertyPath).

  std::string predA, predB;

  // Handle predicate comparison
  if (std::holds_alternative<Variable>(a.p_)) {
    predA = "0:" + std::get<Variable>(a.p_).name();
  } else {
    // PropertyPath case - serialize the path for comparison
    const auto& path = std::get<PropertyPath>(a.p_);
    if (path.isIri()) {
      predA = "1:" + std::string(path.getIri().toStringRepresentation());
    } else {
      // Complex property path - use a deterministic serialization
      predA = "1:PATH_COMPLEX";
    }
  }

  if (std::holds_alternative<Variable>(b.p_)) {
    predB = "0:" + std::get<Variable>(b.p_).name();
  } else {
    const auto& path = std::get<PropertyPath>(b.p_);
    if (path.isIri()) {
      predB = "1:" + std::string(path.getIri().toStringRepresentation());
    } else {
      predB = "1:PATH_COMPLEX";
    }
  }

  if (predA != predB) {
    return predA < predB;
  }

  // Predicates are equal, compare subjects
  std::string subjA = normalizeComponent(a.s_);
  std::string subjB = normalizeComponent(b.s_);

  if (subjA != subjB) {
    return subjA < subjB;
  }

  // Subjects are equal, compare objects
  std::string objA = normalizeComponent(a.o_);
  std::string objB = normalizeComponent(b.o_);

  return objA < objB;
}

}  // namespace queryCanonical

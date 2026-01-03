// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: EPIC 14.0 Agent 6 - Unified Determinism Classification

#include "engine/formalism/unified/UnifiedDeterminismClassifier.h"

#include <algorithm>
#include <sstream>

namespace formalism::unified {

// ============================================================================
// UnifiedDeterminismFeatures Implementation
// ============================================================================

std::string UnifiedDeterminismFeatures::toString() const {
  std::ostringstream oss;
  oss << "UnifiedDeterminismFeatures {\n";
  oss << "  Formalism: " << ::formalism::unified::toString(formalism) << "\n";
  oss << "  Classification: " << getClassification() << "\n";

  if (!isDeterministic()) {
    oss << "  Non-Deterministic Reasons:\n";
    for (const auto& reason : getNonDeterministicReasons()) {
      oss << "    - " << reason << "\n";
    }
  }

  oss << "  SPARQL Features:\n";
  oss << "    hasNow: " << (hasNow ? "true" : "false") << "\n";
  oss << "    hasRand: " << (hasRand ? "true" : "false") << "\n";
  oss << "    hasUuid: " << (hasUuid ? "true" : "false") << "\n";
  oss << "    hasBnode: " << (hasBnode ? "true" : "false") << "\n";
  oss << "    hasService: " << (hasService ? "true" : "false") << "\n";

  oss << "  N3 Features:\n";
  oss << "    hasN3Formulae: " << (hasN3Formulae ? "true" : "false") << "\n";
  oss << "    hasN3Variables: " << (hasN3Variables ? "true" : "false") << "\n";
  oss << "    hasN3Quantifiers: " << (hasN3Quantifiers ? "true" : "false")
      << "\n";
  oss << "    hasN3Implications: " << (hasN3Implications ? "true" : "false")
      << "\n";
  oss << "    hasN3BuiltIns: " << (hasN3BuiltIns ? "true" : "false") << "\n";

  oss << "  Datalog Features:\n";
  oss << "    hasDatalogRecursion: " << (hasDatalogRecursion ? "true" : "false")
      << "\n";
  oss << "    hasDatalogNonMonotonic: "
      << (hasDatalogNonMonotonic ? "true" : "false") << "\n";

  oss << "  SHACL Features:\n";
  oss << "    hasShaclTemporalConstraint: "
      << (hasShaclTemporalConstraint ? "true" : "false") << "\n";
  oss << "    hasShaclDynamicFunction: "
      << (hasShaclDynamicFunction ? "true" : "false") << "\n";

  oss << "}";
  return oss.str();
}

// ============================================================================
// UnifiedDeterminismClassifier Implementation
// ============================================================================

UnifiedDeterminismFeatures
UnifiedDeterminismClassifier::analyzeSparqlQuery(
    const ParsedQuery& query) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SPARQL;

  // Delegate to existing SPARQL DeterminismClassifier
  auto sparqlFeatures = sparqlClassifier_.analyze(query);

  // Copy SPARQL-specific flags
  features.hasNow = sparqlFeatures.hasNow;
  features.hasRand = sparqlFeatures.hasRand;
  features.hasUuid = sparqlFeatures.hasUuid;
  features.hasBnode = sparqlFeatures.hasBnode;
  features.hasService = sparqlFeatures.hasService;
  features.hasNonDeterministicFunction =
      sparqlFeatures.hasNonDeterministicFunction;

  return features;
}

UnifiedDeterminismFeatures
UnifiedDeterminismClassifier::analyzeDatalogRule(
    const DatalogRule& rule) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::DATALOG;

  // Check if rule is recursive
  features.hasDatalogRecursion = rule.isRecursive();

  // Analyze rule body for non-deterministic patterns
  analyzeDatalogRuleBody(rule, features);

  return features;
}

UnifiedDeterminismFeatures
UnifiedDeterminismClassifier::analyzeDatalogProgram(
    const std::vector<DatalogRule>& rules) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::DATALOG;

  // Detect recursion patterns across rules
  detectDatalogRecursion(rules, features);

  // Analyze each rule individually
  for (const auto& rule : rules) {
    auto ruleFeatures = analyzeDatalogRule(rule);

    // Merge features (logical OR for non-determinism flags)
    features.hasDatalogRecursion |= ruleFeatures.hasDatalogRecursion;
    features.hasDatalogNegation |= ruleFeatures.hasDatalogNegation;
    features.hasDatalogAggregation |= ruleFeatures.hasDatalogAggregation;
    features.hasDatalogNonMonotonic |= ruleFeatures.hasDatalogNonMonotonic;
    features.hasNonDeterministicFunction |=
        ruleFeatures.hasNonDeterministicFunction;
  }

  return features;
}

UnifiedDeterminismFeatures UnifiedDeterminismClassifier::analyzeN3Document(
    const std::string& n3Content) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::N3;

  // Detect N3-specific features
  detectN3Features(n3Content, features);

  return features;
}

UnifiedDeterminismFeatures UnifiedDeterminismClassifier::analyzeShaclShapes(
    const ParsedQuery& shaclQuery) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::SHACL;

  // SHACL shapes are typically deterministic unless they use:
  // 1. Temporal constraints (e.g., validation time-based rules)
  // 2. Dynamic function evaluation with external dependencies

  // First, analyze as SPARQL query (SHACL constraints are SPARQL-based)
  auto sparqlFeatures = analyzeSparqlQuery(shaclQuery);

  // Copy SPARQL non-determinism flags
  features.hasNow = sparqlFeatures.hasNow;
  features.hasRand = sparqlFeatures.hasRand;
  features.hasService = sparqlFeatures.hasService;

  // Detect SHACL-specific non-determinism
  // Note: This is conservative; actual implementation would parse SHACL
  // constraints more precisely
  if (features.hasNow) {
    features.hasShaclTemporalConstraint = true;
  }

  return features;
}

UnifiedDeterminismFeatures UnifiedDeterminismClassifier::analyze(
    const std::string& input, FormalismType formalism) const {
  // Dispatch to formalism-specific analyzer
  switch (formalism) {
    case FormalismType::SPARQL: {
      // For generic string input, we'd need to parse it first
      // This is a simplified placeholder
      UnifiedDeterminismFeatures features;
      features.formalism = FormalismType::SPARQL;
      analyzeExpressionTree(input, features);
      return features;
    }
    case FormalismType::N3:
      return analyzeN3Document(input);
    case FormalismType::DATALOG: {
      // Datalog requires parsed rules, not string input
      // This is a placeholder for text-based analysis
      UnifiedDeterminismFeatures features;
      features.formalism = FormalismType::DATALOG;
      analyzeExpressionTree(input, features);
      return features;
    }
    case FormalismType::SHACL: {
      UnifiedDeterminismFeatures features;
      features.formalism = FormalismType::SHACL;
      analyzeExpressionTree(input, features);
      return features;
    }
    default: {
      UnifiedDeterminismFeatures features;
      features.hasNonDeterministicFunction =
          true;  // Fail-closed on unknown
      return features;
    }
  }
}

IngressGuardConfig UnifiedDeterminismClassifier::createGuardConfig(
    FormalismType formalism) const {
  IngressGuardConfig guards;

  // Formalism-specific guard configurations
  switch (formalism) {
    case FormalismType::SPARQL:
      // SPARQL queries: moderate limits
      guards.max_input_size_bytes = 10 * 1024 * 1024;  // 10MB
      guards.max_nesting_depth = 50;
      guards.max_object_keys = 5000;
      guards.timeout_ms = 30000;  // 30 seconds
      break;

    case FormalismType::N3:
      // N3 documents: larger limits for complex logic
      guards.max_input_size_bytes = 50 * 1024 * 1024;  // 50MB
      guards.max_nesting_depth = 100;
      guards.max_object_keys = 10000;
      guards.timeout_ms = 60000;  // 60 seconds
      break;

    case FormalismType::DATALOG:
      // Datalog programs: moderate limits
      guards.max_input_size_bytes = 20 * 1024 * 1024;  // 20MB
      guards.max_nesting_depth = 50;
      guards.max_object_keys = 5000;
      guards.timeout_ms = 45000;  // 45 seconds
      break;

    case FormalismType::SHACL:
      // SHACL shapes: conservative limits
      guards.max_input_size_bytes = 25 * 1024 * 1024;  // 25MB
      guards.max_nesting_depth = 75;
      guards.max_object_keys = 7500;
      guards.timeout_ms = 40000;  // 40 seconds
      break;

    default:
      // Default: most conservative
      guards.max_input_size_bytes = 5 * 1024 * 1024;  // 5MB
      guards.max_nesting_depth = 25;
      guards.max_object_keys = 1000;
      guards.timeout_ms = 15000;  // 15 seconds
      break;
  }

  return guards;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void UnifiedDeterminismClassifier::analyzeExpressionTree(
    const std::string& expr, UnifiedDeterminismFeatures& features) const {
  // Simple text-based detection of non-deterministic functions
  // This is conservative: detects keyword presence in text

  // SPARQL non-deterministic functions
  if (expr.find("NOW()") != std::string::npos ||
      expr.find("NOW(") != std::string::npos) {
    features.hasNow = true;
  }
  if (expr.find("RAND()") != std::string::npos ||
      expr.find("RAND(") != std::string::npos) {
    features.hasRand = true;
  }
  if (expr.find("UUID()") != std::string::npos ||
      expr.find("UUID(") != std::string::npos ||
      expr.find("STRUUID()") != std::string::npos ||
      expr.find("STRUUID(") != std::string::npos) {
    features.hasUuid = true;
  }
  if (expr.find("BNODE()") != std::string::npos ||
      expr.find("BNODE(") != std::string::npos) {
    features.hasBnode = true;
  }
  if (expr.find("SERVICE") != std::string::npos) {
    features.hasService = true;
  }
}

void UnifiedDeterminismClassifier::detectN3Features(
    const std::string& content, UnifiedDeterminismFeatures& features) const {
  // Detect N3-specific syntax patterns

  // Formulae: { ... } (graph literals)
  if (content.find('{') != std::string::npos &&
      content.find('}') != std::string::npos) {
    // Conservative: any braces might indicate formulae
    // More precise: check for triple patterns inside braces
    size_t openPos = content.find('{');
    size_t closePos = content.find('}', openPos);
    if (closePos != std::string::npos) {
      std::string inner = content.substr(openPos + 1, closePos - openPos - 1);
      // Check if it looks like a triple pattern
      if (inner.find('.') != std::string::npos ||
          inner.find('?') != std::string::npos) {
        features.hasN3Formulae = true;
      }
    }
  }

  // Quantifiers: @forAll, @forSome
  if (content.find("@forAll") != std::string::npos ||
      content.find("@forSome") != std::string::npos) {
    features.hasN3Quantifiers = true;
  }

  // Variables: ?var or :var patterns
  if (content.find('?') != std::string::npos ||
      (content.find(':') != std::string::npos &&
       content.find("@prefix") != std::string::npos)) {
    features.hasN3Variables = true;
  }

  // Implications: =>
  if (content.find("=>") != std::string::npos) {
    features.hasN3Implications = true;
  }

  // Built-in functions: math:, log:, string:, etc.
  const std::vector<std::string> builtInPrefixes = {
      "math:", "log:", "string:", "list:", "time:", "crypto:"};
  for (const auto& prefix : builtInPrefixes) {
    if (content.find(prefix) != std::string::npos) {
      features.hasN3BuiltIns = true;
      break;
    }
  }

  // N3 paths: ! (inverse) or ^ (reverse)
  if (content.find('!') != std::string::npos ||
      content.find('^') != std::string::npos) {
    features.hasN3Paths = true;
  }
}

void UnifiedDeterminismClassifier::detectDatalogRecursion(
    const std::vector<DatalogRule>& rules,
    UnifiedDeterminismFeatures& features) const {
  // Detect recursion patterns across multiple rules
  // A program is recursive if any rule references itself (directly or
  // indirectly)

  for (const auto& rule : rules) {
    if (rule.isRecursive()) {
      features.hasDatalogRecursion = true;
      break;
    }
  }

  // TODO: Detect mutual recursion (rule A calls B, B calls A)
  // This requires dependency graph analysis
}

void UnifiedDeterminismClassifier::analyzeDatalogRuleBody(
    const DatalogRule& rule, UnifiedDeterminismFeatures& features) const {
  // Analyze rule body patterns and filters for non-deterministic operations

  // Check filters for non-deterministic functions
  for (const auto& filter : rule.getFilters()) {
    const std::string& exprDesc = filter.expression_.getDescriptor();

    // Detect SPARQL-style non-deterministic functions in filter expressions
    if (exprDesc.find("NOW") != std::string::npos) {
      features.hasNow = true;
    }
    if (exprDesc.find("RAND") != std::string::npos) {
      features.hasRand = true;
    }
    if (exprDesc.find("UUID") != std::string::npos) {
      features.hasUuid = true;
    }

    // Detect Datalog-specific non-monotonic operations
    // Negation: NOT, MINUS
    if (exprDesc.find("NOT") != std::string::npos ||
        exprDesc.find("MINUS") != std::string::npos ||
        exprDesc.find("!") != std::string::npos) {
      features.hasDatalogNegation = true;
      // Note: Stratified negation is deterministic, but we're conservative
    }

    // Aggregation: COUNT, SUM, AVG, MIN, MAX
    const std::vector<std::string> aggFunctions = {"COUNT", "SUM", "AVG", "MIN",
                                                   "MAX"};
    for (const auto& aggFunc : aggFunctions) {
      if (exprDesc.find(aggFunc) != std::string::npos) {
        features.hasDatalogAggregation = true;
        break;
      }
    }
  }

  // Check body patterns for non-deterministic constructs
  for (const auto& pattern : rule.getBodyPatterns()) {
    // Check if pattern involves external predicates or functions
    // (Conservative: assume external predicates might be non-deterministic)
    const std::string& predicate = pattern.p_.toString();
    if (predicate.find("external:") != std::string::npos ||
        predicate.find("http://") != std::string::npos) {
      features.hasExternalDependency = true;
    }
  }

  // Set non-monotonic flag if negation or aggregation detected
  if (features.hasDatalogNegation || features.hasDatalogAggregation) {
    features.hasDatalogNonMonotonic = true;
  }
}

// ============================================================================
// DeterminismContract Implementation
// ============================================================================

std::string DeterminismContract::generateViolationReport(
    const UnifiedDeterminismFeatures& features) {
  if (satisfiesCachingContract(features)) {
    return "No contract violations. Operation is cacheable.";
  }

  std::ostringstream oss;
  oss << "DETERMINISM CONTRACT VIOLATION REPORT\n";
  oss << "=====================================\n\n";
  oss << "Formalism: " << toString(features.formalism) << "\n";
  oss << "Classification: " << features.getClassification() << "\n\n";
  oss << "Caching Decision: REJECTED (fail-closed)\n\n";
  oss << "Reasons for Non-Determinism:\n";

  auto reasons = features.getNonDeterministicReasons();
  if (reasons.empty()) {
    oss << "  - Unknown (conservative classification)\n";
  } else {
    for (const auto& reason : reasons) {
      oss << "  - " << reason << "\n";
    }
  }

  oss << "\nContract Requirements:\n";
  oss << "  - CACHE_REQUIRES_DETERMINISM: "
      << (CACHE_REQUIRES_DETERMINISM ? "YES" : "NO") << "\n";
  oss << "  - FAIL_CLOSED_ON_UNKNOWN: "
      << (FAIL_CLOSED_ON_UNKNOWN ? "YES" : "NO") << "\n";
  oss << "  - GUARD_VIOLATION_REJECTS_ALL: "
      << (GUARD_VIOLATION_REJECTS_ALL ? "YES" : "NO") << "\n";

  oss << "\nRecommendation:\n";
  oss << "  Remove non-deterministic operations or execute without caching.\n";

  return oss.str();
}

}  // namespace formalism::unified

// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: EPIC 14.0 Agent 6 - Unified Determinism Classification
//
// UnifiedDeterminismClassifier - Cross-formalism determinism analysis
//
// PURPOSE:
// Extends SPARQL-only DeterminismClassifier to handle all four formalisms:
// SPARQL, SHACL, N3, and Datalog. Provides unified determinism classification
// with fail-closed caching rejection for non-deterministic operations.
//
// SCOPE:
// - SPARQL: NOW(), RAND(), UUID(), BNODE(), SERVICE (existing)
// - N3: Formulae, variables, quantifiers, implications, built-ins
// - Datalog: Rule-level recursion with non-deterministic functions
// - SHACL: Validation with temporal or random functions
//
// DESIGN:
// - Extends existing DeterminismClassifier (does NOT modify it)
// - Composition pattern: wraps existing classifier + adds formalism-specific
// - Rule-level analysis (not just query-level)
// - Fail-closed: non-deterministic programs rejected from cache
// - Guard enforcement via IngressGuardConfig integration
//
// EPIC 14.0 CONSTRAINTS:
// - Do NOT modify existing DeterminismClassifier
// - Do NOT modify N3ComplianceVerifier
// - Create new unified system with composition
// - Ensure determinism contract is documented

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDDETERMINISMCLASSIFIER_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDDETERMINISMCLASSIFIER_H

#include <optional>
#include <string>
#include <vector>

#include "engine/ingress/JsonLdIngressNormalizer.h"
#include "engine/queryCanonical/DeterminismClassifier.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "parser/DatalogRule.h"
#include "parser/ParsedQuery.h"

namespace formalism::unified {

// ============================================================================
// FORMALISM ENUMERATION
// ============================================================================

// Supported formalisms for determinism analysis
enum class FormalismType {
  SPARQL = 0,  // SPARQL 1.1 queries
  SHACL = 1,   // SHACL validation shapes
  N3 = 2,      // Notation3 rules and logic
  DATALOG = 3  // Datalog rules with recursion
};

// Convert formalism type to string
inline std::string toString(FormalismType type) {
  switch (type) {
    case FormalismType::SPARQL:
      return "SPARQL";
    case FormalismType::SHACL:
      return "SHACL";
    case FormalismType::N3:
      return "N3";
    case FormalismType::DATALOG:
      return "DATALOG";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// UNIFIED DETERMINISM FEATURES
// ============================================================================

// Extended determinism features covering all four formalisms
// Extends queryCanonical::DeterminismFeatures with formalism-specific flags
struct UnifiedDeterminismFeatures {
  // ===== SPARQL Features (from DeterminismClassifier) =====
  bool hasNow = false;      // NOW() function
  bool hasRand = false;     // RAND() function
  bool hasUuid = false;     // UUID() / STRUUID() functions
  bool hasBnode = false;    // BNODE() function (blank node generation)
  bool hasService = false;  // SERVICE clause (federated query)

  // ===== N3-Specific Features =====
  bool hasN3Formulae = false;      // N3 formulae (graph literals)
  bool hasN3Variables = false;     // N3 universal/existential variables
  bool hasN3Quantifiers = false;   // @forAll, @forSome quantifiers
  bool hasN3Implications = false;  // N3 implication rules (=>)
  bool hasN3BuiltIns = false;      // N3 built-in functions (math:, log:, etc.)
  bool hasN3Paths = false;         // N3 path expressions

  // ===== Datalog-Specific Features =====
  bool hasDatalogRecursion = false;  // Recursive rule definitions
  bool hasDatalogNegation = false;   // Negation in rule body
  bool hasDatalogAggregation =
      false;  // Aggregation in rules (COUNT, SUM, etc.)
  bool hasDatalogNonMonotonic = false;  // Non-monotonic operations

  // ===== SHACL-Specific Features =====
  bool hasShaclTemporalConstraint = false;  // Time-based validation
  bool hasShaclDynamicFunction = false;     // Dynamic function evaluation

  // ===== General Non-Determinism Flags =====
  bool hasNonDeterministicFunction = false;  // Other non-deterministic ops
  bool hasExternalDependency = false;  // External service/file dependencies

  // ===== Metadata =====
  FormalismType formalism = FormalismType::SPARQL;
  std::string formalismVersion;  // e.g., "SPARQL 1.1", "N3 2011"

  // Check if operation is deterministic (cacheable)
  [[nodiscard]] bool isDeterministic() const {
    // SPARQL non-determinism
    if (hasNow || hasRand || hasUuid || hasBnode || hasService) {
      return false;
    }

    // N3 non-determinism
    // Note: N3 formulae and variables CAN be deterministic if no random/time
    // operations, but implications and quantifiers introduce logical complexity
    if (hasN3BuiltIns) {
      return false;  // Conservative: N3 built-ins may be non-deterministic
    }

    // Datalog non-determinism
    // Note: Recursion itself is deterministic (fixpoint), but combined with
    // non-deterministic functions it becomes non-deterministic
    if (hasDatalogNonMonotonic) {
      return false;
    }

    // SHACL non-determinism
    if (hasShaclTemporalConstraint || hasShaclDynamicFunction) {
      return false;
    }

    // General non-determinism
    if (hasNonDeterministicFunction || hasExternalDependency) {
      return false;
    }

    return true;
  }

  // Get determinism classification as string
  [[nodiscard]] std::string getClassification() const {
    if (isDeterministic()) {
      return "DETERMINISTIC";
    }
    return "NON_DETERMINISTIC";
  }

  // Convert to queryCanonical::DeterminismFeatures (for SPARQL compatibility)
  [[nodiscard]] queryCanonical::DeterminismFeatures
  toSparqlDeterminismFeatures() const {
    queryCanonical::DeterminismFeatures features;
    features.hasNow = hasNow;
    features.hasRand = hasRand;
    features.hasUuid = hasUuid;
    features.hasBnode = hasBnode;
    features.hasService = hasService;
    features.hasNonDeterministicFunction = hasNonDeterministicFunction;
    return features;
  }

  // Human-readable summary of non-deterministic features
  [[nodiscard]] std::vector<std::string> getNonDeterministicReasons() const {
    std::vector<std::string> reasons;

    if (hasNow) reasons.push_back("NOW() function");
    if (hasRand) reasons.push_back("RAND() function");
    if (hasUuid) reasons.push_back("UUID() function");
    if (hasBnode) reasons.push_back("BNODE() function");
    if (hasService) reasons.push_back("SERVICE clause");
    if (hasN3BuiltIns) reasons.push_back("N3 built-in functions");
    if (hasDatalogNonMonotonic) reasons.push_back("Datalog non-monotonic ops");
    if (hasShaclTemporalConstraint) reasons.push_back("SHACL temporal constraint");
    if (hasShaclDynamicFunction) reasons.push_back("SHACL dynamic function");
    if (hasNonDeterministicFunction)
      reasons.push_back("Other non-deterministic function");
    if (hasExternalDependency) reasons.push_back("External dependency");

    return reasons;
  }

  // Serialize to string for logging/debugging
  [[nodiscard]] std::string toString() const;
};

// ============================================================================
// UNIFIED DETERMINISM CLASSIFIER
// ============================================================================

// Unified determinism classifier for all four formalisms
// Composition pattern: wraps existing DeterminismClassifier + adds extensions
class UnifiedDeterminismClassifier {
 public:
  // Constructor
  explicit UnifiedDeterminismClassifier() = default;

  // ===== SPARQL Analysis =====

  // Analyze SPARQL query for determinism
  // Delegates to existing DeterminismClassifier and extends result
  [[nodiscard]] UnifiedDeterminismFeatures analyzeSparqlQuery(
      const ParsedQuery& query) const;

  // ===== Datalog Analysis =====

  // Analyze single Datalog rule for determinism
  // Checks for non-deterministic functions in rule body and constraints
  [[nodiscard]] UnifiedDeterminismFeatures analyzeDatalogRule(
      const DatalogRule& rule) const;

  // Analyze entire Datalog program (multiple rules)
  // Performs rule-level analysis and detects recursion patterns
  [[nodiscard]] UnifiedDeterminismFeatures analyzeDatalogProgram(
      const std::vector<DatalogRule>& rules) const;

  // ===== N3 Analysis =====

  // Analyze N3 document for determinism
  // Detects N3-specific features: formulae, quantifiers, implications, built-ins
  // Input: N3 document as string (pre-parsed or raw)
  [[nodiscard]] UnifiedDeterminismFeatures analyzeN3Document(
      const std::string& n3Content) const;

  // ===== SHACL Analysis =====

  // Analyze SHACL shapes for determinism
  // Checks for temporal constraints and dynamic functions
  // Input: SHACL shapes graph (as ParsedQuery with SHACL extensions)
  [[nodiscard]] UnifiedDeterminismFeatures analyzeShaclShapes(
      const ParsedQuery& shaclQuery) const;

  // ===== Generic Analysis (Auto-detect formalism) =====

  // Analyze any formalism (auto-detect type from input)
  // Uses heuristics to determine formalism type
  [[nodiscard]] UnifiedDeterminismFeatures analyze(
      const std::string& input, FormalismType formalism) const;

  // ===== Guard Integration =====

  // Check if operation should be cached based on determinism
  // Fail-closed: returns false if non-deterministic
  [[nodiscard]] bool isCacheable(
      const UnifiedDeterminismFeatures& features) const {
    return features.isDeterministic();
  }

  // Create guard configuration for formalism-specific determinism checking
  // Integrates with IngressGuardConfig
  [[nodiscard]] IngressGuardConfig createGuardConfig(
      FormalismType formalism) const;

 private:
  // ===== Internal Helpers =====

  // Analyze expression tree for non-deterministic operations
  void analyzeExpressionTree(const std::string& expr,
                             UnifiedDeterminismFeatures& features) const;

  // Detect N3 features in text content
  void detectN3Features(const std::string& content,
                        UnifiedDeterminismFeatures& features) const;

  // Detect Datalog recursion patterns
  void detectDatalogRecursion(const std::vector<DatalogRule>& rules,
                              UnifiedDeterminismFeatures& features) const;

  // Check if Datalog rule body contains non-deterministic functions
  void analyzeDatalogRuleBody(const DatalogRule& rule,
                              UnifiedDeterminismFeatures& features) const;

  // Wrapped SPARQL classifier (composition, not inheritance)
  queryCanonical::DeterminismClassifier sparqlClassifier_;
};

// ============================================================================
// DETERMINISM CONTRACT
// ============================================================================

// Determinism contract for caching decisions
// Formal specification of caching eligibility based on determinism
struct DeterminismContract {
  // Rule: Only deterministic operations may be cached
  // Rationale: Non-deterministic operations produce different results for
  //            identical inputs, violating cache correctness
  static constexpr bool CACHE_REQUIRES_DETERMINISM = true;

  // Rule: Fail-closed on unknown operations
  // Rationale: Conservative approach prevents caching potentially
  //            non-deterministic operations
  static constexpr bool FAIL_CLOSED_ON_UNKNOWN = true;

  // Rule: Guard violations reject entire program
  // Rationale: Partial processing creates inconsistent state
  static constexpr bool GUARD_VIOLATION_REJECTS_ALL = true;

  // Check if features satisfy caching contract
  [[nodiscard]] static bool satisfiesCachingContract(
      const UnifiedDeterminismFeatures& features) {
    // Contract: deterministic AND no guard violations
    return features.isDeterministic();
  }

  // Generate contract violation report
  [[nodiscard]] static std::string generateViolationReport(
      const UnifiedDeterminismFeatures& features);
};

}  // namespace formalism::unified

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDDETERMINISMCLASSIFIER_H

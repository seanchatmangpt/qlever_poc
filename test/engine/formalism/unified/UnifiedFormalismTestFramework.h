// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Agent 9 - EPIC 14.0 Formalism Convergence

#ifndef QLEVER_TEST_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMTESTFRAMEWORK_H
#define QLEVER_TEST_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMTESTFRAMEWORK_H

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#include "engine/Operation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/Result.h"
#include "util/HashSet.h"

/**
 * EPIC 14.0 Unified Formalism Testing Framework
 *
 * This framework addresses critical testing gaps identified in the formalism
 * convergence audit (MURA_DELTA_SEVERITY.md):
 *
 * 1. DETERMINISM TESTING GAP (🔴 HIGH SEVERITY):
 *    - No rule-level determinism tests across all formalisms
 *    - Only query-level determinism exists
 *
 * 2. COVERAGE GAP (🔴 HIGH SEVERITY):
 *    - N3/Datalog lack W3C-style golden test suites
 *    - Only SHACL has formal external validation
 *
 * 3. NEGATIVE TEST COVERAGE IMBALANCE (🟡 MEDIUM):
 *    - Only N3 has explicit negative corpus
 *    - SHACL/Datalog rely on implicit assumptions
 *
 * 4. CROSS-FORMALISM EQUIVALENCE (NEW):
 *    - No tests verify equivalent behavior across formalisms
 *
 * Design Principles:
 * - Formalism-agnostic: Tests work across SHACL, N3, Datalog, ShEx
 * - Determinism-first: All tests validate deterministic execution
 * - Golden corpus: External reference standards for validation
 * - Negative corpus: Explicit invalid input testing
 * - Equivalence testing: Cross-formalism semantic validation
 */

namespace formalism::unified {

// =============================================================================
// Formalism Enum
// =============================================================================

enum class FormalismType {
  SHACL,
  ShEx,
  N3,
  Datalog,
  SPARQL  // For baseline comparison
};

inline std::string toString(FormalismType type) {
  switch (type) {
    case FormalismType::SHACL: return "SHACL";
    case FormalismType::ShEx: return "ShEx";
    case FormalismType::N3: return "N3";
    case FormalismType::Datalog: return "Datalog";
    case FormalismType::SPARQL: return "SPARQL";
  }
  return "Unknown";
}

// =============================================================================
// Test Result Types
// =============================================================================

/**
 * Unified validation result structure.
 * Maps all formalism-specific outputs to common schema.
 */
struct UnifiedValidationResult {
  bool conforms;
  std::vector<std::string> violations;
  std::string focusNode;
  std::string constraintComponent;
  std::string severity;
  std::string message;

  // Determinism metadata
  bool isDeterministic;
  std::string fingerprintHash;

  // Execution metadata
  FormalismType formalism;
  std::chrono::milliseconds executionTime;

  // Serialization
  nlohmann::json toJson() const;
  static UnifiedValidationResult fromJson(const nlohmann::json& j);

  bool operator==(const UnifiedValidationResult& other) const;
};

/**
 * Determinism test result.
 * Validates that repeated execution produces identical results.
 */
struct DeterminismTestResult {
  bool isDeterministic;
  std::string fingerprintHash;
  std::vector<std::string> fingerprintSequence;  // For multi-run validation
  std::optional<std::string> nonDeterminismCause;

  nlohmann::json toJson() const;
  static DeterminismTestResult fromJson(const nlohmann::json& j);
};

/**
 * Equivalence test result.
 * Validates that different formalisms produce semantically equivalent results.
 */
struct EquivalenceTestResult {
  bool areEquivalent;
  FormalismType formalism1;
  FormalismType formalism2;
  std::string equivalenceType;  // "semantic", "structural", "output"
  std::optional<std::string> divergenceDescription;

  nlohmann::json toJson() const;
  static EquivalenceTestResult fromJson(const nlohmann::json& j);
};

// =============================================================================
// Test Case Specification
// =============================================================================

/**
 * Golden test case specification.
 * Defines input, expected output, and validation criteria.
 */
struct GoldenTestCase {
  std::string testId;
  std::string description;
  FormalismType formalism;

  // Input specification
  std::string inputFormat;  // "turtle", "jsonld", "datalog-text", etc.
  std::string input;

  // Expected output
  bool expectedConforms;
  std::vector<std::string> expectedViolations;
  std::optional<nlohmann::json> expectedResult;

  // Validation metadata
  std::string source;  // "W3C", "custom", "RFC", etc.
  std::string sourceUrl;
  std::vector<std::string> tags;

  nlohmann::json toJson() const;
  static GoldenTestCase fromJson(const nlohmann::json& j);
};

/**
 * Negative test case specification.
 * Defines invalid inputs with expected rejection behavior.
 */
struct NegativeTestCase {
  std::string testId;
  std::string description;
  FormalismType formalism;

  // Invalid input
  std::string inputFormat;
  std::string invalidInput;

  // Expected error behavior
  std::string expectedErrorType;  // "ParseException", "ValidationError", etc.
  std::string expectedErrorPattern;  // Regex pattern for error message
  std::optional<int> expectedErrorLine;

  // Metadata
  std::vector<std::string> tags;

  nlohmann::json toJson() const;
  static NegativeTestCase fromJson(const nlohmann::json& j);
};

/**
 * Cross-formalism equivalence test specification.
 * Defines equivalent inputs across different formalisms.
 */
struct EquivalenceTestCase {
  std::string testId;
  std::string description;

  // Inputs for different formalisms
  struct FormalismInput {
    FormalismType formalism;
    std::string inputFormat;
    std::string input;
  };
  std::vector<FormalismInput> inputs;

  // Expected equivalence properties
  std::string equivalenceType;  // "semantic", "structural", "output"
  std::optional<nlohmann::json> canonicalResult;

  // Metadata
  std::vector<std::string> tags;

  nlohmann::json toJson() const;
  static EquivalenceTestCase fromJson(const nlohmann::json& j);
};

// =============================================================================
// Test Corpus Management
// =============================================================================

/**
 * Test corpus loader and validator.
 * Loads test cases from JSON files and validates structure.
 */
class TestCorpusLoader {
 public:
  explicit TestCorpusLoader(const std::filesystem::path& corpusRoot);

  // Load test cases
  std::vector<GoldenTestCase> loadGoldenTests(FormalismType formalism);
  std::vector<NegativeTestCase> loadNegativeTests(FormalismType formalism);
  std::vector<EquivalenceTestCase> loadEquivalenceTests();

  // Validate corpus structure
  bool validateCorpusStructure();
  std::vector<std::string> getValidationErrors() const;

 private:
  std::filesystem::path corpusRoot_;
  std::vector<std::string> validationErrors_;

  nlohmann::json loadJsonFile(const std::filesystem::path& path);
  bool validateGoldenTestSchema(const nlohmann::json& testCase);
  bool validateNegativeTestSchema(const nlohmann::json& testCase);
  bool validateEquivalenceTestSchema(const nlohmann::json& testCase);
};

// =============================================================================
// Determinism Testing Framework
// =============================================================================

/**
 * Determinism tester for rule-level and query-level validation.
 * Addresses the critical testing gap identified in audit.
 */
class DeterminismTester {
 public:
  explicit DeterminismTester(size_t numRuns = 10);

  // Rule-level determinism testing
  DeterminismTestResult testRuleDeterminism(
      FormalismType formalism,
      const std::string& ruleInput,
      const std::string& dataGraph);

  // Query-level determinism testing
  DeterminismTestResult testQueryDeterminism(
      const std::string& sparqlQuery,
      const std::string& dataGraph);

  // Combined formalism + query determinism
  DeterminismTestResult testEndToEndDeterminism(
      FormalismType formalism,
      const std::string& formalismInput,
      const std::string& sparqlQuery,
      const std::string& dataGraph);

 private:
  size_t numRuns_;

  std::string computeResultFingerprint(const Result& result);
  bool checkFingerprintConsistency(const std::vector<std::string>& fingerprints);
  std::optional<std::string> detectNonDeterminismCause(
      const std::vector<Result>& results);
};

// =============================================================================
// Cross-Formalism Equivalence Testing
// =============================================================================

/**
 * Equivalence tester for validating semantic equivalence across formalisms.
 */
class EquivalenceTester {
 public:
  EquivalenceTester();

  // Test semantic equivalence
  EquivalenceTestResult testSemanticEquivalence(
      const EquivalenceTestCase& testCase);

  // Test structural equivalence (AST-level)
  EquivalenceTestResult testStructuralEquivalence(
      FormalismType formalism1,
      const std::string& input1,
      FormalismType formalism2,
      const std::string& input2);

  // Test output equivalence (result-level)
  EquivalenceTestResult testOutputEquivalence(
      const Result& result1,
      const Result& result2);

 private:
  bool compareResults(const Result& r1, const Result& r2);
  bool compareIdTables(const IdTable& t1, const IdTable& t2);
  std::string describeResultDifference(const Result& r1, const Result& r2);
};

// =============================================================================
// Unified Test Executor
// =============================================================================

/**
 * Main test executor that coordinates all test types.
 * Provides unified interface for running formalism tests.
 */
class UnifiedTestExecutor {
 public:
  explicit UnifiedTestExecutor(QueryExecutionContext* ctx);

  // Execute golden tests
  UnifiedValidationResult executeGoldenTest(const GoldenTestCase& testCase);

  // Execute negative tests
  bool executeNegativeTest(const NegativeTestCase& testCase);

  // Execute equivalence tests
  EquivalenceTestResult executeEquivalenceTest(
      const EquivalenceTestCase& testCase);

  // Execute determinism tests
  DeterminismTestResult executeDeterminismTest(
      FormalismType formalism,
      const std::string& input,
      const std::string& dataGraph);

  // Batch execution
  std::vector<UnifiedValidationResult> executeAllGoldenTests(
      FormalismType formalism);
  std::vector<bool> executeAllNegativeTests(FormalismType formalism);
  std::vector<EquivalenceTestResult> executeAllEquivalenceTests();

 private:
  QueryExecutionContext* ctx_;
  TestCorpusLoader loader_;
  DeterminismTester determinismTester_;
  EquivalenceTester equivalenceTester_;

  // Formalism-specific execution
  UnifiedValidationResult executeShaclTest(const GoldenTestCase& testCase);
  UnifiedValidationResult executeN3Test(const GoldenTestCase& testCase);
  UnifiedValidationResult executeDatalogTest(const GoldenTestCase& testCase);
  UnifiedValidationResult executeShExTest(const GoldenTestCase& testCase);
};

// =============================================================================
// Test Utilities
// =============================================================================

namespace util {

/**
 * Compute SHA256 fingerprint of a result.
 * Used for determinism validation.
 */
std::string computeResultFingerprint(const Result& result);

/**
 * Compute SHA256 fingerprint of an IdTable.
 * Canonical serialization for deterministic hashing.
 */
std::string computeIdTableFingerprint(const IdTable& table);

/**
 * Parse formalism input into internal representation.
 * Handles all supported formats (Turtle, JSON-LD, Datalog text, etc.)
 */
template<typename FormalismAST>
FormalismAST parseFormalismInput(
    FormalismType formalism,
    const std::string& inputFormat,
    const std::string& input);

/**
 * Convert formalism-specific result to unified result.
 * Maps SHACL violations, N3 compliance issues, Datalog tuples to common schema.
 */
UnifiedValidationResult convertToUnifiedResult(
    FormalismType formalism,
    const auto& formalismSpecificResult);

/**
 * Compare two results for equivalence.
 * Handles different result formats and checks semantic equivalence.
 */
bool areResultsEquivalent(const Result& r1, const Result& r2);

}  // namespace util

}  // namespace formalism::unified

#endif  // QLEVER_TEST_ENGINE_FORMALISM_UNIFIED_UNIFIEDFORMALISMTESTFRAMEWORK_H

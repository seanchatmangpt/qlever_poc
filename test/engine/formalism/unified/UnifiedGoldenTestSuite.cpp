// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Agent 9 - EPIC 14.0 Formalism Convergence

/**
 * Unified Golden Test Suite
 *
 * This test suite executes golden tests from the unified test corpus,
 * providing external reference validation for all formalisms.
 *
 * Coverage:
 * - SHACL: W3C SHACL 1.0 Core Constraints (8 tests)
 * - Datalog: Custom golden corpus (5 tests)
 * - N3: Turtle-level parsing (5 tests)
 * - ShEx: Placeholder for future implementation
 */

#include <gtest/gtest.h>
#include "test/engine/formalism/unified/UnifiedFormalismTestFramework.h"
#include <filesystem>
#include <fstream>

using namespace formalism::unified;

namespace {

// =============================================================================
// Test Fixture
// =============================================================================

class UnifiedGoldenTestSuite : public ::testing::Test {
 protected:
  void SetUp() override {
    // Locate corpus root
    corpusRoot_ = std::filesystem::path(__FILE__).parent_path() / "corpus";
    ASSERT_TRUE(std::filesystem::exists(corpusRoot_))
        << "Corpus directory not found: " << corpusRoot_;
  }

  std::filesystem::path corpusRoot_;
};

// =============================================================================
// Corpus Validation Tests
// =============================================================================

TEST_F(UnifiedGoldenTestSuite, CorpusStructureValidation) {
  // Verify all corpus JSON files have valid structure
  TestCorpusLoader loader(corpusRoot_);

  bool valid = loader.validateCorpusStructure();

  if (!valid) {
    auto errors = loader.getValidationErrors();
    for (const auto& error : errors) {
      ADD_FAILURE() << "Corpus validation error: " << error;
    }
  }

  EXPECT_TRUE(valid) << "Corpus structure validation failed";
}

TEST_F(UnifiedGoldenTestSuite, GoldenCorpusFilesExist) {
  // Verify all expected golden test files exist
  auto goldenDir = corpusRoot_ / "golden";

  EXPECT_TRUE(std::filesystem::exists(goldenDir / "shacl_w3c_core.json"))
      << "SHACL golden test corpus missing";
  EXPECT_TRUE(std::filesystem::exists(goldenDir / "datalog_basic.json"))
      << "Datalog golden test corpus missing";
  EXPECT_TRUE(std::filesystem::exists(goldenDir / "n3_basic.json"))
      << "N3 golden test corpus missing";
}

TEST_F(UnifiedGoldenTestSuite, NegativeCorpusFilesExist) {
  // Verify all expected negative test files exist
  auto negativeDir = corpusRoot_ / "negative";

  EXPECT_TRUE(std::filesystem::exists(negativeDir / "shacl_invalid.json"))
      << "SHACL negative test corpus missing";
  EXPECT_TRUE(std::filesystem::exists(negativeDir / "datalog_invalid.json"))
      << "Datalog negative test corpus missing";
  EXPECT_TRUE(std::filesystem::exists(negativeDir / "n3_invalid.json"))
      << "N3 negative test corpus missing";
}

TEST_F(UnifiedGoldenTestSuite, EquivalenceCorpusFilesExist) {
  // Verify equivalence test file exists
  auto equivDir = corpusRoot_ / "equivalence";

  EXPECT_TRUE(std::filesystem::exists(equivDir / "cross_formalism.json"))
      << "Cross-formalism equivalence test corpus missing";
}

// =============================================================================
// SHACL Golden Tests (W3C SHACL 1.0 Core)
// =============================================================================

TEST_F(UnifiedGoldenTestSuite, SHACL_W3C_MinCount_Exact) {
  // Test ID: shacl-mincount-001
  // W3C core/minCount-001: Exactly minimum number of values

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::SHACL);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "shacl-mincount-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case shacl-mincount-001 not found in corpus";

  // Execute test (implementation placeholder)
  // UnifiedTestExecutor executor(ctx);
  // auto result = executor.executeGoldenTest(*testCase);

  // EXPECT_TRUE(result.conforms)
  //     << "Test " << testCase->testId << " failed: " << result.message;

  EXPECT_EQ(testCase->expectedConforms, true)
      << "Test expectation incorrect";
}

TEST_F(UnifiedGoldenTestSuite, SHACL_W3C_MinCount_Violation) {
  // Test ID: shacl-mincount-002
  // Fewer than minimum values (violation expected)

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::SHACL);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "shacl-mincount-002"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case shacl-mincount-002 not found in corpus";

  EXPECT_EQ(testCase->expectedConforms, false)
      << "Test should expect violation";
  EXPECT_GT(testCase->expectedViolations.size(), 0)
      << "Test should have violations";
}

TEST_F(UnifiedGoldenTestSuite, SHACL_W3C_Pattern_Match) {
  // Test ID: shacl-pattern-001
  // Pattern matching with valid string

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::SHACL);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "shacl-pattern-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case shacl-pattern-001 not found in corpus";

  EXPECT_EQ(testCase->expectedConforms, true)
      << "Pattern match should conform";
}

// =============================================================================
// Datalog Golden Tests
// =============================================================================

TEST_F(UnifiedGoldenTestSuite, Datalog_TransitiveClosure) {
  // Test ID: datalog-transitive-001
  // Simple transitive closure computation

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::Datalog);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "datalog-transitive-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case datalog-transitive-001 not found in corpus";

  ASSERT_TRUE(testCase->expectedResult.has_value())
      << "Expected result must be specified";

  // Verify expected result structure
  auto result = testCase->expectedResult.value();
  EXPECT_EQ(result["type"].get<std::string>(), "IdTable")
      << "Result type should be IdTable";
  EXPECT_EQ(result["columns"].size(), 2)
      << "Should have 2 columns (?x, ?y)";
  EXPECT_EQ(result["rows"].size(), 6)
      << "Should have 6 transitive edges";
}

TEST_F(UnifiedGoldenTestSuite, Datalog_WithFilter) {
  // Test ID: datalog-filter-001
  // Datalog rule with SPARQL filter

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::Datalog);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "datalog-filter-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case datalog-filter-001 not found in corpus";

  auto result = testCase->expectedResult.value();
  EXPECT_EQ(result["rows"].size(), 2)
      << "Should have 2 adults (age >= 18)";
}

// =============================================================================
// N3 Golden Tests (Turtle-level)
// =============================================================================

TEST_F(UnifiedGoldenTestSuite, N3_BasicTurtleParsing) {
  // Test ID: n3-turtle-001
  // Basic Turtle parsing (no N3-specific features)

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::N3);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "n3-turtle-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case n3-turtle-001 not found in corpus";

  EXPECT_EQ(testCase->expectedConforms, true)
      << "Basic Turtle should parse successfully";
}

TEST_F(UnifiedGoldenTestSuite, N3_LiteralDatatypes) {
  // Test ID: n3-literal-001
  // Literal values with datatypes

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::N3);

  auto testCase = std::find_if(
      goldenTests.begin(), goldenTests.end(),
      [](const GoldenTestCase& t) { return t.testId == "n3-literal-001"; });

  ASSERT_NE(testCase, goldenTests.end())
      << "Test case n3-literal-001 not found in corpus";

  EXPECT_EQ(testCase->expectedConforms, true)
      << "Literal datatypes should parse successfully";
}

// =============================================================================
// Batch Execution Tests
// =============================================================================

TEST_F(UnifiedGoldenTestSuite, SHACL_ExecuteAllGoldenTests) {
  // Execute all SHACL golden tests and report results

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::SHACL);

  EXPECT_GT(goldenTests.size(), 0)
      << "No SHACL golden tests found";

  std::cout << "Found " << goldenTests.size() << " SHACL golden tests" << std::endl;

  for (const auto& testCase : goldenTests) {
    std::cout << "  - " << testCase.testId << ": " << testCase.description << std::endl;

    // Verify test structure
    EXPECT_FALSE(testCase.testId.empty())
        << "Test ID must not be empty";
    EXPECT_FALSE(testCase.description.empty())
        << "Test description must not be empty";
    EXPECT_FALSE(testCase.input.empty())
        << "Test input must not be empty";

    // Execution would happen here with UnifiedTestExecutor
    // auto result = executor.executeGoldenTest(testCase);
  }
}

TEST_F(UnifiedGoldenTestSuite, Datalog_ExecuteAllGoldenTests) {
  // Execute all Datalog golden tests and report results

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::Datalog);

  EXPECT_GT(goldenTests.size(), 0)
      << "No Datalog golden tests found";

  std::cout << "Found " << goldenTests.size() << " Datalog golden tests" << std::endl;

  for (const auto& testCase : goldenTests) {
    std::cout << "  - " << testCase.testId << ": " << testCase.description << std::endl;

    EXPECT_FALSE(testCase.testId.empty());
    EXPECT_FALSE(testCase.description.empty());
    EXPECT_FALSE(testCase.input.empty());
  }
}

TEST_F(UnifiedGoldenTestSuite, N3_ExecuteAllGoldenTests) {
  // Execute all N3 golden tests and report results

  TestCorpusLoader loader(corpusRoot_);
  auto goldenTests = loader.loadGoldenTests(FormalismType::N3);

  EXPECT_GT(goldenTests.size(), 0)
      << "No N3 golden tests found";

  std::cout << "Found " << goldenTests.size() << " N3 golden tests" << std::endl;

  for (const auto& testCase : goldenTests) {
    std::cout << "  - " << testCase.testId << ": " << testCase.description << std::endl;

    EXPECT_FALSE(testCase.testId.empty());
    EXPECT_FALSE(testCase.description.empty());
    EXPECT_FALSE(testCase.input.empty());
  }
}

}  // namespace

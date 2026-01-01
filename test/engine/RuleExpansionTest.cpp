//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 3)

#include <gtest/gtest.h>

#include <memory>

#include "engine/QueryExecutionTree.h"
#include "engine/RuleExpansion.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "parser/SparqlTriple.h"
#include "rdfTypes/Variable.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

namespace {

using ad_utility::testing::getQec;

// Helper to create a simple triple
SparqlTriple makeTriple(const std::string& s, const std::string& p,
                        const std::string& o) {
  auto parseComponent = [](const std::string& str) -> TripleComponent {
    if (str[0] == '?') {
      return TripleComponent(Variable(str));
    } else if (str[0] == '<') {
      return TripleComponent(
          ad_utility::triple_component::Iri::fromIriref(str));
    } else {
      return TripleComponent(str);
    }
  };

  auto subject = parseComponent(s);
  auto predicate = parseComponent(p);
  auto object = parseComponent(o);

  SparqlTripleSimple simple(subject, predicate, object);
  return SparqlTriple::fromSimple(simple);
}

// Test fixture
class RuleExpansionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ruleDb_ = std::make_shared<RuleDatabase>();
    qec_ = getQec();
  }

  std::shared_ptr<RuleDatabase> ruleDb_;
  std::shared_ptr<QueryExecutionContext> qec_;
};

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, BasicConstruction) {
  // Create a simple rule: parent(?x, ?y) :- triple(?x, <parentOf>, ?y)
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});

  ruleDb_->addRule(rule);

  // Create RuleExpansion with arguments
  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "parent", args);

  // Check basic properties
  EXPECT_EQ(expansion.getResultWidth(), 2);
  EXPECT_FALSE(expansion.knownEmptyResult());
  EXPECT_THAT(expansion.getDescriptor(),
              ::testing::HasSubstr("RuleExpansion parent"));
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, SimpleRuleExpansion) {
  // Rule: parent(?x, ?y) :- triple(?x, <parentOf>, ?y)
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});

  ruleDb_->addRule(rule);

  // Query with variables
  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "parent", args);

  // Verify variable mapping is created
  auto varMap = expansion.computeVariableToColumnMap();
  EXPECT_EQ(varMap.size(), 2);
  EXPECT_TRUE(varMap.contains(Variable("?a")));
  EXPECT_TRUE(varMap.contains(Variable("?b")));
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, VariableMappingWithConstants) {
  // Rule: knows(?x, ?y) :- triple(?x, <knows>, ?y)
  DatalogRule rule("knows", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<knows>", "?y")});

  ruleDb_->addRule(rule);

  // Query with one constant
  std::vector<TripleComponent> args = {
      TripleComponent(
          ad_utility::triple_component::Iri::fromIriref("<Alice>")),
      TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "knows", args);

  // Only one variable in the result
  EXPECT_EQ(expansion.getResultWidth(), 1);

  auto varMap = expansion.computeVariableToColumnMap();
  EXPECT_EQ(varMap.size(), 1);
  EXPECT_TRUE(varMap.contains(Variable("?b")));
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, MultipleBodyPatterns) {
  // Rule: grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z)
  // Represented as triples
  DatalogRule rule("grandparent", {Variable("?x"), Variable("?z")},
                   {makeTriple("?x", "<parentOf>", "?y"),
                    makeTriple("?y", "<parentOf>", "?z")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?c"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "grandparent", args);

  // Should have 2 variables in result
  EXPECT_EQ(expansion.getResultWidth(), 2);
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, NoRulesForPredicate) {
  // No rules added for "unknown"

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "unknown", args);

  // Should be known empty
  EXPECT_TRUE(expansion.knownEmptyResult());
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, ArityMismatch) {
  // Rule with 2 variables
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<related>", "?y")});

  ruleDb_->addRule(rule);

  // Try to call with wrong arity (3 arguments instead of 2)
  std::vector<TripleComponent> args = {
      TripleComponent(Variable("?a")), TripleComponent(Variable("?b")),
      TripleComponent(Variable("?c"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "test", args);

  // This should throw when trying to compute the result due to arity mismatch
  // We test that the expansion detects this
  EXPECT_ANY_THROW(expansion.computeResultOnlyForTesting());
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, CacheKeyGeneration) {
  // Rule: test(?x, ?y)
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args1 = {TripleComponent(Variable("?a")),
                                        TripleComponent(Variable("?b"))};

  std::vector<TripleComponent> args2 = {TripleComponent(Variable("?x")),
                                        TripleComponent(Variable("?y"))};

  RuleExpansion exp1(qec_.get(), ruleDb_, "test", args1);
  RuleExpansion exp2(qec_.get(), ruleDb_, "test", args2);

  // Cache keys should be different (different variable names)
  EXPECT_NE(exp1.getCacheKey(), exp2.getCacheKey());
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, GetChildren) {
  // Simple rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "test", args);

  // Before expansion, no children
  EXPECT_EQ(expansion.getChildren().size(), 0);

  // Note: After computeResult is called, there would be children
  // but we can't test that easily without a real index
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, Cloning) {
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion original(qec_.get(), ruleDb_, "test", args);

  auto cloned = original.clone();

  // Cloned operation should have same properties
  EXPECT_EQ(original.getDescriptor(), cloned->getDescriptor());
  EXPECT_EQ(original.getCacheKey(), cloned->getCacheKey());
  EXPECT_EQ(original.getResultWidth(), cloned->getResultWidth());
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, EmptyBodyPatterns) {
  // Invalid rule with no body patterns
  DatalogRule rule("empty", {Variable("?x")}, {});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "empty", args);

  // Should throw when trying to build execution tree from empty patterns
  EXPECT_ANY_THROW(expansion.computeResultOnlyForTesting());
}

// _____________________________________________________________________________
TEST_F(RuleExpansionTest, MultiplicityEstimation) {
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  RuleExpansion expansion(qec_.get(), ruleDb_, "test", args);

  // Get multiplicity for column 0
  float mult = expansion.getMultiplicity(0);

  // Should return a positive value (default is 1.0)
  EXPECT_GT(mult, 0.0f);
}

}  // namespace

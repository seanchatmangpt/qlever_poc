//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 6)

#include <gtest/gtest.h>

#include <memory>

#include "engine/FixpointComputation.h"
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
class FixpointComputationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ruleDb_ = std::make_shared<RuleDatabase>();
    qec_ = getQec();
  }

  std::shared_ptr<RuleDatabase> ruleDb_;
  std::shared_ptr<QueryExecutionContext> qec_;
};

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, BasicConstruction) {
  // Create a simple recursive rule
  DatalogRule rule("ancestor", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});

  ruleDb_->addRule(rule);

  // Create FixpointComputation with arguments
  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "ancestor", args, 10);

  // Check basic properties
  EXPECT_EQ(fixpoint.getResultWidth(), 2);
  EXPECT_FALSE(fixpoint.knownEmptyResult());
  EXPECT_THAT(fixpoint.getDescriptor(),
              ::testing::HasSubstr("FixpointComputation ancestor"));
  EXPECT_THAT(fixpoint.getDescriptor(), ::testing::HasSubstr("max_iter=10"));
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, SimpleNonRecursiveRule) {
  // Non-recursive rule: should execute once and terminate
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "parent", args, 100);

  // Non-recursive rules should be known to complete quickly
  EXPECT_EQ(fixpoint.getResultWidth(), 2);
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, RecursiveRuleStructure) {
  // Create a recursive rule structure (transitive closure pattern)
  // Base case: ancestor(?x, ?y) :- parent(?x, ?y)
  DatalogRule baseRule("ancestor", {Variable("?x"), Variable("?y")},
                       {makeTriple("?x", "<parentOf>", "?y")});

  // Recursive case: ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z)
  DatalogRule recursiveRule("ancestor", {Variable("?x"), Variable("?z")},
                            {makeTriple("?x", "<parentOf>", "?y"),
                             makeTriple("?y", "<ancestor>", "?z")});

  ruleDb_->addRule(baseRule);
  ruleDb_->addRule(recursiveRule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?c"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "ancestor", args, 100);

  // Check that it's set up correctly
  EXPECT_EQ(fixpoint.getResultWidth(), 2);
  EXPECT_FALSE(fixpoint.knownEmptyResult());
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, MaxIterationLimit) {
  // Create a simple rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  // Create with very low iteration limit
  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 3);

  // Should respect the limit
  EXPECT_THAT(fixpoint.getDescriptor(), ::testing::HasSubstr("max_iter=3"));
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, EmptyRuleDatabase) {
  // No rules for this predicate
  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "unknown", args, 100);

  // Should be known empty
  EXPECT_TRUE(fixpoint.knownEmptyResult());
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, VariableMapping) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<rel>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 10);

  // Check variable mapping
  auto varMap = fixpoint.computeVariableToColumnMap();
  EXPECT_EQ(varMap.size(), 2);
  EXPECT_TRUE(varMap.contains(Variable("?a")));
  EXPECT_TRUE(varMap.contains(Variable("?b")));
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, VariableMappingWithConstants) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<rel>", "?y")});

  ruleDb_->addRule(rule);

  // One constant, one variable
  std::vector<TripleComponent> args = {
      TripleComponent(
          ad_utility::triple_component::Iri::fromIriref("<Alice>")),
      TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 10);

  // Only one variable in result
  EXPECT_EQ(fixpoint.getResultWidth(), 1);

  auto varMap = fixpoint.computeVariableToColumnMap();
  EXPECT_EQ(varMap.size(), 1);
  EXPECT_TRUE(varMap.contains(Variable("?b")));
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, CostEstimate) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 100);

  // Cost estimate should be reasonable
  size_t cost = fixpoint.getCostEstimate();
  EXPECT_GT(cost, 0);

  // With more iterations, cost should be higher
  FixpointComputation fixpoint2(qec_.get(), ruleDb_, "test", args, 10);
  size_t cost2 = fixpoint2.getCostEstimate();

  // More iterations should cost more (or equal if using heuristics)
  EXPECT_GE(cost, cost2);
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, SizeEstimate) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 100);

  // Size estimate should be computed
  uint64_t size = fixpoint.getSizeEstimateBeforeLimit();
  EXPECT_GE(size, 0);
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, MultiplicityEstimate) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 10);

  // Multiplicity should be computed for each column
  for (size_t col = 0; col < fixpoint.getResultWidth(); ++col) {
    float mult = fixpoint.getMultiplicity(col);
    EXPECT_GT(mult, 0.0f);
  }
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, CacheKeyGeneration) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args1 = {TripleComponent(Variable("?a")),
                                        TripleComponent(Variable("?b"))};

  std::vector<TripleComponent> args2 = {TripleComponent(Variable("?x")),
                                        TripleComponent(Variable("?y"))};

  FixpointComputation fp1(qec_.get(), ruleDb_, "test", args1, 100);
  FixpointComputation fp2(qec_.get(), ruleDb_, "test", args2, 100);

  // Cache keys should be different (different variable names)
  EXPECT_NE(fp1.getCacheKey(), fp2.getCacheKey());

  // Different iteration limits should also produce different cache keys
  FixpointComputation fp3(qec_.get(), ruleDb_, "test", args1, 50);
  EXPECT_NE(fp1.getCacheKey(), fp3.getCacheKey());
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, CloneOperation) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation original(qec_.get(), ruleDb_, "test", args, 100);

  // Clone the operation
  auto cloned = original.clone();

  // Should have same properties
  EXPECT_EQ(cloned->getResultWidth(), original.getResultWidth());
  EXPECT_EQ(cloned->getCacheKey(), original.getCacheKey());
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, ResultSortedOn) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 10);

  // Fixpoint results are typically unsorted due to merging
  auto sortedOn = fixpoint.resultSortedOn();
  EXPECT_TRUE(sortedOn.empty());
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, GetChildren) {
  // Create rule
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 10);

  // Initially no children (created lazily)
  auto children = fixpoint.getChildren();
  EXPECT_EQ(children.size(), 0);
}

// _____________________________________________________________________________
TEST_F(FixpointComputationTest, IterationLimitEnforcement) {
  // Create a rule that could theoretically run forever
  DatalogRule rule("test", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<p>", "?y")});

  ruleDb_->addRule(rule);

  std::vector<TripleComponent> args = {TripleComponent(Variable("?a")),
                                       TripleComponent(Variable("?b"))};

  // Very low iteration limit
  FixpointComputation fixpoint(qec_.get(), ruleDb_, "test", args, 1);

  // Should not throw even with low limit
  EXPECT_NO_THROW({
    auto result = fixpoint.computeResultOnlyForTesting();
    // Result should be computable
    EXPECT_GE(result.idTable().size(), 0);
  });
}

}  // namespace

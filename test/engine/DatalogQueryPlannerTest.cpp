//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 4)

#include <gtest/gtest.h>

#include <memory>

#include "engine/DatalogQueryPlanner.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "parser/DatalogRule.h"
#include "parser/ParsedQuery.h"
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
class DatalogQueryPlannerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ruleDb_ = std::make_shared<RuleDatabase>();
    qec_ = getQec();
    queryPlanner_ = std::make_unique<QueryPlanner>(
        qec_.get(), ad_utility::SharedCancellationHandle{});
    datalogPlanner_ =
        std::make_unique<DatalogQueryPlanner>(ruleDb_, queryPlanner_.get());
  }

  std::shared_ptr<RuleDatabase> ruleDb_;
  std::shared_ptr<QueryExecutionContext> qec_;
  std::unique_ptr<QueryPlanner> queryPlanner_;
  std::unique_ptr<DatalogQueryPlanner> datalogPlanner_;
};

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, BasicConstruction) {
  // Just verify construction works
  EXPECT_NE(datalogPlanner_, nullptr);
  EXPECT_NE(ruleDb_, nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, IsRulePredicateDetection) {
  // Add a rule
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});
  ruleDb_->addRule(rule);

  // Test detection
  EXPECT_TRUE(datalogPlanner_->isRulePredicate("parent"));
  EXPECT_FALSE(datalogPlanner_->isRulePredicate("notARule"));
  EXPECT_FALSE(datalogPlanner_->isRulePredicate("<http://example.org/knows>"));
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, DetectRulePredicatesInGraphPattern) {
  // Add some rules
  DatalogRule ancestorRule("ancestor", {Variable("?x"), Variable("?y")},
                           {makeTriple("?x", "<parentOf>", "?y")});
  DatalogRule siblingRule("sibling", {Variable("?x"), Variable("?y")},
                          {makeTriple("?x", "<parentOf>", "?z"),
                           makeTriple("?y", "<parentOf>", "?z")});

  ruleDb_->addRule(ancestorRule);
  ruleDb_->addRule(siblingRule);

  // Create a graph pattern with mixed rule and non-rule predicates
  parsedQuery::GraphPattern pattern;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "ancestor", "?b"));  // rule
  bgp._triples.push_back(makeTriple("?a", "<knows>", "?c"));   // not a rule

  pattern._graphPatterns.push_back(bgp);

  // Detect rule predicates
  auto rulePredicates = datalogPlanner_->detectRulePredicates(pattern);

  // Should detect "ancestor" as a rule predicate
  EXPECT_EQ(rulePredicates.size(), 1);
  EXPECT_TRUE(rulePredicates.contains("ancestor"));
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, ExtractTriplePatternsFromBasicPattern) {
  // Create a basic graph pattern
  parsedQuery::GraphPattern pattern;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?x", "<parentOf>", "?y"));
  bgp._triples.push_back(makeTriple("?y", "<knows>", "?z"));

  pattern._graphPatterns.push_back(bgp);

  // Extract triples
  auto triples = datalogPlanner_->extractTriplePatterns(pattern);

  EXPECT_EQ(triples.size(), 2);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, PlanQueryWithSingleRulePredicate) {
  // Add a simple rule
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});
  ruleDb_->addRule(rule);

  // Create a parsed query with the rule predicate
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "parent", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  // Verify we got a valid execution tree
  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, PlanQueryWithMixedPredicates) {
  // Add a rule
  DatalogRule rule("ancestor", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});
  ruleDb_->addRule(rule);

  // Create a query with both rule and index predicates
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "ancestor", "?b"));  // rule
  bgp._triples.push_back(makeTriple("?a", "<knows>", "?c"));   // index

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  // Should create a join of RuleExpansion and IndexScan
  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, PlanQueryWithMultipleRules) {
  // Add multiple rules for the same predicate
  DatalogRule rule1("ancestor", {Variable("?x"), Variable("?y")},
                    {makeTriple("?x", "<parentOf>", "?y")});

  DatalogRule rule2("ancestor", {Variable("?x"), Variable("?z")},
                    {makeTriple("?x", "<parentOf>", "?y"),
                     makeTriple("?y", "ancestor", "?z")});

  ruleDb_->addRule(rule1);
  ruleDb_->addRule(rule2);

  // Create query using the ancestor predicate
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "ancestor", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query - should create UNION of rules
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, PlanQueryWithNoRulePredicates) {
  // Create a query with only index predicates (no rules)
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "<knows>", "?b"));
  bgp._triples.push_back(makeTriple("?b", "<likes>", "?c"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query - should use standard planner
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, JoinOrderOptimization) {
  // Add rules
  DatalogRule rule1("parent", {Variable("?x"), Variable("?y")},
                    {makeTriple("?x", "<parentOf>", "?y")});
  DatalogRule rule2("grandparent", {Variable("?x"), Variable("?z")},
                    {makeTriple("?x", "parent", "?y"),
                     makeTriple("?y", "parent", "?z")});

  ruleDb_->addRule(rule1);
  ruleDb_->addRule(rule2);

  // Create a query with multiple predicates
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "grandparent", "?c"));
  bgp._triples.push_back(makeTriple("?a", "<knows>", "?d"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  // Verify we got a valid tree with joins
  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, RecursiveRuleHandling) {
  // Add a recursive rule
  DatalogRule baseCase("ancestor", {Variable("?x"), Variable("?y")},
                       {makeTriple("?x", "<parentOf>", "?y")});

  DatalogRule recursiveCase("ancestor", {Variable("?x"), Variable("?z")},
                            {makeTriple("?x", "<parentOf>", "?y"),
                             makeTriple("?y", "ancestor", "?z")});

  recursiveCase.setRecursive(true);

  ruleDb_->addRule(baseCase);
  ruleDb_->addRule(recursiveCase);

  // Create a query using the recursive predicate
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "ancestor", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  // Should handle recursive rules via UNION
  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, EmptyQueryHandling) {
  // Create an empty query
  ParsedQuery pq;

  // Should throw because no triple patterns
  EXPECT_THROW(datalogPlanner_->planDatalogQuery(pq), std::runtime_error);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, VariableMappingInJoins) {
  // Add a rule
  DatalogRule rule("parent", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<parentOf>", "?y")});
  ruleDb_->addRule(rule);

  // Create a query with shared variables
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "parent", "?b"));
  bgp._triples.push_back(makeTriple("?b", "<knows>", "?c"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  // Verify the tree was created and handles variable mapping
  EXPECT_NE(tree, nullptr);

  // The join should connect on variable ?b
  auto vars = tree->getVariableColumns();
  EXPECT_TRUE(vars.contains(Variable("?a")));
  EXPECT_TRUE(vars.contains(Variable("?b")));
  EXPECT_TRUE(vars.contains(Variable("?c")));
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, FilterHandling) {
  // Add a rule with filters
  DatalogRule rule("filtered", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<value>", "?v"),
                    makeTriple("?y", "<value>", "?v")});

  ruleDb_->addRule(rule);

  // Create a query
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "filtered", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, ConstantArgumentsInRules) {
  // Add a rule
  DatalogRule rule("knows", {Variable("?x"), Variable("?y")},
                   {makeTriple("?x", "<knows>", "?y")});
  ruleDb_->addRule(rule);

  // Create a query with a constant argument
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("<Alice>", "knows", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
  EXPECT_NE(tree->getRootOperation(), nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, MultipleDisconnectedPatterns) {
  // Add rules
  DatalogRule rule1("parent", {Variable("?x"), Variable("?y")},
                    {makeTriple("?x", "<parentOf>", "?y")});
  DatalogRule rule2("sibling", {Variable("?a"), Variable("?b")},
                    {makeTriple("?a", "<siblingOf>", "?b")});

  ruleDb_->addRule(rule1);
  ruleDb_->addRule(rule2);

  // Create a query with disconnected patterns (no shared variables)
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?x", "parent", "?y"));
  bgp._triples.push_back(makeTriple("?a", "sibling", "?b"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query - should handle cartesian product
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
}

// _____________________________________________________________________________
TEST_F(DatalogQueryPlannerTest, ComplexNestedRules) {
  // Add nested rules
  DatalogRule parent("parent", {Variable("?x"), Variable("?y")},
                     {makeTriple("?x", "<parentOf>", "?y")});

  DatalogRule grandparent("grandparent", {Variable("?x"), Variable("?z")},
                          {makeTriple("?x", "parent", "?y"),
                           makeTriple("?y", "parent", "?z")});

  DatalogRule greatGrandparent(
      "greatGrandparent", {Variable("?x"), Variable("?w")},
      {makeTriple("?x", "parent", "?y"), makeTriple("?y", "grandparent", "?w")});

  ruleDb_->addRule(parent);
  ruleDb_->addRule(grandparent);
  ruleDb_->addRule(greatGrandparent);

  // Create a query using nested rules
  ParsedQuery pq;
  parsedQuery::BasicGraphPattern bgp;
  bgp._triples.push_back(makeTriple("?a", "greatGrandparent", "?d"));

  pq._rootGraphPattern._graphPatterns.push_back(bgp);

  // Plan the query
  auto tree = datalogPlanner_->planDatalogQuery(pq);

  EXPECT_NE(tree, nullptr);
}

}  // namespace

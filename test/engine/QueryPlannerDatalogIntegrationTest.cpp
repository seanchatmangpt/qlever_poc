//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 5)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include "engine/QueryPlanner.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "parser/SparqlParser.h"
#include "util/GTestHelpers.h"

namespace {

// Helper function to create a QueryPlanner for testing
QueryPlanner makeQueryPlanner() {
  return QueryPlanner{ad_utility::testing::getQec(),
                      std::make_shared<ad_utility::CancellationHandle<>>()};
}

// Helper function to parse a SPARQL query
ParsedQuery parseQuery(const std::string& query) {
  static EncodedIriManager iriManager;
  return SparqlParser::parseQuery(&iriManager, query);
}

// Helper function to create a simple RuleDatabase with test rules
std::shared_ptr<RuleDatabase> createTestRuleDatabase() {
  auto db = std::make_shared<RuleDatabase>();

  // Add a simple ancestor rule
  // ancestor(?x, ?y) :- parent(?x, ?y).
  DatalogRule rule1(
      "<http://example.org/ancestor>",
      {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(Variable{"?x"},
                    TripleComponent::Iri::fromIriref("<http://example.org/parent>"),
                    Variable{"?y"})});
  db->addRule(rule1);

  // Add a recursive ancestor rule
  // ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  DatalogRule rule2(
      "<http://example.org/ancestor>",
      {Variable{"?x"}, Variable{"?z"}},
      {SparqlTriple(Variable{"?x"},
                    TripleComponent::Iri::fromIriref("<http://example.org/parent>"),
                    Variable{"?y"}),
       SparqlTriple(Variable{"?y"},
                    TripleComponent::Iri::fromIriref("<http://example.org/ancestor>"),
                    Variable{"?z"})},
      {},
      true);
  db->addRule(rule2);

  return db;
}

}  // namespace

// Test QueryPlanner without RuleDatabase (standard SPARQL queries)
TEST(QueryPlannerDatalogIntegration, StandardSparqlWithoutRuleDatabase) {
  QueryPlanner planner = makeQueryPlanner();
  // RuleDatabase is not set (nullptr by default)

  // Parse a standard SPARQL query
  auto pq = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/parent> ?y "
      "}");

  // Should not have rule predicates (no RuleDatabase)
  EXPECT_FALSE(planner.hasRulePredicates(pq));

  // Should plan normally without errors
  EXPECT_NO_THROW(planner.createExecutionTree(pq));
}

// Test QueryPlanner with RuleDatabase but query has no rule predicates
TEST(QueryPlannerDatalogIntegration, SparqlQueryWithRuleDatabaseNoRules) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Query uses a predicate that is NOT defined by rules
  auto pq = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/friend> ?y "
      "}");

  // Should not detect rule predicates (friend is not in RuleDatabase)
  EXPECT_FALSE(planner.hasRulePredicates(pq));

  // Should plan as standard SPARQL (using IndexScan)
  EXPECT_NO_THROW(planner.createExecutionTree(pq));
}

// Test QueryPlanner with RuleDatabase and query has rule predicates
TEST(QueryPlannerDatalogIntegration, DatalogQueryWithRulePredicate) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Query uses the 'ancestor' predicate which is defined by rules
  auto pq = parseQuery(
      "SELECT ?x ?z WHERE { "
      "?x <http://example.org/ancestor> ?z "
      "}");

  // Should detect rule predicates
  EXPECT_TRUE(planner.hasRulePredicates(pq));

  // Should delegate to DatalogQueryPlanner
  // Note: This will only work if DatalogQueryPlanner is properly implemented
  EXPECT_NO_THROW(planner.createExecutionTree(pq));
}

// Test mixed query with both rule and index predicates
TEST(QueryPlannerDatalogIntegration, MixedQueryRuleAndIndexPredicates) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Query uses both 'ancestor' (rule) and 'friend' (index)
  auto pq = parseQuery(
      "SELECT ?x ?y ?z WHERE { "
      "?x <http://example.org/ancestor> ?y . "
      "?y <http://example.org/friend> ?z "
      "}");

  // Should detect rule predicates (ancestor is a rule predicate)
  EXPECT_TRUE(planner.hasRulePredicates(pq));

  // Should delegate to DatalogQueryPlanner for handling mixed query
  EXPECT_NO_THROW(planner.createExecutionTree(pq));
}

// Test hasRulePredicates with nested graph patterns (OPTIONAL)
TEST(QueryPlannerDatalogIntegration, RulePredicateInOptional) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Rule predicate in OPTIONAL clause
  auto pq = parseQuery(
      "SELECT ?x ?y ?z WHERE { "
      "?x <http://example.org/friend> ?y . "
      "OPTIONAL { ?x <http://example.org/ancestor> ?z } "
      "}");

  // Should detect rule predicates (ancestor in OPTIONAL)
  EXPECT_TRUE(planner.hasRulePredicates(pq));
}

// Test hasRulePredicates with UNION
TEST(QueryPlannerDatalogIntegration, RulePredicateInUnion) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Rule predicate in UNION
  auto pq = parseQuery(
      "SELECT ?x ?y WHERE { "
      "{ ?x <http://example.org/friend> ?y } UNION "
      "{ ?x <http://example.org/ancestor> ?y } "
      "}");

  // Should detect rule predicates (ancestor in second part of UNION)
  EXPECT_TRUE(planner.hasRulePredicates(pq));
}

// Test hasRulePredicates with MINUS
TEST(QueryPlannerDatalogIntegration, RulePredicateInMinus) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Rule predicate in MINUS clause
  auto pq = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/friend> ?y . "
      "MINUS { ?x <http://example.org/ancestor> ?y } "
      "}");

  // Should detect rule predicates (ancestor in MINUS)
  EXPECT_TRUE(planner.hasRulePredicates(pq));
}

// Test setRuleDatabase updates the internal state
TEST(QueryPlannerDatalogIntegration, SetRuleDatabaseUpdatesState) {
  QueryPlanner planner = makeQueryPlanner();

  // Initially no RuleDatabase
  auto pq1 = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}");
  EXPECT_FALSE(planner.hasRulePredicates(pq1));

  // Set RuleDatabase
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Now should detect rule predicates
  auto pq2 = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}");
  EXPECT_TRUE(planner.hasRulePredicates(pq2));
}

// Test backward compatibility: queries work without RuleDatabase
TEST(QueryPlannerDatalogIntegration, BackwardCompatibilityNoRuleDatabase) {
  QueryPlanner planner = makeQueryPlanner();
  // Don't set RuleDatabase

  // Various SPARQL queries should work normally
  std::vector<std::string> queries = {
      "SELECT ?x WHERE { ?x <p> <o> }",
      "SELECT ?x ?y WHERE { ?x <p> ?y . ?y <q> <z> }",
      "SELECT ?x WHERE { ?x <p> ?y OPTIONAL { ?y <q> ?z } }",
      "SELECT ?x WHERE { { ?x <p> <o1> } UNION { ?x <p> <o2> } }"};

  for (const auto& query : queries) {
    auto pq = parseQuery(query);
    EXPECT_FALSE(planner.hasRulePredicates(pq));
    EXPECT_NO_THROW(planner.createExecutionTree(pq));
  }
}

// Test that hasRulePredicates returns false for queries with variable
// predicates
TEST(QueryPlannerDatalogIntegration, VariablePredicatesNotRulePredicates) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = createTestRuleDatabase();
  planner.setRuleDatabase(ruleDb);

  // Query with variable predicate
  auto pq = parseQuery("SELECT ?x ?p ?y WHERE { ?x ?p ?y }");

  // Should not detect rule predicates (predicate is a variable)
  EXPECT_FALSE(planner.hasRulePredicates(pq));
}

// Test empty RuleDatabase
TEST(QueryPlannerDatalogIntegration, EmptyRuleDatabase) {
  QueryPlanner planner = makeQueryPlanner();
  auto emptyDb = std::make_shared<RuleDatabase>();
  planner.setRuleDatabase(emptyDb);

  auto pq = parseQuery(
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}");

  // Should not detect rule predicates (RuleDatabase is empty)
  EXPECT_FALSE(planner.hasRulePredicates(pq));

  // Should plan as standard SPARQL
  EXPECT_NO_THROW(planner.createExecutionTree(pq));
}

// Test multiple rule predicates in same query
TEST(QueryPlannerDatalogIntegration, MultipleRulePredicatesInQuery) {
  QueryPlanner planner = makeQueryPlanner();
  auto ruleDb = std::make_shared<RuleDatabase>();

  // Add two different rule predicates
  DatalogRule rule1("<http://example.org/ancestor>", {Variable{"?x"}, Variable{"?y"}},
                    {SparqlTriple(Variable{"?x"},
                                  TripleComponent::Iri::fromIriref("<http://example.org/parent>"),
                                  Variable{"?y"})});
  DatalogRule rule2("<http://example.org/sibling>", {Variable{"?x"}, Variable{"?y"}},
                    {SparqlTriple(Variable{"?x"},
                                  TripleComponent::Iri::fromIriref("<http://example.org/parent>"),
                                  Variable{"?z"}),
                     SparqlTriple(Variable{"?y"},
                                  TripleComponent::Iri::fromIriref("<http://example.org/parent>"),
                                  Variable{"?z"})});

  ruleDb->addRule(rule1);
  ruleDb->addRule(rule2);
  planner.setRuleDatabase(ruleDb);

  // Query uses both rule predicates
  auto pq = parseQuery(
      "SELECT ?x ?y ?z WHERE { "
      "?x <http://example.org/ancestor> ?y . "
      "?y <http://example.org/sibling> ?z "
      "}");

  // Should detect rule predicates
  EXPECT_TRUE(planner.hasRulePredicates(pq));
}

}  // namespace

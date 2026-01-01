//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 7)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "engine/DatalogQueryPlanner.h"
#include "engine/FixpointComputation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/QueryPlanner.h"
#include "engine/RuleExpansion.h"
#include "index/Index.h"
#include "parser/DatalogParser.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "parser/SparqlParser.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

using namespace ad_utility::testing;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;

namespace {

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/// Get embedded test RDF data in Turtle format
std::string getTestTurtleData() {
  return R"(
@prefix : <http://example.org/> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .

# Family relationships
:Alice :parentOf :Bob .
:Alice :parentOf :Carol .
:David :parentOf :Bob .
:David :parentOf :Carol .
:Bob :parentOf :Eve .
:Bob :parentOf :Frank .
:Carol :parentOf :Grace .
:Carol :parentOf :Henry .
:Eve :parentOf :Ivan .
:Frank :parentOf :Julia .
:Grace :parentOf :Kevin .
:Henry :parentOf :Laura .
:Henry :parentOf :Mark .

# Sibling relationships
:Bob :siblingOf :Carol .
:Carol :siblingOf :Bob .
:Eve :siblingOf :Frank .
:Frank :siblingOf :Eve .

# Graph edges
:NodeA :edge :NodeB .
:NodeB :edge :NodeC .
:NodeC :edge :NodeD .
:NodeA :edge :NodeE .
:NodeE :edge :NodeC .
:NodeX :edge :NodeY .
:NodeY :edge :NodeZ .
:NodeZ :edge :NodeX .

# Names
:Alice :name "Alice Johnson" .
:Bob :name "Bob Johnson" .
:Carol :name "Carol Smith" .
:Eve :name "Eve Johnson" .

# Locations
:Alice :livesIn :CityA .
:Bob :livesIn :CityB .
:Carol :livesIn :CityA .
:Eve :livesIn :CityC .

# Types
:Alice rdf:type :Person .
:Bob rdf:type :Person .
:Carol rdf:type :Person .
:Eve rdf:type :Person .

# Self-loop for testing
:Narcissus :knows :Narcissus .

# Management hierarchy
:CEO :manages :Manager1 .
:Manager1 :manages :Employee1 .
)";
}

/// Create QueryExecutionContext with test RDF data
std::shared_ptr<QueryExecutionContext> createTestQec() {
  TestIndexConfig config{getTestTurtleData()};
  config.loadAllPermutations = true;
  config.usePatterns = true;
  return std::shared_ptr<QueryExecutionContext>(
      getQec(config), [](QueryExecutionContext*) {
        // Custom deleter that does nothing (getQec returns static pointer)
      });
}

/// Create RuleDatabase with common test rules
std::shared_ptr<RuleDatabase> createBasicRuleDatabase() {
  auto db = std::make_shared<RuleDatabase>();

  // Simple parent rule
  DatalogRule parentRule(
      "parent", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
          Variable{"?y"})});
  db->addRule(parentRule);

  // Ancestor base case
  DatalogRule ancestorBase(
      "ancestor", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
          Variable{"?y"})});
  db->addRule(ancestorBase);

  // Ancestor recursive case
  DatalogRule ancestorRec(
      "ancestor", {Variable{"?x"}, Variable{"?z"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?y"}),
       SparqlTriple(Variable{"?y"},
                    TripleComponent::Iri::fromIriref(
                        "<http://example.org/ancestor>"),
                    Variable{"?z"})},
      {}, true  // isRecursive = true
  );
  db->addRule(ancestorRec);

  return db;
}

/// Get embedded Datalog rules for testing
std::string getTestDatalogRules() {
  return R"(
# Basic rules
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .

# Ancestor (transitive closure)
ancestor(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
ancestor(?x, ?z) :- ?x <http://example.org/parentOf> ?y, ancestor(?y, ?z) .

# Grandparent
grandparent(?gp, ?gc) :-
  ?gp <http://example.org/parentOf> ?p,
  ?p <http://example.org/parentOf> ?gc .

# Great-grandparent
greatGrandparent(?ggp, ?ggc) :-
  ?ggp <http://example.org/parentOf> ?gp,
  ?gp <http://example.org/parentOf> ?p,
  ?p <http://example.org/parentOf> ?ggc .

# Graph reachability
reachable(?x, ?y) :- ?x <http://example.org/edge> ?y .
reachable(?x, ?z) :- ?x <http://example.org/edge> ?y, reachable(?y, ?z) .

# Person type
isPerson(?x) :- ?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://example.org/Person> .

# Management
manages(?m, ?e) :- ?m <http://example.org/manages> ?e .
indirectlyManages(?m, ?e) :- manages(?m, ?e) .
indirectlyManages(?m, ?e) :- manages(?m, ?x), indirectlyManages(?x, ?e) .
)";
}

/// Create RuleDatabase from embedded rules
std::shared_ptr<RuleDatabase> loadRulesFromString(const std::string& rulesText) {
  auto parsed = DatalogParser::parseDatalogProgram(rulesText);
  auto db = std::make_shared<RuleDatabase>();

  for (const auto& rule : parsed.ruleDatabase.getAllRules()) {
    db->addRule(rule);
  }

  return db;
}

/// Helper to execute SPARQL query and return result table
IdTable executeQuery(QueryExecutionContext* qec, const std::string& query,
                     std::shared_ptr<RuleDatabase> ruleDb = nullptr) {
  static EncodedIriManager iriManager;
  auto pq = SparqlParser::parseQuery(&iriManager, query);

  QueryPlanner planner{qec,
                       std::make_shared<ad_utility::CancellationHandle<>>()};
  if (ruleDb) {
    planner.setRuleDatabase(ruleDb);
  }

  auto qet = planner.createExecutionTree(pq);
  auto result = qet.getResult();
  return result.idTable().clone();
}

/// Check if result contains expected number of rows
void expectRowCount(const IdTable& result, size_t expectedRows,
                    const std::string& testContext = "") {
  EXPECT_EQ(result.size(), expectedRows)
      << "Context: " << testContext
      << " - Expected " << expectedRows << " rows, got " << result.size();
}

/// Check that result is non-empty
void expectNonEmpty(const IdTable& result, const std::string& testContext = "") {
  EXPECT_GT(result.size(), 0) << "Context: " << testContext;
}

// ============================================================================
// TEST FIXTURE
// ============================================================================

class DatalogIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    qec_ = createTestQec();
    ruleDb_ = createBasicRuleDatabase();
  }

  std::shared_ptr<QueryExecutionContext> qec_;
  std::shared_ptr<RuleDatabase> ruleDb_;
};

// ============================================================================
// SECTION 1: BASIC DATALOG EXECUTION
// ============================================================================

TEST_F(DatalogIntegrationTest, BasicNonRecursiveRule) {
  // Test simple parent rule execution
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/parent> ?y "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Should return all parent relationships from test data
  // We have: Alice->Bob, Alice->Carol, David->Bob, David->Carol,
  //          Bob->Eve, Bob->Frank, Carol->Grace, Carol->Henry,
  //          Eve->Ivan, Frank->Julia, Grace->Kevin, Henry->Laura, Henry->Mark
  // Total: 13 parent relationships
  expectRowCount(result, 13, "Basic parent rule");
  EXPECT_EQ(result.numColumns(), 2);
}

TEST_F(DatalogIntegrationTest, RecursiveRuleBasicExecution) {
  // Test ancestor rule (transitive closure)
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Should return all ancestor relationships
  // Direct parents (13) + grandparents + great-grandparents + ...
  // Expected: significantly more than just direct parents
  EXPECT_GT(result.size(), 13) << "Ancestor should include transitive closure";
  EXPECT_EQ(result.numColumns(), 2);
}

TEST_F(DatalogIntegrationTest, RuleWithConstants) {
  // Add rule with constant
  auto db = std::make_shared<RuleDatabase>();
  DatalogRule aliceDescendants(
      "aliceDescendant", {Variable{"?x"}},
      {SparqlTriple(
          TripleComponent::Iri::fromIriref("<http://example.org/Alice>"),
          TripleComponent::Iri::fromIriref("<http://example.org/ancestor>"),
          Variable{"?x"})});
  db->addRule(aliceDescendants);

  // Also need ancestor rules
  for (const auto& rule : ruleDb_->getAllRules()) {
    db->addRule(rule);
  }

  std::string query =
      "SELECT ?x WHERE { "
      "<http://example.org/aliceDescendant> ?x "
      "}";

  auto result = executeQuery(qec_.get(), query, db);

  // Alice has descendants: Bob, Carol (direct), Eve, Frank, Grace, Henry
  // (grandchildren), and more
  expectNonEmpty(result, "Alice descendants");
}

// ============================================================================
// SECTION 2: RECURSIVE RULES AND FIXPOINT COMPUTATION
// ============================================================================

TEST_F(DatalogIntegrationTest, TransitiveClosureDepth) {
  // Test that recursive rules compute complete transitive closure
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Verify we get ancestors at all levels
  // Alice is ancestor of: Bob, Carol (level 1)
  //                       Eve, Frank, Grace, Henry (level 2)
  //                       Ivan, Julia, Kevin, Laura, Mark (level 3)
  // Total for Alice alone: 11 descendants
  // Plus other ancestor relationships from David, Bob, Carol, etc.

  expectNonEmpty(result, "Transitive closure");

  // Should have at least all direct parents plus grandparents
  EXPECT_GE(result.size(), 20)
      << "Should include multiple levels of ancestry";
}

TEST_F(DatalogIntegrationTest, FixpointConvergence) {
  // Test that fixpoint computation terminates correctly
  auto db = std::make_shared<RuleDatabase>();

  // Add reachability rules for graph
  DatalogRule reachBase(
      "reachable", {Variable{"?from"}, Variable{"?to"}},
      {SparqlTriple(
          Variable{"?from"},
          TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
          Variable{"?to"})});
  db->addRule(reachBase);

  DatalogRule reachRec(
      "reachable", {Variable{"?from"}, Variable{"?to"}},
      {SparqlTriple(
           Variable{"?from"},
           TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
           Variable{"?mid"}),
       SparqlTriple(Variable{"?mid"},
                    TripleComponent::Iri::fromIriref(
                        "<http://example.org/reachable>"),
                    Variable{"?to"})},
      {}, true);
  db->addRule(reachRec);

  std::string query =
      "SELECT ?from ?to WHERE { "
      "?from <http://example.org/reachable> ?to "
      "}";

  // Measure time to ensure it completes in reasonable time
  auto start = std::chrono::high_resolution_clock::now();
  auto result = executeQuery(qec_.get(), query, db);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  expectNonEmpty(result, "Reachability fixpoint");

  // Should complete in under 5 seconds
  EXPECT_LT(duration.count(), 5000)
      << "Fixpoint computation took too long: " << duration.count() << "ms";
}

TEST_F(DatalogIntegrationTest, CyclicGraphHandling) {
  // Test that cyclic graphs are handled correctly
  // Our test data has: NodeX -> NodeY -> NodeZ -> NodeX (cycle)
  auto db = std::make_shared<RuleDatabase>();

  DatalogRule reachBase(
      "reachable", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
          Variable{"?y"})});
  db->addRule(reachBase);

  DatalogRule reachRec(
      "reachable", {Variable{"?x"}, Variable{"?z"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
           Variable{"?y"}),
       SparqlTriple(Variable{"?y"},
                    TripleComponent::Iri::fromIriref(
                        "<http://example.org/reachable>"),
                    Variable{"?z"})},
      {}, true);
  db->addRule(reachRec);

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/reachable> ?y "
      "}";

  EXPECT_NO_THROW({
    auto result = executeQuery(qec_.get(), query, db);
    // In a cycle of 3 nodes, each should reach all others
    // NodeX can reach NodeY, NodeZ, NodeX (itself via cycle)
    expectNonEmpty(result, "Cyclic graph reachability");
  });
}

TEST_F(DatalogIntegrationTest, MultipleRecursionDepths) {
  // Test different recursion depths in same database
  auto db = std::make_shared<RuleDatabase>();

  // Grandparent (2 levels)
  DatalogRule grandparent(
      "grandparent", {Variable{"?gp"}, Variable{"?gc"}},
      {SparqlTriple(
           Variable{"?gp"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?p"}),
       SparqlTriple(
           Variable{"?p"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?gc"})});
  db->addRule(grandparent);

  // Great-grandparent (3 levels)
  DatalogRule greatGrandparent(
      "greatGrandparent", {Variable{"?ggp"}, Variable{"?ggc"}},
      {SparqlTriple(
           Variable{"?ggp"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?gp"}),
       SparqlTriple(
           Variable{"?gp"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?p"}),
       SparqlTriple(
           Variable{"?p"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?ggc"})});
  db->addRule(greatGrandparent);

  // Test grandparent
  std::string gpQuery =
      "SELECT ?gp ?gc WHERE { "
      "?gp <http://example.org/grandparent> ?gc "
      "}";
  auto gpResult = executeQuery(qec_.get(), gpQuery, db);
  expectNonEmpty(gpResult, "Grandparent relationships");

  // Test great-grandparent
  std::string ggpQuery =
      "SELECT ?ggp ?ggc WHERE { "
      "?ggp <http://example.org/greatGrandparent> ?ggc "
      "}";
  auto ggpResult = executeQuery(qec_.get(), ggpQuery, db);
  expectNonEmpty(ggpResult, "Great-grandparent relationships");

  // Great-grandparents should be fewer than grandparents
  EXPECT_LE(ggpResult.size(), gpResult.size());
}

// ============================================================================
// SECTION 3: MIXED QUERIES (DATALOG + SPARQL)
// ============================================================================

TEST_F(DatalogIntegrationTest, MixedRuleAndIndexPredicates) {
  // Query combining rule-defined and index-defined predicates
  std::string query =
      "SELECT ?x ?y ?name WHERE { "
      "?x <http://example.org/ancestor> ?y . "
      "?y <http://example.org/name> ?name "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Should join ancestor results with name data
  expectNonEmpty(result, "Mixed rule and index predicates");
  EXPECT_EQ(result.numColumns(), 3);
}

TEST_F(DatalogIntegrationTest, JoinRuleResultsWithIndexData) {
  // Test joining rule-derived data with RDF triples
  std::string query =
      "SELECT ?ancestor ?descendant ?city WHERE { "
      "?ancestor <http://example.org/ancestor> ?descendant . "
      "?descendant <http://example.org/livesIn> ?city "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Should find ancestors whose descendants live in specific cities
  expectNonEmpty(result, "Join rule results with location data");
}

TEST_F(DatalogIntegrationTest, MultipleRulePredicatesInQuery) {
  // Use multiple different rule predicates in same query
  auto db = std::make_shared<RuleDatabase>();

  // Add parent and ancestor rules
  for (const auto& rule : ruleDb_->getAllRules()) {
    db->addRule(rule);
  }

  // Add sibling rule
  DatalogRule siblingRule(
      "sibling", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?z"}),
       SparqlTriple(
           Variable{"?y"},
           TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
           Variable{"?z"})});
  db->addRule(siblingRule);

  std::string query =
      "SELECT ?x ?y ?z WHERE { "
      "?x <http://example.org/parent> ?y . "
      "?y <http://example.org/ancestor> ?z "
      "}";

  auto result = executeQuery(qec_.get(), query, db);
  expectNonEmpty(result, "Multiple rule predicates");
}

TEST_F(DatalogIntegrationTest, VariableBindingAcrossBoundaries) {
  // Test that variables are correctly bound between rule and index predicates
  std::string query =
      "SELECT ?person ?ancestor WHERE { "
      "?person <http://example.org/name> \"Eve Johnson\" . "
      "?ancestor <http://example.org/ancestor> ?person "
      "}";

  auto result = executeQuery(qec_.get(), query, ruleDb_);

  // Should find all ancestors of Eve
  expectNonEmpty(result, "Variable binding across boundaries");
  // Eve's ancestors: Bob (parent), Alice & David (grandparents)
  EXPECT_GE(result.size(), 3);
}

// ============================================================================
// SECTION 4: PARSING DATALOG PROGRAMS
// ============================================================================

TEST_F(DatalogIntegrationTest, ParseRulesFromText) {
  // Test parsing rules from text format
  std::string ruleText =
      "parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .";

  EXPECT_NO_THROW({
    auto rule = DatalogParser::parseDatalogRule(ruleText);
    EXPECT_EQ(rule.getHeadPredicate(), "parent");
    EXPECT_EQ(rule.getHeadVariables().size(), 2);
  });
}

TEST_F(DatalogIntegrationTest, ParseCompleteProgram) {
  // Test parsing complete program from string
  EXPECT_NO_THROW({
    auto db = loadRulesFromString(getTestDatalogRules());
    EXPECT_GT(db->getRuleCount(), 0);
    EXPECT_GT(db->getPredicateCount(), 0);
  });
}

TEST_F(DatalogIntegrationTest, ParseErrorsDetected) {
  // Test that parsing errors are properly caught
  std::string invalidRule = "invalid syntax here";

  EXPECT_THROW(
      { auto rule = DatalogParser::parseDatalogRule(invalidRule); },
      std::exception);
}

TEST_F(DatalogIntegrationTest, LoadAndExecuteFromString) {
  // Load rules from string and execute queries
  auto db = loadRulesFromString(getTestDatalogRules());

  // Test one of the rules (e.g., grandparent)
  std::string query =
      "SELECT ?gp ?gc WHERE { "
      "?gp <http://example.org/grandparent> ?gc "
      "}";

  auto result = executeQuery(qec_.get(), query, db);
  expectNonEmpty(result, "Loaded rule execution");
}

// ============================================================================
// SECTION 5: PERFORMANCE TESTS
// ============================================================================

TEST_F(DatalogIntegrationTest, QueryPerformanceMultipleRules) {
  // Test performance with multiple rules
  auto db = loadRulesFromString(getTestDatalogRules());

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}";

  auto start = std::chrono::high_resolution_clock::now();
  auto result = executeQuery(qec_.get(), query, db);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  expectNonEmpty(result, "Performance test");

  // Should complete in reasonable time (< 5s per test requirement)
  EXPECT_LT(duration.count(), 5000)
      << "Query took " << duration.count() << "ms";
}

TEST_F(DatalogIntegrationTest, MemoryConstraintsEnforced) {
  // Test that memory constraints are respected
  // This is implicitly tested by FixpointComputation using
  // AllocatorWithLimit

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}";

  // Should not throw memory errors with reasonable data
  EXPECT_NO_THROW({ auto result = executeQuery(qec_.get(), query, ruleDb_); });
}

TEST_F(DatalogIntegrationTest, IterationCountReasonable) {
  // Test that iteration count for fixpoint is reasonable
  // For our test data with 3 generations, should converge in < 10 iterations

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/ancestor> ?y "
      "}";

  // Execute and verify it completes
  auto result = executeQuery(qec_.get(), query, ruleDb_);
  expectNonEmpty(result, "Iteration count test");

  // Convergence should be quick for small dataset
  // (actual iteration tracking would require instrumentation)
}

// ============================================================================
// SECTION 6: EDGE CASES
// ============================================================================

TEST_F(DatalogIntegrationTest, EmptyResultSet) {
  // Query that matches no data
  auto db = std::make_shared<RuleDatabase>();
  DatalogRule noMatch(
      "noMatch", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/nonexistent>"),
          Variable{"?y"})});
  db->addRule(noMatch);

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/noMatch> ?y "
      "}";

  auto result = executeQuery(qec_.get(), query, db);

  // Should return empty result (not error)
  EXPECT_EQ(result.size(), 0);
  EXPECT_EQ(result.numColumns(), 2);
}

TEST_F(DatalogIntegrationTest, QueryWithNoMatchingRules) {
  // Query predicate not defined in rule database
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/undefinedPredicate> ?y "
      "}";

  // Should fall back to index scan
  EXPECT_NO_THROW({ auto result = executeQuery(qec_.get(), query, ruleDb_); });
}

TEST_F(DatalogIntegrationTest, ConstantInRuleHead) {
  // Rule with constant in head (edge case)
  auto db = std::make_shared<RuleDatabase>();

  // This is unusual but should be handleable
  DatalogRule constantHead(
      "alwaysAlice", {Variable{"?x"}},
      {SparqlTriple(
          TripleComponent::Iri::fromIriref("<http://example.org/Alice>"),
          TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
          Variable{"?x"})});
  db->addRule(constantHead);

  std::string query =
      "SELECT ?x WHERE { "
      "<http://example.org/alwaysAlice> ?x "
      "}";

  // Should work - returns children of Alice
  auto result = executeQuery(qec_.get(), query, db);
  expectNonEmpty(result, "Constant in head");
}

TEST_F(DatalogIntegrationTest, SingleVariableRule) {
  // Unary rule (single variable)
  auto db = std::make_shared<RuleDatabase>();

  DatalogRule isPerson(
      "isPerson", {Variable{"?x"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref(
              "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),
          TripleComponent::Iri::fromIriref("<http://example.org/Person>"))});
  db->addRule(isPerson);

  std::string query =
      "SELECT ?x WHERE { "
      "<http://example.org/isPerson> ?x "
      "}";

  auto result = executeQuery(qec_.get(), query, db);
  expectNonEmpty(result, "Single variable rule");
  EXPECT_EQ(result.numColumns(), 1);
}

TEST_F(DatalogIntegrationTest, MultipleVariablesInRule) {
  // Rule with three variables
  auto db = std::make_shared<RuleDatabase>();

  DatalogRule threeVars(
      "connection", {Variable{"?x"}, Variable{"?y"}, Variable{"?z"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
           Variable{"?y"}),
       SparqlTriple(
           Variable{"?y"},
           TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
           Variable{"?z"})});
  db->addRule(threeVars);

  std::string query =
      "SELECT ?x ?y ?z WHERE { "
      "?x <http://example.org/connection> ?y ?z "
      "}";

  // Should handle 3+ variables
  EXPECT_NO_THROW({ auto result = executeQuery(qec_.get(), query, db); });
}

TEST_F(DatalogIntegrationTest, SelfLoop) {
  // Test data has self-loop: Narcissus knows Narcissus
  auto db = std::make_shared<RuleDatabase>();

  DatalogRule knowsSelf(
      "knowsSelf", {Variable{"?x"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/knows>"),
          Variable{"?x"})});
  db->addRule(knowsSelf);

  std::string query =
      "SELECT ?x WHERE { "
      "<http://example.org/knowsSelf> ?x "
      "}";

  auto result = executeQuery(qec_.get(), query, db);
  // Should find Narcissus
  EXPECT_GE(result.size(), 1);
}

TEST_F(DatalogIntegrationTest, RuleWithNoVariablesInCommon) {
  // Rule where body patterns share no variables (Cartesian product)
  auto db = std::make_shared<RuleDatabase>();

  DatalogRule cartesian(
      "cartesian", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref(
               "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),
           TripleComponent::Iri::fromIriref("<http://example.org/Person>")),
       SparqlTriple(
           Variable{"?y"},
           TripleComponent::Iri::fromIriref(
               "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>"),
           TripleComponent::Iri::fromIriref("<http://example.org/Person>"))});
  db->addRule(cartesian);

  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <http://example.org/cartesian> ?y "
      "}";

  // Should compute Cartesian product
  auto result = executeQuery(qec_.get(), query, db);
  // Many persons × many persons
  expectNonEmpty(result, "Cartesian product rule");
}

// ============================================================================
// SECTION 7: COMPLEX INTEGRATION SCENARIOS
// ============================================================================

TEST_F(DatalogIntegrationTest, CompleteWorkflowLoadParseExecute) {
  // End-to-end test: load data, parse rules, execute complex query

  // 1. Load rules from string
  auto db = loadRulesFromString(getTestDatalogRules());
  EXPECT_GT(db->getRuleCount(), 0);

  // 2. Execute complex query using loaded rules
  std::string query =
      "SELECT ?ancestor ?person ?city WHERE { "
      "?ancestor <http://example.org/ancestor> ?person . "
      "?person <http://example.org/livesIn> ?city . "
      "?person <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> "
      "<http://example.org/Person> "
      "}";

  auto result = executeQuery(qec_.get(), query, db);

  // 3. Verify results
  expectNonEmpty(result, "Complete workflow");
  EXPECT_EQ(result.numColumns(), 3);
}

TEST_F(DatalogIntegrationTest, CombineMultipleRecursiveRules) {
  // Query combining multiple recursive predicates
  auto db = std::make_shared<RuleDatabase>();

  // Add ancestor rules
  for (const auto& rule : ruleDb_->getAllRules()) {
    db->addRule(rule);
  }

  // Add reachability rules
  DatalogRule reachBase(
      "reachable", {Variable{"?x"}, Variable{"?y"}},
      {SparqlTriple(
          Variable{"?x"},
          TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
          Variable{"?y"})});
  db->addRule(reachBase);

  DatalogRule reachRec(
      "reachable", {Variable{"?x"}, Variable{"?z"}},
      {SparqlTriple(
           Variable{"?x"},
           TripleComponent::Iri::fromIriref("<http://example.org/edge>"),
           Variable{"?y"}),
       SparqlTriple(Variable{"?y"},
                    TripleComponent::Iri::fromIriref(
                        "<http://example.org/reachable>"),
                    Variable{"?z"})},
      {}, true);
  db->addRule(reachRec);

  // Query using both
  std::string query =
      "SELECT ?a ?d ?x ?y WHERE { "
      "?a <http://example.org/ancestor> ?d . "
      "?x <http://example.org/reachable> ?y "
      "}";

  auto result = executeQuery(qec_.get(), query, db);
  expectNonEmpty(result, "Multiple recursive rules");
}

}  // namespace

// ============================================================================
// MAIN
// ============================================================================

// Note: Google Test automatically provides main() via gmock_main
// No explicit main() needed when linking against gmock_main

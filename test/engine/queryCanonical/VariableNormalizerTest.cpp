// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2: Query Shape Canonicalization - Step C)

#include <gtest/gtest.h>

#include <string>

#include "engine/queryCanonical/VariableNormalizer.h"
#include "parser/SparqlParser.h"

namespace {

// Helper function to parse a SPARQL query
ParsedQuery parseQuery(const std::string& query) {
  return SparqlParser::parseQuery(query);
}

// _____________________________________________________________________________
// T3: Rename variables → same canonical names for logically identical queries
TEST(VariableNormalizerTest, T3_IdenticalQueriesSameCanonicalNames) {
  // Two queries with different variable names but identical structure
  std::string query1 = "SELECT ?x ?y WHERE { ?x <p> ?y }";
  std::string query2 = "SELECT ?a ?b WHERE { ?a <p> ?b }";

  auto parsed1 = parseQuery(query1);
  auto parsed2 = parseQuery(query2);

  queryCanonical::VariableNormalizer normalizer1(parsed1);
  queryCanonical::VariableNormalizer normalizer2(parsed2);

  auto map1 = normalizer1.buildVariableMap();
  auto map2 = normalizer2.buildVariableMap();

  // Both should map to ?v0 and ?v1
  EXPECT_EQ(map1.at("?x"), "?v0");
  EXPECT_EQ(map1.at("?y"), "?v1");

  EXPECT_EQ(map2.at("?a"), "?v0");
  EXPECT_EQ(map2.at("?b"), "?v1");
}

// _____________________________________________________________________________
// T3b: Different variable names, same structure → same canonical form
TEST(VariableNormalizerTest, T3b_DifferentNamesStructureSameCanonical) {
  // More complex queries with different variable names
  std::string query1 =
      "SELECT ?person ?name WHERE { "
      "?person <hasName> ?name . "
      "?person <hasAge> ?age "
      "}";

  std::string query2 =
      "SELECT ?x ?y WHERE { "
      "?x <hasName> ?y . "
      "?x <hasAge> ?z "
      "}";

  auto parsed1 = parseQuery(query1);
  auto parsed2 = parseQuery(query2);

  queryCanonical::VariableNormalizer normalizer1(parsed1);
  queryCanonical::VariableNormalizer normalizer2(parsed2);

  auto map1 = normalizer1.buildVariableMap();
  auto map2 = normalizer2.buildVariableMap();

  // Both should have the same canonical form:
  // first SELECT variable -> ?v0
  // second SELECT variable -> ?v1
  // third variable (in WHERE) -> ?v2
  EXPECT_EQ(map1.size(), 3u);
  EXPECT_EQ(map2.size(), 3u);

  EXPECT_EQ(map1.at("?person"), "?v0");
  EXPECT_EQ(map1.at("?name"), "?v1");
  EXPECT_EQ(map1.at("?age"), "?v2");

  EXPECT_EQ(map2.at("?x"), "?v0");
  EXPECT_EQ(map2.at("?y"), "?v1");
  EXPECT_EQ(map2.at("?z"), "?v2");
}

// _____________________________________________________________________________
// Test that variables are renamed in order of first appearance
TEST(VariableNormalizerTest, OrderOfFirstAppearance) {
  std::string query =
      "SELECT ?b ?a WHERE { "
      "?a <p> ?c . "
      "?b <q> ?a "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?b (SELECT), ?a (SELECT), ?c (triple 1)
  EXPECT_EQ(map.at("?b"), "?v0");
  EXPECT_EQ(map.at("?a"), "?v1");
  EXPECT_EQ(map.at("?c"), "?v2");
}

// _____________________________________________________________________________
// Test renameVariable method
TEST(VariableNormalizerTest, RenameVariableMethod) {
  std::string query = "SELECT ?x ?y WHERE { ?x <p> ?y }";
  auto parsed = parseQuery(query);

  queryCanonical::VariableNormalizer normalizer(parsed);

  Variable x("?x");
  Variable y("?y");

  Variable renamed_x = normalizer.renameVariable(x);
  Variable renamed_y = normalizer.renameVariable(y);

  EXPECT_EQ(renamed_x.name(), "?v0");
  EXPECT_EQ(renamed_y.name(), "?v1");
}

// _____________________________________________________________________________
// Test with FILTER expressions
TEST(VariableNormalizerTest, WithFilterExpressions) {
  std::string query =
      "SELECT ?x WHERE { "
      "?x <p> ?y . "
      "FILTER(?y > 5) "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?y (triple, then FILTER)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
}

// _____________________________________________________________________________
// Test with OPTIONAL clause
TEST(VariableNormalizerTest, WithOptionalClause) {
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <p> ?y . "
      "OPTIONAL { ?y <q> ?z } "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?y (SELECT/triple), ?z (OPTIONAL)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
  EXPECT_EQ(map.at("?z"), "?v2");
}

// _____________________________________________________________________________
// Test with UNION clause
TEST(VariableNormalizerTest, WithUnionClause) {
  std::string query =
      "SELECT ?x WHERE { "
      "{ ?x <p> ?y } UNION { ?x <q> ?z } "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?y (first union branch), ?z (second union branch)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
  EXPECT_EQ(map.at("?z"), "?v2");
}

// _____________________________________________________________________________
// Test with BIND clause
TEST(VariableNormalizerTest, WithBindClause) {
  std::string query =
      "SELECT ?x ?computed WHERE { "
      "?x <p> ?y . "
      "BIND(?y + 1 AS ?computed) "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?computed (SELECT), ?y (triple)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?computed"), "?v1");
  EXPECT_EQ(map.at("?y"), "?v2");
}

// _____________________________________________________________________________
// Test with VALUES clause
TEST(VariableNormalizerTest, WithValuesClause) {
  std::string query =
      "SELECT ?x ?y WHERE { "
      "VALUES (?a ?b) { (1 2) (3 4) } "
      "?x <p> ?a . "
      "?x <q> ?b "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?y (SELECT), ?a (VALUES), ?b (VALUES)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
  EXPECT_EQ(map.at("?a"), "?v2");
  EXPECT_EQ(map.at("?b"), "?v3");
}

// _____________________________________________________________________________
// Test with GROUP BY and HAVING
TEST(VariableNormalizerTest, WithGroupByAndHaving) {
  std::string query =
      "SELECT ?x (COUNT(?y) AS ?count) WHERE { "
      "?x <p> ?y "
      "} GROUP BY ?x HAVING(COUNT(?y) > 5)";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?count (SELECT/alias), ?y (expression and triple)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?count"), "?v1");
  EXPECT_EQ(map.at("?y"), "?v2");
}

// _____________________________________________________________________________
// Test with ORDER BY
TEST(VariableNormalizerTest, WithOrderBy) {
  std::string query =
      "SELECT ?x ?y WHERE { "
      "?x <p> ?y "
      "} ORDER BY ?y";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Order: ?x (SELECT), ?y (SELECT/triple/ORDER BY)
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
}

// _____________________________________________________________________________
// Test that buildVariableMap is idempotent
TEST(VariableNormalizerTest, BuildVariableMapIdempotent) {
  std::string query = "SELECT ?x ?y WHERE { ?x <p> ?y }";
  auto parsed = parseQuery(query);

  queryCanonical::VariableNormalizer normalizer(parsed);

  auto map1 = normalizer.buildVariableMap();
  auto map2 = normalizer.buildVariableMap();

  EXPECT_EQ(map1, map2);
}

// _____________________________________________________________________________
// Test with variable appearing multiple times
TEST(VariableNormalizerTest, VariableMultipleOccurrences) {
  std::string query =
      "SELECT ?x WHERE { "
      "?x <p> ?y . "
      "?y <q> ?x . "
      "?x <r> ?x "
      "}";

  auto parsed = parseQuery(query);
  queryCanonical::VariableNormalizer normalizer(parsed);
  auto map = normalizer.buildVariableMap();

  // Only 2 distinct variables: ?x and ?y
  EXPECT_EQ(map.size(), 2u);
  EXPECT_EQ(map.at("?x"), "?v0");
  EXPECT_EQ(map.at("?y"), "?v1");
}

}  // namespace

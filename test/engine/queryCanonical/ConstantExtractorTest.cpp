// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#include <gtest/gtest.h>

#include "engine/queryCanonical/ConstantExtractor.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Literal.h"

using namespace queryCanonical;
using ad_utility::triple_component::Iri;
using ad_utility::triple_component::Literal;

// Helper to parse a SPARQL query string
ParsedQuery parseQuery(const std::string& query) {
  return SparqlParser::parseQuery(query);
}

class ConstantExtractorTest : public ::testing::Test {};

// T4b: Test classification of different literal types
TEST_F(ConstantExtractorTest, ClassifyLiterals) {
  // Plain literal
  auto plainLiteral = Literal::literalWithoutQuotes("hello");
  EXPECT_EQ(ConstantExtractor::classifyConstant(plainLiteral),
            PlaceholderType::PLAIN_LITERAL);

  // Language-tagged literal
  auto langLiteral = Literal::literalWithoutQuotes("hello");
  langLiteral.addLanguageTag("en");
  EXPECT_EQ(ConstantExtractor::classifyConstant(langLiteral),
            PlaceholderType::LANG_LITERAL);

  // Boolean literal
  auto boolLiteral = Literal::literalWithoutQuotes("true");
  boolLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#boolean"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(boolLiteral),
            PlaceholderType::BOOLEAN_LITERAL);

  // Numeric literal (integer)
  auto intLiteral = Literal::literalWithoutQuotes("42");
  intLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#integer"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(intLiteral),
            PlaceholderType::NUMERIC_LITERAL);

  // Numeric literal (double)
  auto doubleLiteral = Literal::literalWithoutQuotes("3.14");
  doubleLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#double"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(doubleLiteral),
            PlaceholderType::NUMERIC_LITERAL);

  // Temporal literal (dateTime)
  auto dateLiteral = Literal::literalWithoutQuotes("2025-01-01T00:00:00");
  dateLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#dateTime"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(dateLiteral),
            PlaceholderType::TEMPORAL_LITERAL);

  // Temporal literal (date)
  auto dateOnlyLiteral = Literal::literalWithoutQuotes("2025-01-01");
  dateOnlyLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#date"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(dateOnlyLiteral),
            PlaceholderType::TEMPORAL_LITERAL);

  // Temporal literal (gYear)
  auto yearLiteral = Literal::literalWithoutQuotes("2025");
  yearLiteral.addDatatype(
      Iri::fromIrirefWithoutBrackets("http://www.w3.org/2001/XMLSchema#gYear"));
  EXPECT_EQ(ConstantExtractor::classifyConstant(yearLiteral),
            PlaceholderType::TEMPORAL_LITERAL);
}

// Test classification of IRIs
TEST_F(ConstantExtractorTest, ClassifyIris) {
  auto iri = Iri::fromIrirefWithoutBrackets("http://example.org/resource");
  EXPECT_EQ(ConstantExtractor::classifyConstant(iri), PlaceholderType::IRI);
}

// Test placeholder creation with correct indices
TEST_F(ConstantExtractorTest, CreatePlaceholders) {
  auto iri = Iri::fromIrirefWithoutBrackets("http://example.org/resource");
  auto placeholder = ConstantExtractor::replaceWithPlaceholder(iri, 0);

  EXPECT_EQ(placeholder.placeholderType, PlaceholderType::IRI);
  EXPECT_EQ(placeholder.index, 0u);
  EXPECT_EQ(placeholder.originalValue, "<http://example.org/resource>");

  auto literal = Literal::literalWithoutQuotes("test");
  auto litPlaceholder = ConstantExtractor::replaceWithPlaceholder(literal, 1);

  EXPECT_EQ(litPlaceholder.placeholderType, PlaceholderType::PLAIN_LITERAL);
  EXPECT_EQ(litPlaceholder.index, 1u);
  EXPECT_EQ(litPlaceholder.originalValue, "\"test\"");
}

// T4: Extract constants from query, verify deterministic ordering
TEST_F(ConstantExtractorTest, ExtractConstantsDeterministic) {
  // Query with multiple constants
  std::string query1 = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      ex:Alice ex:knows ex:Bob .
      ex:Bob ex:age "30"^^<http://www.w3.org/2001/XMLSchema#integer> .
    }
  )";

  ParsedQuery parsed1 = parseQuery(query1);
  ConstantExtractor extractor1(parsed1);
  auto constants1 = extractor1.extractConstants();

  // Same query structure, different constants
  std::string query2 = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      ex:Charlie ex:knows ex:Dave .
      ex:Dave ex:age "25"^^<http://www.w3.org/2001/XMLSchema#integer> .
    }
  )";

  ParsedQuery parsed2 = parseQuery(query2);
  ConstantExtractor extractor2(parsed2);
  auto constants2 = extractor2.extractConstants();

  // Both queries should extract the same number of constants
  EXPECT_EQ(constants1.size(), constants2.size());

  // Constants should be different (different values)
  EXPECT_NE(constants1, constants2);

  // But both should be sorted (deterministic)
  auto constants1_copy = constants1;
  std::sort(constants1_copy.begin(), constants1_copy.end());
  EXPECT_EQ(constants1, constants1_copy);

  auto constants2_copy = constants2;
  std::sort(constants2_copy.begin(), constants2_copy.end());
  EXPECT_EQ(constants2, constants2_copy);
}

// Test extraction from VALUES clause
TEST_F(ConstantExtractorTest, ExtractFromValues) {
  std::string query = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      VALUES ?x { ex:Alice ex:Bob }
      ?x ex:knows ?y .
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  ConstantExtractor extractor(parsed);
  auto constants = extractor.extractConstants();

  // Should extract IRIs from VALUES clause
  EXPECT_GT(constants.size(), 0u);

  // Check that VALUES IRIs are included
  bool foundAlice = std::any_of(
      constants.begin(), constants.end(),
      [](const std::string& c) { return c.find("Alice") != std::string::npos; });
  bool foundBob = std::any_of(
      constants.begin(), constants.end(),
      [](const std::string& c) { return c.find("Bob") != std::string::npos; });

  EXPECT_TRUE(foundAlice);
  EXPECT_TRUE(foundBob);
}

// Test extraction from nested graph patterns (OPTIONAL)
TEST_F(ConstantExtractorTest, ExtractFromOptional) {
  std::string query = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      ex:Alice ex:knows ?x .
      OPTIONAL { ?x ex:age "30"^^<http://www.w3.org/2001/XMLSchema#integer> }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  ConstantExtractor extractor(parsed);
  auto constants = extractor.extractConstants();

  // Should extract constants from both main pattern and OPTIONAL
  EXPECT_GT(constants.size(), 0u);

  // Check for the literal from OPTIONAL
  bool foundAge = std::any_of(constants.begin(), constants.end(),
                              [](const std::string& c) {
                                return c.find("30") != std::string::npos;
                              });
  EXPECT_TRUE(foundAge);
}

// Test extraction from UNION
TEST_F(ConstantExtractorTest, ExtractFromUnion) {
  std::string query = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      { ex:Alice ex:knows ?x }
      UNION
      { ex:Bob ex:knows ?y }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  ConstantExtractor extractor(parsed);
  auto constants = extractor.extractConstants();

  // Should extract constants from both UNION branches
  EXPECT_GT(constants.size(), 0u);

  bool foundAlice = std::any_of(
      constants.begin(), constants.end(),
      [](const std::string& c) { return c.find("Alice") != std::string::npos; });
  bool foundBob = std::any_of(
      constants.begin(), constants.end(),
      [](const std::string& c) { return c.find("Bob") != std::string::npos; });

  EXPECT_TRUE(foundAlice);
  EXPECT_TRUE(foundBob);
}

// Test that variables are NOT extracted as constants
TEST_F(ConstantExtractorTest, VariablesNotExtracted) {
  std::string query = R"(
    PREFIX ex: <http://example.org/>
    SELECT * WHERE {
      ?x ex:knows ?y .
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  ConstantExtractor extractor(parsed);
  auto constants = extractor.extractConstants();

  // Should only extract the predicate IRI, not the variables
  for (const auto& constant : constants) {
    EXPECT_TRUE(constant.find("?x") == std::string::npos);
    EXPECT_TRUE(constant.find("?y") == std::string::npos);
  }
}

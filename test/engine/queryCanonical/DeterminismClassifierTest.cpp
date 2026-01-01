// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#include <gtest/gtest.h>

#include "engine/queryCanonical/DeterminismClassifier.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "index/EncodedIriManager.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"

using namespace queryCanonical;

namespace {

// Helper to parse a SPARQL query string
ParsedQuery parseQuery(const std::string& queryString) {
  static EncodedIriManager encodedIriManager;
  return SparqlParser::parseQuery(&encodedIriManager, queryString);
}

}  // namespace

class DeterminismClassifierTest : public ::testing::Test {};

// ===========================================================================
// T7: Queries with NOW/RAND marked non-deterministic
// ===========================================================================

TEST_F(DeterminismClassifierTest, DetectNowFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(NOW() AS ?time)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasNow);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DetectRandFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(RAND() AS ?random)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasRand);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DetectUuidFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(UUID() AS ?id)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasUuid);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DetectStruuidFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(STRUUID() AS ?id)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasUuid);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DetectNowInFilter) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      FILTER(?o < NOW())
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasNow);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DetectServiceClause) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      SERVICE <http://example.org/sparql> {
        ?x ?q ?r
      }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasService);
  EXPECT_FALSE(features.isDeterministic());
}

// ===========================================================================
// T7b: Regular queries marked deterministic
// ===========================================================================

TEST_F(DeterminismClassifierTest, DeterministicSimpleQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
  EXPECT_FALSE(features.hasNow);
  EXPECT_FALSE(features.hasRand);
  EXPECT_FALSE(features.hasUuid);
  EXPECT_FALSE(features.hasBnode);
  EXPECT_FALSE(features.hasService);
}

TEST_F(DeterminismClassifierTest, DeterministicFilterQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      FILTER(?o > 10)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DeterministicBindQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(?o + 1 AS ?result)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DeterministicOptionalQuery) {
  std::string query = R"(
    SELECT ?x ?y WHERE {
      ?x ?p ?o .
      OPTIONAL { ?x ?q ?y }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DeterministicUnionQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      { ?x ?p ?o } UNION { ?x ?q ?r }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, DeterministicSubquery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      {
        SELECT ?y WHERE {
          ?y ?q ?r
        }
      }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.isDeterministic());
}

// ===========================================================================
// Complex Query Tests
// ===========================================================================

TEST_F(DeterminismClassifierTest, SubqueryWithNonDeterministicFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      {
        SELECT ?y WHERE {
          ?y ?q ?r .
          BIND(NOW() AS ?time)
        }
      }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasNow);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, NestedOptionalWithRand) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      OPTIONAL {
        ?x ?q ?r .
        BIND(RAND() AS ?random)
      }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  EXPECT_TRUE(features.hasRand);
  EXPECT_FALSE(features.isDeterministic());
}

TEST_F(DeterminismClassifierTest, ComplexDeterministicQuery) {
  std::string query = R"(
    SELECT DISTINCT ?x WHERE {
      ?x ?p ?o .
      FILTER(?o > 10)
      OPTIONAL { ?x ?q ?r }
      {
        SELECT ?y WHERE {
          ?y ?s ?t
        }
      }
    }
    ORDER BY ?x
    LIMIT 100
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier;
  auto features = classifier.analyze(parsed);

  // Should be deterministic (no NOW/RAND/SERVICE)
  EXPECT_TRUE(features.isDeterministic());
}

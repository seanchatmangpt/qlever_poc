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
  DeterminismClassifier classifier(parsed);

  EXPECT_FALSE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::BIND));
}

TEST_F(DeterminismClassifierTest, DetectRandFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(RAND() AS ?random)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  EXPECT_FALSE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
}

TEST_F(DeterminismClassifierTest, DetectUuidFunction) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(UUID() AS ?id)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  EXPECT_FALSE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
}

TEST_F(DeterminismClassifierTest, DetectNowInFilter) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      FILTER(?o < NOW())
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  EXPECT_FALSE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::FILTER));
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
  DeterminismClassifier classifier(parsed);

  EXPECT_FALSE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::SERVICE));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
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
  DeterminismClassifier classifier(parsed);

  EXPECT_TRUE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_FALSE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
}

TEST_F(DeterminismClassifierTest, DeterministicFilterQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      FILTER(?o > 10)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  EXPECT_TRUE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_FALSE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::FILTER));
}

TEST_F(DeterminismClassifierTest, DeterministicBindQuery) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      BIND(?o + 1 AS ?result)
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  EXPECT_TRUE(classifier.isResultCacheable());
  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_FALSE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::BIND));
}

// ===========================================================================
// Feature Flag Detection Tests
// ===========================================================================

TEST_F(DeterminismClassifierTest, DetectDistinct) {
  std::string query = R"(
    SELECT DISTINCT ?x WHERE {
      ?x ?p ?o
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::DISTINCT));
}

TEST_F(DeterminismClassifierTest, DetectReduced) {
  std::string query = R"(
    SELECT REDUCED ?x WHERE {
      ?x ?p ?o
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::REDUCED));
}

TEST_F(DeterminismClassifierTest, DetectOptional) {
  std::string query = R"(
    SELECT ?x ?y WHERE {
      ?x ?p ?o .
      OPTIONAL { ?x ?q ?y }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::OPTIONAL));
}

TEST_F(DeterminismClassifierTest, DetectUnion) {
  std::string query = R"(
    SELECT ?x WHERE {
      { ?x ?p ?o } UNION { ?x ?q ?r }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::UNION));
}

TEST_F(DeterminismClassifierTest, DetectMinus) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      MINUS { ?x ?q ?r }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::MINUS));
}

TEST_F(DeterminismClassifierTest, DetectValues) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o .
      VALUES ?o { 1 2 3 }
    }
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::VALUES));
}

TEST_F(DeterminismClassifierTest, DetectGroupBy) {
  std::string query = R"(
    SELECT ?x (COUNT(?o) AS ?count) WHERE {
      ?x ?p ?o
    }
    GROUP BY ?x
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::GROUP_BY));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::AGGREGATES));
}

TEST_F(DeterminismClassifierTest, DetectOrderBy) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o
    }
    ORDER BY ?x
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::ORDER_BY));
}

TEST_F(DeterminismClassifierTest, DetectLimit) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o
    }
    LIMIT 10
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::LIMIT));
}

TEST_F(DeterminismClassifierTest, DetectOffset) {
  std::string query = R"(
    SELECT ?x WHERE {
      ?x ?p ?o
    }
    OFFSET 5
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::OFFSET));
}

TEST_F(DeterminismClassifierTest, DetectHaving) {
  std::string query = R"(
    SELECT ?x (COUNT(?o) AS ?count) WHERE {
      ?x ?p ?o
    }
    GROUP BY ?x
    HAVING (COUNT(?o) > 5)
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::HAVING));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::GROUP_BY));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::AGGREGATES));
}

TEST_F(DeterminismClassifierTest, DetectSubquery) {
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
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::SUBQUERY));
}

// ===========================================================================
// Complex Query Tests
// ===========================================================================

TEST_F(DeterminismClassifierTest, ComplexQueryWithMultipleFeatures) {
  std::string query = R"(
    SELECT DISTINCT ?x (COUNT(?o) AS ?count) WHERE {
      ?x ?p ?o .
      FILTER(?o > 10)
      OPTIONAL { ?x ?q ?r }
    }
    GROUP BY ?x
    HAVING (COUNT(?o) > 5)
    ORDER BY DESC(?count)
    LIMIT 100
    OFFSET 10
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::DISTINCT));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::FILTER));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::OPTIONAL));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::GROUP_BY));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::HAVING));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::ORDER_BY));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::LIMIT));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::OFFSET));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::AGGREGATES));

  // Should be cacheable (no NOW/RAND/SERVICE)
  EXPECT_TRUE(classifier.isResultCacheable());
}

TEST_F(DeterminismClassifierTest, ComplexNonDeterministicQuery) {
  std::string query = R"(
    SELECT DISTINCT ?x ?time WHERE {
      ?x ?p ?o .
      BIND(NOW() AS ?time)
      OPTIONAL { ?x ?q ?r }
    }
    ORDER BY ?time
    LIMIT 100
  )";

  ParsedQuery parsed = parseQuery(query);
  DeterminismClassifier classifier(parsed);

  uint32_t flags = classifier.analyzeFeatures();
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::DISTINCT));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::BIND));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::OPTIONAL));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::ORDER_BY));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::LIMIT));
  EXPECT_TRUE(hasFlag(flags, FeatureFlag::NON_DETERMINISTIC));

  // Should NOT be cacheable (contains NOW)
  EXPECT_FALSE(classifier.isResultCacheable());
}

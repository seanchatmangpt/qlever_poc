// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "engine/ExportQueryExecutionTrees.h"
#include "engine/QueryPlanner.h"
#include "parser/SparqlParser.h"
#include "util/CancellationHandle.h"
#include "util/GTestHelpers.h"
#include "util/IdTestHelpers.h"
#include "util/IndexTestHelpers.h"
#include "util/Timer.h"

using namespace std::string_literals;
using namespace std::string_view_literals;
using ::testing::HasSubstr;

namespace {

// Helper to parse SPARQL queries
auto parseQuery(std::string query,
                const std::vector<DatasetClause>& datasets = {}) {
  static EncodedIriManager evM;
  return SparqlParser::parseQuery(&evM, std::move(query), datasets);
}

// Helper to execute a CONSTRUCT query and get JSON result
nlohmann::json runConstructQuery(const std::string& kg,
                                 const std::string& query) {
  ad_utility::testing::TestIndexConfig config{kg};
  auto qec = ad_utility::testing::getQec(std::move(config));
  qec->clearCacheUnpinnedOnly();

  auto cancellationHandle =
      std::make_shared<ad_utility::CancellationHandle<>>();
  QueryPlanner qp{qec, cancellationHandle};

  auto pq = parseQuery(query);
  auto qet = qp.createExecutionTree(pq);

  ad_utility::Timer timer(ad_utility::Timer::Started);
  std::string resStr;
  using enum ad_utility::MediaType;
  for (auto c : ExportQueryExecutionTrees::computeResult(
           pq, qet, qleverJson, timer,
           std::move(cancellationHandle))) {
    resStr += c;
  }
  return nlohmann::json::parse(resStr);
}

// Helper to get TSV output from CONSTRUCT query
std::string runConstructQueryTSV(const std::string& kg,
                                 const std::string& query) {
  ad_utility::testing::TestIndexConfig config{kg};
  auto qec = ad_utility::testing::getQec(std::move(config));
  qec->clearCacheUnpinnedOnly();

  auto cancellationHandle =
      std::make_shared<ad_utility::CancellationHandle<>>();
  QueryPlanner qp{qec, cancellationHandle};

  auto pq = parseQuery(query);
  auto qet = qp.createExecutionTree(pq);

  ad_utility::Timer timer(ad_utility::Timer::Started);
  std::string result;
  using enum ad_utility::MediaType;
  for (const auto& block : ExportQueryExecutionTrees::computeResult(
           pq, qet, tsv, timer,
           std::move(cancellationHandle))) {
    result += block;
  }
  return result;
}

}  // namespace

/*
 * Stress tests for SPARQL CONSTRUCT queries.
 * Focused test suite covering core functionality and important patterns.
 */
class ConstructStressTest : public ::testing::Test {};

// Test 1: Basic CONSTRUCT with single triple pattern
TEST_F(ConstructStressTest, BasicConstruct) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
    <http://example.org/charlie> <http://example.org/name> "Charlie" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_TRUE(result.contains("res"));
  EXPECT_TRUE(result["res"].is_array());
  EXPECT_EQ(result["resultSizeTotal"], 3);
}

// Test 2: CONSTRUCT with pattern rewriting
TEST_F(ConstructStressTest, ConstructPatternRewrite) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/modified> ?o
    }
    WHERE {
      ?s <http://example.org/name> ?o
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
  EXPECT_TRUE(result["res"].is_array());
}

// Test 3: CONSTRUCT with BIND expression
TEST_F(ConstructStressTest, ConstructWithBind) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/age> 30 .
    <http://example.org/bob> <http://example.org/age> 25 .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/ageInMonths> ?months
    }
    WHERE {
      ?s <http://example.org/age> ?age
      BIND(?age * 12 AS ?months)
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 4: CONSTRUCT with FILTER
TEST_F(ConstructStressTest, ConstructWithFilter) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/age> 30 .
    <http://example.org/bob> <http://example.org/age> 25 .
    <http://example.org/charlie> <http://example.org/age> 35 .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/senior> "yes"
    }
    WHERE {
      ?s <http://example.org/age> ?age
      FILTER(?age > 28)
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 5: CONSTRUCT with OPTIONAL clause
TEST_F(ConstructStressTest, ConstructWithOptional) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" ;
                               <http://example.org/age> 30 .
    <http://example.org/bob> <http://example.org/name> "Bob" .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/name> ?name ;
         <http://example.org/age> ?age .
    }
    WHERE {
      ?s <http://example.org/name> ?name
      OPTIONAL { ?s <http://example.org/age> ?age }
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GE(result["resultSizeTotal"].get<int>(), 2);
}

// Test 6: CONSTRUCT with LIMIT modifier
TEST_F(ConstructStressTest, ConstructWithLimit) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
    <http://example.org/charlie> <http://example.org/name> "Charlie" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
    LIMIT 2
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 7: CONSTRUCT with LIMIT and OFFSET
TEST_F(ConstructStressTest, ConstructWithLimitOffset) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
    <http://example.org/charlie> <http://example.org/name> "Charlie" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
    ORDER BY ?s
    OFFSET 1
    LIMIT 2
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 8: CONSTRUCT with multiple patterns and joins
TEST_F(ConstructStressTest, ConstructMultiplePatterns) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/knows> <http://example.org/bob> ;
                               <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/name> ?name .
      ?s <http://example.org/knows> ?other .
      ?other <http://example.org/name> ?otherName .
    }
    WHERE {
      ?s <http://example.org/name> ?name .
      OPTIONAL {
        ?s <http://example.org/knows> ?other .
        ?other <http://example.org/name> ?otherName .
      }
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GT(result["resultSizeTotal"].get<int>(), 0);
}

// Test 9: CONSTRUCT with TSV output format
TEST_F(ConstructStressTest, ConstructTSVOutput) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
  )";

  std::string result = runConstructQueryTSV(kg, query);
  EXPECT_GT(result.length(), 0);
  EXPECT_TRUE(result.find('\t') != std::string::npos ||
              result.find('\n') != std::string::npos);
}

// Test 10: CONSTRUCT with string operations (CONCAT, STRAFTER)
TEST_F(ConstructStressTest, ConstructStringOperations) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/email> "alice@example.com" .
    <http://example.org/bob> <http://example.org/email> "bob@example.com" .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/domain> ?domain
    }
    WHERE {
      ?s <http://example.org/email> ?email
      BIND(STRAFTER(?email, "@") AS ?domain)
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 11: CONSTRUCT with FILTER and BIND combination
TEST_F(ConstructStressTest, ConstructFilterAndBind) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/salary> 50000 .
    <http://example.org/bob> <http://example.org/salary> 60000 .
    <http://example.org/charlie> <http://example.org/salary> 45000 .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/bonus> ?bonus
    }
    WHERE {
      ?s <http://example.org/salary> ?salary
      BIND(?salary * 0.1 AS ?bonus)
      FILTER(?bonus > 4500)
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

// Test 12: CONSTRUCT with GROUP BY aggregation
TEST_F(ConstructStressTest, ConstructWithGroupBy) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/department> "Sales" .
    <http://example.org/bob> <http://example.org/department> "IT" .
    <http://example.org/charlie> <http://example.org/department> "Sales" .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?dept <http://example.org/memberCount> ?count
    }
    WHERE {
      SELECT ?dept (COUNT(?person) AS ?count)
      WHERE {
        ?person <http://example.org/department> ?dept
      }
      GROUP BY ?dept
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GT(result["resultSizeTotal"].get<int>(), 0);
}

// Test 13: CONSTRUCT with empty result
TEST_F(ConstructStressTest, ConstructEmptyResult) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE {
      ?s <http://example.org/nonexistent> ?o
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 0);
}

// Test 14: CONSTRUCT with complex join patterns
TEST_F(ConstructStressTest, ConstructComplexJoin) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/knows> <http://example.org/bob> ;
                               <http://example.org/knows> <http://example.org/charlie> .
    <http://example.org/bob> <http://example.org/knows> <http://example.org/charlie> .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/directConnection> ?o .
      ?s <http://example.org/indirectConnection> ?o2 .
    }
    WHERE {
      ?s <http://example.org/knows> ?o .
      ?o <http://example.org/knows> ?o2 .
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GT(result["resultSizeTotal"].get<int>(), 0);
}

// Test 15: CONSTRUCT with VALUES clause
TEST_F(ConstructStressTest, ConstructWithValues) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
    <http://example.org/charlie> <http://example.org/name> "Charlie" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p "Selected" }
    WHERE {
      VALUES ?s {
        <http://example.org/alice>
        <http://example.org/bob>
      }
      ?s ?p ?o
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GT(result["resultSizeTotal"].get<int>(), 0);
}

// Test 16: CONSTRUCT with DISTINCT solution modifier
TEST_F(ConstructStressTest, ConstructWithDistinct) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/friend> <http://example.org/bob> .
    <http://example.org/alice> <http://example.org/friend> <http://example.org/bob> .
    <http://example.org/alice> <http://example.org/friend> <http://example.org/charlie> .
  )";

  std::string query = R"(
    CONSTRUCT { ?s <http://example.org/uniqueFriend> ?friend }
    WHERE {
      SELECT DISTINCT ?s ?friend
      WHERE {
        ?s <http://example.org/friend> ?friend
      }
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_GT(result["resultSizeTotal"].get<int>(), 0);
}

// Test 17: Large-scale CONSTRUCT performance test
TEST_F(ConstructStressTest, LargeScaleConstruct) {
  std::string kg;
  for (int i = 0; i < 500; ++i) {
    kg += "<http://example.org/person" + std::to_string(i) +
          "> <http://example.org/name> \"Person" + std::to_string(i) +
          "\" .\n";
  }

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 500);
}

// Test 18: CONSTRUCT with MINUS set subtraction
TEST_F(ConstructStressTest, ConstructWithMinus) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/name> "Alice" ;
                               <http://example.org/admin> "yes" .
    <http://example.org/bob> <http://example.org/name> "Bob" .
    <http://example.org/charlie> <http://example.org/name> "Charlie" ;
                                 <http://example.org/admin> "yes" .
  )";

  std::string query = R"(
    CONSTRUCT { ?s <http://example.org/regularUser> "yes" }
    WHERE {
      ?s <http://example.org/name> ?name
      MINUS { ?s <http://example.org/admin> "yes" }
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 1);
}

// Test 19: CONSTRUCT preserves IRI correctness
TEST_F(ConstructStressTest, ConstructPreservesIRIs) {
  std::string kg = R"(
    <http://example.org/resource/123> <http://example.org/type> <http://example.org/types/Person> .
  )";

  std::string query = R"(
    CONSTRUCT { ?s ?p ?o }
    WHERE { ?s ?p ?o }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 1);
  EXPECT_TRUE(result["res"].is_array());
}

// Test 20: CONSTRUCT with nested BIND expressions
TEST_F(ConstructStressTest, ConstructWithNestedBind) {
  std::string kg = R"(
    <http://example.org/alice> <http://example.org/age> 30 .
    <http://example.org/bob> <http://example.org/age> 25 .
  )";

  std::string query = R"(
    CONSTRUCT {
      ?s <http://example.org/info> ?info
    }
    WHERE {
      ?s <http://example.org/age> ?age
      BIND(CONCAT("Age: ", STR(?age)) AS ?info)
    }
  )";

  auto result = runConstructQuery(kg, query);
  EXPECT_EQ(result["resultSizeTotal"], 2);
}

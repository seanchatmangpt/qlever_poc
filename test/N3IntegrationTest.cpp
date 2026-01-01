// Copyright 2025, Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// SPDX-License-Identifier: Apache-2.0
//
// Comprehensive integration tests for N3 format support in QLever.
// Tests end-to-end functionality including index building, querying, and
// error handling.

#include <gtest/gtest.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>

#include "./util/GTestHelpers.h"
#include "./util/IndexTestHelpers.h"
#include "./util/TripleComponentTestHelpers.h"
#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "global/Constants.h"
#include "index/Index.h"
#include "index/IndexImpl.h"
#include "index/InputFileSpecification.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlParser.h"

namespace {

namespace fs = std::filesystem;
using ad_utility::testing::makeTestIndex;
using ad_utility::testing::TestIndexConfig;
using qlever::Filetype;

// Test fixture for N3 integration tests
class N3IntegrationTest : public ::testing::Test {
 protected:
  // Base directory for test data
  const std::string testDataDir_ = "/home/user/qlever/examples/n3-test-data/";

  // Test index base paths - use unique names per test instance for parallel execution
  std::string basicIndexBase_;
  std::string peopleIndexBase_;
  std::string mixedIndexBase_;
  std::string errorIndexBase_;

  void SetUp() override {
    // Generate unique basenames per test instance for parallel execution
    std::string uniqueId = std::to_string(getpid()) + "_" + 
        std::to_string(::testing::UnitTest::GetInstance()->random_seed());
    basicIndexBase_ = "N3IntegrationTest_basic_" + uniqueId;
    peopleIndexBase_ = "N3IntegrationTest_people_" + uniqueId;
    mixedIndexBase_ = "N3IntegrationTest_mixed_" + uniqueId;
    errorIndexBase_ = "N3IntegrationTest_error_" + uniqueId;
    
    // Clean up any existing test indices from previous runs
    cleanupIndexFiles(basicIndexBase_);
    cleanupIndexFiles(peopleIndexBase_);
    cleanupIndexFiles(mixedIndexBase_);
    cleanupIndexFiles(errorIndexBase_);
  }

  void TearDown() override {
    // Clean up test indices after test completion
    cleanupIndexFiles(basicIndexBase_);
    cleanupIndexFiles(peopleIndexBase_);
    cleanupIndexFiles(mixedIndexBase_);
    cleanupIndexFiles(errorIndexBase_);
  }

  // Helper to delete all index files
  void cleanupIndexFiles(const std::string& indexBasename) {
    auto files = ad_utility::testing::getAllIndexFilenames(indexBasename);
    for (const auto& file : files) {
      std::filesystem::remove(file);
    }
  }

  // Helper to create an N3 test file from content
  std::string createTestFile(const std::string& basename,
                             const std::string& content) {
    std::string filename = basename + ".n3";
    std::ofstream out(filename);
    out << content;
    out.close();
    return filename;
  }

  // Helper to execute a SPARQL query and return the result
  std::shared_ptr<QueryExecutionTree> executeQuery(
      const Index& index, const std::string& sparqlQuery) {
    ParsedQuery pq = SparqlParser::parseQuery(sparqlQuery);
    QueryPlanner qp(nullptr, std::make_shared<ad_utility::CancellationHandle<>>());
    auto qec = buildQueryExecutionContext(index);
    auto qet = qp.createExecutionTree(pq, *qec);
    return qet;
  }

  // Helper to build a query execution context from an index
  std::shared_ptr<QueryExecutionContext> buildQueryExecutionContext(
      const Index& index) {
    return std::make_shared<QueryExecutionContext>(
        index, &index.getImpl().allocator(),
        std::make_shared<ad_utility::CancellationHandle<>>());
  }

  // Helper to count triples in index by executing a simple query
  size_t countTriplesInIndex(const Index& index) {
    auto query = "SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }";
    auto qet = executeQuery(index, query);
    auto result = qet->computeResultOnlyForTesting();
    EXPECT_EQ(result.idTable().numColumns(), 1);
    if (result.idTable().numRows() > 0) {
      return result.idTable()(0, 0).getInt();
    }
    return 0;
  }
};

// Test 1: Basic N3 index building from string
TEST_F(N3IntegrationTest, BuildIndexFromN3String) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice a foaf:Person ;
      foaf:name "Alice" ;
      foaf:age 30 .

    ex:bob a foaf:Person ;
      foaf:name "Bob" ;
      foaf:age 28 .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;
  config.loadAllPermutations = true;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Verify index was created
  EXPECT_TRUE(index.getImpl().hasAllPermutations());

  // Verify vocabulary contains expected entries
  auto getId = ad_utility::testing::makeGetId(index);
  EXPECT_NO_THROW(getId("<http://example.org/alice>"));
  EXPECT_NO_THROW(getId("<http://example.org/bob>"));
  EXPECT_NO_THROW(getId("<http://xmlns.com/foaf/0.1/Person>"));
  EXPECT_NO_THROW(getId("<http://xmlns.com/foaf/0.1/name>"));
  EXPECT_NO_THROW(getId("\"Alice\""));
  EXPECT_NO_THROW(getId("\"Bob\""));
}

// Test 2: Load and query basic.n3 test file
TEST_F(N3IntegrationTest, LoadAndQueryBasicN3File) {
  std::string n3FilePath = testDataDir_ + "basic.n3";
  ASSERT_TRUE(fs::exists(n3FilePath)) << "Test file not found: " << n3FilePath;

  // Read file content
  std::ifstream file(n3FilePath);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  TestIndexConfig config;
  config.turtleInput = content;
  config.indexType = Filetype::N3;
  config.loadAllPermutations = true;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Test query for people
  auto query = R"(
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT ?name WHERE {
      ?person a foaf:Person .
      ?person foaf:name ?name .
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  // Should find Alice, Bob, and Carol
  EXPECT_GE(result.idTable().numRows(), 3);
}

// Test 3: Language-tagged literals
TEST_F(N3IntegrationTest, LanguageTaggedLiterals) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

    ex:book1 rdfs:label "The Book"@en ;
             rdfs:label "Le Livre"@fr ;
             rdfs:label "Das Buch"@de .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  auto getId = ad_utility::testing::makeGetId(index);

  // All three language-tagged literals should be in vocabulary
  EXPECT_NO_THROW(getId("\"The Book\"@en"));
  EXPECT_NO_THROW(getId("\"Le Livre\"@fr"));
  EXPECT_NO_THROW(getId("\"Das Buch\"@de"));
}

// Test 4: Typed literals
TEST_F(N3IntegrationTest, TypedLiterals) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix xsd: <http://www.w3.org/2001/XMLSchema#> .

    ex:event1 ex:date "2024-01-15"^^xsd:date ;
              ex:count "42"^^xsd:integer ;
              ex:price "19.99"^^xsd:decimal ;
              ex:active "true"^^xsd:boolean .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Query for typed literals
  auto query = R"(
    PREFIX ex: <http://example.org/>
    SELECT ?count WHERE {
      ex:event1 ex:count ?count .
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  EXPECT_EQ(result.idTable().numRows(), 1);
}

// Test 5: Blank nodes
TEST_F(N3IntegrationTest, BlankNodes) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice foaf:knows [ foaf:name "Anonymous Friend" ;
                          foaf:age 25 ] .

    [] a foaf:Organization ;
       foaf:name "Mystery Corp" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Count total triples - should include blank node triples
  auto tripleCount = countTriplesInIndex(index);

  // Should have at least 5 triples (2 for alice knows, 2 for blank node, 2 for org)
  EXPECT_GE(tripleCount, 5);
}

// Test 6: Property lists
TEST_F(N3IntegrationTest, PropertyLists) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice a foaf:Person ;
             foaf:name "Alice" ;
             foaf:email "alice@example.org" ;
             foaf:knows ex:bob, ex:carol ;
             foaf:age 30 .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Query for people Alice knows
  auto query = R"(
    PREFIX ex: <http://example.org/>
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT ?friend WHERE {
      ex:alice foaf:knows ?friend .
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  // Should know 2 people (bob and carol)
  EXPECT_EQ(result.idTable().numRows(), 2);
}

// Test 7: Collections (RDF lists)
TEST_F(N3IntegrationTest, Collections) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .

    ex:list1 rdf:value (1 2 3 4 5) .
    ex:colors rdf:value ("red" "green" "blue") .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Collections should be expanded into multiple triples
  auto tripleCount = countTriplesInIndex(index);

  // Should have many triples from collection expansion
  EXPECT_GE(tripleCount, 10);
}

// Test 8: Large N3 file (people-dataset.n3)
TEST_F(N3IntegrationTest, LargeN3Dataset) {
  std::string n3FilePath = testDataDir_ + "people-dataset.n3";
  ASSERT_TRUE(fs::exists(n3FilePath)) << "Test file not found: " << n3FilePath;

  std::ifstream file(n3FilePath);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  TestIndexConfig config;
  config.turtleInput = content;
  config.indexType = Filetype::N3;
  config.loadAllPermutations = true;
  config.parserBufferSize = 100_kB;  // Larger buffer for bigger file

  auto index = makeTestIndex(peopleIndexBase_, config);

  // Query to count people
  auto query = R"(
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT (COUNT(?person) AS ?count) WHERE {
      ?person a foaf:Person .
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  EXPECT_EQ(result.idTable().numRows(), 1);

  // Should have 30 people in the dataset
  auto personCount = result.idTable()(0, 0).getInt();
  EXPECT_EQ(personCount, 30);
}

// Test 9: Mixed format loading (N3 + Turtle)
TEST_F(N3IntegrationTest, MixedFormatLoading) {
  // Create N3 file
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice a foaf:Person ;
             foaf:name "Alice" .
  )";

  // Create Turtle file
  std::string turtleContent = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:bob a foaf:Person ;
           foaf:name "Bob" .
  )";

  // Note: For this test, we'll combine both into a single N3 file
  // since the parser should handle turtle syntax within N3
  std::string combinedContent = n3Content + "\n" + turtleContent;

  TestIndexConfig config;
  config.turtleInput = combinedContent;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(mixedIndexBase_, config);

  // Both Alice and Bob should be in the index
  auto getId = ad_utility::testing::makeGetId(index);
  EXPECT_NO_THROW(getId("<http://example.org/alice>"));
  EXPECT_NO_THROW(getId("<http://example.org/bob>"));
}

// Test 10: Query with FILTER on N3 data
TEST_F(N3IntegrationTest, QueryWithFilter) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice a foaf:Person ; foaf:age 30 .
    ex:bob a foaf:Person ; foaf:age 25 .
    ex:carol a foaf:Person ; foaf:age 35 .
    ex:dave a foaf:Person ; foaf:age 28 .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Query for people older than 28
  auto query = R"(
    PREFIX ex: <http://example.org/>
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT ?person ?age WHERE {
      ?person a foaf:Person .
      ?person foaf:age ?age .
      FILTER(?age > 28)
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  // Should find Alice (30) and Carol (35)
  EXPECT_EQ(result.idTable().numRows(), 2);
}

// Test 11: Complex SPARQL query on N3 data
TEST_F(N3IntegrationTest, ComplexSparqlQuery) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    ex:alice a foaf:Person ;
             foaf:name "Alice" ;
             foaf:knows ex:bob, ex:carol .

    ex:bob a foaf:Person ;
           foaf:name "Bob" ;
           foaf:knows ex:carol .

    ex:carol a foaf:Person ;
             foaf:name "Carol" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Query for friends of friends
  auto query = R"(
    PREFIX ex: <http://example.org/>
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT DISTINCT ?fof WHERE {
      ex:alice foaf:knows ?friend .
      ?friend foaf:knows ?fof .
      FILTER(?fof != ex:alice)
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  // Should find Carol (Alice -> Bob -> Carol)
  EXPECT_GE(result.idTable().numRows(), 1);
}

// Test 12: Long prefix chains
TEST_F(N3IntegrationTest, LongPrefixChains) {
  std::string n3Content = R"(
    @prefix ex1: <http://example1.org/> .
    @prefix ex2: <http://example2.org/> .
    @prefix ex3: <http://example3.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .
    @prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
    @prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .

    ex1:alice a foaf:Person ;
              rdfs:label "Alice from ex1" .

    ex2:alice a foaf:Person ;
              rdfs:label "Alice from ex2" .

    ex3:alice a foaf:Person ;
              rdfs:label "Alice from ex3" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // All three different alice IRIs should be distinct
  auto getId = ad_utility::testing::makeGetId(index);
  auto id1 = getId("<http://example1.org/alice>");
  auto id2 = getId("<http://example2.org/alice>");
  auto id3 = getId("<http://example3.org/alice>");

  EXPECT_NE(id1, id2);
  EXPECT_NE(id2, id3);
  EXPECT_NE(id1, id3);
}

// Test 13: Error handling - Invalid N3 syntax
TEST_F(N3IntegrationTest, InvalidN3Syntax) {
  std::string invalidN3 = R"(
    @prefix ex: <http://example.org/> .

    ex:alice foaf:name "Alice"  # Missing dot at end
    ex:bob foaf:name "Bob" .    # This line should also fail
  )";

  TestIndexConfig config;
  config.turtleInput = invalidN3;
  config.indexType = Filetype::N3;

  // Index building should throw an exception for invalid syntax
  EXPECT_THROW(makeTestIndex(errorIndexBase_, config), std::exception);
}

// Test 14: Error handling - Missing prefix
TEST_F(N3IntegrationTest, MissingPrefix) {
  std::string n3WithMissingPrefix = R"(
    @prefix ex: <http://example.org/> .

    ex:alice missing:name "Alice" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3WithMissingPrefix;
  config.indexType = Filetype::N3;

  // Should throw exception for undefined prefix
  EXPECT_THROW(makeTestIndex(errorIndexBase_, config), std::exception);
}

// Test 15: Base URI handling
TEST_F(N3IntegrationTest, BaseUriHandling) {
  std::string n3Content = R"(
    @base <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .

    <alice> a foaf:Person ;
            foaf:name "Alice" .

    <bob> a foaf:Person ;
          foaf:name "Bob" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Relative IRIs should be resolved against base
  auto getId = ad_utility::testing::makeGetId(index);
  EXPECT_NO_THROW(getId("<http://example.org/alice>"));
  EXPECT_NO_THROW(getId("<http://example.org/bob>"));
}

// Test 16: Unicode in N3
TEST_F(N3IntegrationTest, UnicodeSupport) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .
    @prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

    ex:person1 rdfs:label "Müller" .
    ex:person2 rdfs:label "北京" .
    ex:person3 rdfs:label "Москва" .
    ex:person4 rdfs:label "🌍" .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // All unicode strings should be in vocabulary
  auto getId = ad_utility::testing::makeGetId(index);
  EXPECT_NO_THROW(getId("\"Müller\""));
  EXPECT_NO_THROW(getId("\"北京\""));
  EXPECT_NO_THROW(getId("\"Москва\""));
  EXPECT_NO_THROW(getId("\"🌍\""));
}

// Test 17: Empty N3 file
TEST_F(N3IntegrationTest, EmptyN3File) {
  std::string emptyN3 = "";

  TestIndexConfig config;
  config.turtleInput = emptyN3;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Empty index should have 0 triples
  auto tripleCount = countTriplesInIndex(index);
  EXPECT_EQ(tripleCount, 0);
}

// Test 18: N3 with only comments
TEST_F(N3IntegrationTest, OnlyComments) {
  std::string n3Content = R"(
    # This is a comment
    # Another comment
    # @prefix ex: <http://example.org/> .  # This is commented out
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Should have 0 triples (only comments)
  auto tripleCount = countTriplesInIndex(index);
  EXPECT_EQ(tripleCount, 0);
}

// Test 19: Performance benchmark hint (large dataset)
TEST_F(N3IntegrationTest, PerformanceBenchmark) {
  // Generate a larger N3 dataset
  std::stringstream ss;
  ss << "@prefix ex: <http://example.org/> .\n";
  ss << "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n\n";

  for (int i = 0; i < 100; ++i) {
    ss << "ex:person" << i << " a foaf:Person ;\n";
    ss << "  foaf:name \"Person " << i << "\" ;\n";
    ss << "  foaf:age " << (20 + (i % 50)) << " .\n\n";
  }

  TestIndexConfig config;
  config.turtleInput = ss.str();
  config.indexType = Filetype::N3;
  config.parserBufferSize = 100_kB;

  // Measure index building time
  auto startTime = std::chrono::high_resolution_clock::now();
  auto index = makeTestIndex(basicIndexBase_, config);
  auto endTime = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  // Verify all 100 people are indexed
  auto query = R"(
    PREFIX foaf: <http://xmlns.com/foaf/0.1/>
    SELECT (COUNT(?person) AS ?count) WHERE {
      ?person a foaf:Person .
    }
  )";

  auto qet = executeQuery(index, query);
  auto result = qet->computeResultOnlyForTesting();

  EXPECT_EQ(result.idTable().numRows(), 1);
  auto personCount = result.idTable()(0, 0).getInt();
  EXPECT_EQ(personCount, 100);

  // Log performance (informational)
  std::cout << "Built index for 100 people in " << duration.count()
            << " milliseconds" << std::endl;
}

// Test 20: All permutations loaded
TEST_F(N3IntegrationTest, AllPermutationsLoaded) {
  std::string n3Content = R"(
    @prefix ex: <http://example.org/> .

    ex:subject ex:predicate ex:object .
  )";

  TestIndexConfig config;
  config.turtleInput = n3Content;
  config.indexType = Filetype::N3;
  config.loadAllPermutations = true;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Verify all six permutations are loaded
  EXPECT_TRUE(index.getImpl().hasAllPermutations());
}

// Test 21: Vocabulary verification
TEST_F(N3IntegrationTest, VocabularyVerification) {
  std::string n3FilePath = testDataDir_ + "basic.n3";
  ASSERT_TRUE(fs::exists(n3FilePath));

  std::ifstream file(n3FilePath);
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  file.close();

  TestIndexConfig config;
  config.turtleInput = content;
  config.indexType = Filetype::N3;

  auto index = makeTestIndex(basicIndexBase_, config);

  // Verify specific vocabulary entries from basic.n3
  auto getId = ad_utility::testing::makeGetId(index);

  // Check IRIs
  EXPECT_NO_THROW(getId("<http://example.org/alice>"));
  EXPECT_NO_THROW(getId("<http://xmlns.com/foaf/0.1/Person>"));
  EXPECT_NO_THROW(getId("<http://xmlns.com/foaf/0.1/name>"));

  // Check literals
  EXPECT_NO_THROW(getId("\"Alice Smith\""));
  EXPECT_NO_THROW(getId("\"Bob Jones\""));
  EXPECT_NO_THROW(getId("\"Carol White\""));
}

}  // namespace

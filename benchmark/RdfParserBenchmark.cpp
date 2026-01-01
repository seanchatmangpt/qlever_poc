// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// N3 Format Support Benchmarks

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark/infrastructure/Benchmark.h"
#include "index/EncodedIriManager.h"
#include "parser/RdfParser.h"
#include "parser/Tokenizer.h"
#include "util/MemorySize/MemorySize.h"

// Benchmark fixture for RDF parsing performance across formats
class RdfParserBenchmark : public benchmark::Benchmark {
 protected:
  // Test data: N3 format with 30 people and relationships
  static const std::string_view peopleN3;

  // Equivalent Turtle data
  static const std::string_view peopleTurtle;

  // Small basic test data
  static const std::string_view basicN3;
  static const std::string_view basicTurtle;

  const EncodedIriManager encodedIriManager_;

 public:
  RdfParserBenchmark() { setName("RdfParserBenchmark"); }

  // Helper to parse N3 and return triple count
  size_t parseN3(std::string_view input) {
    using Parser = RdfStringParser<N3Parser<Tokenizer>>;
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  // Helper to parse Turtle and return triple count
  size_t parseTurtle(std::string_view input) {
    using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  // Helper to parse NQuads and return triple count
  size_t parseNQuad(std::string_view input) {
    using Parser = RdfStringParser<NQuadParser<Tokenizer>>;
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }
};

// Test data definitions
const std::string_view RdfParserBenchmark::basicN3 = R"(
@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

<alice> a foaf:Person ;
  foaf:name "Alice" ;
  foaf:age 30 ;
  foaf:knows <bob> .

<bob> a foaf:Person ;
  foaf:name "Bob" ;
  foaf:age 28 .
)";

const std::string_view RdfParserBenchmark::basicTurtle = R"(
@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

<alice> a foaf:Person ;
  foaf:name "Alice" ;
  foaf:age 30 ;
  foaf:knows <bob> .

<bob> a foaf:Person ;
  foaf:name "Bob" ;
  foaf:age 28 .
)";

// Larger dataset with 30 people (see examples/n3-test-data/people-dataset.n3)
const std::string_view RdfParserBenchmark::peopleN3 = R"(
@prefix ex: <http://example.org/people/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .
@prefix vcard: <http://www.w3.org/2006/vcard/ns#> .
@prefix : <http://example.org/people/> .

:person_1 a foaf:Person ;
  foaf:name "Person 1" ;
  foaf:email "person1@example.org" ;
  foaf:age 25 ;
  foaf:knows :person_2, :person_3, :person_4 ;
  vcard:hasURL <http://example.org/person1> ;
  foaf:givenName "First"@en ;
  foaf:familyName "Last"@en ;
  foaf:mbox <mailto:person1@example.org> .

:person_2 a foaf:Person ;
  foaf:name "Person 2" ;
  foaf:email "person2@example.org" ;
  foaf:age 30 ;
  foaf:knows :person_1, :person_5, :person_10 .

:person_3 a foaf:Person ;
  foaf:name "Person 3" ;
  foaf:email "person3@example.org" ;
  foaf:age 28 ;
  foaf:knows :person_1, :person_6, :person_7 .

:person_4 a foaf:Person ;
  foaf:name "Person 4" ;
  foaf:email "person4@example.org" ;
  foaf:age 35 ;
  foaf:knows :person_1, :person_8, :person_9 .

:person_5 a foaf:Person ;
  foaf:name "Person 5" ;
  foaf:email "person5@example.org" ;
  foaf:age 26 ;
  foaf:knows :person_2, :person_11, :person_12 .

:person_6 a foaf:Person ;
  foaf:name "Person 6" ;
  foaf:email "person6@example.org" ;
  foaf:age 31 ;
  foaf:knows :person_3, :person_13, :person_14 .

:person_7 a foaf:Person ;
  foaf:name "Person 7" ;
  foaf:email "person7@example.org" ;
  foaf:age 29 ;
  foaf:knows :person_3, :person_15, :person_16 .

:person_8 a foaf:Person ;
  foaf:name "Person 8" ;
  foaf:email "person8@example.org" ;
  foaf:age 33 ;
  foaf:knows :person_4, :person_17, :person_18 .

:person_9 a foaf:Person ;
  foaf:name "Person 9" ;
  foaf:email "person9@example.org" ;
  foaf:age 27 ;
  foaf:knows :person_4, :person_19, :person_20 .

:person_10 a foaf:Person ;
  foaf:name "Person 10" ;
  foaf:email "person10@example.org" ;
  foaf:age 32 ;
  foaf:knows :person_2, :person_21, :person_22 .
)";

const std::string_view RdfParserBenchmark::peopleTurtle = R"(
@prefix ex: <http://example.org/people/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .
@prefix vcard: <http://www.w3.org/2006/vcard/ns#> .
@prefix : <http://example.org/people/> .

:person_1 a foaf:Person ;
  foaf:name "Person 1" ;
  foaf:email "person1@example.org" ;
  foaf:age 25 ;
  foaf:knows :person_2, :person_3, :person_4 ;
  vcard:hasURL <http://example.org/person1> ;
  foaf:givenName "First"@en ;
  foaf:familyName "Last"@en ;
  foaf:mbox <mailto:person1@example.org> .

:person_2 a foaf:Person ;
  foaf:name "Person 2" ;
  foaf:email "person2@example.org" ;
  foaf:age 30 ;
  foaf:knows :person_1, :person_5, :person_10 .

:person_3 a foaf:Person ;
  foaf:name "Person 3" ;
  foaf:email "person3@example.org" ;
  foaf:age 28 ;
  foaf:knows :person_1, :person_6, :person_7 .

:person_4 a foaf:Person ;
  foaf:name "Person 4" ;
  foaf:email "person4@example.org" ;
  foaf:age 35 ;
  foaf:knows :person_1, :person_8, :person_9 .

:person_5 a foaf:Person ;
  foaf:name "Person 5" ;
  foaf:email "person5@example.org" ;
  foaf:age 26 ;
  foaf:knows :person_2, :person_11, :person_12 .

:person_6 a foaf:Person ;
  foaf:name "Person 6" ;
  foaf:email "person6@example.org" ;
  foaf:age 31 ;
  foaf:knows :person_3, :person_13, :person_14 .

:person_7 a foaf:Person ;
  foaf:name "Person 7" ;
  foaf:email "person7@example.org" ;
  foaf:age 29 ;
  foaf:knows :person_3, :person_15, :person_16 .

:person_8 a foaf:Person ;
  foaf:name "Person 8" ;
  foaf:email "person8@example.org" ;
  foaf:age 33 ;
  foaf:knows :person_4, :person_17, :person_18 .

:person_9 a foaf:Person ;
  foaf:name "Person 9" ;
  foaf:email "person9@example.org" ;
  foaf:age 27 ;
  foaf:knows :person_4, :person_19, :person_20 .

:person_10 a foaf:Person ;
  foaf:name "Person 10" ;
  foaf:email "person10@example.org" ;
  foaf:age 32 ;
  foaf:knows :person_2, :person_21, :person_22 .
)";

// Benchmark: Basic N3 parsing (small dataset)
BENCHMARK_F(RdfParserBenchmark, ParseBasicN3, 1000) {
  auto tripleCount = parseN3(basicN3);
  ad_utility::unused(tripleCount);
}

// Benchmark: Basic Turtle parsing (small dataset) - baseline
BENCHMARK_F(RdfParserBenchmark, ParseBasicTurtle, 1000) {
  auto tripleCount = parseTurtle(basicTurtle);
  ad_utility::unused(tripleCount);
}

// Benchmark: Larger N3 dataset parsing
BENCHMARK_F(RdfParserBenchmark, ParsePeopleN3, 100) {
  auto tripleCount = parseN3(peopleN3);
  ad_utility::unused(tripleCount);
}

// Benchmark: Larger Turtle dataset parsing - baseline
BENCHMARK_F(RdfParserBenchmark, ParsePeopleTurtle, 100) {
  auto tripleCount = parseTurtle(peopleTurtle);
  ad_utility::unused(tripleCount);
}

// Test: Verify N3 and Turtle produce same results for compatible input
TEST_F(RdfParserBenchmark, N3TurtleCompatibility) {
  // Both parsers should produce the same number of triples for identical input
  size_t n3Count = parseN3(basicN3);
  size_t turtleCount = parseTurtle(basicTurtle);

  std::cout << "N3 triples parsed: " << n3Count << std::endl;
  std::cout << "Turtle triples parsed: " << turtleCount << std::endl;

  EXPECT_EQ(n3Count, turtleCount)
      << "N3 and Turtle parsers should produce identical results for "
         "Turtle-compatible input";
}

// Test: Verify parsing correctness for specific N3 features
TEST_F(RdfParserBenchmark, N3FeaturesCorrectness) {
  std::string_view n3WithPrefixes = R"(
    @prefix ex: <http://example.org/> .
    @prefix foaf: <http://xmlns.com/foaf/0.1/> .
    ex:alice foaf:knows ex:bob .
  )";

  size_t tripleCount = parseN3(n3WithPrefixes);
  EXPECT_EQ(tripleCount, 1) << "N3 parser should correctly handle prefixes";

  std::string_view n3WithLanguageTags = R"(
    <http://example.org/book> <http://purl.org/dc/elements/1.1/title>
    "The Title"@en, "Le Titre"@fr .
  )";

  tripleCount = parseN3(n3WithLanguageTags);
  EXPECT_EQ(tripleCount, 2) << "N3 parser should handle language tags";
}

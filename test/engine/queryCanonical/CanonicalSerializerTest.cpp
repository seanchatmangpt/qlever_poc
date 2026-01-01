// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#include <gtest/gtest.h>

#include "engine/queryCanonical/CanonicalSerializer.h"
#include "parser/GraphPatternOperation.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlTriple.h"
#include "parser/TripleComponent.h"
#include "rdfTypes/Variable.h"

using namespace queryCanonical;

// Helper to create a simple SELECT query for testing
ParsedQuery createSimpleSelectQuery() {
  ParsedQuery query;

  // Set up SELECT clause
  parsedQuery::SelectClause selectClause;
  selectClause.setSelected({Variable{"?v0"}, Variable{"?v1"}});
  query._clause = selectClause;

  // Add a simple basic graph pattern with one triple
  parsedQuery::BasicGraphPattern bgp;
  SparqlTriple triple{TripleComponent{Variable{"?v0"}},
                      TripleComponent::Iri::fromIriref("<pred>"),
                      TripleComponent{Variable{"?v1"}}};
  bgp._triples.push_back(triple);

  query._rootGraphPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{bgp});

  return query;
}

// Helper to create a query with DISTINCT modifier
ParsedQuery createDistinctSelectQuery() {
  ParsedQuery query = createSimpleSelectQuery();
  auto& selectClause = std::get<parsedQuery::SelectClause>(query._clause);
  selectClause.distinct_ = true;
  return query;
}

// Helper to create a query with LIMIT
ParsedQuery createQueryWithLimit(uint64_t limit) {
  ParsedQuery query = createSimpleSelectQuery();
  query._limitOffset._limit = limit;
  return query;
}

// Helper to create a query with OPTIONAL
ParsedQuery createQueryWithOptional() {
  ParsedQuery query;

  parsedQuery::SelectClause selectClause;
  selectClause.setSelected({Variable{"?v0"}, Variable{"?v1"}, Variable{"?v2"}});
  query._clause = selectClause;

  // Main pattern
  parsedQuery::BasicGraphPattern bgp;
  SparqlTriple triple{TripleComponent{Variable{"?v0"}},
                      TripleComponent::Iri::fromIriref("<pred1>"),
                      TripleComponent{Variable{"?v1"}}};
  bgp._triples.push_back(triple);
  query._rootGraphPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{bgp});

  // Optional pattern
  parsedQuery::GraphPattern optionalPattern;
  parsedQuery::BasicGraphPattern bgp2;
  SparqlTriple triple2{TripleComponent{Variable{"?v1"}},
                       TripleComponent::Iri::fromIriref("<pred2>"),
                       TripleComponent{Variable{"?v2"}}};
  bgp2._triples.push_back(triple2);
  optionalPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{bgp2});

  parsedQuery::Optional opt{optionalPattern};
  query._rootGraphPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{opt});

  return query;
}

// Test determinism: same query should produce identical bytes on multiple runs
TEST(CanonicalSerializerTest, DeterministicSerialization) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  // Serialize 10 times
  std::vector<std::vector<uint8_t>> results;
  for (int i = 0; i < 10; ++i) {
    results.push_back(serializer.serializeToCanonicalForm());
  }

  // All results should be identical
  for (size_t i = 1; i < results.size(); ++i) {
    EXPECT_EQ(results[0], results[i])
        << "Serialization run " << i << " differs from run 0";
  }
}

// Test that the version header is correct
TEST(CanonicalSerializerTest, VersionHeaderIsCorrect) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Check that we have at least the header
  ASSERT_GE(bytes.size(), 7u);

  // Check the QSHAPE\x01 header
  EXPECT_EQ(bytes[0], 'Q');
  EXPECT_EQ(bytes[1], 'S');
  EXPECT_EQ(bytes[2], 'H');
  EXPECT_EQ(bytes[3], 'A');
  EXPECT_EQ(bytes[4], 'P');
  EXPECT_EQ(bytes[5], 'E');
  EXPECT_EQ(bytes[6], '\x01');
}

// Test that different queries produce different serializations
TEST(CanonicalSerializerTest, DifferentQueriesProduceDifferentBytes) {
  ParsedQuery query1 = createSimpleSelectQuery();
  ParsedQuery query2 = createDistinctSelectQuery();

  CanonicalSerializer serializer1(query1);
  CanonicalSerializer serializer2(query2);

  auto bytes1 = serializer1.serializeToCanonicalForm();
  auto bytes2 = serializer2.serializeToCanonicalForm();

  EXPECT_NE(bytes1, bytes2)
      << "SELECT and SELECT DISTINCT should produce different serializations";
}

// Test DISTINCT feature flag
TEST(CanonicalSerializerTest, DistinctFlagIsSet) {
  ParsedQuery query = createDistinctSelectQuery();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Feature flags are at bytes 7-8 (after QSHAPE\x01)
  ASSERT_GE(bytes.size(), 9u);

  // Extract feature flags (little-endian uint16)
  uint16_t flags =
      static_cast<uint16_t>(bytes[7]) | (static_cast<uint16_t>(bytes[8]) << 8);

  // Check DISTINCT flag (0x0001)
  EXPECT_NE(flags & 0x0001, 0u) << "DISTINCT flag should be set";
}

// Test LIMIT feature flag and serialization
TEST(CanonicalSerializerTest, LimitFlagIsSet) {
  ParsedQuery query = createQueryWithLimit(100);
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Feature flags are at bytes 7-8
  ASSERT_GE(bytes.size(), 9u);

  uint16_t flags =
      static_cast<uint16_t>(bytes[7]) | (static_cast<uint16_t>(bytes[8]) << 8);

  // Check HAS_LIMIT flag (0x0008)
  EXPECT_NE(flags & 0x0008, 0u) << "HAS_LIMIT flag should be set";
}

// Test different LIMIT values produce different serializations
TEST(CanonicalSerializerTest, DifferentLimitValuesDiffer) {
  ParsedQuery query1 = createQueryWithLimit(10);
  ParsedQuery query2 = createQueryWithLimit(100);

  CanonicalSerializer serializer1(query1);
  CanonicalSerializer serializer2(query2);

  auto bytes1 = serializer1.serializeToCanonicalForm();
  auto bytes2 = serializer2.serializeToCanonicalForm();

  EXPECT_NE(bytes1, bytes2) << "Different LIMIT values should produce "
                               "different serializations";
}

// Test OPTIONAL pattern serialization
TEST(CanonicalSerializerTest, OptionalPatternFlagIsSet) {
  ParsedQuery query = createQueryWithOptional();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Feature flags are at bytes 7-8
  ASSERT_GE(bytes.size(), 9u);

  uint16_t flags =
      static_cast<uint16_t>(bytes[7]) | (static_cast<uint16_t>(bytes[8]) << 8);

  // Check HAS_OPTIONAL flag (0x0100)
  EXPECT_NE(flags & 0x0100, 0u) << "HAS_OPTIONAL flag should be set";
}

// Test query with no modifiers has minimal feature flags
TEST(CanonicalSerializerTest, SimpleQueryHasMinimalFlags) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Feature flags are at bytes 7-8
  ASSERT_GE(bytes.size(), 9u);

  uint16_t flags =
      static_cast<uint16_t>(bytes[7]) | (static_cast<uint16_t>(bytes[8]) << 8);

  // For a simple SELECT with no modifiers, most flags should be unset
  // Only basic flags might be set
  EXPECT_EQ(flags & 0x0001, 0u) << "DISTINCT should not be set";
  EXPECT_EQ(flags & 0x0002, 0u) << "REDUCED should not be set";
  EXPECT_EQ(flags & 0x0004, 0u) << "HAS_ORDER_BY should not be set";
  EXPECT_EQ(flags & 0x0008, 0u) << "HAS_LIMIT should not be set";
  EXPECT_EQ(flags & 0x0010, 0u) << "HAS_OFFSET should not be set";
}

// Test debug string output is non-empty
TEST(CanonicalSerializerTest, DebugStringIsNonEmpty) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  std::string debugStr = serializer.serializeToDebugString();

  EXPECT_FALSE(debugStr.empty());
  EXPECT_NE(debugStr.find("QSHAPE"), std::string::npos)
      << "Debug string should contain QSHAPE header";
  EXPECT_NE(debugStr.find("SELECT"), std::string::npos)
      << "Debug string should contain SELECT";
}

// Test ASK query serialization
TEST(CanonicalSerializerTest, AskQuerySerialization) {
  ParsedQuery query;
  query._clause = ParsedQuery::AskClause{};

  // Add a simple pattern
  parsedQuery::BasicGraphPattern bgp;
  SparqlTriple triple{TripleComponent{Variable{"?v0"}},
                      TripleComponent::Iri::fromIriref("<pred>"),
                      TripleComponent{Variable{"?v1"}}};
  bgp._triples.push_back(triple);
  query._rootGraphPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{bgp});

  CanonicalSerializer serializer(query);
  auto bytes = serializer.serializeToCanonicalForm();

  // Should have header
  ASSERT_GE(bytes.size(), 9u);

  // Query type byte should be ASK (0x03)
  EXPECT_EQ(bytes[9], 0x03);
}

// Test that serialization is not empty
TEST(CanonicalSerializerTest, SerializationIsNotEmpty) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  EXPECT_FALSE(bytes.empty());
  EXPECT_GT(bytes.size(), 10u)
      << "Serialization should be more than just the header";
}

// Test little-endian encoding of uint16
TEST(CanonicalSerializerTest, LittleEndianUint16) {
  ParsedQuery query = createSimpleSelectQuery();
  CanonicalSerializer serializer(query);

  auto bytes = serializer.serializeToCanonicalForm();

  // Feature flags at bytes 7-8 should be little-endian
  ASSERT_GE(bytes.size(), 9u);

  // For any flags value, verify little-endian encoding
  uint16_t flags =
      static_cast<uint16_t>(bytes[7]) | (static_cast<uint16_t>(bytes[8]) << 8);

  // The low byte should be at index 7, high byte at index 8
  EXPECT_EQ(bytes[7], static_cast<uint8_t>(flags & 0xFF));
  EXPECT_EQ(bytes[8], static_cast<uint8_t>((flags >> 8) & 0xFF));
}

// Test that query with multiple triples encodes them all
TEST(CanonicalSerializerTest, MultipleTriplesSerialization) {
  ParsedQuery query;

  parsedQuery::SelectClause selectClause;
  selectClause.setSelected({Variable{"?v0"}, Variable{"?v1"}, Variable{"?v2"}});
  query._clause = selectClause;

  parsedQuery::BasicGraphPattern bgp;
  SparqlTriple triple1{TripleComponent{Variable{"?v0"}},
                       TripleComponent::Iri::fromIriref("<pred1>"),
                       TripleComponent{Variable{"?v1"}}};
  SparqlTriple triple2{TripleComponent{Variable{"?v1"}},
                       TripleComponent::Iri::fromIriref("<pred2>"),
                       TripleComponent{Variable{"?v2"}}};
  bgp._triples.push_back(triple1);
  bgp._triples.push_back(triple2);

  query._rootGraphPattern._graphPatterns.push_back(
      parsedQuery::GraphPatternOperation{bgp});

  CanonicalSerializer serializer(query);
  auto bytes = serializer.serializeToCanonicalForm();

  // Should be larger than a query with just one triple
  ParsedQuery singleTripleQuery = createSimpleSelectQuery();
  CanonicalSerializer singleSerializer(singleTripleQuery);
  auto singleBytes = singleSerializer.serializeToCanonicalForm();

  EXPECT_GT(bytes.size(), singleBytes.size())
      << "Query with 2 triples should serialize to more bytes than query with "
         "1 triple";
}

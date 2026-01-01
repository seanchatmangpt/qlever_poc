// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include <gtest/gtest.h>

#include "engine/QueryExecutionContext.h"
#include "engine/queryCanonical/FingerprintRegistry.h"
#include "engine/queryCanonical/QueryFingerprintingPipeline.h"
#include "parser/ParsedQuery.h"
#include "util/IndexTestHelpers.h"

using namespace queryCanonical;

// Test fixture for QueryFingerprintingPipeline tests
class QueryFingerprintingPipelineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear registry before each test
    FingerprintRegistry::getInstance().clear();
  }

  // Helper to create a minimal ParsedQuery
  ParsedQuery createMinimalQuery() {
    ParsedQuery query;
    query._originalString = "SELECT * WHERE { ?s ?p ?o }";
    return query;
  }

  // Helper to create a QueryExecutionContext
  std::unique_ptr<QueryExecutionContext> createContext() {
    auto qec = ad_utility::testing::getQec();
    return std::make_unique<QueryExecutionContext>(
        qec->getIndex(), &qec->getQueryTreeCache(), qec->getAllocator(),
        qec->getSortPerformanceEstimator(), &qec->namedResultCache(),
        &qec->materializedViewsManager());
  }
};

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, GeneratesFingerprintSuccessfully) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Verify fingerprint is valid
  EXPECT_FALSE(fingerprint.canonicalSerialization().empty());
  EXPECT_NE(fingerprint.shapeHash(), 0u);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, IncludesEpochBinding) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Verify epoch binding
  EXPECT_EQ(fingerprint.epochId(), context->getCurrentEpochId());
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, ProducesStatistics) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Verify statistics are present
  ASSERT_TRUE(fingerprint.stats().has_value());
  const auto& stats = fingerprint.stats().value();

  EXPECT_GE(stats.serializationLength, 0u);
  EXPECT_GE(stats.totalTime.count(), 0);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, DeterministicQueries) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Simple queries should be deterministic
  EXPECT_TRUE(fingerprint.isDeterministic());
  EXPECT_FALSE(fingerprint.features().hasNow);
  EXPECT_FALSE(fingerprint.features().hasRand);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, SameQueryProducesSameHash) {
  auto query1 = createMinimalQuery();
  auto query2 = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline1(std::move(query1), *context);
  QueryFingerprintingPipeline pipeline2(std::move(query2), *context);

  QueryFingerprint fp1 = pipeline1.generateFingerprint();
  QueryFingerprint fp2 = pipeline2.generateFingerprint();

  // Same query should produce same hash
  EXPECT_EQ(fp1.shapeHash(), fp2.shapeHash());
  EXPECT_EQ(fp1, fp2);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, RegistryTracksFingerprints) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Record in registry
  FingerprintRegistry::getInstance().recordFingerprint(fingerprint);

  // Verify registry tracked it
  EXPECT_EQ(FingerprintRegistry::getInstance().getTotalQueries(), 1u);

  auto frequencies = FingerprintRegistry::getInstance().getShapeFrequencies();
  EXPECT_EQ(frequencies.size(), 1u);
  EXPECT_EQ(frequencies[fingerprint.shapeHash()], 1u);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, RegistryTracksMultipleQueries) {
  auto context = createContext();

  // Generate and record multiple fingerprints
  for (int i = 0; i < 5; ++i) {
    auto query = createMinimalQuery();
    QueryFingerprintingPipeline pipeline(std::move(query), *context);
    QueryFingerprint fingerprint = pipeline.generateFingerprint();
    FingerprintRegistry::getInstance().recordFingerprint(fingerprint);
  }

  // Verify registry tracked all
  EXPECT_EQ(FingerprintRegistry::getInstance().getTotalQueries(), 5u);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, TopShapesReturnsCorrectly) {
  auto context = createContext();

  // Generate and record the same fingerprint multiple times
  auto query = createMinimalQuery();
  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  for (int i = 0; i < 10; ++i) {
    FingerprintRegistry::getInstance().recordFingerprint(fingerprint);
  }

  auto topShapes = FingerprintRegistry::getInstance().getTopShapes(1);
  ASSERT_EQ(topShapes.size(), 1u);
  EXPECT_EQ(topShapes[0].first, fingerprint.shapeHash());
  EXPECT_EQ(topShapes[0].second, 10u);
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, ContextStoresFingerprint) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  // Store in context
  context->setQueryFingerprint(fingerprint);

  // Verify retrieval
  auto retrieved = context->getQueryFingerprint();
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->shapeHash(), fingerprint.shapeHash());
}

// ____________________________________________________________________________
TEST_F(QueryFingerprintingPipelineTest, ContextRetrievesStats) {
  auto query = createMinimalQuery();
  auto context = createContext();

  QueryFingerprintingPipeline pipeline(std::move(query), *context);
  QueryFingerprint fingerprint = pipeline.generateFingerprint();

  context->setQueryFingerprint(fingerprint);

  // Verify stats retrieval
  auto stats = context->getStats();
  ASSERT_TRUE(stats.has_value());
  EXPECT_GE(stats->serializationLength, 0u);
}

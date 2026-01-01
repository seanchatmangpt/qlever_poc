// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/QueryFingerprintingPipeline.h"

namespace queryCanonical {

// ____________________________________________________________________________
QueryFingerprintingPipeline::QueryFingerprintingPipeline(
    ParsedQuery query, const QueryExecutionContext& executionContext)
    : query_(std::move(query)), executionContext_(executionContext) {}

// ____________________________________________________________________________
QueryFingerprint QueryFingerprintingPipeline::generateFingerprint() {
  auto startTime = std::chrono::steady_clock::now();

  // Step A: Parse (already done - query_ is the parsed query)

  // Step B: Normalize IRIs
  stepNormalizeIris();

  // Step C: Rename variables
  stepRenameVariables();

  // Step D: Lift constants
  stepLiftConstants();

  // Step E: Normalize triple patterns
  stepNormalizeTriplePatterns();

  // Step F: Serialize to canonical string
  std::string canonicalSerialization = stepSerialize();

  // Step G: Analyze features (determinism)
  DeterminismFeatures features = stepAnalyzeFeatures();

  // Step H: Embed epoch context
  ad_utility::EpochId epochId = executionContext_.getCurrentEpochId();
  std::string manifestSha256 = executionContext_.getEpochDeterministicKey();

  // Calculate total time
  auto endTime = std::chrono::steady_clock::now();
  stats_.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  // Create fingerprint
  QueryFingerprint fingerprint(std::move(canonicalSerialization), features,
                                epochId, std::move(manifestSha256));
  fingerprint.setStats(stats_);

  return fingerprint;
}

// ____________________________________________________________________________
void QueryFingerprintingPipeline::stepNormalizeIris() {
  iriNormalizer_.normalizeQuery(query_);
  stats_.normalizedIriCount = iriNormalizer_.getNormalizedCount();
}

// ____________________________________________________________________________
void QueryFingerprintingPipeline::stepRenameVariables() {
  variableNormalizer_.normalizeQuery(query_);
  stats_.renamedVariableCount = variableNormalizer_.getRenamedCount();
}

// ____________________________________________________________________________
void QueryFingerprintingPipeline::stepLiftConstants() {
  constantExtractor_.extractConstants(query_);
  stats_.liftedConstantCount = constantExtractor_.getLiftedCount();
}

// ____________________________________________________________________________
void QueryFingerprintingPipeline::stepNormalizeTriplePatterns() {
  triplePatternNormalizer_.normalizeQuery(query_);
  stats_.normalizedTripleCount =
      triplePatternNormalizer_.getNormalizedTripleCount();
}

// ____________________________________________________________________________
std::string QueryFingerprintingPipeline::stepSerialize() {
  std::string serialization = canonicalSerializer_.serialize(query_);
  stats_.serializationLength = canonicalSerializer_.getSerializationLength();
  return serialization;
}

// ____________________________________________________________________________
DeterminismFeatures QueryFingerprintingPipeline::stepAnalyzeFeatures() {
  return determinismClassifier_.analyze(query_);
}

}  // namespace queryCanonical

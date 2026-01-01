// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINTINGPIPELINE_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINTINGPIPELINE_H

#include <chrono>

#include "engine/QueryExecutionContext.h"
#include "engine/queryCanonical/CanonicalSerializer.h"
#include "engine/queryCanonical/ConstantExtractor.h"
#include "engine/queryCanonical/DeterminismClassifier.h"
#include "engine/queryCanonical/IriNormalizer.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "engine/queryCanonical/TriplePatternNormalizer.h"
#include "engine/queryCanonical/VariableNormalizer.h"
#include "parser/ParsedQuery.h"

namespace queryCanonical {

// Orchestrates the complete query fingerprinting pipeline
// Steps:
// A. Parse (use existing ParsedQuery)
// B. Normalize IRIs (expand prefixes, canonicalize)
// C. Rename variables (to ?v0, ?v1, ?v2, ...)
// D. Lift constants (replace with ?const_0, ?const_1, ...)
// E. Normalize triple patterns (sort, canonicalize)
// F. Serialize (to canonical string)
// G. Analyze features (determinism classification)
// H. Embed epoch context (from ExecutionContext)
class QueryFingerprintingPipeline {
 public:
  // Constructor takes parsed query and execution context
  explicit QueryFingerprintingPipeline(
      ParsedQuery query, const QueryExecutionContext& executionContext);

  // Generate the fingerprint by running the full pipeline
  [[nodiscard]] QueryFingerprint generateFingerprint();

  // Get detailed statistics about the fingerprinting process
  [[nodiscard]] const QueryFingerprintStats& getStats() const {
    return stats_;
  }

 private:
  ParsedQuery query_;
  const QueryExecutionContext& executionContext_;
  QueryFingerprintStats stats_;

  // Pipeline components
  IriNormalizer iriNormalizer_;
  VariableNormalizer variableNormalizer_;
  ConstantExtractor constantExtractor_;
  TriplePatternNormalizer triplePatternNormalizer_;
  CanonicalSerializer canonicalSerializer_;
  DeterminismClassifier determinismClassifier_;

  // Execute each pipeline step
  void stepNormalizeIris();
  void stepRenameVariables();
  void stepLiftConstants();
  void stepNormalizeTriplePatterns();
  [[nodiscard]] std::string stepSerialize();
  [[nodiscard]] DeterminismFeatures stepAnalyzeFeatures();
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINTINGPIPELINE_H

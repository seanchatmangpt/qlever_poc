// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: AI Agent Implementation
//
// Purpose: EnvelopeDiff - Machine-readable comparison of two ExecutionDigests
// (EPIC 4 - Deterministic Execution Envelope)
//
// EnvelopeDiff provides structured comparison between two ExecutionDigests,
// identifying which components differ. This enables precise diagnosis of
// execution divergence during replay or cross-machine comparison.
//
// Use cases:
// - Replay verification: Compare captured vs replayed execution
// - Cross-machine reproducibility: Verify same query behaves identically
// - Regression detection: Identify when plan or resource behavior changes
// - Debugging: Pinpoint exactly which aspect of execution diverged
//
// Invariants:
// - is_identical == true IFF all component hashes match
// - If fingerprint_differs == true, comparison is invalid (different queries)
// - All differences include before/after hash values for debugging

#ifndef QLEVER_SRC_ENGINE_READPLANE_ENVELOPEDIFF_H
#define QLEVER_SRC_ENGINE_READPLANE_ENVELOPEDIFF_H

#include <optional>
#include <string>

#include "engine/readPlane/ExecutionDigest.h"
#include "util/json.h"

namespace readPlane {

// ComponentDiff - Records before/after values for a single hash component.
// Only populated when the component differs between digests.
struct ComponentDiff {
  std::string component_name;  // e.g., "plan_hash", "resource_signature"
  std::string before_hash;     // Hash value from first digest
  std::string after_hash;      // Hash value from second digest

  ComponentDiff() = default;
  ComponentDiff(std::string name, std::string before, std::string after)
      : component_name(std::move(name)),
        before_hash(std::move(before)),
        after_hash(std::move(after)) {}

  // Serialize to JSON
  [[nodiscard]] nlohmann::ordered_json toJson() const;
};

// DivergenceClassification - Categorizes the type of divergence detected.
// Used for automated triage and alerting.
enum class DivergenceClassification {
  // No divergence - executions are identical
  IDENTICAL,

  // Query identity mismatch - should never happen if comparing same query
  QUERY_FINGERPRINT_MISMATCH,

  // Plan divergence - different execution plan was used
  // Indicates non-deterministic planning or plan reuse failure
  PLAN_DIVERGENCE,

  // Resource envelope divergence - same plan, different resource behavior
  // May indicate cache state differences or memory allocation variations
  RESOURCE_ENVELOPE_DIVERGENCE,

  // Result shape divergence - different result structure
  // May indicate data differences or non-deterministic execution
  RESULT_SHAPE_DIVERGENCE,

  // Result length divergence only - same structure, different size
  // May indicate pagination differences or data changes
  RESULT_LENGTH_DIVERGENCE,

  // Multiple divergences - more than one component differs
  MULTIPLE_DIVERGENCES
};

// EnvelopeDiff - Complete comparison result between two ExecutionDigests.
//
// Provides:
// - Boolean flags for quick checks
// - Detailed component diffs for debugging
// - Classification for automated handling
// - JSON-LD serialization for logging/storage
struct EnvelopeDiff {
  // ========== BOOLEAN FLAGS ==========

  // True IFF all component hashes match (digests are identical)
  bool is_identical = false;

  // True if query fingerprint differs (should never happen for same query)
  bool fingerprint_differs = false;

  // True if plan hash differs (different execution plan used)
  bool plan_changed = false;

  // True if resource signature differs (different cache/memory behavior)
  bool resource_envelope_changed = false;

  // True if result shape hash differs (different result structure)
  bool result_shape_changed = false;

  // True if result length hash differs (different result size)
  bool result_length_changed = false;

  // ========== CLASSIFICATION ==========

  // Categorization of the divergence type for automated handling
  DivergenceClassification classification =
      DivergenceClassification::IDENTICAL;

  // ========== COMPONENT DETAILS ==========

  // List of all components that differ, with before/after values
  std::vector<ComponentDiff> differing_components;

  // Digest hashes for reference
  std::string digest1_hash;  // Full digest hash of first input
  std::string digest2_hash;  // Full digest hash of second input

  // ========== METADATA ==========

  // Format version for forward compatibility
  static constexpr const char* FORMAT_VERSION = "1.0";

  // ========== CONSTRUCTORS ==========

  // Default constructor (empty diff)
  EnvelopeDiff() = default;

  // ========== FACTORY METHODS ==========

  // Compute EnvelopeDiff from two ExecutionDigests.
  // This is the primary entry point for comparing digests.
  //
  // Parameters:
  // - digest1: First ExecutionDigest (e.g., from capture)
  // - digest2: Second ExecutionDigest (e.g., from replay)
  //
  // Returns: EnvelopeDiff describing all differences
  //
  // Notes:
  // - Order matters: digest1 is "before", digest2 is "after"
  // - If fingerprint_differs is true, comparison is suspect (different
  // queries)
  [[nodiscard]] static EnvelopeDiff compute(const ExecutionDigest& digest1,
                                            const ExecutionDigest& digest2);

  // ========== QUERY METHODS ==========

  // Get the number of components that differ
  [[nodiscard]] size_t differenceCount() const {
    return differing_components.size();
  }

  // Check if a specific component differs
  [[nodiscard]] bool hasComponentDiff(const std::string& component_name) const;

  // Get the diff for a specific component (if it differs)
  [[nodiscard]] std::optional<ComponentDiff> getComponentDiff(
      const std::string& component_name) const;

  // ========== SERIALIZATION ==========

  // Serialize to deterministic JSON-LD format.
  // Suitable for storage in ReplayResult artifacts.
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;

  // Deserialize from JSON-LD format.
  // Throws std::runtime_error if format is invalid.
  [[nodiscard]] static EnvelopeDiff fromJsonLD(
      const nlohmann::ordered_json& json);

  // ========== DEBUG ==========

  // Human-readable summary string (for logging only).
  [[nodiscard]] std::string toString() const;

  // Human-readable classification name
  [[nodiscard]] static std::string classificationToString(
      DivergenceClassification c);

  // ========== INTERNAL HELPERS ==========

 private:
  // Classify the divergence based on which components differ
  static DivergenceClassification classifyDivergence(
      bool fingerprint_diff, bool plan_diff, bool resource_diff,
      bool shape_diff, bool length_diff);
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_ENVELOPEDIFF_H

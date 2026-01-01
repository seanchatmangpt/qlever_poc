// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_CONTRACTS_RESULTDIGEST_H
#define QLEVER_SRC_ENGINE_CONTRACTS_RESULTDIGEST_H

#include <absl/strings/str_join.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "util/CryptographicHashUtils.h"

namespace qlever::contracts {

// Compute a deterministic SHA-256 digest over a result set
// Used for reproducibility testing in conformance runners
class ResultDigest {
 public:
  // Compute digest from a vector of strings
  // Each string represents a row or result element
  // Results are sorted before hashing to ensure determinism
  static std::string computeDigest(std::vector<std::string> results) {
    // Sort for determinism
    std::sort(results.begin(), results.end());

    // Concatenate with newline separator
    std::string concatenated = absl::StrJoin(results, "\n");

    // Compute SHA-256 hash
    ad_utility::HashSha256 hasher;
    auto hashBytes = hasher(concatenated);

    // Convert to hex string
    return absl::StrJoin(hashBytes, "", ad_utility::hexFormatter);
  }

  // Compute digest from a single string (e.g., serialized JSON)
  static std::string computeDigestFromString(std::string_view input) {
    ad_utility::HashSha256 hasher;
    auto hashBytes = hasher(input);
    return absl::StrJoin(hashBytes, "", ad_utility::hexFormatter);
  }

  // Compute digest from violation report JSON
  // This allows comparing violation reports across test runs
  static std::string computeViolationDigest(const nlohmann::json& violationReport) {
    // Serialize to string (sorted keys for determinism)
    std::string serialized = violationReport.dump();
    return computeDigestFromString(serialized);
  }

  // Verify that a digest matches expected value
  static bool verifyDigest(const std::string& computed, const std::string& expected) {
    return computed == expected;
  }

  // Compute incremental digest by combining existing digest with new data
  // Useful for streaming scenarios where results arrive incrementally
  static std::string combineDigests(const std::string& digest1,
                                     const std::string& digest2) {
    // Concatenate and rehash for combining
    std::string combined = digest1 + digest2;
    return computeDigestFromString(combined);
  }
};

// Helper to compute digest from a range of values
template <typename Range>
std::string computeRangeDigest(const Range& range) {
  std::vector<std::string> elements;
  for (const auto& elem : range) {
    // Convert element to string representation
    // This requires element type to be streamable
    std::ostringstream oss;
    oss << elem;
    elements.push_back(oss.str());
  }
  return ResultDigest::computeDigest(std::move(elements));
}

}  // namespace qlever::contracts

#endif  // QLEVER_SRC_ENGINE_CONTRACTS_RESULTDIGEST_H

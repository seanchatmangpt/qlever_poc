// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: AI Agent Implementation
//
// Implementation of ExecutionDigest and EnvelopeDiff
// (EPIC 4 - Deterministic Execution Envelope)

#include "engine/readPlane/ExecutionDigest.h"

#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "engine/readPlane/EnvelopeDiff.h"
#include "util/CryptographicHashUtils.h"

namespace readPlane {

// ============================================================================
// SHA-256 Utility Functions
// ============================================================================

// Compute SHA256 hash and return as 64-char lowercase hex string.
// Uses OpenSSL EVP interface for portability.
std::string ExecutionDigest::sha256Hex(std::string_view input) {
  // Use the existing HashSha256 utility
  ad_utility::HashSha256 hasher;
  std::vector<unsigned char> digest = hasher(input);

  // Convert to hex string
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : digest) {
    oss << std::setw(2) << static_cast<unsigned int>(c);
  }
  return oss.str();
}

// ============================================================================
// ResourceMetrics Implementation
// ============================================================================

std::string ResourceMetrics::toCanonicalBytes() const {
  // Deterministic serialization: fixed format, sorted fields
  // Format: "RSRC:bytes_hits:plan_hits:neg_hits:mem_pages:bytes_written"
  std::ostringstream oss;
  oss << "RSRC:" << bytes_cache_hits << ":" << plan_cache_hits << ":"
      << negative_cache_hits << ":" << memory_pages_accessed << ":"
      << result_bytes_written;
  return oss.str();
}

// ============================================================================
// ResultMetadata Implementation
// ============================================================================

std::string ResultMetadata::toCanonicalBytes() const {
  // Deterministic serialization: fixed format, sorted column types
  // Format: "RESULT:cols:rows:format:type1,type2,..."
  std::ostringstream oss;
  oss << "RESULT:" << column_count << ":" << row_count << ":" << output_format
      << ":";

  // Sort column types for determinism
  std::vector<std::string> sorted_types = column_types;
  std::sort(sorted_types.begin(), sorted_types.end());
  oss << absl::StrJoin(sorted_types, ",");

  return oss.str();
}

// ============================================================================
// PlanInfo Implementation
// ============================================================================

std::string PlanInfo::toCanonicalBytes() const {
  // Deterministic serialization: cache key + sorted descriptors + estimates
  // Format: "PLAN:cache_key:cost:size:desc1|desc2|..."
  std::ostringstream oss;
  oss << "PLAN:" << plan_cache_key << ":" << cost_estimate << ":"
      << size_estimate << ":";

  // Descriptors are already in deterministic order (depth-first traversal)
  oss << absl::StrJoin(operation_descriptors, "|");

  return oss.str();
}

// ============================================================================
// ExecutionDigest Implementation
// ============================================================================

std::string ExecutionDigest::computeFinalDigest(
    const std::string& query_fp, const std::string& plan,
    const std::string& resource, const std::string& result_len,
    const std::string& result_shape) {
  // Concatenate all component hashes in fixed order
  // Order is critical for determinism - defined in EPIC4_SHARED_INVARIANTS.md
  std::string combined =
      absl::StrCat(query_fp, plan, resource, result_len, result_shape);
  return sha256Hex(combined);
}

// Serialize QueryFingerprint to a deterministic string for hashing.
// This creates a canonical representation of all fingerprint fields.
static std::string serializeFingerprint(
    const queryCanonical::QueryFingerprint& fp) {
  // Deterministic format: fixed field order, sorted for reproducibility
  // Format: "FP:epoch_id:manifest:raw:normalized:shape:params:flags:feature_vec"
  std::ostringstream oss;
  oss << "FP:" << fp.epoch_id << ":" << fp.epoch_manifest_sha256 << ":"
      << fp.raw_query_sha256 << ":" << fp.normalized_text_sha256 << ":"
      << fp.shape_sha256 << ":" << fp.params_sha256 << ":"
      << static_cast<uint32_t>(fp.feature_flags) << ":"
      << fp.shape_feature_vector_hash;
  return oss.str();
}

ExecutionDigest ExecutionDigest::compute(
    const queryCanonical::QueryFingerprint& fingerprint, const PlanInfo& plan,
    const ResourceMetrics& resources, const ResultMetadata& result,
    uint64_t result_size) {
  ExecutionDigest digest;

  // Validate inputs (fail-closed semantics)
  if (!fingerprint.isValid()) {
    throw std::runtime_error(
        "ExecutionDigest::compute: Invalid QueryFingerprint - fail-closed");
  }

  // 1. Query fingerprint hash - serialize fingerprint and hash
  // Uses deterministic serialization of all fingerprint fields
  std::string fingerprint_serialized = serializeFingerprint(fingerprint);
  digest.query_fingerprint_sha256 = sha256Hex(fingerprint_serialized);

  // 2. Plan hash - serialize plan info and hash
  std::string plan_serialized = plan.toCanonicalBytes();
  digest.plan_hash = sha256Hex(plan_serialized);

  // 3. Resource signature - serialize resource metrics and hash
  std::string resource_serialized = resources.toCanonicalBytes();
  digest.resource_signature = sha256Hex(resource_serialized);

  // 4. Result length hash - SHA256 of size as string
  std::string size_str = std::to_string(result_size);
  digest.result_length_hash = sha256Hex(size_str);

  // 5. Result shape hash - serialize result metadata and hash
  std::string result_serialized = result.toCanonicalBytes();
  digest.result_shape_hash = sha256Hex(result_serialized);

  // 6. Final digest hash - combine all components
  digest.digest_hash = computeFinalDigest(
      digest.query_fingerprint_sha256, digest.plan_hash,
      digest.resource_signature, digest.result_length_hash,
      digest.result_shape_hash);

  return digest;
}

bool ExecutionDigest::isValid() const {
  // All hash fields must be 64 hex characters (256 bits = 32 bytes = 64 hex)
  auto isValidHex = [](const std::string& s) {
    if (s.length() != 64) return false;
    for (char c : s) {
      if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
  };

  return isValidHex(query_fingerprint_sha256) && isValidHex(plan_hash) &&
         isValidHex(resource_signature) && isValidHex(result_length_hash) &&
         isValidHex(result_shape_hash) && isValidHex(digest_hash);
}

bool ExecutionDigest::verifyIntegrity() const {
  // Recompute digest_hash from components and verify
  std::string expected =
      computeFinalDigest(query_fingerprint_sha256, plan_hash,
                         resource_signature, result_length_hash,
                         result_shape_hash);
  return digest_hash == expected;
}

nlohmann::ordered_json ExecutionDigest::toJsonLD() const {
  // JSON-LD format with deterministic field ordering (ordered_json)
  nlohmann::ordered_json json;

  // JSON-LD context
  json["@context"] = {
      {"@vocab", "https://qlever.cs.uni-freiburg.de/readplane/v1#"},
      {"digest_hash", "@id"}};

  json["@type"] = "ExecutionDigest";
  json["format_version"] = FORMAT_VERSION;

  // Component hashes in defined order
  json["query_fingerprint_sha256"] = query_fingerprint_sha256;
  json["plan_hash"] = plan_hash;
  json["resource_signature"] = resource_signature;
  json["result_length_hash"] = result_length_hash;
  json["result_shape_hash"] = result_shape_hash;

  // Final digest
  json["digest_hash"] = digest_hash;

  return json;
}

ExecutionDigest ExecutionDigest::fromJsonLD(const nlohmann::ordered_json& json) {
  ExecutionDigest digest;

  try {
    // Verify type
    if (json.at("@type").get<std::string>() != "ExecutionDigest") {
      throw std::runtime_error("Invalid @type: expected ExecutionDigest");
    }

    // Extract component hashes
    digest.query_fingerprint_sha256 =
        json.at("query_fingerprint_sha256").get<std::string>();
    digest.plan_hash = json.at("plan_hash").get<std::string>();
    digest.resource_signature = json.at("resource_signature").get<std::string>();
    digest.result_length_hash = json.at("result_length_hash").get<std::string>();
    digest.result_shape_hash = json.at("result_shape_hash").get<std::string>();
    digest.digest_hash = json.at("digest_hash").get<std::string>();

    // Validate
    if (!digest.isValid()) {
      throw std::runtime_error("Deserialized ExecutionDigest is invalid");
    }

    // Verify integrity
    if (!digest.verifyIntegrity()) {
      throw std::runtime_error(
          "ExecutionDigest integrity check failed - digest_hash does not "
          "match components");
    }

  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error(
        absl::StrCat("Failed to parse ExecutionDigest JSON-LD: ", e.what()));
  }

  return digest;
}

std::string ExecutionDigest::toString() const {
  std::ostringstream oss;
  oss << "ExecutionDigest {\n"
      << "  query_fingerprint_sha256: " << query_fingerprint_sha256.substr(0, 16)
      << "...\n"
      << "  plan_hash:                " << plan_hash.substr(0, 16) << "...\n"
      << "  resource_signature:       " << resource_signature.substr(0, 16)
      << "...\n"
      << "  result_length_hash:       " << result_length_hash.substr(0, 16)
      << "...\n"
      << "  result_shape_hash:        " << result_shape_hash.substr(0, 16)
      << "...\n"
      << "  digest_hash:              " << digest_hash << "\n"
      << "}";
  return oss.str();
}

// ============================================================================
// ComponentDiff Implementation
// ============================================================================

nlohmann::ordered_json ComponentDiff::toJson() const {
  nlohmann::ordered_json json;
  json["component"] = component_name;
  json["before"] = before_hash;
  json["after"] = after_hash;
  return json;
}

// ============================================================================
// EnvelopeDiff Implementation
// ============================================================================

DivergenceClassification EnvelopeDiff::classifyDivergence(
    bool fingerprint_diff, bool plan_diff, bool resource_diff, bool shape_diff,
    bool length_diff) {
  // Count differences
  int diff_count = (fingerprint_diff ? 1 : 0) + (plan_diff ? 1 : 0) +
                   (resource_diff ? 1 : 0) + (shape_diff ? 1 : 0) +
                   (length_diff ? 1 : 0);

  if (diff_count == 0) {
    return DivergenceClassification::IDENTICAL;
  }

  // Fingerprint mismatch is always the most severe
  if (fingerprint_diff) {
    return DivergenceClassification::QUERY_FINGERPRINT_MISMATCH;
  }

  // Multiple differences
  if (diff_count > 1) {
    return DivergenceClassification::MULTIPLE_DIVERGENCES;
  }

  // Single difference classification
  if (plan_diff) {
    return DivergenceClassification::PLAN_DIVERGENCE;
  }
  if (resource_diff) {
    return DivergenceClassification::RESOURCE_ENVELOPE_DIVERGENCE;
  }
  if (shape_diff) {
    return DivergenceClassification::RESULT_SHAPE_DIVERGENCE;
  }
  if (length_diff) {
    return DivergenceClassification::RESULT_LENGTH_DIVERGENCE;
  }

  // Should never reach here
  return DivergenceClassification::IDENTICAL;
}

EnvelopeDiff EnvelopeDiff::compute(const ExecutionDigest& digest1,
                                   const ExecutionDigest& digest2) {
  EnvelopeDiff diff;

  // Store digest hashes for reference
  diff.digest1_hash = digest1.digest_hash;
  diff.digest2_hash = digest2.digest_hash;

  // Quick check: are they identical?
  if (digest1.matches(digest2)) {
    diff.is_identical = true;
    diff.classification = DivergenceClassification::IDENTICAL;
    return diff;
  }

  // Compare each component
  diff.fingerprint_differs =
      (digest1.query_fingerprint_sha256 != digest2.query_fingerprint_sha256);
  diff.plan_changed = (digest1.plan_hash != digest2.plan_hash);
  diff.resource_envelope_changed =
      (digest1.resource_signature != digest2.resource_signature);
  diff.result_shape_changed =
      (digest1.result_shape_hash != digest2.result_shape_hash);
  diff.result_length_changed =
      (digest1.result_length_hash != digest2.result_length_hash);

  // Record detailed diffs
  if (diff.fingerprint_differs) {
    diff.differing_components.emplace_back(
        "query_fingerprint_sha256", digest1.query_fingerprint_sha256,
        digest2.query_fingerprint_sha256);
  }
  if (diff.plan_changed) {
    diff.differing_components.emplace_back("plan_hash", digest1.plan_hash,
                                           digest2.plan_hash);
  }
  if (diff.resource_envelope_changed) {
    diff.differing_components.emplace_back(
        "resource_signature", digest1.resource_signature,
        digest2.resource_signature);
  }
  if (diff.result_shape_changed) {
    diff.differing_components.emplace_back(
        "result_shape_hash", digest1.result_shape_hash,
        digest2.result_shape_hash);
  }
  if (diff.result_length_changed) {
    diff.differing_components.emplace_back(
        "result_length_hash", digest1.result_length_hash,
        digest2.result_length_hash);
  }

  // Classify the divergence
  diff.classification = classifyDivergence(
      diff.fingerprint_differs, diff.plan_changed,
      diff.resource_envelope_changed, diff.result_shape_changed,
      diff.result_length_changed);

  return diff;
}

bool EnvelopeDiff::hasComponentDiff(const std::string& component_name) const {
  return std::any_of(
      differing_components.begin(), differing_components.end(),
      [&](const ComponentDiff& d) { return d.component_name == component_name; });
}

std::optional<ComponentDiff> EnvelopeDiff::getComponentDiff(
    const std::string& component_name) const {
  auto it = std::find_if(
      differing_components.begin(), differing_components.end(),
      [&](const ComponentDiff& d) { return d.component_name == component_name; });
  if (it != differing_components.end()) {
    return *it;
  }
  return std::nullopt;
}

std::string EnvelopeDiff::classificationToString(DivergenceClassification c) {
  switch (c) {
    case DivergenceClassification::IDENTICAL:
      return "IDENTICAL";
    case DivergenceClassification::QUERY_FINGERPRINT_MISMATCH:
      return "QUERY_FINGERPRINT_MISMATCH";
    case DivergenceClassification::PLAN_DIVERGENCE:
      return "PLAN_DIVERGENCE";
    case DivergenceClassification::RESOURCE_ENVELOPE_DIVERGENCE:
      return "RESOURCE_ENVELOPE_DIVERGENCE";
    case DivergenceClassification::RESULT_SHAPE_DIVERGENCE:
      return "RESULT_SHAPE_DIVERGENCE";
    case DivergenceClassification::RESULT_LENGTH_DIVERGENCE:
      return "RESULT_LENGTH_DIVERGENCE";
    case DivergenceClassification::MULTIPLE_DIVERGENCES:
      return "MULTIPLE_DIVERGENCES";
  }
  return "UNKNOWN";
}

nlohmann::ordered_json EnvelopeDiff::toJsonLD() const {
  nlohmann::ordered_json json;

  // JSON-LD context
  json["@context"] = {
      {"@vocab", "https://qlever.cs.uni-freiburg.de/readplane/v1#"}};

  json["@type"] = "EnvelopeDiff";
  json["format_version"] = FORMAT_VERSION;

  // Summary flags
  json["is_identical"] = is_identical;
  json["classification"] = classificationToString(classification);

  // Digest references
  json["digest1_hash"] = digest1_hash;
  json["digest2_hash"] = digest2_hash;

  // Component flags
  json["fingerprint_differs"] = fingerprint_differs;
  json["plan_changed"] = plan_changed;
  json["resource_envelope_changed"] = resource_envelope_changed;
  json["result_shape_changed"] = result_shape_changed;
  json["result_length_changed"] = result_length_changed;

  // Detailed diffs
  nlohmann::ordered_json diffs = nlohmann::json::array();
  for (const auto& comp : differing_components) {
    diffs.push_back(comp.toJson());
  }
  json["differing_components"] = diffs;

  return json;
}

EnvelopeDiff EnvelopeDiff::fromJsonLD(const nlohmann::ordered_json& json) {
  EnvelopeDiff diff;

  try {
    // Verify type
    if (json.at("@type").get<std::string>() != "EnvelopeDiff") {
      throw std::runtime_error("Invalid @type: expected EnvelopeDiff");
    }

    // Extract summary flags
    diff.is_identical = json.at("is_identical").get<bool>();

    // Classification
    std::string class_str = json.at("classification").get<std::string>();
    if (class_str == "IDENTICAL") {
      diff.classification = DivergenceClassification::IDENTICAL;
    } else if (class_str == "QUERY_FINGERPRINT_MISMATCH") {
      diff.classification = DivergenceClassification::QUERY_FINGERPRINT_MISMATCH;
    } else if (class_str == "PLAN_DIVERGENCE") {
      diff.classification = DivergenceClassification::PLAN_DIVERGENCE;
    } else if (class_str == "RESOURCE_ENVELOPE_DIVERGENCE") {
      diff.classification =
          DivergenceClassification::RESOURCE_ENVELOPE_DIVERGENCE;
    } else if (class_str == "RESULT_SHAPE_DIVERGENCE") {
      diff.classification = DivergenceClassification::RESULT_SHAPE_DIVERGENCE;
    } else if (class_str == "RESULT_LENGTH_DIVERGENCE") {
      diff.classification = DivergenceClassification::RESULT_LENGTH_DIVERGENCE;
    } else if (class_str == "MULTIPLE_DIVERGENCES") {
      diff.classification = DivergenceClassification::MULTIPLE_DIVERGENCES;
    }

    // Digest references
    diff.digest1_hash = json.at("digest1_hash").get<std::string>();
    diff.digest2_hash = json.at("digest2_hash").get<std::string>();

    // Component flags
    diff.fingerprint_differs = json.at("fingerprint_differs").get<bool>();
    diff.plan_changed = json.at("plan_changed").get<bool>();
    diff.resource_envelope_changed =
        json.at("resource_envelope_changed").get<bool>();
    diff.result_shape_changed = json.at("result_shape_changed").get<bool>();
    diff.result_length_changed = json.at("result_length_changed").get<bool>();

    // Detailed diffs
    for (const auto& comp_json : json.at("differing_components")) {
      ComponentDiff comp;
      comp.component_name = comp_json.at("component").get<std::string>();
      comp.before_hash = comp_json.at("before").get<std::string>();
      comp.after_hash = comp_json.at("after").get<std::string>();
      diff.differing_components.push_back(comp);
    }

  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error(
        absl::StrCat("Failed to parse EnvelopeDiff JSON-LD: ", e.what()));
  }

  return diff;
}

std::string EnvelopeDiff::toString() const {
  std::ostringstream oss;
  oss << "EnvelopeDiff {\n"
      << "  is_identical: " << (is_identical ? "true" : "false") << "\n"
      << "  classification: " << classificationToString(classification) << "\n"
      << "  differences: " << differenceCount() << "\n";

  if (!is_identical) {
    oss << "  digest1_hash: " << digest1_hash.substr(0, 16) << "...\n"
        << "  digest2_hash: " << digest2_hash.substr(0, 16) << "...\n";

    for (const auto& comp : differing_components) {
      oss << "    " << comp.component_name << ":\n"
          << "      before: " << comp.before_hash.substr(0, 16) << "...\n"
          << "      after:  " << comp.after_hash.substr(0, 16) << "...\n";
    }
  }

  oss << "}";
  return oss.str();
}

}  // namespace readPlane

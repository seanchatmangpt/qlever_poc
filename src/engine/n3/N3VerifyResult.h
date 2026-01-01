// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Verification Result with JSON Serialization
// Provides standardized result format for N3 document verification

#ifndef QLEVER_SRC_ENGINE_N3_N3VERIFYRESULT_H
#define QLEVER_SRC_ENGINE_N3_N3VERIFYRESULT_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "engine/n3/N3ErrorCode.h"

namespace ad_engine::n3 {

// Statistics about the verified N3 document
struct N3VerificationStats {
  size_t input_size_bytes = 0;
  size_t blank_nodes_count = 0;
  size_t nesting_depth_max = 0;
  uint64_t runtime_ms = 0;
  size_t total_lines = 0;
  size_t triple_count_estimate = 0;

  // Convert to JSON
  nlohmann::json toJson() const {
    return nlohmann::json{{"input_size_bytes", input_size_bytes},
                          {"blank_nodes_count", blank_nodes_count},
                          {"nesting_depth_max", nesting_depth_max},
                          {"runtime_ms", runtime_ms},
                          {"total_lines", total_lines},
                          {"triple_count_estimate", triple_count_estimate}};
  }

  // Create from JSON
  static N3VerificationStats fromJson(const nlohmann::json& j) {
    N3VerificationStats stats;
    stats.input_size_bytes = j.value("input_size_bytes", 0UL);
    stats.blank_nodes_count = j.value("blank_nodes_count", 0UL);
    stats.nesting_depth_max = j.value("nesting_depth_max", 0UL);
    stats.runtime_ms = j.value("runtime_ms", 0UL);
    stats.total_lines = j.value("total_lines", 0UL);
    stats.triple_count_estimate = j.value("triple_count_estimate", 0UL);
    return stats;
  }
};

// Result of N3 document verification
struct N3VerifyResult {
  // Verification success status
  bool ok = false;

  // List of errors (empty if ok = true)
  std::vector<ErrorRecord> errors;

  // List of warnings (non-fatal issues)
  std::vector<WarningRecord> warnings;

  // SHA-256 hash of normalized document for reproducibility
  std::string compliance_digest;

  // Verification statistics
  N3VerificationStats stats;

  // Input filename (for reporting)
  std::string filename;

  // Constructor
  N3VerifyResult() = default;

  // Create successful result
  static N3VerifyResult success(const std::string& digest,
                                 const N3VerificationStats& s,
                                 const std::string& fname = "") {
    N3VerifyResult result;
    result.ok = true;
    result.compliance_digest = digest;
    result.stats = s;
    result.filename = fname;
    return result;
  }

  // Create error result
  static N3VerifyResult error(const std::vector<ErrorRecord>& errs,
                              const N3VerificationStats& s,
                              const std::string& fname = "") {
    N3VerifyResult result;
    result.ok = false;
    result.errors = errs;
    result.stats = s;
    result.filename = fname;
    return result;
  }

  // Add error to result
  void addError(const ErrorRecord& error) {
    errors.push_back(error);
    ok = false;
  }

  // Add warning to result
  void addWarning(const WarningRecord& warning) {
    warnings.push_back(warning);
  }

  // Convert to JSON
  nlohmann::json toJson() const {
    nlohmann::json j;
    j["ok"] = ok;
    j["compliance_digest"] = compliance_digest;
    j["filename"] = filename;

    // Errors
    nlohmann::json errors_json = nlohmann::json::array();
    for (const auto& err : errors) {
      nlohmann::json err_json;
      err_json["code"] = std::string(errorCodeToString(err.code));
      err_json["code_int"] = static_cast<int>(err.code);
      err_json["message"] = err.message;
      err_json["line_number"] = err.line_number;
      err_json["line_content"] = err.line_content;
      errors_json.push_back(err_json);
    }
    j["errors"] = errors_json;

    // Warnings
    nlohmann::json warnings_json = nlohmann::json::array();
    for (const auto& warn : warnings) {
      nlohmann::json warn_json;
      warn_json["message"] = warn.message;
      warn_json["line_number"] = warn.line_number;
      warn_json["line_content"] = warn.line_content;
      warnings_json.push_back(warn_json);
    }
    j["warnings"] = warnings_json;

    // Stats
    j["stats"] = stats.toJson();

    return j;
  }

  // Create from JSON
  static N3VerifyResult fromJson(const nlohmann::json& j) {
    N3VerifyResult result;
    result.ok = j.value("ok", false);
    result.compliance_digest = j.value("compliance_digest", std::string{});
    result.filename = j.value("filename", std::string{});

    // Errors
    if (j.contains("errors")) {
      for (const auto& err_json : j["errors"]) {
        ErrorRecord err(
            static_cast<N3ErrorCode>(err_json.value("code_int", 999)),
            err_json.value("message", std::string{}),
            err_json.value("line_number", 0UL),
            err_json.value("line_content", std::string{}));
        result.errors.push_back(err);
      }
    }

    // Warnings
    if (j.contains("warnings")) {
      for (const auto& warn_json : j["warnings"]) {
        WarningRecord warn(warn_json.value("message", std::string{}),
                           warn_json.value("line_number", 0UL),
                           warn_json.value("line_content", std::string{}));
        result.warnings.push_back(warn);
      }
    }

    // Stats
    if (j.contains("stats")) {
      result.stats = N3VerificationStats::fromJson(j["stats"]);
    }

    return result;
  }

  // Human-readable summary
  std::string summary() const {
    if (ok) {
      return "N3 verification PASSED (" + std::to_string(stats.total_lines) +
             " lines)";
    } else {
      return "N3 verification FAILED with " + std::to_string(errors.size()) +
             " error(s)";
    }
  }
};

}  // namespace ad_engine::n3

#endif  // QLEVER_SRC_ENGINE_N3_N3VERIFYRESULT_H

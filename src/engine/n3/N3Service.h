// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Verification Service
// Wrapper over existing N3 compliance verifier with standardized
// error codes, guards, and machine-readable output

#ifndef QLEVER_SRC_ENGINE_N3_N3SERVICE_H
#define QLEVER_SRC_ENGINE_N3_N3SERVICE_H

#include <memory>
#include <string>

#include "engine/n3/N3Config.h"
#include "engine/n3/N3VerifyResult.h"

namespace ad_engine::n3 {

// Main N3 verification service
// Wraps existing N3 parser/verifier with standardized interface
// NOT full N3 rules evaluation - just parse/verify
class N3Service {
 public:
  // Construct service with configuration
  explicit N3Service(N3Config config = N3Config::defaultConfig());

  // Verify N3 document from file
  // Returns verification result with errors, warnings, and digest
  N3VerifyResult verifyFile(const std::string& filename) const;

  // Verify N3 document from string content
  // Returns verification result with errors, warnings, and digest
  N3VerifyResult verifyContent(const std::string& content,
                               const std::string& filename = "<string>") const;

  // Get current configuration
  const N3Config& config() const { return config_; }

  // Update configuration
  void setConfig(const N3Config& config) { config_ = config; }

 private:
  // Configuration with guards
  N3Config config_;

  // Verify document with guards enforced
  N3VerifyResult verifyImpl(const std::string& content,
                            const std::string& filename) const;

  // Check input size guard
  bool checkInputSize(const std::string& content, N3VerifyResult& result) const;

  // Check blank node limit guard
  bool checkBlankNodeLimit(const std::string& content,
                           N3VerifyResult& result) const;

  // Check nesting depth guard
  bool checkNestingDepth(const std::string& content,
                         N3VerifyResult& result) const;

  // Calculate SHA-256 digest of normalized content
  std::string calculateDigest(const std::string& content) const;

  // Normalize content for digest calculation
  std::string normalizeContent(const std::string& content) const;

  // Convert N3ComplianceIssue to ErrorRecord/WarningRecord
  void processIssues(const std::vector<ad_utility::N3ComplianceIssue>& issues,
                     N3VerifyResult& result) const;

  // Map feature to error code
  N3ErrorCode mapFeatureToErrorCode(const std::string& feature) const;
};

}  // namespace ad_engine::n3

#endif  // QLEVER_SRC_ENGINE_N3_N3SERVICE_H

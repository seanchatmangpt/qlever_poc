// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include "engine/n3/N3Service.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

#include "util/Exception.h"
#include "util/N3ComplianceVerifier.h"

namespace ad_engine::n3 {

// ____________________________________________________________________________
N3Service::N3Service(N3Config config) : config_(std::move(config)) {
  AD_CONTRACT_CHECK(config_.isValid(), "Invalid N3Config");
}

// ____________________________________________________________________________
N3VerifyResult N3Service::verifyFile(const std::string& filename) const {
  // Check file exists and read content
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    N3VerifyResult result;
    result.ok = false;
    result.filename = filename;
    result.addError(ErrorRecord(N3ErrorCode::FILE_NOT_FOUND,
                                 "File not found: " + filename));
    return result;
  }

  // Get file size
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Check input size guard before reading
  if (static_cast<size_t>(size) > config_.max_input_size_bytes) {
    N3VerifyResult result;
    result.ok = false;
    result.filename = filename;
    result.addError(ErrorRecord(
        N3ErrorCode::INPUT_SIZE_EXCEEDED,
        "Input file size (" + std::to_string(size) + " bytes) exceeds limit (" +
            std::to_string(config_.max_input_size_bytes) + " bytes)"));
    result.stats.input_size_bytes = size;
    return result;
  }

  // Read file content
  std::string content(size, '\0');
  if (!file.read(&content[0], size)) {
    N3VerifyResult result;
    result.ok = false;
    result.filename = filename;
    result.addError(
        ErrorRecord(N3ErrorCode::IO_ERROR, "Failed to read file: " + filename));
    return result;
  }

  return verifyContent(content, filename);
}

// ____________________________________________________________________________
N3VerifyResult N3Service::verifyContent(const std::string& content,
                                        const std::string& filename) const {
  return verifyImpl(content, filename);
}

// ____________________________________________________________________________
N3VerifyResult N3Service::verifyImpl(const std::string& content,
                                     const std::string& filename) const {
  auto start_time = std::chrono::steady_clock::now();

  N3VerifyResult result;
  result.filename = filename;
  result.stats.input_size_bytes = content.size();

  // Check input size guard
  if (!checkInputSize(content, result)) {
    return result;
  }

  // Check blank node limit guard
  if (!checkBlankNodeLimit(content, result)) {
    return result;
  }

  // Check nesting depth guard
  if (!checkNestingDepth(content, result)) {
    return result;
  }

  // Use existing N3ComplianceVerifier by writing content to temp file
  // (The existing verifier only works with files)
  std::string temp_filename = "/tmp/n3_verify_" + std::to_string(
                                                       std::chrono::system_clock::now()
                                                           .time_since_epoch()
                                                           .count()) +
                              ".n3";
  std::ofstream temp_file(temp_filename);
  if (!temp_file.is_open()) {
    result.addError(ErrorRecord(N3ErrorCode::IO_ERROR,
                                 "Failed to create temporary file"));
    return result;
  }
  temp_file << content;
  temp_file.close();

  // Run existing verifier
  ad_utility::N3ComplianceVerifier verifier;
  verifier.analyzeFile(temp_filename);

  // Remove temp file
  std::remove(temp_filename.c_str());

  // Process results
  result.stats.total_lines = verifier.getTotalLines();
  result.ok = verifier.isCompatible();

  // Process issues from verifier
  processIssues(verifier.getIssues(), result);

  // Calculate compliance digest
  result.compliance_digest = calculateDigest(normalizeContent(content));

  // Calculate runtime
  auto end_time = std::chrono::steady_clock::now();
  result.stats.runtime_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                             start_time)
          .count();

  return result;
}

// ____________________________________________________________________________
bool N3Service::checkInputSize(const std::string& content,
                               N3VerifyResult& result) const {
  if (content.size() > config_.max_input_size_bytes) {
    result.ok = false;
    result.addError(ErrorRecord(
        N3ErrorCode::INPUT_SIZE_EXCEEDED,
        "Input size (" + std::to_string(content.size()) +
            " bytes) exceeds limit (" +
            std::to_string(config_.max_input_size_bytes) + " bytes)"));
    return false;
  }
  return true;
}

// ____________________________________________________________________________
bool N3Service::checkBlankNodeLimit(const std::string& content,
                                    N3VerifyResult& result) const {
  // Count blank nodes in content
  // Blank nodes: [] or _:identifier
  std::regex blank_node_pattern(R"(\[\s*\]|_:[a-zA-Z0-9]+)");
  auto blank_nodes_begin =
      std::sregex_iterator(content.begin(), content.end(), blank_node_pattern);
  auto blank_nodes_end = std::sregex_iterator();
  size_t blank_node_count =
      std::distance(blank_nodes_begin, blank_nodes_end);

  result.stats.blank_nodes_count = blank_node_count;

  if (blank_node_count > config_.max_blank_nodes) {
    result.ok = false;
    result.addError(ErrorRecord(
        N3ErrorCode::BLANK_NODE_LIMIT_EXCEEDED,
        "Blank node count (" + std::to_string(blank_node_count) +
            ") exceeds limit (" + std::to_string(config_.max_blank_nodes) +
            ")"));
    return false;
  }
  return true;
}

// ____________________________________________________________________________
bool N3Service::checkNestingDepth(const std::string& content,
                                  N3VerifyResult& result) const {
  // Check nesting depth by counting nested structures
  // Simple heuristic: count max depth of ( ) [ ] { } nesting
  size_t max_depth = 0;
  size_t current_depth = 0;

  for (char c : content) {
    if (c == '(' || c == '[' || c == '{') {
      current_depth++;
      max_depth = std::max(max_depth, current_depth);
    } else if (c == ')' || c == ']' || c == '}') {
      if (current_depth > 0) {
        current_depth--;
      }
    }
  }

  result.stats.nesting_depth_max = max_depth;

  if (max_depth > config_.max_nesting_depth) {
    result.ok = false;
    result.addError(ErrorRecord(
        N3ErrorCode::NESTING_DEPTH_EXCEEDED,
        "Nesting depth (" + std::to_string(max_depth) + ") exceeds limit (" +
            std::to_string(config_.max_nesting_depth) + ")"));
    return false;
  }
  return true;
}

// ____________________________________________________________________________
std::string N3Service::calculateDigest(const std::string& content) const {
  // Calculate SHA-256 hash
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(content.c_str()),
         content.size(), hash);

  // Convert to hex string
  std::ostringstream hex_stream;
  hex_stream << std::hex << std::setfill('0');
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    hex_stream << std::setw(2) << static_cast<int>(hash[i]);
  }
  return hex_stream.str();
}

// ____________________________________________________________________________
std::string N3Service::normalizeContent(const std::string& content) const {
  // Normalize content for digest calculation
  // - Remove comments
  // - Normalize whitespace
  // - Sort prefixes (for reproducibility)

  std::string normalized;
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line)) {
    // Remove comments
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }

    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    // Skip empty lines
    if (line.empty()) {
      continue;
    }

    normalized += line + "\n";
  }

  return normalized;
}

// ____________________________________________________________________________
void N3Service::processIssues(
    const std::vector<ad_utility::N3ComplianceIssue>& issues,
    N3VerifyResult& result) const {
  for (const auto& issue : issues) {
    if (issue.isSupported) {
      // Supported features generate warnings (if any)
      // Currently the existing verifier doesn't report supported features as
      // issues
      continue;
    } else {
      // Unsupported features generate errors
      N3ErrorCode code = mapFeatureToErrorCode(issue.feature);
      result.addError(ErrorRecord(code, issue.description, issue.lineNumber,
                                   issue.lineContent));
    }
  }
}

// ____________________________________________________________________________
N3ErrorCode N3Service::mapFeatureToErrorCode(const std::string& feature) const {
  // Map feature names to error codes
  if (feature == "Formulae/Quoted Graphs" || feature == "formulae") {
    return N3ErrorCode::FORMULAE_NOT_SUPPORTED;
  } else if (feature == "Implication Rules" || feature == "implication") {
    return N3ErrorCode::IMPLICATION_NOT_SUPPORTED;
  } else if (feature == "Quantifiers" || feature == "quantifier") {
    return N3ErrorCode::QUANTIFIER_NOT_SUPPORTED;
  } else if (feature == "Variables" || feature == "variable") {
    return N3ErrorCode::VARIABLE_NOT_SUPPORTED;
  } else if (feature == "Built-in Functions" || feature == "builtin_function") {
    return N3ErrorCode::BUILTIN_FUNCTION_NOT_SUPPORTED;
  } else if (feature == "N3 Paths" || feature == "n3_path") {
    return N3ErrorCode::N3_PATH_NOT_SUPPORTED;
  } else if (config_.strict_mode) {
    return N3ErrorCode::STRICT_MODE_VIOLATION;
  } else {
    return N3ErrorCode::UNKNOWN_FEATURE;
  }
}

}  // namespace ad_engine::n3

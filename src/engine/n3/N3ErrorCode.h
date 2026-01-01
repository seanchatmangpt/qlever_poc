// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Verification Error Code Taxonomy
// Provides standardized error codes for N3 document verification

#ifndef QLEVER_SRC_ENGINE_N3_N3ERRORCODE_H
#define QLEVER_SRC_ENGINE_N3_N3ERRORCODE_H

#include <string>
#include <string_view>

namespace ad_engine::n3 {

// N3-specific error codes for verification failures
// Each error code has a stable string representation for reproducibility
enum class N3ErrorCode {
  // Parsing errors (1xx)
  PARSE_FAILED = 100,
  INVALID_SYNTAX = 101,
  INVALID_IRI = 102,
  INVALID_LITERAL = 103,
  INVALID_PREFIX = 104,
  INVALID_BASE = 105,

  // Unsupported N3 features (2xx)
  FORMULAE_NOT_SUPPORTED = 200,
  IMPLICATION_NOT_SUPPORTED = 201,
  QUANTIFIER_NOT_SUPPORTED = 202,
  VARIABLE_NOT_SUPPORTED = 203,
  BUILTIN_FUNCTION_NOT_SUPPORTED = 204,
  N3_PATH_NOT_SUPPORTED = 205,
  RULES_NOT_SUPPORTED = 206,

  // Guard violations (3xx)
  INPUT_SIZE_EXCEEDED = 300,
  BLANK_NODE_LIMIT_EXCEEDED = 301,
  NESTING_DEPTH_EXCEEDED = 302,
  MEMORY_LIMIT_EXCEEDED = 303,

  // Unknown/strict mode errors (4xx)
  UNKNOWN_FEATURE = 400,
  STRICT_MODE_VIOLATION = 401,

  // General errors (9xx)
  FILE_NOT_FOUND = 900,
  IO_ERROR = 901,
  INTERNAL_ERROR = 999
};

// Get stable string representation of error code
constexpr std::string_view errorCodeToString(N3ErrorCode code) {
  switch (code) {
    case N3ErrorCode::PARSE_FAILED:
      return "PARSE_FAILED";
    case N3ErrorCode::INVALID_SYNTAX:
      return "INVALID_SYNTAX";
    case N3ErrorCode::INVALID_IRI:
      return "INVALID_IRI";
    case N3ErrorCode::INVALID_LITERAL:
      return "INVALID_LITERAL";
    case N3ErrorCode::INVALID_PREFIX:
      return "INVALID_PREFIX";
    case N3ErrorCode::INVALID_BASE:
      return "INVALID_BASE";
    case N3ErrorCode::FORMULAE_NOT_SUPPORTED:
      return "FORMULAE_NOT_SUPPORTED";
    case N3ErrorCode::IMPLICATION_NOT_SUPPORTED:
      return "IMPLICATION_NOT_SUPPORTED";
    case N3ErrorCode::QUANTIFIER_NOT_SUPPORTED:
      return "QUANTIFIER_NOT_SUPPORTED";
    case N3ErrorCode::VARIABLE_NOT_SUPPORTED:
      return "VARIABLE_NOT_SUPPORTED";
    case N3ErrorCode::BUILTIN_FUNCTION_NOT_SUPPORTED:
      return "BUILTIN_FUNCTION_NOT_SUPPORTED";
    case N3ErrorCode::N3_PATH_NOT_SUPPORTED:
      return "N3_PATH_NOT_SUPPORTED";
    case N3ErrorCode::RULES_NOT_SUPPORTED:
      return "RULES_NOT_SUPPORTED";
    case N3ErrorCode::INPUT_SIZE_EXCEEDED:
      return "INPUT_SIZE_EXCEEDED";
    case N3ErrorCode::BLANK_NODE_LIMIT_EXCEEDED:
      return "BLANK_NODE_LIMIT_EXCEEDED";
    case N3ErrorCode::NESTING_DEPTH_EXCEEDED:
      return "NESTING_DEPTH_EXCEEDED";
    case N3ErrorCode::MEMORY_LIMIT_EXCEEDED:
      return "MEMORY_LIMIT_EXCEEDED";
    case N3ErrorCode::UNKNOWN_FEATURE:
      return "UNKNOWN_FEATURE";
    case N3ErrorCode::STRICT_MODE_VIOLATION:
      return "STRICT_MODE_VIOLATION";
    case N3ErrorCode::FILE_NOT_FOUND:
      return "FILE_NOT_FOUND";
    case N3ErrorCode::IO_ERROR:
      return "IO_ERROR";
    case N3ErrorCode::INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    default:
      return "UNKNOWN_ERROR";
  }
}

// Get human-readable description of error code
constexpr std::string_view errorCodeDescription(N3ErrorCode code) {
  switch (code) {
    case N3ErrorCode::PARSE_FAILED:
      return "Failed to parse N3 document";
    case N3ErrorCode::INVALID_SYNTAX:
      return "Invalid N3 syntax detected";
    case N3ErrorCode::INVALID_IRI:
      return "Invalid IRI format";
    case N3ErrorCode::INVALID_LITERAL:
      return "Invalid literal format";
    case N3ErrorCode::INVALID_PREFIX:
      return "Invalid prefix declaration";
    case N3ErrorCode::INVALID_BASE:
      return "Invalid base declaration";
    case N3ErrorCode::FORMULAE_NOT_SUPPORTED:
      return "N3 formulae (quoted graphs) are not supported";
    case N3ErrorCode::IMPLICATION_NOT_SUPPORTED:
      return "N3 implication rules (=>, <=) are not supported";
    case N3ErrorCode::QUANTIFIER_NOT_SUPPORTED:
      return "N3 quantifiers (@forAll, @forSome) are not supported";
    case N3ErrorCode::VARIABLE_NOT_SUPPORTED:
      return "N3 variables (?var) are not supported";
    case N3ErrorCode::BUILTIN_FUNCTION_NOT_SUPPORTED:
      return "N3 built-in functions (log:, math:, etc.) are not supported";
    case N3ErrorCode::N3_PATH_NOT_SUPPORTED:
      return "N3 path expressions (!,:^) are not supported";
    case N3ErrorCode::RULES_NOT_SUPPORTED:
      return "N3 rules and logic predicates are not supported";
    case N3ErrorCode::INPUT_SIZE_EXCEEDED:
      return "Input document exceeds maximum allowed size";
    case N3ErrorCode::BLANK_NODE_LIMIT_EXCEEDED:
      return "Document exceeds maximum allowed blank nodes";
    case N3ErrorCode::NESTING_DEPTH_EXCEEDED:
      return "Document exceeds maximum allowed nesting depth";
    case N3ErrorCode::MEMORY_LIMIT_EXCEEDED:
      return "Document processing exceeds memory limit";
    case N3ErrorCode::UNKNOWN_FEATURE:
      return "Unknown or unrecognized N3 feature";
    case N3ErrorCode::STRICT_MODE_VIOLATION:
      return "Feature not allowed in strict mode";
    case N3ErrorCode::FILE_NOT_FOUND:
      return "Input file not found";
    case N3ErrorCode::IO_ERROR:
      return "I/O error reading input file";
    case N3ErrorCode::INTERNAL_ERROR:
      return "Internal verification error";
    default:
      return "Unknown error";
  }
}

// Error record for detailed error reporting
struct ErrorRecord {
  N3ErrorCode code;
  std::string message;
  size_t line_number;
  std::string line_content;

  ErrorRecord(N3ErrorCode c, std::string msg, size_t line = 0,
              std::string content = "")
      : code(c),
        message(std::move(msg)),
        line_number(line),
        line_content(std::move(content)) {}
};

// Warning record for non-fatal issues
struct WarningRecord {
  std::string message;
  size_t line_number;
  std::string line_content;

  WarningRecord(std::string msg, size_t line = 0, std::string content = "")
      : message(std::move(msg)),
        line_number(line),
        line_content(std::move(content)) {}
};

}  // namespace ad_engine::n3

#endif  // QLEVER_SRC_ENGINE_N3_N3ERRORCODE_H

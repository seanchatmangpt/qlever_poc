// EPIC 7: Error Code Infrastructure
// All ingress errors reported via codes, not exceptions
// This file implements the complete IngressErrorCode enum

#ifndef QLEVER_ENGINE_INGRESS_ERROR_CODES_H
#define QLEVER_ENGINE_INGRESS_ERROR_CODES_H

#include <cstdint>
#include <string_view>

namespace qlever::ingress {

enum class IngressErrorCode : uint16_t {
  // SUCCESS
  OK = 0,

  // PARSING ERRORS (1-99)
  PARSE_ERROR_SYNTAX = 1,           // JSON syntax invalid
  PARSE_ERROR_UTF8 = 2,              // UTF-8 encoding error
  PARSE_ERROR_NESTED_DEPTH = 3,      // Nesting exceeds max depth
  PARSE_ERROR_ARRAY_EXPECTED = 4,    // Expected array, got scalar
  PARSE_ERROR_OBJECT_EXPECTED = 5,   // Expected object, got array
  PARSE_ERROR_DUPLICATE_KEY = 6,     // Duplicate key in object
  PARSE_ERROR_TRAILING_CHARS = 7,    // Unexpected trailing characters
  PARSE_ERROR_UNESCAPED_CONTROL = 8, // Unescaped control character
  PARSE_ERROR_INVALID_ESCAPE = 9,    // Invalid escape sequence
  PARSE_ERROR_EMPTY_INPUT = 10,      // Empty input stream

  // JSON-LD VALIDATION (100-199)
  JSONLD_MISSING_CONTEXT = 100,      // No @context field
  JSONLD_INVALID_CONTEXT = 101,      // @context value not valid
  JSONLD_MISSING_ID = 102,           // @id field missing (required)
  JSONLD_INVALID_ID = 103,           // @id not valid IRI
  JSONLD_UNKNOWN_TYPE = 104,         // @type not recognized
  JSONLD_INVALID_VALUE = 105,        // Value type mismatch
  JSONLD_EXPANSION_FAILED = 106,     // Context expansion failed
  JSONLD_COMPACTION_FAILED = 107,    // Compaction failed
  JSONLD_FLATTENING_FAILED = 108,    // Flattening failed
  JSONLD_FRAMING_FAILED = 109,       // Framing failed

  // STRUCTURAL VALIDATION (200-299)
  VALIDATION_FAILED = 200,           // Generic validation failure
  VALIDATION_TYPE_MISMATCH = 201,    // Field type mismatch
  VALIDATION_CONSTRAINT_VIOLATED = 202, // Constraint violated
  VALIDATION_SHAPE_VIOLATION = 203,  // SHACL shape violation
  VALIDATION_RANGE_ERROR = 204,      // Value out of range
  VALIDATION_LENGTH_ERROR = 205,     // String length violation
  VALIDATION_REGEX_MISMATCH = 206,   // Regex pattern mismatch
  VALIDATION_FORMAT_ERROR = 207,     // Format validation failed
  VALIDATION_ENUM_INVALID = 208,     // Not valid enum value
  VALIDATION_REFERENCE_ERROR = 209,  // Reference resolution failed

  // RESOURCE ERRORS (300-399)
  MEMORY_ALLOCATION_FAILED = 300,    // malloc/new failed
  BUFFER_OVERFLOW = 301,             // Input exceeds max size
  TIMEOUT = 302,                     // Parsing exceeded time limit
  IO_ERROR = 303,                    // I/O error during read
  FILE_NOT_FOUND = 304,              // Input file not found
  PERMISSION_DENIED = 305,           // Permission denied
  DISK_FULL = 306,                   // No space on device
  RESOURCE_EXHAUSTED = 307,          // Resource limit exceeded

  // IMPLEMENTATION ERRORS (400-499)
  UNSUPPORTED_FORMAT = 400,          // Format not supported
  UNSUPPORTED_ENCODING = 401,        // Character encoding not supported
  UNSUPPORTED_VERSION = 402,         // Version not supported
  UNIMPLEMENTED = 403,               // Operation not implemented
  NOT_INITIALIZED = 404,             // Component not initialized
  ALREADY_INITIALIZED = 405,         // Component already initialized
  INVALID_ARGUMENT = 406,            // Invalid argument provided
  INVALID_STATE = 407,               // Invalid internal state
  INTERNAL_ERROR = 500,              // Unexpected internal error

  // DIGEST/DETERMINISM ERRORS (600-699)
  DIGEST_COMPUTE_FAILED = 600,       // SHA256 computation failed
  DIGEST_MISMATCH = 601,             // Digest does not match expected
  NORMALIZATION_FAILED = 602,        // Canonicalization failed
};

// Error code to human-readable description (cold-path only)
constexpr std::string_view error_description(IngressErrorCode code) noexcept {
  switch (code) {
    case IngressErrorCode::OK: return "OK";
    
    // Parsing errors
    case IngressErrorCode::PARSE_ERROR_SYNTAX: return "JSON syntax error";
    case IngressErrorCode::PARSE_ERROR_UTF8: return "UTF-8 encoding error";
    case IngressErrorCode::PARSE_ERROR_NESTED_DEPTH: return "Nesting exceeds max depth";
    case IngressErrorCode::PARSE_ERROR_ARRAY_EXPECTED: return "Expected array, got scalar";
    case IngressErrorCode::PARSE_ERROR_OBJECT_EXPECTED: return "Expected object, got array";
    case IngressErrorCode::PARSE_ERROR_DUPLICATE_KEY: return "Duplicate key in object";
    case IngressErrorCode::PARSE_ERROR_TRAILING_CHARS: return "Unexpected trailing characters";
    case IngressErrorCode::PARSE_ERROR_UNESCAPED_CONTROL: return "Unescaped control character";
    case IngressErrorCode::PARSE_ERROR_INVALID_ESCAPE: return "Invalid escape sequence";
    case IngressErrorCode::PARSE_ERROR_EMPTY_INPUT: return "Empty input stream";
    
    // JSON-LD errors
    case IngressErrorCode::JSONLD_MISSING_CONTEXT: return "Missing @context field";
    case IngressErrorCode::JSONLD_INVALID_CONTEXT: return "Invalid @context value";
    case IngressErrorCode::JSONLD_MISSING_ID: return "Missing @id field";
    case IngressErrorCode::JSONLD_INVALID_ID: return "Invalid @id IRI";
    case IngressErrorCode::JSONLD_UNKNOWN_TYPE: return "Unknown @type";
    case IngressErrorCode::JSONLD_INVALID_VALUE: return "Value type mismatch";
    case IngressErrorCode::JSONLD_EXPANSION_FAILED: return "Context expansion failed";
    case IngressErrorCode::JSONLD_COMPACTION_FAILED: return "Compaction failed";
    case IngressErrorCode::JSONLD_FLATTENING_FAILED: return "Flattening failed";
    case IngressErrorCode::JSONLD_FRAMING_FAILED: return "Framing failed";
    
    // Validation errors
    case IngressErrorCode::VALIDATION_FAILED: return "Validation failed";
    case IngressErrorCode::VALIDATION_TYPE_MISMATCH: return "Type mismatch";
    case IngressErrorCode::VALIDATION_CONSTRAINT_VIOLATED: return "Constraint violated";
    case IngressErrorCode::VALIDATION_SHAPE_VIOLATION: return "SHACL shape violation";
    case IngressErrorCode::VALIDATION_RANGE_ERROR: return "Value out of range";
    case IngressErrorCode::VALIDATION_LENGTH_ERROR: return "String length violation";
    case IngressErrorCode::VALIDATION_REGEX_MISMATCH: return "Regex pattern mismatch";
    case IngressErrorCode::VALIDATION_FORMAT_ERROR: return "Format validation failed";
    case IngressErrorCode::VALIDATION_ENUM_INVALID: return "Invalid enum value";
    case IngressErrorCode::VALIDATION_REFERENCE_ERROR: return "Reference resolution failed";
    
    // Resource errors
    case IngressErrorCode::MEMORY_ALLOCATION_FAILED: return "Memory allocation failed";
    case IngressErrorCode::BUFFER_OVERFLOW: return "Input exceeds max size";
    case IngressErrorCode::TIMEOUT: return "Operation timed out";
    case IngressErrorCode::IO_ERROR: return "I/O error";
    case IngressErrorCode::FILE_NOT_FOUND: return "File not found";
    case IngressErrorCode::PERMISSION_DENIED: return "Permission denied";
    case IngressErrorCode::DISK_FULL: return "Disk full";
    case IngressErrorCode::RESOURCE_EXHAUSTED: return "Resource exhausted";
    
    // Implementation errors
    case IngressErrorCode::UNSUPPORTED_FORMAT: return "Unsupported format";
    case IngressErrorCode::UNSUPPORTED_ENCODING: return "Unsupported encoding";
    case IngressErrorCode::UNSUPPORTED_VERSION: return "Unsupported version";
    case IngressErrorCode::UNIMPLEMENTED: return "Not implemented";
    case IngressErrorCode::NOT_INITIALIZED: return "Not initialized";
    case IngressErrorCode::ALREADY_INITIALIZED: return "Already initialized";
    case IngressErrorCode::INVALID_ARGUMENT: return "Invalid argument";
    case IngressErrorCode::INVALID_STATE: return "Invalid state";
    case IngressErrorCode::INTERNAL_ERROR: return "Internal error";
    
    // Digest errors
    case IngressErrorCode::DIGEST_COMPUTE_FAILED: return "Digest computation failed";
    case IngressErrorCode::DIGEST_MISMATCH: return "Digest mismatch";
    case IngressErrorCode::NORMALIZATION_FAILED: return "Normalization failed";
    
    default: return "UNKNOWN_ERROR";
  }
}

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_ERROR_CODES_H

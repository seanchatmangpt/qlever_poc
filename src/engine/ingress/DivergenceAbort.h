// EPIC 10.2: FMEA Abort Logic - Fail-Closed Error Handling
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 7 - FMEA Abort Logic
//
// Centralized divergence abort handler - replaces best-effort recovery
// with deterministic fail-closed shutdown.

#ifndef QLEVER_ENGINE_INGRESS_DIVERGENCE_ABORT_H
#define QLEVER_ENGINE_INGRESS_DIVERGENCE_ABORT_H

#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace qlever::ingress {

// ============================================================================
// ABORT CATEGORIES: Which errors trigger immediate abort
// ============================================================================

enum class AbortCategory : uint8_t {
  // CRITICAL: Data integrity violations (MUST abort)
  HASH_MISMATCH = 1,        // Digest verification failed
  CORRUPT_INDEX_DATA = 2,   // Index data corruption detected
  EPOCH_VIOLATION = 3,      // Epoch guard breach

  // CRITICAL: Resource exhaustion (MUST abort)
  OUT_OF_MEMORY = 10,       // Memory allocation failed
  RESOURCE_LIMIT_EXCEEDED = 11,  // Resource guard breach

  // CRITICAL: Missing critical files (MUST abort)
  MISSING_CRITICAL_FILE = 20,   // Required file not found
  IO_CORRUPTION = 21,           // I/O error indicating corruption

  // CRITICAL: Guard breaches (MUST abort)
  GUARD_BREACH = 30,        // Any guard violation
  INVARIANT_VIOLATION = 31, // Structural invariant violated

  // INTERNAL: Unexpected states (MUST abort)
  INTERNAL_ERROR = 100,     // Unexpected internal error
};

// ============================================================================
// ABORT CONTEXT: Diagnostic information for forensics
// ============================================================================

struct AbortContext {
  AbortCategory category;
  const char* file;          // Source file location
  int line;                  // Source line number
  const char* function;      // Function name
  std::string_view message;  // Human-readable error message

  // Optional: specific error details
  uint64_t expected_hash = 0;  // For hash mismatches
  uint64_t actual_hash = 0;    // For hash mismatches
  size_t resource_requested = 0;  // For OOM
  size_t resource_available = 0;  // For OOM
};

// ============================================================================
// DIVERGENCE ABORT: Centralized fail-closed shutdown
// ============================================================================

// Primary abort function - NEVER returns
// Logs error to stderr, flushes caches, closes files, exits with code 42
[[noreturn]] void DivergenceAbort(const AbortContext& context) noexcept;

// ============================================================================
// CONVENIENCE MACROS: Abort with source location
// ============================================================================

#define DIVERGENCE_ABORT(category, message) \
  ::qlever::ingress::DivergenceAbort( \
      {category, __FILE__, __LINE__, __func__, message})

#define DIVERGENCE_ABORT_HASH_MISMATCH(expected, actual, message) \
  ::qlever::ingress::DivergenceAbort( \
      {::qlever::ingress::AbortCategory::HASH_MISMATCH, \
       __FILE__, __LINE__, __func__, message, expected, actual})

#define DIVERGENCE_ABORT_OOM(requested, available, message) \
  ::qlever::ingress::DivergenceAbort( \
      {::qlever::ingress::AbortCategory::OUT_OF_MEMORY, \
       __FILE__, __LINE__, __func__, message, 0, 0, requested, available})

#define DIVERGENCE_ABORT_GUARD_BREACH(message) \
  DIVERGENCE_ABORT(::qlever::ingress::AbortCategory::GUARD_BREACH, message)

#define DIVERGENCE_ABORT_EPOCH_VIOLATION(message) \
  DIVERGENCE_ABORT(::qlever::ingress::AbortCategory::EPOCH_VIOLATION, message)

#define DIVERGENCE_ABORT_CORRUPT_INDEX(message) \
  DIVERGENCE_ABORT(::qlever::ingress::AbortCategory::CORRUPT_INDEX_DATA, message)

// ============================================================================
// ABORT-OR-RETURN: Check condition, abort if false
// ============================================================================

// Check condition - abort if false with custom message
#define DIVERGENCE_CHECK(condition, category, message) \
  do { \
    if (!(condition)) { \
      DIVERGENCE_ABORT(category, message); \
    } \
  } while (false)

// Check hash equality - abort on mismatch
#define DIVERGENCE_CHECK_HASH(expected, actual, message) \
  do { \
    if ((expected) != (actual)) { \
      DIVERGENCE_ABORT_HASH_MISMATCH(expected, actual, message); \
    } \
  } while (false)

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_DIVERGENCE_ABORT_H

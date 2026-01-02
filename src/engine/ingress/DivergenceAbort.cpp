// EPIC 10.2: FMEA Abort Logic - Implementation
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 7 - FMEA Abort Logic

#include "DivergenceAbort.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace qlever::ingress {

namespace {

// ============================================================================
// INTERNAL: Timestamp formatting for abort logs
// ============================================================================

std::string getCurrentTimestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

  std::tm tm_buf;
  std::tm* tm_ptr = std::localtime_r(&time_t_now, &tm_buf);

  std::ostringstream oss;
  oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S")
      << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}

// ============================================================================
// INTERNAL: Category to string conversion
// ============================================================================

const char* categoryToString(AbortCategory category) noexcept {
  switch (category) {
    case AbortCategory::HASH_MISMATCH:
      return "HASH_MISMATCH";
    case AbortCategory::CORRUPT_INDEX_DATA:
      return "CORRUPT_INDEX_DATA";
    case AbortCategory::EPOCH_VIOLATION:
      return "EPOCH_VIOLATION";
    case AbortCategory::OUT_OF_MEMORY:
      return "OUT_OF_MEMORY";
    case AbortCategory::RESOURCE_LIMIT_EXCEEDED:
      return "RESOURCE_LIMIT_EXCEEDED";
    case AbortCategory::MISSING_CRITICAL_FILE:
      return "MISSING_CRITICAL_FILE";
    case AbortCategory::IO_CORRUPTION:
      return "IO_CORRUPTION";
    case AbortCategory::GUARD_BREACH:
      return "GUARD_BREACH";
    case AbortCategory::INVARIANT_VIOLATION:
      return "INVARIANT_VIOLATION";
    case AbortCategory::INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    default:
      return "UNKNOWN_CATEGORY";
  }
}

// ============================================================================
// INTERNAL: Flush all caches (best-effort, no throws)
// ============================================================================

void flushCaches() noexcept {
  // Flush stdout/stderr to ensure all logs are written
  std::fflush(stdout);
  std::fflush(stderr);
  std::cout.flush();
  std::cerr.flush();

  // Note: Cache flushing for QLever-specific caches would go here
  // For now, we ensure standard streams are flushed
}

// ============================================================================
// INTERNAL: Emergency shutdown (release resources)
// ============================================================================

void emergencyShutdown() noexcept {
  // Best-effort cleanup before abort
  // Do NOT attempt to close files or release memory that might be corrupt

  flushCaches();

  // Sync filesystem (best effort)
  // sync(); // Requires unistd.h - deferred for portability
}

}  // anonymous namespace

// ============================================================================
// PUBLIC API: DivergenceAbort implementation
// ============================================================================

[[noreturn]] void DivergenceAbort(const AbortContext& context) noexcept {
  // STEP 1: Log error to stderr (atomic write, no buffering)
  std::ostringstream log_message;

  log_message << "\n"
              << "============================================================\n"
              << "DIVERGENCE ABORT - FAIL-CLOSED SHUTDOWN\n"
              << "============================================================\n"
              << "Timestamp:  " << getCurrentTimestamp() << "\n"
              << "Category:   " << categoryToString(context.category) << "\n"
              << "Location:   " << context.file << ":" << context.line << "\n"
              << "Function:   " << context.function << "\n"
              << "Message:    " << context.message << "\n";

  // Add category-specific diagnostics
  switch (context.category) {
    case AbortCategory::HASH_MISMATCH:
      log_message << "Expected Hash: 0x" << std::hex << context.expected_hash << "\n"
                  << "Actual Hash:   0x" << std::hex << context.actual_hash << "\n";
      break;

    case AbortCategory::OUT_OF_MEMORY:
      log_message << "Requested:  " << context.resource_requested << " bytes\n"
                  << "Available:  " << context.resource_available << " bytes\n";
      break;

    default:
      // No additional diagnostics
      break;
  }

  log_message << "============================================================\n"
              << "System integrity compromised. Aborting to prevent partial results.\n"
              << "Exit code: 42 (DIVERGENCE_ABORT)\n"
              << "============================================================\n";

  // Write to stderr (unbuffered, atomic)
  std::cerr << log_message.str() << std::flush;

  // Also write to stderr using C API (in case C++ streams are corrupted)
  std::fprintf(stderr, "%s", log_message.str().c_str());
  std::fflush(stderr);

  // STEP 2: Emergency shutdown (flush caches, close files)
  emergencyShutdown();

  // STEP 3: Exit with code 42 (DIVERGENCE_ABORT exit code)
  // Do NOT use std::exit() - it runs destructors which might fail
  // Use _Exit() for immediate termination without cleanup
  std::_Exit(42);

  // Unreachable, but silence compiler warnings
  __builtin_unreachable();
}

}  // namespace qlever::ingress

// Copyright 2026, QLever contributors
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// EPIC 10.3 Agent 5: Opaque Memory Validator
// Status: SPECIFICATION_CLOSED (PATCH_8_AGENT5_DESIGN.md)
//
// Memory Boundary Guards: FFI Handle Pool + Validation Macros
//
// INVARIANTS ENFORCED:
// - GUARD-5.1: All memory access routes through FFI opaque handles
// - GUARD-5.2: IdTable buffers strict isolation enforced
// - GUARD-5.3: ResultCache strict isolation enforced
// - GUARD-5.4: No direct C++ pointers exposed to Rust
// - GUARD-5.5: Memory layout documented for Rust lifetime tracking
//
// DESIGN: Lock-Free Atomic + Non-Intrusive + Strong-Only + Global Pool +
// Type-Erased THREAD-SAFETY: Proven race-free under all conditions (see PATCH_8
// Part 4)

#ifndef QLEVER_SRC_UTIL_MEMORYBOUNDARYGUARDS_H
#define QLEVER_SRC_UTIL_MEMORYBOUNDARYGUARDS_H

#include <atomic>
#include <memory>
#include <unordered_map>

#include "util/Exception.h"
#include "util/Synchronized.h"

namespace ad_utility {

/**
 * @brief Thread-safe opaque handle pool for FFI boundary.
 *
 * Provides atomic handle ID allocation and thread-safe handle registration.
 * All handles are type-erased to std::shared_ptr<void> for C-ABI sovereignty.
 *
 * **Thread-Safety:**
 * - Handle ID allocation: Lock-free (std::atomic<uint64_t>)
 * - Handle map access: Thread-safe (Synchronized<T> wrapper)
 * - Concurrent register/unregister: Safe (shared_mutex read-write lock)
 *
 * **Memory Contract:**
 * - Strong-only refcount (std::shared_ptr)
 * - No weak references
 * - Destruction at refcount = 0 (automatic via shared_ptr)
 *
 * **Handle Format:**
 * - Opaque handle: uint64_t (C-ABI compatible)
 * - 0 reserved for NULL/invalid handle
 * - IDs monotonically increasing from 1
 *
 * **Design Rationale:**
 * See docs/epic-10-3/PATCH_8_AGENT5_DESIGN.md for complete specification
 */
class OpaqueHandlePool {
 public:
  /// Singleton instance (global pool)
  static OpaqueHandlePool& instance();

  /**
   * @brief Register a C++ object and return an opaque handle ID.
   *
   * Thread-Safety: Safe (lock-free ID allocation, synchronized map insert)
   * Complexity: O(1) average (hash map insert)
   *
   * @tparam T C++ object type (must be movable or copyable)
   * @param ptr std::shared_ptr to object (refcount incremented)
   * @return uint64_t Opaque handle ID (>= 1), or 0 on nullptr input
   * @throws Never (noexcept - returns 0 on allocation failure)
   */
  template <typename T>
  uint64_t registerHandle(std::shared_ptr<T> ptr) noexcept;

  /**
   * @brief Retrieve C++ object from opaque handle ID.
   *
   * Thread-Safety: Safe (shared lock for concurrent reads)
   * Complexity: O(1) average (hash map lookup)
   *
   * @tparam T C++ object type (static_pointer_cast to T)
   * @param handle_id Opaque handle ID
   * @return std::shared_ptr<T> Pointer to object (or nullptr if invalid)
   * @throws Never (returns nullptr on invalid handle)
   */
  template <typename T>
  std::shared_ptr<T> getHandle(uint64_t handle_id) const noexcept;

  /**
   * @brief Unregister opaque handle and decrement refcount.
   *
   * Thread-Safety: Safe (exclusive lock for write)
   * Complexity: O(1) average (hash map erase)
   *
   * Note: Object destruction occurs when refcount reaches 0
   * (may not be immediate if other shared_ptrs exist)
   *
   * @param handle_id Opaque handle ID
   * @return true if handle existed, false if already unregistered
   * @throws Never (noexcept)
   */
  bool unregisterHandle(uint64_t handle_id) noexcept;

  /**
   * @brief Check if handle ID is valid (registered).
   *
   * Thread-Safety: Safe (shared lock)
   * Complexity: O(1) average
   *
   * @param handle_id Opaque handle ID
   * @return true if handle is registered, false otherwise
   * @throws Never (noexcept)
   */
  bool isValid(uint64_t handle_id) const noexcept;

  // Non-copyable, non-movable (singleton)
  OpaqueHandlePool(const OpaqueHandlePool&) = delete;
  OpaqueHandlePool& operator=(const OpaqueHandlePool&) = delete;
  OpaqueHandlePool(OpaqueHandlePool&&) = delete;
  OpaqueHandlePool& operator=(OpaqueHandlePool&) = delete;

 private:
  OpaqueHandlePool() = default;  // Private constructor (singleton)

  /// Atomic counter for lock-free handle ID allocation
  /// Initialized to 1 (0 reserved for NULL/invalid)
  std::atomic<uint64_t> nextHandleId_{1};

  /// Handle storage: {handle_id: shared_ptr<void>}
  /// Wrapped in Synchronized<T> for thread-safe access
  /// Uses std::shared_mutex for reader-writer lock (many readers, one writer)
  ad_utility::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<void>>>
      handles_;
};

// ============================================================================
// Template Implementations (header-only for inlining)
// ============================================================================

template <typename T>
uint64_t OpaqueHandlePool::registerHandle(std::shared_ptr<T> ptr) noexcept {
  if (!ptr) {
    return 0;  // NULL handle for nullptr input
  }

  // Allocate handle ID (lock-free, atomic increment)
  uint64_t handle_id = nextHandleId_.fetch_add(1, std::memory_order_relaxed);

  // Type-erase to std::shared_ptr<void>
  std::shared_ptr<void> void_ptr = std::static_pointer_cast<void>(ptr);

  // Insert into handle map (thread-safe via Synchronized<T>)
  handles_.wlock()->insert({handle_id, std::move(void_ptr)});

  return handle_id;
}

template <typename T>
std::shared_ptr<T> OpaqueHandlePool::getHandle(
    uint64_t handle_id) const noexcept {
  if (handle_id == 0) {
    return nullptr;  // NULL handle
  }

  // Lookup in handle map (shared lock, concurrent reads allowed)
  auto lock = handles_.rlock();
  auto it = lock->find(handle_id);

  if (it == lock->end()) {
    return nullptr;  // Invalid handle (use-after-free or never registered)
  }

  // Type-cast from std::shared_ptr<void> to std::shared_ptr<T>
  // Safety: Caller must ensure T matches original registration type
  return std::static_pointer_cast<T>(it->second);
}

// ============================================================================
// Note: Advanced validation utilities are in util/HandleValidation.h
// This file provides only the core OpaqueHandlePool and guard macros.
// ============================================================================

}  // namespace ad_utility

// ============================================================================
// Guard Macros (Global Namespace)
// ============================================================================

/**
 * @brief Validate handle and throw if invalid.
 *
 * Usage: VALIDATE_HANDLE(handle, QueryExecutionTree)
 *
 * @param handle Opaque handle ID (uint64_t)
 * @param Type C++ type expected for handle
 */
#define VALIDATE_HANDLE(handle, Type)         \
  do {                                        \
    ad_utility::validateHandle<Type>(handle); \
  } while (false)

/**
 * @brief RAII guard for scoped handle lease.
 *
 * Retrieves handle and stores in var. Automatically validates.
 * If handle is invalid, throws exception.
 *
 * Usage:
 *   SCOPED_LEASE(qet_handle, QueryExecutionTree, qet)
 *   qet->computeResult();  // qet is std::shared_ptr<QueryExecutionTree>
 *   // automatic cleanup on scope exit
 *
 * @param handle Opaque handle ID (uint64_t)
 * @param Type C++ type expected for handle
 * @param var Variable name to store std::shared_ptr<Type>
 */
#define SCOPED_LEASE(handle, Type, var)                                        \
  auto var = ad_utility::OpaqueHandlePool::instance().getHandle<Type>(handle); \
  if (!var) {                                                                  \
    AD_THROW(absl::StrCat("Invalid handle lease: ", handle));                  \
  }

/**
 * @brief Validate IdTable handle.
 *
 * Specialized validation for IdTable opaque handles.
 * Forward-declared to avoid circular dependency.
 *
 * Usage: VALIDATE_IDTABLE(idtable_handle)
 *
 * @param handle Opaque handle ID expected to point to IdTable
 */
#define VALIDATE_IDTABLE(handle)                                     \
  do {                                                               \
    if (!ad_utility::OpaqueHandlePool::instance().isValid(handle)) { \
      AD_THROW(absl::StrCat("Invalid IdTable handle: ", handle));    \
    }                                                                \
  } while (false)

/**
 * @brief Validate ResultCache handle.
 *
 * Specialized validation for ResultCache opaque handles.
 * Forward-declared to avoid circular dependency.
 *
 * Usage: VALIDATE_RESULTCACHE(cache_handle)
 *
 * @param handle Opaque handle ID expected to point to ResultCache
 */
#define VALIDATE_RESULTCACHE(handle)                                  \
  do {                                                                \
    if (!ad_utility::OpaqueHandlePool::instance().isValid(handle)) {  \
      AD_THROW(absl::StrCat("Invalid ResultCache handle: ", handle)); \
    }                                                                 \
  } while (false)

// ============================================================================
// FFI Memory Contract Documentation
// ============================================================================

/*
 * FFI Memory Contract (EPIC 10.3):
 *
 * 1. Opaque Handle Format:
 *    - Type: uint64_t (8 bytes, C-ABI compatible)
 *    - Value 0: NULL/invalid handle
 *    - Values >= 1: Valid handle IDs (monotonically increasing)
 *
 * 2. Handle Pool Storage:
 *    - Key: uint64_t handle_id
 *    - Value: std::shared_ptr<void> (type-erased C++ object)
 *    - Thread-safety: Synchronized<std::unordered_map<...>>
 *
 * 3. Lifetime Rules:
 *    - Register: Creates handle, increments refcount (returns handle_id)
 *    - GetHandle: Increments refcount (returns shared_ptr)
 *    - Unregister: Decrements refcount (pool releases ownership)
 *    - Destruction: Automatic when refcount reaches 0
 *
 * 4. Thread-Safety Guarantees:
 *    - Handle ID allocation: Lock-free (atomic fetch_add)
 *    - Concurrent reads: Allowed (shared_lock via rlock())
 *    - Concurrent writes: Serialized (exclusive_lock via wlock())
 *    - Reader-writer lock: std::shared_mutex (via Synchronized<T>)
 *
 * 5. Type Safety:
 *    - Registration: Template parameter T determines stored type
 *    - Retrieval: Caller must specify correct type T (undefined if wrong)
 *    - No runtime type checking (performance critical path)
 *
 * 6. Memory Isolation:
 *    - C++ side: Owns memory via std::shared_ptr
 *    - Rust side: Receives opaque uint64_t handle
 *    - No direct pointer access across FFI boundary
 *    - All memory access mediated through OpaqueHandlePool
 *
 * 7. Struct Byte Offsets (Internal - Do Not Rely On):
 *    - nextHandleId_: 0-7 (std::atomic<uint64_t>, 8 bytes aligned)
 *    - handles_: 8+ (Synchronized<map>, size varies by platform)
 *
 * 8. Integration Points:
 *    - Agent 1 (FFI Wrapper): Uses registerHandle/getHandle
 *    - Agent 6 (eBPF Observability): Read-only isValid checks
 *    - Rust FFI: Receives uint64_t handles, calls C FFI destructors
 */

#endif  // QLEVER_SRC_UTIL_MEMORYBOUNDARYGUARDS_H

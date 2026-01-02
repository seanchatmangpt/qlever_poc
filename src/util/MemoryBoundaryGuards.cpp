// Copyright 2026, QLever contributors
// SPDX-License-Identifier: Apache-2.0 OR MIT
//
// EPIC 10.3 Agent 5: Opaque Memory Validator
// Implementation of non-template OpaqueHandlePool methods

#include "util/MemoryBoundaryGuards.h"

namespace ad_utility {

OpaqueHandlePool& OpaqueHandlePool::instance() {
  static OpaqueHandlePool pool;
  return pool;
}

bool OpaqueHandlePool::unregisterHandle(uint64_t handle_id) noexcept {
  if (handle_id == 0) {
    return false;  // NULL handle
  }

  // Erase from handle map (exclusive lock, write operation)
  auto lock = handles_.wlock();
  size_t erased = lock->erase(handle_id);

  // Return true if handle existed, false if already unregistered
  return erased > 0;
}

bool OpaqueHandlePool::isValid(uint64_t handle_id) const noexcept {
  if (handle_id == 0) {
    return false;  // NULL handle
  }

  // Check existence in handle map (shared lock)
  auto lock = handles_.rlock();
  return lock->find(handle_id) != lock->end();
}

size_t OpaqueHandlePool::size() const noexcept {
  // Get number of registered handles (shared lock)
  auto lock = handles_.rlock();
  return lock->size();
}

}  // namespace ad_utility

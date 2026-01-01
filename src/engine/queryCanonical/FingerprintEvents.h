// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTEVENTS_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTEVENTS_H

#include <chrono>
#include <functional>

#include "engine/queryCanonical/QueryFingerprint.h"

namespace queryCanonical {

// Event data emitted when a query is fingerprinted
struct FingerprintEvent {
  const QueryFingerprint& fingerprint;
  const QueryFingerprintStats& stats;
  std::chrono::steady_clock::time_point timestamp;
};

// Hook function signature for fingerprint events
using FingerprintEventHandler = std::function<void(const FingerprintEvent&)>;

// Event emission system (stub for now - EPIC 10 will activate)
class FingerprintEventEmitter {
 public:
  // Get singleton instance
  static FingerprintEventEmitter& getInstance();

  // Register event handler (for EPIC 10 observability integration)
  void registerHandler(FingerprintEventHandler handler);

  // Emit event when query is fingerprinted
  void emit(const QueryFingerprint& fingerprint,
            const QueryFingerprintStats& stats);

  // Enable/disable event emission
  void setEnabled(bool enabled) { enabled_ = enabled; }
  [[nodiscard]] bool isEnabled() const { return enabled_; }

 private:
  FingerprintEventEmitter() = default;
  ~FingerprintEventEmitter() = default;

  // Disable copy and move
  FingerprintEventEmitter(const FingerprintEventEmitter&) = delete;
  FingerprintEventEmitter& operator=(const FingerprintEventEmitter&) = delete;

  std::vector<FingerprintEventHandler> handlers_;
  bool enabled_ = false;  // Disabled by default until EPIC 10
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTEVENTS_H

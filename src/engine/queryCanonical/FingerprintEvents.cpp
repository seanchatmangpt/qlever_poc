// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/FingerprintEvents.h"

namespace queryCanonical {

// ____________________________________________________________________________
FingerprintEventEmitter& FingerprintEventEmitter::getInstance() {
  static FingerprintEventEmitter instance;
  return instance;
}

// ____________________________________________________________________________
void FingerprintEventEmitter::registerHandler(
    FingerprintEventHandler handler) {
  handlers_.push_back(std::move(handler));
}

// ____________________________________________________________________________
void FingerprintEventEmitter::emit(const QueryFingerprint& fingerprint,
                                   const QueryFingerprintStats& stats) {
  if (!enabled_) {
    return;  // No-op until EPIC 10 activates observability
  }

  FingerprintEvent event{fingerprint, stats,
                         std::chrono::steady_clock::now()};

  for (const auto& handler : handlers_) {
    handler(event);
  }
}

}  // namespace queryCanonical

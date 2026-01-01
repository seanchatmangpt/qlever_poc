#include "global/Epoch.h"

#include "util/Log.h"

namespace ad_utility {

// Global singleton instance
ad_utility::Synchronized<EpochManager> globalEpochManager;

// Transition: INIT -> INGEST
void EpochManager::transitionToIngest() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INIT) {
    throw std::logic_error("Cannot transition to INGEST from non-INIT state");
  }
  lock->state_ = EpochState::INGEST;
}

// Transition: INGEST -> SEAL
void EpochManager::transitionToSeal() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error("Cannot transition to SEAL from non-INGEST state");
  }
  lock->state_ = EpochState::SEAL;
}

// Transition: SEAL -> SERVE (increments transition counter)
void EpochManager::transitionToServe() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SEAL) {
    throw std::logic_error("Cannot transition to SERVE from non-SEAL state");
  }
  lock->state_ = EpochState::SERVE;
  lock->transitionCount_++;

  // Log epoch SERVE transition
  // This signals to the system that the cache is now valid for this epoch.
  // Cache invalidation is handled implicitly via snapshot indices in
  // QueryCacheKey.
  AD_LOG_INFO << "Epoch transitioned to SERVE with ID " << lock->epochId_;
}

// Restart: SERVE -> INIT (increments epoch ID)
void EpochManager::restart() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error("Cannot restart epoch from non-SERVE state");
  }
  lock->state_ = EpochState::INIT;
  lock->epochId_++;
}

// Query epoch binding: return current epoch ID if in SERVE state
EpochId EpochManager::getCurrentEpochIdForQuery() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error("Query attempted outside SERVE epoch");
  }
  return lock->epochId_;
}

// Mutation check: allow only in INGEST state
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error("Write attempted outside INGEST epoch");
  }
}

// Observability: get current state
EpochState EpochManager::getState() const {
  auto lock = state_.acquire();
  return lock->state_;
}

// Observability: get current epoch ID
EpochId EpochManager::getEpochId() const {
  auto lock = state_.acquire();
  return lock->epochId_;
}

// Observability: get transition count
uint64_t EpochManager::getTransitionCount() const {
  auto lock = state_.acquire();
  return lock->transitionCount_;
}

// Ingress capability token: proof of authorized write during INGEST
// Returns token with current epoch ID, throws if not in INGEST state.
EpochManager::IngressCapabilityToken EpochManager::getIngressCapabilityToken()
    const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error(
        "getIngressCapabilityToken() called outside INGEST state. "
        "Ingress writes only permitted during INGEST epoch.");
  }
  // Generate unique token ID by incrementing counter
  uint64_t tokenId = lock->tokenCounter_++;
  // Return token bound to current epoch
  return IngressCapabilityToken(lock->epochId_, tokenId);
}

// Validate ingress capability token
// Checks that token was issued for the current epoch.
// Returns true if token is valid, false if expired (epoch changed).
bool EpochManager::validateIngressCapability(
    const IngressCapabilityToken& token) const {
  auto lock = state_.acquire();
  // Token is valid only if its epoch matches current epoch
  // This ensures tokens from previous epochs are rejected after restart()
  if (token.getEpochId() != lock->epochId_) {
    AD_LOG_WARN << "Ingress capability token rejected: "
                << "token epoch=" << token.getEpochId()
                << " current epoch=" << lock->epochId_;
    return false;
  }
  return true;
}

// Record ingress write with token
// Increments audit counter for observability and compliance.
// Should be called by barriers after validating token.
void EpochManager::recordIngressWrite(const IngressCapabilityToken& token) {
  auto lock = state_.acquire();
  // Validate token still belongs to current epoch
  if (token.getEpochId() != lock->epochId_) {
    throw std::logic_error(
        "recordIngressWrite() called with token from different epoch");
  }
  // Increment ingress write counter for audit trail
  lock->ingressWriteCount_++;
  AD_LOG_DEBUG << "Recorded ingress write (count=" << lock->ingressWriteCount_
               << ") for token " << token.getTokenId() << " in epoch "
               << lock->epochId_;
}

// Observability: get ingress write count
uint64_t EpochManager::getIngressWriteCount() const {
  auto lock = state_.acquire();
  return lock->ingressWriteCount_;
}

// Get manifest bound to current epoch
// Returns the manifest if one exists, std::nullopt otherwise.
// Used for deterministic cache keying and validation.
std::optional<EpochManifest> EpochManager::getCurrentEpochManifest() const {
  auto lock = state_.acquire();
  return lock->currentManifest_;
}

// EPIC 1.1: Atomic promotion pattern for safe two-epoch handshake
// ================================================================

// Atomic promotion: atomically transition to new epoch with validation
// Precondition: current epoch must be in SERVE state
// Postcondition: new epoch in SERVE state, old epoch backed up for rollback
//
// Thread Safety:
//   - All state modifications are protected by the Synchronized<State> lock
//   - Callbacks are invoked without holding the lock to prevent deadlock
//   - promotionInProgress_ flag prevents concurrent promotion attempts
EpochId EpochManager::atomicPromoteToNewEpoch(
    std::function<void(EpochId)> onBeforePromote,
    std::function<void(EpochId)> onAfterPromote) {
  EpochId newEpochId;

  // Phase 1: Atomic setup (check preconditions, increment epoch, save backup)
  {
    auto lock = state_.acquire();

    if (lock->state_ != EpochState::SERVE) {
      throw std::logic_error(
          "Cannot promote: current state must be SERVE, got " +
          std::string(lock->state_ == EpochState::INIT     ? "INIT"
                      : lock->state_ == EpochState::INGEST ? "INGEST"
                                                           : "SEAL"));
    }

    if (lock->promotionInProgress_) {
      throw std::logic_error(
          "Promotion already in progress: cannot start concurrent promotion");
    }

    // Mark promotion as in progress (prevents concurrent attempts)
    lock->promotionInProgress_ = true;

    // Save backup for rollback capability
    lock->backupEpochId_ = lock->epochId_;

    // Increment epoch ID and capture new ID
    lock->epochId_++;
    newEpochId = lock->epochId_;

    // Transition to INIT to start building new epoch offline
    lock->state_ = EpochState::INIT;

    AD_LOG_INFO << "Atomic promotion started: "
                << "saving backup epoch " << lock->backupEpochId_ << ", "
                << "new epoch " << newEpochId << " in INIT state";
  }  // Lock released here

  // Phase 2: Validation hook (outside lock to prevent deadlock)
  // If validation fails, we automatically rollback
  try {
    if (onBeforePromote) {
      onBeforePromote(newEpochId);
    }
  } catch (const std::exception& e) {
    AD_LOG_WARN << "Atomic promotion failed during validation: " << e.what();
    // Automatically rollback on validation failure
    rollbackPromotion();
    throw;
  }

  // Phase 3: Complete state transitions
  // These transitions must succeed; if they don't, something is seriously
  // wrong with the state machine
  try {
    transitionToIngest();
    AD_LOG_DEBUG << "Epoch " << newEpochId << " transitioned to INGEST";

    transitionToSeal();
    AD_LOG_DEBUG << "Epoch " << newEpochId << " transitioned to SEAL";

    transitionToServe();
    AD_LOG_INFO << "Epoch " << newEpochId << " transitioned to SERVE";
  } catch (const std::exception& e) {
    AD_LOG_ERROR << "Fatal: state transition failed during promotion: "
                 << e.what();
    // This is a critical error; manual intervention required
    // but we attempt rollback anyway
    try {
      rollbackPromotion();
    } catch (...) {
      AD_LOG_ERROR << "Rollback also failed!";
    }
    throw;
  }

  // Phase 4: Post-promotion hook (outside lock to prevent deadlock)
  if (onAfterPromote) {
    try {
      onAfterPromote(newEpochId);
    } catch (const std::exception& e) {
      AD_LOG_WARN << "Exception during post-promotion hook (non-fatal): "
                  << e.what();
      // Don't fail the promotion if post-hook fails, but log it
      // The promotion is already committed
    }
  }

  // Phase 5: Mark promotion complete
  {
    auto lock = state_.acquire();
    lock->promotionInProgress_ = false;
    AD_LOG_INFO << "Atomic promotion completed: "
                << "epoch " << newEpochId << " is now SERVE";
  }  // Lock released here

  return newEpochId;
}

// Query: is promotion in progress?
// Returns: true if atomicPromoteToNewEpoch is currently executing
bool EpochManager::isPromotionInProgress() const {
  auto lock = state_.acquire();
  return lock->promotionInProgress_;
}

// Rollback: return to previous epoch (disaster recovery only)
// Precondition: promotion must be in progress
// Postcondition: state restored to previous epoch, promotionInProgress
// cleared
void EpochManager::rollbackPromotion() {
  auto lock = state_.acquire();

  if (!lock->promotionInProgress_) {
    throw std::logic_error(
        "No promotion in progress: cannot rollback when promotionInProgress_ "
        "is false");
  }

  // Restore backup epoch ID
  lock->epochId_ = lock->backupEpochId_;

  // Return to SERVE state (the state before promotion started)
  lock->state_ = EpochState::SERVE;

  // Clear promotion flag
  lock->promotionInProgress_ = false;

  AD_LOG_WARN << "Atomic promotion rolled back: "
              << "restored to epoch " << lock->epochId_ << " in SERVE state";
}

}  // namespace ad_utility

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

}  // namespace ad_utility

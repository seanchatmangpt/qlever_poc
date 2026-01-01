#ifndef AD_UTILITY_EPOCH_H
#define AD_UTILITY_EPOCH_H

#include <cstdint>
#include <stdexcept>

#include "ad_utility/Synchronized.h"

namespace ad_utility {

// Epoch enumeration
enum class EpochState { INIT, INGEST, SEAL, SERVE };

// Epoch identifier - simple monotonic counter
using EpochId = uint64_t;

// Epoch immutability manager
class EpochManager {
 private:
  struct State {
    EpochState state_ = EpochState::INIT;
    EpochId epochId_ = 0;
    uint64_t transitionCount_ = 0;
  };

  Synchronized<State> state_;

 public:
  EpochManager() = default;

  // State machine transitions
  void transitionToIngest();
  void transitionToSeal();
  void transitionToServe();

  // Restart: resets to INIT, increments epoch
  void restart();

  // Query: assert we're in SERVE and return epoch ID
  EpochId getCurrentEpochIdForQuery() const;

  // Mutation check: throws if not in INGEST
  void checkAllowedToMutate() const;

  // Observability
  EpochState getState() const;
  EpochId getEpochId() const;
  uint64_t getTransitionCount() const;
};

// Global singleton
extern ad_utility::Synchronized<EpochManager> globalEpochManager;

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_H

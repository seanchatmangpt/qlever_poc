#ifndef AD_UTILITY_EPOCH_H
#define AD_UTILITY_EPOCH_H

#include <cstdint>
#include <optional>
#include <stdexcept>

#include "util/Synchronized.h"

namespace ad_utility {

// Epoch identifier - simple monotonic counter
using EpochId = uint64_t;

}  // namespace ad_utility

// Include EpochManifest after EpochId is defined to break circular dependency
#include "global/EpochManifest.h"

namespace ad_utility {

// Epoch enumeration
enum class EpochState { INIT, INGEST, SEAL, SERVE };

// Ingress source classification
enum class IngressSource { DIRECT_API, INGRESS_PIPELINE };

// Epoch immutability manager
class EpochManager {
 private:
  struct State {
    EpochState state_ = EpochState::INIT;
    EpochId epochId_ = 0;
    uint64_t transitionCount_ = 0;
    uint64_t tokenCounter_ = 0;       // For generating unique token IDs
    uint64_t ingressWriteCount_ = 0;  // Audit trail of ingress writes
    bool promotionInProgress_ = false;
    EpochId backupEpochId_ = 0;  // Saved epoch ID for rollback
    std::optional<EpochManifest> currentManifest_;  // Manifest of current epoch
  };

  Synchronized<State> state_;

 public:
  // Ingress capability token - proof of authorized write during INGEST
  class IngressCapabilityToken {
   private:
    EpochId epochId_;
    uint64_t tokenId_;

    friend class EpochManager;
    explicit IngressCapabilityToken(EpochId epoch, uint64_t id)
        : epochId_(epoch), tokenId_(id) {}

   public:
    EpochId getEpochId() const { return epochId_; }
    uint64_t getTokenId() const { return tokenId_; }
  };

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

  // Ingress-only write enforcement
  // Returns ingress write capability token (only during INGEST).
  // Throws if not in INGEST state.
  IngressCapabilityToken getIngressCapabilityToken() const;

  // Check if write came through authorized ingress (used by barrier).
  // Internal method: verifies token validity against current epoch.
  bool validateIngressCapability(const IngressCapabilityToken& token) const;

  // Mark ingress source (informational, for audit trail).
  // Increments ingress write counter for observability.
  void recordIngressWrite(const IngressCapabilityToken& token);

  // Observability
  EpochState getState() const;
  EpochId getEpochId() const;
  uint64_t getTransitionCount() const;
  uint64_t getIngressWriteCount() const;

  // Get manifest bound to current epoch (for deterministic cache keying)
  std::optional<EpochManifest> getCurrentEpochManifest() const;

  // EPIC 1.1: Atomic promotion pattern for safe two-epoch handshake
  // ================================================================
  // Build new epoch offline (indexing, validation) → epoch(n+1)
  // When ready: atomically promote epoch(n+1) → SERVE
  // No half-baked visibility

  // Atomic promotion: atomically transition to new epoch with validation
  // Precondition: current epoch must be in SERVE state
  // Postcondition: new epoch in SERVE state, old epoch backed up for rollback
  //
  // Flow:
  // 1. Atomically: increment epochId, save backup, mark promotion in progress
  // 2. Call onBeforePromote(newEpochId) hook for validation
  // 3. Complete state transitions: INIT → INGEST → SEAL → SERVE
  // 4. Call onAfterPromote(newEpochId) hook for cache invalidation
  //
  // Parameters:
  //   onBeforePromote: Optional hook called before state transitions. Should
  //                    validate that new epoch is ready. Throws exception to
  //                    prevent promotion and trigger automatic rollback.
  //   onAfterPromote:  Optional hook called after promotion completes. Should
  //                    invalidate caches and notify subscribers.
  //
  // Returns: The new epoch ID
  //
  // Throws:
  //   - std::logic_error if not in SERVE state
  //   - std::logic_error if promotion already in progress
  //   - Any exception from onBeforePromote hook (triggers rollback)
  EpochId atomicPromoteToNewEpoch(
      std::function<void(EpochId)> onBeforePromote = nullptr,
      std::function<void(EpochId)> onAfterPromote = nullptr);

  // Query: is promotion in progress?
  // Returns: true if atomicPromoteToNewEpoch is currently executing
  // Purpose: Prevent concurrent promotion attempts; allows external
  //          systems to wait for or reject promotion requests.
  bool isPromotionInProgress() const;

  // Rollback: return to previous epoch (disaster recovery only)
  // Precondition: promotion must be in progress
  // Postcondition: state restored to previous epoch, promotionInProgress
  // cleared
  //
  // This is called automatically if onBeforePromote throws.
  // Can also be called manually for emergency rollback.
  //
  // Throws: std::logic_error if no promotion in progress
  void rollbackPromotion();
};

// Global singleton
extern ad_utility::Synchronized<EpochManager> globalEpochManager;

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_H

// EPIC 7: SIMD-Accelerated JSON-LD Ingress Wrapper
// Wraps simdjson behind QLever-owned interface with error codes
// Implements shared invariant: silence + determinism + error codes

#ifndef QLEVER_ENGINE_INGRESS_SIMD_JSON_INGRESS_WRAPPER_H
#define QLEVER_ENGINE_INGRESS_SIMD_JSON_INGRESS_WRAPPER_H

#include "IngressResult.h"
#include "ErrorCodes.h"
#include <string_view>
#include <string>

namespace qlever::ingress {

class SimdJsonIngressWrapper {
public:
  SimdJsonIngressWrapper() = default;
  ~SimdJsonIngressWrapper() = default;
  
  // Deleted copy/move constructors (stateless wrapper)
  SimdJsonIngressWrapper(const SimdJsonIngressWrapper&) = delete;
  SimdJsonIngressWrapper(SimdJsonIngressWrapper&&) = delete;
  SimdJsonIngressWrapper& operator=(const SimdJsonIngressWrapper&) = delete;
  SimdJsonIngressWrapper& operator=(SimdJsonIngressWrapper&&) = delete;

  // Parse JSON-LD document with SIMD acceleration
  // - Uses simdjson::ondemand parser for streaming
  // - Returns error code + digest, never throws exceptions
  // - Deterministic: same input always produces same digest
  IngressResult parseJsonLd(std::string_view json_input) noexcept;

  // Validate structural correctness only (no semantic checks)
  // - Checks JSON syntax, nesting depth, bracket matching
  // - No type validation or @context checking
  IngressResult validateStructure(std::string_view json_input) noexcept;

  // Deterministic normalization to canonical form
  // - Alphabetical field ordering at all nesting levels
  // - UTF-8 NFC normalization
  // - Consistent whitespace removal
  // - Number canonicalization
  // - Output: normalized_output parameter filled, digest computed
  IngressResult normalizeJsonLd(std::string_view json_input,
                                std::string& normalized_output) noexcept;

private:
  // SIMD technique flags (set by parser)
  enum class SimdTechnique : uint32_t {
    StructuralScanning = 1u << 0,   // SIMD byte classification
    QuoteDetection = 1u << 1,       // SIMD vectorized quote pairing
    NumberValidation = 1u << 2,     // SIMD digit classification
    BracePairing = 1u << 3,         // Branchless brace/bracket pairing
    ErrorClassification = 1u << 4,  // Bitmask error classification
  };

  // Compute SHA256 digest of canonical JSON-LD
  // - Includes validation bitmask and error code
  // - Deterministic across all runs and machines
  std::string compute_digest(std::string_view normalized_json,
                             uint32_t validation_mask,
                             IngressErrorCode error_code) noexcept;

  // Fallback to native C++ parser (explicit, not implicit)
  // - Called only on non-SIMD paths or simdjson parse failures
  IngressResult fallback_parse(std::string_view json_input) noexcept;
};

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_SIMD_JSON_INGRESS_WRAPPER_H

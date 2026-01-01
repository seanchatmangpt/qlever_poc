// EPIC 7: Ingress Result Artifact
// Return type for all ingress operations (error codes + digest)

#ifndef QLEVER_ENGINE_INGRESS_INGRESS_RESULT_H
#define QLEVER_ENGINE_INGRESS_INGRESS_RESULT_H

#include <cstdint>
#include <string>

#include "ErrorCodes.h"

namespace qlever::ingress {

struct IngressResult {
  // Error code (0 = success, non-zero = error)
  IngressErrorCode error = IngressErrorCode::OK;

  // Metrics (for observability, not diagnostics)
  uint64_t bytes_parsed = 0;
  uint64_t document_count = 0;

  // Deterministic SHA256 digest (hex-encoded, 64 chars)
  std::string digest_sha256;

  // SIMD validation bitmask (which techniques passed)
  uint32_t simd_validation_mask = 0;

  // Constructor
  IngressResult() = default;
  IngressResult(IngressErrorCode ec) noexcept : error(ec) {}
};

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_INGRESS_RESULT_H

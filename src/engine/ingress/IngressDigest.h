// EPIC 7: Deterministic Digest Computation
// SHA256-based digest for reproducible JSON-LD processing
// Same input always produces same digest (determinism guarantee)

#ifndef QLEVER_ENGINE_INGRESS_INGRESS_DIGEST_H
#define QLEVER_ENGINE_INGRESS_INGRESS_DIGEST_H

#include "ErrorCodes.h"
#include <string>
#include <string_view>
#include <array>

namespace qlever::ingress {

// SHA256 digest (32 bytes, hex-encoded = 64 chars)
using Digest = std::array<unsigned char, 32>;

class IngressDigest {
public:
  // Compute deterministic digest of normalized JSON-LD
  // Includes: canonical JSON + validation bitmask + error code
  // Result: stable, reproducible SHA256 hash
  static Digest compute(std::string_view normalized_json,
                       uint32_t validation_mask,
                       IngressErrorCode error_code) noexcept;

  // Hex-encode digest for human-readable output
  // Input: 32-byte binary digest
  // Output: 64-character hex string (lowercase)
  static std::string hex_encode(const Digest& binary_digest) noexcept;

  // Hex-decode digest from human-readable format
  // Input: 64-character hex string
  // Output: 32-byte binary digest
  static Digest hex_decode(std::string_view hex_string) noexcept;

  // Verify determinism: digest consistency across multiple computations
  // Input: same normalized_json, mask, error_code
  // Output: true if all produced digests match
  // (Used for testing, not in production hot-path)
  static bool verify_determinism(std::string_view normalized_json,
                                uint32_t validation_mask,
                                IngressErrorCode error_code,
                                int iterations = 100) noexcept;

private:
  // Internal SHA256 implementation (or wrapper around OpenSSL/mbedTLS)
  // Must produce identical results across all platforms
  static Digest sha256(const unsigned char* input,
                      size_t input_len) noexcept;

  // Canonical serialization: JSON + bitmask + error code
  // All serialization deterministic (no floating-point, byte order issues)
  static std::string serialize_canonical(
      std::string_view normalized_json,
      uint32_t validation_mask,
      IngressErrorCode error_code) noexcept;
};

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_INGRESS_DIGEST_H

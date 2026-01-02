// EPIC 10.1: JSON-LD-Only Ingress Normalizer for Rule & Constraint Plane
// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Agent 6 - JSON-LD Ingress Normalization
//
// SPEC-LOCK CONSTRAINTS:
// - Section 2: JSON-LD ingress for SHACL, ShEx, N3, Datalog (no other dialects)
// - Section 6.5: SHACL/ShEx/N3/Datalog ingestion accepts JSON-LD and rejects
//                other dialects at ingress
// - Section 6.5: Ingestion normalization is deterministic and bound to epoch
//                identity + guard identity
// - Section 4.3: Bounded compute everywhere (ingress parsing must have guards)
// - Section 4.4: Hot path silence (no logging in JSON-LD parsing)

#ifndef QLEVER_ENGINE_INGRESS_JSONLD_INGRESS_NORMALIZER_H
#define QLEVER_ENGINE_INGRESS_JSONLD_INGRESS_NORMALIZER_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ErrorCodes.h"
#include "IngressDigest.h"
#include "IngressResult.h"
#include "global/Epoch.h"

namespace qlever::ingress {

// Supported rule & constraint language types for JSON-LD ingress
enum class RuleLanguageDialect : uint8_t {
  SHACL = 1,   // SHACL constraint language (W3C)
  SHEX = 2,    // ShEx schema language
  N3 = 3,      // Notation3 (superset of Turtle with rules)
  DATALOG = 4  // Datalog rule language
};

// Guard configuration for bounded parsing
struct IngressGuardConfig {
  // Maximum input size (bytes) - default 100MB
  size_t max_input_size_bytes = 100 * 1024 * 1024;

  // Maximum nesting depth - default 100 levels
  size_t max_nesting_depth = 100;

  // Maximum number of JSON keys in single object - default 10000
  size_t max_object_keys = 10000;

  // Maximum string length (bytes) - default 1MB
  size_t max_string_length_bytes = 1024 * 1024;

  // Timeout for parsing operation (milliseconds) - default 30 seconds
  uint64_t timeout_ms = 30000;

  // Guard identity for audit trail (binds to epoch for determinism)
  uint64_t guard_identity_hash = 0;
};

// Result of dialect detection at ingress
struct DialectDetectionResult {
  bool is_json_ld = false;   // True if input is JSON-LD
  bool is_turtle = false;    // True if input is Turtle (REJECTED)
  bool is_ntriples = false;  // True if input is N-Triples (REJECTED)
  bool is_rdfxml = false;    // True if input is RDF/XML (REJECTED)
  IngressErrorCode error = IngressErrorCode::OK;
  std::string detection_info;  // Cold-path only: why detection failed
};

// JSON-LD-only ingress normalizer for rule & constraint languages
//
// GUARANTEES:
// - Only accepts JSON-LD format (rejects Turtle, N-Triples, RDF/XML at ingress)
// - Deterministic normalization: same input → same canonical form → same digest
// - Epoch-bound: normalization result binds to EpochId and GuardConfig
// - Bounded compute: enforces max size, max depth, timeout guards
// - Hot-path silence: no logging/printing in parsing loop
// - Fail-closed: any guard violation → immediate error return
//
// USAGE:
//   JsonLdIngressNormalizer normalizer;
//   auto token = epochManager.getIngressCapabilityToken();
//   IngressGuardConfig guards;
//   guards.max_input_size_bytes = 50 * 1024 * 1024; // 50MB
//
//   auto result = normalizer.normalizeForDialect(
//       json_ld_input,
//       RuleLanguageDialect::SHACL,
//       token,
//       guards);
//
//   if (result.error != IngressErrorCode::OK) {
//     // Handle error (fail-closed)
//   }
//
//   // Use result.digest_sha256 for caching/determinism
//
class JsonLdIngressNormalizer {
 public:
  JsonLdIngressNormalizer() = default;
  ~JsonLdIngressNormalizer() = default;

  // Deleted copy/move (stateless normalizer)
  JsonLdIngressNormalizer(const JsonLdIngressNormalizer&) = delete;
  JsonLdIngressNormalizer(JsonLdIngressNormalizer&&) = delete;
  JsonLdIngressNormalizer& operator=(const JsonLdIngressNormalizer&) = delete;
  JsonLdIngressNormalizer& operator=(JsonLdIngressNormalizer&&) = delete;

  // ========================================================================
  // PRIMARY INGRESS API: Detect dialect and reject non-JSON-LD at interface
  // ========================================================================

  // Detect input format dialect
  // Returns: DialectDetectionResult indicating format type
  // Purpose: Reject non-JSON-LD formats at ingress before normalization
  //
  // REJECTION CRITERIA:
  // - Turtle: input starts with "@prefix" or "@base" (not JSON)
  // - N-Triples: input contains "<...> <...> <...> ." pattern
  // - RDF/XML: input starts with "<?xml" or "<rdf:RDF"
  // - JSON-LD: input is valid JSON with "@context" field
  //
  // Thread-safe: Yes
  // Hot-path silence: Yes (no logging)
  // Deterministic: Yes (same input → same detection result)
  static DialectDetectionResult detectDialect(std::string_view input) noexcept;

  // Normalize JSON-LD for specific rule/constraint language dialect
  //
  // PRECONDITIONS:
  // - Input MUST be JSON-LD format (other dialects rejected)
  // - token MUST be valid for current epoch (checked internally)
  // - guards MUST be initialized with valid bounds
  //
  // POSTCONDITIONS:
  // - If error != OK: normalization failed, output undefined
  // - If error == OK: normalized_output contains canonical JSON-LD
  // - Digest computed from (canonical JSON + epoch + guard hash)
  //
  // NORMALIZATION RULES (deterministic):
  // 1. UTF-8 NFC normalization (all strings)
  // 2. Alphabetical key ordering (all nesting levels)
  // 3. Whitespace removal (canonical compact form)
  // 4. Number canonicalization (no trailing zeros, scientific notation)
  // 5. Escape sequence normalization (\uXXXX to minimal form)
  //
  // GUARDS ENFORCED:
  // - max_input_size_bytes: input.size() <= max (BUFFER_OVERFLOW)
  // - max_nesting_depth: JSON depth <= max (PARSE_ERROR_NESTED_DEPTH)
  // - max_object_keys: keys per object <= max (RESOURCE_EXHAUSTED)
  // - max_string_length_bytes: string length <= max (VALIDATION_LENGTH_ERROR)
  // - timeout_ms: parsing completes within timeout (TIMEOUT)
  //
  // EPOCH BINDING:
  // - Digest includes token.getEpochId() for cache invalidation
  // - Digest includes guards.guard_identity_hash for guard versioning
  //
  // Thread-safe: Yes (each call independent)
  // Hot-path silence: Yes (no logging in parsing loop)
  // Exceptions: Never throws (returns error code)
  //
  // ERRORS RETURNED:
  // - UNSUPPORTED_FORMAT: input is not JSON-LD (Turtle/N-Triples/RDF-XML)
  // - BUFFER_OVERFLOW: input size exceeds max_input_size_bytes
  // - PARSE_ERROR_NESTED_DEPTH: nesting exceeds max_nesting_depth
  // - PARSE_ERROR_SYNTAX: JSON syntax invalid
  // - RESOURCE_EXHAUSTED: guard limit exceeded
  // - TIMEOUT: parsing exceeded timeout_ms
  // - NORMALIZATION_FAILED: canonicalization failed
  // - INVALID_ARGUMENT: token invalid for current epoch
  //
  IngressResult normalizeForDialect(
      std::string_view json_ld_input, RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const IngressGuardConfig& guards,
      std::string& normalized_output) noexcept;

  // Convenience overload: normalize with default guards
  IngressResult normalizeForDialect(
      std::string_view json_ld_input, RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      std::string& normalized_output) noexcept {
    IngressGuardConfig default_guards;
    return normalizeForDialect(json_ld_input, dialect, token, default_guards,
                               normalized_output);
  }

  // ========================================================================
  // DIALECT-SPECIFIC VALIDATION: Verify JSON-LD structure for each language
  // ========================================================================

  // Validate JSON-LD structure for SHACL constraints
  // Requirements:
  // - "@context" must include SHACL namespace
  // - Root must be object or array of objects
  // - Each shape must have "sh:targetClass" or "sh:targetNode"
  static IngressErrorCode validateShaclJsonLd(
      std::string_view normalized_json_ld) noexcept;

  // Validate JSON-LD structure for ShEx schemas
  // Requirements:
  // - "@context" must include ShEx namespace
  // - Must contain "shapes" array or single shape object
  static IngressErrorCode validateShExJsonLd(
      std::string_view normalized_json_ld) noexcept;

  // Validate JSON-LD structure for N3 rules
  // Requirements:
  // - "@context" must include N3 namespace
  // - Must contain "rules" array or single rule object
  static IngressErrorCode validateN3JsonLd(
      std::string_view normalized_json_ld) noexcept;

  // Validate JSON-LD structure for Datalog rules
  // Requirements:
  // - "@context" must include Datalog namespace
  // - Must contain "rules" array with head/body structure
  static IngressErrorCode validateDatalogJsonLd(
      std::string_view normalized_json_ld) noexcept;

  // ========================================================================
  // DETERMINISM VERIFICATION: Test suite support
  // ========================================================================

  // Verify determinism: same input → same digest (multiple iterations)
  // Returns: true if all digests match across iterations
  // Purpose: Test suite validation of deterministic normalization
  // Note: NOT for production use (cold-path only)
  static bool verifyDeterminism(
      std::string_view json_ld_input, RuleLanguageDialect dialect,
      const ad_utility::EpochManager::IngressCapabilityToken& token,
      const IngressGuardConfig& guards, int iterations = 100) noexcept;

 private:
  // ========================================================================
  // INTERNAL IMPLEMENTATION: Normalization steps
  // ========================================================================

  // Parse JSON-LD with guards enforced
  // Returns: error code (never throws)
  static IngressErrorCode parseJsonLdWithGuards(
      std::string_view json_ld_input, const IngressGuardConfig& guards,
      std::string& parsed_output) noexcept;

  // Normalize parsed JSON to canonical form
  // - Alphabetical key ordering
  // - UTF-8 NFC normalization
  // - Whitespace removal
  // - Number canonicalization
  static IngressErrorCode canonicalizeJson(
      std::string_view parsed_json, std::string& canonical_output) noexcept;

  // Compute digest binding to epoch + guard config
  // Digest = SHA256(canonical_json || epoch_id || guard_hash)
  static std::string computeEpochBoundDigest(std::string_view canonical_json,
                                             ad_utility::EpochId epoch_id,
                                             uint64_t guard_identity_hash,
                                             uint32_t validation_mask) noexcept;

  // Check if JSON-LD contains required context for dialect
  static bool hasRequiredContext(std::string_view normalized_json_ld,
                                 RuleLanguageDialect dialect) noexcept;
};

// ========================================================================
// UTILITY FUNCTIONS: Dialect names for logging/diagnostics (cold-path only)
// ========================================================================

constexpr std::string_view dialectName(RuleLanguageDialect dialect) noexcept {
  switch (dialect) {
    case RuleLanguageDialect::SHACL:
      return "SHACL";
    case RuleLanguageDialect::SHEX:
      return "ShEx";
    case RuleLanguageDialect::N3:
      return "N3";
    case RuleLanguageDialect::DATALOG:
      return "Datalog";
    default:
      return "UNKNOWN";
  }
}

}  // namespace qlever::ingress

#endif  // QLEVER_ENGINE_INGRESS_JSONLD_INGRESS_NORMALIZER_H

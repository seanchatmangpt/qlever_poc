// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 2 - EPIC 10.1 Definition Set A
//
// Implementation of GuardConfiguration serialization methods

#include "engine/readPlane/GuardConfiguration.h"

#include <openssl/sha.h>

#include <cstring>
#include <iomanip>
#include <sstream>

#include "util/CryptographicHashUtils.h"
#include "util/Log.h"

namespace readPlane {

// ===========================================================================
// Canonical Serialization Implementation
// ===========================================================================

std::string GuardConfiguration::toCanonicalBytes() const {
  std::ostringstream oss;

  // Serialize with fixed byte order and field ordering
  // Format: 4 bytes for active_guards (uint8_t stored as uint32_t for
  // alignment)
  //         1 byte for abort_strategy
  //         4 bytes for abort_timeout_ms
  //         8 bytes for envelope_match_threshold (IEEE 754 double)
  //         1 byte for log_guard_violations (bool -> 0x00/0x01)
  //         1 byte for collect_guard_metrics (bool -> 0x00/0x01)
  //         4 bytes for max_logged_violations

  // Field 1: active_guards (4 bytes, big-endian for determinism)
  uint32_t guards_int = static_cast<uint32_t>(active_guards);
  oss << char((guards_int >> 24) & 0xFF) << char((guards_int >> 16) & 0xFF)
      << char((guards_int >> 8) & 0xFF) << char(guards_int & 0xFF);

  // Field 2: abort_strategy (1 byte)
  oss << char(static_cast<uint8_t>(abort_strategy));

  // Field 3: abort_timeout_ms (4 bytes, big-endian)
  oss << char((abort_timeout_ms >> 24) & 0xFF)
      << char((abort_timeout_ms >> 16) & 0xFF)
      << char((abort_timeout_ms >> 8) & 0xFF) << char(abort_timeout_ms & 0xFF);

  // Field 4: envelope_match_threshold (8 bytes, IEEE 754 double, big-endian)
  // WARNING: Using floating-point for serialization breaks determinism
  // across architectures. For EPIC 10.1, we convert to fixed-point instead.
  // Fixed-point: multiply by 1,000,000 (6 decimal places precision)
  uint64_t threshold_fixed =
      static_cast<uint64_t>(envelope_match_threshold * 1000000.0);
  oss << char((threshold_fixed >> 56) & 0xFF)
      << char((threshold_fixed >> 48) & 0xFF)
      << char((threshold_fixed >> 40) & 0xFF)
      << char((threshold_fixed >> 32) & 0xFF)
      << char((threshold_fixed >> 24) & 0xFF)
      << char((threshold_fixed >> 16) & 0xFF)
      << char((threshold_fixed >> 8) & 0xFF) << char(threshold_fixed & 0xFF);

  // Field 5: log_guard_violations (1 byte, 0x00 or 0x01)
  oss << char(log_guard_violations ? 0x01 : 0x00);

  // Field 6: collect_guard_metrics (1 byte, 0x00 or 0x01)
  oss << char(collect_guard_metrics ? 0x01 : 0x00);

  // Field 7: max_logged_violations (4 bytes, big-endian)
  oss << char((max_logged_violations >> 24) & 0xFF)
      << char((max_logged_violations >> 16) & 0xFF)
      << char((max_logged_violations >> 8) & 0xFF)
      << char(max_logged_violations & 0xFF);

  return oss.str();
}

GuardConfiguration GuardConfiguration::fromCanonicalBytes(
    const std::string& bytes) {
  // Verify size: 4 + 1 + 4 + 8 + 1 + 1 + 4 = 23 bytes
  if (bytes.size() != 23) {
    throw std::runtime_error(
        "Invalid canonical bytes: expected 23 bytes, got " +
        std::to_string(bytes.size()));
  }

  GuardConfiguration config;
  size_t offset = 0;

  // Field 1: active_guards (4 bytes, big-endian)
  uint32_t guards_int = (static_cast<uint8_t>(bytes[0]) << 24) |
                        (static_cast<uint8_t>(bytes[1]) << 16) |
                        (static_cast<uint8_t>(bytes[2]) << 8) |
                        static_cast<uint8_t>(bytes[3]);
  config.active_guards = static_cast<GuardRuleType>(guards_int);
  offset = 4;

  // Field 2: abort_strategy (1 byte)
  config.abort_strategy =
      static_cast<AbortStrategy>(static_cast<uint8_t>(bytes[offset]));
  offset++;

  // Field 3: abort_timeout_ms (4 bytes, big-endian)
  config.abort_timeout_ms = (static_cast<uint8_t>(bytes[offset]) << 24) |
                            (static_cast<uint8_t>(bytes[offset + 1]) << 16) |
                            (static_cast<uint8_t>(bytes[offset + 2]) << 8) |
                            static_cast<uint8_t>(bytes[offset + 3]);
  offset += 4;

  // Field 4: envelope_match_threshold (8 bytes, fixed-point)
  uint64_t threshold_fixed =
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset])) << 56) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 1])) << 48) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 2])) << 40) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 3])) << 32) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 4])) << 24) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 5])) << 16) |
      (static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 6])) << 8) |
      static_cast<uint64_t>(static_cast<uint8_t>(bytes[offset + 7]));
  config.envelope_match_threshold = threshold_fixed / 1000000.0;
  offset += 8;

  // Field 5: log_guard_violations (1 byte)
  config.log_guard_violations = (bytes[offset] != 0x00);
  offset++;

  // Field 6: collect_guard_metrics (1 byte)
  config.collect_guard_metrics = (bytes[offset] != 0x00);
  offset++;

  // Field 7: max_logged_violations (4 bytes, big-endian)
  config.max_logged_violations =
      (static_cast<uint8_t>(bytes[offset]) << 24) |
      (static_cast<uint8_t>(bytes[offset + 1]) << 16) |
      (static_cast<uint8_t>(bytes[offset + 2]) << 8) |
      static_cast<uint8_t>(bytes[offset + 3]);

  return config;
}

// ===========================================================================
// JSON-LD Serialization Implementation
// ===========================================================================

nlohmann::ordered_json GuardConfiguration::toJsonLD() const {
  nlohmann::ordered_json j;

  // Alphabetical field ordering for determinism
  j["@type"] = "GuardConfiguration";
  j["abortStrategy"] = abortStrategyToString(abort_strategy);
  j["abortTimeoutMs"] = abort_timeout_ms;
  j["activeGuards"] = guardRuleTypeToString(active_guards);
  j["collectGuardMetrics"] = collect_guard_metrics;
  j["envelopeMatchThreshold"] = envelope_match_threshold;
  j["logGuardViolations"] = log_guard_violations;
  j["maxLoggedViolations"] = max_logged_violations;

  return j;
}

GuardConfiguration GuardConfiguration::fromJsonLD(
    const nlohmann::ordered_json& json) {
  GuardConfiguration config;

  // Parse abort_strategy
  if (json.contains("abortStrategy")) {
    std::string strategy_str = json["abortStrategy"];
    if (strategy_str == "ABORT_IMMEDIATELY") {
      config.abort_strategy = AbortStrategy::ABORT_IMMEDIATELY;
    } else if (strategy_str == "LOG_AND_ABORT") {
      config.abort_strategy = AbortStrategy::LOG_AND_ABORT;
    } else if (strategy_str == "ALERT_AND_ABORT") {
      config.abort_strategy = AbortStrategy::ALERT_AND_ABORT;
    }
  }

  // Parse abort_timeout_ms
  if (json.contains("abortTimeoutMs")) {
    config.abort_timeout_ms = json["abortTimeoutMs"];
  }

  // Parse active_guards (simplified: only supports single guards for now)
  if (json.contains("activeGuards")) {
    std::string guards_str = json["activeGuards"];
    config.active_guards = GuardRuleType(0);
    if (guards_str.find("ALL_GUARDS_STRICT") != std::string::npos) {
      config.active_guards = GuardRuleType::ALL_GUARDS_STRICT;
    } else if (guards_str.find("ENVELOPE_STRICT") != std::string::npos) {
      config.active_guards = GuardRuleType::ENVELOPE_STRICT;
    } else if (guards_str.find("EPOCH_STRICT") != std::string::npos) {
      config.active_guards = GuardRuleType::EPOCH_STRICT;
    }
  }

  // Parse collect_guard_metrics
  if (json.contains("collectGuardMetrics")) {
    config.collect_guard_metrics = json["collectGuardMetrics"];
  }

  // Parse envelope_match_threshold
  if (json.contains("envelopeMatchThreshold")) {
    config.envelope_match_threshold = json["envelopeMatchThreshold"];
  }

  // Parse log_guard_violations
  if (json.contains("logGuardViolations")) {
    config.log_guard_violations = json["logGuardViolations"];
  }

  // Parse max_logged_violations
  if (json.contains("maxLoggedViolations")) {
    config.max_logged_violations = json["maxLoggedViolations"];
  }

  return config;
}

// ===========================================================================
// Canonical Hash Implementation
// ===========================================================================

std::string GuardConfiguration::computeCanonicalHash() const {
  std::string bytes = toCanonicalBytes();

  // Use HashSha256 utility to compute hash
  ad_utility::HashSha256 hasher;
  std::vector<unsigned char> digest = hasher(bytes);

  // Convert to 64-character hex string
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (unsigned char c : digest) {
    oss << std::setw(2) << static_cast<unsigned int>(c);
  }
  return oss.str();
}

// ===========================================================================
// String Representation Implementation
// ===========================================================================

std::string GuardConfiguration::toString() const {
  std::ostringstream oss;
  oss << "GuardConfiguration{"
      << "activeGuards=" << guardRuleTypeToString(active_guards)
      << ", abortStrategy=" << abortStrategyToString(abort_strategy)
      << ", abortTimeoutMs=" << abort_timeout_ms
      << ", envelopeMatchThreshold=" << envelope_match_threshold
      << ", logGuardViolations=" << (log_guard_violations ? "true" : "false")
      << ", collectGuardMetrics=" << (collect_guard_metrics ? "true" : "false")
      << ", maxLoggedViolations=" << max_logged_violations << "}";
  return oss.str();
}

}  // namespace readPlane

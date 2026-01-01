// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Verification Configuration with Guards
// Provides standardized configuration for N3 document verification

#ifndef QLEVER_SRC_ENGINE_N3_N3CONFIG_H
#define QLEVER_SRC_ENGINE_N3_N3CONFIG_H

#include <cstddef>
#include <cstdint>

namespace ad_engine::n3 {

// Configuration for N3 verification with resource guards
// All guards fail closed: exceeding any limit produces an error
struct N3Config {
  // Maximum input document size in bytes
  // Default: 100MB (guards against excessive memory consumption)
  size_t max_input_size_bytes = 100 * 1024 * 1024;

  // Maximum number of blank nodes in document
  // Default: 1,000,000 (prevents blank node explosion)
  size_t max_blank_nodes = 1'000'000;

  // Maximum nesting depth for collections/structures
  // Default: 100 (prevents stack overflow)
  size_t max_nesting_depth = 100;

  // Strict mode: fail on any unknown or uncertain feature
  // Default: false (lenient mode)
  bool strict_mode = false;

  // Maximum memory allocation for verification (bytes)
  // Default: 200MB (2x input size for processing overhead)
  size_t max_memory_bytes = 200 * 1024 * 1024;

  // Timeout for verification in milliseconds
  // Default: 60000ms (60 seconds)
  // Note: 0 means no timeout
  uint64_t timeout_ms = 60'000;

  // Create default configuration
  static N3Config defaultConfig() { return N3Config{}; }

  // Create strict configuration (fail on any uncertainty)
  static N3Config strictConfig() {
    N3Config config;
    config.strict_mode = true;
    return config;
  }

  // Create permissive configuration (large limits, lenient)
  static N3Config permissiveConfig() {
    N3Config config;
    config.max_input_size_bytes = 1024 * 1024 * 1024;  // 1GB
    config.max_blank_nodes = 10'000'000;               // 10M
    config.max_nesting_depth = 1000;                   // 1000
    config.max_memory_bytes = 2 * 1024 * 1024 * 1024;  // 2GB
    config.timeout_ms = 300'000;                       // 5 minutes
    config.strict_mode = false;
    return config;
  }

  // Create minimal configuration (small limits for testing)
  static N3Config minimalConfig() {
    N3Config config;
    config.max_input_size_bytes = 1024 * 1024;  // 1MB
    config.max_blank_nodes = 1000;
    config.max_nesting_depth = 10;
    config.max_memory_bytes = 2 * 1024 * 1024;  // 2MB
    config.timeout_ms = 5'000;                  // 5 seconds
    return config;
  }

  // Validate configuration parameters
  bool isValid() const {
    return max_input_size_bytes > 0 && max_blank_nodes > 0 &&
           max_nesting_depth > 0 && max_memory_bytes > 0 &&
           max_memory_bytes >= max_input_size_bytes;
  }
};

}  // namespace ad_engine::n3

#endif  // QLEVER_SRC_ENGINE_N3_N3CONFIG_H

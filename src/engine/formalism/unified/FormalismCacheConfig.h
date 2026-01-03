// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 4 (EPIC 14.0 - Formalism Delta Discovery)
//
// Purpose: Formalism-specific cache configuration presets
//
// This header provides pre-configured cache settings optimized for each
// formalism type (SHACL, Datalog, N3, ShEx).

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_FORMALISMCACHECONFIG_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_FORMALISMCACHECONFIG_H

#include "UnifiedFormalismCache.h"

namespace formalism {

// ============================================================================
// SHACL-Optimized Configuration
// ============================================================================

// SHACL validation typically has:
// - Moderate number of shapes (10-100)
// - Large number of resources to validate (10K-1M)
// - High constraint reuse (same constraints checked many times)
// - Moderate result size (validation reports with violations)
//
// Optimization strategy:
// - Large validation cache (constraint reuse)
// - Large negative cache (most resources don't match most shapes)
// - Moderate result cache (validation reports)
inline CacheConfiguration shaclOptimizedConfig() {
  CacheConfiguration config;

  // Result cache: validation reports
  config.resultCacheSize = 10000;  // 10K reports
  config.maxResultEntrySizeBytes = 5 * 1024 * 1024;  // 5 MB max per report

  // Validation cache: constraint evaluations
  config.validationCacheSize = 100000;  // 100K constraint checks
  config.maxValidationEntrySizeBytes = 1024 * 1024;  // 1 MB max

  // Negative cache: resource-shape mismatches
  config.negativeCacheSize = 50000;  // 50K negative lookups
  config.enableBloomFilter = true;  // Essential for SHACL

  // Performance tuning
  config.enableLRU = true;
  config.evictionThreshold = 0.9;
  config.numShards = 4;
  config.enableMetrics = true;

  // Epoch lifecycle
  config.autoInvalidateOnEpochChange = true;
  config.enableCheckpointing = false;  // Not yet implemented

  return config;
}

// ============================================================================
// Datalog-Optimized Configuration
// ============================================================================

// Datalog evaluation typically has:
// - Small number of rules (1-50)
// - Complex fixpoint computation (expensive)
// - Large intermediate results (IdTables)
// - High result reuse (same queries repeated)
//
// Optimization strategy:
// - Large result cache (fixpoint results are expensive)
// - Smaller validation cache (less intermediate validation)
// - Smaller negative cache (fewer negative lookups)
inline CacheConfiguration datalogOptimizedConfig() {
  CacheConfiguration config;

  // Result cache: fixpoint computation results
  config.resultCacheSize = 50000;  // 50K query results
  config.maxResultEntrySizeBytes = 20 * 1024 * 1024;  // 20 MB max (large IdTables)

  // Validation cache: rule applicability checks
  config.validationCacheSize = 10000;  // 10K rule checks
  config.maxValidationEntrySizeBytes = 256 * 1024;  // 256 KB max

  // Negative cache: inapplicable rules
  config.negativeCacheSize = 5000;  // 5K negative lookups
  config.enableBloomFilter = true;

  // Performance tuning
  config.enableLRU = true;
  config.evictionThreshold = 0.85;  // Evict earlier (large entries)
  config.numShards = 8;  // More shards (higher concurrency)
  config.enableMetrics = true;

  // Epoch lifecycle
  config.autoInvalidateOnEpochChange = true;
  config.enableCheckpointing = false;

  return config;
}

// ============================================================================
// N3-Optimized Configuration
// ============================================================================

// N3 parsing typically has:
// - Parse-only (no evaluation kernel yet)
// - Feature detection (formulae, implications, etc.)
// - Small result size (compliance reports)
// - High validation cache reuse (same features checked repeatedly)
//
// Optimization strategy:
// - Small result cache (compliance reports are small)
// - Large validation cache (feature detection is common)
// - Moderate negative cache (unsupported features)
inline CacheConfiguration n3OptimizedConfig() {
  CacheConfiguration config;

  // Result cache: compliance reports
  config.resultCacheSize = 1000;  // 1K reports (small)
  config.maxResultEntrySizeBytes = 1024 * 1024;  // 1 MB max

  // Validation cache: feature checks
  config.validationCacheSize = 50000;  // 50K feature checks
  config.maxValidationEntrySizeBytes = 256 * 1024;  // 256 KB max

  // Negative cache: unsupported features
  config.negativeCacheSize = 10000;  // 10K negative lookups
  config.enableBloomFilter = true;

  // Performance tuning
  config.enableLRU = true;
  config.evictionThreshold = 0.9;
  config.numShards = 2;  // Fewer shards (lower concurrency, parse-only)
  config.enableMetrics = true;

  // Epoch lifecycle
  config.autoInvalidateOnEpochChange = true;
  config.enableCheckpointing = false;

  return config;
}

// ============================================================================
// ShEx-Optimized Configuration
// ============================================================================

// ShEx is currently stub-only, but anticipated characteristics:
// - Similar to SHACL (shape-based validation)
// - Potentially simpler constraints (less intermediate caching)
// - Moderate result size (validation reports)
//
// Optimization strategy:
// - Balanced configuration (between SHACL and N3)
// - Can be tuned after implementation
inline CacheConfiguration shexOptimizedConfig() {
  CacheConfiguration config;

  // Result cache: validation reports
  config.resultCacheSize = 5000;  // 5K reports
  config.maxResultEntrySizeBytes = 2 * 1024 * 1024;  // 2 MB max

  // Validation cache: constraint checks
  config.validationCacheSize = 25000;  // 25K checks
  config.maxValidationEntrySizeBytes = 512 * 1024;  // 512 KB max

  // Negative cache: shape mismatches
  config.negativeCacheSize = 10000;  // 10K negative lookups
  config.enableBloomFilter = true;

  // Performance tuning
  config.enableLRU = true;
  config.evictionThreshold = 0.9;
  config.numShards = 4;
  config.enableMetrics = true;

  // Epoch lifecycle
  config.autoInvalidateOnEpochChange = true;
  config.enableCheckpointing = false;

  return config;
}

// ============================================================================
// Environment-Based Configurations
// ============================================================================

// Low-memory environment (embedded systems, CI/CD)
inline CacheConfiguration lowMemoryConfig(const std::string& formalism = "") {
  CacheConfiguration config;

  // Reduce all cache sizes by 10x
  if (formalism == "shacl") {
    config = shaclOptimizedConfig();
  } else if (formalism == "datalog") {
    config = datalogOptimizedConfig();
  } else if (formalism == "n3") {
    config = n3OptimizedConfig();
  } else if (formalism == "shex") {
    config = shexOptimizedConfig();
  } else {
    config = CacheConfiguration::defaultConfig();
  }

  config.resultCacheSize /= 10;
  config.validationCacheSize /= 10;
  config.negativeCacheSize /= 10;
  config.maxResultEntrySizeBytes /= 10;
  config.maxValidationEntrySizeBytes /= 10;

  return config;
}

// High-throughput environment (production servers, benchmarking)
inline CacheConfiguration highThroughputConfig(
    const std::string& formalism = "") {
  CacheConfiguration config;

  // Increase all cache sizes by 5x
  if (formalism == "shacl") {
    config = shaclOptimizedConfig();
  } else if (formalism == "datalog") {
    config = datalogOptimizedConfig();
  } else if (formalism == "n3") {
    config = n3OptimizedConfig();
  } else if (formalism == "shex") {
    config = shexOptimizedConfig();
  } else {
    config = CacheConfiguration::defaultConfig();
  }

  config.resultCacheSize *= 5;
  config.validationCacheSize *= 5;
  config.negativeCacheSize *= 5;
  config.numShards *= 2;  // More shards for higher concurrency

  return config;
}

// ============================================================================
// Configuration Selection Helper
// ============================================================================

// Select configuration based on formalism type and environment
inline CacheConfiguration selectConfiguration(
    const std::string& formalism,
    const std::string& environment = "default") {
  CacheConfiguration config;

  // Select formalism-specific config
  if (formalism == "shacl") {
    config = shaclOptimizedConfig();
  } else if (formalism == "datalog") {
    config = datalogOptimizedConfig();
  } else if (formalism == "n3") {
    config = n3OptimizedConfig();
  } else if (formalism == "shex") {
    config = shexOptimizedConfig();
  } else {
    config = CacheConfiguration::defaultConfig();
  }

  // Adjust for environment
  if (environment == "low_memory") {
    return lowMemoryConfig(formalism);
  } else if (environment == "high_throughput") {
    return highThroughputConfig(formalism);
  }

  return config;
}

}  // namespace formalism

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_FORMALISMCACHECONFIG_H

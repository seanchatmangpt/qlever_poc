// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Metrics export for read cache monitoring (EPIC 3)
// Provides JSON and Prometheus-format exports of cache metrics for external
// monitoring systems.

#ifndef QLEVER_SRC_ENGINE_READCACHE_METRICSEXPORT_H
#define QLEVER_SRC_ENGINE_READCACHE_METRICSEXPORT_H

#include <string>

namespace readCache {

// Export read cache metrics in JSON format
// Returns a JSON string with all cache metrics including:
// - Bytes cache stats (hits, misses, inserts, evicts, served_bytes)
// - Plan cache stats
// - Negative cache stats
// - Inflight waiters
// - Per-shape latency statistics (count, avg, p50, p95, p99, min, max)
// - Epoch management stats (promotes, prewarm duration)
std::string exportMetricsJson();

// Export read cache metrics in Prometheus format
// Returns a Prometheus-compatible text format with:
// - Counter metrics (hits, misses, inserts, evicts)
// - Gauge metrics (inflight_waiters)
// - Histogram metrics (latency percentiles)
// Format follows: https://prometheus.io/docs/instrumenting/exposition_formats/
std::string exportMetricsPrometheus();

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_METRICSEXPORT_H

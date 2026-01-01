// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant

#include "engine/readCache/MetricsExport.h"

#include <sstream>

#include "global/EpochMetrics.h"

namespace readCache {

std::string exportMetricsJson() {
  // Get current metrics snapshot
  auto metrics = ad_utility::globalReadCacheMetrics.withReadLock(
      [](const auto& collector) { return collector.getMetrics(); });

  // Use the built-in JSON export from ReadCacheMetrics
  return metrics.toJSON();
}

std::string exportMetricsPrometheus() {
  // Get current metrics snapshot
  auto metrics = ad_utility::globalReadCacheMetrics.withReadLock(
      [](const auto& collector) { return collector.getMetrics(); });

  std::ostringstream oss;

  // Bytes cache metrics
  oss << "# HELP qlever_read_cache_bytes_hits_total Total number of bytes "
         "cache hits\n";
  oss << "# TYPE qlever_read_cache_bytes_hits_total counter\n";
  oss << "qlever_read_cache_bytes_hits_total " << metrics.bytes_hits << "\n\n";

  oss << "# HELP qlever_read_cache_bytes_misses_total Total number of bytes "
         "cache misses\n";
  oss << "# TYPE qlever_read_cache_bytes_misses_total counter\n";
  oss << "qlever_read_cache_bytes_misses_total " << metrics.bytes_misses
      << "\n\n";

  oss << "# HELP qlever_read_cache_bytes_inserts_total Total number of bytes "
         "cache inserts\n";
  oss << "# TYPE qlever_read_cache_bytes_inserts_total counter\n";
  oss << "qlever_read_cache_bytes_inserts_total " << metrics.bytes_inserts
      << "\n\n";

  oss << "# HELP qlever_read_cache_bytes_evicts_total Total number of bytes "
         "cache evictions\n";
  oss << "# TYPE qlever_read_cache_bytes_evicts_total counter\n";
  oss << "qlever_read_cache_bytes_evicts_total " << metrics.bytes_evicts
      << "\n\n";

  oss << "# HELP qlever_read_cache_bytes_served_total Total bytes served from "
         "cache\n";
  oss << "# TYPE qlever_read_cache_bytes_served_total counter\n";
  oss << "qlever_read_cache_bytes_served_total "
      << metrics.bytes_served_from_cache << "\n\n";

  // Plan cache metrics
  oss << "# HELP qlever_read_cache_plan_hits_total Total number of plan cache "
         "hits\n";
  oss << "# TYPE qlever_read_cache_plan_hits_total counter\n";
  oss << "qlever_read_cache_plan_hits_total " << metrics.plan_hits << "\n\n";

  oss << "# HELP qlever_read_cache_plan_misses_total Total number of plan "
         "cache misses\n";
  oss << "# TYPE qlever_read_cache_plan_misses_total counter\n";
  oss << "qlever_read_cache_plan_misses_total " << metrics.plan_misses
      << "\n\n";

  oss << "# HELP qlever_read_cache_plan_inserts_total Total number of plan "
         "cache inserts\n";
  oss << "# TYPE qlever_read_cache_plan_inserts_total counter\n";
  oss << "qlever_read_cache_plan_inserts_total " << metrics.plan_inserts
      << "\n\n";

  oss << "# HELP qlever_read_cache_plan_evicts_total Total number of plan "
         "cache evictions\n";
  oss << "# TYPE qlever_read_cache_plan_evicts_total counter\n";
  oss << "qlever_read_cache_plan_evicts_total " << metrics.plan_evicts
      << "\n\n";

  // Negative cache metrics
  oss << "# HELP qlever_read_cache_negative_hits_total Total number of "
         "negative cache hits\n";
  oss << "# TYPE qlever_read_cache_negative_hits_total counter\n";
  oss << "qlever_read_cache_negative_hits_total " << metrics.neg_hits << "\n\n";

  oss << "# HELP qlever_read_cache_negative_inserts_total Total number of "
         "negative cache inserts\n";
  oss << "# TYPE qlever_read_cache_negative_inserts_total counter\n";
  oss << "qlever_read_cache_negative_inserts_total " << metrics.neg_inserts
      << "\n\n";

  // Inflight gauge
  oss << "# HELP qlever_read_cache_inflight_waiters Current number of "
         "inflight waiters\n";
  oss << "# TYPE qlever_read_cache_inflight_waiters gauge\n";
  oss << "qlever_read_cache_inflight_waiters " << metrics.inflight_waiters
      << "\n\n";

  // Epoch metrics
  oss << "# HELP qlever_read_cache_epoch_promotes_total Total number of epoch "
         "promotes\n";
  oss << "# TYPE qlever_read_cache_epoch_promotes_total counter\n";
  oss << "qlever_read_cache_epoch_promotes_total "
      << metrics.epoch_promote_count << "\n\n";

  oss << "# HELP qlever_read_cache_prewarm_duration_ms Duration of last "
         "prewarm in milliseconds\n";
  oss << "# TYPE qlever_read_cache_prewarm_duration_ms gauge\n";
  oss << "qlever_read_cache_prewarm_duration_ms "
      << metrics.prewarm_duration_ms << "\n\n";

  // Per-shape latency metrics (aggregate across all shapes for now)
  // In a production system, you might want to export only top-K shapes
  uint64_t total_queries = 0;
  uint64_t total_latency_sum = 0;

  for (const auto& [shape_sha256, stats] : metrics.shape_latency) {
    total_queries += stats.count_;
    total_latency_sum += stats.sum_us_;
  }

  oss << "# HELP qlever_read_cache_query_count_total Total number of queries "
         "executed\n";
  oss << "# TYPE qlever_read_cache_query_count_total counter\n";
  oss << "qlever_read_cache_query_count_total " << total_queries << "\n\n";

  if (total_queries > 0) {
    uint64_t avg_latency = total_latency_sum / total_queries;
    oss << "# HELP qlever_read_cache_query_latency_avg_us Average query "
           "latency in microseconds\n";
    oss << "# TYPE qlever_read_cache_query_latency_avg_us gauge\n";
    oss << "qlever_read_cache_query_latency_avg_us " << avg_latency << "\n\n";
  }

  // Hit rate calculations
  uint64_t bytes_total = metrics.bytes_hits + metrics.bytes_misses;
  if (bytes_total > 0) {
    double bytes_hit_rate =
        static_cast<double>(metrics.bytes_hits) / bytes_total;
    oss << "# HELP qlever_read_cache_bytes_hit_rate Bytes cache hit rate "
           "(0.0-1.0)\n";
    oss << "# TYPE qlever_read_cache_bytes_hit_rate gauge\n";
    oss << "qlever_read_cache_bytes_hit_rate " << bytes_hit_rate << "\n\n";
  }

  uint64_t plan_total = metrics.plan_hits + metrics.plan_misses;
  if (plan_total > 0) {
    double plan_hit_rate = static_cast<double>(metrics.plan_hits) / plan_total;
    oss << "# HELP qlever_read_cache_plan_hit_rate Plan cache hit rate "
           "(0.0-1.0)\n";
    oss << "# TYPE qlever_read_cache_plan_hit_rate gauge\n";
    oss << "qlever_read_cache_plan_hit_rate " << plan_hit_rate << "\n\n";
  }

  return oss.str();
}

}  // namespace readCache

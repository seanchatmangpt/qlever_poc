# Cache Metrics Quick Reference

## Import Headers

```cpp
#include "engine/readCache/CacheMetrics.h"       // For recording metrics
#include "engine/readCache/MetricsExport.h"      // For exporting metrics
#include "global/EpochMetrics.h"                 // For accessing global metrics
```

## Recording Cache Operations

### Bytes Cache
```cpp
// Cache hit (with bytes served)
readCache::CacheMetrics::recordBytesHit(1024);

// Cache miss
readCache::CacheMetrics::recordBytesMiss();

// Insert
readCache::CacheMetrics::recordBytesInsert(2048);

// Evict
readCache::CacheMetrics::recordBytesEvict(512);
```

### Plan Cache
```cpp
readCache::CacheMetrics::recordPlanHit();
readCache::CacheMetrics::recordPlanMiss();
readCache::CacheMetrics::recordPlanInsert();
readCache::CacheMetrics::recordPlanEvict();
```

### Negative Cache
```cpp
readCache::CacheMetrics::recordNegativeHit();
readCache::CacheMetrics::recordNegativeInsert();
```

### Inflight Tracking
```cpp
// When starting async operation
readCache::CacheMetrics::recordInflightWaiterAdd();

// When completing async operation
readCache::CacheMetrics::recordInflightWaiterRemove();
```

### Latency Tracking
```cpp
auto start = std::chrono::steady_clock::now();
// ... execute query ...
auto end = std::chrono::steady_clock::now();
auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

readCache::CacheMetrics::recordShapeLatency(shape_sha256, elapsed_us);
```

### Epoch Management
```cpp
auto start = std::chrono::steady_clock::now();
// ... prewarm cache ...
auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start).count();

readCache::CacheMetrics::recordEpochPromote(duration_ms);
```

## Logging (Optional)

### Sampled Logging (1-in-100)
```cpp
// Log cache hit (DEBUG level, sampled)
readCache::CacheMetrics::logCacheHit("bytes", "key_abc123...");

// Log cache miss (DEBUG level, sampled)
readCache::CacheMetrics::logCacheMiss("plan", "key_xyz789...");

// Log admission decision (DEBUG level, sampled)
readCache::CacheMetrics::logAdmissionDecision("negative", true, "query failed");
```

### Always Logged (INFO level)
```cpp
// Log epoch promote
readCache::CacheMetrics::logEpochPromote(12345);

// Log prewarm completion
readCache::CacheMetrics::logPrewarmComplete(5678, 100);
```

## Exporting Metrics

### JSON Export
```cpp
std::string json = readCache::exportMetricsJson();
std::cout << json << std::endl;
```

### Prometheus Export
```cpp
std::string prom = readCache::exportMetricsPrometheus();
// Send to Prometheus scraper
```

### Detailed Report
```cpp
auto report = ad_utility::globalReadCacheMetrics.withReadLock(
    [](const auto& collector) {
        return collector.getDetailedMetricsReport();
    });
std::cout << report << std::endl;
```

## Common Patterns

### Pattern 1: Cache Lookup with Metrics
```cpp
auto result = cache.lookup(key);
if (result) {
    readCache::CacheMetrics::recordBytesHit(result->size());
    readCache::CacheMetrics::logCacheHit("bytes", key.preview());
    return *result;
} else {
    readCache::CacheMetrics::recordBytesMiss();
    readCache::CacheMetrics::logCacheMiss("bytes", key.preview());
    return std::nullopt;
}
```

### Pattern 2: Cache Insert with Admission
```cpp
bool admitted = admissionPolicy.shouldAdmit(entry);
if (admitted) {
    cache.insert(key, value);
    readCache::CacheMetrics::recordBytesInsert(value.size());
    readCache::CacheMetrics::logAdmissionDecision("bytes", true, "passed policy");
} else {
    readCache::CacheMetrics::logAdmissionDecision("bytes", false, "rejected: too large");
}
```

### Pattern 3: Timed Query Execution
```cpp
class ScopedLatencyTracker {
    std::string shape_;
    std::chrono::steady_clock::time_point start_;

public:
    explicit ScopedLatencyTracker(std::string shape)
        : shape_(std::move(shape)), start_(std::chrono::steady_clock::now()) {}

    ~ScopedLatencyTracker() {
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start_).count();
        readCache::CacheMetrics::recordShapeLatency(shape_, elapsed_us);
    }
};

// Usage:
void executeQuery(const std::string& shape) {
    ScopedLatencyTracker tracker(shape);
    // ... query execution ...
}
```

### Pattern 4: Inflight Tracking with RAII
```cpp
class InflightGuard {
public:
    InflightGuard() {
        readCache::CacheMetrics::recordInflightWaiterAdd();
    }
    ~InflightGuard() {
        readCache::CacheMetrics::recordInflightWaiterRemove();
    }
};

// Usage:
std::future<Result> asyncOperation() {
    InflightGuard guard;  // Automatically tracks inflight count
    // ... async work ...
}
```

## Thread Safety

All metric operations are thread-safe and can be called from multiple threads concurrently:
- Atomic counters for increments
- Mutex-protected map for per-shape latency
- Lock-free sampling for logging

## Performance

- Metric recording: ~50-100ns overhead per call
- Latency tracking: ~200ns overhead
- Logging (sampled): ~10ns amortized
- No significant impact on cache performance

## Testing

Run metrics tests:
```bash
cd build
ctest -R CacheMetricsTest --output-on-failure
```

## Monitoring Integration

### Prometheus Endpoint
```cpp
// In your HTTP server
app.get("/metrics", [](auto& req, auto& res) {
    res.set_content(readCache::exportMetricsPrometheus(), "text/plain");
});
```

### JSON API Endpoint
```cpp
app.get("/api/cache/metrics", [](auto& req, auto& res) {
    res.set_content(readCache::exportMetricsJson(), "application/json");
});
```

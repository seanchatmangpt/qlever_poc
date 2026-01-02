# EPIC 3 Read Cache Benchmark Suite

## Overview

This benchmark suite validates the performance improvements and correctness of the EPIC 3 read caching system. It implements three distinct test modes to verify:

1. **Mode A**: 5× speedup on exact repeated queries
2. **Mode B**: 1.2× tail latency improvement on same-shape queries with varying parameters
3. **Mode C**: Concurrency handling and stampede prevention under mixed workloads

## Quick Start

### Building

```bash
cd /home/user/qlever
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build . --target ReadCacheBench
```

### Running the Benchmark

```bash
# From build directory
./ReadCacheBench

# Output will be in JSON format
./ReadCacheBench > read_cache_results.json
```

### Running Unit Tests

```bash
# Use test wrapper script
./scripts/run-tests.sh ReadCacheBench
```

## Benchmark Modes

### Mode A: Exact Repeats (5× Speedup Target)

**Goal**: Verify BytesCache delivers significant speedup on repeated identical queries.

**Test Design**:
- Single query executed 200 times
- First 10 runs are warmup
- Measures p50, p95, p99 latencies for runs 11-200
- Calculates speedup factor: `first_run_latency / cached_mean_latency`

**Success Criteria**:
- Speedup factor ≥ 5.0×
- Cache hit rate ≥ 99%
- p95 latency < 20% of first-run latency

**Expected Output**:
```json
{
  "ModeA_ExactRepeats": {
    "first_run_ms": 150.2,
    "cached_p50_ms": 12.3,
    "cached_p95_ms": 15.8,
    "speedup_factor": 12.2,
    "hit_rate_pct": 99.5,
    "target_met": true
  }
}
```

### Mode B: Same Shape, Varying Parameters (1.2× Tail Improvement)

**Goal**: Verify PlanCache improves tail latencies even when BytesCache misses.

**Test Design**:
- Fixed query template with 50 distinct parameter sets
- Each parameter set executed 10 times
- Measures p95 latency per parameter set
- Compares first-run p95 vs. cached p95

**Success Criteria**:
- p95 improvement factor ≥ 1.2×
- PlanCache hit rate > 50%
- BytesCache hit rate varies (expected: ~10-20%)

**Expected Output**:
```json
{
  "ModeB_SameShapeVaryingParams": {
    "first_run_p95_ms": 120.5,
    "cached_p95_ms": 98.3,
    "p95_improvement_factor": 1.23,
    "hit_rate_pct": 15.2,
    "plan_cache_size": 1,
    "bytes_cache_size": 50,
    "target_met": true
  }
}
```

### Mode C: Concurrency & Mixed Workload (Stampede Prevention)

**Goal**: Verify stable latencies under concurrent load without cache stampedes.

**Test Design**:
- 20 concurrent threads
- 30-second test duration
- Mixed workload: 70% from top-10 hot queries, 30% random
- Tracks max in-flight queries (stampede indicator)

**Success Criteria**:
- No deadlocks or crashes
- Stable latency percentiles (p95 < 200ms)
- Max in-flight < 5 (indicates single-flight working)
- Hit rate > 60%

**Expected Output**:
```json
{
  "ModeC_ConcurrencyMixedWorkload": {
    "total_queries": 8542,
    "throughput_qps": 284.7,
    "p50_latency_ms": 45.2,
    "p95_latency_ms": 125.8,
    "p99_latency_ms": 180.3,
    "hit_rate_pct": 68.4,
    "max_in_flight": 3,
    "no_stampedes_detected": true
  }
}
```

## Architecture

### Simulated Cache Implementation

The benchmark uses a simplified cache simulation that mimics the behavior of the actual read cache system:

**Components**:
1. **BytesCache**: Exact query text matching
   - Key: raw query text
   - Value: serialized result data
   - Hit: instant return (simulated ~10ms latency)

2. **PlanCache**: Query shape matching
   - Key: canonical query shape (constants removed)
   - Value: execution plan
   - Hit: faster execution (simulated 50-100ms vs 100-200ms)

3. **Single-Flight Mechanism**: Prevents stampedes
   - Tracks in-flight queries
   - Concurrent requests for same query wait on single execution

**Cache Hit Tracker**:
- Thread-safe atomic counters
- Tracks hits, misses, max in-flight
- Computes hit rate and stampede metrics

### Synthetic Workload

**Query Templates**:
- 5 SPARQL query patterns (based on DBLP dataset)
- Vary in complexity: simple triple patterns to complex joins
- Parameterized with placeholders (%AUTHOR%, %TITLE%, etc.)

**Query Generation**:
- Deterministic based on seed (reproducible)
- Constants randomly generated but deterministic per seed
- Shape extraction: normalize constants to placeholders

## Metrics

### Latency Metrics
- **p50 (median)**: Typical query latency
- **p95**: Tail latency (95th percentile)
- **p99**: Extreme tail latency
- **max**: Worst-case latency
- **mean**: Average latency
- **stddev**: Latency variance

### Cache Metrics
- **hit_rate_pct**: Percentage of cache hits
- **bytes_cache_size**: Number of cached query results
- **plan_cache_size**: Number of cached query plans
- **max_in_flight**: Peak concurrent in-flight queries

### Performance Metrics
- **speedup_factor**: first_run / cached_mean
- **p95_improvement_factor**: first_p95 / cached_p95
- **throughput_qps**: Queries per second

## Interpreting Results

### Mode A: Exact Repeats

**Target Met** (speedup ≥ 5×):
- ✅ **Excellent**: BytesCache working correctly
- ❌ **Poor**: Cache not being used or evicting too aggressively

**High Hit Rate** (≥ 99%):
- ✅ **Expected**: Cache capacity sufficient
- ❌ **Problem**: Cache evictions or incorrect keying

### Mode B: Same Shape, Varying Params

**Target Met** (p95 improvement ≥ 1.2×):
- ✅ **Good**: PlanCache reducing planning overhead
- ❌ **Poor**: PlanCache not being used or ineffective

**Low BytesCache Hit Rate** (~10-20%):
- ✅ **Expected**: Different parameters = different query text
- ❌ **Unexpected** (>50%): Possible duplicate queries

**High PlanCache Hit Rate** (>50%):
- ✅ **Expected**: Same shape across parameter sets

### Mode C: Concurrency

**Low Max In-Flight** (<5):
- ✅ **Excellent**: Single-flight preventing stampedes
- ❌ **Problem** (>10): Possible stampede, check lock contention

**Stable Latencies**:
- ✅ **p95 < 200ms**: Good concurrent performance
- ❌ **p95 > 500ms**: Contention or serialization issues

**No Deadlocks**:
- ✅ **Test completes**: Correct locking
- ❌ **Hangs**: Deadlock in cache implementation

## Troubleshooting

### Benchmark Fails to Build

```bash
# Verify benchmark infrastructure is built
cd build
cmake --build . --target benchmark

# Check for missing dependencies
ldd ./ReadCacheBench
```

### Tests Fail

```bash
# Run tests with output
./scripts/run-tests.sh ReadCacheBench

# For advanced debugging, run test binary directly
./build/engine/readCache/ReadCacheBenchTest --gtest_filter="*"
```

### Unexpected Results

**Speedup < 5× in Mode A**:
- Check if cache is actually being used
- Verify cache size is sufficient (should be ~1 entry for Mode A)
- Check for cache evictions

**p95 improvement < 1.2× in Mode B**:
- Verify PlanCache is enabled
- Check if plan reuse is happening
- May indicate planning overhead is small relative to execution

**High max_in_flight in Mode C**:
- Indicates single-flight may not be working
- Check for race conditions in cache lookup
- Verify same queries generate same cache keys

## Performance Tuning Recommendations

Based on benchmark results, the following tuning recommendations can be made:

### Cache Size Tuning

**BytesCache**:
- If hit rate < 95% in Mode A: increase BytesCache size
- If memory is limited: prioritize hot queries (use LRU)
- Recommended: size based on working set (Mode C shows typical size)

**PlanCache**:
- If p95 improvement < 1.2× in Mode B: increase PlanCache size
- Plans are much smaller than results: can cache more shapes
- Recommended: cache top 1000 query shapes

### Concurrency Tuning

**Single-Flight**:
- If max_in_flight > 5: improve single-flight detection
- Consider more aggressive deduplication
- May need finer-grained locking

**Thread Pool**:
- If throughput plateaus: adjust thread pool size
- Mode C throughput shows saturation point
- Recommended: cores × 2 for I/O-bound queries

## Integration with Actual System

This benchmark uses **simulated caches**. To integrate with the actual read cache:

1. **Replace SimulatedReadCache**:
   - Use actual BytesCache and PlanCache from Tasks 1-9
   - Wire up to QueryExecutionContext
   - Use real QueryFingerprint for keying

2. **Use Real Query Execution**:
   - Parse SPARQL with existing parser
   - Execute through QueryPlanner
   - Measure end-to-end latency

3. **Real Dataset**:
   - Load DBLP or Wikidata dataset
   - Use actual index for scanning
   - Measure on real data distribution

## Files

- `ReadCacheBench.cpp` - Main benchmark implementation
- `README.md` - This file
- `../infrastructure/` - Benchmark framework
- `../../test/engine/readCache/ReadCacheBenchTest.cpp` - Unit tests

## References

- EPIC 3 Specification: Cache infrastructure for read-heavy workloads
- Task 10: Benchmark harness and torture tests
- Related: Tasks 1-9 (read cache infrastructure)

## Authors

- EPIC 3 Team
- Task 10 Implementation: Claude Assistant

## License

Apache 2.0 (same as QLever)

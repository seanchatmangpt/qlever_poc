# QLever WASM Performance Test Plan
## Real-World JavaScript Integration Performance

**Goal**: Measure actual end-to-end performance from JavaScript to WASM to QLever C++ and back with results.

**Key Principle**: All measurements include **JavaScript marshaling, WASM processing, and result serialization/deserialization**. This is how the WASM binding is actually used.

**Baseline**: QLever C++ native performance (from official benchmarks on 390M DBLP triples):
- Simple queries: 0.02s (50 q/s)
- Filtered queries: 0.05s (20 q/s)
- Complex joins: 0.11s (9 q/s)
- Large results (7.2M triples): 4.2s

**Target**: 80% of native C++ performance (20% overhead for WASM FFI and serialization)

---

## Core Metrics We Care About

| Metric | QLever C++ | WASM Target (80%) | Status |
|--------|-----------|-------------------|--------|
| Simple Query Throughput | 50 q/s | 32+ q/s | Test: throughput.perf.ts |
| Filtered Query Throughput | 20 q/s | 16+ q/s | Test: throughput.perf.ts |
| Complex Query Throughput | 9 q/s | 6+ q/s | Test: throughput.perf.ts |
| P50 Latency (small results) | 20ms | 31ms | Test: latency.perf.ts |
| P99 Latency (interactive) | 250ms | 500ms | Test: latency.perf.ts |
| Large Result Throughput | 1.7M results/s | 50K+ results/s | Test: throughput.perf.ts |
| Memory Growth (100 queries) | baseline | < 5MB | Test: memory.perf.ts |
| Concurrent Scaling | Linear | Linear up to 10 | Test: concurrency.perf.ts |
| Memory Leak (500 queries) | None | < 2MB | Test: memory.perf.ts |

---

## Test Suite 1: Throughput Performance
**File**: `tests/throughput.perf.ts`

**What It Measures**: Queries per second for different query complexities.

**WASM Warmup**: 10 warmup queries before timing starts.

**End-to-End Measurement**:
- Start timer before `client.query()` call
- Include WASM processing time
- Include result serialization/deserialization
- Stop timer after `response.data()` completes

**Tests**:
1. **Simple triple pattern** - Target: 32+ q/s
2. **Filtered patterns** - Target: 16+ q/s
3. **Multi-pattern joins** - Target: 11+ q/s
4. **GROUP BY aggregation** - Target: 6+ q/s
5. **UNION patterns** - Target: 8+ q/s
6. **Sustained load (1000 queries)** - Target: 28+ q/s sustained
7. **Mixed complexity** - Target: 20+ q/s
8. **Large result throughput (500K+ results)** - Target: 50K+ results/second

**Key Insight**: Large result set throughput measures how fast WASM can serialize and transfer massive result sets back to JavaScript.

---

## Test Suite 2: Latency Distribution
**File**: `tests/latency.perf.ts`

**What It Measures**: Response time percentiles (P50, P95, P99) for different result sizes.

**WASM Warmup**: 10 warmup queries before timing starts.

**Latency Measurements** (end-to-end from JavaScript):
1. **Tiny results (1-10)** - P50: 31ms, P95: 63ms, P99: 125ms
2. **Small results (10-100)** - P50: 31ms, P95: 63ms, P99: 125ms
3. **Medium results (100-1000)** - P50: 63ms, P95: 125ms, P99: 250ms
4. **Large results (1000+)** - P50: 138ms, P95: 275ms, P99: 550ms
5. **Interactive queries** - P50: <100ms, P95: <250ms, P99: <500ms
6. **Distribution analysis** - Skewness, tail ratio, normality check
7. **Percentile sweep** - P10 through P99 detailed breakdown

**Key Metric**: Interactive queries should complete in <500ms P99 for good user experience.

---

## Test Suite 3: Memory Efficiency
**File**: `tests/memory.perf.ts`

**What It Measures**: Heap memory growth, leak detection, and memory scaling with result size.

**WASM Warmup**: 5 warmup queries before measurements.

**Memory Tests**:
1. **Baseline heap** - Establish initial memory footprint
2. **Single query impact** - Memory delta for one query
3. **Stability (100 queries)** - Growth: <5MB, Leak: <1MB
4. **Memory scaling** - With result sizes 10 to 10,000
5. **Extended session (500 queries)** - Growth: <10MB, Leak: <2MB
6. **Result retention** - Verify garbage collection
7. **Concurrent queries** - 10 parallel: <20MB delta
8. **WASM module footprint** - <300MB heap for runtime

**Key Goal**: No memory leaks detected over extended session, minimal per-query overhead.

---

## Test Suite 4: Concurrency & Parallelism
**File**: `tests/concurrency.perf.ts`

**What It Measures**: Scaling behavior with multiple concurrent queries.

**WASM Warmup**: 20 concurrent warmup queries.

**Concurrency Tests**:
1. **Sequential baseline** - 1 client: 20+ q/s
2. **Dual concurrent** - 2 clients: 30+ q/s
3. **Multiple concurrent** - 5 clients: 40+ q/s
4. **High concurrency** - 10 clients: 60+ q/s
5. **Extreme concurrency** - 20 clients: 50+ q/s
6. **Mixed complexity** - 70% simple, 30% complex: 20+ q/s
7. **Sustained 30s load** - 5 concurrent: 20+ q/s sustained
8. **Scalability analysis** - Efficiency across 1,2,4,8 concurrent clients

**Key Insight**: System should scale linearly up to saturation point (around 10 concurrent), then plateau.

---

---

## Running the Test Suite

All tests are written using **Vitest** and measure end-to-end JavaScript performance.

**Key Setup**:
- WASM module is warmed up before each test suite
- Tests require QLever server running at `http://localhost:7023` (or `QLEVER_ENDPOINT` env var)
- All timings include marshaling, processing, and serialization

**Running Tests**:
```bash
# Run all performance tests
npm test

# Run specific test file
npm test throughput.perf.ts
npm test latency.perf.ts
npm test memory.perf.ts
npm test concurrency.perf.ts

# With custom endpoint
QLEVER_ENDPOINT=http://custom-host:7023 npm test

# Watch mode
npm run test:watch
```

---

## Running Tests

```bash
# All performance tests
npm run perf

# Individual tests
npm run perf:throughput      # Raw query throughput
npm run perf:latency         # Latency percentiles
npm run perf:memory          # Memory profiling
npm run perf:concurrent      # Concurrent operations
npm run perf:complexity      # Query complexity scaling
npm run perf:bundle          # Bundle size & load time
npm run perf:stress          # Saturation testing
npm run perf:realistic       # Real-world scenario
npm run perf:baseline        # Comparison with alternatives

# Continuous profiling
npm run perf:profile         # CPU/memory profiling output

# Generate report
npm run perf:report          # HTML report with charts
```

---

## Acceptance Criteria

**PASS** if:
- [ ] Throughput ≥ 100 q/s (simple queries)
- [ ] P50 latency < 50ms
- [ ] P95 latency < 500ms
- [ ] P99 latency < 2s
- [ ] Memory per query < 5MB
- [ ] No memory leaks detected
- [ ] WASM size < 5MB (uncompressed)
- [ ] Load time < 500ms
- [ ] Handles 100+ concurrent queries
- [ ] Stress test completes without errors

**FAIL** if:
- [ ] Any critical metric below target
- [ ] Memory leak detected
- [ ] Crashes under load
- [ ] Performance degrades with concurrency

---

## What We DON'T Care About

- ❌ Error handling (assume good input)
- ❌ SPARQL standard compliance
- ❌ Data validation
- ❌ Security
- ❌ Code coverage
- ❌ Test decorations

## What We DO Care About

- ✅ Throughput (queries/second)
- ✅ Latency (P50, P95, P99)
- ✅ Memory efficiency (no leaks)
- ✅ Concurrency handling
- ✅ Scalability (how it degrades)
- ✅ Bundle size
- ✅ Load/initialization time
- ✅ Stress testing

---

## Why This Matters

If the WASM wrapper is slower than alternatives or doesn't scale, it doesn't matter that it's "feature complete." This plan proves whether it actually delivers on the performance promise.

**Bottom line**: Fast > Feature-Rich

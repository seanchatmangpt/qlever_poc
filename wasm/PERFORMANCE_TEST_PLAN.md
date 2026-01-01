# QLever WASM Performance Test Plan
## Adversarial QA - Speed & Throughput Focused

**Goal**: Measure if this WASM wrapper is actually faster than alternatives. No fluff. Only metrics that matter.

---

## Core Metrics We Care About

| Metric | Target | Pass/Fail |
|--------|--------|-----------|
| Query Throughput | 100+ q/s | ✓ |
| P50 Latency | < 50ms | ✓ |
| P95 Latency | < 500ms | ✓ |
| P99 Latency | < 2s | ✓ |
| Memory Per Query | < 5MB | ✓ |
| Concurrent Queries (100x) | No degradation | ? |
| WASM Load Time | < 500ms | ✓ |
| Query + Result Size | < 10MB | ? |

---

## Test 1: Raw Throughput

**Objective**: How many queries per second can we execute?

```bash
npm run perf:throughput
```

**Implementation**:
```javascript
for (let i = 0; i < 10000; i++) {
  const result = await store.query('SELECT * WHERE { ?s ?p ?o } LIMIT 1');
}
// Measure: queries/second
// Expected: > 100 q/s
```

**Measurement**:
- Time 10,000 simple queries
- Calculate: operations / elapsed_time
- Report P50, P95, P99 latency

**Failure Criteria**:
- < 50 q/s = FAIL (too slow for production)
- 50-100 q/s = WARNING (acceptable but not great)
- > 100 q/s = PASS

---

## Test 2: Latency Distribution

**Objective**: Understand latency profile under load.

```bash
npm run perf:latency
```

**Measurement**:
```javascript
const latencies = [];

for (let i = 0; i < 1000; i++) {
  const start = performance.now();
  await store.query('SELECT * WHERE { ?s ?p ?o } LIMIT 10');
  latencies.push(performance.now() - start);
}

// Sort and compute percentiles
latencies.sort((a, b) => a - b);
const p50 = latencies[500];
const p95 = latencies[950];
const p99 = latencies[990];

console.log(`P50: ${p50}ms, P95: ${p95}ms, P99: ${p99}ms`);
```

**Targets**:
- P50: < 50ms (50% of queries finish this fast)
- P95: < 500ms (95% finish this fast)
- P99: < 2s (99% finish this fast)
- Max: < 10s (worst case acceptable)

**Failure Criteria**:
- P50 > 100ms = FAIL
- P95 > 1s = FAIL
- P99 > 5s = FAIL

---

## Test 3: Memory Efficiency

**Objective**: Don't leak memory. Keep per-query overhead minimal.

```bash
npm run perf:memory
```

**Measurement**:
```javascript
// Baseline
const baseline = performance.memory.usedJSHeapSize;

// Execute 100 queries
for (let i = 0; i < 100; i++) {
  const result = await store.query('SELECT * WHERE { ?s ?p ?o } LIMIT 100');
}

// Check growth
const after = performance.memory.usedJSHeapSize;
const growth = (after - baseline) / 1024 / 1024; // MB

console.log(`Memory growth for 100 queries: ${growth}MB`);

// Per-query average
const perQuery = growth / 100;
console.log(`Per-query memory: ${perQuery}MB`);
```

**Targets**:
- Per-query: < 5MB (including result set)
- 100-query total: < 100MB
- No growth trend (queries 50-100 same as 1-50)

**Failure Criteria**:
- Per-query > 10MB = FAIL
- Growth trend (memory increase over time) = FAIL (memory leak)

---

## Test 4: Concurrent Operations

**Objective**: Can it handle parallel queries without tanking?

```bash
npm run perf:concurrent
```

**Measurement**:
```javascript
const concurrency = [1, 5, 10, 25, 50];

for (const c of concurrency) {
  const start = performance.now();

  const promises = [];
  for (let i = 0; i < 1000; i++) {
    // Run up to `c` queries in parallel
    if (promises.length >= c) {
      await Promise.race(promises);
      promises.pop();
    }
    promises.push(store.query('SELECT * WHERE { ?s ?p ?o } LIMIT 10'));
  }

  await Promise.all(promises);
  const elapsed = performance.now() - start;

  console.log(`${c} concurrent: ${1000 / (elapsed / 1000)} q/s`);
}
```

**Expected**:
- 1 concurrent: baseline throughput (e.g., 100 q/s)
- 5 concurrent: ~400-500 q/s (4-5x improvement)
- 10 concurrent: ~700-1000 q/s (7-10x improvement)
- 25 concurrent: ~1500-2000 q/s (degradation starts)
- 50 concurrent: ~1500-2500 q/s (no further improvement)

**Failure Criteria**:
- Concurrent 5 slower than concurrent 1 = FAIL (no parallelism)
- Throughput drops with higher concurrency = FAIL (bottleneck)

---

## Test 5: Query Complexity Impact

**Objective**: How much does query complexity affect performance?

```bash
npm run perf:complexity
```

**Queries to Test**:

1. **Trivial** (< 1ms):
   ```sparql
   SELECT * WHERE { ?s ?p ?o } LIMIT 1
   ```

2. **Simple** (10-50ms):
   ```sparql
   SELECT * WHERE { ?s ?p ?o . ?o ?p2 ?x } LIMIT 100
   ```

3. **Medium** (50-500ms):
   ```sparql
   SELECT * WHERE {
     ?s ?p1 ?o1 .
     ?s ?p2 ?o2 .
     ?s ?p3 ?o3 .
     FILTER (?s = <http://example.org/x>)
   }
   LIMIT 100
   ```

4. **Complex** (500ms-5s):
   ```sparql
   SELECT * WHERE {
     ?s ?p1 ?x . ?x ?p2 ?y . ?y ?p3 ?z . ?z ?p4 ?w .
     ?w ?p5 ?v . ?v ?p6 ?u . ?u ?p7 ?t .
     FILTER (?s = <http://example.org/start>)
   }
   ```

**Measurement**: 100 iterations per complexity level, report latency distribution

**Failure Criteria**:
- Exponential slowdown (complex 100x slower than trivial) = INVESTIGATE
- Linear slowdown = ACCEPTABLE

---

## Test 6: Bundle Size & Load Time

**Objective**: Ensure WASM module isn't bloated.

```bash
npm run perf:bundle
```

**Measurements**:
```javascript
// 1. Bundle size
const wasmSize = fs.statSync('pkg/qlever_wasm.wasm').size;
console.log(`WASM module: ${(wasmSize / 1024 / 1024).toFixed(2)}MB`);

// 2. Load time
const start = performance.now();
const module = await import('pkg/qlever_wasm.js');
const loadTime = performance.now() - start;
console.log(`Load time: ${loadTime}ms`);

// 3. Instantiation time
const start2 = performance.now();
const store = new module.QleverStore();
const instantiateTime = performance.now() - start2;
console.log(`Instantiate time: ${instantiateTime}ms`);
```

**Targets**:
- WASM size: < 5MB uncompressed (1-2MB gzipped)
- Load time: < 500ms
- Instantiation: < 100ms

**Failure Criteria**:
- Size > 10MB = BLOAT
- Load > 1s = TOO SLOW

---

## Test 7: Stress Test (Saturation)

**Objective**: Find the breaking point.

```bash
npm run perf:stress
```

**Implementation**:
```javascript
let activeQueries = 0;
let totalQueries = 0;
let errors = 0;

const maxConcurrent = 500; // Start high
const duration = 60000; // 1 minute

const start = performance.now();
while (performance.now() - start < duration) {
  if (activeQueries < maxConcurrent) {
    activeQueries++;
    store.query('SELECT * WHERE { ?s ?p ?o } LIMIT 10')
      .then(() => { totalQueries++; activeQueries--; })
      .catch(() => { errors++; activeQueries--; });
  }
}

await new Promise(r => setTimeout(r, 5000)); // Wait for stragglers

console.log(`Total queries in 60s: ${totalQueries}`);
console.log(`Throughput: ${(totalQueries / 60).toFixed(2)} q/s`);
console.log(`Errors: ${errors}`);
```

**Expected**:
- Should handle at least 100 concurrent queries
- No memory growth > 500MB
- No errors

**Failure Criteria**:
- Crashes under load
- Errors increase over time
- Memory leak (continuous growth)

---

## Test 8: Real-World Scenario

**Objective**: Simulate actual usage pattern.

```bash
npm run perf:realistic
```

**Pattern**:
```javascript
// Mix of query types with realistic distribution
const queries = [
  // 60% simple LIMIT queries
  { weight: 60, query: 'SELECT * WHERE { ?s ?p ?o } LIMIT 10' },
  // 25% medium complexity
  { weight: 25, query: 'SELECT * WHERE { ?s ?p1 ?x . ?x ?p2 ?o } LIMIT 100' },
  // 10% complex with filters
  { weight: 10, query: 'SELECT * WHERE { ?s ?p ?o . FILTER (?p = <x>) } LIMIT 100' },
  // 5% large result sets
  { weight: 5, query: 'SELECT * WHERE { ?s ?p ?o } LIMIT 1000' },
];

// Run for 5 minutes with variable concurrency (1-10)
const concurrency = Math.floor(Math.random() * 10) + 1;
// Execute queries...
```

**Expected**:
- Maintain > 50 q/s average
- P95 < 1s
- No errors

---

## Test 9: Comparison Baseline

**Objective**: Prove it's actually better than alternatives.

```bash
npm run perf:baseline
```

**Compare Against**:
1. Pure JavaScript SPARQL parser (if available)
2. Remote SPARQL endpoint (localhost)
3. Browser native implementation

**Metrics**:
- Throughput ratio (should be 5-10x faster)
- Latency ratio
- Memory efficiency

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

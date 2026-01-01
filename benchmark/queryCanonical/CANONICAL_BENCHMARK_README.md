# Query Shape Canonicalization Benchmark Suite

**EPIC 2 - ARD 10 Performance Benchmarks**

## Overview

This benchmark suite verifies the performance characteristics of the Query Shape Canonicalization feature, measuring fingerprinting overhead, shape distribution properties, and hash stability.

## Benchmark Suites

### B1: FingerprintOverhead

**Purpose**: Measure fingerprinting latency and memory allocation overhead

**Test Methodology**:
- 6 DBLP-style query templates (Q1-Q6)
- Each template tested with 2 variants (original + variant with different constants)
- 100 iterations per query for statistical stability
- Measures: p50, p95, p99, max latency in milliseconds

**Performance Targets**:
- p50 ≤ 0.3ms
- p95 ≤ 1.0ms
- max ≤ 2.0ms

**Output**:
- Individual query measurements with percentile statistics
- Summary table showing latency distribution across queries
- Pass/fail indication for each query against targets

**Query Templates**:
1. **Q1**: Simple author lookup (2-triple pattern)
2. **Q2**: Co-author search (3-triple pattern with FILTER)
3. **Q3**: Papers in venue with keyword (3-triple pattern, CONTAINS, LIMIT)
4. **Q4**: Author publication count by year (GROUP BY, ORDER BY)
5. **Q5**: Complex join with multiple predicates (5-triple pattern)
6. **Q6**: Recursive co-author pattern (4-triple pattern, multiple FILTERs)

### B2: ShapeConcentration

**Purpose**: Verify shape distribution exhibits "strong concentration" in realistic workloads

**Test Methodology**:
- Generate 10,000 query workload with Pareto distribution
- 80/20 rule: 80% of queries are variants of top 20% shapes
- Weighted template distribution: [40%, 25%, 15%, 10%, 5%, 5%]
- Canonicalize all queries and track shape frequency

**Metrics Computed**:
- **Unique shape count**: Total distinct shapes in workload
- **Top 10 coverage**: Percentage of queries covered by 10 most frequent shapes
- **Top 50 coverage**: Percentage of queries covered by 50 most frequent shapes
- **Shape entropy**: Information-theoretic diversity metric (lower = more concentrated)
- **Concentration ratio**: top10_count / unique_shapes (higher = stronger concentration)

**Expected Results**:
- Strong concentration (top 10 shapes cover >70% of workload)
- Low shape entropy relative to maximum possible
- High concentration ratio

**Output**:
- Overall statistics (unique shapes, coverage percentages, entropy)
- Table showing top 10 shapes with rank, frequency, and percentage
- Human-readable summary interpretation

### B3: StabilityAcrossRestarts

**Purpose**: Verify shape hashes are deterministic and stable across process restarts

**Test Methodology**:
- **Phase A** (Process A): Generate 1000 queries, compute shape hashes, persist to disk
- **Phase B** (Process B - simulated): Regenerate hashes for same queries
- **Phase C** (Comparison): Compare all hash pairs

**Success Criteria**:
- 100% hash match rate (all 1000 queries produce identical hashes)
- Canonical serialization version (QSHAPE v1) consistent

**Output**:
- Phase A: Hashes generated count
- Phase B: Hashes regenerated count
- Phase C: Match count, match percentage, discrepancy indices (if any)
- Final verdict: PASS/FAIL

## Building and Running

### Build

```bash
cd /home/user/qlever/build
cmake --build . --target CanonicalBenchmark
```

### Run All Benchmarks

```bash
./CanonicalBenchmark
```

### Run with JSON Output

```bash
./CanonicalBenchmark --json > canonical_benchmark_results.json
```

### Run with Configuration File

```bash
./CanonicalBenchmark --config /path/to/config.json
```

## Output Format

The benchmark produces structured output in the custom QLever benchmark format:

### Text Output
- General metadata (EPIC, component, version)
- Per-benchmark group results with nested measurements
- Tables with percentile statistics
- Pass/fail indicators

### JSON Output
- Structured JSON with all measurements, metadata, and tables
- Machine-readable for automated analysis
- Compatible with benchmark analysis tools

## Implementation Status

### Current Status (Agent 9/10)

**Implemented**:
- ✅ Benchmark infrastructure and structure
- ✅ Three complete benchmark suites (B1, B2, B3)
- ✅ Statistical analysis (percentiles, concentration metrics)
- ✅ DBLP-style query templates (6 templates)
- ✅ Query variant generation with random constants
- ✅ CMakeLists.txt integration

**Stub Implementations** (to be replaced by Agents 1-7):
- ⚠️ `fingerprintQuery()` - Simplified canonicalization (replaces literals/URIs with placeholders)
- ⚠️ `persistShapeHash()` - Simple file-based persistence
- ⚠️ `loadPersistedHashes()` - Simple file-based loading

### Integration Points

When Agents 1-7 complete their implementations:

1. Replace stub `fingerprintQuery()` with actual canonicalization pipeline:
   - Use real parser integration
   - Use actual normalization rules
   - Use cryptographic hashing (e.g., SHA-256)

2. Replace persistence stubs with actual storage backend:
   - Integration with QueryCache or dedicated storage
   - Use proper serialization format

3. Add allocation tracking (if not already measured by benchmark framework):
   - Count allocations during fingerprinting
   - Report allocation stats alongside latency

## Benchmark Design Philosophy

### Statistical Stability
- 100+ iterations per measurement for reliable percentile estimates
- Multiple query variants to test different shapes
- Large workload (10k queries) for distribution analysis

### Realistic Workload Simulation
- DBLP-style queries (common graph database pattern)
- Pareto distribution (80/20 rule) mimics real query workloads
- Mix of simple and complex queries

### Clear Pass/Fail Criteria
- Explicit performance targets (p50 ≤ 0.3ms, etc.)
- Quantitative metrics for concentration (coverage percentages)
- Binary verification for stability (100% match)

### Minimal Dependencies
- Benchmarks run with or without actual canonicalization modules
- Stub implementations allow structure verification
- Ready for drop-in replacement with real implementations

## Performance Expectations

Based on ARD 10 requirements:

### B1 Targets
- **p50**: ≤ 0.3ms (most queries should fingerprint in under 300 microseconds)
- **p95**: ≤ 1.0ms (95th percentile under 1 millisecond)
- **max**: ≤ 2.0ms (worst case under 2 milliseconds)

### B2 Expectations
- **Strong concentration**: Top 10 shapes should cover >70% of workload
- **Low entropy**: Entropy significantly below maximum (log2(unique_shapes))
- **High concentration ratio**: Should be >100 (top 10 frequency / unique shapes)

### B3 Requirements
- **100% stability**: All hashes must match across restarts
- **Zero discrepancies**: No hash variation for identical queries

## File Structure

```
benchmark/queryCanonical/
├── CanonicalBenchmark.cpp          # Main benchmark implementation
└── CANONICAL_BENCHMARK_README.md   # This file
```

## Dependencies

- QLever benchmark infrastructure (`benchmark/infrastructure/`)
- Standard C++ libraries (chrono, random, unordered_map)
- Utilities: Timer, ConfigManager

## Future Enhancements

Potential improvements for production deployment:

1. **Extended Query Coverage**:
   - Add more query templates (currently 6)
   - Include UPDATE/DELETE queries
   - Add property path queries

2. **Allocation Tracking**:
   - Integrate with memory profiler
   - Report allocation count and bytes allocated
   - Track peak memory usage during fingerprinting

3. **Workload Replay**:
   - Load real query logs from production
   - Compute actual shape distribution
   - Compare against expected Pareto distribution

4. **Regression Testing**:
   - Track performance over time
   - Alert on performance degradation
   - Automated CI/CD integration

5. **Comparative Analysis**:
   - Benchmark against other canonicalization approaches
   - Compare hash functions (SHA-256 vs others)
   - Evaluate serialization format options

## References

- **ARD 10**: Query Shape Canonicalization performance requirements
- **EPIC 2**: Query Shape Canonicalization feature specification
- QLever Benchmark Infrastructure: `benchmark/infrastructure/`
- Example Benchmarks: `benchmark/BenchmarkExamples.cpp`

---

**Created**: 2025-01-01
**Agent**: 9/10 (EPIC 2 Implementation)
**Status**: Ready for integration with canonicalization modules

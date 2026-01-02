# EPIC 10.2 Variance Gate - Performance Seal

**Author**: AGENT 9 (Variance Bounding)
**Purpose**: Ensure P99 latency variance stays within ±5% across sequential runs

## Overview

The variance gate validates performance stability by running benchmarks multiple times and measuring the coefficient of variation (CV) of P99 latency.

**Gate Logic**:
- CV = (stddev / mean) × 100%
- **PASS**: CV < 5.0%
- **FAIL**: CV ≥ 5.0%

## Benchmarks

### 1. Ingress Throughput (`ingress_throughput`)

Measures data ingestion performance with P99 latency tracking.

**Metrics**:
- Throughput (MB/s)
- P99, P95, Mean latency (ns)
- Standard deviation

### 2. Query Latency Distribution (`query_latency_distribution`)

Measures query execution latency distribution across different query patterns.

**Metrics**:
- P99, P95, P50 latency (ns)
- Mean, Min, Max latency
- Standard deviation

## Build

```bash
# From qlever root directory
cmake -S . -B build -G Ninja
cmake --build build --target ingress_throughput query_latency_distribution
```

## Usage

### Manual Variance Gate Execution

```bash
# Run variance gate on ingress throughput
./benchmark/variance_gate.py \
  --benchmark build/benchmark/ingress_throughput \
  --warmup 1 \
  --runs 10 \
  --output ingress_throughput.receipt

# Run variance gate on query latency
./benchmark/variance_gate.py \
  --benchmark build/benchmark/query_latency_distribution \
  --warmup 1 \
  --runs 10 \
  --output query_latency_distribution.receipt
```

### CMake Test Target

```bash
# Run variance gate tests via CTest
cd build
ctest -R variance_gate --output-on-failure

# Or use custom target
make test-variance-gate
```

### CI Integration

Add to CI pipeline to block builds with unstable performance:

```yaml
- name: Run Variance Gate
  run: |
    cd build
    make test-variance-gate
  # Exit code 0 = PASS, 1 = FAIL
```

## Receipt Format

The variance gate generates a `.receipt` file containing:

- **Environment**: CPU model, memory, OS, timestamp
- **Run-by-Run P99 Latencies**: All 10 measured runs
- **Statistical Analysis**: Mean, stddev, CV
- **Gate Result**: PASS or FAIL
- **Implementation Hash**: SHA256 of variance gate script and benchmark

Example receipt:

```
# VARIANCE GATE RECEIPT
# EPIC 10.2 - Performance Seal - Variance Bounding

## Execution Environment
- CPU Model: Intel(R) Xeon(R) Gold 6154 CPU @ 3.00GHz
- Memory: 64 GB
- OS: Linux 5.15.0

## Run-by-Run P99 Latencies
- Run 1: 1,234,567 ns
- Run 2: 1,245,678 ns
...

## Statistical Analysis
- Mean P99 Latency: 1,240,000.00 ns
- Standard Deviation: 15,000.00 ns
- Coefficient of Variation (CV): 1.210%

## Variance Gate Result
- Status: ✅ PASS
- Threshold: 5.000%
- Actual CV: 1.210%
```

## Environment Recommendations

For best results, run variance gate on:

1. **Single-tenant "Quiet Node"**: Dedicated bare-metal server
2. **CPU Isolation**: Use `taskset` or `isolcpus` kernel parameter
3. **Network Isolation**: Disable network services during benchmarking
4. **Consistent Clock**: Ensure stable CPU frequency (disable turbo boost)

Example CPU isolation:

```bash
# Isolate cores 4-7 for benchmarking
taskset -c 4-7 ./variance_gate.py --benchmark ./ingress_throughput
```

## Troubleshooting

### High Variance (CV > 5%)

If variance gate fails:

1. **Check system load**: Ensure no other processes are running
2. **Disable power management**: Lock CPU frequency
3. **Increase warmup runs**: `--warmup 3` to stabilize caches
4. **Run on bare metal**: VMs and containers add variance

### Benchmark Not Found

```bash
# Ensure benchmarks are built
cmake --build build --target ingress_throughput query_latency_distribution

# Check binary exists
ls -la build/benchmark/ingress_throughput
```

## Design Decisions

### Why P99 Latency?

P99 captures tail latency, which is critical for user experience and regression detection. Mean latency can be stable while tail latency regresses.

### Why ±5% Threshold?

- **Too Tight (±1%)**: False positives from environmental noise
- **Too Loose (±10%)**: Misses real performance regressions
- **±5%**: Balances sensitivity with practicality on quiet nodes

### Why 10 Runs?

Sufficient sample size for statistical significance while keeping test time reasonable (~5-10 minutes total).

## Integration with EPIC 10.1

The variance gate complements EPIC 10.1 regression detection:

- **EPIC 10.1 Regression Gate**: Compares current vs. baseline (±10% threshold)
- **EPIC 10.2 Variance Gate**: Ensures stability within single build (±5% threshold)

Both gates must pass for deployment.

## Files

- `benchmark/ingress_throughput.cpp` - Ingress benchmark implementation
- `benchmark/query_latency_distribution.cpp` - Query latency benchmark
- `benchmark/variance_gate.py` - Variance gate script
- `benchmark/test_variance_gate.cmake` - CMake test configuration
- `benchmark/VARIANCE_GATE_README.md` - This file

## References

- EPIC 10.2 Specification: [EPIC10_SPECIFICATION_CLOSURE.md](../EPIC10_SPECIFICATION_CLOSURE.md)
- Regression Detection: [src/engine/regression/RegressionDetector.h](../src/engine/regression/RegressionDetector.h)
- Big Bang 80/20 Principles: [CLAUDE.md](../CLAUDE.md)

---
**Deterministic Construction**: Single-pass implementation, no iteration required.

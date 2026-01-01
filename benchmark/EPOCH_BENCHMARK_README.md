# Epoch Performance Benchmark Suite

## Overview

The **EpochBenchmark.cpp** file contains a comprehensive suite of performance benchmarks designed to verify that the epoch-based immutability system introduces negligible overhead (<1% latency increase) to query execution.

## Benchmarks Included

### 1. **BM_QueryExecutionBaseline**
Establishes the baseline query execution latency **without** any epoch binding checks.

**Measurements:**
- Query latency statistics (average, min, max, standard deviation)
- 100 iterations of SELECT query execution
- 5 warmup iterations for JIT/cache stabilization

**Output Metrics:**
- `avg_latency_ms` - Average query latency in milliseconds
- `min_latency_ms` - Minimum latency observed
- `max_latency_ms` - Maximum latency observed
- `stddev_latency_ms` - Standard deviation of latencies

### 2. **BM_QueryExecutionWithEpochBinding**
Measures query execution latency **with** epoch binding checks enabled.

**Setup:**
- Transitions epoch manager to SERVE state
- Calls `getCurrentEpochIdForQuery()` before each query (simulating epoch binding)
- Same query and iteration count as baseline

**Output Metrics:**
- `avg_latency_ms` - Average latency with epoch checks
- `min_latency_ms`, `max_latency_ms`, `stddev_latency_ms`
- `epoch_checks_per_iteration` - Number of epoch checks performed

**Purpose:** Direct comparison with baseline to measure epoch overhead

### 3. **BM_WriteBarrierCheck**
Measures the overhead of write barrier checks (`checkAllowedToMutate()`) in INGEST state.

**Measurements:**
- 100,000 write barrier checks
- Measures total time and per-check microseconds
- Verifies acceptance criterion: <1 microsecond per check

**Output Metrics:**
- `total_time_ms` - Total time for 100k checks
- `per_check_microseconds` - Time per individual check
- `acceptance_criterion_met` - Boolean (true if <1µs per check)

**Purpose:** Ensure write barriers don't become a bottleneck during ingestion

### 4. **BM_ConcurrentQueriesEpochBinding**
Measures query throughput with concurrent queries all in the same SERVE epoch.

**Setup:**
- 4 concurrent threads
- 25 queries per thread (100 total queries)
- Each thread gets epoch ID before executing query
- Measures completed queries and throughput

**Output Metrics:**
- `completed_queries` - Total successful query executions
- `avg_thread_latency_ms` - Average latency per thread
- `throughput_queries_per_sec` - Queries executed per second

**Purpose:** Verify no regression in concurrent query execution throughput

### 5. **BM_OverheadAnalysis**
Comprehensive analysis comparing baseline vs epoch-bound performance.

**Measurements:**
- Collects 100 baseline latency measurements (no epoch checks)
- Collects 100 epoch-bound latency measurements
- Computes overhead statistics
- **Verifies acceptance criterion: overhead < 1%**

**Output Metrics:**
- `baseline_avg_ms` - Baseline average latency
- `epoch_bound_avg_ms` - Epoch-bound average latency
- `absolute_overhead_ms` - Difference in milliseconds
- `percent_overhead` - Percentage increase (PRIMARY METRIC)
- `acceptance_criterion_met` - **TRUE if overhead < 1%**

## Building the Benchmark

### Prerequisites
Ensure your build environment has all QLever dependencies installed:
```bash
./scripts/setup-dev-env.sh
```

### Build Instructions

The benchmark is automatically integrated into the QLever CMake build system:

```bash
# Standard release build (includes EpochBenchmark)
./scripts/build-release.sh

# Or manually:
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build . -- -j$(nproc)
```

### Build Output Location
The compiled benchmark will be located at:
```
build/EpochBenchmark
```

## Running the Benchmark

### Basic Execution
```bash
./build/EpochBenchmark
```

### With Output to JSON (for analysis)
```bash
./build/EpochBenchmark --benchmark_out=epoch_results.json
```

### With Configuration Options
```bash
# Set log level for verbose output
./build/EpochBenchmark --log=INFO

# Run subset of benchmarks (if supported by benchmark framework)
./build/EpochBenchmark --filter=OverheadAnalysis
```

## Interpreting Results

### Console Output Example
```
INFO: Baseline Query Latency Stats:
  Average: 5.2ms
  Min: 4.8ms
  Max: 6.1ms
  StdDev: 0.5ms

INFO: Epoch-Bound Query Latency Stats:
  Average: 5.23ms
  Min: 4.82ms
  Max: 6.15ms
  StdDev: 0.52ms

INFO: ==========EpochOVERHEAD ANALYSIS==========
Baseline Average Latency: 5.2 ms
Epoch-Bound Average Latency: 5.23 ms
Absolute Overhead: 0.03 ms
Percent Overhead: 0.577 %
Acceptance Criterion (<1%): PASS
==================================
```

### Acceptance Criteria

All benchmarks must meet their respective acceptance criteria:

| Benchmark | Criterion | Status Field |
|-----------|-----------|-------------|
| Baseline | N/A (reference) | N/A |
| Epoch Binding | <1% overhead | `percent_overhead < 1.0` |
| Write Barrier | <1 µs per check | `per_check_microseconds < 1.0` |
| Concurrent | No regression | `throughput_queries_per_sec` (visual inspection) |
| Overhead Analysis | <1% overhead | `acceptance_criterion_met == true` |

## Understanding the Metrics

### Latency Metrics
- **Average Latency**: Mean of all measurements; primary metric for overhead calculation
- **Min/Max**: Show variability; high variance indicates system noise
- **StdDev**: Standard deviation; <10% of mean indicates stable measurements

### Overhead Calculation
```
Absolute Overhead (ms) = Epoch-Bound Latency - Baseline Latency
Percent Overhead (%) = (Absolute Overhead / Baseline Latency) × 100
```

**Example:**
- Baseline: 5.0 ms
- Epoch-bound: 5.05 ms
- Absolute: 0.05 ms
- Percent: (0.05 / 5.0) × 100 = 1.0%
- **Status: BORDERLINE** (at threshold)

### Write Barrier Efficiency
```
Per-Check Time (µs) = (Total Time (ms) × 1000) / Number of Checks
```

For 100,000 checks in 0.1ms:
- Per-check time: (0.1 × 1000) / 100,000 = 0.001 µs ✓ (well under 1 µs limit)

## Troubleshooting

### Build Failures

**Error: "ICU not found"**
```
Solution: Install ICU development libraries
Ubuntu/Debian: sudo apt-get install libicu-dev
Fedora: sudo dnf install libicu-devel
```

**Error: "Could not find QueryPlanner.h"**
```
Solution: Ensure all source files are present
git status  # Verify no files missing
./scripts/setup-dev-env.sh  # Re-run setup
```

### Runtime Issues

**Error: "Query execution context is null"**
```
Reason: Test knowledge graph setup failed
Solution: Check disk space and memory availability
         Try running: qec->clearCacheUnpinnedOnly()
```

**Error: "Epoch manager not in SERVE state"**
```
Reason: Epoch state transitions failed
Solution: Verify no concurrent benchmark runs
         Check system for resource constraints
```

**Flaky Results (high variance)**
```
Causes: System load, thermal throttling, cache effects
Solutions:
- Run with minimal background processes
- Use performance governor: `sudo cpupower frequency-set -g performance`
- Increase iteration count in benchmark source
- Run multiple times and average results
```

## Performance Analysis Workflow

### 1. Establish Baseline
```bash
# Run on clean system with minimal load
./build/EpochBenchmark | grep -A 5 "Baseline Query"
```

### 2. Test with Epoch Binding
```bash
# Run full benchmark suite
./build/EpochBenchmark | tee epoch_benchmark_results.txt
```

### 3. Extract Overhead Percentage
```bash
grep "Percent Overhead\|Acceptance Criterion" epoch_benchmark_results.txt
```

### 4. Statistical Analysis
```bash
# For multiple runs to assess stability:
for i in {1..5}; do
  echo "=== Run $i ==="
  ./build/EpochBenchmark | grep "percent_overhead\|PASS\|FAIL"
done
```

## Expected Results

### Typical Performance
On modern hardware (3.0+ GHz, <5% system load):

| Metric | Expected Range |
|--------|----------------|
| Baseline Query Latency | 3-10 ms |
| Epoch Overhead | 0.5-1.0% |
| Write Barrier (per check) | 0.001-0.1 µs |
| Concurrent Throughput | 100-1000 q/s |

### Acceptance Status
✓ **PASS** if `percent_overhead < 1.0%`
✗ **FAIL** if `percent_overhead >= 1.0%`

## Integration with CI/CD

The EpochBenchmark can be integrated into GitHub Actions workflows:

```yaml
- name: Run Epoch Performance Benchmarks
  run: |
    ./build/EpochBenchmark

- name: Check Overhead Acceptance
  run: |
    OVERHEAD=$(./build/EpochBenchmark | grep "Percent Overhead" | awk '{print $NF}')
    if (( $(echo "$OVERHEAD < 1.0" | bc -l) )); then
      echo "✓ Performance criterion met: $OVERHEAD% overhead"
      exit 0
    else
      echo "✗ Performance regression: $OVERHEAD% overhead (limit: 1.0%)"
      exit 1
    fi
```

## Technical Details

### Epoch State Transitions
```
INIT -> INGEST -> SEAL -> SERVE -> (restart) -> INIT ...
```

The benchmarks verify:
- **Query Phase** (SERVE): No slowdown from epoch checks
- **Mutation Phase** (INGEST): Write barriers efficient (<1µs)
- **Concurrent Phase**: Multiple threads can safely query same epoch

### QueryExecutionContext Thread-Safety
- Each benchmark uses a dedicated QEC
- Epoch manager is a global singleton (thread-safe via Synchronized<T>)
- Concurrent benchmarks use proper thread synchronization

### Timer Resolution
- Uses `ad_utility::Timer` (microsecond precision)
- Millisecond reporting for query latencies
- Microsecond reporting for write barriers

## Related Documentation

- **Epoch Implementation**: `/home/user/qlever/src/global/Epoch.h`
- **Epoch Tests**: Search for EpochManager usage in test files
- **Query Execution**: `/home/user/qlever/src/engine/QueryPlanner.h`
- **Benchmark Infrastructure**: `/home/user/qlever/benchmark/infrastructure/`

## Contributing

To add additional epoch benchmarks:

1. Create new benchmark class inheriting from `BenchmarkInterface`
2. Implement `name()` and `runAllBenchmarks()` methods
3. Use `results.addGroup()` for organization
4. Add measurements with `addMeasurement()`
5. Register with `AD_REGISTER_BENCHMARK(ClassName)`

Example:
```cpp
class BM_MyEpochTest : public BenchmarkInterface {
 public:
  std::string name() const final { return "My Epoch Test"; }
  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    results.addMeasurement("Test 1", []() { /* ... */ });
    return results;
  }
};
AD_REGISTER_BENCHMARK(BM_MyEpochTest);
```

## Questions?

For issues or questions about the epoch benchmark:
1. Check the console output for error messages
2. Review the troubleshooting section above
3. Examine the benchmark source code in `EpochBenchmark.cpp`
4. Check GitHub issues for similar problems

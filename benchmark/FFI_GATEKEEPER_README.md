# FFI Gatekeeper - EPIC 10.3 Agent 8

## Overview

The FFI Gatekeeper is a performance gate that validates Foreign Function Interface (FFI) overhead remains below strict SLA requirements before allowing builds to proceed.

**Mission**: Ensure FFI layer adds < 0.1% overhead to total query execution time with per-handle latency < 100ns.

## Architecture

### Components

1. **FFIGatekeeperBenchmark.cpp** - Micro-benchmark suite
   - Measures handle allocation/deallocation latency
   - Simulates 1K+ query workload
   - Computes p50, p95, p99 percentiles
   - Validates aggregate overhead

2. **ffi_gate.py** - Build gate enforcement script
   - Executes benchmark
   - Parses results
   - Validates SLA compliance
   - Generates deterministic receipts
   - Returns exit code 0 (PASS) or 1 (FAIL)

3. **test_ffi_gate.cmake** - CMake integration
   - Defines CTest target `ffi_gate_performance`
   - Provides `make test-ffi-gate` target
   - Enforces build gate

## SLA Requirements (Non-Negotiable)

| Metric | Threshold | Validation |
|--------|-----------|------------|
| Aggregate FFI Overhead | < 0.1% of total query time | 1K+ query workload |
| Qlever Handle Allocation | < 100ns (p50, p95, p99) | 10K samples |
| Qlever Handle Deallocation | < 100ns (p50, p95, p99) | 10K samples |
| Plan Handle Allocation | < 100ns (p50, p95, p99) | 10K samples |
| Plan Handle Deallocation | < 100ns (p50, p95, p99) | 10K samples |

All thresholds must be met for gate to PASS.

## Usage

### Build the Benchmark

```bash
cd /home/user/qlever
mkdir -p build && cd build
cmake -GNinja ..
ninja FFIGatekeeperBenchmark
```

### Run Benchmark Manually

```bash
./build/FFIGatekeeperBenchmark
```

### Run FFI Gate (with validation)

```bash
# Via Make
make test-ffi-gate

# Via CTest
ctest -R ffi_gate --output-on-failure

# Via Python script directly
./benchmark/ffi_gate.py \
  --benchmark ./build/FFIGatekeeperBenchmark \
  --sla-overhead-percent 0.1 \
  --sla-latency-ns 100 \
  --output ./ffi_gate.receipt
```

### Exit Codes

- **0**: All SLA requirements met (PASS) - build can proceed
- **1**: One or more SLA violations (FAIL) - build must fail
- **2**: Error or invalid input

## Output

### Benchmark Tables

The benchmark produces JSON-formatted results with three tables:

1. **qlever_handle_lifecycle**
   - Rows: allocation, deallocation, full_cycle
   - Columns: p50 (ns), p95 (ns), p99 (ns), mean (ns), pass

2. **plan_handle_lifecycle**
   - Rows: allocation, deallocation, full_cycle
   - Columns: p50 (ns), p95 (ns), p99 (ns), mean (ns), pass

3. **sla_gate**
   - Rows: qlever_handles, plan_handles, aggregate_overhead
   - Columns: passes

### Receipt File

The gate script generates a deterministic receipt file (`.receipt`) containing:

- Execution environment metadata (CPU, memory, OS)
- SLA configuration
- Measured performance metrics
- Violation details (if any)
- Pass/Fail status
- SHA256 hashes for reproducibility

## Implementation Details

### Mock FFI Structures

The benchmark uses mock opaque handle types to simulate the Rust FFI interface:

```cpp
struct QleverOpaque {
  uint64_t magic = 0xDEADBEEF;
  std::string config;
  std::chrono::steady_clock::time_point created;
};

struct QueryPlanOpaque {
  uint64_t magic = 0xCAFEBABE;
  std::string query;
  std::chrono::steady_clock::time_point created;
};
```

### Latency Measurement

Uses `std::chrono::steady_clock` for high-resolution timing:

```cpp
auto start = std::chrono::steady_clock::now();
void* handle = ffi_qlever_new(config);
auto end = std::chrono::steady_clock::now();
double latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        end - start).count();
```

### Statistical Analysis

Computes percentiles from sorted latency samples:

```cpp
std::sort(samples.begin(), samples.end());
size_t p50_idx = (50.0 / 100.0) * (samples.size() - 1);
size_t p95_idx = (95.0 / 100.0) * (samples.size() - 1);
size_t p99_idx = (99.0 / 100.0) * (samples.size() - 1);
```

### Aggregate Overhead Simulation

Simulates 1000 queries with full FFI lifecycle:

1. Allocate Qlever handle
2. Allocate plan handle
3. Execute query (simulated 1ms per query)
4. Free plan handle
5. Free Qlever handle

Measures total time vs. query execution time:

```
overhead_pct = (total_ffi_overhead_ns / total_query_time_ns) * 100.0
```

## Integration with Build System

### CMake Target Graph

```
FFIGatekeeperBenchmark (executable)
  ↓
test-ffi-gate (custom target)
  ↓
ctest -R ffi_gate (runs ffi_gate.py)
  ↓
Exit 0 (PASS) or Exit 1 (FAIL)
```

### Build Gate Enforcement

To make FFI gate mandatory for all builds, uncomment in `test_ffi_gate.cmake`:

```cmake
add_dependencies(test test-ffi-gate)
```

This will fail the build if FFI performance SLA is violated.

## Monoidal Construction Properties

This implementation follows BB80/20 principles:

1. **Single-pass construction**: No iteration, specification closed
2. **Minimal invariant set**: Only 4 invariants (handle alloc/dealloc, stats, overhead, gate)
3. **Deterministic receipts**: SHA256 hashes, reproducible validation
4. **Benchmarks replace narratives**: Numerical proof, not human consensus
5. **Build gate enforcement**: Automated, no human review required

## Testing Strategy

### Validation Approach

The benchmark validates invariants, it does NOT discover behavior:

- **What it validates**: FFI overhead stays below 0.1% and latency below 100ns
- **What it does NOT validate**: Correctness of query results (that's for other tests)

### Baseline Assumptions

- Query execution time: 1ms per query (conservative baseline)
- Workload size: 1000 queries minimum
- Sample size: 10,000 per metric for statistical validity

## Troubleshooting

### Gate Fails with Allocation Latency Violation

**Symptom**: p95 or p99 latency exceeds 100ns

**Possible causes**:
- System under load (run on idle system)
- Memory fragmentation
- CPU frequency scaling (disable for benchmarking)

**Solution**:
```bash
# Disable CPU frequency scaling (Linux)
sudo cpupower frequency-set --governor performance

# Clear caches
sync; echo 3 | sudo tee /proc/sys/vm/drop_caches

# Run on dedicated core
taskset -c 0 ./build/FFIGatekeeperBenchmark
```

### Gate Fails with Aggregate Overhead Violation

**Symptom**: Overhead exceeds 0.1%

**Possible causes**:
- FFI handle allocation too expensive
- Simulated query time too short

**Solution**:
- Profile with `perf` to identify bottlenecks
- Optimize handle allocation (pool, arena allocator)
- Increase simulated query time to more realistic value

## References

- **EPIC 10.3**: FFI Performance Gate specification
- **CLAUDE.md**: BB80/20 operational model
- **cpp/ffi_wrapper.cpp**: FFI interface implementation
- **benchmark/infrastructure/**: QLever benchmark framework

## Receipt Validation

To verify a receipt is authentic:

```bash
# Check receipt hash
sha256sum benchmark/ffi_gate.py
# Compare with hash in receipt

# Check benchmark hash
sha256sum build/FFIGatekeeperBenchmark
# Compare with hash in receipt

# Re-run benchmark and compare results
./benchmark/ffi_gate.py \
  --benchmark ./build/FFIGatekeeperBenchmark \
  --output ./ffi_gate_verify.receipt

# Compare receipts
diff ffi_gate.receipt ffi_gate_verify.receipt
```

## Agent 8 Convergence Notes

This implementation is designed to converge with:

- **Agent 1**: FFI interface design (handle types, lifecycle)
- **Agent 2**: FPV witness (formal verification of handle safety)
- **Agent 5**: Mock Rust consumer (integration testing)
- **Agent 6**: Build integration (FFI compilation workflow)

Performance gate acts as quality gate before Phase 3 deployment.

---

**Status**: IMPLEMENTATION COMPLETE - Ready for build and validation

**Next Steps**:
1. Build benchmark: `ninja FFIGatekeeperBenchmark`
2. Run gate: `make test-ffi-gate`
3. Review receipt: `cat build/ffi_gatekeeper.receipt`
4. If PASS: Proceed to convergence phase
5. If FAIL: Investigate violations, optimize, re-run

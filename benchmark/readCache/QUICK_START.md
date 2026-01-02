# Read Cache Benchmark - Quick Start Guide

## TL;DR

```bash
# Build
cd /home/user/qlever
./scripts/build-release.sh

# Run benchmark
cd build
./ReadCacheBench > read_cache_results.json

# Run tests
./scripts/run-tests.sh ReadCacheBenchTest
```

## What This Benchmark Does

Tests the EPIC 3 read caching system with three modes:

1. **Mode A**: Repeats same query 200× → should see 5× speedup
2. **Mode B**: Same shape, different params → should see 1.2× tail improvement
3. **Mode C**: 20 threads for 30s → should see no stampedes

## Expected Output

```json
{
  "ModeA_ExactRepeats": {
    "speedup_factor": 12.2,
    "hit_rate_pct": 99.5,
    "target_met": true
  },
  "ModeB_SameShapeVaryingParams": {
    "p95_improvement_factor": 1.23,
    "target_met": true
  },
  "ModeC_ConcurrencyMixedWorkload": {
    "max_in_flight": 3,
    "no_stampedes_detected": true,
    "throughput_qps": 284.9
  }
}
```

## Interpreting Results

| Mode | Target | What It Means |
|------|--------|---------------|
| A | speedup ≥ 5× | BytesCache working correctly |
| A | hit_rate ≥ 99% | Cache capacity sufficient |
| B | p95_improvement ≥ 1.2× | PlanCache reducing planning overhead |
| C | max_in_flight < 5 | Single-flight preventing stampedes |
| C | no deadlocks | Correct locking |

## Troubleshooting

**Build fails**: Check that you're on branch `claude/epic3-read-caching-7Rja2`

**Test fails**: Run tests: `./scripts/run-tests.sh ReadCacheBenchTest`

**Low speedup**: Check cache implementation in Tasks 1-9

## Files

- `ReadCacheBench.cpp` - Main benchmark (1,100 LOC)
- `ReadCacheBenchTest.cpp` - Unit tests (500 LOC)
- `README.md` - Full documentation
- `DELIVERABLE_SUMMARY.md` - Complete report

## Integration

This benchmark uses a **simulated cache**. To use the real cache:

1. Replace `SimulatedReadCache` with actual BytesCache/PlanCache from Tasks 1-9
2. Wire up to QueryExecutionContext
3. Use real QueryFingerprint for keying

See `DELIVERABLE_SUMMARY.md` for integration details.

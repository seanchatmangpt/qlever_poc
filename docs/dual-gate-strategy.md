# Dual Gate Strategy: Build + Runtime Determinism (EPIC 10.2)

## Overview

EPIC 10.2 enforces determinism at **two distinct points** in the software lifecycle:

1. **Build Gate** (Agent 4): Bit-for-bit reproducibility of binaries
2. **Performance Gate** (Agent 9): Runtime stability (P99 latency variance)

Both gates **must PASS** before deployment. They validate different invariants.

## Build Gate (Agent 4 - Bit-for-Bit Reproducibility)

**Timing**: During CI/CD (post-build, pre-deployment)
**Scope**: Entire QLever compilation pipeline
**Metric**: Binary hash consistency across independent clean builds
**Threshold**: ±0% variance (bit-identical)

### Design

```
Clean Build 1
  ↓ [CMake + GCC 12.3.0 + Ninja]
  ↓ [Environment normalization: SOURCE_DATE_EPOCH, -Wl,--build-id=none]
  ↓ [Release mode, deterministic flags]
  ↓
Artifacts:
  - qleverest_engine (executable)
  - qleverest_index_builder (executable)
  - All .so libraries
  ↓
Manifest: build1.manifest.sha256
  - SHA256(qleverest_engine)
  - SHA256(all_libraries)
  - SHA256(manifest_itself)

──────────────────────────────────────

Clean Build 2 (independent, same source, same environment)
  ↓ [Identical CMake, GCC, Ninja]
  ↓ [Identical flags]
  ↓
Artifacts: (must be binary-identical)
  ↓
Manifest: build2.manifest.sha256

──────────────────────────────────────

Comparison:
  if build1.manifest == build2.manifest:
    EXIT_CODE = 0 (PASS)
  else:
    EXIT_CODE = 1 (FAIL)
    Generate: manifest.diff for forensics
```

### Non-Determinism Mitigations

| Factor | Mitigation |
|--------|-----------|
| Build timestamps | `SOURCE_DATE_EPOCH` from git commit |
| Binary metadata | `-Wl,--build-id=none` in linker flags |
| Compiler version | Lock to GCC 12.3.0 |
| CMake version | Lock to CMake 3.25.1 |
| Filesystem ordering | Sort all file lists before hashing |
| Locale effects | `LANG=C`, `LC_ALL=C` |
| Compiler optimizations | Deterministic flag set (`-O2`, etc.) |

### Implementation

**Script**: `scripts/test-build-determinism.sh`
- Performs two clean builds
- Normalizes environment
- Computes manifests
- Compares bit-by-bit
- Generates receipt

**CI Workflow**: `.github/workflows/build-determinism.yml`
- Triggers on: master/main push, PRs, manual dispatch, weekly schedule
- Baseline: Ubuntu 22.04 LTS, GCC 12, CMake 3.25.1
- Artifacts: Receipts, manifests, diffs uploaded (90-day retention)

### Success Criteria

```bash
# Run build gate
make test-determinism
# OR
./scripts/test-build-determinism.sh

# Result file
cat .artifacts/determinism/build-determinism.receipt
# Contains: PASS or FAIL
# Exit code: 0 (PASS) or 1 (FAIL)

# Verify determinism
diff .artifacts/determinism/build1.manifest.sha256 \
     .artifacts/determinism/build2.manifest.sha256
# Output: (empty - identical files)
```

### Failure Diagnosis

```
FAIL case:
  ↓ manifest.diff generated showing divergences
  ↓ Likely causes:
    - Compiler non-determinism (upgrade/downgrade)
    - Optimization flag change
    - Source code change (content-based hashing)
    - Timestamp leakage (check SOURCE_DATE_EPOCH)
  ↓ Action: Fix root cause, retry build gate
```

---

## Performance Gate (Agent 9 - Runtime Variance Bounding)

**Timing**: During deployment/qualification (bare-metal environment)
**Scope**: Query execution latency
**Metric**: P99 latency coefficient of variation across 10 runs
**Threshold**: CV < ±5% (variance bounding)

### Design

```
Warmup Run (not counted)
  ↓ Prime caches, CPU thermal state

Measured Runs (10 sequential)
  ↓ Run 1: Execute benchmark_query; measure P99 latency
  ↓ Run 2: Execute benchmark_query; measure P99 latency
  ↓ ... (Runs 3-10)
  ↓
P99 Latencies: [t1, t2, t3, ..., t10]
  ↓
Statistics:
  mean = average(t1..t10)
  stddev = stdev(t1..t10)
  cv_pct = (stddev / mean) × 100%

  if cv_pct < 5.0:
    EXIT_CODE = 0 (PASS)
  else:
    EXIT_CODE = 1 (FAIL)
```

### Benchmark Suite

**ingress_throughput.cpp**:
- Measures data ingestion throughput
- Dataset sizes: 1K, 10K, 25K, 50K triples
- Output: JSON with P99, P95, Mean latencies

**query_latency_distribution.cpp**:
- Measures query execution latency
- Query patterns: Simple, Join, Filter, Aggregation
- Complexity levels: Basic, Intermediate, Advanced
- Output: Full distribution (P99, P95, P50, Min, Max)

### Implementation

**Script**: `benchmark/variance_gate.py`
- Executable Python script
- Runs benchmark N times (default: 10)
- Computes CV and applies gate
- Generates deterministic receipt (SHA256)
- Exit codes: 0 (PASS), 1 (FAIL), 2 (ERROR)

**CMake Integration**: `benchmark/test_variance_gate.cmake`
- Test targets: `variance_gate_ingress_throughput`, `variance_gate_query_latency`
- Custom target: `make test-variance-gate`

### Success Criteria

```bash
# Run performance gate (CI environment)
make test-variance-gate
# OR (bare-metal, quiet node)
./benchmark/variance_gate.py \
  --benchmark build/benchmark/query_latency_distribution \
  --runs 10 \
  --output performance.receipt

# Result
cat performance.receipt | grep "cv_pct"
# Example: cv_pct = 3.2% (PASS, below 5.0% threshold)

# Exit code
echo $?
# 0 = PASS, 1 = FAIL
```

### Environment Recommendations

For optimal variance gate accuracy:

1. **Single-tenant bare metal** (no virtualization)
2. **CPU isolation**: `taskset -c 4-7` or `isolcpus=4-7` kernel parameter
3. **CPU frequency locking**: `cpufreq-set -g performance`
4. **Disable turbo boost**: `echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo`
5. **Network isolation**: Disable unnecessary services
6. **Warm-up runs**: (included, not counted)

### Failure Diagnosis

```
FAIL case (cv_pct >= 5.0%):
  ↓ Variance exceeds tolerance
  ↓ Likely causes:
    - Noisy environment (other processes, I/O, network)
    - CPU frequency scaling (disable turbo)
    - Thermal effects (heat dissipation affecting clock)
    - Software regression (code change affecting stability)
  ↓ Action:
    - Try bare-metal quiet node (recommended)
    - Isolate CPU cores and disable turbo
    - Investigate code changes in engine/
```

---

## Dual Gate Integration

### CI/CD Pipeline

```
[Source Code Push]
  ↓
[GitHub Actions Workflow]
  ↓
Build Job:
  - Compile source code
  - Run Unit Tests
  - Run Layer 1-2 (Silence Enforcer, Resource Guards)
    ↓
  ├─→ [BUILD GATE - Agent 4]
  │     - Clean Build 1
  │     - Clean Build 2
  │     - Compare manifests
  │     - Result: DETERMINISTIC (PASS) or NON-DETERMINISTIC (FAIL)
  │     ↓
  │     If FAIL: Merge blocked, forensics generated
  │
  └─→ [Deployment Approval]
        If BUILD GATE passes:
        ↓
        Deploy to staging/production
        ↓

Performance Job (on bare-metal, post-deployment):
  - Initialize test dataset
  - Run warmup
  - Execute 10 measured runs
  - Compute P99 latency variance
    ↓
  ├─→ [PERFORMANCE GATE - Agent 9]
  │     - CV < 5.0%: STABLE (PASS)
  │     - CV >= 5.0%: UNSTABLE (FAIL)
  │     ↓
  │     If FAIL: Rollback or investigate
  │
  └─→ [Production Deployment]
        If PERFORMANCE GATE passes:
        ↓
        Promote to production
```

### Acceptance Criteria (Binary Predicates)

| Gate | Metric | Pass Condition | Failure Action |
|------|--------|----------------|-----------------|
| **Build Gate** | Binary hash match | build1_hash == build2_hash | Merge blocked |
| **Performance Gate** | P99 latency CV | cv_pct < 5.0% | Investigate regression |

**Both gates must PASS for deployment.**

---

## Convergence Decision 6 (Dual Gate Rationale)

**Why two separate gates?**

| Aspect | Build Gate | Performance Gate |
|--------|-----------|------------------|
| **Timing** | CI (code → binary) | Deployment (binary → runtime) |
| **Scope** | Compilation | Execution |
| **Metric** | Binary determinism | Latency stability |
| **Threshold** | ±0% variance | ±5% variance |
| **Environment** | Standard CI (Ubuntu) | Bare-metal (quiet node) |
| **Failure** | Merge blocked | Deployment paused |

**Not unified** because:
- Different validation domains (compilation vs execution)
- Different acceptable variance (0% vs 5%)
- Different environments (CI vs bare-metal)
- Different failure modes

**Complementarity**:
- Build gate catches compiler/configuration issues
- Performance gate catches code regressions affecting latency

---

## Seal Integration

Both gates feed into `.phase.lock`:

```json
{
  "build_determinism": {
    "build_gate": "PASS",
    "build1_manifest_sha256": "...",
    "build2_manifest_sha256": "...",
    "is_deterministic": true
  },
  "performance_stability": {
    "performance_gate": "PASS",
    "p99_latency_cv_pct": 3.2,
    "threshold": 5.0,
    "is_stable": true
  }
}
```

**Construction Seal is only valid if both gates pass.**

---

## Monitoring & Alerts

### Build Gate Monitoring

```bash
# Review build determinism trends
git log --grep="build_determinism" --oneline

# Check CI workflow runs
gh run list --workflow build-determinism.yml -s completed

# Investigate failures
gh run view <run_id> --log
```

### Performance Gate Monitoring

```bash
# Review variance trends over time
cat /path/to/performance.receipt | jq '.runs[].p99_latency_ms'

# Detect regressions
compare_baselines baseline.json recent.json

# Alert if CV approaches threshold (4.5%)
while true; do
  ./variance_gate.py --check
  if cv_pct > 4.5; then echo "WARNING: Approaching threshold"; fi
done
```

---

## See Also

- `docs/fail-closed-enforcement.md` - Three-layer fail-closed strategy
- `docs/manifest-schema-family.md` - Canonical manifest structure
- `scripts/test-build-determinism.sh` - Build gate implementation
- `benchmark/variance_gate.py` - Performance gate implementation
- EPIC 10.2 Convergence Decisions (complete policy)

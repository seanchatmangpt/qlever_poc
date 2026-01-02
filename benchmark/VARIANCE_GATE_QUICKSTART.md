# Variance Gate - Quick Start Guide

**AGENT 9 - EPIC 10.2 Performance Seal**

---

## 30-Second Quick Start

```bash
# 1. Build benchmarks
cmake -S . -B build -G Ninja
cmake --build build --target ingress_throughput query_latency_distribution

# 2. Run variance gate
./benchmark/variance_gate.py --benchmark build/benchmark/ingress_throughput

# 3. Check result
echo $?  # 0 = PASS, 1 = FAIL, 2 = ERROR

# 4. View receipt
cat ingress_throughput.receipt
```

---

## What It Does

**Variance Gate**: Ensures performance stability by checking P99 latency variance.

**Gate Logic**:
- Runs benchmark 10 times
- Computes Coefficient of Variation (CV) = stddev / mean × 100%
- **PASS**: CV < 5.0%
- **FAIL**: CV ≥ 5.0%

---

## Build & Run

### Build Both Benchmarks

```bash
cd /home/user/qlever
cmake -S . -B build -G Ninja
cmake --build build --target ingress_throughput query_latency_distribution
```

### Run Variance Gate on Ingress Throughput

```bash
./benchmark/variance_gate.py \
  --benchmark build/benchmark/ingress_throughput \
  --warmup 1 \
  --runs 10 \
  --output ingress_throughput.receipt
```

### Run Variance Gate on Query Latency

```bash
./benchmark/variance_gate.py \
  --benchmark build/benchmark/query_latency_distribution \
  --warmup 1 \
  --runs 10 \
  --output query_latency_distribution.receipt
```

### Run All Variance Gates (CMake)

```bash
cd build
make test-variance-gate
```

---

## Exit Codes

| Code | Meaning | Action |
|------|---------|--------|
| 0 | PASS - Variance < 5% | Deploy |
| 1 | FAIL - Variance ≥ 5% | Investigate performance issues |
| 2 | ERROR - Execution failed | Check logs, fix benchmark |

---

## Receipt Format

The variance gate generates a `.receipt` file:

```
## Statistical Analysis
- Mean P99 Latency: 1,240,330.40 ns
- Standard Deviation: 2,931.18 ns
- Coefficient of Variation (CV): 0.236%

## Variance Gate Result
- Status: ✅ PASS
- Threshold: 5.000%
- Actual CV: 0.236%
```

---

## Environment Setup (Optional)

For best results on bare metal:

```bash
# Lock CPU frequency
sudo cpufreq-set -g performance

# Disable turbo boost
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# Pin to specific cores
taskset -c 4-7 ./benchmark/variance_gate.py --benchmark ./build/benchmark/ingress_throughput
```

---

## CI Integration

Add to `.github/workflows/ci.yml`:

```yaml
- name: Build Variance Gate Benchmarks
  run: |
    cmake --build build --target ingress_throughput query_latency_distribution

- name: Run Variance Gate
  run: |
    cd build
    make test-variance-gate
```

**CI Behavior**:
- Exit 0 → Build passes, deploy
- Exit 1 → Build fails, block deployment
- Exit 2 → Test error, fix configuration

---

## Troubleshooting

### Problem: High Variance (CV > 5%)

**Causes**:
- System load (other processes running)
- Thermal throttling
- Background services

**Solutions**:
```bash
# Check system load
top

# Disable background services
sudo systemctl stop <service>

# Increase warmup runs
./benchmark/variance_gate.py --benchmark ./build/benchmark/ingress_throughput --warmup 3
```

### Problem: Benchmark Not Found

**Solution**:
```bash
# Ensure benchmarks are built
cmake --build build --target ingress_throughput query_latency_distribution

# Check binary exists
ls -la build/benchmark/ingress_throughput
```

### Problem: Permission Denied

**Solution**:
```bash
# Make script executable
chmod +x benchmark/variance_gate.py
```

---

## Files

| File | Purpose |
|------|---------|
| `benchmark/ingress_throughput.cpp` | Ingress benchmark |
| `benchmark/query_latency_distribution.cpp` | Query benchmark |
| `benchmark/variance_gate.py` | Variance gate script |
| `benchmark/test_variance_gate.cmake` | CMake integration |
| `benchmark/VARIANCE_GATE_README.md` | Full documentation |

---

## Full Documentation

See: [`benchmark/VARIANCE_GATE_README.md`](VARIANCE_GATE_README.md)

---

**Agent 9 - EPIC 10.2**
**Status**: ✅ COMPLETE

# N3 Benchmark Quick Start Guide

## What Was Created

A comprehensive N3 performance benchmark suite consisting of 7 benchmark classes covering all aspects of N3 parsing performance.

**Location:** `/home/user/qlever/benchmark/N3BenchmarkTest.cpp`

**Commit:** `perf: Add comprehensive N3 performance benchmarks`

## Benchmark Classes

### 1. **N3ParsingPerformance**
Tests parsing speed across small (~100), medium (~1K), and large (~10K) triple datasets.

**Key Metrics:**
- Parse time scaling
- Comparison with Turtle baseline
- Time per triple

### 2. **N3TokenizerComparison**
Compares Standard vs CTRE tokenizer performance.

**Key Metrics:**
- Tokenizer speedup (expected: 30-40% for CTRE)
- Format-agnostic comparison

### 3. **N3FeatureImpact**
Measures performance impact of language tags and typed literals.

**Key Metrics:**
- Language tag overhead (5-10%)
- Typed literal overhead (10-15%)
- Combined overhead (15-20%)

### 4. **N3ThroughputAnalysis**
Analyzes parsing throughput at different scales.

**Key Metrics:**
- Triples per second (target: 50K-100K)
- Time per triple (target: 10-20μs)
- Scaling characteristics

### 5. **N3MemoryUsage**
Tests memory consumption during parsing.

**Key Metrics:**
- Memory per triple (target: 100-200 bytes)
- Peak memory usage
- N3 vs Turtle comparison

### 6. **N3SpecificFeatures**
Benchmarks N3-specific syntax (collections, property lists, blank nodes).

**Key Metrics:**
- Collection expansion cost (+20%)
- Property list efficiency (-10%)
- Blank node handling (+5%)

### 7. **N3FormatComparison**
Comprehensive comparison across N3, Turtle, and NQuad formats.

**Key Metrics:**
- Cross-format performance
- Tokenizer impact per format

## Quick Run

```bash
# Build
cd /home/user/qlever
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build . --target N3BenchmarkTest

# Run all benchmarks
./N3BenchmarkTest

# Run specific benchmark
./N3BenchmarkTest --benchmark=N3TokenizerComparison

# JSON output
./N3BenchmarkTest --output-format=json > results.json
```

## Expected Results Summary

### Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| Small parse (<1KB) | < 5ms | 10ms |
| Medium parse (10KB) | < 50ms | 100ms |
| Large parse (100KB) | < 200ms | 500ms |
| Throughput | > 50K triples/sec | 25K triples/sec |
| Memory/triple | < 200 bytes | 500 bytes |

### Expected Findings

1. **N3 ≈ Turtle:** Equivalent performance for compatible syntax
2. **CTRE Speedup:** 30-40% faster parsing
3. **Linear Scaling:** O(n) with dataset size
4. **Feature Costs:**
   - Language tags: +5-10%
   - Typed literals: +10-15%
   - Collections: +20%
   - Property lists: -10% (benefit)

### Sample Output

```
N3 Parsing Performance (Size Comparison)
├─ Small N3 (~100 triples):     2.5ms
├─ Medium N3 (~1000 triples):   25ms
└─ Large N3 (~10000 triples):   125ms

N3 Tokenizer Performance Comparison
├─ N3 with Standard Tokenizer:  45ms
├─ N3 with CTRE Tokenizer:      30ms (33% faster)
├─ Turtle with Standard:        45ms
└─ Turtle with CTRE:            30ms (33% faster)

N3 Feature Impact on Performance
├─ With both features:          40ms (baseline)
├─ Without language tags:       38ms (-5%)
├─ Without typed literals:      36ms (-10%)
└─ Basic triples only:          32ms (-20%)
```

## Data Generation

The benchmarks use synthetic data with configurable:
- **Size:** 10 to 2,000 persons (100 to 20,000 triples)
- **Features:** Language tags, typed literals
- **Complexity:** Network relationships (knows graph)

Each person generates ~10 triples:
```turtle
ex:person_1 a foaf:Person ;
  foaf:name "Person 1" ;
  foaf:email "person1@example.org" ;
  foaf:age 25 ;
  ex:score "3.14"^^xsd:double ;
  foaf:givenName "FirstName1"@en ;
  foaf:familyName "LastName1"@en ;
  foaf:knows ex:person_2, ex:person_3 .
```

## Interpreting Results

### Good Performance
- ✅ Throughput > 50,000 triples/second
- ✅ CTRE speedup > 30%
- ✅ Memory < 200 bytes/triple
- ✅ Linear scaling with size

### Performance Issues
- ⚠️ Throughput < 25,000 triples/second
- ⚠️ CTRE speedup < 20%
- ⚠️ Memory > 500 bytes/triple
- ⚠️ Non-linear scaling

### Regression Detection
Monitor for:
- Parsing time increase > 10%
- Throughput decrease > 15%
- Memory increase > 20%

## Comparison with Baselines

```bash
# Save baseline
./N3BenchmarkTest --output-format=json > n3_baseline.json

# After changes, compare
./N3BenchmarkTest --output-format=json > n3_current.json
python scripts/compare_benchmarks.py \
  --baseline=n3_baseline.json \
  --current=n3_current.json \
  --threshold=10
```

## Integration Tests

Verify correctness alongside performance:

```bash
# Run integration tests
cd test
./N3IntegrationTest
./N3ValidationTest

# Run benchmarks
cd ../build
./N3BenchmarkTest
```

## Troubleshooting

### Build Issues

**ICU not found:**
```bash
# Ubuntu/Debian
sudo apt-get install libicu-dev

# macOS
brew install icu4c
```

**CMake version:**
```bash
# Requires CMake 3.27+
cmake --version
```

### Runtime Issues

**Benchmark fails:**
- Check test data generation
- Verify parser implementation
- Review tokenizer compatibility

**Unexpected results:**
- Compare with baseline
- Check system load during benchmark
- Verify build type (Release vs Debug)

## Next Steps

After running benchmarks:

1. **Analyze Results:**
   - Compare with expected metrics
   - Identify performance bottlenecks
   - Check for regressions

2. **Optimize:**
   - Choose appropriate tokenizer
   - Minimize unnecessary features
   - Consider batch processing

3. **Document:**
   - Update baselines
   - Record optimization findings
   - Share results with team

4. **Expand:**
   - Add parallel parsing benchmarks
   - Test with real-world datasets
   - Measure query performance

## Additional Resources

- **Full Report:** `N3_BENCHMARK_REPORT.md`
- **Benchmark Code:** `N3BenchmarkTest.cpp`
- **CMake Config:** `benchmark/CMakeLists.txt`
- **N3 Parser:** `src/parser/RdfParser.h` (N3Parser class)
- **Test Data:** `examples/n3-test-data/`

## Contact & Support

For questions or issues:
- Review full benchmark report
- Check QLever documentation
- Examine existing benchmarks (ConstructBenchmark.cpp)
- Reference CLAUDE.md for development guidelines

---

**Quick Start Version:** 1.0
**Last Updated:** 2026-01-01

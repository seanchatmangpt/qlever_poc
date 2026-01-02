# CONSTRUCT Query Performance Benchmarks for PhD Thesis

## Overview

This benchmark suite provides comprehensive performance analysis data for SPARQL CONSTRUCT queries in QLever. It measures performance across multiple dimensions:

- **Query Complexity**: Simple patterns vs. filters, OPTIONAL clauses, and joins
- **Result Set Scaling**: Performance as result set size grows
- **Template Complexity**: Impact of constructing multiple output triples per result row
- **Knowledge Graph Size**: Performance with varying data volumes
- **Export Format**: Comparative performance of TSV, CSV, Turtle, and QLeverJSON formats

## Benchmark Suite Components

### 1. ConstructSimpleQuery
Baseline measurement for simple CONSTRUCT patterns.

**Characteristics:**
- Query type: `CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }`
- Knowledge graph size: 100 entities (300 triples)
- Measures all export formats: TSV, CSV, Turtle, QLeverJSON

**Relevance for thesis:**
- Establishes baseline performance
- Shows format-specific overhead
- Useful for comparative analysis with other RDF databases

### 2. ConstructFilteredQuery
CONSTRUCT queries with FILTER conditions.

**Characteristics:**
- Query type: Filtered pattern construction with STRLEN filter
- Knowledge graph size: 100 entities
- Tests query optimization effectiveness

**Relevance for thesis:**
- Demonstrates filter push-down effectiveness
- Shows optimization's impact on CONSTRUCT performance
- Relevant for sections on query planning

### 3. ConstructOptionalQuery
CONSTRUCT with OPTIONAL clauses (outer joins).

**Characteristics:**
- Query type: Pattern with optional bindings
- Knowledge graph size: 100 entities
- Tests handling of UNDEF values in output

**Relevance for thesis:**
- Analyzes performance of optional pattern handling
- Tests UNDEF value propagation in templates
- Important for completeness analysis

### 4. ConstructScalingBenchmark
Performance scaling analysis with growing result sets.

**Characteristics:**
- Knowledge graph sizes: 10, 50, 100, 200 entities
- All export formats measured
- Shows O(n) characteristics

**Table Structure:**
```
        | 10 entities | 50 entities | 100 entities | 200 entities
--------|-------------|-------------|--------------|-------------
TSV     | ...         | ...         | ...          | ...
CSV     | ...         | ...         | ...          | ...
Turtle  | ...         | ...         | ...          | ...
JSON    | ...         | ...         | ...          | ...
```

**Thesis section:** Scalability analysis, performance modeling

### 5. ConstructTemplateComplexity
Impact of constructing multiple triples per result row.

**Characteristics:**
- Measures: 1, 2, 3, and 5 output triples
- Fixed KG size: 100 entities
- Shows linear scaling with template size

**Measurements:**
- Single triple template: baseline
- Two triples: ~2x overhead?
- Three triples: ~3x overhead?
- Five triples (with repetition): Shows pattern repetition costs

**Thesis section:** Template processing efficiency, output format overhead

### 6. ConstructKGSizeImpact
Performance variation with increasing knowledge graph size.

**Characteristics:**
- KG sizes: 50, 100, 200, 500, 1000 entities
- Fixed query: Simple name extraction
- Shows scalability to larger datasets

**Thesis section:** Scalability analysis, industrial applicability

### 7. ConstructExportFormatComparison
Detailed analysis of export format performance.

**Characteristics:**
- Fixed query with 3 output triples
- KG size: 150 entities
- Detailed metadata per format

**Expected findings:**
- TSV/CSV: Fastest, simpler format
- Turtle: Slower, RDF-specific encoding overhead
- QLeverJSON: Structured format with additional metadata

**Thesis section:** Format efficiency analysis, implementation details

## Building the Benchmarks

### Prerequisites

```bash
# Install required system dependencies
sudo apt-get install libicu-dev libboost-all-dev openssl libssl-dev

# Or use conan for dependency management
conan install . --build=missing
```

### Build Steps

```bash
cd /path/to/qlever

# Build
make build

# Verify build success
ls -lh build/ConstructBenchmark

# Run the benchmark
./build/ConstructBenchmark
```

## Running the Benchmarks

### Basic Execution

```bash
# Run all CONSTRUCT benchmarks
./ConstructBenchmark

# Output will be generated in JSON and text formats
# Results are timestamped and include all metadata
```

### Generate Results for Paper

```bash
# Run and save results to file
./ConstructBenchmark > construct_benchmark_results.json

# Or with timestamping
./ConstructBenchmark > construct_benchmark_results_$(date +%Y%m%d_%H%M%S).json
```

### Repeat for Multiple Runs (Statistical Validity)

```bash
#!/bin/bash
# Run 5 iterations for statistical analysis

for i in {1..5}; do
  echo "Run $i..."
  ./ConstructBenchmark > results_run_${i}.json
done

# Combine and analyze results
# Use statistical tools to compute means, std dev, confidence intervals
```

## Interpreting Results

### Key Metrics

Each benchmark produces measurements with:

1. **Execution Time (milliseconds)**
   - Includes query planning, execution, and export
   - Represents end-to-end performance
   - Should be averaged across multiple runs

2. **Metadata**
   - Query type description
   - Knowledge graph characteristics
   - Template complexity
   - Export format used

### Analysis Techniques for Thesis

#### 1. Performance Scaling Analysis
```
Plot: Execution Time vs. KG Size
Data source: ConstructKGSizeImpact, ConstructScalingBenchmark

Expected pattern:
- Linear or near-linear scaling: O(n) algorithm
- Super-linear scaling: O(n log n) or worse
- Plateauing: I/O or memory bound
```

#### 2. Format Efficiency Comparison
```
Bar chart: Export format overhead
Data source: ConstructExportFormatComparison

Questions to answer:
- Which format is fastest? By how much?
- Does format speed correlate with output size?
- What's the overhead of RDF-specific formats (Turtle)?
```

#### 3. Template Complexity Impact
```
Line chart: Template triple count vs. time
Data source: ConstructTemplateComplexity

Expected findings:
- Roughly linear scaling with template size
- Constant overhead per template triple
- Optimization opportunities in template evaluation
```

#### 4. Query Complexity Impact
```
Bar chart: Query type performance comparison
Data source: Multiple benchmarks (Simple, Filtered, Optional)

Questions:
- How much overhead for FILTER evaluation?
- Cost of OPTIONAL (outer join) vs. inner joins?
- Is optimization effective?
```

## Integration with Thesis Chapters

### Chapter: Query Execution
- Reference: ConstructSimpleQuery, ConstructFilteredQuery
- Use to show end-to-end query execution performance
- Compare with SELECT performance if available

### Chapter: Query Optimization
- Reference: ConstructFilteredQuery performance improvement
- Analyze filter push-down effectiveness
- Show optimization impact on complex queries

### Chapter: CONSTRUCT Implementation
- Reference: All benchmarks
- Provide empirical validation of design choices
- Show trade-offs between different implementation approaches

### Chapter: Scalability
- Reference: ConstructKGSizeImpact, ConstructScalingBenchmark
- Demonstrate linear or near-linear scaling
- Industrial applicability of CONSTRUCT implementation

### Chapter: Export Formats
- Reference: ConstructExportFormatComparison
- Analyze format-specific overhead
- Design decisions for each format

## Expected Results (Hypothetical for Planning)

Based on typical RDF system performance characteristics:

### Scaling Behavior
```
KG Size: 10 entities    -> ~1-5 ms total
KG Size: 100 entities   -> ~5-15 ms total
KG Size: 1000 entities  -> ~50-150 ms total
```

### Format Comparison (fixed 100 entity KG, 3 triple template)
```
TSV:        10-15 ms  (baseline)
CSV:        10-15 ms  (similar to TSV)
Turtle:     15-30 ms  (RDF encoding overhead)
QLeverJSON: 12-20 ms  (structured format)
```

### Template Impact (100 entity KG)
```
1 triple:   10 ms   (baseline)
2 triples:  15 ms   (+50%)
3 triples:  20 ms   (+100%)
5 triples:  30 ms   (+200%)
```

## Customizing Benchmarks for Your Research

### Add New Query Types

Edit `ConstructBenchmark.cpp` to add custom queries:

```cpp
class ConstructCustomQuery : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "Your custom benchmark name";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;

    // Your benchmark code here
    const std::string kg = createScalableKG(yourSize, triplesPerEntity);
    const std::string query = "Your SPARQL query";

    auto& measurement = results.addMeasurement(
        "Your measurement name", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::qleverJson);
          (void)result;
        });

    return results;
  }
};

// Register at the end of the file
AD_REGISTER_BENCHMARK(ConstructCustomQuery);
```

### Modify Knowledge Graph Generation

The `createScalableKG()` function creates synthetic RDF data. Customize it:

```cpp
// Current: Creates name, type, and relatedTo predicates
// Modify to add more realistic RDF patterns specific to your research
```

### Add Comparison with Other Systems

To compare with competing implementations:

1. Export benchmark queries to SPARQL files
2. Run on alternative systems (Apache Jena, RDF3X, etc.)
3. Compare results in thesis table

## Troubleshooting

### Build Issues

**Error: ICU library not found**
```bash
# Solution: Install ICU development headers
sudo apt-get install libicu-dev
```

**Error: Boost library not found**
```bash
# Solution: Install Boost development headers
sudo apt-get install libboost-all-dev
```

**Error: OpenSSL not found**
```bash
# Solution: Install OpenSSL development headers
sudo apt-get install libssl-dev
```

### Runtime Issues

**Segmentation fault when running benchmarks**
- Ensure the build directory is clean: `make clean && make build`
- For debug symbols: Use CMake directly with `-DCMAKE_BUILD_TYPE=Debug`

**Out of memory errors**
- Reduce `ConstructKGSizeImpact` sizes (currently 50-1000)
- Add memory limiting flags to CMake

## References for Thesis Writing

### Format Specifications
- **Turtle**: https://www.w3.org/TR/turtle/
- **SPARQL Results JSON**: https://www.w3.org/TR/sparql11-results-json/
- **SPARQL CONSTRUCT**: https://www.w3.org/TR/sparql11-query/#construct

### Related Work to Cite
- Virtuoso RDF engine performance comparisons
- Apache Jena CONSTRUCT optimizations
- RDF3X benchmark methodology

### Performance Analysis Best Practices
- Run benchmarks multiple times (5-10 iterations)
- Report means and standard deviations
- Account for cache warmup effects
- Test on consistent hardware/OS configuration

## Future Enhancements

### Benchmarks to Consider Adding
1. **Union queries**: `{ ?s ?p ?o } UNION { ?s ?p ?o }`
2. **Nested patterns**: Multi-level pattern complexity
3. **Large templates**: 10+ triple patterns
4. **Real-world datasets**: DBpedia, Wikidata excerpts
5. **Concurrent queries**: Multiple simultaneous CONSTRUCT queries

### Analysis Tools
Consider implementing scripts to:
- Automatically generate comparison charts
- Compute statistical significance
- Generate thesis-ready tables/figures
- Compare across multiple runs/systems

## Contact and Questions

For questions about the benchmark suite or integrating results into your thesis:
- Review the CLAUDE.md documentation
- Check ExportQueryExecutionTreesTest.cpp for more query examples
- Examine existing benchmark implementations in the benchmark/ directory

# Golden Corpus Reference Physics Suite

**EPIC 10.2 - Agent 8 Deliverable**

## Overview

The Golden Corpus is a deterministic test suite for validating QLever's SPARQL query execution against canonical reference results. It implements "Reference Physics" - a fixed set of queries with pre-computed cryptographic digests that must match exactly on every execution.

## Purpose

- **Regression Detection**: Detect any deviation from canonical query results
- **Deterministic Validation**: Replace subjective testing with cryptographic proof
- **CI/CD Integration**: Fail builds immediately when query behavior diverges
- **Reference Standard**: Establish ground truth for SPARQL 1.1 compliance

## Structure

```
golden_corpus/
├── manifest.json              # Authoritative query registry with digests
├── q01_basic_triple.sparql    # W3C SPARQL 1.1: Basic triple pattern
├── q02_filter.sparql          # W3C SPARQL 1.1: FILTER clause
├── q03_optional.sparql        # W3C SPARQL 1.1: OPTIONAL clause
├── q04_union.sparql           # W3C SPARQL 1.1: UNION clause
├── q05_distinct.sparql        # W3C SPARQL 1.1: DISTINCT modifier
├── q06_order_by.sparql        # W3C SPARQL 1.1: ORDER BY clause
├── q07_aggregate_count.sparql # W3C SPARQL 1.1: COUNT aggregate
├── q08_group_by.sparql        # W3C SPARQL 1.1: GROUP BY clause
├── q09_subquery.sparql        # W3C SPARQL 1.1: Subquery
├── q10_regex.sparql           # W3C SPARQL 1.1: REGEX function
├── README.md                  # This file
└── AGENT8_GOLDEN_CORPUS.receipt  # Construction receipt
```

## Manifest Format

```json
{
  "version": "1.0.0",
  "source": "W3C SPARQL 1.1 Query Language",
  "queries": [
    {
      "id": "q01_basic_triple",
      "file": "q01_basic_triple.sparql",
      "description": "Basic triple pattern - W3C SPARQL 1.1",
      "digest": "PENDING_BASELINE_COMPUTATION"
    }
  ]
}
```

## Usage

### Running the Test Suite

```bash
# Build with golden corpus test
make build

# Run all golden corpus tests
ctest -R GoldenCorpusTest -V

# Or via CMake target (once added)
make test-golden-corpus
```

### Computing Baseline Digests (First Time Setup)

**Prerequisites:**
1. Build QLever with test harness
2. Load LUBM(1,0) or equivalent RDF test dataset into QLever instance

**Steps:**
```bash
# Run baseline computation (currently DISABLED)
ctest -R GoldenCorpusTest -V --gtest_also_run_disabled_tests \
      --gtest_filter="*ComputeBaselineDigests"

# Output will show digests for each query - example:
# Query: q01_basic_triple
#   File: q01_basic_triple.sparql
#   Digest: a1b2c3d4e5f6...

# Update manifest.json with computed digests
# Replace "PENDING_BASELINE_COMPUTATION" with actual digests

# Enable validation test
# In GoldenCorpusTest.cpp, remove DISABLED_ from:
#   TEST_F(GoldenCorpusTest, DISABLED_ValidateGoldenQueryResults)
# Change to:
#   TEST_F(GoldenCorpusTest, ValidateGoldenQueryResults)

# Rebuild and run validation
make build
ctest -R GoldenCorpusTest -V
```

### Adding New Queries

1. Create new `.sparql` file in `golden_corpus/` directory
2. Add entry to `manifest.json` with `"digest": "PENDING_BASELINE_COMPUTATION"`
3. Run baseline computation to generate digest
4. Update manifest with computed digest
5. Commit both query file and updated manifest

## Canonical Result Format

Query results are serialized in canonical form before hashing:

- **Format**: TSV (Tab-Separated Values)
- **Encoding**: UTF-8
- **Line endings**: LF (Unix)
- **Column order**: Deterministic (alphabetical by variable name)
- **Row order**: Sorted (stable sort on all columns)
- **Value serialization**: Consistent RDF term representation

## Digest Algorithm

- **Current**: SHA256 (via OpenSSL EVP interface)
- **Preferred**: BLAKE3 (per specification)
- **Location**: `src/util/CryptographicHashUtils.h`

To upgrade to BLAKE3:
1. Add BLAKE3 library dependency to CMake
2. Extend `CryptographicHashUtils.h` with BLAKE3 hasher
3. Update `GoldenCorpusTest.cpp` to use BLAKE3
4. Recompute all baseline digests
5. Update manifest.json

## Validation Rules

**STRICT ENFORCEMENT:**
- ✅ All query files must exist and be readable
- ✅ All digests must match exactly (no tolerance)
- ✅ Any mismatch fails the build immediately
- ✅ No queries can be skipped (unless explicitly disabled)

**ON FAILURE:**
- Test reports expected vs actual digest
- Test reports query ID and file
- Build fails (non-zero exit code)
- Developer must investigate divergence

## Test Implementation

**File**: `/home/user/qlever/test/GoldenCorpusTest.cpp`

**Test Cases:**
1. `ManifestLoads` - Validates manifest.json structure
2. `QueryFilesExist` - Verifies all query files are present
3. `ValidateGoldenQueryResults` - Validates digests (currently DISABLED)
4. `ComputeBaselineDigests` - Helper to generate digests (DISABLED)

**Integration**: CMake test target via `addLinkAndDiscoverTest`

## Monoidal Composition

This suite composes independently:
- No dependencies on other test infrastructure
- Queries can be added incrementally
- Each query validation is independent (parallelizable)
- Manifest is append-only for new queries
- Clean separation of concerns

## Future Enhancements

1. **LUBM Integration**: Add LUBM 100-query stress suite
2. **BLAKE3 Migration**: Upgrade from SHA256 to BLAKE3
3. **Dataset Management**: Automated LUBM(1,0) dataset loading
4. **Parallel Execution**: Leverage QLever's concurrency for query execution
5. **Result Caching**: Cache canonical results for faster validation
6. **Differential Analysis**: On mismatch, show result diff (not just digest)

## References

- W3C SPARQL 1.1: https://www.w3.org/TR/sparql11-query/
- LUBM Benchmark: http://swat.cse.lehigh.edu/projects/lubm/
- BLAKE3: https://github.com/BLAKE3-team/BLAKE3

## Agent 8 Receipt

See `AGENT8_GOLDEN_CORPUS.receipt` for complete construction metadata, including:
- Timestamp of construction
- Harness implementation hash
- Detailed inventory
- Convergence notes

---

**Status**: Infrastructure complete, awaiting baseline computation
**Last Updated**: 2026-01-02
**Maintained By**: EPIC 10.2 Construction Seal (Agent 8)

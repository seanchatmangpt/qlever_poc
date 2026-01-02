# Golden Corpus: Dual Manifest Strategy (EPIC 10.2)

## Overview

The golden corpus uses **two separate manifestfiles** to represent different validation concerns:

1. **query_manifest.json** - Golden Query Reference Physics (Agent 8)
2. **simd_determinism_manifest.json** - SIMD vs Scalar Equivalence (Agent 3)

## Rationale (Convergence Decision 1)

Per the EPIC 10.2 Convergence Phase, maintaining separate manifests provides:

- **Clarity**: Each manifest serves a distinct validation purpose
- **Modularity**: Agents can evolve independently (e.g., add more LUBM queries to SIMD manifest)
- **Fail-Closed**: Validation failures are immediately traceable to specific test suites
- **Extensibility**: Future agents can add manifests without collision (e.g., `n3_determinism_manifest.json`)

## Manifest 1: query_manifest.json (Agent 8 - Golden Queries)

**Purpose**: Define the authoritative result digests for W3C SPARQL 1.1 test queries

**Format**:
```json
{
  "version": "1.0.0",
  "queries": [
    {
      "id": "q01_basic_triple",
      "file": "queries/q01_basic_triple.sparql",
      "digest": "PENDING_BASELINE_COMPUTATION"
    },
    ...
  ]
}
```

**Test Harness**: `GoldenCorpusTest.cpp` → Validates query results against digests

**Next Steps**:
1. Execute 10 W3C SPARQL queries against reference QLever instance
2. Compute BLAKE3 digests of canonical result sets
3. Populate `digest` field for each query
4. Enable `ValidateGoldenQueryResults` test case

## Manifest 2: simd_determinism_manifest.json (Agent 3 - SIMD Equivalence)

**Purpose**: Ensure SIMD-vectorized parsing produces bit-identical results to scalar fallback

**Format**:
```json
{
  "w3c_sparql_queries": {
    "basic_select_001": {
      "query_file": "queries/w3c/basic_select_001.sparql",
      "data_file": "data/w3c/basic_triples.nt",
      "expected_result_blake3": "pending_authoritative_run",
      "determinism_validation_protocol": [...]
    }
  },
  "lubm_stress_queries": {...},
  "determinism_validation_protocol": {...}
}
```

**Test Harness**: (Currently planned in `test/engine/ingress/test_golden_corpus_determinism.cpp`)

**Validation Contract**: `bit_parity(SIMD_Path, Scalar_Path) == TRUE` for all queries

**Next Steps**:
1. Implement `SimdJsonIngressWrapper.cpp` with SIMD parsing
2. Implement scalar fallback path (`fallback_parse()`)
3. Execute queries via both paths with identical data
4. Compute digests and compare for equivalence
5. Record results in manifest

## Directory Structure

```
test/golden_corpus/
├── query_manifest.json              # Agent 8: Golden queries
├── simd_determinism_manifest.json   # Agent 3: SIMD equivalence
├── README.md                        # Overview
├── MANIFEST_STRATEGY.md             # This file
├── compute_baseline.sh              # Script to compute query baselines
├── data/
│   ├── basic_triples.nt
│   └── order_data.nt
├── queries/
│   ├── q01_basic_triple.sparql
│   ├── q02_filter.sparql
│   └── ... (q03-q10)
└── results/
    └── (populated by test harness)
```

## Validation Workflow

### Phase 1: Query Baseline Computation
```
1. Run: compute_baseline.sh
2. Executes: GoldenCorpusTest.cpp → ValidateGoldenQueryResults
3. Output: query_manifest.json with computed digests
4. Status: PASS if all digests match expected values
```

### Phase 2: SIMD Determinism Validation
```
1. Build: test/engine/ingress/test_golden_corpus_determinism.cpp
2. Execute: Both SIMD and scalar parsing paths
3. Validate: Digests match for all queries
4. Output: simd_determinism_manifest.json with results
5. Status: PASS if bit_parity == TRUE for all
```

## Integration with EPIC 10.2 Seal

Both manifests feed into the final `.phase.lock` artifact:

```cmake
# In PhaseLock.cmake (Agent 10)
compute_digest(
  query_manifest.json
  → golden_corpus_query_digest
)
compute_digest(
  simd_determinism_manifest.json
  → golden_corpus_simd_digest
)
# Both included in final manifest.json
```

## Migration Notes

- **Deprecated**: `/home/user/qlever/tests/golden_corpus/` (removed; consolidated into `test/golden_corpus/`)
- **Backward Compatibility**: Old path no longer exists; all references updated to `test/golden_corpus/`
- **No Data Loss**: All Agent 3 data files preserved in `test/golden_corpus/data/`

## See Also

- `docs/manifest-schema-family.md` - Canonical manifest schema documentation
- `docs/fail-closed-enforcement.md` - Layered enforcement (Agents 2, 6, 7)
- EPIC 10.2 Convergence Decisions (complete specification)

# Manifest Schema Family (EPIC 10.2)

## Overview

EPIC 10.2 uses **four specialized manifest files**, each serving a distinct validation purpose. They share a **canonical JSON base structure** but specialize for their domain.

## Canonical Base Schema

All manifests conform to this base structure:

```json
{
  "version": "1.0.0",
  "algorithm": "BLAKE3|SHA256",
  "created_at": "ISO8601 timestamp",
  "description": "Human-readable purpose",
  "digest_registry": {
    "entity_id": "hash_value",
    ...
  }
}
```

## Manifest 1: query_manifest.json (Golden Query Results)

**Location**: `test/golden_corpus/query_manifest.json`
**Agent**: Agent 8 (Golden Query Harness)
**Purpose**: Authorize result digests for W3C SPARQL 1.1 reference queries

**Specialized Schema**:
```json
{
  "version": "1.0.0",
  "algorithm": "BLAKE3",
  "created_at": "2026-01-02T...",
  "description": "Golden Query Reference Physics - W3C SPARQL 1.1 test suite",
  "queries": [
    {
      "id": "q01_basic_triple",
      "file": "queries/q01_basic_triple.sparql",
      "description": "Basic triple pattern matching",
      "digest": "pending_baseline_computation|blake3_hex_64_chars"
    }
  ]
}
```

**Validation Rules**:
- All `digest` fields must be 64-character hex strings (BLAKE3) or "PENDING_BASELINE_COMPUTATION"
- Query result sets serialized in **canonical form** (TSV, sorted, deterministic column order)
- Test fails if any digest mismatches

**CI Gate**: `GoldenCorpusTest.cpp` → `ValidateGoldenQueryResults`

---

## Manifest 2: simd_determinism_manifest.json (SIMD Equivalence)

**Location**: `test/golden_corpus/simd_determinism_manifest.json`
**Agent**: Agent 3 (Ingress Determinism)
**Purpose**: Validate SIMD-vectorized parsing ≡ scalar fallback (bit-for-bit equivalence)

**Specialized Schema**:
```json
{
  "version": "1.0.0",
  "algorithm": "BLAKE3",
  "description": "SIMD vs Scalar Equivalence Validation",
  "w3c_sparql_queries": {
    "basic_select_001": {
      "query_file": "queries/w3c/basic_select_001.sparql",
      "data_file": "data/w3c/basic_triples.nt",
      "expected_result_blake3": "pending|hash",
      "simd_result_blake3": "pending|hash",
      "scalar_result_blake3": "pending|hash",
      "determinism_status": "not_yet_validated|pass|fail"
    }
  },
  "determinism_validation_contract": {
    "rule": "bit_parity(SIMD_Path, Scalar_Path) == TRUE",
    "forbidden_content": ["pointers", "thread_ids", "timestamps", "floats"]
  }
}
```

**Validation Rules**:
- SIMD and scalar results must produce **identical digests**
- Detects forbidden content (non-deterministic values)
- Critical queries (ORDER BY, DISTINCT) marked with `"criticality": "HIGH"`

**CI Gate**: `test/engine/ingress/test_golden_corpus_determinism.cpp`

---

## Manifest 3: build/manifest.json (Build Artifacts - Phase Lock)

**Location**: `${CMAKE_BINARY_DIR}/manifest.json`
**Agent**: Agent 10 (The Phase Lock)
**Purpose**: Cryptographic proof of deterministic "tape-out" build

**Specialized Schema**:
```json
{
  "version": "1.0.0",
  "algorithm": "BLAKE3",
  "created_at": "ISO8601 timestamp",
  "engine_version": "1.0.0",
  "binary_digest": "blake3_hash_of_qleverest_engine",
  "logic_digest": "blake3_hash_of_src_codebase",
  "dependency_receipt": {
    "libicu_uc": "hash",
    "libicu_i18n": "hash",
    "simdjson": "hash"
  },
  "golden_corpus_digests": {
    "query_manifest": "hash_of_query_manifest.json",
    "simd_determinism_manifest": "hash_of_simd_determinism_manifest.json"
  },
  "reproducibility_proof": {
    "build_1_sha256": "manifest_hash",
    "build_2_sha256": "manifest_hash",
    "is_deterministic": true
  }
}
```

**Validation Rules**:
- All digest fields must be present and valid
- `is_deterministic` must be **true**
- Linked to `.phase.lock` witness file (immutable)
- Build refuses to link if manifest integrity fails

**CI Gate**: Agent 4's `scripts/test-build-determinism.sh` + Agent 10's `cmake/modules/PhaseLock.cmake`

---

## Manifest 4: .phase.lock (Build Witness - Immutability Proof)

**Location**: `${CMAKE_BINARY_DIR}/.phase.lock`
**Agent**: Agent 10 (The Phase Lock)
**Purpose**: Immutable witness; prevents tampering post-build

**Specialized Schema**:
```json
{
  "phase_lock": {
    "manifest_sha256": "hash_of_manifest.json",
    "lock_timestamp": "ISO8601 timestamp",
    "chain_hash": "sha256(manifest_sha256 + lock_timestamp)",
    "read_only": true
  }
}
```

**Validation Rules**:
- File must be read-only (chmod 444)
- Chain hash prevents modification of manifest.json without detection
- Signature: `chain_hash = SHA256(manifest_sha256 + lock_timestamp)`
- Runtime verification: `verify_phase_lock()` in `src/util/PhaseLockVerifier.cpp`

**CI Gate**: `PhaseLockVerifier` tests in `test/util/PhaseLockVerifierTest.cpp`

---

## Manifest Relationships

```
test/golden_corpus/
├── query_manifest.json
│   └── Contains: Query ID → Result Digest
│
└── simd_determinism_manifest.json
    └── Contains: Query ID → (SIMD_Digest, Scalar_Digest, Parity_Result)

        Both digests feed into:
        ↓

build/manifest.json (Phase Lock)
├── golden_corpus_digests:
│   ├── query_manifest: hash_of_query_manifest.json
│   └── simd_determinism_manifest: hash_of_simd_determinism_manifest.json
├── binary_digest: hash_of_engine_binary
├── reproducibility_proof: (build1_hash, build2_hash, is_deterministic)
│
└── Witnessed by:

        .phase.lock (Immutable)
        └── chain_hash: SHA256(manifest.json_hash + timestamp)
```

---

## Validation Workflow

### Phase 1: Query Baseline Computation
```
GoldenCorpusTest.cpp
  ↓ reads: query_manifest.json
  ↓ executes: 10 W3C SPARQL queries
  ↓ computes: BLAKE3 digests of results
  ↓ updates: query_manifest.json with digests
  ↓ exit: 0 (PASS) or 1 (FAIL)
```

### Phase 2: SIMD Determinism Validation
```
test_golden_corpus_determinism.cpp
  ↓ reads: simd_determinism_manifest.json
  ↓ executes: queries via SIMD path + scalar path
  ↓ compares: BLAKE3 digests (must match)
  ↓ updates: simd_determinism_manifest.json
  ↓ exit: 0 (PASS) or 1 (FAIL - divergence detected)
```

### Phase 3: Build Determinism + Phase Lock
```
cmake/modules/PhaseLock.cmake
  ↓ reads: query_manifest.json, simd_determinism_manifest.json
  ↓ computes: digest of each
  ↓ computes: digest of engine binary, src/ codebase
  ↓ creates: build/manifest.json with all digests
  ↓ creates: .phase.lock with chain hash
  ↓ enforces: Build refuses to link if manifest tampered
```

### Phase 4: Reproducibility Gate
```
scripts/test-build-determinism.sh
  ↓ builds: Clean build 1
  ↓ computes: SHA256(manifest)
  ↓ builds: Clean build 2 (independent)
  ↓ computes: SHA256(manifest)
  ↓ asserts: build_1_hash == build_2_hash
  ↓ exit: 0 (deterministic) or 1 (non-deterministic)
```

---

## Specialization Decision (Convergence Decision 5)

**Why separate manifests?**

| Manifest | Domain | Validator | Use Case |
|----------|--------|-----------|----------|
| query_manifest.json | Golden queries | GoldenCorpusTest | Regression detection |
| simd_determinism_manifest.json | SIMD equivalence | DeterminismTest | Implementation verification |
| build/manifest.json | Build artifacts | PhaseLock | Seal integrity |
| .phase.lock | Witness | PhaseLockVerifier | Tamper detection |

**Not unified** because:
- Different validation rules
- Different test harnesses
- Different failure modes
- Different update frequencies (query baseline once; simd manifest per build)
- Clarity: No need to parse unrelated fields

---

## Hash Algorithm

**Current**: SHA256 (CMake `file()` command, OpenSSL)
**Target**: BLAKE3 (per EPIC 10.2 specification)
**Migration**: Define `PHASE_LOCK_HASH_ALGO` CMake variable for single-point swap

---

## See Also

- `test/golden_corpus/MANIFEST_STRATEGY.md` - Dual manifest rationale
- `docs/fail-closed-enforcement.md` - Validation layers
- `docs/dual-gate-strategy.md` - Build + runtime determinism gates
- EPIC 10.2 Convergence Decisions (complete policy)

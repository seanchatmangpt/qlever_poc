# Workload Packs - Deterministic Cross-Machine Testing

## Overview

Workload packs are deterministic, pinned collections of SPARQL queries and RDF data designed for cross-machine verification testing. Each workload pack is fully reproducible: given the same manifest, any machine can reconstruct the exact workload and verify identical results.

## Manifest Format

Each workload pack contains a `manifest.json` file with the following structure:

### Top-Level Fields

- **workload_id**: `string` - Deterministic ID computed as SHA256 hash of all input file hashes concatenated
- **workload_name**: `string` - Human-readable name of the workload pack
- **version**: `string` - Semantic version (MAJOR.MINOR.PATCH)
- **created_at**: `string` - ISO 8601 timestamp (UTC)
- **description**: `string` - Human-readable description

### RDF File References

```json
"rdf_file_references": [
  {
    "file_name": "sample_data.ttl",
    "file_path": "sample_data.ttl",
    "format": "turtle",
    "sha256": "dcde8fab296f7c313eafb5c95ac33002dcc8974fad8b3c746be040636764e971",
    "size_bytes": 830,
    "triple_count": 17,
    "deterministic": true
  }
]
```

Fields:
- **file_name**: Name of the RDF file
- **file_path**: Relative path to the file within the workload pack
- **format**: RDF serialization format (turtle, ntriples, rdfxml, etc.)
- **sha256**: SHA256 digest of the file contents (for pinning)
- **size_bytes**: File size in bytes
- **triple_count**: Number of RDF triples (optional, for validation)
- **deterministic**: Boolean indicating if file has deterministic ordering

### Query List

```json
"query_list": [
  {
    "query_id": "q01_select_all_persons",
    "query_file": "query_01_select_all_persons.sparql",
    "sha256": "fa3869398bc24c4652a18537ae264e7319f6edd28610f93bdf39b697028001d5",
    "execution_order": 0,
    "expected_result_digest": "0000000000000000000000000000000000000000000000000000000000000000",
    "expected_result_count": 3,
    "description": "Select all persons with name and age"
  }
]
```

Fields:
- **query_id**: Unique identifier for the query
- **query_file**: Relative path to the SPARQL query file
- **sha256**: SHA256 digest of the query file (for pinning)
- **execution_order**: Integer specifying execution sequence (0-indexed)
- **expected_result_digest**: BLAKE3 digest of expected query results (64 hex chars)
- **expected_result_count**: Number of expected result rows (optional validation)
- **description**: Human-readable description

### Expected Modes

```json
"expected_modes": [
  {
    "mode": "baseline",
    "description": "Cold start with empty cache",
    "cache_enabled": false,
    "expected_cache_hits": 0,
    "expected_cache_misses": 3
  },
  {
    "mode": "cached",
    "description": "Warm cache, all queries should hit",
    "cache_enabled": true,
    "expected_cache_hits": 3,
    "expected_cache_misses": 0
  },
  {
    "mode": "replay",
    "description": "Replay mode with strict digest verification",
    "cache_enabled": true,
    "replay_mode": "Strict",
    "divergence_policy": "ABORT"
  }
]
```

Modes:
- **baseline**: Cold start, no caching, establishes baseline performance
- **cached**: Warm cache, validates cache correctness
- **replay**: Strict replay with digest verification, validates determinism

### Epoch Key

```json
"epoch_key": {
  "key_format": "hex",
  "key_value": "epoch-2026-01-02-deterministic-pack-1",
  "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
  "pinned": true,
  "description": "Deterministically pinned epoch key for reproducibility"
}
```

The epoch key uniquely identifies the workload pack's temporal context and ensures cache isolation across different workload pack runs.

### Pinned Dependencies

```json
"pinned_dependencies": {
  "qlever_version_min": "0.1.0",
  "rust_toolchain": "stable",
  "cbor_format": "CBOR-tagged",
  "digest_algorithm": "BLAKE3",
  "file_hash_algorithm": "SHA256"
}
```

Documents all dependencies to ensure reproducibility.

### Reproducibility Metadata

```json
"reproducibility": {
  "deterministic": true,
  "cross_platform": true,
  "isolation_required": true,
  "seed_value": 42,
  "notes": "All inputs are pinned. Running on any machine with the same manifest should produce the same workload_id."
}
```

### Validation Proof

```json
"validation_proof": {
  "manifest_sha256": "33e3151bfd0efa7fd8e1e173a4eff7ed6aeca95f343833dd41f6250518f8d3f7",
  "all_files_present": true,
  "all_hashes_verified": true
}
```

Self-validation metadata for the manifest itself.

## How to Reproduce on Any Machine

### Step 1: Verify Manifest

```bash
cd qlever-verification/workload_packs/deterministic_pack_1
sha256sum manifest.json
# Should output: 33e3151bfd0efa7fd8e1e173a4eff7ed6aeca95f343833dd41f6250518f8d3f7
```

### Step 2: Verify All Files Present

```bash
ls -1
# Should show:
# manifest.json
# sample_data.ttl
# query_01_select_all_persons.sparql
# query_02_count_cities.sparql
# query_03_knows_graph.sparql
```

### Step 3: Verify File Hashes

```bash
sha256sum sample_data.ttl
# Should output: dcde8fab296f7c313eafb5c95ac33002dcc8974fad8b3c746be040636764e971

sha256sum query_01_select_all_persons.sparql
# Should output: fa3869398bc24c4652a18537ae264e7319f6edd28610f93bdf39b697028001d5

sha256sum query_02_count_cities.sparql
# Should output: f98651608bc2602f9fad21544339829344b12c1fd729386fccdb47ce9adc154b

sha256sum query_03_knows_graph.sparql
# Should output: c87da7d4d45952fd6a845596bf9ee8ffbd1a19732d3449494290b486a83d1057
```

### Step 4: Verify Workload ID

Compute the workload_id by concatenating all file hashes and hashing the result:

```bash
echo -n "dcde8fab296f7c313eafb5c95ac33002dcc8974fad8b3c746be040636764e971fa3869398bc24c4652a18537ae264e7319f6edd28610f93bdf39b697028001d5f98651608bc2602f9fad21544339829344b12c1fd729386fccdb47ce9adc154bc87da7d4d45952fd6a845596bf9ee8ffbd1a19732d3449494290b486a83d1057" | sha256sum
# Should output: ab456c62a9ab2bd1c7e65758c495d7e7189f8b7ac70517bf88c86064dd9c42d3
```

This workload_id MUST match the `workload_id` field in `manifest.json`.

### Step 5: Run Workload

Use the verification harness to run the workload pack:

```bash
cd qlever-verification
cargo run --bin qlever-verification-harness -- \
  --workload-pack workload_packs/deterministic_pack_1 \
  --mode baseline
```

Expected output:
- 3 queries executed in order
- Result digests computed with BLAKE3
- Cache behavior logged
- Receipt generated

### Step 6: Verify Results

The verification harness will:
1. Load all queries in `execution_order`
2. Execute against the RDF data
3. Compute BLAKE3 digest of each result
4. Compare against `expected_result_digest` in manifest
5. Log cache behavior (hits/misses)
6. Generate deterministic receipt

If running on a different machine produces different results, the system will ABORT with divergence details.

## Creating New Workload Packs

### Template Structure

```
workload_packs/
  my_pack_name/
    manifest.json          # Required: Metadata and pinned dependencies
    sample_data.ttl        # Required: RDF data (can be multiple files)
    query_01_*.sparql      # Required: At least 1 query
    query_02_*.sparql      # Optional: Additional queries
    ...
```

### Steps

1. **Create RDF data**: Use deterministic ordering (sorted triples)
2. **Create SPARQL queries**: Number them with execution order
3. **Compute file hashes**: `sha256sum *.ttl *.sparql`
4. **Compute workload_id**: Hash of concatenated file hashes
5. **Create manifest.json**: Fill in all fields with pinned data
6. **Verify**: Run steps 1-4 above to ensure reproducibility

### Validation Rules

- All file paths must be relative to the workload pack directory
- All SHA256 digests must be 64 hex characters
- All BLAKE3 digests must be 64 hex characters
- `execution_order` must be sequential starting from 0
- `workload_id` must match hash of concatenated input hashes
- Manifest must parse as valid JSON

## Determinism Guarantees

A workload pack is **deterministic** if and only if:

1. All input files are pinned with SHA256 hashes
2. RDF data has deterministic serialization order
3. SPARQL queries produce deterministic result ordering (use ORDER BY)
4. Epoch key is pinned
5. All dependencies are versioned
6. Same manifest on different machines produces same workload_id

## Integration with Verification Subsystems

Workload packs integrate with:

- **qlever-digest-verifier**: Validates BLAKE3 digests
- **qlever-cache-verifier**: Validates cache behavior (hits/misses)
- **qlever-replay-verifier**: Replays queries and detects divergence
- **qlever-epoch-verifier**: Validates epoch isolation
- **qlever-regression-verifier**: Detects performance regressions

## Manifest Versioning

Manifest format follows semantic versioning:
- **MAJOR**: Breaking changes to manifest schema
- **MINOR**: Backward-compatible additions
- **PATCH**: Bug fixes, clarifications

Current version: **1.0.0**

## References

- EPIC 11 Invariant C1: Workload pack format specification
- EPIC 11 Invariant C3: CBOR serialization for replay workloads
- BLAKE3 specification: https://github.com/BLAKE3-team/BLAKE3-specs
- SHA256 specification: FIPS 180-4

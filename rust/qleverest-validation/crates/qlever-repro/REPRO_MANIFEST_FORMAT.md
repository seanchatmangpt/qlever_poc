# Reproduction Manifest Format

**Version**: 1
**Author**: Agent 8 (Integration Phase - EPIC 11)
**Date**: 2026-01-02

## Purpose

The Reproduction Manifest Format defines a portable, machine-readable specification for reproducing QLever verification results on any Linux machine. It enables deterministic reproduction of verification runs without absolute paths, user names, or machine-specific references.

## Design Goals

1. **Portability**: Commands work on any Linux machine with the required toolchain
2. **Self-Containment**: No external dependencies beyond Git checkout and toolchain
3. **Reproducibility**: Same commit + same workload = same results
4. **Copy-Paste Ready**: Generated commands can be directly pasted into a shell
5. **No Secrets**: No absolute paths, usernames, or machine-specific data

## File Structure

### repro_manifest.json

The manifest is a JSON file with the following schema:

```json
{
  "manifest_version": 1,
  "generated_at": "2026-01-02T12:00:00Z",

  "environment": {
    "machine_fingerprint": {
      "cpu_model": "Intel(R) Xeon(R) CPU E5-2680 v4",
      "cpu_features": ["sse4_2", "avx2", "popcnt"],
      "os_name": "linux",
      "os_version": "5.15.0",
      "libc_version": "2.35",
      "architecture": "x86_64"
    },
    "git_commit": "abc123def456",
    "git_branch": "main",
    "rust_version": "1.75.0",
    "cmake_version": "3.27.0",
    "cxx_compiler_version": "11.4.0",
    "build_env_vars": [
      ["RUST_BACKTRACE", "1"],
      ["CMAKE_BUILD_TYPE", "Release"]
    ]
  },

  "verdict": {
    "gate_name": "contract",
    "result": "PASS",
    "total_time_ms": 1000,
    "blocking_failures": 0,
    "advisory_failures": 0,
    "verdict_timestamp": "2026-01-02T12:00:00Z"
  },

  "toolchain": {
    "min_rust_version": "1.70.0",
    "min_cmake_version": "3.27",
    "cxx_compiler": "gcc",
    "min_cxx_version": "11.0"
  },

  "workload_pack": {
    "workload_id": "deterministic-corpus-v1",
    "workload_pack_path": "./corpus.workload.cbor",
    "workload_pack_hash": "0000...0000",
    "download_url": null
  },

  "rerun_command": "export RUST_BACKTRACE=1; cargo run -p qlever-verification-harness -- --gate contract --workload ./corpus.workload.cbor --output ./verification_results",

  "setup_script": "#!/bin/bash\n# Setup script...",

  "output_paths": [
    "./verification_results/",
    "./verification_results/receipts/",
    "./verification_results/reports/"
  ]
}
```

## Field Definitions

### manifest_version (integer)
- **Required**: Yes
- **Description**: Version of the manifest format schema
- **Current**: 1

### generated_at (string, ISO 8601)
- **Required**: Yes
- **Description**: Timestamp when the manifest was generated
- **Format**: RFC 3339 (e.g., "2026-01-02T12:00:00Z")

### environment (object)
- **Required**: Yes
- **Description**: Snapshot of the build environment from the original run

#### environment.machine_fingerprint (object)
- **cpu_model**: String identifying the CPU model
- **cpu_features**: Array of CPU feature flags (e.g., "avx2", "sse4_2")
- **os_name**: Operating system name (e.g., "linux")
- **os_version**: OS kernel version
- **libc_version**: Version of the C library (glibc or musl)
- **architecture**: CPU architecture (e.g., "x86_64", "aarch64")

#### environment.git_commit (string)
- **Required**: Yes
- **Description**: Full SHA-1 commit hash of the QLever source
- **Format**: 40-character hex string

#### environment.git_branch (string)
- **Required**: Yes
- **Description**: Git branch name from the original run

#### environment.rust_version (string)
- **Required**: Yes
- **Description**: Rust toolchain version (e.g., "1.75.0")

#### environment.cmake_version (string)
- **Required**: Yes
- **Description**: CMake version (e.g., "3.27.0")

#### environment.cxx_compiler_version (string)
- **Required**: Yes
- **Description**: C++ compiler version (GCC or Clang)

#### environment.build_env_vars (array of [string, string])
- **Required**: No
- **Description**: Environment variables that affect the build
- **Whitelist**: Only safe variables (RUST_BACKTRACE, CMAKE_BUILD_TYPE, CC, CXX, etc.)

### verdict (object)
- **Required**: Yes
- **Description**: Result summary from the original verification run

#### verdict.gate_name (string)
- **Required**: Yes
- **Description**: Name of the verification gate ("contract", "regression", "full")

#### verdict.result (string)
- **Required**: Yes
- **Description**: Overall result ("PASS", "FAIL", "TIMEOUT", "PARTIAL")

#### verdict.total_time_ms (integer)
- **Required**: Yes
- **Description**: Total execution time in milliseconds

#### verdict.blocking_failures (integer)
- **Required**: Yes
- **Description**: Count of blocking failures

#### verdict.advisory_failures (integer)
- **Required**: Yes
- **Description**: Count of advisory (non-blocking) failures

### toolchain (object)
- **Required**: Yes
- **Description**: Minimum toolchain version constraints

#### toolchain.min_rust_version (string)
- **Required**: Yes
- **Description**: Minimum Rust version required
- **Default**: "1.70.0"

#### toolchain.min_cmake_version (string)
- **Required**: Yes
- **Description**: Minimum CMake version required
- **Default**: "3.27"

#### toolchain.cxx_compiler (string)
- **Required**: Yes
- **Description**: C++ compiler name ("gcc" or "clang")
- **Default**: "gcc"

#### toolchain.min_cxx_version (string)
- **Required**: Yes
- **Description**: Minimum compiler version
- **Default**: "11.0"

### workload_pack (object)
- **Required**: Yes
- **Description**: Reference to the workload pack used in verification

#### workload_pack.workload_id (string)
- **Required**: Yes
- **Description**: Unique identifier for the workload

#### workload_pack.workload_pack_path (string)
- **Required**: Yes
- **Description**: Path to the workload pack file (relative or absolute)
- **Constraint**: Must be relative unless download_url is provided

#### workload_pack.workload_pack_hash (string)
- **Required**: Yes
- **Description**: BLAKE3 hash of the workload pack (64 hex characters)

#### workload_pack.download_url (string, nullable)
- **Required**: No
- **Description**: URL to download the workload pack
- **Format**: HTTP/HTTPS URL

### rerun_command (string)
- **Required**: Yes
- **Description**: Complete shell command to reproduce the verification
- **Constraints**:
  - No absolute paths
  - No user names
  - No machine-specific references
  - Single line (no unescaped newlines)
  - Valid shell syntax

### setup_script (string)
- **Required**: Yes
- **Description**: Bash script to prepare the environment
- **Format**: Complete bash script including shebang
- **Steps**:
  1. Git checkout of the correct commit
  2. Toolchain version verification
  3. Build step
  4. Output directory creation

### output_paths (array of strings)
- **Required**: Yes
- **Description**: Expected output directory structure
- **Constraint**: All paths must be relative

## Validation Rules

### Portability Checks

1. **No Absolute User Paths**: rerun_command must not contain `/home/`, `/root/`, or `/Users/`
2. **No Username References**: rerun_command must not contain current username or hostname
3. **Relative Workload Path**: workload_pack_path must be relative unless download_url is provided
4. **Safe Environment Variables**: Only whitelisted environment variables allowed

### Safety Checks

1. **Valid Shell Syntax**: rerun_command must pass `sh -n` validation
2. **Valid Bash Syntax**: setup_script must pass `bash -n` validation
3. **No Secrets**: No environment variables that might contain secrets (HOME, USER, PASSWORD, etc.)

### Completeness Checks

1. **Required Fields**: All required fields must be present and non-empty
2. **Hash Format**: workload_pack_hash must be 64 hex characters (BLAKE3 output)
3. **Timestamp Format**: All timestamps must be valid ISO 8601 / RFC 3339

## Usage Workflow

### 1. Generate Manifest

```bash
# From verification artifacts
cargo run -p qlever-repro -- generate \
  --verdict verdict.json \
  --environment environment.json \
  --workload corpus.workload.cbor \
  --output repro_manifest.json
```

### 2. Validate Manifest

```bash
# Validate portability
cargo run -p qlever-repro -- validate repro_manifest.json
```

### 3. Execute Reproduction

```bash
# On a different machine:

# Step 1: Run setup script
bash setup.sh  # Extracted from repro_manifest.json

# Step 2: Run rerun command
export RUST_BACKTRACE=1; cargo run -p qlever-verification-harness -- \
  --gate contract \
  --workload ./corpus.workload.cbor \
  --output ./verification_results
```

## Example: Complete Reproduction Flow

```bash
# Machine A (original run)
cd /home/user/qlever
cargo test -p qlever-cache-verifier
# Generates verdict.json, environment.json, receipts/

# Generate repro manifest
cargo run -p qlever-repro -- generate \
  --verdict ./verification_results/verdict.json \
  --environment ./verification_results/environment.json \
  --workload ./workloads/corpus.workload.cbor \
  --output ./repro_manifest.json

# Machine B (reproduction)
git clone https://github.com/seanchatmangpt/qlever.git
cd qlever

# Extract and run setup
jq -r .setup_script repro_manifest.json > setup.sh
bash setup.sh

# Copy workload pack
wget https://example.com/corpus.workload.cbor

# Run rerun command
jq -r .rerun_command repro_manifest.json | bash
```

## Security Considerations

1. **No Secrets**: Manifests must never contain passwords, tokens, or API keys
2. **Path Sanitization**: All paths sanitized to remove user-specific information
3. **Environment Variable Whitelist**: Only safe environment variables included
4. **Download Verification**: workload_pack_hash used to verify downloads

## Versioning

- **Version 1** (current): Initial format
- Future versions will maintain backward compatibility where possible
- Breaking changes will increment manifest_version

## References

- EPIC 11: Multi-Agent Parallel Implementations
- Agent 8: Rerun Command Path Generator (Integration Phase)
- QLever Verification Architecture: `/home/user/qlever/qlever-verification/`

## License

Apache-2.0 (same as QLever project)

## Contact

Issues and questions: https://github.com/seanchatmangpt/qlever/issues

# EPIC 10.2 Agent 4: Bit-for-Bit Reproducibility

**Agent Role:** Build Determinism Verification
**LOCKED SPECIFICATION:**
- Baseline: Ubuntu 22.04 LTS, GCC 12.3.0, CMake 3.25.1
- Acceptable variance: ±0% (bit-for-bit reproducibility required)
- Success metric: `is_deterministic(make_universe) == TRUE`

## Overview

This agent ensures that the QLever build system produces **deterministic** (bit-for-bit reproducible) binaries. Given identical source code and environment, the build must produce byte-for-byte identical executables and libraries.

## Components

### 1. Determinism Test Script
**Location:** `/home/user/qlever/scripts/test-build-determinism.sh`

**Purpose:** Performs two independent clean builds and verifies binary identity.

**Process:**
1. **Build 1:** Clean configuration + build in `build_determinism_1/`
2. **Compute Hashes:** SHA256 of all executables and libraries
3. **Build 2:** Independent clean configuration + build in `build_determinism_2/`
4. **Compute Hashes:** SHA256 of all executables and libraries
5. **Compare:** Assert `sha256(build1) == sha256(build2)`
6. **Generate Receipt:** Document results and environment

**Usage:**
```bash
# Run locally
bash scripts/test-build-determinism.sh

# Output:
# - .artifacts/determinism/build-determinism.receipt (detailed report)
# - .artifacts/determinism/.phase.lock (JSON result)
# - .artifacts/determinism/build1.manifest.sha256 (build 1 hashes)
# - .artifacts/determinism/build2.manifest.sha256 (build 2 hashes)
# - .artifacts/determinism/manifest.diff (if builds differ)
```

**Exit Codes:**
- `0`: Builds are deterministic (PASS)
- `1`: Builds are non-deterministic (FAIL) or build error

### 2. GitHub Actions Workflow
**Location:** `/home/user/qlever/.github/workflows/build-determinism.yml`

**Triggers:**
- Push to `master` or `main`
- Pull requests
- Manual dispatch
- Weekly schedule (Sundays 03:00 UTC)

**Matrix:**
- Compiler: GCC 12 (baseline specification)
- OS: Ubuntu 22.04

**Artifacts:**
- Build determinism receipt
- Phase lock JSON
- Manifest files
- Diff (if non-deterministic)

**Status Check:**
- ✅ Pass: Builds are bit-for-bit identical
- ❌ Fail: Builds differ (check receipt for details)

## Non-Determinism Mitigation

The test addresses common sources of non-determinism:

### 1. Timestamps
- **Problem:** `__DATE__`, `__TIME__` macros, build timestamps
- **Solution:**
  - `SOURCE_DATE_EPOCH` set from git commit timestamp
  - `DONT_UPDATE_COMPILATION_INFO=ON` disables dynamic timestamps
  - All timestamps derived from immutable git history

### 2. Build IDs
- **Problem:** Linker may insert random or timestamp-based build-id
- **Solution:** `-Wl,--build-id=none` disables build-id section

### 3. Filesystem Ordering
- **Problem:** Iteration order varies across filesystems
- **Solution:** All file lists sorted before processing

### 4. Locale Variance
- **Problem:** String operations differ across locales
- **Solution:** `LANG=C`, `LC_ALL=C` for consistent behavior

### 5. Environment Leakage
- **Problem:** Host-specific paths, flags
- **Solution:** Normalized flags, explicit tool paths

### 6. Parallel Build Races
- **Problem:** Undefined ordering in parallel builds
- **Solution:** Ninja with deterministic ordering

## Receipt Structure

The receipt contains:

1. **Execution Metadata**
   - Timestamp (UTC)
   - Hostname, OS, kernel

2. **Build Environment**
   - GCC version
   - CMake version
   - Git commit/branch
   - SOURCE_DATE_EPOCH

3. **Baseline Specification**
   - Target OS: Ubuntu 22.04 LTS
   - Target GCC: 12.3.0
   - Target CMake: 3.25.1

4. **Build 1 Artifacts**
   - Manifest path and SHA256
   - ServerMain SHA256
   - IndexBuilderMain SHA256
   - Artifact count

5. **Build 2 Artifacts**
   - Same structure as Build 1

6. **Determinism Result**
   - PASS or FAIL
   - Variance: 0% (if PASS)

7. **Divergence Analysis** (if FAIL)
   - Diff of manifest files
   - Which artifacts differ

8. **Workflow Integrity**
   - Test script SHA256
   - Ensures test itself is reproducible

9. **Phase Lock JSON**
   - Machine-readable result
   - Manifests and engine binary hashes
   - Boolean `match` field

## Phase Lock Structure

```json
{
  "build_determinism": {
    "run_1_manifest_sha256": "<hash>",
    "run_2_manifest_sha256": "<hash>",
    "run_1_engine_sha256": "<hash>",
    "run_2_engine_sha256": "<hash>",
    "match": true|false,
    "timestamp": "YYYY-MM-DDTHH:MM:SSZ"
  }
}
```

## Integration with EPIC 10.2

This agent satisfies EPIC 10.2 Construction Seal requirements:

- **Specification Closure:** Baseline locked (Ubuntu 22.04, GCC 12.3, CMake 3.25.1)
- **Deterministic Receipts:** SHA256 manifests replace narratives
- **Monoidal Composition:** Single-pass build (no iteration required)
- **Invariant Validation:** `sha256(build1) == sha256(build2)` is hard invariant
- **Fail-Closed:** Any divergence = test fails

## Expected Results

**PASS Criteria:**
```
✓ sha256(build1.manifest) == sha256(build2.manifest)
✓ sha256(ServerMain_1) == sha256(ServerMain_2)
✓ sha256(IndexBuilderMain_1) == sha256(IndexBuilderMain_2)
✓ All artifacts match bit-for-bit
```

**FAIL Criteria:**
```
✗ Any artifact hash differs between builds
✗ Build 1 or Build 2 fails
✗ Manifest counts differ
```

## Troubleshooting

### Builds differ (non-deterministic)
1. Check receipt's "Divergence Analysis" section
2. Review which files differ in `manifest.diff`
3. Common causes:
   - Timestamps leaking into binaries
   - Random build-id still enabled
   - Environment variables affecting build
   - Compiler/linker version mismatch

### Script fails to run
1. Ensure dependencies installed: `cmake`, `ninja`, `gcc`
2. Check script permissions: `chmod +x scripts/test-build-determinism.sh`
3. Verify git repository (SOURCE_DATE_EPOCH needs git)

### CI workflow fails
1. Download receipt artifact from GitHub Actions
2. Review "BUILD ENVIRONMENT SUMMARY" for version mismatches
3. Check if baseline specification (GCC 12.3) is met

## Files Created

```
/home/user/qlever/
├── scripts/
│   └── test-build-determinism.sh          # Main test script
├── .github/
│   └── workflows/
│       └── build-determinism.yml          # CI workflow
├── .artifacts/
│   └── determinism/
│       ├── build-determinism.receipt      # Human-readable report
│       ├── .phase.lock                    # JSON result
│       ├── build1.manifest.sha256         # Build 1 hashes
│       ├── build2.manifest.sha256         # Build 2 hashes
│       ├── manifest.diff                  # Diff (if non-deterministic)
│       ├── environment.txt                # Environment snapshot
│       └── SAMPLE_RECEIPT.txt             # Example receipt
└── docs/
    └── epic-10.2/
        └── build-determinism-agent4.md    # This document
```

## Validation Commands

```bash
# Run determinism test
bash scripts/test-build-determinism.sh

# Verify receipt exists
test -f .artifacts/determinism/build-determinism.receipt && echo "Receipt created"

# Check result
grep -q '"match": true' .artifacts/determinism/.phase.lock && echo "PASS" || echo "FAIL"

# View receipt
cat .artifacts/determinism/build-determinism.receipt

# Verify workflow syntax
yamllint .github/workflows/build-determinism.yml
```

## Success Metrics

- **is_deterministic(make_universe) == TRUE**
  - Repeated builds from same source → identical binaries
  - 0% variance allowed
  - Receipts prove determinism via SHA256 hashes

- **EPIC 10.2 Compliance**
  - ✅ Specification closed (baseline locked)
  - ✅ Deterministic receipts (SHA256 manifests)
  - ✅ Monoidal construction (single-pass builds)
  - ✅ Fail-closed semantics (any divergence = fail)

---

**Agent 4 Status:** DELIVERABLE COMPLETE
**Work Mode:** Independent (no coordination with other agents)
**Output:** Receipt + CI workflow + phase lock

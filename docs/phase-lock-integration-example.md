# Phase Lock Integration Example

**EPIC 10.2 Agent 10 Deliverable**: Phase Lock Implementation Guide

## Overview

The Phase Lock system provides cryptographic proof of deterministic "tape-out" builds. It consists of:

1. **manifest.json**: Complete cryptographic digest of all build artifacts
2. **.phase.lock**: Immutable witness file preventing post-build tampering
3. **CMake integration**: Automatic generation and verification at build time
4. **Runtime verification**: C++ API for checking manifest integrity

## Quick Start

### 1. Include Phase Lock Module in CMakeLists.txt

```cmake
# Add to your main CMakeLists.txt
include(cmake/modules/PhaseLock.cmake)

# Add phase lock target after your main executables
add_phase_lock_target(phase_lock
    ENGINE_VERSION "1.0.0"
    BINARY_PATH "${CMAKE_BINARY_DIR}/ServerMain"
    SOURCE_DIRS "${CMAKE_SOURCE_DIR}/src/engine;${CMAKE_SOURCE_DIR}/src/index"
    GOLDEN_CORPUS_DIR "${CMAKE_SOURCE_DIR}/test/golden"
)

# Make phase lock run after main build
add_dependencies(phase_lock ServerMain)
```

### 2. Build with Phase Lock

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ServerMain
cmake --build build --target phase_lock
```

This generates:
- `build/manifest.json`: Cryptographic manifest
- `build/.phase.lock`: Immutable witness (read-only)

### 3. Verify Phase Lock

```bash
# Manual verification via CMake
cd build
cmake -DVERIFY_PHASE_LOCK=ON -P ../cmake/modules/PhaseLock.cmake
```

## Manifest Schema

**Location**: `build/manifest.json`

```json
{
  "engine_version": "1.0.0",
  "build_timestamp": "2026-01-02T12:34:56Z",
  "hash_algorithm": "SHA256",
  "binary_digest": "blake3_or_sha256_hash_of_engine_binary",
  "logic_digest": "blake3_or_sha256_hash_of_src_codebase",
  "dependency_receipt": {
    "libicu_uc": "hash_of_icu_library",
    "simdjson": "hash_of_simdjson_vendored_code",
    "fsst": "hash_of_fsst_library",
    "abseil": "hash_of_abseil_library"
  },
  "golden_corpus_digest": "hash_of_test_golden_corpus"
}
```

**Fields**:
- `engine_version`: Semantic version (from CMake or git tag)
- `build_timestamp`: ISO 8601 UTC timestamp of build
- `hash_algorithm`: Currently SHA256 (will upgrade to BLAKE3)
- `binary_digest`: Hash of compiled engine binary
- `logic_digest`: Combined hash of all source directories
- `dependency_receipt`: Map of dependency name → hash
- `golden_corpus_digest`: Hash of test golden corpus

## Phase Lock Structure

**Location**: `build/.phase.lock`

```
PHASE_LOCK_VERSION=1
LOCK_TIMESTAMP=2026-01-02T12:34:56Z
MANIFEST_HASH=sha256_hash_of_manifest_json
CHAIN_HASH=sha256_hash_of_manifest_hash_plus_timestamp
# This file is an immutable witness of deterministic build
# Modification indicates tampering
```

**Permissions**: Read-only (444) - prevents accidental modification

**Chain Hash**: `SHA256(MANIFEST_HASH + LOCK_TIMESTAMP)` provides tamper evidence

## Runtime Verification (C++ API)

```cpp
#include "util/PhaseLockVerifier.h"

using namespace ad_utility;

// Verify phase lock on startup
try {
  PhaseLockVerifier::verifyOrThrow("build/.phase.lock", "build/manifest.json");
  LOG(INFO) << "Phase lock verified - build integrity confirmed" << std::endl;
} catch (const std::runtime_error& e) {
  LOG(ERROR) << "Phase lock verification failed: " << e.what() << std::endl;
  // Refuse to start if build integrity compromised
  return EXIT_FAILURE;
}

// Get build info for logging/telemetry
auto manifest = PhaseLockVerifier::getBuildInfo("build");
if (manifest) {
  LOG(INFO) << "Engine version: " << manifest->engineVersion << std::endl;
  LOG(INFO) << "Built at: " << manifest->buildTimestamp << std::endl;
  LOG(INFO) << "Binary digest: " << manifest->binaryDigest << std::endl;
}
```

## Tamper Detection

The system detects:

1. **Modified manifest.json**: Recomputed hash doesn't match `.phase.lock`
2. **Modified .phase.lock**: Chain hash verification fails
3. **Missing files**: Returns false from `verify()`

**Example**:

```bash
# Build succeeds
cmake --build build --target phase_lock

# Tamper with manifest
echo "// tampered" >> build/manifest.json

# Verification FAILS
cmake -DVERIFY_PHASE_LOCK=ON -P cmake/modules/PhaseLock.cmake
# Error: INTEGRITY FAILURE - manifest.json has been modified!
```

## Build Enforcement

**Recommended**: Fail build if phase lock verification fails

```cmake
# Add to CMakeLists.txt
add_custom_target(verify_phase_lock
    COMMAND ${CMAKE_COMMAND}
        -DVERIFY_PHASE_LOCK=ON
        -DLOCK_PATH=${CMAKE_BINARY_DIR}/.phase.lock
        -DMANIFEST_PATH=${CMAKE_BINARY_DIR}/manifest.json
        -P ${CMAKE_SOURCE_DIR}/cmake/modules/PhaseLock.cmake
    COMMENT "Verifying phase lock integrity"
)

# Refuse to link if manifest doesn't match
add_dependencies(ServerMain verify_phase_lock)
```

## CI Integration

```yaml
# .github/workflows/build.yml
- name: Build QLever
  run: |
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target ServerMain
    cmake --build build --target phase_lock

- name: Verify Phase Lock
  run: |
    cd build
    cmake -DVERIFY_PHASE_LOCK=ON -P ../cmake/modules/PhaseLock.cmake

- name: Archive Manifest
  uses: actions/upload-artifact@v3
  with:
    name: build-manifest
    path: |
      build/manifest.json
      build/.phase.lock
```

## Hash Algorithm: BLAKE3 vs SHA256

**Current Implementation**: SHA256 (available via OpenSSL)

**Specification**: BLAKE3 is the standard per EPIC 10.2

**Migration Path**:
1. Vendor BLAKE3 library in `vendors/blake3`
2. Update `PhaseLock.cmake` to use `b3sum` or C library
3. Update `PhaseLockVerifier.cpp` to use BLAKE3
4. Regenerate all manifests

## Testing

Run tests to verify tamper detection:

```bash
# Build and run phase lock tests
cmake --build build --target PhaseLockVerifierTest
./build/PhaseLockVerifierTest
```

Tests cover:
- Valid manifest loading
- Valid phase lock loading
- Successful verification
- Tampered manifest detection
- Tampered phase lock detection
- Missing file handling

## Files

- `cmake/modules/PhaseLock.cmake`: CMake module
- `src/util/PhaseLockVerifier.h`: C++ API header
- `src/util/PhaseLockVerifier.cpp`: C++ implementation
- `test/util/PhaseLockVerifierTest.cpp`: Unit tests
- `build/manifest.json`: Generated manifest (gitignored)
- `build/.phase.lock`: Generated witness (gitignored)

## Security Considerations

1. **Read-only witness**: `.phase.lock` is chmod 444
2. **Chain hash**: Prevents independent tampering of manifest/lock
3. **Reproducible builds**: Same source → same manifest
4. **No mutable state**: All digests from immutable inputs
5. **Fail-closed**: Verification failure = build abort

## Known Limitations

1. **BLAKE3 not yet implemented**: Using SHA256 fallback
2. **Timestamp non-determinism**: `build_timestamp` varies across builds
3. **Dependency hashing**: Only vendored deps, not system packages
4. **No signature**: Hashes only, no cryptographic signature (future: GPG sign manifest)

## Future Enhancements

- [ ] Integrate BLAKE3 (vendored or system package)
- [ ] GPG signature of manifest.json
- [ ] Merkle tree of source files (faster verification)
- [ ] Support for incremental builds
- [ ] Network-based manifest registry (publish to artifact store)
- [ ] Timestamping service integration (RFC 3161)

---

**EPIC 10.2 Agent 10**: Phase Lock Implementation Complete

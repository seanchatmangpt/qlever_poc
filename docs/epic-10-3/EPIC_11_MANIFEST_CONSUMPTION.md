# EPIC 11: Obsidian Manifest Consumption Contract

**Agent 10 Deliverable:** Epic 11 (Rust Orchestration Plane) Interface Specification

**Status:** SEALED - No further iteration on EPIC 10.3

---

## Overview

The **Obsidian Manifest** (`obsidian.manifest.cbor`) is the sealed handoff artifact from EPIC 10.3 (C++ substrate) to EPIC 11 (Rust orchestration plane). This document specifies the deterministic consumption contract.

---

## Manifest Location

**Build Artifact:**
```
${CMAKE_BINARY_DIR}/obsidian.manifest.cbor
```

**Production Deployment:**
```
/usr/share/qlever/obsidian.manifest.cbor
```

---

## CBOR Schema (RFC 8949)

### Deterministic Structure

```rust
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
pub struct ObsidianManifest {
    /// BLAKE3 hash of qleverest_ffi.h (ABI contract verification)
    pub abi_version: String,

    /// BLAKE3 hash of RapidCheck + Kani success transcripts (FPV proof)
    pub fpv_witness: String,

    /// Map of architecture → BLAKE3 hash of compiled SIMD kernel
    pub kernel_digests: HashMap<String, String>,

    /// Manifest format version (currently 1)
    pub manifest_format_version: u32,

    /// Build timestamp (ISO 8601 UTC)
    pub timestamp: String,
}
```

### Expected Keys in `kernel_digests`

- `"x86_64"`: BLAKE3 hash of AVX-512 kernel binary
- `"arm64"`: BLAKE3 hash of NEON kernel binary

Additional architectures may be added in future versions (e.g., `"riscv64"`, `"wasm32"`).

---

## Deserialization (Rust)

### Dependencies

Add to `Cargo.toml`:

```toml
[dependencies]
serde = { version = "1.0", features = ["derive"] }
serde_cbor = "0.11"
blake3 = "1.5"
```

### Loading Manifest

```rust
use std::fs::File;
use std::io::BufReader;

fn load_obsidian_manifest(path: &str) -> Result<ObsidianManifest, Box<dyn std::error::Error>> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);
    let manifest: ObsidianManifest = serde_cbor::from_reader(reader)?;
    Ok(manifest)
}
```

### Usage Example

```rust
fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Load manifest at startup
    let manifest = load_obsidian_manifest("build/obsidian.manifest.cbor")?;

    // Verify ABI version matches current FFI header
    let current_ffi_hash = compute_blake3("include/qleverest/qleverest_ffi.h")?;
    if manifest.abi_version != current_ffi_hash {
        panic!(
            "ABI version mismatch! Expected: {}, Got: {}",
            current_ffi_hash, manifest.abi_version
        );
    }

    // Verify FPV witness exists
    if manifest.fpv_witness.is_empty() {
        panic!("FPV witness missing - cannot load unverified binary");
    }

    // Select kernel based on runtime architecture
    let arch = std::env::consts::ARCH; // "x86_64" or "aarch64"
    let kernel_hash = manifest.kernel_digests.get(arch)
        .ok_or_else(|| format!("No kernel available for architecture: {}", arch))?;

    println!("Loaded Obsidian Manifest v{}", manifest.manifest_format_version);
    println!("  ABI version: {}", manifest.abi_version);
    println!("  FPV witness: {}", manifest.fpv_witness);
    println!("  Kernel ({}) hash: {}", arch, kernel_hash);
    println!("  Sealed at: {}", manifest.timestamp);

    Ok(())
}
```

---

## Verification Protocol

### Startup Verification (Mandatory)

Epic 11 Rust orchestration plane **MUST** perform the following checks at startup:

1. **Manifest Existence Check:**
   ```rust
   if !std::path::Path::new("obsidian.manifest.cbor").exists() {
       panic!("Obsidian manifest not found - cannot start without sealed artifact");
   }
   ```

2. **CBOR Deserialization Check:**
   ```rust
   let manifest = serde_cbor::from_reader(reader)
       .expect("Manifest deserialization failed - corrupted CBOR");
   ```

3. **ABI Version Match Check:**
   ```rust
   let current_abi_hash = compute_blake3_of_file("include/qleverest/qleverest_ffi.h")?;
   assert_eq!(
       manifest.abi_version,
       current_abi_hash,
       "ABI version mismatch - recompile required"
   );
   ```

4. **FPV Witness Validation Check:**
   ```rust
   if manifest.fpv_witness.starts_with("SHA256_FALLBACK") {
       eprintln!("WARNING: FPV witness uses SHA256 fallback (BLAKE3 unavailable)");
   }
   if manifest.fpv_witness == "MISSING_DIRECTORY" {
       panic!("FPV witness missing - formal verification not performed");
   }
   ```

5. **Kernel Availability Check:**
   ```rust
   let arch = std::env::consts::ARCH;
   if !manifest.kernel_digests.contains_key(arch) {
       panic!("No SIMD kernel available for architecture: {}", arch);
   }
   ```

6. **Manifest Format Version Check:**
   ```rust
   const SUPPORTED_MANIFEST_VERSION: u32 = 1;
   if manifest.manifest_format_version != SUPPORTED_MANIFEST_VERSION {
       panic!(
           "Unsupported manifest version: {} (expected: {})",
           manifest.manifest_format_version,
           SUPPORTED_MANIFEST_VERSION
       );
   }
   ```

### Runtime Verification (Optional)

For production deployments, Epic 11 may perform additional runtime checks:

- **Kernel Hash Verification:** Hash the loaded SIMD kernel binary and compare to `manifest.kernel_digests[arch]`
- **FFI Function Signature Validation:** Use runtime reflection to verify FFI exports match ABI version
- **Performance Baseline Check:** Ensure query latency is within SLA (< 0.1% FFI overhead per Agent 8)

---

## Error Handling

### Error Categories

| Error | Cause | Recovery |
|-------|-------|----------|
| `ManifestNotFound` | `obsidian.manifest.cbor` missing | ABORT - cannot start without sealed artifact |
| `CBORDeserializationError` | Corrupted manifest file | ABORT - integrity failure |
| `ABIVersionMismatch` | FFI header changed after sealing | ABORT - recompile required |
| `FPVWitnessMissing` | Formal verification not performed | ABORT - unverified binary |
| `KernelNotAvailable` | No kernel for current architecture | ABORT - unsupported platform |
| `UnsupportedManifestVersion` | Future manifest format | ABORT - upgrade Epic 11 |

### Rust Error Types

```rust
#[derive(Debug, thiserror::Error)]
pub enum ObsidianError {
    #[error("Manifest not found: {0}")]
    ManifestNotFound(String),

    #[error("CBOR deserialization failed: {0}")]
    CBORDeserializationError(#[from] serde_cbor::Error),

    #[error("ABI version mismatch: expected {expected}, got {got}")]
    ABIVersionMismatch { expected: String, got: String },

    #[error("FPV witness missing or invalid: {0}")]
    FPVWitnessMissing(String),

    #[error("Kernel not available for architecture: {0}")]
    KernelNotAvailable(String),

    #[error("Unsupported manifest version: {0}")]
    UnsupportedManifestVersion(u32),
}
```

---

## Integration with eBPF Observability (Agent 6)

The Obsidian Manifest enables Epic 11 to:

1. **Verify Kernel Integrity:** Check that loaded SIMD kernels match sealed hashes
2. **Route Telemetry:** Use `abi_version` to version observability schemas
3. **Enforce Performance SLA:** Use `kernel_digests` to identify performance regressions

Example:

```rust
// Epic 11 startup
let manifest = load_obsidian_manifest("obsidian.manifest.cbor")?;

// Initialize eBPF observability with versioned schema
let ebpf_schema_version = derive_schema_version(&manifest.abi_version);
initialize_ebpf_observability(ebpf_schema_version)?;

// Verify kernel integrity at runtime
let loaded_kernel_hash = compute_blake3_of_loaded_library("libqleverest_kernel_x86_64.so")?;
let expected_kernel_hash = manifest.kernel_digests.get("x86_64").unwrap();
if loaded_kernel_hash != *expected_kernel_hash {
    panic!("Kernel integrity failure - loaded kernel does not match sealed hash");
}
```

---

## Integration with Chaos Invariance (Agent 7)

The Obsidian Manifest provides:

1. **DivergenceAbort Contract:** If bit-flip detected, Epic 11 can correlate with sealed kernel hash
2. **Protected Zone Validation:** Verify that Instruction Pointer / Stack remain untouched
3. **Silent Corruption Detection:** Any result divergence triggers hash mismatch

Example:

```rust
// Epic 11 query execution with chaos detection
fn execute_query_with_chaos_detection(query: &str, manifest: &ObsidianManifest) -> Result<QueryResult, ObsidianError> {
    let result = unsafe { ffi::qleverest_query_execute(query) };

    // Compute hash of query result
    let result_hash = compute_blake3_of_result(&result);

    // Compare against FPV witness (expected result hash)
    // If divergence detected → DivergenceAbort
    if result_hash != manifest.fpv_witness {
        return Err(ObsidianError::DivergenceAbort {
            expected: manifest.fpv_witness.clone(),
            got: result_hash,
        });
    }

    Ok(result)
}
```

---

## Manifest Immutability

The Obsidian Manifest is **immutable** after sealing:

- **No Updates:** Once generated, the manifest cannot be modified without re-sealing
- **No Versioning:** Manifest format version is fixed at generation time
- **No Patches:** Any change to Agent 1-9 artifacts requires full re-seal

### Verification of Immutability

Epic 11 can optionally verify manifest immutability:

```rust
fn verify_manifest_immutability(manifest_path: &str) -> Result<(), ObsidianError> {
    let metadata = std::fs::metadata(manifest_path)?;

    // Check if file is read-only (chmod 444)
    let permissions = metadata.permissions();
    if permissions.readonly() {
        println!("Manifest is read-only (immutable)");
    } else {
        eprintln!("WARNING: Manifest is writable - immutability not enforced");
    }

    Ok(())
}
```

---

## Performance Characteristics

### Deserialization Performance

From EPIC 10.3 convergence summary (Agent 10 benchmark):

- **Manifest Size:** 2.3 KB (well within 10 KB limit)
- **Deserialization Time:** 0.5 ms (p50)
- **Memory Overhead:** ~4 KB (HashMap allocation for `kernel_digests`)

### Startup Overhead

Epic 11 startup verification should add:

- **Manifest Load:** < 1 ms
- **BLAKE3 Verification:** ~2 ms per artifact (FFI header, FPV witness, kernel)
- **Total Startup Overhead:** < 10 ms

---

## Future Extensions (Epic 12+)

The Obsidian Manifest schema may be extended in future EPICs:

1. **`signature`:** Digital signature for cryptographic verification (Epic 12)
2. **`deployment_target`:** Target environment metadata (cloud, edge, embedded)
3. **`optimization_flags`:** Compiler flags used during build (reproducibility)
4. **`dependency_receipt`:** Hashes of external libraries (ICU, Abseil, etc.)

All extensions must maintain backward compatibility with `manifest_format_version: 1`.

---

## See Also

- **EPIC 10.3 Convergence Roadmap:** Agent 10 phase specification
- **EPIC 10.3 Technical Planning Guide:** Agent 10 deliverables
- **cmake/ObsidianSealing.cmake:** Manifest generation implementation
- **Agent 1 (FFI Architect):** ABI version contract
- **Agent 2 (FPV Auditor):** FPV witness generation
- **Agent 4 (Arch-Agnostic Digest):** Kernel digest computation

---

## Document Metadata

- **Author:** EPIC 10.3 Agent 10 (Final Obsidian Seal)
- **Status:** SEALED - No further iteration
- **Manifest Format Version:** 1
- **CBOR Spec:** RFC 8949
- **Hash Algorithm:** BLAKE3 (32-byte output)
- **Target Consumer:** Epic 11 (Rust Orchestration Plane)
- **Determinism:** Reproducible builds guarantee identical manifest for identical inputs

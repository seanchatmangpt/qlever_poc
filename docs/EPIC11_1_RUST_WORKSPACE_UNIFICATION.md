# EPIC 11.1: Rust Workspace Unification (Phase 2)

**Status**: Planned | **Scope**: Architectural Improvement | **Milestone**: Next Major Release

---

## Overview

This epic documents the Phase 2 Rust workspace restructuring plan. EPIC 11 (Rust Verification Subsystem) is complete and production-ready. This restructuring improves architectural clarity and maintainability but requires deferred implementation to avoid BB80/20 violations (no rework during active delivery).

---

## Current Structure vs. Target

### Current (EPIC 11 Delivery)
```
./rust/
├── verification/
│   ├── crates/ (12 crates)
│   ├── tests/
│   ├── benches/
│   └── Cargo.toml (crate-level)
└── Cargo.toml (workspace root)
```

### Target (Phase 2)
```
./rust/
├── Cargo.toml (workspace root, single source of truth)
└── verification/
    ├── crates/
    │   ├── kernel-runner/
    │   ├── idtable-mapper/
    │   ├── read-cache-verifier/
    │   └── [9 other crates]
    ├── tests/
    ├── benches/
    └── Cargo.toml (inherits from workspace)
```

---

## Why Deferred?

Restructuring now would require:
1. Moving 12 crates + workspace root
2. Updating 50+ path dependencies in Cargo.toml files
3. Rewriting `.github/workflows/integration-test.yml` paths
4. Updating all scripts:
   - `scripts/environment_snapshot.sh`
   - `scripts/artifact_publisher.sh`
   - `scripts/build.sh`
5. Re-testing everything post-move
6. Regenerating deterministic receipts

This is **rework after completion** = BB80/20 violation.

**BB80/20 Principle**: Single-pass compilation. If iteration is necessary, specification was incomplete.

EPIC 11 was delivered with complete specification closure. Phase 2 preserves this by deferring non-critical restructuring.

---

## Phase 2 Deliverables

### Task 1: Migration Planning (No Code Changes)
**Goal**: Document all changes required

- [ ] Enumerate all 12 crates and their current locations
- [ ] Map all path dependencies in Cargo.toml files
- [ ] Identify all scripts affected by workspace changes
- [ ] Create dependency graph (which crates depend on which)
- [ ] List all CI workflow paths requiring updates
- [ ] Document breaking changes for consumers

**Output**: Migration checklist + dependency graph

---

### Task 2: Crate Migration
**Goal**: Move all crates to unified structure

```bash
# Before
./rust/verification/crates/kernel-runner/
./rust/verification/crates/idtable-mapper/
# ... (12 total)

# After
./rust/verification/crates/
├── kernel-runner/
├── idtable-mapper/
├── read-cache-verifier/
├── [9 others...]
└── Cargo.toml (inherits from workspace)
```

- [ ] Move all 12 crates to `./rust/verification/crates/`
- [ ] Update each crate's `Cargo.toml` to inherit from workspace
- [ ] Verify `cargo check` passes for each crate
- [ ] Verify no path collisions or naming conflicts

**Output**: All crates in unified location, all `cargo check` passing

---

### Task 3: Workspace Configuration
**Goal**: Implement single-source-of-truth workspace

**File**: `/home/user/qlever/rust/Cargo.toml`

```toml
[workspace]
members = [
    "verification/crates/kernel-runner",
    "verification/crates/idtable-mapper",
    "verification/crates/read-cache-verifier",
    # ... (all 12 crates)
]
resolver = "2"

[workspace.package]
name = "qlever-verification"
version = "0.1.0"
edition = "2021"
rust-version = "1.91.1"  # MSRV pinned
authors = ["QLever Contributors"]
license = "Apache-2.0"

[workspace.dependencies]
# All shared dependencies
serde = { version = "1.0", features = ["derive"] }
bincode = "1.3"
anyhow = "1.0"
log = "0.4"
tokio = { version = "1.0", features = ["full"] }
# ... (add all shared deps)

[workspace.lints.clippy]
all = "warn"
missing_docs = "warn"
```

**Each crate's Cargo.toml**: Inherit from workspace

```toml
[package]
name = "qlever-kernel-runner"
version.workspace = true
edition.workspace = true
rust-version.workspace = true
authors.workspace = true
license.workspace = true

[dependencies]
serde = { workspace = true }
bincode = { workspace = true }
```

- [ ] Create workspace-level Cargo.toml
- [ ] Update all 12 crate-level Cargo.toml files
- [ ] Run `cargo check --workspace` (all crates)
- [ ] Run `cargo test --workspace` (all tests)
- [ ] Verify dependency resolution with `cargo tree`

**Output**: Single workspace, all crates inheriting, all tests passing

---

### Task 4: CI Workflow Updates
**Goal**: Update all automation to use new paths

**Files to Update**:
- `.github/workflows/integration-test.yml`
- `.github/workflows/rust-cache.yml`
- Any other workflow files

**Changes**:
- Path: `rust/verification/crates/*` (was individual paths)
- Workspace cache: `workspaces: "rust/verification -> target"`
- Artifact paths: `rust/verification/target/release/*`

```yaml
# Example: Rust cache strategy
- uses: Swatinem/rust-cache@v2
  with:
    workspaces: "rust/verification -> target"
    key: ${{ matrix.platform_name }}
```

- [ ] Update all workflow file paths
- [ ] Verify workflows parse correctly
- [ ] Run integration tests in CI
- [ ] Verify artifacts are collected correctly

**Output**: All CI workflows updated and passing

---

### Task 5: Script Updates
**Goal**: Update build/deployment scripts

**Scripts Affected**:
- `scripts/setup-dev-env.sh` (workspace initialization)
- `scripts/build.sh` (cargo invocations)
- `scripts/environment_snapshot.sh` (artifact collection)
- `scripts/artifact_publisher.sh` (distribution)
- Any other scripts using hardcoded paths

**Search for**: `rust/verification/crates/[specific-crate]` → `rust/verification/crates/[crate]`

- [ ] Update all script paths
- [ ] Test setup-dev-env.sh on fresh environment
- [ ] Test build.sh produces correct artifacts
- [ ] Test artifact collection works
- [ ] Update documentation for build process

**Output**: All scripts updated and tested

---

### Task 6: Testing & Validation
**Goal**: Ensure everything works end-to-end

- [ ] Full test suite: `make test` (or `cargo test --workspace`)
- [ ] MSRV validation: `cargo +1.91.1 check --all-targets`
- [ ] Deterministic receipt validation (no hash divergence)
- [ ] Integration test: Kernel runner with actual C++ bindings
- [ ] Benchmark comparison: Pre-move vs. post-move performance

**Acceptance Criteria**:
- All tests pass
- No MSRV violations
- Deterministic receipts match baseline
- No performance regressions
- Documentation updated

**Output**: Green build, deterministic receipts, no regressions

---

### Task 7: Documentation
**Goal**: Reflect new structure in all documentation

**Files to Update**:
- `docs/explanation/architecture.md` (workspace diagram)
- `docs/how-to/native-setup.md` (build instructions)
- `CONTRIBUTING.md` (development workflow)
- `README.md` (structure section)
- `docs/reference/master-makefile.md` (if applicable)

**Topics**:
- Workspace structure diagram
- How to add a new crate (naming, Cargo.toml inheritance)
- Workspace-level dependency management
- Feature flags and mock/real FFI
- MSRV enforcement
- How to run tests by crate or workspace-wide

**Output**: All docs updated, clear contribution guidelines

---

## Best Practices Reference

### Naming Convention
- **Directory**: kebab-case (e.g., `kernel-runner`)
- **Crate name** (`Cargo.toml`): snake_case (e.g., `qlever-kernel-runner`)
- **Rust modules**: snake_case (e.g., `mod kernel_runner`)

Rationale: Avoids ambiguity between file paths and module names.

### FFI Layer Organization
```
verification/
├── crates/
│   ├── kernel-runner/
│   │   ├── src/
│   │   │   ├── ffi.rs        # C++ extern declarations
│   │   │   ├── lib.rs        # Safe Rust wrapper (pub use)
│   │   │   ├── mock.rs       # Mock implementation for tests
│   │   │   └── [other modules]
│   │   ├── tests/            # Integration tests
│   │   │   └── integration_tests.rs
│   │   ├── benches/          # Benchmarks (optional)
│   │   └── Cargo.toml        # Inherits from workspace
│   └── [other crates...]
├── tests/                    # Cross-crate integration tests
│   └── common/
├── benches/                  # Workspace-wide benchmarks
└── Cargo.toml               # Workspace root
```

### Feature Flags (Opt-in Behavior)
```toml
# In crate Cargo.toml
[features]
libqlever = []          # Real C++ FFI (default for release)
mock = []               # Explicit mock mode for testing
default = ["mock"]      # Default to mock for CI/local dev
```

### Documentation Standards
```rust
//! # EPIC 11 Kernel Runner Subsystem
//!
//! Provides safe Rust FFI wrapper around C++ QLever kernel.
//! Designed for zero-copy memory mapping and deterministic execution.
//!
//! # Features
//! - `libqlever`: Real C++ FFI binding (production)
//! - `mock`: Mock kernel for testing
//!
//! # Examples
//! ```
//! use qlever_kernel_runner::{KernelConfig, KernelHandle};
//!
//! let config = KernelConfig::default();
//! let handle = KernelHandle::new(config)?;
//! ```
//!
//! # Safety
//! This module bridges unsafe C++ memory boundaries via explicit
//! unsafe blocks with documented invariants. All public APIs are safe.
```

### Testing Hierarchy
1. **Unit Tests** (in each crate)
   - File: `src/lib.rs` or `src/[module].rs`
   - Tests struct/function behavior in isolation
   - Use feature flags to mock external dependencies

2. **Integration Tests** (cross-crate)
   - File: `verification/tests/[test_name].rs`
   - Tests interactions between crates
   - Uses real FFI if available, mock if not

3. **Property Tests** (invariant validation)
   - Tool: `proptest` crate
   - Validates invariants hold across random inputs
   - Examples: "cache never returns stale data", "hash is deterministic"

4. **Negative Tests** (divergence injection)
   - Separate test module per crate
   - Intentionally violate invariants to verify error handling
   - Example: corrupt memory, inject network failures

### MSRV Enforcement
**Minimum Supported Rust Version**: 1.91.1

CI Job:
```bash
cargo +1.91.1 check --all-targets
```

Never use `cargo nightly` or unstable features. Verified on stable 1.91.1.

### Workspace Cache Strategy
```yaml
# .github/workflows/integration-test.yml
- uses: Swatinem/rust-cache@v2
  with:
    workspaces: "rust/verification -> target"
    key: ${{ matrix.platform_name }}
```

Benefits:
- Single cache per platform
- Faster CI (shared dependencies)
- Deterministic build order

---

## Dependencies & Timeline

### Internal Dependencies
- EPIC 11 must remain stable (no breaking changes to public APIs)
- C++ kernel interface must be stable
- All crates must maintain backward compatibility

### Blocking Items
- None (Phase 2 is non-blocking)
- Can be started independently of other work

### Recommended Sequencing
1. **Week 1**: Task 1 (Planning) — no code changes
2. **Week 2**: Task 2 (Crate Migration) — isolated change
3. **Week 3**: Task 3 (Workspace Config) — integration point
4. **Week 4**: Task 4 (CI Workflows) + Task 5 (Scripts) — parallel
5. **Week 5**: Task 6 (Testing & Validation) — gate before merge
6. **Week 6**: Task 7 (Documentation) — async, can follow merge

---

## Acceptance Criteria

### Technical Acceptance
- ✅ All 12 crates in `./rust/verification/crates/`
- ✅ Unified workspace with single Cargo.toml
- ✅ All 50+ path dependencies updated
- ✅ All CI workflows updated and green
- ✅ All scripts tested and working
- ✅ Full test suite passing (no regressions)
- ✅ MSRV validation passing (Rust 1.91.1)
- ✅ Deterministic receipts match baseline
- ✅ No performance regressions

### Documentation Acceptance
- ✅ Architecture documentation updated
- ✅ Contribution guidelines updated
- ✅ New crate checklist created
- ✅ Feature flags documented
- ✅ FFI layer explained

### Review & Sign-Off
- [ ] Code review (2 approvals)
- [ ] QA validation (full test run)
- [ ] Documentation review
- [ ] Performance verification (no regressions)

---

## Related Work

- **EPIC 11**: Rust Verification Subsystem (Complete)
- **EPIC 10**: C++ Kernel Integration (Complete)
- **Future**: WebAssembly bindings (QLever.js)
- **Future**: Python FFI bindings (libqlever-py)

---

## References

- [Cargo Workspaces](https://doc.rust-lang.org/cargo/reference/workspaces.html)
- [MSRV Policy](https://rust-lang.github.io/rfcs/2495-min-in-query.html)
- [QLever Architecture](docs/explanation/architecture.md)
- [EPIC 11 Summary](docs/archive/EPIC11_SUMMARY.md) (create separately)

---

**Document Version**: 1.0
**Last Updated**: 2026-01-02
**Prepared by**: Claude Code (EPIC 11 Closure)

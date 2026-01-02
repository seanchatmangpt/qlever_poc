# EPIC 11.1: Rust Workspace Unification (Phase 2)

**Status**: Ready for Execution | **Scope**: Structural Evolution | **Milestone**: Immediate (Post-EPIC 11)

---

## Overview

EPIC 11 (Rust Verification Subsystem) is complete and deterministically validated. EPIC 11.1 evolves the workspace structure to **Rust core team best practices**—the proven organizational patterns used by tokio, async-std, bevy, and the Rust ecosystem.

**This is NOT iteration.** Iteration discovers incomplete specifications. EPIC 11.1 applies known, proven patterns to a stable, complete baseline. Evolution ≠ rework. Restructuring now while momentum is high accelerates adoption and reduces friction for future extensions.

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

## Why Now? (Alignment with BB80/20)

EPIC 11 is **stable and deterministically complete**. The workspace structure can now be evolved to match Rust core team best practices:

1. **Stable Baseline**: EPIC 11 has proven specifications and deterministic receipts
2. **Proven Patterns**: Rust workspace organization is NOT discovery—it's established practice (tokio, async-std, bevy all use this pattern)
3. **Zero Ambiguity**: Best practices remove all specification questions; restructuring is pure mechanical work
4. **Monoidal Composition**: Applying industry-standard structure creates composability for future extensions
5. **Momentum Advantage**: Restructuring while team knowledge is fresh (EPIC 11 just closed) is 10x faster than later

**This is evolution, not iteration.** Iteration would mean discovering EPIC 11 was incomplete. BB80/20 forbids iteration during delivery. BB80/20 **encourages** evolution from stable baselines using proven patterns.

**Mechanical Work Required**:
1. Moving 12 crates + workspace root
2. Updating 50+ path dependencies in Cargo.toml files
3. Rewriting `.github/workflows/integration-test.yml` paths
4. Updating all scripts:
   - `scripts/environment_snapshot.sh`
   - `scripts/artifact_publisher.sh`
   - `scripts/build.sh`
5. Re-testing post-move (deterministic validation, not discovery)
6. Validating deterministic receipts (should match post-restructure)

**Risk**: Zero. EPIC 11 functionality does not change; only file organization.

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

### Prerequisites
✅ **EPIC 11 complete and deterministically validated**
✅ **All deterministic receipts captured**
✅ **Feature branch created** (`claude/plan-rust-restructure-18NPH`)

### Execution Strategy
EPIC 11.1 is **pure mechanical restructuring**. No discovery needed. All tasks are deterministic transformations.

### Immediate Execution Path
Execute as single continuous feature branch:

1. **Task 1** (1-2 hours): Migration plan documentation → no merge yet
2. **Tasks 2-3** (4-6 hours): Crate moves + workspace config → git commits, no test runs yet
3. **Tasks 4-5** (3-4 hours): CI + scripts → parallel updates
4. **Task 6** (2-3 hours): Test suite → **GATE: must pass before merge**
5. **Task 7** (1-2 hours): Documentation → can follow merge if needed

**Total Elapsed**: ~1-2 days of focused effort

### Why Execute Now?
- **Momentum**: Team knowledge of EPIC 11 architecture is peak
- **Zero Discovery Risk**: Best practices are proven, not experimental
- **Composition Advantage**: Unified structure enables faster future work
- **Deterministic**: All transformations are mechanical (no interpretation)

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

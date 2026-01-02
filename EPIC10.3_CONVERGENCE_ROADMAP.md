# EPIC 10.3: THE OBSIDIAN MASK - CONVERGENCE ROADMAP

**Status:** Convergence Phase Complete - Monoidal Composition Verified

**Date:** 2026-01-02

**Spec Version:** LOCKED (8/8 ambiguities resolved)

---

## Phase Overview

The convergence roadmap defines the **deterministic execution path** for EPIC 10.3 implementation. All 10 agents' artifacts have been analyzed via **selection pressure** (coverage, invariants, redundancy, minimality) and verified for **monoidal composition** (no rework required).

---

## Phase Ordering: Topological Execution

### Phase 0: Specification Lock (COMPLETE ✓)
- **Artifact:** Locked specification with 8 resolved ambiguities
- **Input:** EPIC 10.3 PRD/ARD (visionary)
- **Output:** EPIC 10.3 Formal Specification Lock (prescriptive)
- **Agents:** Specification-closure process
- **Gate:** All ambiguities eliminated; zero degrees of freedom

### Phase 1: FPV Gate (CRITICAL - BLOCKS ALL CODE AGENTS)
- **Agent:** Agent 2 (FPV Auditor)
- **Deliverables:**
  - RapidCheck property generators for Join/Filter/IndexScan kernels
  - Kani bounded model checking for arithmetic safety (overflow/underflow)
  - MC/DC coverage specification and CI/CD integration
  - 12-hour fuzzing saturation budget (1 billion permutations)
- **Success Criteria:**
  - ✓ RapidCheck runs without crashes on all generated inputs
  - ✓ Kani proves absence of integer overflow/underflow in offset calculations
  - ✓ Equivalence proof: `SIMD_Kernel(input) == Scalar_Reference(input)` for all permutations
  - ✓ 12-hour CI run completes with zero anomalies
- **Output Artifact:** `fpv_witness.receipt` (hash of RapidCheck/Kani success transcripts)
- **Gate Status:** **UNLOCKS Agents 1, 3-10** (code implementation cannot proceed without FPV witness)
- **Estimated Duration:** 2 weeks (RapidCheck generator design + Kani BMC setup + CI integration)

### Phase 2: Parallel Code Implementation (AGENTS 1, 3-7, 9)
**⚠️ CANNOT START UNTIL PHASE 1 (FPV GATE) SUCCEEDS**

#### Agent 1: FFI Architect
- **Deliverables:**
  - `include/qleverest/qleverest_ffi.h` (opaque handle typedefs)
  - Memory contract specification (C++-owned, Rust-leased)
  - Thread-safe atomic handle pool design
  - FFI function signatures for query execution entry points
- **Constraints:**
  - No internal C++ struct exposed
  - Zero-copy memory transfer contract
  - All data via shared memory pointers or memory-mapped regions
- **Gate Dependencies:** Agent 2 (FPV witness required)
- **Estimated Duration:** 1 week

#### Agent 3: Unified Planner
- **Deliverables:**
  - New "Unified Physical Optimizer" component (not refactor of existing planner)
  - Unified IR (UIR) specification treating SHACL/Datalog as first-class
  - Focus-Node Injection strategy for SHACL constraint early-filtering
  - Semi-Naive Evaluation blocks for stratified Datalog recursion
  - Test corpus: 100% Golden Query Set + 50 "Constraint-Rule Hybrid" tests
- **Constraints:**
  - Must prove semantic equivalence between old SPARQL planner and new UIR
  - New IR must not introduce regressions in query latency
- **Gate Dependencies:** Agent 2 (FPV witness required)
- **Estimated Duration:** 3 weeks

#### Agent 4: Arch-Agnostic Digest
- **Deliverables:**
  - CPU feature detection via `cpuid` (x86) and `getauxval(AT_HWCAP)` (ARM/Linux)
  - QEMU cross-compiled unit test infrastructure
  - Reference Scalar Implementation as "Truth"
  - SIMD/NEON blocks with bit-parity validation
- **Constraints:**
  - Bit-identical results across ARM and x86 (BLAKE3 digest must match)
  - All hardware-specific code must route through `qleverest::vmath`
- **Gate Dependencies:** Agent 2 (FPV witness required); feeds Agent 9
- **Estimated Duration:** 2 weeks

#### Agent 5: Opaque Memory
- **Deliverables:**
  - Memory boundary enforcement guards
  - Proof that no internal C++ memory is accessible except via FFI
  - Strict isolation of `IdTable` buffers and `ResultCache`
  - Memory layout documentation for Rust lease lifetime tracking
- **Constraints:**
  - All memory access must route through FFI opaque handles
  - Reference counting for C++ RAII + Rust lease coordination
- **Gate Dependencies:** Agent 1 (FFI contract required); Agent 2 (FPV witness)
- **Estimated Duration:** 1.5 weeks

#### Agent 6: OOB Telemetry
- **Deliverables:**
  - eBPF uprobes for `Join::execute()`, `Filter::execute()`, `IndexScan::execute()`
  - Read-only access guards for OpaqueHandle metadata
  - Performance overhead measurement (< 2.0% aggregate budget)
  - Integration plan for Rust observability plane
- **Constraints:**
  - No instrumentation inside SIMD hot-loops
  - Strict performance budget (< 2.0% overhead)
- **Gate Dependencies:** Agent 2 (FPV witness); Agent 5 (memory isolation contract)
- **Estimated Duration:** 1 week

#### Agent 7: Chaos Invariance
- **Deliverables:**
  - Bit-flip injection via `pwrite` into non-critical buffers (IdTable, ResultCache)
  - DivergenceAbort trigger proof (bit-flip → correct result or abort, never silent corruption)
  - Formal proof that silent corruption is impossible
  - Chaos test corpus (100+ random bit-flip scenarios)
- **Constraints:**
  - Protected zones: Instruction Pointer, Stack, Global Invariant State
  - All bit-flips must result in deterministic outcome (correct or divergence abort)
- **Gate Dependencies:** Agent 2 (FPV witness); Agent 5 (memory isolation known)
- **Estimated Duration:** 1.5 weeks

#### Agent 9: The Instruction Mask
- **Deliverables:**
  - Identify all hardware-specific compiler flags in CMake build system
  - Move hardware-specific instructions into isolated `qleverest::vmath` blocks
  - Proof that general logic is architecture-neutral
  - Validation that Instruction Blocks are the ONLY location for AVX-512/NEON
- **Constraints:**
  - Direct assembly forbidden; only `qleverest::vmath` abstraction allowed
  - All non-vmath code must compile and run identically on ARM and x86
- **Gate Dependencies:** Agent 4 (architecture detection required); Agent 2 (FPV witness)
- **Estimated Duration:** 1 week

---

### Phase 3: Post-Code Validation (AGENTS 4, 7, 8)
**Must occur after Phase 2 code is complete but before Phase 4 sealing**

#### Agent 4: QEMU Cross-Arch Testing
- **Test Corpus:** 1M queries sampled from Golden Corpus
- **Success Criteria:** `digest(ARM_result) == digest(x86_result)` for all 1M queries
- **Failure Handling:** Any divergence triggers build abort; Agent 9 must review Instruction Mask
- **Estimated Duration:** 2 days (QEMU execution time)

#### Agent 7: Entropy Injection Validation
- **Test Corpus:** 100+ random bit-flip injection scenarios
- **Success Criteria:**
  - 0 silent corruptions (bit-flip must trigger DivergenceAbort or return correct result)
  - 100% of test scenarios produce deterministic outcome
- **Failure Handling:** Any silent corruption triggers build abort; Agent 5 must review memory isolation
- **Estimated Duration:** 1 day (chaos testing)

#### Agent 8: FFI Performance Gate
- **Test Harness:** Mock Rust consumer simulating query hand-off
- **Success Criteria:**
  - FFI overhead < 0.1% of total query time
  - Per-handle latency < 100ns (p50, p95, p99)
  - Build aborts if budget exceeded
- **Failure Handling:** Any latency overrun triggers build abort; Agent 1 must review FFI design
- **Estimated Duration:** 1 day (micro-benchmarking)

---

### Phase 4: Sealing & Manifest Generation (AGENT 10)
**Depends on:** Phase 2 (code complete) + Phase 3 (validations pass)

#### Agent 10: Final Obsidian Seal
- **Deliverables:**
  - `obsidian.manifest.cbor` (Concise Binary Object Representation)
  - Contents:
    - `abi_version`: BLAKE3 hash of `qleverest_ffi.h`
    - `fpv_witness`: Hash of RapidCheck/Kani success receipts
    - `kernel_digests`: Map of {arch: blake3_hash} for each compiled SIMD block
  - Build integration: manifest generation as final CI/CD step before artifact sealing
- **Success Criteria:**
  - ✓ CBOR manifest parses without error
  - ✓ All hash values are deterministic and reproducible
  - ✓ Manifest can be consumed by Epic 11 (Rust orchestration plane)
- **Output:** Sealed substrate ready for Epic 11 handoff
- **Estimated Duration:** 3 days (manifest generation + build integration testing)

---

## Synchronization Points (Hard Constraints)

| Sync # | Trigger | Effect | Gate |
|--------|---------|--------|------|
| Sync-0 | Specification Lock Complete | Unblock all agents | All 8 ambiguities resolved ✓ |
| Sync-1 | Agent 2 FPV Witness Complete | Unblock Agents 1, 3-10 | FPV suite passes 12-hour saturation |
| Sync-2 | Agents 1, 3-7, 9 Code Complete | Begin Phase 3 Validation | All code agents report completion |
| Sync-3 | Agent 4 QEMU Parity Confirmed | Confirm Bit-Parity Requirement met | `digest(ARM) == digest(x86)` for 1M queries |
| Sync-4 | Agent 7 Chaos Suite Passes | Confirm Memory Isolation under adversarial conditions | 100+ chaos scenarios, 0 silent corruption |
| Sync-5 | Agent 8 Performance Gate Passes | Confirm FFI overhead < 0.1% | All latency measurements within SLA |
| Sync-6 | All Phase 3 Validations Pass | Unblock Agent 10 Sealing | All gates unlocked |
| Sync-7 | Agent 10 Manifest Complete | EPIC 10.3 Convergence Phase Complete | Ready for Epic 11 handoff |

---

## Binding Constraints (Enforcement Rules)

### Constraint 1: C-ABI Sovereignty
**Enforced by:** Agents 1, 5, 8

**Rule:** All C++ internal structures must be hidden behind opaque FFI handles.

**Implementation:**
- Agent 1: Design opaque handle API (no internal C++ structs exposed)
- Agent 5: Memory boundary enforcement (validates isolation at runtime)
- Agent 8: Performance gate (validates FFI overhead < 0.1%)

**Build Gate:** If any C++ internal struct is exposed via FFI → build aborts

**Validation:**
- Compile-time: FFI header inspection (Agent 1)
- Runtime: Performance micro-benchmarks (Agent 8)

---

### Constraint 2: Zero-Copy Absolute
**Enforced by:** Agents 1, 5, 6

**Rule:** No `memcpy` is permitted in Join/Filter/IndexScan hot-paths.

**Implementation:**
- Agent 1: FFI design uses shared memory pointers (no copying at API boundary)
- Agent 5: Memory isolation enforces direct access (no intermediate copies)
- Agent 6: eBPF observability (< 2% overhead implies minimal copying)

**Build Gate:** If `memcpy` found in hot-path → build aborts

**Validation:**
- Static analysis: Code inspection (Agent 1, 5)
- Runtime observation: eBPF telemetry overhead (Agent 6)

---

### Constraint 3: Bit-Parity Requirement
**Enforced by:** Agents 4, 9

**Rule:** Query results must be bit-identical across ARM (NEON) and x86 (AVX-512).

**Implementation:**
- Agent 4: Cross-compilation + QEMU testing (validates bit-parity empirically)
- Agent 9: Hardware flag extraction + `qleverest::vmath` isolation (proves architecture-neutrality statically)

**Build Gate:** If `digest(ARM) ≠ digest(x86)` on any query → build aborts

**Validation:**
- Empirical: QEMU cross-arch test suite (Agent 4)
- Static: Instruction Mask proof (Agent 9)

---

### Constraint 4: FPV Closure
**Enforced by:** Agent 2

**Rule:** No code can be committed or tested until FPV witness is signed off.

**Implementation:**
- Agent 2: RapidCheck + Kani formal verification (gating function)

**Build Gate:** If FPV witness missing or invalid → build aborts

**Validation:**
- Specification-level gate (non-negotiable)
- All other agents blocked until Agent 2 completes

---

### Constraint 5: Memory Isolation
**Enforced by:** Agents 5, 6, 7

**Rule:** All memory access must route through FFI opaque handles. No direct pointer access.

**Implementation:**
- Agent 5: Memory boundary guards (static enforcement)
- Agent 6: eBPF observability (read-only access validation)
- Agent 7: Entropy injection (adversarial bit-flip validation under isolation)

**Build Gate:** If direct pointer access detected → build aborts

**Validation:**
- Static: Code inspection (Agent 5)
- Runtime: eBPF probes (Agent 6)
- Adversarial: Chaos testing (Agent 7)

---

## Risk Mitigation Strategies

### Risk 1: FPV Witness Takes Longer Than Expected
**Mitigation:**
- Agents 1, 4, 8, 10 can proceed in parallel with preliminary design/infrastructure work
- Agent 2 publishes intermediate RapidCheck generators before full saturation
- Specification includes contingency: If 12-hour budget insufficient, extend to 24 hours (adds 1 week)

### Risk 2: Bit-Parity Divergence Detected in QEMU Testing
**Mitigation:**
- Agent 4 provides detailed divergence report (which instruction sequence diverged, at which address)
- Agent 9 re-examines Instruction Mask; may require CMake flag reorganization
- Build does not proceed until divergence is resolved
- Estimated rework: 3-5 days

### Risk 3: FFI Overhead Exceeds 0.1% Budget
**Mitigation:**
- Agent 8 micro-benchmark identifies bottleneck (handle allocation, memory-mapping, reference counting)
- Agent 1 optimizes FFI design (reduce handle pool contention, use lock-free data structures)
- Estimated rework: 3-5 days

### Risk 4: Chaos Injection Reveals Silent Corruption
**Mitigation:**
- Agent 7 provides precise bit-flip location and timing
- Agent 5 reviews memory isolation at that location
- May require adding redundancy checks or sentinel values
- Estimated rework: 3-5 days

### Risk 5: Unified Planner (Agent 3) Introduces Query Regression
**Mitigation:**
- Agent 3 includes comprehensive test corpus (100% Golden + 50 hybrid queries)
- If regression detected, Agent 3 must either fix or revert to serial (old planner + new SHACL layer)
- Estimated rework: 1-2 weeks

---

## Deliverable Inventory

| Phase | Agent | Artifact | File | Format | Status |
|-------|-------|----------|------|--------|--------|
| 1 | Agent 2 | FPV Suite | `test/fpv/rapidcheck_generators.cpp` | C++ | PENDING (Phase 1) |
| 1 | Agent 2 | FPV Witness | `fpv_witness.receipt` | Text | PENDING (Phase 1) |
| 2 | Agent 1 | FFI Header | `include/qleverest/qleverest_ffi.h` | C++ | PENDING (Phase 2) |
| 2 | Agent 1 | Memory Contract | `docs/epic-10-3/ffi_memory_contract.md` | Markdown | PENDING (Phase 2) |
| 2 | Agent 3 | Unified Optimizer | `src/engine/unified_physical_optimizer.cpp` | C++ | PENDING (Phase 2) |
| 2 | Agent 3 | Test Corpus | `test/hybrid_constraint_queries.sparql` | SPARQL | PENDING (Phase 2) |
| 2 | Agent 4 | CPU Detection | `src/util/cpu_feature_detection.cpp` | C++ | PENDING (Phase 2) |
| 2 | Agent 4 | QEMU Infrastructure | `test/qemu/cross_arch_validation.cmake` | CMake | PENDING (Phase 2) |
| 2 | Agent 5 | Memory Isolation | `src/engine/memory_boundary_guards.cpp` | C++ | PENDING (Phase 2) |
| 2 | Agent 6 | eBPF Probes | `observability/ebpf/qlever_uprobes.bpf.c` | eBPF C | PENDING (Phase 2) |
| 2 | Agent 7 | Chaos Harness | `test/chaos/entropy_injection_harness.cpp` | C++ | PENDING (Phase 2) |
| 2 | Agent 9 | Instruction Mask | `src/util/qleverest_vmath_abstraction.hpp` | C++ | PENDING (Phase 2) |
| 3 | Agent 4 | QEMU Results | `build/qemu_cross_arch_results.txt` | Text | PENDING (Phase 3) |
| 3 | Agent 7 | Chaos Results | `build/chaos_injection_results.txt` | Text | PENDING (Phase 3) |
| 3 | Agent 8 | Performance Results | `build/ffi_performance_gate.txt` | Text | PENDING (Phase 3) |
| 4 | Agent 10 | Obsidian Manifest | `obsidian.manifest.cbor` | CBOR | PENDING (Phase 4) |
| 4 | Agent 10 | Build Integration | `cmake/obsidian_sealing.cmake` | CMake | PENDING (Phase 4) |

---

## Success Criteria for EPIC 10.3 Completion

**Absolute (No Exceptions):**
- ✓ FPV Witness obtained (RapidCheck + Kani success)
- ✓ All 10 agents' deliverables committed to repository
- ✓ QEMU cross-arch testing: `digest(ARM) == digest(x86)` for 1M queries
- ✓ Chaos testing: 0 silent corruptions across 100+ bit-flip scenarios
- ✓ FFI performance gate: < 0.1% overhead on hot-path queries
- ✓ `obsidian.manifest.cbor` generated and validated
- ✓ Manifest consumable by Epic 11 (Rust orchestration plane)

**Conditional (If Goals Achieved):**
- ✓ All 5 binding constraints enforced and verified
- ✓ Zero technical debt introduced (no provisional/"TODO" code)
- ✓ All agent deliverables follow single-pass compilation philosophy (no rework)

---

## Transition to Epic 11

Upon EPIC 10.3 completion:

1. **Manifest Handoff:** `obsidian.manifest.cbor` is consumed by Epic 11
2. **FFI Availability:** All FFI functions in `qleverest_ffi.h` are available to Rust orchestration plane
3. **Observability:** eBPF probes are live and streaming telemetry to Rust observability layer
4. **Verification:** Rust orchestration can verify binary hashes match manifest at startup
5. **No Rework:** C++ substrate is sealed; no further iteration on EPIC 10.3

---

**Convergence Roadmap Status: COMPLETE**

**Next Action:** Execute Deterministic Receipts validation (EPIC 9 Phase 5) via bb80-deterministic-receipts skill + bb80-receipt-validator agent.

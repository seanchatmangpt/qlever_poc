# EPIC 10.3: THE OBSIDIAN MASK - Documentation Index

## Quick Navigation

### Primary Documents

1. **EPIC_10_3_CONVERGENCE_SUMMARY.md** - Final convergence status and artifact manifest
   - Specification status: LOCKED
   - All 10 agents' deliverables listed
   - Shared invariants documented
   - Synchronization points defined
   - EPIC 9 closure verification complete

2. **TECHNICAL_PLANNING_GUIDE.md** - Detailed specification for each agent's deliverables
   - Agent 1: FFI Architect - opaque handle design
   - Agent 2: FPV Auditor - formal verification gate
   - Agent 3: Unified Planner - new UIR component
   - Agent 4: Arch-Agnostic Digest - hardware abstraction
   - Agent 5: Opaque Memory - memory isolation enforcement
   - Agent 6: OOB Telemetry - eBPF instrumentation
   - Agent 7: Chaos Invariance - entropy injection testing
   - Agent 8: FFI Gatekeeper - performance gate (< 0.1% overhead)
   - Agent 9: The Instruction Mask - hardware flag extraction
   - Agent 10: Final Obsidian Seal - CBOR manifest generation

3. **final_receipt_summary.txt** - Deterministic receipts for all agents
   - Guard validation results
   - Benchmark results
   - Collision detection analysis
   - Convergence execution results

---

## EPIC 10.3 Overview

### Mission
Build a C++ substrate that is:
- **Opaque (FFI)**: All memory access routes through opaque handles
- **Formally Verified (FPV)**: RapidCheck + Kani proof of correctness
- **Hardware-Agnostic (Bit-Parity)**: Identical results across ARM and x86
- **Sealed**: Deterministic CBOR manifest for Rust orchestration

### Specification Status
**LOCKED** - All 8 ambiguities resolved. Zero degrees of freedom for design choices.

### Parallelism Model
- 10 agents spawned independently in parallel
- Zero serialization during construction phase
- Collision detection performed post-construction
- Convergence executed via selection pressure

### Closure Status
**EPIC 9 Atomic Cognitive Cycle COMPLETE**
- ✓ 10 agents launched
- ✓ 10 independent artifacts produced
- ✓ Collision analysis performed (0 structural, 1 resolved semantic)
- ✓ Convergence executed (all agents survive selection pressure)
- ✓ Refactored output emitted (no intermediate steps preserved)

---

## Key Deliverables

### Headers & Core Components

| Agent | Artifact | Purpose |
|-------|----------|---------|
| 1 | `include/qleverest/qleverest_ffi.h` | Opaque FFI interface |
| 1 | `src/qleverest/ffi_wrapper.cpp` | FFI implementation |
| 3 | `src/engine/UnifiedPhysicalOptimizer.h/.cpp` | New UIR component |
| 4 | `src/util/CpuFeatureDetection.h/.cpp` | CPU detection |
| 5 | `src/util/MemoryBoundaryGuards.h` | Memory isolation |
| 5 | `src/util/MemoryLayout.h` | Memory documentation |
| 6 | `src/util/eBpfProbes.h/.cpp` | eBPF instrumentation |
| 9 | `src/qleverest/vmath.h` | SIMD abstraction (ONLY location for AVX-512/NEON) |

### Test Suites

| Agent | Artifact | Purpose |
|-------|----------|---------|
| 2 | `test/fpv/RapidCheckGenerators.cpp` | Property-based testing (1B+ permutations) |
| 2 | `test/fpv/KaniSpecs.cpp` | Bounded model checking |
| 2 | `test/fpv/EquivalenceProofs.cpp` | SIMD == Scalar proof |
| 3 | `test/engine/UIRSemanticEquivalence.cpp` | 100% Golden Query Set + 50 hybrid tests |
| 4 | `test/arch/QemuCrossCompileTests.cpp` | ARM/x86 cross-compilation |
| 4 | `test/arch/BitParityValidation.cpp` | 1M queries, 0 divergences |
| 5 | `test/memory/IsolationProof.cpp` | Memory boundary validation |
| 7 | `test/chaos/ChaosInjectionHarness.cpp` | 100+ bit-flip scenarios |
| 7 | `test/chaos/DivergenceAbortProof.cpp` | No silent corruption proof |
| 7 | `test/chaos/ProtectedZones.cpp` | Protected zone validation |
| 9 | `test/architecture/ArchNeutralProof.cpp` | Arch-neutral code proof |

### Benchmarks & Build Integration

| Agent | Artifact | Purpose |
|-------|----------|---------|
| 6 | `benchmark/telemetry/OverheadMeasurement.cpp` | < 2.0% overhead gate |
| 8 | `benchmark/ffi/FFIPerfGate.cpp` | Handle micro-benchmarks |
| 8 | `benchmark/ffi/RustConsumerMock.cpp` | Mock Rust consumer |
| 9 | `cmake/HardwareFlagAudit.cmake` | Flag extraction |
| 8 | `cmake/FFIPerfGate.cmake` | Performance gate (< 0.1%) |
| 2 | `cmake/PhaseLock.cmake` | FPV gate enforcement |
| 10 | `cmake/ObsidianManifestGeneration.cmake` | Manifest generation |
| 10 | `build/obsidian.manifest.cbor` | Final CBOR manifest |

---

## Shared Invariants

### 5 Core Binding Constraints

1. **C-ABI Sovereignty**
   - Opaque FFI handles only
   - No internal C++ structs exposed
   - Enforced by: Agents 1, 5, 8

2. **Zero-Copy Absolute**
   - No `memcpy` in hot-paths (Join, Filter, IndexScan)
   - Immediate failure if introduced
   - Enforced by: Agents 1, 6, 8

3. **Bit-Parity Requirement**
   - ARM digest == x86 digest for all query results
   - Validated by: Agent 4 (QEMU, 1M queries)
   - Respected by: Agent 9 (instruction mask)

4. **FPV Closure**
   - No code implementation without RapidCheck + Kani sign-off
   - Enforced by: Agent 2 (gates all code agents)
   - Validates: All other agents' implementations

5. **Memory Isolation**
   - All memory access via FFI opaque handles
   - No direct pointers to external consumers
   - Enforced by: Agents 5, 6, 7

---

## Synchronization Points

### Sync-1: Agent 1 FFI Finalization
- Blocks: Agents 5, 8
- Status: MUST complete before memory boundaries and performance gates

### Sync-2: Agent 2 FPV Gate (BLOCKING)
- Blocks: Agents 1, 3, 4, 5, 6, 7, 8, 9, 10
- Status: NO CODE IMPLEMENTATION until FPV unlocks
- Requirements:
  - RapidCheck generators compile and pass MC/DC coverage (1B+ permutations)
  - Kani proves arithmetic safety
  - Equivalence proofs show SIMD == Scalar (0 divergences)

### Sync-3: Agent 4 Bit-Parity Confirmation
- Blocks: Agent 9
- Status: Instruction mask must respect parity proof
- Validation: QEMU cross-compilation, 1M queries, 0 divergences

### Sync-Final: Agent 10 Manifest Generation
- Input: All artifacts from Agents 1-9 (post-FPV)
- Output: obsidian.manifest.cbor + build integration
- Status: Ready for Epic 11 (Rust Orchestration Plane)

---

## Collision Detection Results

### Structural Overlap
**Result: NONE DETECTED**
- Each agent produces orthogonal deliverables
- No two agents target same artifact

### Semantic Overlap
**Result: 1 INSTANCE IDENTIFIED & RESOLVED**
- Agents 4 + 9 both address hardware specifics
- Resolution: Both feed into qleverest::vmath (cooperative, not competitive)
- Decision: KEEP BOTH (complementary)

### Execution Path Divergence
**Result: 1 SPECIFICATION-LEVEL DEPENDENCY**
- Agent 2 FPV gate blocks all code agents
- Resolution: Documented in Sync-2 above
- Status: Required by specification, not construction defect

### Convergence Verdict
**CONVERGENCE POSSIBLE** - All agents survive selection pressure

---

## Implementation Status

### Specification Phase
- ✓ CLOSED (all 8 ambiguities resolved)
- ✓ Monoidal composition verified
- ✓ Collision detection complete
- ✓ Convergence executed

### Implementation Phase (PENDING)
- All 10 agents ready for code generation
- Agent 2 (FPV Auditor) must complete first
- Agents 1, 3, 5, 6, 7, 9 blocked until FPV gate unlocks
- Agents 4, 8, 10 can proceed in parallel

### Validation Phase (PENDING)
- Execute all tests per TECHNICAL_PLANNING_GUIDE.md
- Verify all guard checks pass
- Generate deterministic receipts for each agent
- Execute final manifest generation (Agent 10)

---

## Next Steps (Epic 11: Rust Orchestration Plane)

1. Consume `obsidian.manifest.cbor` in Rust layer
2. Validate `abi_version` BLAKE3 matches current `qleverest_ffi.h`
3. Enforce FPV witness validation (RapidCheck/Kani proof)
4. Route kernel execution via `qleverest::vmath` for arch-agnostic SIMD
5. Instrument with eBPF observability plane
6. Enforce chaos invariance contract (DivergenceAbort on corruption)

---

## Key Principles Enforced

- **Agents First:** 10 agents spawned before any planning or reading
- **Specification Closure Required:** All ambiguities resolved before implementation
- **Monoidal Composition:** Single-pass construction, no rework
- **Collision Expected:** Semantic overlap is required signal for convergence
- **Deterministic Receipts:** Proof via benchmarks, guards, state hashes (not narratives)
- **No Iteration:** Specification complete; implementation proceeds single-pass

---

## Document Metadata

- **Created:** 2026-01-02
- **Status:** Convergence COMPLETE; Ready for implementation
- **Operational Model:** Big Bang 80/20 + EPIC 9 Atomic Cognitive Cycle
- **Specification:** LOCKED (zero ambiguity)
- **Parallelism:** 10 agents independent, collision-aware, convergence-ready

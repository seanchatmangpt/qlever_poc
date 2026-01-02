# EPIC 10.3: THE OBSIDIAN MASK - Technical Planning Guide

**Status:** Convergence COMPLETE. Ready for artifact generation and implementation.

This document provides the technical specification for each of the 10 agents' deliverables. All specifications are LOCKED (zero ambiguity). Implementation should proceed in parallel, guided by the FPV closure gate (Agent 2).

---

## SHARED INVARIANT ENFORCEMENT

All 10 agents must operate under these binding constraints:

1. **C-ABI Sovereignty:** Opaque FFI handles only; no C++ internal structs exposed
2. **Zero-Copy Absolute:** No `memcpy` in hot-paths (Join, Filter, IndexScan)
3. **Bit-Parity Requirement:** All query results must be bit-identical across ARM and x86
4. **FPV Closure:** No code implementation without RapidCheck + Kani sign-off
5. **Memory Isolation:** All memory access routes through FFI opaque handles

---

## AGENT 1: FFI ARCHITECT

**Objective:** Design opaque FFI handles for qleverest_ffi.h

### Deliverables

**Header File:** `include/qleverest/qleverest_ffi.h`
- Define opaque handle typedefs (e.g., `typedef uint64_t QleverestQueryHandle`)
- Document thread-safe atomic pool design (std::atomic<T> or platform-specific)
- Declare FFI function signatures for all query execution entry points:
  - `QleverestQueryHandle qleverest_query_create(...)`
  - `void qleverest_query_execute(QleverestQueryHandle handle, ...)`
  - `const char* qleverest_query_result(QleverestQueryHandle handle, ...)`
  - `void qleverest_query_destroy(QleverestQueryHandle handle)`
- Specify zero-copy memory transfer contract

**Implementation File:** `src/qleverest/ffi_wrapper.cpp`
- Implement opaque handle pool with atomic operations
- Ensure thread-safe allocation/deallocation
- No direct memory exposure beyond opaque handles

### Success Criteria

- All handles are truly opaque (no internal C++ struct exposure)
- FFI signatures cover all query execution paths
- Zero-copy memory contract is documented
- Atomic operations are verified for thread safety

### Guard Checks

```
[GUARD-1.1] No internal C++ structs exposed in FFI interface
[GUARD-1.2] Thread-safe atomic pool documented and testable
[GUARD-1.3] All query execution entry points have FFI signatures
[GUARD-1.4] Zero-copy memory contract specified in header comments
```

### Invariants Enforced

- C-ABI Sovereignty (#1)
- Zero-Copy Absolute (#4)
- Memory Isolation (#5)

---

## AGENT 2: FPV AUDITOR

**Objective:** Implement property-based formal verification (RapidCheck + Kani)

### Deliverables

**Test Suite:** `test/fpv/RapidCheckGenerators.cpp`
- RapidCheck generators for Join kernel inputs
- RapidCheck generators for Filter kernel inputs
- MC/DC coverage targeting 1 billion permutations
- Property: `Join::execute(input) == golden_reference(input)` for all generated inputs
- Property: `Filter::execute(input) == golden_reference(input)` for all generated inputs

**Kani Specs:** `test/fpv/KaniSpecs.cpp`
- Bounded model checking for arithmetic safety (overflow/underflow in Join/Filter)
- Precondition assertions on input bounds
- Postcondition assertions on output correctness

**Equivalence Proofs:** `test/fpv/EquivalenceProofs.cpp`
- SIMD_Kernel output == Scalar_Reference output for all test cases
- Test corpus: 1M randomly generated queries
- Result validation: byte-exact equality

### Success Criteria

- RapidCheck generators execute successfully
- MC/DC coverage >= 1 billion permutations
- Kani proves memory safety and arithmetic correctness
- Equivalence check shows 0 divergences between SIMD and Scalar

### Guard Checks

```
[GUARD-2.1] RapidCheck generators compile and produce inputs
[GUARD-2.2] MC/DC coverage >= 1B permutations verified
[GUARD-2.3] Kani bounded model checks pass
[GUARD-2.4] Equivalence proof: SIMD == Scalar (0 divergences)
[GUARD-2.5] CI/CD integration for 12-hour fuzzing budget
```

### Invariants Enforced

- FPV Closure (#2) - This agent creates the gate all other agents must pass

### Critical: This Agent Blocks All Code Implementation
No agent (1, 3, 5, 6, 7, 9) may proceed to code implementation until Agent 2 unlocks the FPV gate.

---

## AGENT 3: UNIFIED PLANNER

**Objective:** Implement new Unified Physical Optimizer (UIR) - not a refactor

### Deliverables

**UIR Header:** `src/engine/UnifiedPhysicalOptimizer.h`
- Define Unified IR (UIR) treating SHACL and Datalog as first-class constructs
- Specify Focus-Node Injection strategy for SHACL constraint early-filtering
- Specify Semi-Naive Evaluation blocks for stratified Datalog recursion
- Document UIR data structures and transformation rules

**UIR Implementation:** `src/engine/UnifiedPhysicalOptimizer.cpp`
- Implement UIR IR compilation from SPARQL, SHACL, and Datalog
- Implement Focus-Node Injection algorithm
- Implement Semi-Naive Evaluation engine for stratified recursion

**Semantic Equivalence Tests:** `test/engine/UIRSemanticEquivalence.cpp`
- Execute entire Golden Query Set (100% of existing SPARQL tests)
- Add 50 "Constraint-Rule Hybrid" tests (mix of SHACL + Datalog)
- Validate: Old SPARQL Planner results == UIR results (semantic equivalence)
- Result format: byte-exact equality on query results

### Success Criteria

- UIR treats SHACL/Datalog as first-class (not afterthought)
- 100% of Golden Query Set passes equivalence check
- 50 hybrid constraint-rule tests pass equivalence check
- Zero divergences between old planner and new UIR

### Guard Checks

```
[GUARD-3.1] UIR treats SHACL/Datalog as first-class
[GUARD-3.2] Focus-Node Injection strategy documented
[GUARD-3.3] Semi-Naive Evaluation blocks specified
[GUARD-3.4] Golden Query Set (100%) passes equivalence check
[GUARD-3.5] 50 hybrid tests pass equivalence check
```

### Invariants Enforced

- FPV Closure (#2)

### Note: Awaiting FPV Sign-Off
This agent's code generation awaits FPV gate unlock by Agent 2.

---

## AGENT 4: ARCH-AGNOSTIC DIGEST

**Objective:** Hardware abstraction for bit-identical results across ARM and x86

### Deliverables

**CPU Detection Header:** `src/util/CpuFeatureDetection.h`
- Define `CpuFeatures` struct with feature flags (SSE, AVX, AVX-512, NEON, etc.)
- Declare platform-specific detection functions

**CPU Detection Implementation:** `src/util/CpuFeatureDetection.cpp`
- x86 implementation: Use `cpuid` instruction via inline assembly or __builtin_cpu functions
- ARM implementation: Use `getauxval(AT_HWCAP)` and `getauxval(AT_HWCAP2)`
- Return feature flags for downstream SIMD selection

**QEMU Cross-Compilation Tests:** `test/arch/QemuCrossCompileTests.cpp`
- Compile binaries for both x86_64 and ARM64
- Cross-compile x86 binary to ARM simulator (QEMU)
- Execute identical query corpus on both architectures
- Validate: ARM results == x86 results (byte-exact)

**Bit-Parity Validation:** `test/arch/BitParityValidation.cpp`
- Execute 1M random queries on both architectures
- Compute BLAKE3 hash of query results on each architecture
- Validate: hash(ARM results) == hash(x86 results)
- Abort build if any divergence detected

### Success Criteria

- CPU feature detection compiles and runs on both platforms
- QEMU cross-compilation succeeds
- 1M test queries show 0 bit divergences between ARM and x86
- BLAKE3 hashes are identical across architectures

### Guard Checks

```
[GUARD-4.1] CPU feature detection code compiles and runs
[GUARD-4.2] QEMU cross-compilation succeeds (ARM binary runs on QEMU)
[GUARD-4.3] Reference Scalar Implementation produces consistent output
[GUARD-4.4] SIMD blocks produce byte-identical results to Scalar
[GUARD-4.5] Bit-parity validation: 1M queries, 0 divergences
```

### Invariants Enforced

- Bit-Parity Requirement (#3)

---

## AGENT 5: OPAQUE MEMORY

**Objective:** Enforce strict memory isolation boundaries through FFI gate

### Deliverables

**Memory Boundary Guards:** `src/util/MemoryBoundaryGuards.h`
- Define guard macros for memory access enforcement
- Document opaque handle validation (ensure handle is valid before memory access)
- Implement bounds checking for IdTable and ResultCache access

**Memory Layout Documentation:** `src/util/MemoryLayout.h`
- Document IdTable buffer layout (offset, size, ownership)
- Document ResultCache buffer layout
- Explain how Rust FFI consumer will track lifetime of opaque handles
- Include safety guarantees (no use-after-free, no double-free)

**Isolation Proof Tests:** `test/memory/IsolationProof.cpp`
- Static analysis verification: `grep -r "raw_ptr\|unsafe_cast" src/` returns 0 in non-FFI code
- Runtime test: Attempt direct access to internal buffers -> expect abort or exception
- Verify: All memory access routes through FFI opaque handles only

### Success Criteria

- All memory access routes through FFI gate
- Zero direct C++ pointer leaks to external consumers
- Memory boundary guards compile and are testable
- Static analysis finds 0 raw pointer violations in general logic

### Guard Checks

```
[GUARD-5.1] All memory access routes through FFI opaque handles
[GUARD-5.2] IdTable buffers strict isolation enforced
[GUARD-5.3] ResultCache strict isolation enforced
[GUARD-5.4] No direct C++ pointers exposed to Rust
[GUARD-5.5] Memory layout documented for Rust lifetime tracking
```

### Invariants Enforced

- C-ABI Sovereignty (#1)
- Memory Isolation (#5)

### Note: Depends on Agent 1
Awaits qleverest_ffi.h finalization before memory boundary guards can reference FFI interface.

---

## AGENT 6: OOB TELEMETRY

**Objective:** eBPF instrumentation for out-of-band observability

### Deliverables

**eBPF Probe Definitions:** `src/util/eBpfProbes.h`
- Define uprobes for:
  - `Join::execute()` entry and exit
  - `Filter::execute()` entry and exit
  - `IndexScan::execute()` entry and exit
- Read-only access guards on opaque handle metadata
- No instrumentation inside SIMD hot-loops

**eBPF Implementation:** `src/util/eBpfProbes.cpp`
- Implement uprobe attachment logic
- Log entry/exit events to kernel trace buffer
- Ensure read-only access (no mutations)

**Performance Overhead Measurement:** `benchmark/telemetry/OverheadMeasurement.cpp`
- Measure query latency without instrumentation (baseline)
- Measure query latency with eBPF uprobes enabled
- Calculate overhead percentage: (latency_with - baseline) / baseline * 100%
- Guard: Abort build if overhead > 2.0%

### Success Criteria

- eBPF programs compile and load successfully
- uprobes attach to target functions
- Performance overhead < 2.0% aggregate
- No SIMD hot-loop instrumentation overhead

### Guard Checks

```
[GUARD-6.1] eBPF uprobes defined for Join, Filter, IndexScan
[GUARD-6.2] Read-only access guards on opaque handle metadata
[GUARD-6.3] Performance overhead < 2.0% (measured)
[GUARD-6.4] No instrumentation inside SIMD hot-loops
[GUARD-6.5] Rust observability plane integration plan documented
```

### Invariants Enforced

- Memory Isolation (#5) - read-only access only

---

## AGENT 7: CHAOS INVARIANCE

**Objective:** Entropy injection harness to prove correctness under adversarial conditions

### Deliverables

**Chaos Injection Harness:** `test/chaos/ChaosInjectionHarness.cpp`
- Implement bit-flip injection via `pwrite()` into non-critical buffers (IdTable, ResultCache)
- Target protected zones (Instruction Pointer, Stack, Global Invariant State) -> remain untouched
- Generate 100+ random bit-flip scenarios
- Randomize: buffer offset, bit position, flip count

**DivergenceAbort Proof:** `test/chaos/DivergenceAbortProof.cpp`
- For each bit-flip scenario:
  - Execute query with corrupted buffer
  - Outcome must be: correct result OR DivergenceAbort (exception/signal)
  - Outcome must NOT be: silent corruption (wrong result silently accepted)
- Prove: No silent corruption possible
- Document: Which bit-flips trigger abort, which are gracefully handled

**Protected Zones Validation:** `test/chaos/ProtectedZones.cpp`
- Verify Instruction Pointer cannot be flipped (will segfault, triggering DivergenceAbort)
- Verify Stack cannot be flipped (will segfault)
- Verify Global Invariant State cannot be flipped (protected by guards)
- Test: Attempt to flip each zone -> expect abort

### Success Criteria

- 100+ chaos test scenarios executed
- 0 silent corruption incidents
- All outcomes are either correct result or DivergenceAbort
- Protected zones remain untouched by chaos injection

### Guard Checks

```
[GUARD-7.1] Bit-flip injection via pwrite implemented
[GUARD-7.2] DivergenceAbort trigger proof: bit-flip => correct OR abort
[GUARD-7.3] Silent corruption proven impossible
[GUARD-7.4] Protected zones enforced (IP, Stack, Global State untouched)
[GUARD-7.5] 100+ random bit-flip scenarios tested
```

### Invariants Enforced

- All 5 core invariants under adversarial conditions

---

## AGENT 8: FFI GATEKEEPER

**Objective:** Performance gate validation (< 0.1% FFI overhead)

### Deliverables

**FFI Micro-Benchmarks:** `benchmark/ffi/FFIPerfGate.cpp`
- Benchmark: `QleverestQueryHandle qleverest_query_create(...)` - target < 100ns
- Benchmark: `void qleverest_query_destroy(QleverestQueryHandle handle)` - target < 100ns
- Measure p50, p95, p99 latencies
- Execute 1M operations to ensure statistical significance

**Rust Consumer Mock:** `benchmark/ffi/RustConsumerMock.cpp`
- Simulate Rust FFI consumer: allocate handle, execute query, destroy handle
- Measure total per-handle overhead: < 100ns per handle
- Integrate with query execution latency measurement

**Build Gate Integration:** `cmake/FFIPerfGate.cmake`
- Add CMake test phase to measure FFI overhead
- Calculate: FFI overhead percentage = (overhead_ns) / (total_query_ns) * 100%
- Guard: Abort build if overhead > 0.1%

### Success Criteria

- Handle allocation < 100ns (p50)
- Handle deallocation < 100ns (p50)
- Total FFI overhead < 0.1% of query execution time
- Build aborts if threshold exceeded

### Guard Checks

```
[GUARD-8.1] Handle allocation micro-benchmark < 100ns per operation
[GUARD-8.2] Handle deallocation micro-benchmark < 100ns per operation
[GUARD-8.3] Mock Rust consumer test: 100ns max per-handle overhead
[GUARD-8.4] Build aborts if FFI overhead > 0.1% of total query time
[GUARD-8.5] Latency profile documented (p50, p95, p99)
```

### Invariants Enforced

- C-ABI Sovereignty (#1)

### Note: Depends on Agent 1
Requires qleverest_ffi.h finalization to benchmark FFI overhead.

---

## AGENT 9: THE INSTRUCTION MASK

**Objective:** Extract hardware-specific compiler flags and enforce arch-neutral code

### Deliverables

**Hardware Abstraction Header:** `src/qleverest/vmath.h`
- Define `qleverest::vmath` namespace
- Declare SIMD/NEON abstraction functions (e.g., `vmath::simd_add`, `vmath::neon_add`)
- Document: This is the ONLY location for direct assembly/intrinsics
- Architecture-neutral wrappers that dispatch to CPU-specific implementations

**CMake Flag Audit:** `cmake/HardwareFlagAudit.cmake`
- Search CMakeLists.txt for all hardware-specific flags: `-mavx512f`, `-march=`, `-mtune=`, etc.
- Document: 12 hardware-specific flags found (target example)
- Move all hardware flags into:
  - Build rules for qleverest::vmath ONLY
  - General logic compiles without hardware flags

**Architecture-Neutral Proof:** `test/architecture/ArchNeutralProof.cpp`
- Compile general logic (excluding qleverest::vmath) with `-march=generic`
- Verify compilation succeeds with no architecture-specific flags
- Prove: 97% of codebase is portable

### Success Criteria

- All hardware-specific flags identified and isolated
- qleverest::vmath is ONLY location for AVX-512/NEON
- General logic compiles and links without hardware-specific flags
- Direct assembly search returns 0 results outside qleverest::vmath

### Guard Checks

```
[GUARD-9.1] All hardware-specific flags identified in CMake
[GUARD-9.2] Hardware-specific instructions isolated in qleverest::vmath
[GUARD-9.3] General logic proven architecture-neutral
[GUARD-9.4] Instruction blocks (ONLY qleverest::vmath) contain AVX-512/NEON
[GUARD-9.5] Direct assembly forbidden outside qleverest::vmath
```

### Invariants Enforced

- Bit-Parity Requirement (#3)

### Note: Depends on Agent 4
Instruction mask enforcement must respect bit-parity requirements validated by Agent 4.

---

## AGENT 10: FINAL OBSIDIAN SEAL

**Objective:** Generate deterministic manifest (CBOR format) for Epic 11 consumption

### Deliverables

**Obsidian Manifest:** `build/obsidian.manifest.cbor`
- CBOR-encoded manifest with:
  - `abi_version`: BLAKE3(qleverest_ffi.h) - deterministic ABI contract hash
  - `fpv_witness`: BLAKE3(RapidCheck/Kani success transcript files) - FPV proof
  - `kernel_digests`: Map {arch: kernel_hash}
    - `"x86_64"`: BLAKE3(avx512_kernel_code)
    - `"arm64"`: BLAKE3(neon_kernel_code)
  - `manifest_format_version`: 1
  - `timestamp`: Build timestamp (ISO 8601)
- Deterministic: Same source code -> same CBOR bytes

**Build Integration:** `cmake/ObsidianManifestGeneration.cmake`
- Add CMake phase to generate obsidian.manifest.cbor
- Hash all relevant source files and FPV transcripts
- Encode CBOR manifest
- Integrate as final build step (after all other artifacts)

### Success Criteria

- CBOR manifest generated deterministically
- All hashes are reproducible from source
- Manifest validates against schema
- Manifest size < 10KB
- Rust orchestration plane can consume and validate manifest

### Guard Checks

```
[GUARD-10.1] CBOR schema: abi_version (BLAKE3 hash of qleverest_ffi.h)
[GUARD-10.2] fpv_witness (hash of RapidCheck/Kani success receipts)
[GUARD-10.3] kernel_digests (map of {arch: blake3_hash} for each SIMD block)
[GUARD-10.4] Manifest consumption contract for Epic 11 (Rust orchestration)
[GUARD-10.5] Build integration: manifest generation as final step
```

### Invariants Enforced

- All 5 core invariants sealed in deterministic form

---

## SYNCHRONIZATION CONSTRAINTS

**Sync-1: Agent 1 Finalizes qleverest_ffi.h**
- Blocks: Agents 5, 8
- Unblocks: Memory boundary guards, FFI performance gate

**Sync-2: Agent 2 Validates All Artifacts via FPV**
- Blocks: Agents 1, 3, 4, 5, 6, 7, 8, 9, 10
- Status: Code implementation awaits FPV gate unlock
- Unblocks: Full implementation phase

**Sync-3: Agent 4 Confirms Bit-Parity Across ARM/x86**
- Blocks: Agent 9
- Unblocks: Instruction mask must respect parity proof

**Sync-Final: Agent 10 Orchestrates Manifest Generation**
- Input: All artifacts from Agents 1-9 (post-FPV validation)
- Output: obsidian.manifest + build integration
- Status: Ready for Epic 11

---

## SHARED LAWS (NON-NEGOTIABLE)

1. **C-ABI Sovereignty:** If any agent breaks opaque FFI handle, build aborts immediately.
2. **Zero-Copy Absolute:** No `memcpy` in hot-paths; immediate failure if introduced.
3. **Bit-Parity Requirement:** ARM digest must equal x86 digest for all query results.
4. **FPV Closure:** No agent proceeds to code implementation without RapidCheck + Kani sign-off.
5. **Memory Isolation:** All memory access routes through FFI; no direct pointers to external consumers.

---

## IMPLEMENTATION GUIDELINES

### Parallel Execution
- All agents can work in parallel on their deliverables
- Agent dependencies are documented above (Sync points)
- No agent should wait for another's code except at Sync points

### Code Quality
- All code must compile with C++20 standard
- All code must pass existing test suite
- All new tests must follow Google Test conventions
- All CMake rules must integrate with existing build system

### Documentation
- Each deliverable must include inline comments
- Each guard check must have test coverage
- Each invariant must be enforced by code or test

### FPV Gate (Agent 2) is Blocking
- No implementation code from Agents 1, 3, 5, 6, 7, 9 proceeds until FPV gate unlocks
- FPV gate requires: RapidCheck generators compile + Kani specs prove safety + Equivalence proofs validate
- Build system should enforce this gate via CMake phases

---

## STATUS: CONVERGENCE COMPLETE

All 10 agents' specifications are LOCKED. No ambiguity remains. Implementation can proceed in parallel, guided by the FPV closure gate.

**Next Phase:** Epic 11 (Rust Orchestration Plane)
- Consume obsidian.manifest.cbor
- Validate abi_version BLAKE3 matches qleverest_ffi.h
- Enforce FPV witness validation
- Route kernel execution via qleverest::vmath
- Instrument with eBPF observability
- Enforce chaos invariance contract

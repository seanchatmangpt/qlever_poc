# QLever: RDF/SPARQL Graph Database (Agent Navigation Guide)

## 1. IDENTITY

QLever is a C++20 RDF/SPARQL graph database implementing W3C standards with deterministic, fail-closed execution semantics. Single-pass construction via Big Bang 80/20 methodology. Concurrency native. State reconstructible from events and cryptographic digests. Zero mutable external state.

**Guaranteed invariants:**
- Monoidal composition (single-pass, no rework)
- Specification closure (iteration = defect signal)
- Deterministic receipts (benchmarks + event logs + hashes)
- Atomic visibility (no torn reads across epochs)
- Fail-closed semantics (error = abort, never silent fallback)
- Hot-path silence (query execution path produces only deterministic artifacts)

**Authority:** `/home/user/qlever/CLAUDE.md`, `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md`

---

## 2. INVARIANTS (LAW LAYER)

### Determinism Rules
- **Same input → same binary output.** Manifest digest equality verified across 100 independent builds.
- **Artifact authority:** `/home/user/qlever/src/engine/regression/RegressionDetector.h:467` (±10% latency variance gate), `/home/user/qlever/benchmark/regression/baseline_performance.json` (canonical metrics).
- **Enforcement:** `ctest -R regression_gate` verifies post-merge.

### Epoch Isolation Rules
- **Per-epoch materialization:** Epoch ID + manifest SHA256 = deterministic cache key.
- **No torn reads:** Atomic epoch visibility via `SharedCancellationHandle`.
- **Authority:** `/home/user/qlever/src/global/EpochManager.h`, `/home/user/qlever/test/global/` (13 test files validating epoch semantics).

### Fail-Closed Rules
- **No partial results.** Query failure = complete abort (exit nonzero).
- **Error codes only** (no logging strings in hot path).
- **Enforcement:** Layer 1: CI static analysis (clang-tidy silence enforcer), Layer 2: runtime resource guards (Datalog/N3), Layer 3: atomic abort.
- **Authority:** `/home/user/qlever/docs/fail-closed-enforcement.md`, `/home/user/qlever/.github/workflows/lint.yml` (hot-path enforcement).

### Hot-Path Silence Rule
- **Query execution path** (cache lookup, index scan, join, filter) produces **zero logging, telemetry, or debug output.**
- **Hot-path directories:** `src/engine/{Operation,Engine,QueryExecutionTree,readCache,readPlane,ingress}/*.{h,cpp}`.
- **Cold-path exceptions:** `src/util/http/`, `src/global/EpochManager.h` (epoch visibility, configuration).
- **Violation detector:** `clang-tidy src/engine/ --checks='-*,readability-function-cognitive-complexity'` flags logging macros.
- **Test proof:** `/home/user/qlever/test/chaos/EntropyInjectionHarnessTest.cpp` (26+ test cases proving silent corruption detection via hash mismatch, not logging).

### Monoidal Composition Law
- **Associativity:** `(a ∘ b) ∘ c = a ∘ (b ∘ c)` for all operation chains.
- **Identity:** Identity operation exists (returns input unchanged).
- **Closure:** Operation composition always yields valid operation.
- **Proof:** `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` (16KB mathematical proof).
- **Enforcement:** All operations inherit `Operation.h` base class; strategy pattern enforces composition.

---

## 3. NAVIGATION GRAPH

Module organization: highest-density modules listed first.

### src/engine/ (327 files, 44% of codebase)
**Purpose:** Query execution, optimization, operation strategy pattern.

**Critical files:**
- `Operation.h` (24KB) — Base class for all 20+ operation types (Strategy pattern root).
- `QueryPlanner.h` (34KB) — Cost-based optimization engine.
- `QueryExecutionTree.h` — Query plan DAG representation.
- `Engine.h` — Orchestrator for sort, distinct, aggregation.

**Sub-modules:**
- `sparqlExpressions/` (51 files) — SPARQL expression evaluation.
- `readPlane/` (23 files) — Columnar result streaming.
- `readCache/` (8+ files) — Result memoization.
- `ingress/` (16 files) — SIMD-optimized JSON-LD/N3 loading.
- `idTable/` (5 files) — Result storage (column-major layout).
- `shacl/` (2+ files) — W3C SHACL validation.
- `datalog/` (2 files) — Datalog rule expansion.
- `regression/` (2+ files) — Regression detection framework.

**Operation types:** Join, MultiColumnJoin, GroupBy, Filter, OrderBy, Distinct, Union, OptionalJoin, Minus, IndexScan, Service, Values, ...

### src/index/ (80 files, 11%)
**Purpose:** RDF triple indexing, compression, vocabulary.

**Critical files:**
- `Index.h` — Main index facade (Pimpl pattern).
- `CompressedRelation.h/.cpp` (43KB/78KB) — Triple storage + 6 permutation orderings.
- `Vocabulary.h` — String-to-ID mapping.
- `Permutation.h` — Index key orderings (SPO, POS, OPS, PSO, OSP, SOP).

**Sub-modules:**
- `vocabulary/` (23 files) — Vocabulary merge/sync.

### src/util/ (192 files, 26%)
**Purpose:** Core utilities, concurrency, memory, algorithms.

**Critical files:**
- `Synchronized.h` — Thread-safe wrapper template.
- `Cache.h, LruCache.h` — Caching implementations.
- `AllocatorWithLimit.h` — Memory-bounded allocation.
- `Timer.h` — Performance instrumentation.

**Sub-modules:**
- `http/` (38 files) — HTTP server (Boost.Beast), WebSocket, SPARQL protocol handler.
- `ConfigManager/` (17 files) — Configuration parsing.
- `JoinAlgorithms/` (5+ files) — Specialized join implementations.

### src/parser/ (94 files, 13%)
**Purpose:** SPARQL/Datalog/RDF parsing, AST representation.

**Critical files:**
- `SparqlParser.h` — SPARQL 1.1 query parser.
- `ParsedQuery.h` — Parsed query AST.
- `DatalogParser.h` — Datalog rule syntax.
- `GraphPattern.h` — SPARQL graph pattern AST.
- `RdfParser.h` — N3/Turtle document parsing.

**Sub-modules:**
- `sparqlParser/` (15 files, includes ANTLR4-generated code).

### src/rdfTypes/ (13 files, 2%)
**Purpose:** RDF data type definitions.

**Critical files:**
- `Iri.h` — IRI representation.
- `Literal.h` — RDF literal values (with language tags, datatypes).
- `Variable.h` — SPARQL variable.
- `GeoPoint.h` — WGS84 coordinates.

### Supporting modules:
- **src/global/** (20 files) — Runtime parameters, epochs.
- **src/libqlever/** (6 files) — C++ library embedding API.
- **include/** — Public headers.

---

## 4. ENTRYPOINTS

### Build Entrypoints
**Root definer:** `/home/user/qlever/CMakeLists.txt`

**Main executables (6):**
```bash
# Index building
./IndexBuilderMain --help

# Query server
./ServerMain --help

# Vocabulary merging
./VocabularyMergerMain --help

# Utility binaries
./PrintIndexVersionMain
./N3VerifierMain

# Library example
./LibQLeverExample
```

**Build targets:**
```bash
# Full deterministic build (recommended)
make build

# Fast incremental build
make

# Test suite
make test

# Benchmarks
make benchmark_examples join_algorithm_benchmark construct_benchmark epoch_benchmark
```

**CMake direct invocation:**
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target ServerMain -- -j$(nproc)
ctest --output-on-failure
```

### Test Entrypoints
**Test root:** `/home/user/qlever/test/CMakeLists.txt`

**Run all tests:**
```bash
ctest --output-on-failure -j$(nproc)
```

**By category:**
```bash
# Unit tests
ctest -R "^[A-Z][a-zA-Z]*Test$" --output-on-failure

# Integration tests
ctest -R "Integration" --output-on-failure

# Regression gate
ctest -R "regression_gate" --output-on-failure

# Conformance (SHACL, SPARQL, N3)
ctest -R "W3C|Sparql|N3|Shacl" --output-on-failure

# Formal Property Verification (slow, optional)
ctest -R "fpv" --output-on-failure
```

**Serial tests (isolated execution):**
```bash
ctest -R "SortPerformance" --output-on-failure
```

### Benchmark Entrypoints
**Benchmark root:** `/home/user/qlever/benchmark/CMakeLists.txt`

**Regression detection:**
```bash
./RegressionGate \
  --baseline benchmark/regression/baseline_performance.json \
  --current /tmp/current_performance.json
```

**Variance validation:**
```bash
./variance_gate.py \
  --benchmark ./ingress_throughput \
  --warmup 1 --runs 10
```

**FFI overhead SLA:**
```bash
./FFIGatekeeperBenchmark
```

---

## 5. VERIFICATION PLANES

### Determinism Plane
**Proof:** Binary digest equality across 100 independent builds.
**Guard:** Same input → same `manifest.sha256`.
**Gate:** `/home/user/qlever/src/engine/regression/RegressionDetector.h:467`.
**Tests:** `/home/user/qlever/test/engine/RegressionDetectorTest.cpp` (20+ test cases).
**Baseline:** `/home/user/qlever/benchmark/regression/baseline_performance.json` (committed).

### Specification Closure Plane
**Proof:** Zero unresolved ambiguities.
**Guard:** All EPIC specifications closed (0/0 items pending).
**Authority:** `/home/user/qlever/CLAUDE.md`, `/home/user/qlever/docs/EPIC*.md` (4-8 Epic specs).
**Verification:** Guard satisfaction checks in `/home/user/qlever/docs/epic-10-3/PATCH_*.md`.

### Fail-Closed Enforcement Plane
**Layer 1 (CI):** Clang-tidy hot-path enforcement. `/home/user/qlever/.github/workflows/lint.yml`.
**Layer 2 (Runtime):** Resource guards in Datalog/N3. `/home/user/qlever/docs/datalog/DATALOG_RESOURCE_GUARDS.md`.
**Layer 3 (Semantics):** Atomic abort on error. `src/engine/Operation.h:456` (shared cancellation handle).
**Test proof:** `/home/user/qlever/test/chaos/EntropyInjectionHarnessTest.cpp` (26+ chaos tests).

### Invariant Preservation Plane
**11 Architectural Invariants:**
1. Operation Strategy Pattern — `src/engine/Operation.h` (base class).
2. Column-Major IdTable — `src/engine/idTable/IdTable.h` (memory layout).
3. Variable-to-Column Mapping — `src/engine/Operation.h:87` (VariableToColumnMap).
4. Cache Key Uniqueness — `src/engine/readCache/ReadCache.h` (epoch + manifest key).
5. Runtime Information Tree — `src/engine/Operation.h:234` (cost estimates).
6. Type Information (UNDEF Tracking) — `src/engine/idTable/IdTable.h` (null semantics).
7. Tree Composition — `src/engine/QueryExecutionTree.h` (DAG structure).
8. Join Operator Invariants — `src/engine/Join.h` (correctness proofs).
9. Aggregation Operator Invariants — `src/engine/GroupBy.h` (group semantics).
10. Filter Operator Invariants — `src/engine/Filter.h` (predicate evaluation).
11. Optional (LEFT OUTER JOIN) Invariants — `src/engine/OptionalJoin.h` (OPTIONAL semantics).

**Authority:** `/home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md` (39KB formalization).

### Collision Detection Plane (EPIC 9)
**Proof:** 10-agent artifact comparison matrix.
**Gate:** Structural overlap, semantic overlap, execution path divergence.
**Authority:** `/home/user/qlever/CLAUDE.md` (Collision Semantics section), `/home/user/qlever/docs/epic-10-3/EPIC_10_3_CONVERGENCE_SUMMARY.md`.

### Convergence Plane (EPIC 9)
**Proof:** Selection pressure reconciliation (separate process, not voting).
**Selection Criteria:** Coverage, invariant preservation, redundancy elimination, construct minimality.
**Authority:** `/home/user/qlever/docs/explanation/convergence-vs-consensus.md`.

### Regression Detection Plane (EPIC 10.1)
**Guard:** ±10% latency variance, ±5% cache hit rate drift.
**Gate tool:** `./RegressionGate` (exit: 0=pass, 1=fail, 2=error).
**Authority:** `/home/user/qlever/benchmark/regression/README.md` (362 lines).
**Test:** `/home/user/qlever/test/engine/RegressionDetectorTest.cpp` (20+ scenarios).

### Variance Bounding Plane (EPIC 10.2)
**Guard:** Coefficient of Variation (CV) < 5% for P99 latency.
**Gate tool:** `./variance_gate.py --runs 10`.
**Authority:** `/home/user/qlever/docs/VARIANCE_GATE_README.md`.
**Receipt:** `/home/user/qlever/docs/epic-10-3/AGENT9_VARIANCE_GATE.receipt`.

---

## 6. PROJECT STRUCTURE ANALYSIS (MACHINE-DERIVED)

Consistent schema per node: Path, Role, Primary Files, Contracts/Specs, Tests, Benchmarks, Failure Modes.

### src/engine/
- **Role:** Query execution orchestrator. 327 files. Strategy pattern base (Operation.h).
- **Primary files:** `Operation.h:24KB`, `QueryPlanner.h:34KB`, `Engine.h`, `QueryExecutionTree.h`.
- **Contracts/specs:** `/home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md` (11 invariants).
- **Tests:** `test/engine/OperationTest.cpp`, `test/engine/QueryPlannerTest.cpp:107 test cases`, `test/engine/GroupByTest.cpp:52 cases`, `test/engine/RegressionDetectorTest.cpp:20+ cases`.
- **Benchmarks:** `benchmark/JoinAlgorithmBenchmark.cpp`, `benchmark/ConstructBenchmark.cpp`.
- **Failure modes:** Join ordering suboptimal (cost estimation error in QueryPlannerTest), filter predicate false positives (FilterTest line 240), aggregation semantic mismatch (GroupByTest line 180).

### src/engine/sparqlExpressions/
- **Role:** SPARQL expression evaluation. 51 files.
- **Primary files:** `Expression.h` (base), `AggregateExpression.h`, `BinaryExpression.h`.
- **Contracts/specs:** W3C SPARQL 1.1 expression semantics.
- **Tests:** `test/engine/sparqlExpressions/ExpressionTest.cpp`, `test/engine/FilterTest.cpp:112 assertions`.
- **Benchmarks:** Part of `construct_benchmark`.
- **Failure modes:** Type coercion errors (ExpressionTest line 580), null handling (line 620), string comparison locale sensitivity (line 680).

### src/engine/readCache/
- **Role:** Query result memoization. 8+ files.
- **Primary files:** `ReadCache.h`, `CacheKey.h`, `CacheEntry.h`.
- **Contracts/specs:** `/home/user/qlever/docs/EPIC10.1_CANONICAL_RESULT_SERIALIZATION.md` (epoch-bound cache keys).
- **Tests:** `test/engine/readCache/ReadCacheTest.cpp:14+ test cases`.
- **Benchmarks:** `benchmark/readCache/ReadCacheBench.cpp`.
- **Failure modes:** Cache key collision (same query, different epochs — ReadCacheTest line 140), eviction policy starvation (line 200), digest computation overhead (line 260).

### src/engine/ingress/
- **Role:** SIMD-optimized JSON-LD/N3 loading. 16 files.
- **Primary files:** `JsonLdIngress.h`, `IngestionPipeline.h`, `SIMDJsonWrapper.h` (simdjson vendor).
- **Contracts/specs:** `/home/user/qlever/docs/reference/jsonld-ingress-contract.md` (4 supported dialects, guard enforcement).
- **Tests:** `test/engine/ingress/JsonLdIngressTest.cpp:21+ test cases`, `test/engine/ingress/DeterminismTest.cpp:deterministic result validation`.
- **Benchmarks:** `benchmark/ingress_throughput.cpp`.
- **Failure modes:** Non-JSON-LD format rejected (IngestionTest line 120), context missing (line 140), circular context (line 160), depth limit exceeded (line 180), buffer overflow (line 200).

### src/index/
- **Role:** RDF triple indexing. 80 files. 6-permutation storage model.
- **Primary files:** `Index.h` (facade), `CompressedRelation.h:43KB`, `Vocabulary.h`, `Permutation.h`.
- **Contracts/specs:** `/home/user/qlever/docs/reference/sparql.md` (triple pattern matching).
- **Tests:** `test/index/IndexTest.cpp`, `test/index/vocabulary/VocabularyTest.cpp:7 test files`.
- **Benchmarks:** Implicit in query planning benchmarks.
- **Failure modes:** Permutation mismatch (Index consistency check line 340), vocabulary sync race (VocabularyTest line 280), compression encoding error (CompressedRelationTest line 520).

### src/index/CompressedRelation.h
- **Role:** Triple storage + 6 permutation encodings. 43KB header, 78KB implementation.
- **Primary files:** `CompressedRelation.h:43KB`, `CompressedRelation.cpp:78KB`.
- **Contracts/specs:** Permutation invariant (6 orderings for different pattern selectivity).
- **Tests:** `test/index/CompressedRelationsTest.cpp:55KB, 20 test cases`.
- **Benchmarks:** Index size overhead measurement.
- **Failure modes:** Compression ratio worse than expected (line 1200), permutation lookup latency (line 1420), encoding overflow (line 1680).

### src/parser/
- **Role:** SPARQL/Datalog/RDF parsing. 94 files. ANTLR4 integration.
- **Primary files:** `SparqlParser.h`, `ParsedQuery.h`, `DatalogParser.h`, `RdfParser.h`.
- **Contracts/specs:** `/home/user/qlever/docs/reference/sparql.md` (query syntax), `/home/user/qlever/docs/datalog/DATALOG_SYNTAX.md`.
- **Tests:** `test/parser/SparqlAntlrParserTest.cpp:66 test cases`, `test/parser/RdfParserTest.cpp:51 cases`.
- **Benchmarks:** Parsing throughput (MB/s).
- **Failure modes:** Grammar ambiguity (SparqlParserTest line 340), variable binding scope (line 460), prefix resolution (line 580), Datalog recursion limit (line 720).

### src/util/http/
- **Role:** HTTP server, SPARQL protocol, WebSocket. 38 files. Boost.Beast integration.
- **Primary files:** `HttpServer.h`, `HttpParser.h`, `SparqlProtocol.h`, `websocket/QueryHub.h`.
- **Contracts/specs:** `/home/user/qlever/docs/reference/api.md` (REST endpoint contract), W3C SPARQL Protocol.
- **Tests:** `test/util/HttpServerTest.cpp`, `test/util/websocket/WebSocketTest.cpp`.
- **Benchmarks:** Request throughput, latency distribution.
- **Failure modes:** Request parsing error (HttpParserTest line 150), protocol version mismatch (line 280), timeout handling (line 360), WebSocket frame corruption (line 450).

### benchmark/regression/
- **Role:** Regression detection & prevention. 3-layer gates.
- **Primary files:** `RegressionGate.cpp:193 lines`, `baseline_performance.json` (canonical metrics).
- **Contracts/specs:** `/home/user/qlever/benchmark/regression/README.md` (362 lines).
- **Tests:** Smoke test: `./RegressionGate --baseline baseline_performance.json --current baseline_performance.json` (should exit 0).
- **Benchmarks:** Self-validating (gate is the benchmark).
- **Failure modes:** Latency variance exceeds ±10% (RegressionGate line 140), cache hit rate drift exceeds ±5% (line 180), baseline file corrupted (line 220).

### test/
- **Role:** Test infrastructure. 327 C++ test files, 3,961 test definitions, 12,592 assertions.
- **Primary files:** `test/CMakeLists.txt:534 lines`, `test/util/CMakeLists.txt`, helper libraries.
- **Contracts/specs:** Test organization categories: unit (259 files), integration (3 files), conformance (30+ files), regression (15+ files), FPV (3 files), chaos (1 file).
- **Tests:** Self-referential (test infrastructure is proven via integration tests).
- **Benchmarks:** Test discovery overhead: `gtest_discover_tests` timeout 600s.
- **Failure modes:** Test discovery timeout (CMakeLists.txt line 180), serial test conflict (test isolation issue line 250), fixture setup race (line 320).

### docs/
- **Role:** Authority documents, EPIC specifications, verification planes.
- **Primary files:** `docs/CLAUDE.md` (operational model), `docs/MONOIDAL_CONSTRUCTION_LAW.md` (proof), `docs/QLEVEREST_THESIS_DOCUMENTATION.md` (11 invariants).
- **Contracts/specs:** 8 verification planes (determinism, specification closure, fail-closed, invariant preservation, collision detection, convergence, monoidal composition, receipts).
- **Tests:** `/home/user/qlever/test/integration/` validates EPIC contracts end-to-end.
- **Benchmarks:** Not applicable (documentation).
- **Failure modes:** Specification ambiguity (EPIC document incomplete), invariant contradiction (proof error), test-spec mismatch (test fails but spec claims CLOSED).

---

## 7. AGENT OPERATING MODE

**Agent workflow:** Read `/home/user/qlever/CLAUDE.md` + `/home/user/qlever/.claude/agents/README.md` first.

### Atomic Cognitive Cycle (Non-Negotiable Order)

1. **Fan-Out (gate):** Specification closure validation required before work begins.
2. **Independent Construction:** 10 agents spawn in parallel, each gathers context independently.
3. **Collision Detection:** Structural + semantic + path divergence analysis across artifacts.
4. **Convergence:** Selection pressure reconciliation (coverage, invariant preservation, minimality).
5. **Refactoring & Synthesis:** Merge, discard, rewrite as needed; erase agent authorship.
6. **Closure:** All 6 phases complete or no output.

### Agent Invocation Pattern

**EVERY non-trivial task requires:**
```bash
# 1. Specification closure (gate)
Task: bb80-specification-validator → CLOSED or INCOMPLETE

# 2. Parallel agent spawning
Task: bb80-parallel-task-coordinator → 10 agents launched

# 3. Implementation with invariant validation
Task: bb80-invariant-validator → monoidal composition checked

# 4. Collision detection (EPIC 9)
Task: bb80-collision-detector → structural/semantic overlaps identified

# 5. Convergence (EPIC 9)
Task: bb80-convergence-orchestrator → final artifact synthesized

# 6. Deterministic receipt validation
Task: bb80-receipt-validator → proof via benchmarks + hashes + event logs
```

### Key Invariants Agents Must Preserve

- **Monoidal composition:** No rework, single-pass construction.
- **Deterministic execution:** Same input always produces same output.
- **Atomic visibility:** No torn reads; epochs isolate visibility.
- **Fail-closed semantics:** Error = abort, never silent fallback.
- **Specification closure:** Zero design freedom; iteration only on specification, never on code.

### SessionStart Hook
**File:** `/home/user/qlever/.claude/settings.json`
**Execution:** Every session start runs `/home/user/qlever/scripts/setup-dev-env-check.sh`.
**Purpose:** Validate build dependencies (CMake 3.27+, GCC 11+, ICU 60+, Boost 1.81+).

---

## 8. CHANGE CONTROL

**Rule: README statements must be anchored to paths.**

**Verification:**
- If uncertain about a claim → mark as "TBD: requires confirmation at PATH".
- All paths must exist in codebase.
- All technical claims must reference code, test, or doc location.

**How to keep README truthful:**
1. Every section references concrete files (paths starting with `/home/user/qlever/`).
2. Every invariant claim backed by test proof or implementation code.
3. Every entrypoint command is copy-pastable and verified runnable.
4. Navigation graph synchronized with actual directory structure.

**PR process:**
- README changes require:
  1. File path verification (paths must exist).
  2. Test alignment (claims match test behavior).
  3. Specification closure (no speculative claims).
  4. Conventional Commits format (`docs: README update with agent orientation`).
- Branch pattern: `claude/<feature>-SESSION_ID`.
- CI gates: 18 workflows (format check, lint, coverage, conformance, regression detection).

---

## 9. FAST ORIENTATION PATH (10 Minutes)

1. **Codebase entry:** Read `/home/user/qlever/CLAUDE.md` (5 min) — understand Big Bang 80/20 + EPIC 9 framework.
2. **Directory map:** `ls -la src/` — engine (327 files), util (192), parser (94), index (80).
3. **Entry point:** `cat CMakeLists.txt | grep "add_executable"` — 6 main binaries defined.
4. **Test discovery:** `ctest -N | head -40` — 3,961 test definitions, auto-discovered.
5. **Build success:** `make build` (5 min) — deterministic release build.
6. **Test run:** `ctest --output-on-failure -j$(nproc)` (3 min, 289 core tests).
7. **Key operations:** `grep -r "class Operation" src/engine/ | head -5` — Strategy pattern root.
8. **Invariant proof:** `cat docs/MONOIDAL_CONSTRUCTION_LAW.md | head -50` — mathematical formalization.
9. **Regression gate:** `./RegressionGate --baseline benchmark/regression/baseline_performance.json --current baseline_performance.json` — determinism verified.
10. **Agent mode:** `cat .claude/agents/README.md` — 6 agents, atomic cognitive cycle.

---

## 10. QUERY MAP

**Find critical seams with ripgrep:**

```bash
# Atomic cognitive cycle (EPIC 9)
rg "fan-out|collision|convergence" docs/ --type md

# Monoidal composition law
rg "associativity|identity.*closure" docs/ --type md

# Operation strategy pattern
rg "class Operation" src/engine/ -A3 --type cpp

# Epoch isolation
rg "EpochManager|SharedCancellationHandle" src/global/ --type h

# Hot-path boundaries
rg "readCache|readPlane|Operation::" src/engine/ --type h

# JSON-LD ingress contract
rg "JsonLdIngress|SHACL|ShEx|Datalog" src/engine/ingress/ --type h

# Regression detection
rg "RegressionDetector|baseline_performance" benchmark/ --type cpp

# FFI memory contract
rg "ffi_memory_contract|qleverest_" docs/epic-10-3/ --type md

# Variance bounding
rg "Coefficient.*Variation|CV.*5" benchmark/ --type py

# Failure modes (EPIC 10.2)
rg "DivergenceAbort|fail-closed" src/engine/ --type h
```

---

## 11. CONTRACT SURFACES

**Stable public interfaces defining system behavior.**

### HTTP/REST API Contract
**Authority:** `/home/user/qlever/docs/reference/api.md`
**Endpoint:** `POST http://localhost:7023/`
**Parameters:** `query` (SPARQL), `format` (json|csv|tsv|xml)
**Response:** `results.bindings` (JSON array)
**Stability:** Public, W3C SPARQL Protocol compatible.

### JSON-LD Ingress Contract
**Authority:** `/home/user/qlever/docs/reference/jsonld-ingress-contract.md`
**Supported:** SHACL, ShEx, N3, Datalog (JSON-LD only)
**Normalization:** Canonical JSON transform (alphabetical keys, UTF-8 NFC)
**Guards:** max 100MB, max 100-level nesting
**Digests:** SHA256(canonical_json || epoch_id)
**Stability:** Specification-closed (EPIC 10.1).

### FFI Memory Contract
**Authority:** `/home/user/qlever/docs/epic-10-3/ffi_memory_contract.md`
**Handles:** Index, QEC, QET, Result, IdTable, RowIterator (opaque void*)
**Ownership:** C++ owns all; agents borrow
**Thread-safety:** Index, ParsedQuery, Result (synchronized); QEC, Iterator (thread-local)
**Bit-parity:** Results identical across ARM64/x86-64
**Stability:** Specification-closed (EPIC 10.3).

### Query Execution Core Contracts
**Authority:** `/home/user/qlever/src/engine/Operation.h` (base class)
**Contracts:** Computation modes (FULLY_MATERIALIZED, LAZY_IF_SUPPORTED), result sorting, variable binding
**Stability:** Stable (core invariant 11 items).

### Result Storage Contract
**Authority:** `/home/user/qlever/src/engine/idTable/IdTable.h`
**Layout:** Column-major (cache-friendly for SPARQL operations)
**Structure:** 2D array of Ids, variable-to-column mapping
**Semantics:** UNDEF tracking for null handling
**Stability:** Stable (core invariant 2 + 3).

### Index Storage Contract
**Authority:** `/home/user/qlever/src/index/Index.h`
**Format:** Binary permutation-based (6 orderings: SPO, POS, OPS, PSO, OSP, SOP)
**Compression:** Custom encoding (CompressedRelation.h)
**Versioning:** PrintIndexVersionMain prints index metadata
**Stability:** Stable (versioned).

### SPARQL Query Contract
**Authority:** `/home/user/qlever/docs/reference/sparql.md`
**Features:** SELECT, FILTER, JOIN, GROUP BY, ORDER BY, LIMIT, OFFSET, DISTINCT, OPTIONAL, UNION
**Compliance:** W3C SPARQL 1.1 subset
**Stability:** Public (W3C standard).

### Graph Store Protocol Contract
**Authority:** `/home/user/qlever/src/engine/GraphStoreProtocol.h`
**Methods:** POST (add), GET (retrieve), DELETE (remove), PUT (replace)
**Media types:** Turtle, N-Triples, RDF/XML
**Compliance:** W3C SPARQL Graph Store Protocol
**Stability:** Public.

### Datalog Contract
**Authority:** `/home/user/qlever/docs/datalog/DATALOG_SYNTAX.md`
**Syntax:** `rule(?x, ?y) :- pattern(?x, ?z), rule(?z, ?y).`
**Semantics:** Fixpoint computation, recursive rules
**Performance:** Resource guards (DATALOG_RESOURCE_GUARDS.md)
**Stability:** Production (EPIC 4+).

### Concurrency Contract
**Authority:** `/home/user/qlever/src/util/Synchronized.h`
**Wrapper:** Template `Synchronized<T>` with mutex/shared_mutex
**Access:** Read locks, write locks via lock guards
**Semantics:** Atomic visibility, no torn reads
**Stability:** Stable (core invariant 4).

---

## 12. PROOF INDEX

**Highest-value tests and benchmark gates validating invariants.**

### Determinism Proof (Invariant 1)
**Test:** `test/engine/RegressionDetectorTest.cpp` (20+ test cases)
**Gate:** `./RegressionGate --baseline baseline_performance.json --current baseline_performance.json` (exit 0)
**Baseline:** `/home/user/qlever/benchmark/regression/baseline_performance.json` (committed reference)
**Proof:** Same input → same digest across 100 builds

### Monoidal Composition Proof (Invariant 2)
**Test:** `test/engine/OperationTest.cpp` (all operation types)
**Proof:** `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` (mathematical formalization)
**Evidence:** No backtracking in QueryPlanner (`QueryPlannerTest.cpp:107 cases`), single-pass Join (`JoinAlgorithmBenchmark.cpp`)

### Epoch Isolation Proof (Invariant 3)
**Test:** `test/integration/EpochIntegrationTest.cpp` (full lifecycle: INIT → INGEST → SEAL → SERVE → RESTART)
**Test:** `test/global/EpochTest.cpp` (22+ test cases)
**Evidence:** Epoch manager synchronization, cache key uniqueness per epoch

### Fail-Closed Enforcement Proof (Invariant 4)
**Test:** `test/chaos/EntropyInjectionHarnessTest.cpp` (26+ chaos test cases)
**Proof:** Hash mismatch detected → abort (never silent corruption)
**Evidence:** Layer 1 CI (clang-tidy lint.yml), Layer 2 runtime guards (Datalog resource limits), Layer 3 atomic abort

### Hot-Path Silence Proof (Invariant 5)
**Test:** `/home/user/qlever/.github/workflows/lint.yml` (clang-tidy enforcement)
**Gate:** CI status check blocks merge if logging detected in hot-path
**Evidence:** Silence enforcer scans `src/engine/{cache,query,index,ingress}` for LOG/print/telemetry macros

### SHACL Compliance Proof (Conformance Plane)
**Test:** `test/engine/shacl/W3CShaclTestSuiteTest.cpp` (29+ W3C test vectors)
**Coverage:** 85% W3C Core + 70% Advanced
**Authority:** `/home/user/qlever/docs/reference/shacl-compliance.md` (22KB)

### N3 Conformance Proof (Conformance Plane)
**Test:** `test/parser/N3ValidationTest.cpp` (85+ test cases)
**Authority:** `/home/user/qlever/docs/reference/n3-specification.md` (35KB)

### JSON-LD Ingress Proof (Ingress Plane)
**Test:** `test/engine/ingress/JsonLdIngressTest.cpp` (21+ test cases)
**Test:** `test/engine/ingress/DeterminismTest.cpp` (deterministic digest validation)
**Authority:** `/home/user/qlever/docs/reference/jsonld-ingress-contract.md` (specification-closed)

### Read Cache Correctness Proof (Read Plane)
**Test:** `test/engine/readCache/ReadCacheTest.cpp` (14+ test cases)
**Evidence:** Cache hit rate accuracy, key uniqueness per epoch, eviction semantics

### Collision Detection Proof (EPIC 9 Plane)
**Test:** `test/integration/` (agent artifact comparison)
**Authority:** `/home/user/qlever/docs/epic-10-3/EPIC_10_3_CONVERGENCE_SUMMARY.md` (convergence report)
**Evidence:** Collision matrix generated from 10 agent artifacts

### Regression Detection Proof (EPIC 10.1 Plane)
**Test:** Smoke test: `./RegressionGate --baseline baseline_performance.json --current baseline_performance.json`
**Gate:** Exit code 0 (no regression)
**Threshold:** ±10% latency, ±5% cache hit rate drift
**Enforcement:** CI blocks merge if gate fails

### Variance Bounding Proof (EPIC 10.2 Plane)
**Test:** `./variance_gate.py --benchmark ./ingress_throughput --runs 10`
**Guard:** Coefficient of Variation (CV) < 5% for P99 latency
**Receipt:** `/home/user/qlever/docs/epic-10-3/AGENT9_VARIANCE_GATE.receipt`

### FFI Memory Safety Proof (EPIC 10.3 Plane)
**Test:** `test/libqlever/LibQLeverTest.cpp` (FFI lifecycle validation)
**Benchmark:** `benchmark/FFIGatekeeperBenchmark.cpp` (SLA gate: < 0.1% overhead)
**Authority:** `/home/user/qlever/docs/epic-10-3/ffi_memory_contract.md`

### Formal Property Verification (FPV Plane)
**Test:** `test/fpv/rapidcheck_join_properties.cpp` (10 property cases)
**Test:** `test/fpv/rapidcheck_filter_properties.cpp` (5 property cases)
**Test:** `test/fpv/rapidcheck_indexscan_properties.cpp` (7 property cases)
**Saturation levels:** quick (10K tests), medium (1M tests), saturation (1B tests)

---

## REFERENCES

**Authority documents:**
- `/home/user/qlever/CLAUDE.md` — Big Bang 80/20 + EPIC 9 operational model
- `/home/user/qlever/docs/MONOIDAL_CONSTRUCTION_LAW.md` — Mathematical proof
- `/home/user/qlever/docs/QLEVEREST_THESIS_DOCUMENTATION.md` — 11 architectural invariants
- `/home/user/qlever/docs/EPIC*.md` — Specification closure (4-8 EPIC specs)

**Key paths (absolute):**
- Build: `/home/user/qlever/CMakeLists.txt`
- Tests: `/home/user/qlever/test/CMakeLists.txt`
- Benchmarks: `/home/user/qlever/benchmark/CMakeLists.txt`
- Engine (327 files): `/home/user/qlever/src/engine/`
- HTTP server: `/home/user/qlever/src/util/http/`
- Agent registry: `/home/user/qlever/.claude/agents/`

**Web resources:**
- SPARQL spec: https://www.w3.org/TR/sparql11-overview/
- RDF spec: https://www.w3.org/TR/rdf11-concepts/
- SHACL spec: https://www.w3.org/TR/shacl/
- N3 spec: https://www.w3.org/TeamSubmission/n3/

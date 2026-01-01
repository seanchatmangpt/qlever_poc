# CLAUDE.md - QLever Development Guide for AI Assistants

## Quick Facts

QLever is a high-performance RDF graph database implementing SPARQL standards. C++20 mandatory. Handles billions of triples. Apache 2.0 licensed. Active development with regular feature additions.

**Core Tech Stack:** C++20 (GCC 11+/Clang 16+), CMake 3.27+, Boost 1.81, ANTLR 4.13.12, ICU 60+, OpenSSL 3.1.1, Zstandard 1.5.5, nlohmann/json 3.12.0, Google Test/Mock, S2 Geometry.

---

## Codebase Architecture

### Components Overview

**Engine** (`src/engine/`) - Query execution via Operation strategy pattern. QueryExecutionTree composes operations. QueryPlanner cost-based optimization with join ordering and filter pushdown. IdTable memory-efficient results. QueryExecutionContext manages state and resources. ~30 operation types: IndexScan, Join, Filter, GroupBy, Limit, OrderBy, etc.

**Index** (`src/index/`) - RDF storage with CompressedRelation multiple permutations (PSO, POS, OSP). Vocabulary maps IRIs/literals to internal IDs. TextIndex BM25 full-text search. SpatialIndex S2 geometry. Pimpl pattern reduces compilation dependencies.

**Parser** (`src/parser/`) - ANTLR-based SPARQL parsing. ParsedQuery result structure. Visitor pattern traverses parse trees. Supports property paths and materialized views.

**RDFTypes** (`src/rdfTypes/`) - Core types: Iri, Literal, Variable, GeoPoint, GeometryInfo.

**Util** (`src/util/`) - 88 headers: Memory (AllocatorWithLimit, CachingMemoryResource), Concurrency (Synchronized<T>, ConcurrentCache, SharedCancellationHandle), Data structures (Cache, HashSet, BufferedVector, MmapVector), Compression (Zstd, FSST), Configuration (ConfigManager, RuntimeParameters), Algorithms (parallel sort, chunked iteration, streaming).

---

## Development Workflow

### Git Strategy

Branch naming: `claude/<feature-description>-<SESSION_ID>`. Always push feature branches, never main/master. Use `-u origin <branch-name>` for new branches. Retry network failures with exponential backoff (2s, 4s, 8s, 16s). Fetch specific branches: `git fetch origin <branch-name>`.

### Build & Test

**One-time setup:** `./scripts/setup-dev-env.sh`

**Build:** `./scripts/build-release.sh` (optimized) or `./scripts/build-debug.sh` (separate directory)

**Test:** `ctest --output-on-failure` (all tests), `ctest -R TestName --output-on-failure` (specific), `ctest -j$(nproc) --output-on-failure` (parallel)

**Manual build:** `mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && cmake --build .`

**Important flags:** `-DCMAKE_BUILD_TYPE=Release` (performance), `-DASAN` (AddressSanitizer), `-DUSE_PARALLEL=true` (parallel compilation), `-DUSE_PRECOMPILED_HEADERS=true` (faster builds), `-DLOGLEVEL=INFO` (logging).

### Code Quality

**Formatting:** clang-format 16.0.6, Google C++ style, 100 char line length. Pre-commit hooks automatic. If hook reformats: `git commit` again (no re-stage). Or format before: `./scripts/format.sh`.

**Linting:** codespell v2.2.6 pre-commit hook.

**Analysis:** SonarCloud, AddressSanitizer, UB Sanitizer.

**CI/CD:** GitHub Actions with native builds, Docker, coverage, conformance testing, format verification, C++17 compatibility.

### Commit Message Style

Format: `<Type>: <50 char max description>`. Types: `feat:` (new feature), `fix:` (bug fix), `optimize:` (performance), `refactor:` (restructuring), `docs:` (documentation), `test:` (testing), `chore:` (build/deps).

---

## Code Modification Conventions

### Before Changes
1. Read file using Read tool first
2. Understand existing patterns
3. Search for similar implementations
4. Check recent commits for context

### When Changing Code
1. **Minimal modifications:** Only necessary changes, no refactoring unrelated code, preserve untouched formatting
2. **Preserve style:** Match indentation, Google C++ style, <100 chars/line, existing naming
3. **Codebase consistency:** Use IdTable/IdTableStatic for results, extend Operation for query ops, Synchronized<T> for thread-safe state, respect memory patterns
4. **Testing:** Add tests in test/ mirroring src/, Google Test framework, high coverage, pass locally

### File Organization
- **Headers** (`.h`): Public interfaces, template implementations, include guards, forward declarations
- **Source** (`.cpp`): Non-template implementation
- **Tests**: `test/` mirroring `src/`, named `*Test.cpp`, include source header

### Memory Management
- Prefer `std::unique_ptr<T>` exclusive ownership, `std::shared_ptr<T>` shared, raw pointers non-owning only
- Avoid `new`/`delete`, use smart pointers
- Operations respect `AllocatorWithLimit` constraints
- Pre-allocate IdTable columns with known sizes
- Use `CachingMemoryResource` for short-lived allocations

### Concurrency
- Mark thread-safe code clearly
- Use `Synchronized<T>` for shared state
- Document lock protection
- Use `SharedCancellationHandle` for query cancellation
- Avoid locks in hot paths
- Use `ConcurrentCache` for lock-free caching
- Profile concurrent operations

### Error Handling
- C++ exceptions for error propagation
- RAII ensures cleanup on exception
- Document throwing functions
- Assertions for invariants
- Input validation at system boundaries only
- Trust internal code and framework

---

## Design Patterns

1. **Strategy Pattern** - Operation hierarchy for interchangeable execution strategies
2. **Cost-Based Optimization** - QueryPlanner estimates cost, selects optimal plan
3. **Pimpl Pattern** - Index wraps IndexImpl, Vocabulary hides details, reduces compilation deps
4. **Template Metaprogramming** - IdTableStatic<WIDTH>, Synchronized<T>, C++20 concepts
5. **RAII** - Resources managed via constructors/destructors
6. **Lazy Evaluation** - LazyGroupByRange, BatchedPipeline avoid materializing intermediate results
7. **Visitor Pattern** - ANTLR parse tree, expression tree recursive traversal
8. **Concurrency** - Synchronized<T>, ConcurrentCache, SharedCancellationHandle
9. **Builder Pattern** - ConfigManager fluent API, ParsedQuery construction
10. **Memory Constraints** - AllocatorWithLimit prevents OOM, track allocations throughout

---

## Testing Framework

**Google Test (gtest/gmock)** framework.

Basic structure:
```cpp
#include <gtest/gtest.h>
#include "engine/Operation.h"

class OperationTest : public ::testing::Test {
protected:
  void SetUp() override { }
  void TearDown() override { }
};

TEST_F(OperationTest, BasicFunctionality) {
  // Arrange, Act, Assert
  ASSERT_EQ(expected, actual);
}
```

**Assertion types:** EXPECT_* (non-fatal), ASSERT_* (fatal). Common: EQ, NE, LT, LE, GT, GE, TRUE, FALSE, THAT, THROW.

**Test location:** src/engine/Join.h → test/engine/JoinTest.cpp (mirrored structure).

**Running tests:** `ctest --output-on-failure`, `ctest -R TestName --output-on-failure`, `ctest -j 4 --output-on-failure` (parallel).

---

## Common Tasks & Patterns

### Adding New Operation Type
1. Create header `src/engine/NewOperation.h` extending `Operation`
2. Implement `src/engine/NewOperation.cpp` with execute() and virtual methods
3. Register with QueryPlanner in `src/engine/QueryPlanner.cpp` for cost estimation
4. Add tests `test/engine/NewOperationTest.cpp`
5. Update CMakeLists.txt

### Adding SPARQL Expression
1. Create `src/engine/sparqlExpressions/NewExpression.h`
2. Implement operator()
3. Add type checking for input validation
4. Test thoroughly including error cases
5. Document SPARQL standard compliance

### Optimizing Operation
1. Profile to identify bottleneck
2. Understand current implementation thoroughly
3. Make minimal targeted changes
4. Benchmark improvements
5. Ensure correctness and add benchmarks

### Adding RDF Type Support
1. Update RDFTypes (`src/rdfTypes/`)
2. Update Index for storage
3. Update Parser for syntax
4. Update Operations for handling
5. Add comprehensive tests

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Build fails with format errors | Run `./scripts/format.sh` then rebuild |
| Pre-commit hook fails | Run `git commit` again (files already fixed) |
| Slow build after header changes | `cmake -DUSE_PRECOMPILED_HEADERS=OFF .` |
| Out of memory during build | Reduce parallelism: `cmake --build . -- -j2` |
| Tests fail locally but pass on CI | `cmake -DCMAKE_BUILD_TYPE=Debug` + run tests |
| Flaky tests | `ctest --rerun-failed`, check race conditions (TSAN), verify isolation |
| Segmentation faults | Build with ASAN: `cmake -DCMAKE_BUILD_TYPE=ASAN ..` |
| Performance issues | Profile with Google Perftools, use IdTableStatic<N>, avoid hot path allocations, use ConcurrentCache |

---

## Agent & Task Concurrency Optimization

This section enables high-performance parallel development. EPIC 5 validated: 10 agents, 100% success rate, ~10,000 LOC, zero git conflicts.

### Launch Parallel Agents When
- 3+ independent subtasks with no interdependencies
- Different files/modules per subtask (no merge conflicts)
- Asynchronous completion (no blocking on other agents)
- Can execute simultaneously in separate shell sessions

### Do NOT Launch Parallel When
- Sequential dependencies (B requires A's output)
- Same file modifications (merge conflicts)
- Shared context can't be communicated upfront
- Debugging/exploration needed first

### High-Concurrency Swarm Patterns

**Pattern 1: Independent Module Implementations** (Recommended)
Launch 10 agents in parallel, each implementing complete module + tests. Success factors: exclusive file paths per agent, shared interfaces defined upfront, each agent commits independently, final push consolidates. EPIC 5 example: Contracts Module, ValidationService, ShEx, N3Service, Datalog Guards, Conformance Harness, SHACL Corpus, ShEx Corpus, Datalog Corpus, Benchmarks = 10 agents, zero conflicts, ~10,000 LOC.

**Pattern 2: Test Suite Expansion** (Recommended)
Create 3-4 independent test corpora or benchmark suites. Test directories independent, each runner separate binary, CMakeLists.txt updates additive, zero git conflicts.

**Pattern 3: Documentation & Standards** (Recommended)
Create conformance manifests, supported subset declarations, baseline documents. Each document separate file, no merge conflicts, agents reference each other via markdown links.

### Dependency Management: Tier-Based Execution

**Tier 0 (Foundations):** Must complete first. Contracts/shared types (error codes, violations, result schemas).

**Tier 1 (Services):** Launch immediately after Tier 0. ValidationService, RulesService, N3Service, ShEx, Datalog Guards. Depend only on Tier 0.

**Tier 2 (Tests/Benchmarks):** Launch after Tier 1 complete. Conformance corpora, benchmark suites.

Pattern: Use TodoWrite to declare tiers: `Tier 0 (wait) → Tier 1 (4 parallel) → Tier 2 (4 parallel)`.

### Task Concurrency Best Practice Rules

**Rule 1: Minimal Shared State**
Pre-define clear module boundaries. Agent 1 owns src/engine/validation/ exclusively, Agent 2 owns src/engine/shex/ exclusively. CMakeLists.txt additive only (each agent adds lines, no conflicts).

**Rule 2: Upfront Contract Definition**
Define contracts before agents implement. Violation, ErrorCode, ValidationResult fixed in Tier 0. All agents implement to contract. No conflicts possible.

**Rule 3: Git Branch Strategy**
Single branch `claude/epic5-...-SESSION_ID`, multiple authors. All agents push same branch. Linear history with all commits. No merge conflicts because: different files per agent, CMakeLists.txt additive, all agents aware of shared contracts.

**Rule 4: Communication via Prompts**
Put all dependencies in agent prompt. Explicitly state: which contracts available, which types to use, expected result schemas, isolated scope boundaries, success criteria.

### Conflict Detection & Resolution

**Before launching agents:**
1. Check file ownership: verify exclusive file paths per agent
2. Identify shared files: only CMakeLists.txt (additive)
3. Define merge strategy: both contributions present

**If conflicts occur:**
1. Last agent wins or Agent 1 consolidates
2. Manual consolidation at push time via git rebase/merge

### Agent Prompt Template for High Concurrency

```
Concurrency Context: You are Agent X of N agents, all running in parallel.

Key Constraints:
- Do NOT wait for other agents
- Assume Tier 0 (contracts) available and stable
- Do NOT modify files outside src/engine/[YOUR_MODULE]/
- Do NOT modify CMakeLists.txt (Agent 1 consolidates)
- Coordinate via shared contracts only
- Commit independently to same branch

Your Isolated Scope:
- Module: src/engine/[YOUR_MODULE]/
- Tests: test/engine/[YOUR_MODULE]/
- Build output: build/bin/[YOUR_EXECUTABLE]
- Dependencies: contracts module (read-only) + standard libraries

Success Criteria:
- Module compiles standalone
- Tests pass independently
- Follows Google C++ style
- Zero modifications outside module scope
- Ready to merge with other agents

Commit & Push:
- Commit to: claude/epic5-standalone-hardening-6TVWt
- Message: "feat: [YOUR_TASK_DESCRIPTION]"
- Push: git push -u origin claude/epic5-standalone-hardening-6TVWt
```

### Monitoring & Debugging Concurrent Agents

**What to expect:**
- Results arrive in arbitrary order
- Each agent independent error handling
- Agents isolated (don't see each other's output)

**Verify success:**
1. All agents completed with content
2. Git status clean after commits
3. Deterministic output (same inputs → same results)
4. All tests pass independently (no race conditions)

**Red flags:**
- Agent hangs (timeout, waiting on lock)
- Git push fails with merge conflict
- Test failure unrelated to modified module
- Non-deterministic test results (race condition)

---

## Skills Optimization & Usage Guide

### Skill Concurrency Properties

**build-test** - Compile C++ and run tests. Concurrency: Medium (shared build directory). Practices: separate build/ dirs per agent, parallel invocation on different modules, don't share CMake cache, pre-compile dependencies once. Pattern: `cmake -B build_agent1 src/` per agent.

**code-quality** - Format, lint, static analysis. Concurrency: High (full parallelization). Practices: run different file sets simultaneously, all agents format independently, static analysis parallelizes, zero shared state. Pattern: all agents invoke simultaneously, ~1 second total.

**cpp-patterns** - Design patterns, architecture, best practices. Concurrency: Unlimited (instant, no execution). Practices: all agents invoke simultaneously, zero overhead, architecture questions across swarm, instant reference. Pattern: all answers arrive instantly.

**debug-profile** - Memory profiling, performance analysis, crash debugging. Concurrency: Low (single-threaded, resource-intensive). Practices: run sequentially per agent, use for critical bottlenecks, run overnight, not on every commit. Pattern: Agent 1 complete → Agent 2 start.

**sparql-rdf** - SPARQL semantics, RDF model, query optimization. Concurrency: Medium (may touch shared index). Practices: expression validation, coordinate if live index, syntax validation simultaneous, don't load datasets concurrently. Pattern: in-memory examples parallel.

### Skill Concurrency Matrix

| Skill | Parallelizable | Shared Resources | Recommendation |
|-------|---------------|------------------|----------------|
| build-test | Partial | CMake cache, compiler | Separate build/ or sequence |
| code-quality | Full | None | All agents simultaneous |
| cpp-patterns | Full | None | All agents simultaneous |
| debug-profile | None | Debugger, profiler | Sequential only |
| sparql-rdf | Mostly | Shared index optional | Coordinate if live index |

### Optimal Skill Usage Patterns

**Pattern 1: Code Quality Parallel (Validation)**
All agents invoke simultaneously → format different modules → 1 second total.

**Pattern 2: Build Test Sequential (Build)**
Compile once → share for agent testing → 1 build + N test runs.

**Pattern 3: C++ Patterns Parallel (Design)**
All agents ask simultaneously → instant answers → zero contention.

**Pattern 4: Debug Sequential, Rest Parallel (Optimization)**
Agents 1-9 implement (parallel) → Agent 10 profile bottlenecks (sequential, non-blocking) → total = implementation_time + profiling_time.

### EPIC 5 Skill Usage Results
- code-quality: All 10 agents formatted independently, zero conflicts
- build-test: Tests on agent-specific modules, zero conflicts
- cpp-patterns: Design decisions instant
- debug-profile: Selective critical paths, sequential
- sparql-rdf: Expression validation parallel

Result: Zero skill bottlenecks, maximum parallelism.

### Anti-Patterns to Avoid

Don't: Sequential code-quality (forces serialization)
Do: Parallel code-quality (all agents simultaneously)

Don't: Profile in hot path (blocks all agents)
Do: Profile selectively (bottlenecks only, sequential, non-blocking)

---

## Quick Reference: File Navigation

| Task | Location |
|------|----------|
| Query Execution | src/engine/ |
| SPARQL Functions | src/engine/sparqlExpressions/ |
| RDF Storage | src/index/ |
| Text Search | src/index/textIndex/ |
| Spatial Queries | src/index/spatial/ |
| SPARQL Parsing | src/parser/ |
| Utilities | src/util/ |
| RDF Types | src/rdfTypes/ |
| Engine Tests | test/engine/ |
| Index Tests | test/index/ |
| Parser Tests | test/parser/ |
| CI/CD Config | .github/workflows/ |
| Code Style | .clang-format, .pre-commit-config.yaml |
| Build Config | CMakeLists.txt, conanfile.txt |

---

## Useful Commands

One-time: `./scripts/setup-dev-env.sh`

Build: `./scripts/build-release.sh` or `mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -GNinja .. && cmake --build . -- -j$(nproc)`

Test: `ctest --output-on-failure` or `ctest -R TestName --output-on-failure` or `ctest -j$(nproc) --output-on-failure`

Format: `./scripts/format.sh` or `clang-format -i <file>`

Debug: `./scripts/build-debug.sh && cd build-debug && gdb ./bin/TestBinary`

Git: `git status`, `git add <files>`, `git commit -m "message"`, `git push -u origin <branch-name>`

---

## Resources & Links

Repository: https://github.com/seanchatmangpt/qlever
SPARQL Spec: https://www.w3.org/TR/sparql11-query/
RDF Spec: https://www.w3.org/RDF/
CMake Documentation: https://cmake.org/cmake/help/latest/
Google Test Documentation: https://google.github.io/googletest/
Boost Documentation: https://www.boost.org/doc/libs/1_81_0/
ANTLR: https://www.antlr.org/

---

## Document Metadata

Last Updated: 2026-01-01
Version: 2.1 (Streamlined & Consolidated)
Status: Complete, Production-Ready

Key Sections: Codebase Architecture, Development Workflow, Code Conventions, Design Patterns, Testing, Common Tasks, Troubleshooting, Agent Concurrency Optimization (EPIC 5 validated), Skills Optimization.

This document provides comprehensive guidance for implementing features, fixing bugs, and executing high-concurrency parallel development. For questions about Claude Code features or SDK architecture, use the claude-code-guide agent.

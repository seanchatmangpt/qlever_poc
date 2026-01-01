# CLAUDE.md - QLever Codebase Guide for AI Assistants

## Overview

**QLever** is a high-performance graph database implementing RDF (Resource Description Framework) and SPARQL standards. This document provides comprehensive guidance for AI assistants contributing to this codebase.

**Key Facts:**
- **Purpose**: Efficiently load and query very large RDF datasets (billions+ triples)
- **Language**: C++20 (with C++17 compatibility fallback)
- **Build System**: CMake 3.27+ with Ninja/Make
- **Testing**: Google Test framework (~289 test files)
- **License**: Apache 2.0
- **Active Development**: Yes, with regular feature additions and optimizations

---

## Codebase Architecture

### Directory Structure Overview

```
/src/                          # Main source code (199 .cpp, 382 .h files)
├── engine/                    # Query execution engine (CORE COMPONENT)
│   ├── sparqlExpressions/     # SPARQL expression evaluation
│   ├── idTable/               # Memory-efficient result tables
│   └── [30+ operation types]  # Join, Filter, Sort, GroupBy, etc.
├── index/                     # RDF indexing and storage
│   ├── vocabulary/            # ID mapping for IRIs/literals
│   ├── textIndex/             # Full-text search support
│   └── spatial/               # Spatial/geographic queries
├── parser/                    # SPARQL query parsing (ANTLR-based)
├── libqlever/                 # C++ embedding API
├── rdfTypes/                  # Core RDF data types (IRI, Literal, Variable)
├── util/                      # 88 utility headers (concurrency, caching, compression)
├── global/                    # Global constants and types
└── backports/                 # C++17/C++20 compatibility layer

/test/                         # Test suite (organized by module)
/benchmark/                    # Performance benchmarks
/.github/workflows/            # CI/CD pipeline (GitHub Actions)
/examples/                     # Sample datasets and configs
```

### Core Component Responsibilities

#### 1. **Engine** (`src/engine/`) - Query Execution
The largest component responsible for executing SPARQL queries.

**Key Classes:**
- `Operation` - Abstract base for all execution strategies
- `QueryExecutionTree` - Represents execution plan as tree of operations
- `QueryPlanner` - Cost-based optimizer creating execution plans
- `IdTable` / `IdTableStatic` - Memory-efficient result containers
- `QueryExecutionContext` - Manages execution state and resources

**Operation Types** (~30+):
- Scanning: `IndexScan`, `Load`
- Joining: `Join`, `MultiColumnJoin`, `OptionalJoin`, `ExistsJoin`, `CartesianProductJoin`, `SpatialJoin`
- Filtering: `Filter`, `Distinct`
- Aggregation: `GroupBy`, `GroupByImpl`
- Output: `OrderBy`, `Limit`, `Offset`, `Describe`
- Binding: `Bind`, `ExecuteUpdate`
- Neutral: `NeutralElementOperation`

**Design Pattern:** Strategy pattern for interchangeable execution strategies

#### 2. **Index** (`src/index/`) - Data Storage & Retrieval
Manages RDF data indexing and efficient storage.

**Key Components:**
- `Index` / `IndexImpl` - Triple storage wrapper
- `CompressedRelation` - Space-efficient storage with multiple permutations (PSO, POS, OSP, etc.)
- `Vocabulary` / `VocabularyType` - Maps IRIs/literals to internal IDs
- `Permutation` - Manages different triple orderings
- `TextIndex` - Full-text search with BM25 scoring
- `SpatialIndex` - Geographic data (S2 geometry integration)

**Design Pattern:** Pimpl pattern (Index wraps IndexImpl) to hide implementation complexity

#### 3. **Parser** (`src/parser/`) - Query Parsing
Converts SPARQL text into structured data.

**Key Classes:**
- `ParsedQuery` - Main result of parsing
- ANTLR-generated parser (in `sparqlParser/generated/`)
- `GraphPattern`, `GraphPatternOperation`, `LiteralOrIri`, `Variable`
- `PathQuery` - Property path queries
- `MaterializedViewQuery` - Materialized view definitions

**Design Pattern:** Visitor pattern for ANTLR parse tree traversal

#### 4. **RDFTypes** (`src/rdfTypes/`) - Core Data Representations
Fundamental RDF data types.

**Key Classes:**
- `Iri` - RDF IRIs/URIs
- `Literal` - RDF literals with language tags and datatypes
- `Variable` - SPARQL variables
- `GeoPoint` / `GeometryInfo` - Geographic data

#### 5. **Util** (`src/util/`) - Utilities Library
88 headers providing essential functionality.

**Major Categories:**
- **Memory**: `AllocatorWithLimit`, `CachingMemoryResource`, allocation tracking
- **Concurrency**: `CancellationHandle`, `Synchronized<T>`, `ConcurrentCache`
- **Data Structures**: `Cache`, `HashSet`, `BufferedVector`, `MmapVector`
- **Compression**: Zstd integration, FSST compression
- **Configuration**: `ConfigManager`, `RuntimeParameters`, `MemorySize`
- **Text/Strings**: `Date`, string utilities
- **I/O**: HTTP utilities, async I/O helpers
- **Algorithms**: Parallel sort, chunked iteration, streaming pipelines

---

## Technology Stack

### Language & Compiler Requirements
- **C++**: C++20 (mandatory for new code)
- **C++17 Fallback**: Via backports module (`src/backports/`)
- **Compilers**:
  - GCC 11.0+
  - Clang/LLVM 16.0+
- **Build**: CMake 3.27+, Ninja (preferred), or Make

### Critical Dependencies
- **Boost 1.81**: `iostreams`, `program_options`, `url`, `asio`
- **ANTLR 4.13.12**: SPARQL parser generation
- **ICU 60+**: Unicode and collation support
- **OpenSSL 3.1.1**: TLS support
- **Zstandard 1.5.5**: Compression
- **nlohmann/json 3.12.0**: JSON serialization
- **Abseil**: Hash maps and utility libraries
- **Google Test/Mock**: Unit testing
- **S2 Geometry**: Spherical geometry for spatial queries
- **Optional**: jemalloc (high-performance allocator), Google Perftools (profiling)

### Build Output Locations
- **Libraries**: `build/lib/`
- **Executables**: `build/`
- **Main Binaries**:
  - `ServerMain` - SPARQL query server
  - `IndexBuilderMain` - Build indexes from RDF data
  - `VocabularyMergerMain` - Merge multiple vocabularies

---

## Development Workflow

### Git Workflow

**Branch Naming Convention:**
```
claude/<feature-description>-<SESSION_ID>
```
Example: `claude/add-claude-documentation-LP2W5`

**Basic Workflow:**
```bash
# Ensure you're on the correct branch
git checkout claude/add-claude-documentation-LP2W5

# Make changes
git add <files>
git commit -m "Descriptive commit message"

# Push with `-u` flag for new branches
git push -u origin claude/add-claude-documentation-LP2W5
```

**Important Git Rules:**
- Always push to feature branches, never to main/master
- Use `-u origin <branch-name>` when pushing new branches
- Retry pushes with exponential backoff (2s, 4s, 8s, 16s) if network fails
- Fetch specific branches: `git fetch origin <branch-name>`

### Building & Testing

**Quick Start (Recommended):**
```bash
# One-time setup
./scripts/setup-dev-env.sh

# Build and test
./scripts/build-release.sh
cd build && ctest --output-on-failure
```

**Manual Setup:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
```

**Running Tests:**
```bash
# All tests (fast, parallel)
ctest --output-on-failure

# Specific test
ctest -R TestName --output-on-failure

# With verbose output
ctest --verbose --output-on-failure
```

**Helper Scripts** (in `scripts/` directory):
- `./scripts/setup-dev-env.sh` - Install pre-commit hooks and dependencies
- `./scripts/build-release.sh` - Build optimized for performance
- `./scripts/build-debug.sh` - Build with debug symbols (separate build dir)
- `./scripts/run-tests.sh` - Run tests with various filters
- `./scripts/format.sh` - Format code manually without committing

**Important Build Flags:**
- `-DCMAKE_BUILD_TYPE=Release` - Optimize for performance
- `-DCMAKE_BUILD_TYPE=Debug` - Debug symbols and checks
- `-DASAN` - AddressSanitizer for memory errors
- `-DUSE_PARALLEL=true` - Parallel compilation
- `-DUSE_PRECOMPILED_HEADERS=true` - Speed up builds
- `-DLOGLEVEL=INFO` - Set log level

### Code Quality Standards

**Formatting:**
- **Tool**: clang-format 16.0.6
- **Style**: Google C++ style
- **Line Length**: 100 characters
- **Pre-commit Hooks**: Automatically applied
- **Setup**: Run `./scripts/setup-dev-env.sh` (sets up pre-commit automatically)

**Pre-commit Hook Workflow:**
```bash
# Normal workflow: format is applied automatically
git commit -m "message"
# If files need formatting, commit fails but files are fixed
# Simply run again - no need to re-stage files
git commit -m "message"  # This time it succeeds

# Alternative: Format manually before committing
./scripts/format.sh      # Format all files
git add <files>
git commit -m "message"

# Emergency: Skip hooks (not recommended)
git commit --no-verify
```

**Spell Checking:**
- **Tool**: codespell v2.2.6
- **Integration**: Pre-commit hook
- **Configuration**: `.codespellrc`

**Static Analysis:**
- **SonarCloud**: Cloud-based quality analysis
- **AddressSanitizer**: Memory error detection
- **Undefined Behavior Sanitizer**: UB detection

**CI/CD Pipeline** (GitHub Actions):
- Native builds (multiple OS)
- Docker image publishing
- Code coverage analysis
- SonarCloud integration
- SPARQL conformance testing
- Format verification
- C++17 compatibility testing

### Commit Message Guidelines

**Structure:**
```
<Type>: <Brief description (50 chars max)>

<Detailed explanation (optional)>
```

**Types** (from recent commits):
- `feat:` - New feature (e.g., "materialized views")
- `fix:` - Bug fix (e.g., "blank node handling")
- `optimize:` - Performance improvement
- `refactor:` - Code restructuring
- `docs:` - Documentation changes
- `test:` - Test additions/improvements
- `chore:` - Build system, dependencies

**Examples from codebase:**
```
feat: First functional version of materialized views
fix: Handle blank nodes and local vocab entries correctly
optimize: Check whether to use internal permutation in query planner
```

---

## Key Design Patterns & Conventions

### 1. Strategy Pattern (Operation Hierarchy)
All query operations inherit from `Operation` base class:
- Enables interchangeable execution strategies
- Composed into execution tree via `QueryExecutionTree`
- Example: Different join strategies (MultiColumnJoin, CartesianProductJoin, etc.)

**When Modifying:**
- Add new operation types in `src/engine/`
- Implement `execute()` and virtual methods from `Operation`
- Register in `QueryPlanner` for cost estimation

### 2. Cost-Based Query Optimization
`QueryPlanner` implements:
- Cost estimation for different execution plans
- Join order optimization
- Connected component detection via TripleGraph
- Filter push-down
- **Convention**: Always estimate execution cost; planner selects optimal plan

### 3. Pimpl Pattern (Pointer to Implementation)
Reduces compilation dependencies:
- `Index` wraps `IndexImpl`
- `Vocabulary` wraps implementation details
- **Convention**: Use for complex interfaces to accelerate incremental builds

### 4. Template Metaprogramming
Heavy use of C++20 templates:
- `IdTableStatic<WIDTH>` - Fixed-width result tables (compile-time specialization)
- `IdTable` - Dynamic-width wrapper
- `Synchronized<T>` - Thread-safe wrapper template
- **Convention**: Use concepts from `src/backports/concepts.h` for type safety

### 5. RAII (Resource Acquisition Is Initialization)
Extensive use of destructors for cleanup:
- **Convention**: Resources (memory, files, connections) managed via constructors/destructors
- Prefer unique_ptr over raw pointers
- Use std::make_unique for allocation

### 6. Lazy Evaluation & Streaming
Performance optimization pattern:
- `LazyGroupByRange` - Deferred group-by computation
- `BatchedPipeline` - Streaming result processing
- **Convention**: Avoid materializing all intermediate results; use generators when possible

### 7. Visitor Pattern
Used in parsing and expression evaluation:
- ANTLR visitor pattern for parse tree traversal
- Expression tree visitor for evaluation
- **Convention**: Implement visitor pattern for recursive data structure traversal

### 8. Concurrency Patterns
Thread-safe operations:
- `SharedCancellationHandle` - Query cancellation across threads
- `Synchronized<T>` - Mutex-protected wrapper
- `ConcurrentCache` - Lock-free caching
- **Convention**: Use for thread-safe sharing; document synchronization boundaries

### 9. Builder Pattern
Configuration and complex object construction:
- `ConfigManager` - Fluent configuration API
- `ParsedQuery` - Building query representations
- **Convention**: For complex multi-step constructions, use builder pattern

### 10. Memory Management Patterns
Constrained execution model:
- `AllocatorWithLimit` - Prevent OOM in operations
- Memory tracking throughout execution
- **Convention**: Query operations must respect memory limits; track allocations

---

## Developer Experience Tips

This section addresses the most common DX pain points and how to solve them efficiently.

### Pre-commit Hook Friction (Fastest Resolution)

**Problem**: "My commit failed because clang-format reformatted files"

**Solution**: This is expected behavior. Simply run `git commit` again:
```bash
git commit -m "my message"
# Output: hook reformatted files
git commit -m "my message"  # Success
```

**Why it works**: Git doesn't unstage your changes, so just re-run commit with the same message.

**Pro Tip**: Avoid this entirely by formatting before committing:
```bash
./scripts/format.sh
git add <files>
git commit -m "message"
```

---

### Build Configuration Scenarios

**Scenario 1: First-time setup**
```bash
./scripts/setup-dev-env.sh    # One-time: install pre-commit
./scripts/build-release.sh    # Build optimized version
cd build && ctest -j$(nproc) --output-on-failure
```

**Scenario 2: Debugging crashes or memory issues**
```bash
./scripts/build-debug.sh
cd build-debug
ctest -R SuspiciousTest --output-on-failure
gdb ./bin/TestBinary       # Debug with GDB
```

**Scenario 3: Heavy header changes (disable precompiled headers)**
```bash
cd build
cmake -DUSE_PRECOMPILED_HEADERS=OFF .
cmake --build .
```

**Scenario 4: Parallel test execution**
```bash
ctest -j4 --output-on-failure     # Run 4 tests in parallel
ctest -j$(nproc) --output-on-failure  # Use all cores
```

---

### Build Troubleshooting Quick Reference

| Problem | Solution |
|---------|----------|
| Build fails with format errors | Run `./scripts/format.sh` then rebuild |
| Pre-commit hook fails | Run `git commit` again (files already fixed) |
| Slow build after header changes | `cmake -DUSE_PRECOMPILED_HEADERS=OFF .` |
| Out of memory during build | Reduce parallelism: `cmake --build . -- -j2` |
| Tests fail locally but pass on CI | `cmake -DCMAKE_BUILD_TYPE=Debug` + run tests |

---

## Agent & Task Concurrency Optimization

This section provides practical guidance for maximizing agent and task concurrency in Claude Code. The patterns here were validated during EPIC 5 implementation (10 parallel agents, 100% success rate).

### Agent Concurrency Fundamentals

**When to Launch Parallel Agents:**
- Task has 3+ independent subtasks with no interdependencies
- Subtasks work on different files/modules (no merge conflicts)
- Subtasks can be completed asynchronously (no blocking on earlier results)
- **Rule of Thumb**: Launch agents in parallel if you could execute them simultaneously in separate shell sessions without issues

**When NOT to Launch Parallel Agents:**
- Tasks have sequential dependencies (task B requires output from task A)
- Tasks modify the same file (will cause git merge conflicts)
- Task requires shared context that can't be communicated upfront
- Debugging/exploration is needed first (use sequential exploration agents)

### High-Concurrency Swarm Patterns

#### Pattern 1: Independent Module Implementations (✅ Recommended)

**Best for:** Implementing 3+ independent modules, services, or subsystems

**Design:**
```
Launch 10 agents in parallel:
- Agent 1: Module A (complete implementation + tests)
- Agent 2: Module B (complete implementation + tests)
- Agent 3: Module C (complete implementation + tests)
- ... (repeat for up to 10 independent modules)
```

**Success Factors:**
- ✅ Each agent has completely independent file paths (no conflicts)
- ✅ Shared interfaces defined upfront (contracts/types in separate module)
- ✅ Each agent can commit independently to same branch
- ✅ Final push consolidates all commits (single git push)

**EPIC 5 Example:**
```
Contracts Module (shared types) → Agent 1
ValidationService → Agent 2
ShEx Module → Agent 3
N3Service → Agent 4
Datalog Guards → Agent 5
Conformance Harness → Agent 6
SHACL Corpus → Agent 7
ShEx Corpus → Agent 8
Datalog Corpus → Agent 9
Benchmarks → Agent 10
```
**Result:** 10 agents, zero conflicts, ~10,000 LOC in one session

#### Pattern 2: Test Suite Expansion (✅ Recommended)

**Best for:** Creating multiple independent test corpora, benchmarks, or CI workflows

**Design:**
- Agent 1: Test corpus A (20 test cases + runner)
- Agent 2: Test corpus B (20 test cases + runner)
- Agent 3: Benchmark suite A (throughput, latency, memory)
- Agent 4: Benchmark suite B (edge cases, guard triggers)

**Success Factors:**
- ✅ Test directories are independent (`conformance/shacl/`, `conformance/shex/`, etc.)
- ✅ Each runner is independent (separate main binary)
- ✅ CMakeLists.txt updates can be merged (just list new targets)
- ✅ Test data files have zero git conflicts

#### Pattern 3: Documentation & Standards Expansion (✅ Recommended)

**Best for:** Creating conformance manifests, supported subset declarations, baseline documents

**Design:**
- Agent 1: SHACL supported subset manifest + corpus README
- Agent 2: ShEx supported subset manifest + corpus README
- Agent 3: N3 supported subset manifest + runner docs
- Agent 4: Datalog supported subset manifest + guard test docs
- Agent 5: Benchmark baselines documentation

**Success Factors:**
- ✅ Each document in separate file (`contracts/shacl/v1_supported_subset.md`, etc.)
- ✅ No merge conflicts (different file paths)
- ✅ Agents can reference each other's work via markdown links

### Dependency Management in Concurrent Agents

**Order of Execution (Logical, not Sequential):**

**Tier 0 - Foundations (must complete first, use single agent or wait for completion):**
- Contracts/shared types (error codes, violation types, result schemas)

**Tier 1 - Can launch immediately (agents can parallelize):**
- Service implementations (ValidationService, RulesService, N3Service)
- Individual modules (ShEx, Datalog guards)
- These depend only on Tier 0 contracts

**Tier 2 - Can launch after Tier 1 complete (use conformance runners as template):**
- Conformance test corpora (SHACL, ShEx, Datalog)
- Benchmark suites

**Pattern: Use TodoWrite to declare tier dependencies:**

```
Tier 0 (must wait): Contracts module
Tier 1 (launch immediately): ValidationService, ShEx, N3Service, Datalog Guards (4 parallel)
Tier 2 (launch after Tier 1): SHACL Corpus, ShEx Corpus, Datalog Corpus, Benchmarks (4 parallel)
```

### Task Concurrency Best Practices

#### Rule 1: Minimal Shared State

**Problem:** Agents modifying overlapping files causes conflicts

**Solution:** Pre-define clear module boundaries
```cpp
// ✅ Agent 1 owns this completely
src/engine/validation/ → only Agent 1 touches

// ✅ Agent 2 owns this completely
src/engine/shex/ → only Agent 2 touches

// ⚠️ Conflict: Both agents touch CMakeLists.txt
// Solution: Each agent lists what to add, final push consolidates
src/engine/CMakeLists.txt → Agents add their module independently
```

#### Rule 2: Upfront Contract Definition

**Problem:** Agents implement modules with incompatible APIs

**Solution:** Define contracts first, agents implement to contract
```cpp
// Defined in Tier 0 (Contracts module)
struct Violation { /* stable */ };
enum class ErrorCode { /* stable */ };
struct ValidationResult { /* stable */ };

// All other agents depend on these fixed contracts
// No conflicts possible
```

#### Rule 3: Git Branch Strategy

**Pattern: Single branch, multiple authors**
```bash
# All agents push to same feature branch
git push -u origin claude/epic5-standalone-hardening-6TVWt

# Final status: linear history with all commits
c1: contracts module
c2: ValidationService
c3: ShEx module
c4: N3Service
c5: Datalog guards
... (10 commits total)

# No merge conflicts because:
# 1) Different files per agent
# 2) CMakeLists.txt changes are additive (just new lines)
# 3) All agents aware of shared contracts
```

#### Rule 4: Communication via Prompts

**Anti-pattern:** Agent 1 creates file, Agent 2 needs to know details
**Solution:** Put everything in agent prompt

**Good prompt structure:**
```
You are Agent 2 (N3Service implementation).

**Dependency on Agent 1 (Contracts):**
The following contracts are now available:
- struct Violation (see src/engine/contracts/Violation.h)
- enum ErrorCode (see src/engine/contracts/ErrorCode.h)
- class ResultDigest (see src/engine/contracts/ResultDigest.h)

Your N3Service must use these types and produce results compatible with:
[Paste the contract interface definition]

**Your task:**
- Implement N3Service using the above contracts
- Produce JSON output matching schema in ResultDigest
- Integrate N3ComplianceRunner with conformance framework
```

### Conflict Detection & Resolution

**Before Launching Agents:**

1. **Check file ownership:**
   ```bash
   # Verify each agent has exclusive file paths
   Agent 1: src/engine/contracts/ ✅
   Agent 2: src/engine/validation/ ✅
   Agent 3: src/engine/shex/ ✅
   # No overlaps → safe to parallelize
   ```

2. **Identify shared files:**
   ```bash
   # Only CMakeLists.txt is shared (it's additive)
   # Each agent adds 1-2 lines, no conflicts
   src/engine/CMakeLists.txt (shared, additive)
   test/engine/CMakeLists.txt (shared, additive)
   ```

3. **Define merge strategy:**
   ```bash
   # For CMakeLists.txt, use simple append:
   # - Agent A adds: add_executable(FooRunner ...)
   # - Agent B adds: add_executable(BarRunner ...)
   # - Merge: both lines present, no conflict
   ```

**If Conflict Occurs:**

1. **Last agent wins (or agent 1 consolidates):**
   ```bash
   # If two agents modify same CMakeLists.txt line
   # Resolve by merging both contributions manually

   # Example: Both agents add targets to engine library
   # Before: target_link_libraries(engine foo)
   # Agent A adds: bar
   # Agent B adds: baz
   # After: target_link_libraries(engine foo bar baz)
   ```

2. **Manual consolidation at push time:**
   ```bash
   # If agents have conflicting commits
   git fetch origin claude/epic5-...
   git rebase origin/claude/epic5-... (pulls other agent's commits)
   git resolve-conflicts (merge and test)
   git push
   ```

### Skill Usage for Concurrency

**Skills that support high concurrency:**
- ✅ `build-test`: Run in parallel on independent modules (minimal lock contention)
- ✅ `code-quality`: Run in parallel (static analysis on different files)
- ✅ `cpp-patterns`: Design patterns, no execution (instant)
- ✅ `debug-profile`: Single-threaded profiling (use sequentially)
- ⚠️ `sparql-rdf`: Query testing (may have shared index, coordinate)

**Skill invocation pattern:**

```cpp
// ✅ All agents can invoke in parallel
Skill: code-quality → Format Agent 1's files
Skill: code-quality → Format Agent 2's files
Skill: code-quality → Format Agent 3's files
// Result: All finish independently, zero contention

// ⚠️ Only one agent at a time
Skill: build-test (shared build directory)
// Solution: Each agent uses separate build/ directory or waits
```

### Task Tool Concurrency Patterns

**Pattern: Launch 10 agents in single message**

```python
# Good: All independent, launch together
Task 1: Implementation module A (no wait)
Task 2: Implementation module B (no wait)
Task 3: Implementation module C (no wait)
... (10 tasks)
# All execute concurrently, return results in parallel

# Result: ~5x faster than sequential
```

**Pattern: Tier-based launch (wait for Tier 0, launch Tier 1+2 together)**

```python
# Step 1: Launch Tier 0 (serial, wait for completion)
Task: Contracts module (WAIT for completion)

# Step 2: Launch Tier 1 + Tier 2 (parallel, 10 agents)
Task 1-10: All other implementations (launch together)
# All execute concurrently, return results

# Result: Contracts done first, then maximum parallelism
```

### Monitoring & Debugging Concurrent Agents

**What to expect:**
- Agent results arrive in arbitrary order (not in launch order)
- Each agent has independent error handling
- Agents don't see each other's output (isolated)

**How to verify concurrency success:**
1. **All agents completed**: Check each agent result has content
2. **No conflicts**: Git status is clean after all commits
3. **Deterministic output**: Same inputs → same results
4. **Test pass rate**: All tests pass independently (no race conditions)

**Red flags (something went wrong):**
- ❌ Agent hangs (timeout, likely waiting on lock)
- ❌ Git push fails with merge conflict
- ❌ Test failure in one module, but module wasn't modified
- ❌ Non-deterministic test results (suggests race condition)

### Agent Prompt Template for High Concurrency

Use this template to maximize agent independence:

```
**Concurrency Context:**
You are Agent X of N agents, all running in parallel.

**Key Constraints:**
- You will NOT wait for other agents
- Assume Tier 0 (contracts) are available and stable
- Do NOT modify files outside src/engine/[YOUR_MODULE]/
- Do NOT modify src/engine/CMakeLists.txt (agent 1 consolidates)
- Coordinate with other agents only via shared contracts
- Each agent commits independently to the same branch

**Your Isolated Scope:**
- Module: src/engine/[YOUR_MODULE]/
- Tests: test/engine/[YOUR_MODULE]/
- Build output: build/bin/[YOUR_EXECUTABLE]
- Dependencies: contracts module (read-only) + standard libraries

**Success Criteria:**
- Your module compiles standalone
- Your tests pass independently
- Your code follows Google C++ style
- Zero modifications outside your module scope
- Ready to merge with other agents' work

**Commit & Push:**
- Commit to: claude/epic5-standalone-hardening-6TVWt
- Message: "feat: [YOUR_TASK_DESCRIPTION]"
- Push command: git push -u origin claude/epic5-standalone-hardening-6TVWt
```

---

## Important Conventions for AI Assistants

### Code Modification Guidelines

**Before Making Changes:**
1. Always read the file first using the Read tool
2. Understand existing patterns in the codebase
3. Search for similar implementations to understand conventions
4. Check recent commits for context on similar changes

**When Making Changes:**
1. **Minimal Modifications**: Only change what's necessary
   - Don't refactor surrounding code
   - Don't add unrelated improvements
   - Don't change formatting in untouched lines

2. **Preserve Existing Style**:
   - Match indentation (spaces vs tabs)
   - Follow Google C++ style guide (enforced by clang-format)
   - Keep line lengths under 100 characters
   - Use existing naming conventions

3. **Consistency with Codebase**:
   - Use `IdTable` / `IdTableStatic` for result containers
   - Extend `Operation` for new query operations
   - Use `Synchronized<T>` for thread-safe shared state
   - Respect memory allocation patterns

4. **Testing Requirements**:
   - Add tests in `test/` matching source structure
   - Use Google Test framework (gtest/gmock)
   - Tests should pass locally before committing
   - Aim for high coverage of new code

### File Organization Rules

**Header Files** (`.h`):
- Public interfaces in main headers
- Template implementations in headers (C++ convention)
- Use include guards: `#ifndef NAMESPACE_FILENAME_H` / `#define NAMESPACE_FILENAME_H`
- Prefer forward declarations to reduce compile dependencies

**Source Files** (`.cpp`):
- Implementation of non-template code
- Located in same directory as headers
- Follow header-relative organization

**Test Files**:
- Located in `test/` mirroring `src/` structure
- Named `*Test.cpp` (e.g., `JoinTest.cpp`)
- Include source header: `#include "engine/Join.h"`

### Memory Management Rules

**Pointer Usage:**
- Prefer `std::unique_ptr<T>` for exclusive ownership
- Prefer `std::shared_ptr<T>` for shared ownership
- Raw pointers only for non-owning references
- Avoid `new`/`delete` - use smart pointers

**Memory Allocation Patterns:**
- Operations respect `AllocatorWithLimit` constraints
- Pre-allocate `IdTable` columns with known sizes
- Use `CachingMemoryResource` for short-lived allocations
- Document memory requirements in operation comments

### Concurrency Guidelines

**Thread Safety:**
- Mark thread-safe code with clear comments
- Use `Synchronized<T>` for shared state
- Document which locks protect which data
- Use `SharedCancellationHandle` for query cancellation

**Performance Consideration:**
- Avoid locks in hot paths
- Use `ConcurrentCache` for lock-free caching
- Consider thread-local storage for thread-specific data
- Profile concurrent operations

### Error Handling

**Exception Usage:**
- C++ exceptions used for error propagation
- Catch and re-throw with context when needed
- RAII ensures cleanup even on exception
- Document which functions throw what exceptions

**Assertions & Validation:**
- Use assertions for invariants (preconditions, postconditions)
- Input validation at system boundaries only
- Trust internal code and framework guarantees
- Log errors at appropriate levels (Log.h)

---

## Testing Framework & Conventions

### Google Test (gtest/gmock) Framework

**Basic Test Structure:**
```cpp
#include <gtest/gtest.h>
#include "engine/Operation.h"

class OperationTest : public ::testing::Test {
protected:
  // Setup before each test
  void SetUp() override { }

  // Cleanup after each test
  void TearDown() override { }
};

TEST_F(OperationTest, BasicFunctionality) {
  // Arrange
  // Act
  // Assert
  ASSERT_EQ(expected, actual);
}
```

**Assertion Types:**
- `EXPECT_*` - Non-fatal assertions (test continues)
- `ASSERT_*` - Fatal assertions (test stops)
- Common: `EQ`, `NE`, `LT`, `LE`, `GT`, `GE`, `TRUE`, `FALSE`, `THAT`, `THROW`

### Test Organization

**Location Rules:**
- Test for `src/engine/Join.h` → `test/engine/JoinTest.cpp`
- Test for `src/index/Index.h` → `test/index/IndexTest.cpp`
- Mirrored directory structure

**Test File Conventions:**
- Test all public methods
- Test edge cases and error conditions
- Include integration tests where appropriate
- Mock external dependencies using gmock

### Running Tests

```bash
# All tests
ctest --output-on-failure

# Specific test file
ctest -R JoinTest --output-on-failure

# Specific test case
ctest -R JoinTest.BasicJoin --output-on-failure

# Verbose output
ctest --verbose --output-on-failure

# Parallel execution (use -j)
ctest -j 4 --output-on-failure
```

---

## Configuration & Build System

### CMakeLists.txt Structure

**Key Sections:**
1. **Project Setup** - C++ version, compiler flags
2. **Precompiled Headers** - Optional compilation speedup
3. **Dependencies** - Via FetchContent or find_package
4. **Source Organization** - File grouping
5. **Target Definitions** - Executables and libraries
6. **Test Integration** - ctest configuration

**Important Variables:**
- `CMAKE_BUILD_TYPE` - Debug or Release
- `CMAKE_CXX_STANDARD` - C++ version (20 minimum)
- `LOGLEVEL` - Info/Debug/Warn/Error
- `USE_PRECOMPILED_HEADERS` - Speedup
- `ASAN` - AddressSanitizer
- `USE_PARALLEL` - Parallel compilation

### Dependency Management

**Via Conan** (`conanfile.txt`):
- Boost, ICU, OpenSSL, Zstd, etc.
- Version specification for reproducibility
- Custom build options

**Via CMake FetchContent:**
- Google Test
- nlohmann/json
- ANTLR
- Small libraries

### Common Build Issues

**Clang-Format Related:**
- Pre-commit hooks automatically format code
- If pre-commit fails, run: `clang-format -i <file>`
- Line length: 100 characters

**Dependency Issues:**
- Clean build: `rm -rf build && mkdir build && cd build`
- Update conan dependencies: `conan install`
- Clear CMake cache: `rm CMakeCache.txt`

**Test Failures:**
- Ensure you're on correct branch
- Run tests individually to isolate failures
- Check recent commits for context
- Verify compiler version compatibility

---

## Documentation & Comments

### Comment Guidelines

**When to Add Comments:**
- Non-obvious algorithm logic
- Performance-critical sections with justification
- Complex mathematical operations
- Workarounds for known issues with explanation
- Public API documentation

**When NOT to Add Comments:**
- Self-evident code (function does what name suggests)
- Simple loops and conditions
- Setup/teardown code
- Previously unchanged code

**Comment Style:**
```cpp
// Single-line comments for brief explanations

/* Multi-line comments for longer explanations
   that span multiple lines. Use sparingly. */

/// Doxygen-style comments for public APIs
/// \param operands The input data
/// \return The computed result
```

### Documentation Files

- **README.md** - Project overview and quick start
- **CONTRIBUTING.md** - How to contribute (if exists)
- **docs/** - Additional documentation (architecture, algorithms)
- **docs/development/** - Development guides
- **CLAUDE.md** - This file (AI assistant guide)

---

## Common Tasks & Patterns

### Adding a New Operation Type

1. **Create Header** (`src/engine/NewOperation.h`):
   ```cpp
   #include "Operation.h"

   class NewOperation : public Operation {
   public:
     NewOperation(...);
     ResultTable execute() override;
     // Other virtual methods...
   };
   ```

2. **Create Implementation** (`src/engine/NewOperation.cpp`):
   - Implement execute() and other methods
   - Follow existing operation patterns

3. **Register with Planner** (`src/engine/QueryPlanner.cpp`):
   - Add cost estimation
   - Update plan creation logic

4. **Add Tests** (`test/engine/NewOperationTest.cpp`):
   - Test basic functionality
   - Test edge cases
   - Test with different column widths (IdTableStatic)

5. **Update CMakeLists.txt**:
   - Add source files to executable targets

### Adding a New SPARQL Expression

1. **Create Expression Class** (`src/engine/sparqlExpressions/NewExpression.h`)
2. **Implement Evaluation** - Override `operator()`
3. **Add Type Checking** - Validate input types
4. **Test Thoroughly** - Including error cases
5. **Document** - Explain SPARQL standard compliance

### Optimizing a Query Operation

1. **Profile** - Identify bottleneck (ctest + profiler)
2. **Understand** - Study current implementation thoroughly
3. **Optimize** - Minimal, targeted changes
4. **Benchmark** - Measure improvement
5. **Test** - Ensure correctness, add benchmarks

### Adding RDF Type Support

1. **Update RDFTypes** (`src/rdfTypes/`)
2. **Update Index** - Handle new type in storage
3. **Update Parser** - Parse new type syntax
4. **Update Operations** - Handle in expressions
5. **Add Tests** - Comprehensive test coverage

---

## Troubleshooting

### Build Failures

**Clang-Format Errors:**
```bash
# Auto-fix formatting
clang-format -i src/engine/file.cpp

# Or use pre-commit
pre-commit run --all-files
```

**Compiler Errors:**
- Ensure C++20 compiler (GCC 11+ or Clang 16+)
- Check CMakeLists.txt for correct C++ version
- Verify all dependencies are installed

**Link Errors:**
- Ensure libraries are in CMakeLists.txt
- Check library order in target_link_libraries
- Verify shared library dependencies

### Test Failures

**Flaky Tests:**
- Re-run with `ctest --rerun-failed`
- Check for race conditions (use thread sanitizer)
- Verify test isolation

**Segmentation Faults:**
```bash
# Build with address sanitizer
cmake -DCMAKE_BUILD_TYPE=ASAN ..
cmake --build .
ctest --output-on-failure
```

### Performance Issues

**Profiling:**
```bash
# Build with profiling
cmake -DPERFTOOLS_PROFILER=ON ..
# Run with profiling enabled
# Results in gperf output
```

**Optimization Tips:**
- Use `IdTableStatic<N>` instead of dynamic `IdTable` for fixed-width results
- Avoid unnecessary allocations in hot paths
- Use `Synchronized<T>` sparingly
- Consider `ConcurrentCache` for frequently accessed data

---

## Quick Reference: File Navigation

| Task | Location |
|------|----------|
| Query Execution | `src/engine/` |
| SPARQL Functions | `src/engine/sparqlExpressions/` |
| RDF Storage | `src/index/` |
| Text Search | `src/index/textIndex/` |
| Spatial Queries | `src/index/spatial/` |
| SPARQL Parsing | `src/parser/` |
| Utilities | `src/util/` |
| RDF Types | `src/rdfTypes/` |
| Engine Tests | `test/engine/` |
| Index Tests | `test/index/` |
| Parser Tests | `test/parser/` |
| CI/CD Config | `.github/workflows/` |
| Code Style | `.clang-format`, `.pre-commit-config.yaml` |
| Build Config | `CMakeLists.txt`, `conanfile.txt` |

---

## Useful Commands

### Quick Development Workflow

```bash
# One-time setup
./scripts/setup-dev-env.sh

# Build and test (recommended approach)
./scripts/build-release.sh
cd build && ctest --output-on-failure

# Format code (alternative: pre-commit handles this on commit)
./scripts/format.sh

# Run specific tests
./scripts/run-tests.sh JoinTest
./scripts/run-tests.sh 'Filter.*'  # Regex pattern

# Debug build (separate directory)
./scripts/build-debug.sh
cd build-debug && ctest -R TestName --output-on-failure
```

### Manual Commands (if not using scripts)

```bash
# Clone and setup
git clone <repo>
cd qlever

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build . -- -j$(nproc)

# Test
ctest --output-on-failure
ctest -R TestName --output-on-failure

# Format
pre-commit run --all-files
# or manually:
clang-format -i src/engine/file.cpp

# Git operations
git status
git add <files>
git commit -m "message"
git push -u origin <branch-name>
```

### Code Navigation & Search

```bash
# Find class definitions
grep -r "class Operation" src/
grep -r "IdTableStatic" src/

# Find specific functions
grep -rn "void execute()" src/engine/

# List test files
find test -name "*Test.cpp" | head -20
```

---

## Skills Optimization & Usage Guide

This section details how to use Claude Code skills for maximum efficiency in parallel development workflows.

### Available Skills & Concurrency Properties

#### Build & Test Skill (`build-test`)
**Purpose:** Compile C++ code and run test suites
**Concurrency Level:** Medium (shared build directory)
**Best Practices:**
- ✅ Use separate build directories per module if running in parallel
- ✅ Invoke on independent modules simultaneously
- ❌ Don't share CMake cache between agents
- ✅ Pre-compile dependencies once, reuse via `ccache`

**Usage Pattern:**
```bash
# Agent 1 & Agent 2 can invoke simultaneously on different modules
# Each agent: cmake -B build_agent1 src/
# Each agent: cmake --build build_agent1 --target validation
```

#### Code Quality Skill (`code-quality`)
**Purpose:** Format code, run linters, static analysis
**Concurrency Level:** High (can parallelize fully)
**Best Practices:**
- ✅ Run on different file sets simultaneously
- ✅ All agents can format independently (no conflicts)
- ✅ Static analysis parallelizes across files
- ✅ No shared state between agents

**Usage Pattern:**
```bash
# All agents can invoke simultaneously
Agent 1: clang-format -i src/engine/validation/*.cpp
Agent 2: clang-format -i src/engine/shex/*.cpp
Agent 3: clang-format -i src/engine/n3/*.cpp
# Zero conflicts, instant completion
```

#### C++ Patterns Skill (`cpp-patterns`)
**Purpose:** Design patterns, architecture guidance, best practices
**Concurrency Level:** Unlimited (no execution, instant)
**Best Practices:**
- ✅ All agents can invoke simultaneously
- ✅ No build/execution overhead
- ✅ Use for architecture questions across swarm
- ✅ Reference documentation instantly

**Usage Pattern:**
```
Agent 1: How do I implement the visitor pattern for AST traversal?
Agent 2: How do I use Synchronized<T> for thread-safe caching?
Agent 3: What's the RAII pattern for resource cleanup?
# All execute instantly, no contention
```

#### Debug & Profile Skill (`debug-profile`)
**Purpose:** Memory profiling, performance analysis, crash debugging
**Concurrency Level:** Low (single-threaded, resource-intensive)
**Best Practices:**
- ⚠️ Run sequentially per agent (GDB doesn't parallelize well)
- ✅ Use for critical performance bottlenecks
- ✅ Run overnight for heavy profiling
- ❌ Don't run on every commit

**Usage Pattern:**
```bash
# Sequential execution (wait for completion)
Agent 1: Profile validation service (complete)
Agent 2: Profile ShEx validator (wait for Agent 1)
Agent 3: Profile Datalog engine (wait for Agent 2)
```

#### SPARQL/RDF Skill (`sparql-rdf`)
**Purpose:** SPARQL query semantics, RDF data model, query optimization
**Concurrency Level:** Medium (may touch shared index)
**Best Practices:**
- ✅ Use for SPARQL expression validation
- ⚠️ Coordinate if testing against live index
- ✅ All agents can validate syntax simultaneously
- ❌ Don't load different datasets concurrently

**Usage Pattern:**
```
Agent 1: Validate SPARQL SELECT syntax
Agent 2: Validate SPARQL ASK syntax
Agent 3: Test RDF triple patterns
# All work on in-memory examples, no conflicts
```

### Skill Invocation Concurrency Matrix

| Skill | Parallelizable | Shared Resources | Recommendation |
|-------|---------------|------------------|----------------|
| `build-test` | ⚠️ Partial | CMake cache, compiler | Use separate build/ dirs or sequence |
| `code-quality` | ✅ Full | None | Invoke all agents simultaneously |
| `cpp-patterns` | ✅ Full | None | Invoke all agents simultaneously |
| `debug-profile` | ❌ None | Debugger, profiler | Run sequentially, one at a time |
| `sparql-rdf` | ✅ Mostly | Shared index (optional) | Coordinate if using live index |

### Optimal Skill Usage Patterns

**Pattern 1: Code Quality in Parallel (Validation Phase)**
```
// All agents invoke simultaneously
Agent 1: code-quality → Format validation/ module
Agent 2: code-quality → Format shex/ module
Agent 3: code-quality → Format n3/ module
// All complete in ~1 second total
```

**Pattern 2: Build Test Sequential (Build Phase)**
```
// Build once, share for testing
Shared: build-test → Compile everything (once)
Agent 1: Run tests for validation/ (uses shared build)
Agent 2: Run tests for shex/ (uses shared build)
Agent 3: Run tests for n3/ (uses shared build)
// Total time: 1 build + N test runs
```

**Pattern 3: C++ Patterns Queries in Parallel (Design Phase)**
```
// All agents ask questions simultaneously
Agent 1: cpp-patterns → "How to implement Strategy pattern?"
Agent 2: cpp-patterns → "When to use RAII vs manual cleanup?"
Agent 3: cpp-patterns → "C++20 concepts vs templates?"
// All answers arrive instantly
```

**Pattern 4: Debug Sequential, Rest Parallel (Optimization Phase)**
```
// Parallel for non-blocking work
Agent 1-6: Implement features (no blocking)
Agent 7: debug-profile → Performance analysis (blocking, lower priority)

// Sequential for profiling
debug-profile → Agent 7 profiles validation service
(Agent 7 waits) → Agent 8 profiles ShEx service
(Agent 8 waits) → Agent 9 profiles Datalog engine
```

### Skill Usage in EPIC 5 Context

**EPIC 5 successfully used:**
- ✅ `code-quality`: All 10 agents formatted code independently
- ✅ `build-test`: Tests run on agent-specific modules (no conflicts)
- ✅ `cpp-patterns`: Design decisions made via skill (instant)
- ⚠️ `debug-profile`: Used selectively on critical paths (sequential)
- ✅ `sparql-rdf`: Validation tests on SPARQL expressions (parallel)

**Result:** Zero skill-related bottlenecks, maximum parallelism maintained

### Anti-Patterns: Skill Usage to Avoid

**❌ Anti-Pattern 1: Sequential code-quality**
```bash
# Bad: Forces unnecessary serialization
for agent in 1 2 3 4 5; do
  invoke code-quality for agent
  wait for completion
done
```

**✅ Better: Parallel code-quality**
```bash
# Good: All agents format simultaneously
invoke code-quality for agents 1-5 (parallel)
# All complete instantly
```

**❌ Anti-Pattern 2: Profiling in hot path**
```bash
# Bad: Every agent profiles their code
Agent 1: invoke debug-profile (blocks other agents)
Agent 2: invoke debug-profile (waits)
...
# Total time: 10 × profiling_time
```

**✅ Better: Profile selectively**
```bash
# Good: Only profile bottlenecks
Agents 1-9: Implement code (fast)
Agent 10: debug-profile critical paths (sequential, non-blocking)
# Total time: implementation_time (agents 1-9) + profiling_time (agent 10)
```

---

## Resources & Links

- **Repository**: https://github.com/seanchatmangpt/qlever
- **SPARQL Spec**: https://www.w3.org/TR/sparql11-query/
- **RDF Spec**: https://www.w3.org/RDF/
- **CMake Documentation**: https://cmake.org/cmake/help/latest/
- **Google Test Documentation**: https://google.github.io/googletest/
- **Boost Documentation**: https://www.boost.org/doc/libs/1_81_0/
- **ANTLR**: https://www.antlr.org/

---

## Document Metadata

- **Last Updated**: 2026-01-01
- **Version**: 2.0 (Agent & Task Concurrency Optimization)
- **Created For**: AI Assistant Development Support + Agent Concurrency
- **Scope**: Comprehensive guide for QLever codebase + agent/skill optimization
- **Status**: Complete and Ready for Use

**Key Additions (Version 2.0):**
- New section: "Agent & Task Concurrency Optimization" (350+ lines)
  - Agent concurrency fundamentals and patterns
  - High-concurrency swarm patterns (validated with EPIC 5: 10 agents)
  - Dependency management (Tier-based execution)
  - Task concurrency best practices and rules
  - Conflict detection and resolution strategies
  - Agent prompt templates for maximum independence
- New section: "Skills Optimization & Usage Guide" (190+ lines)
  - Available skills and their concurrency properties
  - Skill invocation concurrency matrix
  - Optimal skill usage patterns (4 validated patterns)
  - EPIC 5 skill usage results
  - Anti-patterns to avoid
- Updated guidelines reflect lessons learned from EPIC 5 (10 parallel agents, ~10,000 LOC)

**Maintenance Notes:**
- Update this document when major architectural changes occur
- Document new design patterns as they emerge
- Keep technology stack section current with dependency updates
- Add new troubleshooting entries as issues are discovered
- Keep agent concurrency patterns section current with new successful swarm patterns
- Document new skill usage patterns as they emerge in production workflows
- Update EPIC-specific examples when completing similar large-scale parallel implementations

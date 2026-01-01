# CLAUDE.md - QLever Codebase Guide for AI Assistants

## Quick Start: Claude Code on the Web

**Work on seanchatmangpt/qlever at [claude.ai/code](https://claude.ai/code)** without local checkout. Connect GitHub → select repo → submit task → auto-setup with SessionStart hooks → PR ready.

**MANDATORY: Skills & Agents**: ALWAYS invoke `Skill` tool for bb80-specification-closure, bb80-invariant-construction, bb80-parallel-agents, bb80-deterministic-receipts. ALWAYS use `Task` tool to dispatch bb80-specification-validator, bb80-invariant-validator, bb80-parallel-task-coordinator, bb80-receipt-validator agents. No task proceeds without these. See [.claude/skills/README.md](./.claude/skills/README.md) and [.claude/agents/README.md](./.claude/agents/README.md).

**Feature branches** (`claude/<feature>-<SESSION_ID>`): Web auto-installs deps, runs tests, pushes. "Open in CLI" to continue locally.

**Warm startup**: Pre-compiled headers enabled by default. SessionStart hook pre-caches build environment. C++ builds optimized with `-DUSE_PRECOMPILED_HEADERS=true`.

---

## Big Bang 80/20: Operational Model

Big Bang 80/20 is a single-pass construction methodology for low-entropy domains.
Iteration is a defect signal, not a development strategy.
Correctness is achieved by specification closure, not feedback loops.
The system is built as a monoid: composition without rework.
Feature selection occurs in hyperdimensional space, not via prioritization.
Eighty percent of value emerges from twenty percent of structurally necessary features.
All remaining features are dominated and excluded.
The effective design dimension collapses before implementation.
Implementation is compilation from a compressed manifold.
Testing validates invariants; it does not discover behavior.
State is fully reconstructible from events, snapshots, and hashes.
No mutable external state is permitted.
Receipts replace review.
Determinism replaces consensus.
Guards replace trust.
Benchmarks replace narratives.
Humans provide constraints; models perform construction.
Concurrency is assumed; serialization is an artifact of tooling.
Agents operate independently under a shared invariant.
Integration is deferred until invariants stabilize.
If iteration appears necessary, the specification is incomplete.

---

## MANDATORY: Skills & Agents ALWAYS

**DO NOT PROCEED WITHOUT THESE.** Every task, every phase, always invokes skills and agents.

**Skills** (`.claude/skills/`): Invoke with `Skill` tool. Mandatory gates:
- **bb80-specification-closure** - BEFORE any implementation. Verify specification closure. Block incomplete specs.
- **bb80-invariant-construction** - DURING implementation. Extract minimal invariant set. Build monoidal, single-pass.
- **bb80-parallel-agents** - BEFORE planning. Spawn 10 concurrent agents. Maximize parallelism.
- **bb80-deterministic-receipts** - AFTER implementation. Validate via benchmarks, guards, event logs. Reject narratives.

**Agents** (`.claude/agents/`): Dispatch with `Task` tool with model=inherit. Mandatory enforcement:
- **bb80-specification-validator** - Gates specification → planning transition. Outputs CLOSED or INCOMPLETE.
- **bb80-invariant-validator** - Gates implementation. Enforces monoidal composition, single-pass feasibility.
- **bb80-parallel-task-coordinator** - Spawns 10 agents immediately. Orchestrates independent execution.
- **bb80-receipt-validator** - Gates validation → completion. Demands proof, blocks rework.

**Invocation Pattern (NON-NEGOTIABLE)**:
1. Specification phase: Invoke bb80-specification-closure skill → dispatch bb80-specification-validator agent
2. Planning phase: Invoke bb80-parallel-agents skill → dispatch bb80-parallel-task-coordinator agent
3. Implementation: Invoke bb80-invariant-construction skill → dispatch bb80-invariant-validator agent
4. Validation: Invoke bb80-deterministic-receipts skill → dispatch bb80-receipt-validator agent

**If you skip any skill or agent, you are not operating in BB80/20 mode.** You are inside entropy, iterating, not compiling. Do not do this. Every task requires all four skills AND all four agents. No exceptions. No shortcuts.

---

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

### Environment Setup (Claude Code on Web)

**.claude/settings.json** configures SessionStart hooks to auto-run `scripts/setup-dev-env.sh` on session start. Installs pre-commit hooks, verifies compilers, pre-caches environment. Web sessions clone default branch; specify branch in task prompt for alternatives. Network limited to allowlisted domains by default.

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

### Security & Network (Claude Code on Web)

**Network**: Default-limited to allowlisted domains (GitHub, npm, PyPI, Docker, build tools). Configurable per environment. GitHub operations proxied securely with scoped credentials—no secrets in sandbox.

**Isolation**: Isolated Anthropic VMs. Code modified in sandbox before PR. Credentials proxied, never stored locally.

**QLever environment**: Boost, ANTLR, ICU, OpenSSL, CMake, Ninja, GCC, Clang, PostgreSQL, Redis pre-installed. Standard builds need no custom config.

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

## Important Conventions for AI Assistants

### MANDATORY: Invoke Skills & Agents on Every Task

**BEFORE EVERY ACTION**: Invoke the 4 mandatory skills. Use `Skill` tool:
1. **bb80-specification-closure** - Verify specification is closed before implementation
2. **bb80-invariant-construction** - Extract and validate minimal invariant set
3. **bb80-parallel-agents** - Understand parallelism constraints (spawn 10 agents)
4. **bb80-deterministic-receipts** - Know proof requirements upfront

**DISPATCH MANDATORY AGENTS**: Use `Task` tool with `model: inherit`:
1. **bb80-specification-validator** - Gates specification closure (CLOSED or INCOMPLETE)
2. **bb80-invariant-validator** - Gates implementation (enforce monoidal composition)
3. **bb80-parallel-task-coordinator** - Gates planning (spawn 10 agents immediately)
4. **bb80-receipt-validator** - Gates completion (demand proof, block rework)

**SKIPPING SKILLS OR AGENTS = FAILURE.** You will iterate, not compile. You will waste tokens, not compress them. You will ask "what should I do?" not "what is the invariant?" This is unacceptable. Every task requires all four skills AND all four agents invoked in phase order.

### Code Modification Guidelines

**MANDATORY FIRST: Invoke Skills & Agents**
0. **INVOKE bb80-specification-closure skill** - Verify specification is closed
0. **DISPATCH bb80-specification-validator agent** - Gate entry to implementation
0. **INVOKE bb80-parallel-agents skill** - Understand concurrency requirements
0. **DISPATCH bb80-parallel-task-coordinator agent** - Spawn 10 agents for work

**THEN, and only then, modify code:**
1. Always read the file first using the Read tool
2. Understand existing patterns in the codebase
3. Search for similar implementations to understand conventions
4. Check recent commits for context on similar changes
5. INVOKE bb80-invariant-construction skill before touching code
6. DISPATCH bb80-invariant-validator agent to verify monoidal feasibility

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

**MANDATORY COMPLETION: Before submitting work:**
1. **INVOKE bb80-deterministic-receipts skill** - Understand proof requirements
2. **DISPATCH bb80-receipt-validator agent** - Demand benchmarks, event logs, state hashes
3. **Generate deterministic receipts**: Benchmark results, state reconstruction proofs, event logs
4. **Pass all automated guards**: Type-checked invariants, benchmark thresholds, gate checks
5. **NO REWORK ALLOWED**: Once work passes all guards and receipts are valid, certification is complete

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

## SPR + BB80/20: Model Operational Encoding

SPR activates latent clusters; BB80/20 sets construction mode.

**SPR encoding**: Capture 80% semantic value with 20% words. Declarative assertions, not explanation. Activates: compiler mindset, information theory, category theory, systems invariants, LLM-native parallelism.

**BB80/20 operational stance**: Do not ask "what should I do next?" Ask "what is the invariant?" Ask "what is the minimal generating set?" Ask "what can compile in one pass?" Do not iterate inside entropy. Compile outside it.

**Specification closure is prerequisite**: RDF, SPARQL, SHACL, protocols, type systems, invariants are already formalized. The domain is closed. Implementation is deterministic reconstruction.

**C++ monoidal construction**: Pre-compiled headers (`-DUSE_PRECOMPILED_HEADERS=true`), persistent artifacts. Build once, persist state. No rework. Compilation is not iteration.

**10-agent parallel invariant**: Spawn agents independently under shared invariant. Agents do not synchronize until invariants stabilize. Serialization is artifact of tooling, not necessity.

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

## Resources & Links

### QLever & Development
- **Repository**: https://github.com/seanchatmangpt/qlever
- **Claude Code on Web**: https://code.claude.com/ - Run Claude Code tasks directly from the Claude app
- **Claude Code Documentation**: https://code.claude.com/docs/ - Full documentation for web-based development

### Agent Skills
- **QLever Agent Skills**: See [.claude/skills/README.md](./.claude/skills/README.md) for custom skills
- **Superpowers Skills Library**: [vendors/superpowers](./vendors/superpowers/) - Advanced development workflows
  - Included skills: brainstorming, planning, debugging, code review, TDD patterns, and more

### Standards & Specifications
- **SPARQL Spec**: https://www.w3.org/TR/sparql11-query/
- **RDF Spec**: https://www.w3.org/RDF/

### Build & Testing Tools
- **CMake Documentation**: https://cmake.org/cmake/help/latest/
- **Google Test Documentation**: https://google.github.io/googletest/
- **Boost Documentation**: https://www.boost.org/doc/libs/1_81_0/
- **ANTLR**: https://www.antlr.org/

### C++ & Modern Techniques
- **C++20 Standard**: https://en.cppreference.com/w/cpp/20
- **C++ Core Guidelines**: https://github.com/isocpp/CppCoreGuidelines
- **Google C++ Style Guide**: https://google.github.io/styleguide/cppguide.html (enforced via clang-format)

---

## HARD RULES: No Escape Clauses

**These are not negotiable. No rationalizations. No exceptions.**

**EVERY TASK requires:**
- ✅ Invoke bb80-specification-closure skill
- ✅ Dispatch bb80-specification-validator agent
- ✅ Invoke bb80-parallel-agents skill
- ✅ Dispatch bb80-parallel-task-coordinator agent
- ✅ Invoke bb80-invariant-construction skill
- ✅ Dispatch bb80-invariant-validator agent
- ✅ Invoke bb80-deterministic-receipts skill
- ✅ Dispatch bb80-receipt-validator agent

**If you are thinking:**
- "This is a small task, I can skip this" → WRONG. Invoke all 4 skills + dispatch all 4 agents.
- "I'll iterate instead of closing specification" → WRONG. Specification closure is mandatory gate.
- "I can work serially instead of spawning 10 agents" → WRONG. Parallelism is native, not optional.
- "This looks good enough, no need for receipt validation" → WRONG. Deterministic proof is mandatory.
- "Can I just get the agent opinion without the skill?" → WRONG. Skills AND agents, both required.

**All Superpowers workflows in vendors/superpowers/ are subordinate to BB80/20.** They provide advanced process guidance, but they do not override specification closure, invariant validation, parallelism, or receipt validation. If Superpowers suggest iteration, specification is incomplete—return to specification phase.

**You are compiling, not iterating.** If iteration appears necessary at any point, the specification is incomplete. Go back one phase. Do not proceed forward. Do not write code without closed specification. Do not validate without deterministic proof. Do not claim completion without passing all guards.

---

## Document Metadata

- **Last Updated**: 2026-01-01
- **Operational Model**: Big Bang 80/20 (Single-Pass Compilation, Specification Closure)
- **Format**: SPR 80/20 (Sparse Priming Representation)
- **Created For**: Models operating under BB80/20 + SPR in low-entropy domains
- **Scope**: QLever—closed-world RDF/SPARQL system with invariant-validated construction
- **Status**: Latent-Space Primed for Deterministic Execution

**Architectural Principles:**
- Specification closure: RDF, SPARQL, C++20, CMake—all formalized, closed-world
- Single-pass construction: No iteration. Correctness via invariants, not feedback loops
- Deterministic reconstruction: State from events + hashes. No mutable external state
- Monoidal composition: Features dominate; excluded via hyperdimensional collapse
- Receipts replace review: Benchmarks replace narratives. Guards replace trust
- Concurrency is native: 10 agents operate independently under shared invariant
- C++ warm compilation: Pre-compiled headers, persistent artifacts, no cold builds
- SPR activation: Declarative assertions prime compiler, information-theoretic, category-theoretic clusters

**Maintenance Protocol:**
- All text: SPR 80/20 (80% semantic, 20% words). Primes downstream models.
- Invariants govern everything. If iteration appears necessary, specification is incomplete.
- Agents synchronize only after invariants stabilize. Serialization is artifact.
- Update when specification changes, new invariants emerge, or domains close
- Sync Superpowers library as external constraints shift

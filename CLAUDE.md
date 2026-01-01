# CLAUDE.md - QLever Codebase Guide for AI Assistants

## Quick Start: Claude Code on the Web

**QLever is optimized for [Claude Code on the web](https://code.claude.com/)** - work on this repository directly from the Claude app without needing a local checkout.

### How to use Claude Code on the web with QLever

1. Visit [claude.ai/code](https://claude.ai/code)
2. Connect your GitHub account
3. Select the **seanchatmangpt/qlever** repository
4. Submit your task (bug fix, feature, question about code)
5. Claude will work with automatic dependency installation and pre-configured environment
6. Review changes and create a PR when done

### Available Agent Skills

QLever includes custom Agent Skills (in `.claude/skills/`) that guide Claude on domain-specific tasks:

- **build-test** - CMake builds, ctest execution, build troubleshooting
- **code-quality** - Formatting with clang-format, linting, spell checking
- **cpp-patterns** - C++ design patterns, QLever architecture, modern C++
- **debug-profile** - Memory debugging (ASAN), profiling, performance analysis
- **sparql-rdf** - SPARQL query semantics, RDF models, semantic web standards

These skills are **automatically invoked** based on your task. For example:
- "Build the project and run tests" → Triggers **build-test**
- "Fix this memory leak" → Triggers **debug-profile**
- "Write a SPARQL query for..." → Triggers **sparql-rdf**

See [.claude/skills/README.md](./.claude/skills/README.md) for full details on each skill.

### Working with Feature Branches

All changes develop on feature branches (e.g., `claude/my-feature-XYZ`):

```bash
# Claude Code on web automatically:
# 1. Creates isolated workspace on branch
# 2. Installs dependencies (via SessionStart hook)
# 3. Runs tests and validates work
# 4. Pushes to GitHub branch
# 5. Ready for PR creation
```

**To move from web to terminal:** Click "Open in CLI" and paste the command in your local checkout.

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

**Automatic dependency installation** is configured in `.claude/settings.json` using SessionStart hooks:

```json
{
  "hooks": {
    "SessionStart": [
      {
        "matcher": "startup",
        "hooks": [
          {
            "type": "command",
            "command": "\"$CLAUDE_PROJECT_DIR\"/scripts/setup-dev-env.sh"
          }
        ]
      }
    ]
  }
}
```

When you start a Claude Code session (web or local), this hook automatically:
- Installs pre-commit hooks for code quality checks
- Verifies compiler and build tool availability
- Sets up development environment

**For Claude Code on web specifically:**
- Repository is cloned with default branch (main/master)
- To use a specific branch, specify in your task prompt: "Work on branch `feature/xyz`"
- Environment variables can be configured in your web session settings
- Internet access is limited to allowlisted domains by default (see Security & Network section below)

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

When running Claude Code on the web, the following security measures apply:

**Network Access:**
- **Default**: Limited to allowlisted domains (GitHub, package managers, build tools, cloud services)
- **Configuration**: Can be changed per environment in web session settings
- **Examples of allowed domains**: npmjs.org, pypi.org, github.com, docker.io, etc.
- **GitHub operations**: Handled through secure proxy with scoped credentials (no credentials in sandbox)

**Isolation & Protection:**
- Each session runs in isolated, Anthropic-managed virtual machines
- Code is analyzed and modified within isolated VMs before PR creation
- Credentials (git tokens, signing keys) never stored in sandbox - proxied securely
- Pre-configured environment with common toolchains pre-installed

**For QLever specifically:**
- All Boost, ANTLR, ICU, OpenSSL dependencies are available in the universal image
- Build tools: CMake, Ninja, GCC, Clang pre-installed
- Database support: PostgreSQL, Redis available if needed
- No custom network configuration needed for standard builds

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

### Using Agent Skills

Before working on tasks, **invoke relevant Agent Skills** from `.claude/skills/`:

1. **build-test** - When building, running tests, or troubleshooting builds
2. **code-quality** - When formatting, checking style, or analyzing code
3. **debug-profile** - When debugging crashes, memory issues, or optimizing performance
4. **cpp-patterns** - When designing new features or refactoring code
5. **sparql-rdf** - When working with SPARQL queries or RDF data

Skills are automatically discovered and invoked based on your task context. If a skill applies, you'll see guidance integrated into your work.

### Code Modification Guidelines

**Before Making Changes:**
1. Always read the file first using the Read tool
2. Understand existing patterns in the codebase
3. Search for similar implementations to understand conventions
4. Check recent commits for context on similar changes
5. If relevant to your task, invoke the appropriate Agent Skill for domain-specific guidance

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

## Document Metadata

- **Last Updated**: 2026-01-01
- **Created For**: AI Assistant Development Support (Cloud & Web Optimized)
- **Scope**: Comprehensive guide for QLever codebase with Claude Code on the web
- **Status**: Complete and Ready for Use

**Key Updates:**
- Added Claude Code on the web quick start and environment setup
- Documented 5 custom Agent Skills for domain-specific guidance
- Added SessionStart hook configuration for automatic dependency installation
- Included Superpowers skills library for advanced workflows
- Added security & network information for web-based development

**Maintenance Notes:**
- Update this document when major architectural changes occur
- Document new design patterns as they emerge
- Keep technology stack section current with dependency updates
- Add new troubleshooting entries as issues are discovered
- Update Agent Skills in `.claude/skills/` as domain knowledge improves
- Sync with Superpowers updates in `vendors/superpowers/`

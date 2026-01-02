# Contributing to QLever

Thank you for your interest in contributing to QLever! This document provides guidance for developers and contributors. QLever follows the **Big Bang 80/20 + EPIC 9** development methodology for single-pass, specification-driven construction.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Workflow](#development-workflow)
3. [Code Standards](#code-standards)
4. [Commit Message Guidelines](#commit-message-guidelines)
5. [Pull Request Process](#pull-request-process)
6. [Testing Requirements](#testing-requirements)
7. [Branch Protection Rules](#branch-protection-rules)
8. [Collaboration & SLA](#collaboration--sla)

---

## Getting Started

### Prerequisites

- **OS**: Ubuntu 22.04 LTS, macOS 13+, or Docker
- **CMake**: 3.27 or later
- **Compiler**: GCC 11+, Clang 16+
- **Build System**: Ninja
- **Language**: C++20

### One-Time Setup

```bash
# Clone the repository
git clone https://github.com/seanchatmangpt/qlever.git
cd qlever

# Set up development environment
./scripts/setup-dev-env.sh

# Install pre-commit hooks
pre-commit install

# Verify setup
make build
make test
```

See [Quick Start](docs/how-to/quick-start.md) for detailed instructions.

---

## Development Workflow

### Feature Branch Strategy

QLever uses AI-assisted feature branches with a standardized naming pattern:

**Branch Name Format**: `claude/<feature>-<SESSION_ID>`

**Examples**:
- `claude/optimize-join-algorithm-ABC123`
- `claude/fix-vocabulary-race-condition-XYZ789`
- `claude/add-spatial-index-caching-QRS456`

**Why this pattern?**
- `claude/` prefix signals AI-assisted implementation
- `<SESSION_ID>` ensures global uniqueness across concurrent agents
- Enables automatic archival & cleanup after merge

### Creating a Feature Branch

```bash
# Create & push feature branch (example)
git checkout -b claude/my-feature-SESSION_ID
git push -u origin claude/my-feature-SESSION_ID

# Open PR on GitHub
# (GitHub will auto-detect from pushed branch)
```

### Keeping Branch Updated

```bash
# Before merge, sync with master
git fetch origin master
git rebase origin/master

# If conflicts exist, resolve locally
# Then force-update feature branch
git push -f origin claude/my-feature-SESSION_ID
```

---

## Code Standards

### C++ Style Guidelines

QLever enforces code style via **pre-commit hooks** and **CI/CD workflows**. All code must comply before merge.

#### Format Enforcement

**Tool**: `clang-format` v16.0.6
**Config**: `.clang-format` (Google style + customizations)
**Trigger**: Automatic on every commit

**Auto-fix formatting**:
```bash
# Pre-commit will auto-fix on commit
git commit -m "feat(engine): Add adaptive cost estimation"
# If formatting violations detected:
#   Files will be reformatted automatically
#   Commit will fail (requires re-run)
git commit -m "feat(engine): Add adaptive cost estimation"  # Re-run
```

**Manual formatting**:
```bash
# Format entire codebase
./scripts/format.sh

# Check formatting without fixing
clang-format --dry-run src/**/*.cpp
```

#### Static Analysis

**Tool**: `clang-tidy` (80+ checks)
**Config**: `.clang-tidy`
**Checks Enabled**:
- readability-* (function complexity, naming)
- performance-* (inefficient patterns)
- concurrency-* (thread safety)
- bugprone-* (common errors)

**Run locally**:
```bash
clang-tidy -p build src/engine/QueryPlanner.cpp
```

**Fix violations**:
```bash
clang-tidy -p build -fix src/engine/QueryPlanner.cpp
```

#### Spelling & Documentation

**Tool**: `codespell` v2.2.6
**Config**: `.codespellrc`

Catches common typos in code, comments, and documentation.

```bash
# Check spelling
codespell src/

# Automatically fix
codespell -w src/
```

### C++ Best Practices

- **C++20 features** encouraged (std::ranges, concepts, coroutines)
- **No exceptions in hot paths** (use std::optional, std::expected)
- **RAII pattern** for resource management
- **Const-correctness** enforced throughout
- **No raw new/delete** (use std::unique_ptr, std::make_shared)
- **Thread-safe by default** (use Synchronized<T>, atomic, mutexes)

### Comments & Documentation

- **Class headers**: Brief description of responsibility
- **Function headers**: Parameter descriptions, return value, preconditions
- **Complex algorithms**: Explain non-obvious logic with comments
- **TODO comments**: Include author name and reason
  ```cpp
  // TODO(sean): Optimize this with SIMD operations (EPIC 10.3)
  ```

---

## Commit Message Guidelines

QLever follows **Conventional Commits** specification for clear, machine-readable commit messages.

### Format

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

### Types

- **feat**: New feature
- **fix**: Bug fix
- **docs**: Documentation update
- **test**: Test addition/modification
- **refactor**: Code refactoring (no behavior change)
- **perf**: Performance improvement
- **chore**: Build, CI, dependency update
- **style**: Code style (formatting, whitespace)

### Scope

Optional but recommended. Identifies affected module:

- `engine` - Query execution engine
- `index` - RDF indexing & storage
- `parser` - SPARQL/N3 parsing
- `util` - Utility functions & data structures
- `rdf` - RDF type system
- `build` - CMake, Makefile, compilation
- `test` - Testing infrastructure
- `docs` - Documentation
- `dx` - Developer experience
- `EPIC-N` - EPIC-specific work

### Examples

**Good Commits**:
```
feat(engine): Add adaptive join cost estimation with ML feedback

Implements machine learning-based cost estimation for join order
optimization. Reduces query planning overhead by 15% on complex queries.

Closes #123
```

```
fix(index): Prevent vocabulary race condition in concurrent ingress

Use atomic swap instead of manual lock to prevent TOCTOU violation
when updating vocabulary entries during parallel data ingestion.
```

```
docs(dx): Add macOS setup guide with Homebrew instructions

Provides step-by-step instructions for setting up QLever development
environment on macOS using Homebrew package manager.
```

**Avoid**:
- ❌ `fix: stuff` (unclear)
- ❌ `Updated files` (non-informative)
- ❌ `Merge branch master` (generated messages)

### Commit Message Validation

Commits are automatically validated in CI/CD via `commitlint` to ensure format compliance.

---

## Pull Request Process

### Before Opening PR

1. **Ensure branch is up-to-date**:
   ```bash
   git fetch origin master
   git rebase origin/master
   ```

2. **Run local quality checks**:
   ```bash
   make lint        # clang-tidy
   make format-check  # clang-format
   make test        # Unit tests
   ```

3. **Verify your changes don't introduce new warnings**:
   ```bash
   # Build with strict compilation flags
   cmake -DCMAKE_CXX_FLAGS="-Werror" ..
   cmake --build .
   ```

### Opening PR

1. **Use semantic PR title**:
   - Format: `type(scope): description`
   - Example: `feat(engine): Add adaptive join cost estimation`

2. **Fill out PR template** (auto-filled):
   - Select type of change
   - Describe what changed and why
   - List testing performed
   - Note any documentation updates
   - Mention breaking changes (if any)

3. **Link related issues**:
   ```markdown
   Closes #123
   Related to #124
   ```

### PR Review Process

**Checklist for reviewers**:
- [ ] Conventional Commits format
- [ ] All CI/CD checks passing (18 workflows, ~7.5 min)
- [ ] Code follows style guidelines (clang-format, clang-tidy)
- [ ] New features have tests
- [ ] Documentation updated
- [ ] No breaking changes (or documented)

### Merge Requirements

All of the following must be satisfied before merge:

- ✅ All GitHub Actions workflows passing
- ✅ Code review approval (minimum: 1 human)
- ✅ No unresolved conversations
- ✅ Branch up-to-date with `master`
- ✅ Commit message format: Conventional Commits
- ✅ No force-push to master (branch protection enforced)

### Post-Merge

- Feature branch automatically deleted by GitHub
- Commit is tagged with branch name for traceability
- Artifact sealing (Phase F) runs automatically
- Deterministic build verification (Phase E) validates reproducibility

---

## Testing Requirements

### Unit Tests

**Framework**: Google Test (GTest)
**Location**: `test/` directory mirrors `src/`
**Execution**: `make test` (runs 289+ tests in parallel)

**For new features**:
```bash
# Add test file: test/engine/MyNewFeatureTest.cpp
# Compile and run
make test

# Run specific test
ctest -R "MyNewFeature" -j$(nproc)
```

**For bug fixes**:
- Add regression test that fails without the fix
- Verify test passes with the fix applied
- Prevents re-introduction of bug

### Integration Tests

**Location**: `test/integration/`
**Scope**: Cross-module workflows, end-to-end scenarios

### Performance Tests

**Location**: `benchmark/`
**Framework**: Custom `BenchmarkInterface`
**Execution**: `make benchmark`

For performance-sensitive changes:
```bash
# Run benchmark before & after change
./benchmark/JoinAlgorithmBenchmark > before.json
# Apply change
./benchmark/JoinAlgorithmBenchmark > after.json

# Compare results
python3 misc/compare_performance.py before.json after.json
```

### Test Coverage

Current coverage: ~75% for core modules, verified via `code-coverage.yml`

**Target**: Maintain > 70% coverage for new code

---

## Branch Protection Rules

The `master` branch enforces the following protections:

1. **Status Checks**
   - All GitHub Actions workflows must pass (18 total)
   - Format check (clang-format)
   - Lint check (clang-tidy hot-path)
   - Native build (multi-compiler matrix)
   - Code coverage (LLVM instrumentation)
   - SPARQL conformance (W3C RDF spec)

2. **Code Review**
   - Minimum: 1 approval from code owner
   - Dismiss stale reviews on new commits
   - Require re-review on force-push (not recommended)

3. **Commit Requirements**
   - Linear history (rebase required, no merge commits to feature branches)
   - Squash commits before merge recommended (keeps history clean)
   - Conventional Commits format validated by CI/CD

4. **Conflict Resolution**
   - PR must be up-to-date with `master` before merge
   - Conflicts must be resolved locally

---

## Collaboration & SLA

### Code Review Turnaround Times

| Category | Target | Escalation |
|----------|--------|-----------|
| Critical (blocking bug) | 4 hours | @seanchatmangpt if delayed |
| High priority (feature) | 24 hours | @seanchatmangpt if delayed |
| Documentation | 48 hours | Close if no response after 1 week |
| Routine improvements | 48 hours | Auto-close if stale > 2 weeks |

### Communication

- **Questions about process**: Comment on PR
- **Blocked on review**: Tag `@seanchatmangpt`
- **Discussion on design**: Open GitHub Discussion (not Issue)
- **Bug reports**: Use Issue template
- **Feature requests**: Use Discussion or Issue template

### Respect Constraints

QLever uses **Big Bang 80/20 + EPIC 9** methodology:

- **Specification closure** required before implementation
- **Single-pass construction** preferred (avoid iteration)
- **Monoidal composition** enforced (no rework)
- **Deterministic receipts** validate work (benchmarks, not narratives)
- **No breaking changes** without deprecation period

See [`CLAUDE.md`](CLAUDE.md) for full methodology.

---

## Additional Resources

- **Quick Start**: [docs/how-to/quick-start.md](docs/how-to/quick-start.md)
- **Architecture**: [docs/explanation/architecture.md](docs/explanation/architecture.md)
- **Build System**: [docs/how-to/master-makefile.md](docs/how-to/master-makefile.md)
- **Performance**: [docs/explanation/performance.md](docs/explanation/performance.md)
- **Development Methodology**: [CLAUDE.md](CLAUDE.md)

---

## Code of Conduct

QLever follows the [Contributor Covenant](https://www.contributor-covenant.org/) Code of Conduct. By participating, you agree to uphold this code.

---

**Questions?** Open a GitHub Discussion or contact [@seanchatmangpt](https://github.com/seanchatmangpt).

Thank you for contributing to QLever!

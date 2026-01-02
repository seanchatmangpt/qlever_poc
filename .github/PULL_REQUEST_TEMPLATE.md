## Type of Change

- [ ] **feat** - New feature
- [ ] **fix** - Bug fix
- [ ] **docs** - Documentation update
- [ ] **refactor** - Code refactoring (no behavior change)
- [ ] **perf** - Performance improvement
- [ ] **test** - Test addition/modification
- [ ] **chore** - Build/CI/dependency update
- [ ] **style** - Code style (formatting, whitespace)

## Scope (if applicable)

- [ ] `engine` - Query execution engine
- [ ] `index` - RDF indexing & storage
- [ ] `parser` - SPARQL/N3 parsing
- [ ] `util` - Utility functions & data structures
- [ ] `rdf` - RDF type system
- [ ] `build` - CMake, Makefile, compilation
- [ ] `test` - Testing infrastructure
- [ ] `docs` - Documentation
- [ ] `dx` - Developer experience

## Description

<!-- Clear description of what changed and why -->

## Changes Made

- [ ] Change 1
- [ ] Change 2
- [ ] Change 3

## Testing

### Unit Tests
- [ ] New unit tests added
- [ ] Existing unit tests pass
- [ ] Test command: `make test`

### Integration Tests
- [ ] Integration tests pass (if applicable)
- [ ] Command: `ctest -R "pattern"`

### Performance Tests
- [ ] Benchmark comparison (if performance-sensitive)
- [ ] No regression detected
- [ ] Before/after metrics attached (if applicable)

### Manual Testing
- [ ] Tested locally on development environment
- [ ] OS: <!-- Ubuntu 22.04 / macOS / Docker / Other -->
- [ ] Compiler: <!-- GCC 11 / Clang 16 / Other -->

## Documentation

- [ ] Code comments added for complex logic
- [ ] Function/class headers documented
- [ ] Public API documentation updated (if applicable)
- [ ] README.md updated (if new feature)
- [ ] CLAUDE.md updated (if methodology impact)
- [ ] docs/ folder updated (if user-facing change)
- [ ] Architecture documentation updated (if structural change)

## Breaking Changes?

- [ ] No breaking changes
- [ ] **YES - Breaking changes** (specify below):
  <!-- Describe what changed and migration path for users -->

## Related Issues

<!-- Link related issues: Closes #123, Related to #124 -->
Closes #<!-- issue number -->

## Checklist Before Merge

- [ ] Follows Conventional Commits format: `type(scope): description`
- [ ] Commit messages are clear and informative
- [ ] No conflicts with `master` branch
- [ ] All GitHub Actions workflows passing (18 total, ~7.5 min)
- [ ] Code passes `clang-format` check
- [ ] Code passes `clang-tidy` lint
- [ ] `codespell` passes (no typos)
- [ ] Test coverage maintained (> 70% for new code)
- [ ] No new compiler warnings introduced
- [ ] No breaking changes (or documented with migration path)

## Additional Notes

<!-- Any additional context, trade-offs, or notes -->

---

**Note**: This PR template is auto-filled. Please fill out all sections and ensure all items in the checklist are satisfied before requesting review.

For questions, see [CONTRIBUTING.md](../CONTRIBUTING.md) or [CLAUDE.md](../CLAUDE.md).

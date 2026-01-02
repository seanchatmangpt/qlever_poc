---
name: Bug Report
about: Report a bug or unexpected behavior
title: "[BUG] "
labels: bug
assignees: ''
---

## Describe the Bug

A clear and concise description of what the bug is. What did you expect to happen?

<!-- Example: "When running a SPARQL FILTER with string operations, the query hangs instead of returning results" -->

## Steps to Reproduce

Steps to reproduce the behavior:

1. <!-- e.g., "Build with GCC 11 in Release mode" -->
2. <!-- e.g., "Load the DBLP dataset (1B triples)" -->
3. <!-- e.g., "Execute the following SPARQL query:" -->
4. <!-- e.g., "Observe the result" -->

### SPARQL Query (if applicable)

```sparql
# Paste your SPARQL query here
SELECT ?x WHERE {
  ?x a dbo:Thing .
}
```

### RDF Data (if applicable)

```turtle
# Paste minimal RDF data to reproduce the issue
@prefix ex: <http://example.org/> .
ex:subject ex:predicate ex:object .
```

## Expected Behavior

What should happen instead?

## Actual Behavior

What actually happens? Include error messages, stack traces, or logs.

```
<paste error message or log output here>
```

## Environment

- **OS**: <!-- Ubuntu 22.04 / macOS / Docker / Other -->
- **OS Version**: <!-- e.g., Ubuntu 22.04 LTS -->
- **Architecture**: <!-- x86_64 / ARM64 / Other -->
- **Compiler**: <!-- GCC 11 / GCC 13 / Clang 16 / Clang 18 / Other -->
- **Compiler Version**: <!-- e.g., GCC 11.3.0 -->
- **QLever Build Type**: <!-- Release / Debug / RelWithDebInfo -->
- **QLever Version**: <!-- Branch name or commit hash -->
- **Dataset Size**: <!-- Number of triples, e.g., "1M triples" -->

## Installation Method

- [ ] Built from source (`make build`)
- [ ] Built via Docker
- [ ] Pre-built binary
- [ ] Other (specify): <!-- describe -->

## Reproduction Command

Provide the exact command(s) to reproduce the issue:

```bash
# e.g., "make build && make test" or "ctest -R MyTest"
```

## Logs & Debugging Info

Attach relevant logs or debug output:

```
<paste logs here>
```

### Build Log (if applicable)

```
<paste CMake/Ninja build output here>
```

### Test Output (if applicable)

```
<paste test output here>
```

## Possible Solution

If you have any ideas about what might be causing the issue, share them here.

## Additional Context

Any other context about the problem? Screenshots, performance profiles, or related issues?

---

**Note**: The more detailed your report, the faster we can diagnose and fix the issue. Thank you!

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines.

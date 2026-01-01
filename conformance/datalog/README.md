# Datalog Conformance Test Suite

This directory contains the conformance test suite for QLever's Datalog implementation (EPIC 5).

## Overview

The test suite validates:
- **Datalog fixpoint computation** with proper termination
- **Guard triggers** (max_iterations, max_derived_facts, max_rule_fires, max_runtime)
- **Output reproducibility** (deterministic digest computation)
- **Complex rule interactions** (recursion, joins, circular dependencies)

## Test Case Structure

Each test case is organized in a directory with the following files:

```
test_XXX_description/
├── input.ttl          # Initial RDF facts (Turtle format)
├── rules.datalog      # Datalog rules (simple text format)
├── expected.json      # Expected RuleExecutionResult
└── metadata.json      # Test description and feature coverage
```

## Test Cases (13 total)

### Regular Tests (9 tests)

1. **test_001_simple_derivation** - Simple fact derivation from base facts
2. **test_002_transitive_closure** - Transitive closure (ancestor from parent)
3. **test_003_multiple_rules** - Multiple rules firing in sequence
4. **test_004_recursive_rule** - Recursive rule with proper termination
5. **test_005_multi_predicate_join** - Join over multiple predicates
6. **test_006_complex_graph** - Complex graph with many interacting rules
7. **test_011_safe_termination** - Safe termination on cyclic graph (no infinite loop)
8. **test_012_reproducibility** - Verify output digest is reproducible
9. **test_013_circular_dependency** - Circular dependencies between predicates

### Guard Trigger Tests (4 tests)

These tests verify that guards properly prevent runaway execution:

7. **test_007_guard_max_iterations** - Trigger max_iterations guard
8. **test_008_guard_max_derived_facts** - Trigger max_derived_facts guard
9. **test_009_guard_max_rule_fires** - Trigger max_rule_fires guard
10. **test_010_guard_max_runtime** - Trigger max_runtime guard

**Important:** Guard trigger tests must:
- Return `outcome: "GUARDED"`
- Set `guard_triggered` to the guard name
- NOT return full results (fail closed)
- Have empty `output_digest_sha256`

## Running the Test Suite

```bash
# Build the conformance runner
cd build
cmake ..
make DatalogConformanceRunner

# Run all tests
./DatalogConformanceRunner

# Run tests from custom directory
./DatalogConformanceRunner /path/to/conformance/datalog /path/to/output.json
```

## Output Format

The runner produces `runner_output.json` with:

```json
{
  "total_tests": 13,
  "passed": 13,
  "failed": 0,
  "errors": 0,
  "success_rate": 1.0,
  "test_results": [
    {
      "test_name": "simple_derivation",
      "passed": true,
      "message": "PASS"
    },
    ...
  ]
}
```

## Guard Coverage

The test suite covers all 4 Datalog guards:

- ✅ **max_iterations** (test_007)
- ✅ **max_derived_facts** (test_008)
- ✅ **max_rule_fires** (test_009)
- ✅ **max_runtime** (test_010)

Each guard test verifies:
1. Guard triggers at the expected point
2. Execution stops (fail closed)
3. `outcome == "GUARDED"`
4. `guard_triggered` field is set correctly
5. No partial results are returned

## Feature Coverage (80/20 Datalog)

The suite provides 80/20 coverage of essential Datalog features:

- ✅ Simple fact derivation
- ✅ Transitive closure
- ✅ Recursive rules
- ✅ Multiple rules firing in sequence
- ✅ Joins over multiple predicates
- ✅ Complex rule interactions
- ✅ Circular dependencies
- ✅ Safe fixpoint termination
- ✅ Guard-based protection
- ✅ Reproducibility (deterministic output)

## Datalog Rule Format

Rules use a simple, SPARQL-compatible format:

```datalog
# Base rule
ancestor(?x, ?y) :- parent(?x, ?y).

# Recursive rule
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Multiple predicates in body
coworker(?x, ?y) :- worksAt(?x, ?company), worksAt(?y, ?company).
```

**Features:**
- SPARQL-style variables: `?x`, `?y`, `?z`
- Predicates: lowercase identifiers
- Comments: `#` or `//`
- Body patterns separated by commas

## Reproducibility

Test cases `test_012_reproducibility` and all regular tests verify that:
- Output digests are deterministic
- Multiple runs produce identical results
- Canonicalization is consistent

Digests are computed from canonicalized output facts using SHA-256.

## Success Criteria

All tests pass when:
- Regular tests: `outcome == "OK"` and all metrics match expected values
- Guard tests: `outcome == "GUARDED"` and correct guard is triggered
- Reproducibility: Same digest across multiple runs
- No partial results on guard triggers

## Implementation Notes

**Current Status (EPIC 5 Task 9):**
- ✅ Test case corpus created (13 test cases)
- ✅ DatalogConformanceRunner implemented
- ✅ CMakeLists.txt updated
- ⏳ Actual rule execution (stubbed - to be implemented in integration)

**Next Steps:**
- Integrate with RulesService for actual execution
- Implement digest computation
- Configure guard thresholds
- Validate against production Datalog engine

## Related Files

- `src/engine/RuleExecutionResult.h` - Result structure
- `src/engine/conformance/DatalogConformanceRunner.cpp` - Test runner
- `src/parser/DatalogParser.h` - Rule parser
- `src/engine/DatalogQueryPlanner.h` - Query planner
- `CMakeLists.txt` - Build configuration

---

**Created:** 2026-01-01
**EPIC:** 5 - Datalog Guards & Hardening
**Task:** 9 - Conformance Corpus & Runner

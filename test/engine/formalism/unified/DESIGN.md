# Unified Formalism Testing Framework - Design Document

**EPIC**: 14.0 Formalism Delta Discovery
**Agent**: Agent 9
**Phase**: Testing Framework Design
**Date**: 2026-01-03

---

## Executive Summary

This document describes the **Unified Formalism Testing Framework**, a comprehensive test infrastructure designed to address critical testing gaps identified in the EPIC 14.0 formalism convergence audit.

The framework provides:
1. **Golden test corpus**: Formalism-agnostic test cases with external reference standards
2. **Determinism test suite**: Rule-level and query-level determinism validation
3. **Negative test corpus**: Explicit invalid input testing with expected error behavior
4. **Cross-formalism equivalence tests**: Semantic equivalence validation across formalisms

---

## Problem Statement

### Critical Gaps Identified in Audit

From `audit/MURA_DELTA_SEVERITY.md`:

#### 🔴 INTEGRITY RISK #1: Determinism Testing Gap (HIGH SEVERITY)

**Gap**: SHACL/Datalog have no explicit rule-level determinism tests; only query-level tests exist in other modules.

**Impact**: Cannot guarantee that SHACL constraint evaluation is deterministic; cannot guarantee that Datalog rule execution is deterministic. Only query-level determinism is verified.

**Evidence**:
- `ShaclComplianceTest.cpp`: No determinism tests (only compliance)
- `DatalogQueryPlannerTest.cpp`: No determinism tests (only planning)
- `DeterminismClassifierTest.cpp`: Tests query features only (not rule execution)

#### 🔴 INTEGRITY RISK #2: Coverage Gap in N3 and Datalog (HIGH SEVERITY)

**Gap**: SHACL has W3C test suite (golden tests); N3 has 2 integration tests; Datalog has 8+ tests but no external standard.

**Impact**: N3 and Datalog lack external validation; SHACL has W3C reference. Cannot verify correctness against external spec for N3/Datalog.

**Evidence**:
- `ShaclComplianceTest.cpp`: References W3C SHACL spec (https://www.w3.org/TR/shacl/)
- `N3IntegrationTest.cpp`: Local tests only
- `DatalogQueryPlannerTest.cpp`: Local tests only

#### 🟡 ACCIDENTAL DELTA #1: Negative Test Coverage Imbalance (MEDIUM)

**Gap**: SHACL has implicit negative cases (violations); N3 has explicit negative corpus (invalid test cases); Datalog has implicit negative cases.

**Impact**: N3 is better documented for error handling; SHACL/Datalog rely on implicit assumptions.

**Evidence**:
- `ShaclComplianceTest.cpp`: Violations expected implicitly
- N3: Explicit `invalid_*.n3` test files with parse errors
- `DatalogQueryPlannerTest.cpp`: No negative test cases documented

#### NEW GAP: Cross-Formalism Equivalence

**Gap**: No tests verify that semantically equivalent operations across different formalisms produce equivalent results.

**Impact**: Cannot validate that SHACL property paths, Datalog recursive rules, and SPARQL queries produce consistent behavior for equivalent semantics.

---

## Solution Architecture

### Design Principles

1. **Formalism-Agnostic Design**: Tests work across SHACL, N3, Datalog, ShEx without formalism-specific assumptions
2. **Determinism-First**: All tests validate deterministic execution at rule level and query level
3. **External Standards**: Golden tests reference W3C specs, RFCs, or academic papers
4. **Explicit Negative Testing**: Invalid inputs documented with expected error patterns
5. **Semantic Equivalence**: Cross-formalism tests validate equivalent behavior

### Framework Components

```
test/engine/formalism/unified/
├── UnifiedFormalismTestFramework.h    # Main framework API
├── DeterminismTestSuite.cpp           # Determinism tests (NEW)
├── corpus/
│   ├── golden/                        # Golden test corpus
│   │   ├── shacl_w3c_core.json       # W3C SHACL tests
│   │   ├── datalog_basic.json        # Datalog golden tests
│   │   ├── n3_basic.json             # N3 Turtle tests
│   │   └── shex_basic.json           # ShEx tests (future)
│   ├── negative/                      # Negative test corpus
│   │   ├── shacl_invalid.json        # SHACL error cases
│   │   ├── datalog_invalid.json      # Datalog error cases
│   │   ├── n3_invalid.json           # N3 error cases
│   │   └── shex_invalid.json         # ShEx error cases (future)
│   └── equivalence/                   # Equivalence test corpus
│       └── cross_formalism.json      # Cross-formalism tests
└── DESIGN.md                          # This document
```

---

## Component Design

### 1. Golden Test Corpus

**Purpose**: Provide external reference standards for validation correctness.

**Format**: JSON with schema validation

**Structure**:
```json
{
  "testId": "unique-identifier",
  "description": "Human-readable description",
  "formalism": "SHACL|N3|Datalog|ShEx",
  "inputFormat": "turtle|jsonld|datalog-text",
  "input": "formalism-specific input",
  "expectedConforms": true|false,
  "expectedViolations": [...],
  "source": "W3C|RFC|custom",
  "sourceUrl": "reference URL",
  "tags": ["category", "feature", ...]
}
```

**Coverage**:
- **SHACL**: W3C SHACL 1.0 Core Constraints (all test cases from W3C suite)
- **Datalog**: Custom golden tests (transitive closure, ancestor, filtering, joins)
- **N3**: Turtle-level parsing (80/20 scope: no formulae/rules/variables)
- **ShEx**: Placeholder for future implementation

**Validation**:
- Schema validation on load
- Cross-reference to external specs (W3C URLs)
- Automated regeneration from upstream sources

### 2. Determinism Test Suite

**Purpose**: Validate rule-level and query-level determinism across all formalisms.

**Test Structure**:
```cpp
TEST_F(DeterminismTestSuite, Formalism_RuleLevelDeterminism_Feature) {
  // 1. Define rule/constraint
  // 2. Execute N times (default: 10)
  // 3. Compute result fingerprints
  // 4. Assert all fingerprints identical
}
```

**Coverage**:

#### SHACL Rule-Level Tests:
- `sh:minCount`, `sh:maxCount` (cardinality)
- `sh:pattern` (regex evaluation)
- `sh:nodeKind` (type checking)
- `sh:node` (recursive shapes)
- `sh:datatype` (literal type validation)

#### Datalog Rule-Level Tests:
- Simple fact inference
- Transitive closure (fixpoint iteration)
- Rules with FILTER expressions
- Multi-predicate joins
- Recursive rule stratification

#### N3 Rule-Level Tests:
- Turtle parsing determinism
- Prefix expansion
- Literal value parsing
- Blank node handling

#### Query-Level Tests:
- SPARQL queries over SHACL validation results
- SPARQL queries over Datalog-derived facts
- End-to-end pipeline tests

**Methodology**:
1. **Fingerprinting**: SHA256 hash of canonical result serialization
2. **Multi-Run Validation**: Execute same operation 10 times, verify identical fingerprints
3. **Cause Detection**: If non-deterministic, identify source (NOW(), RAND(), etc.)

### 3. Negative Test Corpus

**Purpose**: Explicit documentation of invalid inputs with expected error behavior.

**Format**: JSON with error pattern specification

**Structure**:
```json
{
  "testId": "unique-identifier",
  "description": "What is invalid",
  "formalism": "SHACL|N3|Datalog|ShEx",
  "inputFormat": "turtle|jsonld|datalog-text",
  "invalidInput": "malformed input",
  "expectedErrorType": "ParseException|ValidationError|...",
  "expectedErrorPattern": "regex pattern for error message",
  "expectedErrorLine": 42,
  "tags": ["category", ...]
}
```

**Coverage**:

#### SHACL Negative Tests:
- Malformed Turtle syntax
- Invalid constraint values (non-integer `sh:minCount`)
- Invalid `sh:nodeKind` values
- Invalid regex patterns in `sh:pattern`
- Missing targets
- Circular shape references

#### Datalog Negative Tests:
- Missing body separators
- Multiple predicates in head (invalid in Datalog)
- Unsafe variables (head variables not in body)
- Unsafe negation
- Unbounded recursion
- Invalid FILTER syntax
- Empty rule body

#### N3 Negative Tests:
- Invalid escape sequences
- Malformed IRIs
- Unterminated literals
- Undefined prefixes
- Invalid datatypes
- Missing statement terminators
- Conflicting @base declarations

**Validation**:
- Regex pattern matching on error messages
- Optional line number validation
- Error type classification

### 4. Cross-Formalism Equivalence Tests

**Purpose**: Validate semantic equivalence across different formalisms.

**Format**: JSON with multi-formalism inputs

**Structure**:
```json
{
  "testId": "unique-identifier",
  "description": "What equivalence is tested",
  "equivalenceType": "semantic|structural|output",
  "inputs": [
    {
      "formalism": "SHACL",
      "inputFormat": "turtle",
      "input": "SHACL-specific input"
    },
    {
      "formalism": "Datalog",
      "inputFormat": "datalog-text",
      "input": "Datalog-specific input"
    }
  ],
  "dataGraph": "shared RDF data",
  "canonicalResult": {...},
  "tags": [...]
}
```

**Coverage**:
- **Transitive closure**: SHACL property path (`ex:edge+`) vs. Datalog recursion
- **Cardinality**: SHACL `sh:minCount` vs. SPARQL `NOT EXISTS`
- **Filtering**: Datalog `FILTER` vs. SPARQL `FILTER`
- **Pattern matching**: SHACL `sh:pattern` vs. SPARQL `REGEX`
- **Type checking**: SHACL `sh:nodeKind sh:IRI` vs. SPARQL `isIRI()`

**Validation**:
- Semantic equivalence: Both produce same violations
- Output equivalence: Result sets are identical (modulo ordering)
- Structural equivalence: AST structures are isomorphic

---

## API Design

### Core Classes

#### `UnifiedValidationResult`
- Unified result structure across all formalisms
- Maps SHACL violations, N3 compliance issues, Datalog tuples to common schema
- Includes determinism metadata (fingerprint hash)

#### `DeterminismTester`
- Rule-level determinism testing
- Query-level determinism testing
- Multi-run execution with fingerprint comparison

#### `EquivalenceTester`
- Semantic equivalence validation
- Structural equivalence validation
- Output equivalence validation

#### `TestCorpusLoader`
- Load golden, negative, equivalence tests from JSON
- Schema validation
- Corpus structure verification

#### `UnifiedTestExecutor`
- Main test execution interface
- Batch execution for all test types
- Formalism-specific adapters

---

## Test Execution Model

### Test Lifecycle

```
1. LOAD
   ├─ TestCorpusLoader.loadGoldenTests(formalism)
   ├─ Validate JSON schema
   └─ Parse test specifications

2. EXECUTE
   ├─ UnifiedTestExecutor.executeGoldenTest(testCase)
   ├─ Parse formalism input
   ├─ Execute operation
   └─ Capture result + fingerprint

3. VALIDATE
   ├─ Compare result to expected
   ├─ Validate fingerprint consistency
   └─ Report violations/errors

4. REPORT
   ├─ UnifiedValidationResult.toJson()
   └─ Export test report
```

### Determinism Testing Flow

```
1. DEFINE OPERATION
   ├─ Load formalism input (rule/constraint)
   └─ Prepare data graph

2. MULTI-RUN EXECUTION
   ├─ for i in 1..N:
   │   ├─ Execute operation
   │   ├─ Compute result fingerprint
   │   └─ Store fingerprint
   └─ Collect fingerprint sequence

3. FINGERPRINT VALIDATION
   ├─ Check all fingerprints identical
   ├─ If divergent: detect non-determinism cause
   └─ Return DeterminismTestResult

4. REPORT
   ├─ isDeterministic: true|false
   ├─ fingerprintHash: "sha256..."
   ├─ nonDeterminismCause: optional
   └─ Export determinism report
```

### Equivalence Testing Flow

```
1. LOAD EQUIVALENCE TEST
   ├─ Parse multi-formalism inputs
   └─ Prepare shared data graph

2. EXECUTE ALL INPUTS
   ├─ for each formalism input:
   │   ├─ Parse input
   │   ├─ Execute operation
   │   └─ Capture result
   └─ Collect results

3. COMPARE RESULTS
   ├─ Convert to unified format
   ├─ Semantic comparison
   ├─ Output comparison
   └─ Structural comparison

4. REPORT
   ├─ areEquivalent: true|false
   ├─ equivalenceType: "semantic"|"output"|"structural"
   ├─ divergenceDescription: optional
   └─ Export equivalence report
```

---

## Integration with Existing Tests

### Non-Destructive Integration

**Principle**: Do NOT modify existing test suites.

**Strategy**:
- Existing tests remain in `test/engine/shacl/`, `test/engine/datalog/`, etc.
- Unified framework lives in `test/engine/formalism/unified/`
- Unified tests **complement** (not replace) existing tests

### Test Coverage Matrix

| Test Type | SHACL | N3 | Datalog | ShEx | Location |
|-----------|-------|----|---------| -----|----------|
| **Existing Unit Tests** | 13 files | 2 files | 8+ files | 0 | `test/engine/{formalism}/` |
| **Existing W3C Tests** | ✓ | ✗ | ✗ | ✗ | `test/engine/shacl/` |
| **Unified Golden Tests** | ✓ | ✓ | ✓ | stub | `test/engine/formalism/unified/corpus/golden/` |
| **Unified Determinism Tests** | ✓ | ✓ | ✓ | stub | `test/engine/formalism/unified/DeterminismTestSuite.cpp` |
| **Unified Negative Tests** | ✓ | ✓ | ✓ | stub | `test/engine/formalism/unified/corpus/negative/` |
| **Unified Equivalence Tests** | ✓ | N/A | ✓ | stub | `test/engine/formalism/unified/corpus/equivalence/` |

### Integration Points

1. **Existing SHACL tests** (`ShaclComplianceTest.cpp`, `W3CShaclTestSuiteTest.cpp`):
   - Continue to provide W3C-specific validation
   - Unified framework adds determinism and equivalence tests

2. **Existing Datalog tests** (`DatalogQueryPlannerTest.cpp`, `FixpointComputationTest.cpp`):
   - Continue to provide planning and fixpoint logic tests
   - Unified framework adds determinism and golden corpus

3. **Existing N3 tests** (`N3IntegrationTest.cpp`):
   - Continue to provide basic integration tests
   - Unified framework adds golden corpus and explicit negative tests

---

## Test Corpus Management

### Corpus Structure

```
test/engine/formalism/unified/corpus/
├── golden/                    # Reference-standard tests
│   ├── shacl_w3c_core.json   # 8 tests from W3C SHACL spec
│   ├── datalog_basic.json    # 5 tests (transitive, ancestor, filter, join, empty)
│   ├── n3_basic.json         # 5 tests (turtle, prefix, literal, blank, collection)
│   └── shex_basic.json       # Placeholder (future)
│
├── negative/                  # Invalid input tests
│   ├── shacl_invalid.json    # 6 error cases
│   ├── datalog_invalid.json  # 7 error cases
│   ├── n3_invalid.json       # 7 error cases
│   └── shex_invalid.json     # Placeholder (future)
│
└── equivalence/               # Cross-formalism tests
    └── cross_formalism.json  # 5 equivalence tests
```

### Corpus Validation

```cpp
TestCorpusLoader loader("/path/to/corpus");

// Validate all test files have valid JSON schema
bool valid = loader.validateCorpusStructure();
if (!valid) {
  auto errors = loader.getValidationErrors();
  for (const auto& error : errors) {
    std::cerr << "Corpus validation error: " << error << std::endl;
  }
}
```

### Corpus Extension

**Adding new golden tests**:
1. Create JSON test case following schema
2. Add to appropriate `{formalism}_*.json` file
3. Run `validateCorpusStructure()` to verify schema
4. Add GTest case to execute test

**Adding new negative tests**:
1. Document invalid input with error pattern
2. Add to `{formalism}_invalid.json`
3. Run test to verify error pattern matches

**Adding new equivalence tests**:
1. Identify semantically equivalent operations across formalisms
2. Create multi-input test case
3. Add to `cross_formalism.json`
4. Run equivalence validation

---

## Implementation Roadmap

### Phase 1: Framework Foundation (EPIC 14.1)
- ✅ `UnifiedFormalismTestFramework.h` header
- ✅ Golden test corpus (SHACL, Datalog, N3)
- ✅ Negative test corpus (SHACL, Datalog, N3)
- ✅ Equivalence test corpus
- ✅ `DeterminismTestSuite.cpp`
- ⏳ Implementation of core classes (next step)

### Phase 2: Test Execution (EPIC 14.2)
- Implement `TestCorpusLoader`
- Implement `UnifiedTestExecutor`
- Implement formalism-specific adapters
- Integrate with existing test infrastructure

### Phase 3: Determinism Validation (EPIC 14.3)
- Implement `DeterminismTester`
- Implement fingerprint computation utilities
- Execute all determinism tests
- Report determinism violations

### Phase 4: Equivalence Validation (EPIC 14.4)
- Implement `EquivalenceTester`
- Execute cross-formalism equivalence tests
- Report semantic divergences
- Document equivalence boundaries

### Phase 5: Continuous Integration (EPIC 14.5)
- Add unified tests to CI pipeline
- Automated corpus regeneration from W3C suite
- Regression detection on determinism changes
- Test coverage reporting

---

## Metrics and Success Criteria

### Coverage Metrics

| Metric | Baseline (Audit) | Target (Post-Framework) |
|--------|------------------|-------------------------|
| **SHACL golden tests** | 13 (local) + W3C suite | 13 (local) + W3C suite + 8 (unified) |
| **Datalog golden tests** | 27 (local queries) | 27 (local) + 5 (unified) |
| **N3 golden tests** | 2 (integration) | 2 (local) + 5 (unified) |
| **SHACL determinism tests** | 0 (rule-level) | 5 (rule-level) + 2 (query-level) |
| **Datalog determinism tests** | 0 (rule-level) | 3 (rule-level) + 1 (query-level) |
| **N3 determinism tests** | 0 | 1 (parse-level) |
| **Negative test cases** | 0 (SHACL/Datalog), 7 (N3) | 6 (SHACL) + 7 (Datalog) + 7 (N3) |
| **Equivalence tests** | 0 | 5 (cross-formalism) |

### Success Criteria

✅ **Determinism Gap Closed**:
- All formalisms have rule-level determinism tests
- All tests pass with 100% fingerprint consistency

✅ **Coverage Gap Closed**:
- N3 and Datalog have golden test corpus with external references
- All formalisms have explicit negative test cases

✅ **Equivalence Validation**:
- Cross-formalism equivalence tests execute successfully
- Semantic equivalence is validated for all test cases

✅ **Non-Destructive Integration**:
- Existing tests remain unchanged
- Unified tests run in parallel with existing tests
- CI pipeline includes unified tests

---

## References

- **EPIC 14.0 Audit Documents**:
  - `audit/FORMALISM_DELTA_MATRIX.md`
  - `audit/MURA_DELTA_SEVERITY.md`
  - `audit/FORMALISM_BEST_OF.md`

- **W3C Standards**:
  - SHACL 1.0: https://www.w3.org/TR/shacl/
  - N3 Submission: https://www.w3.org/TeamSubmission/n3/

- **Existing Test Files**:
  - `test/engine/shacl/ShaclComplianceTest.cpp`
  - `test/engine/shacl/W3CShaclTestSuiteTest.cpp`
  - `test/engine/DatalogQueryPlannerTest.cpp`
  - `test/engine/queryCanonical/DeterminismClassifierTest.cpp`

---

## Appendix A: JSON Schema Examples

### Golden Test Schema

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["testId", "description", "formalism", "inputFormat", "input", "expectedConforms"],
  "properties": {
    "testId": { "type": "string" },
    "description": { "type": "string" },
    "formalism": { "enum": ["SHACL", "ShEx", "N3", "Datalog"] },
    "inputFormat": { "type": "string" },
    "input": { "type": "string" },
    "expectedConforms": { "type": "boolean" },
    "expectedViolations": { "type": "array" },
    "source": { "type": "string" },
    "sourceUrl": { "type": "string", "format": "uri" },
    "tags": { "type": "array", "items": { "type": "string" } }
  }
}
```

### Negative Test Schema

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["testId", "description", "formalism", "inputFormat", "invalidInput", "expectedErrorType", "expectedErrorPattern"],
  "properties": {
    "testId": { "type": "string" },
    "description": { "type": "string" },
    "formalism": { "enum": ["SHACL", "ShEx", "N3", "Datalog"] },
    "inputFormat": { "type": "string" },
    "invalidInput": { "type": "string" },
    "expectedErrorType": { "type": "string" },
    "expectedErrorPattern": { "type": "string" },
    "expectedErrorLine": { "type": "integer" },
    "tags": { "type": "array", "items": { "type": "string" } }
  }
}
```

---

**Status**: Design Complete — Awaiting Implementation Phase
**Next Phase**: EPIC 14.1 Implementation

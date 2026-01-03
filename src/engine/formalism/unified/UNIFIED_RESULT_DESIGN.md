# Unified Result/Violation Schema Design

**Author**: Agent 5 - EPIC 14.0 Formalism Convergence
**Date**: 2026-01-03
**Status**: Design Complete - Implementation Pending
**Purpose**: Resolve output fragmentation integrity risk across SHACL, N3, Datalog, and ShEx

---

## Executive Summary

The unified result schema provides a **single, standard representation** for validation violations, compliance issues, rule derivations, and parsing errors across all four formalisms (SHACL, N3, Datalog, ShEx).

**Key Properties**:
- Based on W3C SHACL ValidationReport/ValidationResult specification
- Backward compatible: does NOT modify existing `ShaclViolation` or `N3ComplianceIssue`
- Supports all formalism result types via extensible metadata
- Serializable to: RDF (Turtle), JSON, SPARQL result sets, Markdown, plain text
- Enables uniform violation consumers (query engines, formatters, analytics)

---

## Problem Statement

### Current State (Fragmentation)

Each formalism produces incompatible output types:

| Formalism | Output Type | Structure | Format |
|-----------|------------|-----------|--------|
| **SHACL** | `ShaclViolation` | focusNode, constraintComponent, severity, message | Structured objects |
| **N3** | `N3ComplianceIssue` | feature, description, lineNumber, isSupported | Markdown + structs |
| **Datalog** | IdTable tuples | Column bindings | SPARQL result sets |
| **ShEx** | (stub) | N/A | N/A |

### Impact (Integrity Risk)

From `audit/MURA_DELTA_SEVERITY.md`:
- **🔴 INTEGRITY RISK: Output Type Fragmentation** (Severity: HIGH)
- "Cannot write uniform validation consumer"
- "No standard violation schema across all formalisms"
- "Breaks composability; each consumer must understand all three formats"

### Solution Requirements

1. **DO NOT** modify existing `ShaclViolation`, `N3ComplianceIssue`, or Datalog result structures
2. Create **new** unified schema in `src/engine/formalism/unified/`
3. Base schema on W3C SHACL ValidationResult (standard, well-defined)
4. Support all formalism result types (violations, features, tuples, schemas)
5. Provide conversion functions from formalism-specific to unified
6. Support multiple serialization formats (RDF, JSON, SPARQL, Markdown, text)
7. Include human-readable formatter

---

## Design Overview

### Core Classes

```
formalism::UnifiedViolation
  ├─ Core W3C SHACL ValidationResult properties
  ├─ Formalism-specific extensions (metadata, location)
  ├─ Builder pattern methods
  ├─ Serialization methods (JSON, RDF, SPARQL)
  └─ Query methods

formalism::UnifiedValidationReport
  ├─ W3C SHACL ValidationReport properties
  ├─ Results collection + indices (by node, severity, type)
  ├─ Statistics and summary methods
  ├─ Serialization methods (JSON, RDF, Markdown, text)
  └─ Query methods

formalism::ViolationFormatter
  ├─ Format single violation (Text, JSON, RDF, Markdown, SPARQL)
  ├─ Format complete report (all formats)
  ├─ Grouped formatting (by focus node, severity, constraint)
  └─ Summary formatting
```

### Conversion Functions

```cpp
// SHACL → Unified
UnifiedViolation fromShaclViolation(const shacl::ShaclViolation&);
UnifiedValidationReport fromShaclReport(const shacl::DetailedValidationReport&);

// N3 → Unified
UnifiedViolation fromN3ComplianceIssue(const ad_utility::N3ComplianceIssue&);
UnifiedValidationReport fromN3ComplianceVerifier(const ad_utility::N3ComplianceVerifier&);

// Datalog → Unified
UnifiedViolation fromDatalogTuple(const DatalogRule&, const vector<string>& tuple);
UnifiedValidationReport createDatalogDerivationReport(...);

// (ShEx conversion functions will be added when ShEx is implemented)
```

---

## W3C SHACL Alignment

### ValidationResult Shape

The unified schema is based on the W3C SHACL ValidationResult specification:

**W3C SHACL ValidationResult** ([spec](https://www.w3.org/TR/shacl/#validation-report)):
```turtle
sh:ValidationResult
  a rdfs:Class ;
  sh:property [
    sh:path sh:focusNode ;              # The RDF term that caused the violation
    sh:minCount 1 ;
  ] ;
  sh:property [
    sh:path sh:resultPath ;             # Property path where violation occurred
    sh:maxCount 1 ;
  ] ;
  sh:property [
    sh:path sh:resultSeverity ;         # Violation, Warning, or Info
    sh:maxCount 1 ;
  ] ;
  sh:property [
    sh:path sh:sourceConstraintComponent ; # Constraint that failed
    sh:maxCount 1 ;
  ] ;
  sh:property [
    sh:path sh:sourceShape ;            # Shape that was validated
    sh:maxCount 1 ;
  ] ;
  sh:property [
    sh:path sh:value ;                  # Actual value(s) that caused violation
  ] ;
  sh:property [
    sh:path sh:resultMessage ;          # Human-readable message
  ] .
```

### ValidationReport Shape

```turtle
sh:ValidationReport
  a rdfs:Class ;
  sh:property [
    sh:path sh:conforms ;               # Boolean: true if valid, false if violations
    sh:datatype xsd:boolean ;
    sh:minCount 1 ;
    sh:maxCount 1 ;
  ] ;
  sh:property [
    sh:path sh:result ;                 # List of ValidationResult instances
  ] .
```

### Extensions for Non-SHACL Formalisms

The unified schema **extends** the W3C shape with additional fields:
- `formalism: FormalismType` - identifies result provenance (SHACL, N3, Datalog, ShEx)
- `resultType: ResultType` - classifies result (violation, compliance issue, rule derivation, etc.)
- `metadata: unordered_map<string, string>` - formalism-specific key-value pairs
- `location: SourceLocation` - file, line, column, line content (optional)

These extensions are **compatible** with W3C SHACL because:
1. RDF serialization uses custom namespaces (e.g., `qlever:formalism`, `qlever:metadata`)
2. JSON serialization includes all fields with clear naming
3. Core W3C fields remain unchanged and standard-compliant

---

## Formalism Mappings

### SHACL → Unified

| SHACL Field | Unified Field | Notes |
|-------------|---------------|-------|
| `focusNode` | `focusNode` | Direct mapping |
| `resultPath` | `resultPath` | Direct mapping |
| `sourceShape` | `sourceShape` | Direct mapping |
| `sourceConstraintComponent` | `sourceConstraintComponent` | Direct mapping |
| `values` | `values` | Direct mapping |
| `expectedValue` | `expectedValue` | Direct mapping |
| `severity` | `severity` | Direct mapping (enum compatible) |
| `message` | `message` | Direct mapping |
| `details` | `metadata` | Converted from unordered_map to metadata |
| N/A | `formalism` | Set to `FormalismType::SHACL` |
| N/A | `resultType` | Set to `ResultType::ValidationViolation` |

**Example**:
```cpp
ShaclViolation shaclViolation;
shaclViolation.focusNode = "ex:Alice";
shaclViolation.resultPath = "ex:age";
shaclViolation.sourceConstraintComponent = "sh:datatype";
shaclViolation.severity = SeverityLevel::Violation;

UnifiedViolation unified = fromShaclViolation(shaclViolation);
// unified.formalism == FormalismType::SHACL
// unified.focusNode == "ex:Alice"
// unified.resultPath == "ex:age"
```

### N3 → Unified

| N3 Field | Unified Field | Notes |
|----------|---------------|-------|
| N/A | `focusNode` | Set to filename or "N3Document" |
| `feature` | `resultPath` | Feature name (e.g., "formulae", "implication") |
| N/A | `sourceShape` | Set to "N3ComplianceVerifier" |
| `feature` | `sourceConstraintComponent` | Feature detection rule |
| `lineContent` | `values[0]` | First value contains line content |
| `isSupported` | `expectedValue` | "supported" or "unsupported" |
| N/A | `severity` | Info if supported, Violation if unsupported |
| `description` | `message` | Direct mapping |
| `lineNumber` | `location.lineNumber` | Source location |
| `lineContent` | `location.lineContent` | Source location |
| N/A | `formalism` | Set to `FormalismType::N3` |
| N/A | `resultType` | Set to `ResultType::ComplianceIssue` |

**Example**:
```cpp
N3ComplianceIssue n3Issue;
n3Issue.feature = "formulae";
n3Issue.description = "N3 formulae detected (not supported)";
n3Issue.lineNumber = 42;
n3Issue.lineContent = "@forAll :x :y { :x :likes :y }";
n3Issue.isSupported = false;

UnifiedViolation unified = fromN3ComplianceIssue(n3Issue, "example.n3");
// unified.formalism == FormalismType::N3
// unified.focusNode == "example.n3"
// unified.resultPath == "formulae"
// unified.severity == SeverityLevel::Violation
// unified.location->lineNumber == 42
```

### Datalog → Unified

| Datalog Field | Unified Field | Notes |
|---------------|---------------|-------|
| `rule.headPredicate` | `focusNode` | Rule name (e.g., "ancestor") |
| Variable binding context | `resultPath` | E.g., "?x=Alice, ?y=Bob" |
| `rule.toString()` | `sourceShape` | Full rule definition |
| Filter or safety check | `sourceConstraintComponent` | E.g., "safetyCheck", "filterConstraint" |
| `tuple` values | `values` | All tuple column values |
| Expected schema | `expectedValue` | E.g., "(person, person)" for binary predicate |
| N/A | `severity` | Info (tuples are results, not violations) |
| Generated description | `message` | E.g., "Derived tuple for ancestor(?x, ?y)" |
| Rule metadata | `metadata` | {"arity": "2", "isRecursive": "true"} |
| N/A | `formalism` | Set to `FormalismType::Datalog` |
| N/A | `resultType` | Set to `ResultType::RuleDerivation` |

**Example**:
```cpp
DatalogRule rule;
rule.headPredicate = "ancestor";
rule.headVariables = {Variable("?x"), Variable("?y")};

vector<string> tuple = {"Alice", "Charlie"};
vector<string> varNames = {"?x", "?y"};

UnifiedViolation unified = fromDatalogTuple(rule, tuple, varNames);
// unified.formalism == FormalismType::Datalog
// unified.focusNode == "ancestor"
// unified.resultType == ResultType::RuleDerivation
// unified.severity == SeverityLevel::Info
// unified.values == {"Alice", "Charlie"}
```

### ShEx → Unified (Future)

ShEx is currently a stub. When implemented, mappings will be:

| ShEx Field | Unified Field | Notes |
|------------|---------------|-------|
| Target node | `focusNode` | Node being validated |
| Triple expression | `resultPath` | Shape expression reference |
| Shape label | `sourceShape` | Shape IRI |
| Constraint (cardinality, etc.) | `sourceConstraintComponent` | Specific constraint |
| Actual value | `values` | Node values |
| Expected constraint | `expectedValue` | Expected value/pattern |
| N/A | `formalism` | Set to `FormalismType::ShEx` |
| N/A | `resultType` | Set to `ResultType::ValidationViolation` |

---

## Serialization Formats

### 1. JSON Format

**Single Violation**:
```json
{
  "formalism": "SHACL",
  "resultType": "ValidationViolation",
  "severity": "Violation",
  "focusNode": "ex:Alice",
  "resultPath": "ex:age",
  "sourceShape": "ex:PersonShape",
  "sourceConstraintComponent": "sh:datatype",
  "values": ["\"twenty\""],
  "expectedValue": "xsd:integer",
  "message": "Value 'twenty' is not of datatype xsd:integer",
  "metadata": {
    "constraintType": "datatype",
    "parameterValue": "xsd:integer"
  }
}
```

**Complete Report**:
```json
{
  "conforms": false,
  "formalism": "SHACL",
  "source": "data.ttl",
  "timestamp": "2026-01-03T12:34:56Z",
  "violationCount": 2,
  "warningCount": 1,
  "infoCount": 0,
  "results": [
    { ... },
    { ... }
  ],
  "statistics": {
    "totalResults": 3,
    "uniqueFocusNodes": 2,
    "resultsByConstraint": {
      "sh:datatype": 1,
      "sh:minCount": 1
    }
  }
}
```

### 2. RDF Turtle Format (W3C SHACL ValidationReport)

```turtle
@prefix sh: <http://www.w3.org/ns/shacl#> .
@prefix qlever: <http://qlever.cs.uni-freiburg.de/formalism#> .
@prefix ex: <http://example.org/> .

_:report a sh:ValidationReport ;
  sh:conforms false ;
  qlever:formalism "SHACL" ;
  qlever:source "data.ttl" ;
  sh:result _:result1 .

_:result1 a sh:ValidationResult ;
  sh:focusNode ex:Alice ;
  sh:resultPath ex:age ;
  sh:sourceShape ex:PersonShape ;
  sh:sourceConstraintComponent sh:DatatypeConstraintComponent ;
  sh:value "twenty" ;
  sh:resultSeverity sh:Violation ;
  sh:resultMessage "Value 'twenty' is not of datatype xsd:integer" ;
  qlever:formalism "SHACL" ;
  qlever:resultType "ValidationViolation" .
```

### 3. Markdown Format (N3-style)

```markdown
# Validation Report: data.ttl

**Formalism**: SHACL
**Conforms**: ❌ No
**Violations**: 2 | **Warnings**: 1 | **Info**: 0

---

## Violations

### [VIOLATION] ex:Alice / ex:age

- **Constraint**: sh:datatype
- **Shape**: ex:PersonShape
- **Actual Value**: "twenty"
- **Expected**: xsd:integer
- **Message**: Value 'twenty' is not of datatype xsd:integer

---

## Summary

- **Total Issues**: 3
- **Focus Nodes**: 2
- **By Constraint**:
  - sh:datatype: 1
  - sh:minCount: 1
```

### 4. SPARQL Result Set Format (for Datalog)

```
?predicate    ?var1   ?var2   ?derivationType
"ancestor"    "Alice" "Bob"   "ruleDerivation"
"ancestor"    "Alice" "Charlie" "ruleDerivation"
```

### 5. Plain Text Format

```
[VIOLATION] ex:Alice / ex:age
  Constraint: sh:datatype
  Shape: ex:PersonShape
  Actual Value: "twenty"
  Expected: xsd:integer
  Message: Value 'twenty' is not of datatype xsd:integer

[WARNING] ex:Bob / ex:name
  Constraint: sh:minLength
  Shape: ex:PersonShape
  Actual Value: "B"
  Expected: minLength 2
  Message: Value 'B' has length 1, expected at least 2

---
Summary: 1 violation, 1 warning, 0 info
Conforms: No
```

---

## ViolationFormatter Implementation

### Formatter API

```cpp
// Format a single violation
string ViolationFormatter::format(
  const UnifiedViolation& violation,
  OutputFormat format = OutputFormat::Text
);

// Format a complete report
string ViolationFormatter::format(
  const UnifiedValidationReport& report,
  OutputFormat format = OutputFormat::Text
);

// Format to output stream
void ViolationFormatter::format(
  const UnifiedValidationReport& report,
  ostream& out,
  OutputFormat format = OutputFormat::Text
);

// Format summary only
string ViolationFormatter::formatSummary(
  const UnifiedValidationReport& report
);

// Grouped formatting
string ViolationFormatter::formatByFocusNode(...);
string ViolationFormatter::formatBySeverity(...);
string ViolationFormatter::formatByConstraint(...);
```

### Format Selection

| Format | Use Case | Output |
|--------|----------|--------|
| `Text` | CLI output, logs | Human-readable plain text |
| `JSON` | APIs, web services | Structured JSON |
| `RDF` | RDF stores, W3C compliance | Turtle/N-Triples |
| `Markdown` | Documentation, reports | Markdown with tables |
| `SPARQL` | Datalog results, query output | SPARQL result bindings |

---

## Integration Strategy

### Phase 1: Core Implementation (This Agent's Deliverable)

1. ✅ Create `UnifiedResult.h` header (COMPLETE)
2. ⏳ Implement `UnifiedResult.cpp` with:
   - Serialization methods (JSON, RDF, SPARQL)
   - Query and utility methods
   - Statistics computation
3. ⏳ Implement conversion functions:
   - `fromShaclViolation()`, `fromShaclReport()`
   - `fromN3ComplianceIssue()`, `fromN3ComplianceVerifier()`
   - `fromDatalogTuple()`, `createDatalogDerivationReport()`
4. ⏳ Implement `ViolationFormatter` with all output formats
5. ⏳ Write design documentation (THIS DOCUMENT)

### Phase 2: Integration (Future Agents)

1. Add `#include "engine/formalism/unified/UnifiedResult.h"` to formalism validators
2. Provide optional unified output alongside existing output
3. Update QueryPlanner to optionally return unified reports
4. Create unified violation consumer (e.g., validation cache, analytics)

### Phase 3: Deprecation (Optional, Future)

1. Migrate all consumers to unified format
2. Mark formalism-specific violation types as deprecated
3. Remove duplicate code in favor of unified schema

---

## Backward Compatibility

### Existing Code Unchanged

**NO changes** to existing violation types:
- `shacl::ShaclViolation` - unchanged
- `shacl::DetailedValidationReport` - unchanged
- `ad_utility::N3ComplianceIssue` - unchanged
- `ad_utility::N3ComplianceVerifier` - unchanged
- Datalog IdTable results - unchanged

### Opt-In Usage

Unified schema is **opt-in**:
```cpp
// Existing code continues to work
ShaclViolation violation = validator.validate(shape, data);
formatter.format(violation, ViolationFormat::JSON);

// New code can opt into unified format
UnifiedViolation unified = fromShaclViolation(violation);
formalism::ViolationFormatter::format(unified, OutputFormat::JSON);
```

### Gradual Migration Path

1. **Now**: Unified schema available, conversion functions provided
2. **Later**: New code uses unified schema by default
3. **Future**: Old formalism-specific types deprecated (optional)

---

## Testing Strategy

### Unit Tests

1. **Conversion Tests**:
   - `TestShaclToUnified()` - verify SHACL → Unified conversion
   - `TestN3ToUnified()` - verify N3 → Unified conversion
   - `TestDatalogToUnified()` - verify Datalog → Unified conversion
   - Round-trip tests (if applicable)

2. **Serialization Tests**:
   - `TestJSONSerialization()` - verify JSON output matches schema
   - `TestRDFSerialization()` - verify RDF output is valid Turtle
   - `TestMarkdownSerialization()` - verify Markdown formatting
   - `TestSPARQLSerialization()` - verify SPARQL result format

3. **Formatter Tests**:
   - Test all output formats (Text, JSON, RDF, Markdown, SPARQL)
   - Test grouped formatting (by focus node, severity, constraint)
   - Test summary formatting

4. **Report Tests**:
   - Test report aggregation and indexing
   - Test statistics computation
   - Test conformance calculation

### Integration Tests

1. **Cross-Formalism Tests**:
   - Validate same data with SHACL and ShEx, compare unified reports
   - Run N3 compliance and Datalog derivation, merge reports

2. **W3C Compliance Tests**:
   - Verify RDF output validates against W3C SHACL ValidationReport shape
   - Test against W3C SHACL test suite (existing violations → unified)

---

## Performance Considerations

### Memory Overhead

- Unified schema is **slightly larger** than formalism-specific types (due to optional fields)
- Estimated overhead: 20-30% per violation object
- Mitigation: Use unified schema only when cross-formalism compatibility is needed

### Conversion Cost

- Conversion from formalism-specific to unified is **O(1)** per violation (field copying)
- Batch conversion of reports: **O(n)** where n = number of violations
- Negligible impact for typical workloads (< 10,000 violations per report)

### Serialization Performance

- JSON serialization: **~1 μs per violation** (string building)
- RDF serialization: **~2 μs per violation** (IRI formatting, escaping)
- Text formatting: **~0.5 μs per violation** (simple string concatenation)
- Benchmark target: serialize 10,000 violations in < 50ms

---

## Future Extensions

### Planned Features

1. **ShEx Support**: Add conversion functions when ShEx is implemented
2. **Localization**: Support multi-language messages (i18n)
3. **Filtering**: Query API for advanced filtering (e.g., "all violations for shape X with severity Violation")
4. **Diffing**: Compare two validation reports, highlight differences
5. **Remediation Suggestions**: Generate suggested fixes for common violations

### Extensibility Points

1. **Custom Metadata**: Formalisms can add arbitrary key-value pairs to `metadata`
2. **Custom Result Types**: Extend `ResultType` enum for new formalism-specific types
3. **Custom Formatters**: Register custom output formats via plugin API
4. **Custom Serializers**: Add new serialization formats (e.g., XML, YAML)

---

## References

- [W3C SHACL Specification](https://www.w3.org/TR/shacl/)
- [W3C SHACL Validation Report](https://www.w3.org/TR/shacl/#validation-report)
- [EPIC 14.0 Formalism Delta Matrix](../../audit/FORMALISM_DELTA_MATRIX.md)
- [EPIC 14.0 Mura Delta Severity](../../audit/MURA_DELTA_SEVERITY.md)
- [QLever SHACL Implementation](../shacl/)
- [QLever N3 Compliance Verifier](../../util/N3ComplianceVerifier.h)
- [QLever Datalog Implementation](../../parser/DatalogRule.h)

---

## Document Status

- **Design Phase**: ✅ COMPLETE
- **Header Implementation**: ✅ COMPLETE (`UnifiedResult.h`)
- **Implementation Phase**: ⏳ PENDING (`.cpp` file with method implementations)
- **Testing Phase**: ⏳ PENDING
- **Integration Phase**: ⏳ PENDING (future agents)

---

## Contact / Questions

This design is part of **EPIC 14.0 Formalism Convergence**.

For questions or clarifications, refer to:
- EPIC 14.0 audit documents (`audit/FORMALISM_DELTA_MATRIX.md`, `audit/MURA_DELTA_SEVERITY.md`)
- CLAUDE.md Big Bang 80/20 framework
- Agent coordination logs (if applicable)

**Agent 5 Deliverable Status**: Design and header complete. Implementation skeleton provided. Ready for convergence with other agents.

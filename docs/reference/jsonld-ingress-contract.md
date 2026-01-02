# JSON-LD Ingress Contract for Rule & Constraint Languages

**Document Version:** 1.0
**EPIC:** 10.1 - JSON-LD-Only Ingress Normalization
**Status:** SPECIFICATION CLOSED
**Author:** Agent 6 - JSON-LD Ingress Normalization
**Date:** 2026-01-02

---

## Overview

This document specifies the JSON-LD subset accepted by QLever's ingress normalizer for rule and constraint languages (SHACL, ShEx, N3, Datalog). The specification is **deterministic** and **epoch-bound** to ensure reproducible cache invalidation and fail-closed security.

---

## 1. Supported Dialects

The JSON-LD ingress normalizer accepts **only** the following rule and constraint languages:

| Dialect  | Format      | Namespace/Context Required                       | Rejection at Ingress |
|----------|-------------|--------------------------------------------------|----------------------|
| SHACL    | JSON-LD     | `http://www.w3.org/ns/shacl#` or `sh:` prefix    | ✅ Turtle, N-Triples, RDF/XML rejected |
| ShEx     | JSON-LD     | `http://www.w3.org/ns/shex#` or `shex:` prefix   | ✅ Turtle, N-Triples, RDF/XML rejected |
| N3       | JSON-LD     | `http://www.w3.org/2000/10/swap/log#` or `log:`/`n3:` prefix | ✅ Turtle, N-Triples, RDF/XML rejected |
| Datalog  | JSON-LD     | Custom context with `rules`, `head`, `body`      | ✅ Turtle, N-Triples, RDF/XML rejected |

**Non-JSON-LD formats are rejected at ingress with error code `UNSUPPORTED_FORMAT`.**

---

## 2. JSON-LD Subset (Structural Requirements)

### 2.1 Valid JSON Structure

All inputs MUST be valid JSON (RFC 8259):
- UTF-8 encoding (UTF-16/UTF-32 rejected)
- No trailing commas
- No comments (JSON does not support comments)
- No NaN, Infinity, or -Infinity (only finite numbers)

### 2.2 Required JSON-LD Fields

Every JSON-LD document MUST contain:
- `@context`: String, object, or array defining namespace bindings
- `@type`: String or array specifying RDF type(s)

Optional fields:
- `@id`: IRI identifier for the resource
- `@graph`: Array of graph nodes
- `@value`: Literal value (for typed literals)
- `@language`: Language tag (for language-tagged strings)

### 2.3 Rejected Constructs

The following are **NOT supported** (fail with `UNSUPPORTED_FORMAT` or `PARSE_ERROR_SYNTAX`):
- JSON-LD framing (`@frame`)
- JSON-LD remote contexts (all contexts must be embedded)
- JSON-LD 1.1 `@included` (only 1.0 core features)
- Recursive `@context` references (circular dependencies)

---

## 3. Dialect-Specific JSON-LD Requirements

### 3.1 SHACL (Shapes Constraint Language)

**Context Requirements:**
- MUST include SHACL namespace: `http://www.w3.org/ns/shacl#`
- Recommended prefix: `sh:`

**Structural Requirements:**
- Root MUST be object or array of objects
- Each shape MUST have `@type` = `sh:NodeShape` or `sh:PropertyShape`
- Node shapes MUST have `sh:targetClass`, `sh:targetNode`, or `sh:targetSubjectsOf`
- Property shapes MUST have `sh:path`

**Example:**
```json
{
  "@context": {
    "sh": "http://www.w3.org/ns/shacl#",
    "ex": "http://example.org/"
  },
  "@id": "ex:PersonShape",
  "@type": "sh:NodeShape",
  "sh:targetClass": "ex:Person",
  "sh:property": {
    "@type": "sh:PropertyShape",
    "sh:path": "ex:name",
    "sh:datatype": "xsd:string",
    "sh:minCount": 1
  }
}
```

**Validation:**
- Missing `sh:` namespace → `JSONLD_MISSING_CONTEXT`
- Missing shape type → `VALIDATION_SHAPE_VIOLATION`

---

### 3.2 ShEx (Shape Expressions)

**Context Requirements:**
- MUST include ShEx namespace: `http://www.w3.org/ns/shex#`
- Recommended prefix: `shex:`

**Structural Requirements:**
- Root MUST contain `shapes` array or single `Shape` object
- Each shape MUST have `@type` = `Shape` or `NodeConstraint`

**Example:**
```json
{
  "@context": {
    "shex": "http://www.w3.org/ns/shex#",
    "ex": "http://example.org/"
  },
  "shapes": [
    {
      "@id": "ex:PersonShape",
      "@type": "Shape",
      "expression": {
        "@type": "TripleConstraint",
        "predicate": "ex:name",
        "valueExpr": {"@type": "NodeConstraint", "datatype": "xsd:string"}
      }
    }
  ]
}
```

**Validation:**
- Missing `shex:` namespace → `JSONLD_MISSING_CONTEXT`
- Missing `shapes` → `VALIDATION_FAILED`

---

### 3.3 N3 (Notation3 Rules)

**Context Requirements:**
- MUST include N3 namespace: `http://www.w3.org/2000/10/swap/log#`
- Recommended prefix: `log:` or `n3:`

**Structural Requirements:**
- Root MUST contain `log:implies` or `rules` array
- Each rule MUST have premise and conclusion

**Example:**
```json
{
  "@context": {
    "log": "http://www.w3.org/2000/10/swap/log#",
    "ex": "http://example.org/"
  },
  "rules": [
    {
      "@type": "log:Implication",
      "log:implies": {
        "premise": "?x ex:parent ?y",
        "conclusion": "?y ex:child ?x"
      }
    }
  ]
}
```

**Validation:**
- Missing `log:` or `n3:` namespace → `JSONLD_MISSING_CONTEXT`
- Missing `log:implies` or `rules` → `VALIDATION_FAILED`

---

### 3.4 Datalog

**Context Requirements:**
- Custom context (no standard namespace)
- MUST include `rules` array

**Structural Requirements:**
- Root MUST contain `rules` array
- Each rule MUST have `head` (string) and `body` (array of strings)

**Example:**
```json
{
  "@context": "http://example.org/datalog",
  "rules": [
    {
      "head": "ancestor(?x, ?y)",
      "body": ["parent(?x, ?y)"]
    },
    {
      "head": "ancestor(?x, ?z)",
      "body": ["parent(?x, ?y)", "ancestor(?y, ?z)"]
    }
  ]
}
```

**Validation:**
- Missing `rules` → `VALIDATION_FAILED`
- Missing `head` or `body` → `VALIDATION_FAILED`

---

## 4. Normalization Rules (Determinism Guarantees)

### 4.1 Canonical JSON Transformation

The ingress normalizer applies the following transformations to ensure deterministic output:

1. **Alphabetical Key Ordering**
   - All object keys sorted alphabetically (lexicographic, case-sensitive)
   - Applied recursively at all nesting levels

2. **UTF-8 NFC Normalization**
   - All strings normalized to Unicode NFC (Canonical Composition)
   - Ensures byte-for-byte reproducibility across platforms

3. **Whitespace Removal**
   - Compact JSON output (no indentation, no extra spaces)
   - Format: `{"key":"value"}` (not `{ "key" : "value" }`)

4. **Number Canonicalization**
   - Integers: minimal representation (no leading zeros)
   - Floats: minimal representation (no trailing zeros)
   - Scientific notation: normalized to minimal form

5. **Escape Sequence Normalization**
   - Unicode escapes (`\uXXXX`) minimized where possible
   - Control characters escaped consistently

### 4.2 Digest Computation (Epoch-Bound)

Digest is computed as:

```
SHA256(canonical_json || epoch_id || guard_identity_hash || validation_mask)
```

Where:
- `canonical_json`: normalized JSON-LD output
- `epoch_id`: 8-byte big-endian epoch identifier
- `guard_identity_hash`: 8-byte big-endian guard configuration hash
- `validation_mask`: 4-byte big-endian SIMD validation bitmask

**Determinism Guarantee:**
- Same input + same epoch + same guards → **same digest (100% reproducible)**
- Different epochs → **different digests (cache invalidation)**

---

## 5. Guard Configuration (Bounded Compute)

### 5.1 Default Guards

| Guard                  | Default Value | Error Code if Exceeded       |
|------------------------|---------------|------------------------------|
| `max_input_size_bytes` | 100 MB        | `BUFFER_OVERFLOW`            |
| `max_nesting_depth`    | 100 levels    | `PARSE_ERROR_NESTED_DEPTH`   |
| `max_object_keys`      | 10,000 keys   | `RESOURCE_EXHAUSTED`         |
| `max_string_length_bytes` | 1 MB       | `VALIDATION_LENGTH_ERROR`    |
| `timeout_ms`           | 30,000 ms     | `TIMEOUT`                    |

### 5.2 Guard Enforcement Policy

**Fail-Closed Behavior:**
- Any guard violation → **immediate error return** (no partial processing)
- No exceptions thrown (all errors returned via error codes)
- No logging in hot-path (silent failure for performance)

**Configurable Guards:**
Users can override defaults via `IngressGuardConfig`:

```cpp
IngressGuardConfig guards;
guards.max_input_size_bytes = 50 * 1024 * 1024;  // 50 MB
guards.max_nesting_depth = 50;  // 50 levels
guards.guard_identity_hash = 12345;  // Custom hash
```

---

## 6. Error Codes Reference

### 6.1 Dialect Rejection Errors

| Error Code            | Description                              | Dialect Rejected   |
|-----------------------|------------------------------------------|--------------------|
| `UNSUPPORTED_FORMAT`  | Input is not JSON-LD                     | Turtle, N-Triples, RDF/XML |
| `PARSE_ERROR_SYNTAX`  | JSON syntax invalid                      | Malformed JSON     |
| `PARSE_ERROR_EMPTY_INPUT` | Empty input stream                   | N/A                |

### 6.2 JSON-LD Validation Errors

| Error Code                  | Description                              |
|-----------------------------|------------------------------------------|
| `JSONLD_MISSING_CONTEXT`    | No `@context` field or wrong namespace   |
| `JSONLD_INVALID_CONTEXT`    | `@context` value not valid               |
| `JSONLD_MISSING_ID`         | `@id` field missing (when required)      |
| `JSONLD_INVALID_ID`         | `@id` not valid IRI                      |

### 6.3 Guard Enforcement Errors

| Error Code                  | Description                              |
|-----------------------------|------------------------------------------|
| `BUFFER_OVERFLOW`           | Input exceeds `max_input_size_bytes`     |
| `PARSE_ERROR_NESTED_DEPTH`  | Nesting exceeds `max_nesting_depth`      |
| `RESOURCE_EXHAUSTED`        | Object keys exceed `max_object_keys`     |
| `VALIDATION_LENGTH_ERROR`   | String exceeds `max_string_length_bytes` |
| `TIMEOUT`                   | Parsing exceeds `timeout_ms`             |

### 6.4 Validation Errors (Dialect-Specific)

| Error Code                  | Description                              |
|-----------------------------|------------------------------------------|
| `VALIDATION_SHAPE_VIOLATION`| SHACL shape missing required fields      |
| `VALIDATION_FAILED`         | Generic validation failure               |

---

## 7. Compatibility Matrix

| Feature                     | SHACL | ShEx | N3 | Datalog | Status       |
|-----------------------------|-------|------|----|---------|--------------|
| JSON-LD 1.0 Core            | ✅    | ✅   | ✅ | ✅      | Supported    |
| JSON-LD 1.1 Extended        | ❌    | ❌   | ❌ | ❌      | Not Supported|
| Turtle Format               | ❌    | ❌   | ❌ | ❌      | Rejected     |
| N-Triples Format            | ❌    | ❌   | ❌ | ❌      | Rejected     |
| RDF/XML Format              | ❌    | ❌   | ❌ | ❌      | Rejected     |
| Remote Contexts             | ❌    | ❌   | ❌ | ❌      | Not Supported|
| Framing                     | ❌    | ❌   | ❌ | ❌      | Not Supported|
| Epoch Binding               | ✅    | ✅   | ✅ | ✅      | Required     |
| Guard Enforcement           | ✅    | ✅   | ✅ | ✅      | Required     |
| Deterministic Normalization | ✅    | ✅   | ✅ | ✅      | Guaranteed   |

---

## 8. Testing Requirements

### 8.1 Determinism Tests

All dialects MUST pass:
- **100-iteration determinism test**: Same input → same digest 100 times
- **Key ordering test**: Different key orders → same normalized output
- **Epoch binding test**: Different epochs → different digests

### 8.2 Rejection Tests

All dialects MUST reject:
- Turtle format (starting with `@prefix` or `@base`)
- N-Triples format (pattern `<...> <...> ... .`)
- RDF/XML format (starting with `<?xml` or `<rdf:RDF`)

### 8.3 Guard Enforcement Tests

All dialects MUST enforce:
- `max_input_size_bytes`: Oversized input → `BUFFER_OVERFLOW`
- `max_nesting_depth`: Deep nesting → `PARSE_ERROR_NESTED_DEPTH`
- Fail-closed behavior: No partial processing on guard violation

---

## 9. Version History

| Version | Date       | Changes                              | Author   |
|---------|------------|--------------------------------------|----------|
| 1.0     | 2026-01-02 | Initial specification closure        | Agent 6  |

---

## 10. References

- [JSON-LD 1.0 Specification](https://www.w3.org/TR/json-ld/)
- [SHACL Specification](https://www.w3.org/TR/shacl/)
- [ShEx Specification](http://shex.io/shex-semantics/)
- [N3 Specification](https://www.w3.org/TeamSubmission/n3/)
- [Datalog (QLever Custom)](docs/datalog/DATALOG_SYNTAX.md)
- [EPIC 10 Specification](EPIC10_CRITICAL_PATH_AND_DEPENDENCIES.md)
- [Epoch Identity System](src/global/Epoch.h)

---

**END OF CONTRACT**

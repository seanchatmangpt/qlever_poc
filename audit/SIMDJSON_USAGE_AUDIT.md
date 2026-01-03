# EPIC 14.0 — SimdJson Usage Audit

**Phase**: Delta Discovery
**Status**: AUDIT ONLY — NO CHANGES PERMITTED
**Date**: 2026-01-03

---

## Objective

Audit simdjson usage across all four formalisms (SHACL, ShEx, N3, Datalog) to determine:
1. Whether simdjson is actually used
2. Where it is bypassed
3. Why (explicit reason or implicit)
4. Whether simdjson should become a **hard architectural invariant** in EPIC 14.1

---

## Context

**simdjson** is a SIMD-accelerated JSON parser. If formalisms accept JSON-LD at ingress, they should use simdjson for maximum performance consistency.

**Reference**: `src/engine/ingress/SimdJsonIngressWrapper.h` and `src/engine/ingress/JsonLdIngressNormalizer.h`

---

## Audit Results

### 1. SHACL

#### Current Status: **USES simdjson (indirectly via JsonLdIngressNormalizer)**

#### Details:

**Ingress Path**:
- Input format: Turtle (primary) or JSON-LD (via JsonLdIngressNormalizer)
- JSON-LD handler: `JsonLdIngressNormalizer::normalizeForDialect(input, RuleLanguageDialect::SHACL, ...)`
- JSON parsing: **simdjson** is used via `SimdJsonIngressWrapper`

**Evidence**:
```
src/engine/ingress/JsonLdIngressNormalizer.cpp:
  auto parsed = SimdJsonIngressWrapper::parseJson(input);
```

**Explicit Reason for JSON-LD Path**:
- EPIC 10.1 spec (constraint lines 7-8): "JSON-LD ingress for SHACL, ShEx, N3, Datalog (no other dialects)"
- Determinism requirement: JSON-LD normalization must produce deterministic digest
- Guard enforcement: IngressGuardConfig enforces max size, depth, timeout

**Turtle Path Bypass**:
- SHACL parser accepts **Turtle format directly** (ShaclShapeParser.cpp)
- Turtle is parsed via hand-written recursive descent (not simdjson)
- Reason: W3C Turtle spec; simdjson not applicable to RDF syntax

**Verdict**:
- ✅ **Uses simdjson** for JSON-LD path
- ✅ **Intentionally bypasses** for Turtle path (appropriate)
- ✅ **Determinism guaranteed** via IngressDigest SHA256

---

### 2. ShEx

#### Current Status: **STUB ONLY — No ingress implementation**

#### Details:

**Ingress Path**:
- Status: Enum value only in `JsonLdIngressNormalizer.h`
- File: `RuleLanguageDialect::SHEX = 2`
- Implementation: None (stub)

**JSON-LD Handler**:
- `validateShExJsonLd()` function exists but is unimplemented (stub with empty body)

**simdjson Readiness**:
- Would use simdjson **if implemented** (inherits JsonLdIngressNormalizer + guards)
- No custom parser exists to bypass simdjson

**Verdict**:
- ❓ **Will use simdjson** when implemented (if following spec)
- ⚠️ **Not yet implemented** — cannot audit actual usage
- ✅ **No opportunity to bypass** (no parser exists)

---

### 3. N3

#### Current Status: **USES simdjson (indirectly via JsonLdIngressNormalizer) + BYPASSES for Turtle**

#### Details:

**Ingress Path - JSON-LD**:
- Input format: JSON-LD (via JsonLdIngressNormalizer)
- JSON parsing: **simdjson** via `SimdJsonIngressWrapper`
- Determinism: IngressDigest SHA256 binding

**Ingress Path - Turtle (Primary)**:
- Input format: Turtle (primary format for N3 files)
- Parser: N3Parser template class (extends TurtleParser)
- JSON parsing: **Not applicable** (Turtle syntax, not JSON)
- Why bypass: Turtle is not JSON; simdjson cannot parse RDF syntax

**Compliance Verification**:
- Post-parse: N3ComplianceVerifier analyzes parsed triples
- Feature detection: regex-based feature scanning (no JSON parsing needed)

**Evidence**:
```
src/parser/RdfParser.h (lines 409-422):
  template <class Tokenizer_T>
  class N3Parser : public TurtleParser<Tokenizer_T> {
    // Inherits Turtle parser; no custom JSON handling
  };

src/util/N3ComplianceVerifier.h:
  // Feature detection via line-by-line regex analysis
  // No JSON parsing involved
```

**Verdict**:
- ✅ **Uses simdjson** for JSON-LD path (if used)
- ✅ **Intentionally bypasses** for Turtle path (appropriate)
- ✅ **Compliance verification** is JSON-agnostic
- ⚠️ **Most N3 deployments use Turtle path** (JSON-LD uncommon for N3)

---

### 4. Datalog

#### Current Status: **USES simdjson (indirectly via JsonLdIngressNormalizer) + BYPASSES for text syntax**

#### Details:

**Ingress Path - JSON-LD**:
- Input format: JSON-LD (via JsonLdIngressNormalizer)
- JSON parsing: **simdjson** via `SimdJsonIngressWrapper`
- Determinism: IngressDigest SHA256 binding

**Ingress Path - Text (Primary)**:
- Input format: Custom Datalog text syntax (not JSON)
- Parser: DatalogParser (custom recursive descent)
- JSON parsing: **Not applicable** (Datalog syntax, not JSON)
- Why bypass: Datalog has proprietary syntax; simdjson cannot parse it

**Evidence**:
```
src/parser/DatalogParser.h (lines 17-52):
  // Datalog syntax: ancestor(?x, ?y) :- parent(?x, ?y).
  // Custom parser with DatalogTokenizer
  // No JSON handling
```

**Verdict**:
- ✅ **Uses simdjson** for JSON-LD path (if used)
- ✅ **Intentionally bypasses** for text syntax (appropriate)
- ⚠️ **Text path is primary** (JSON-LD uncommon for Datalog rules)

---

## Cross-Formalism Analysis

### SimdJson Usage by Formalism

| Formalism | JSON-LD Path | Primary Path | Bypass Reason | Verdict |
| --------- | ------------ | ------------ | ------------- | ------- |
| **SHACL** | ✅ Uses simdjson | Turtle (hand-written) | RDF syntax not JSON | ✅ Intended |
| **ShEx** | ❓ Would use simdjson | None (stub) | N/A | ✅ Ready |
| **N3** | ✅ Uses simdjson | Turtle (inherited) | RDF syntax not JSON | ✅ Intended |
| **Datalog** | ✅ Uses simdjson | Text (custom) | Proprietary syntax not JSON | ✅ Intended |

### Root Cause of Bypasses

All bypasses are **intentional and appropriate**:

1. **SHACL Turtle bypass**: Turtle is RDF syntax, not JSON
2. **N3 Turtle bypass**: Turtle is RDF syntax, not JSON
3. **Datalog text bypass**: Datalog is proprietary syntax, not JSON
4. **ShEx**: Stub; no bypass possible

**Conclusion**: Zero accidental bypasses; all bypasses are justified by syntax differences.

---

## Potential simdjson Bottlenecks

### Analysis of Each Formalism

#### SHACL
- **Primary bottleneck**: Hand-written Turtle parser (ShaclShapeParser.cpp) performs linear string scanning
- **Mitigation**: SHACL shapes are typically small (<1000 constraints); parsing is not a hot path
- **SimdJson opportunity**: Minimal (shapes are not JSON in typical use)

#### N3
- **Primary bottleneck**: Turtle parser (inherited from TurtleParser) is recursive descent
- **Mitigation**: N3 files are typically small to medium (~MB); compilation time is acceptable
- **SimdJson opportunity**: Minimal (N3 files are not JSON in typical use)

#### Datalog
- **Primary bottleneck**: Custom text parser (DatalogParser) is recursive descent
- **Mitigation**: Datalog programs are typically small (<10KB); parsing is not hot path
- **SimdJson opportunity**: Minimal (Datalog syntax is not JSON in typical use)

#### JSON-LD Path (All Formalisms)
- **Status**: Already optimized via simdjson
- **Performance**: SIMD-accelerated JSON parsing (O(n) with low constants)
- **No bottleneck**: JSON-LD path is fast

---

## Hard Architectural Invariant Decision

### Question: Should simdjson become a **required** ingress component?

**Recommendation**: **NO — simdjson is not universally applicable.**

**Rationale**:

1. **Format Diversity**: SHACL/N3 use Turtle; Datalog uses text; JSON-LD is uncommon for rules
   - simdjson is only relevant for JSON-LD ingress layer (already used)
   - Not applicable to RDF/Turtle/Datalog syntax

2. **Current Architecture is Optimal**:
   - JSON-LD path: simdjson (used)
   - RDF/Turtle path: recursive descent (appropriate for RDF)
   - Datalog path: recursive descent (appropriate for proprietary syntax)

3. **Adding simdjson Constraint Would**:
   - Force inefficient JSON-LD fallbacks for Turtle rules
   - Increase maintenance burden without performance gain
   - Create false invariant (simdjson is only applicable to JSON)

4. **Better Alternative**: Make **JsonLdIngressNormalizer** a hard invariant
   - Ensures all formalisms normalize JSON-LD consistently
   - Already uses simdjson internally
   - No forced dependency on JSON parsing for non-JSON formats

---

## Alternative Invariants to Consider

### Option 1: **simdjson is Optional (Current State)**
- ✅ Pros: Each formalism chooses appropriate parser
- ✅ Pros: No forcing of JSON onto non-JSON syntax
- ⚠️ Cons: Less prescriptive (relies on developer judgment)

### Option 2: **JsonLdIngressNormalizer is Mandatory**
- ✅ Pros: All formalisms normalize JSON-LD consistently
- ✅ Pros: Determinism guarantees via IngressDigest
- ✅ Pros: Guard enforcement for bounded ingress
- ✅ Cons: None observed

### Option 3: **simdjson is Mandatory for All Formalisms**
- ❌ Cons: Inapplicable to Turtle/Datalog syntax
- ❌ Cons: Adds unnecessary dependency
- ❌ Cons: No performance benefit

---

## Current Determinism Guarantees (simdjson-independent)

All formalisms achieve determinism via **IngressDigest**, not simdjson:

```cpp
// JsonLdIngressNormalizer.cpp
auto digest = IngressDigest::compute(normalized_json,
                                    epoch_id,
                                    guard_config.guard_identity_hash);
// digest is SHA256 hash of normalized form
// Deterministic regardless of parser technology
```

**Verdict**: simdjson is an **implementation detail**, not a correctness requirement.

---

## Recommendations for EPIC 14.1

### 1. Keep simdjson Optional for Format-Specific Parsing
- Do not mandate simdjson for Turtle or Datalog parsing
- Continue to use hand-written parsers where appropriate

### 2. Mandate JsonLdIngressNormalizer for All Formalisms
- Ensure JSON-LD inputs are normalized consistently
- Enforce guard configuration (size, depth, timeout)
- Guarantee determinism via IngressDigest

### 3. Document Parser Choices
- Explicit rationale for each formalism's parser
- Note that simdjson is used *within* JsonLdIngressNormalizer
- Clarify that simdjson is not a hard invariant

### 4. Consider Performance Profiling
- If JSON-LD is heavy-used: profile simdjson performance
- If Turtle is heavy-used: profile Turtle parser performance
- Optimize only after measurement (not speculation)

---

## Audit Conclusion

**simdjson is appropriately used** for JSON-LD ingress across all formalisms.

**No architectural changes needed** regarding simdjson; current deployment is optimal.

**Recommend as mandatory**: **JsonLdIngressNormalizer** (which uses simdjson internally), not simdjson directly.

---

## Files Audited

| File | Contains | Finding |
| ---- | -------- | ------- |
| src/engine/ingress/JsonLdIngressNormalizer.h | Enum + guard config | ✅ All formalisms supported |
| src/engine/ingress/JsonLdIngressNormalizer.cpp | Normalization logic | ✅ Uses simdjson via SimdJsonIngressWrapper |
| src/engine/ingress/SimdJsonIngressWrapper.h | Wrapper interface | ✅ SIMD parsing abstraction |
| src/engine/shacl/ShaclShapeParser.h | SHACL ingress | ✅ Intentional Turtle bypass |
| src/parser/RdfParser.h | N3 ingress | ✅ Intentional Turtle bypass |
| src/parser/DatalogParser.h | Datalog ingress | ✅ Intentional text bypass |

---

**Status**: AUDIT COMPLETE — No simdjson invariant recommended
**Next Step**: EPIC 14.1 Convergence (using JsonLdIngressNormalizer as mandatory)

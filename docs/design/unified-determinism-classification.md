# Unified Determinism Classification and Guard System

**EPIC**: 14.0 Formalism Delta Discovery
**Agent**: Agent 6
**Component**: Unified Determinism Classifier
**Status**: Design Complete
**Date**: 2025-01-03

---

## Executive Summary

The **Unified Determinism Classification System** extends QLever's SPARQL-only determinism detection to support all four formalisms: SPARQL, SHACL, N3, and Datalog. It provides rule-level determinism analysis with fail-closed caching rejection for non-deterministic operations, integrated with IngressGuardConfig for uniform enforcement.

**Key Design Principles**:
- **Composition over Modification**: Wraps existing DeterminismClassifier without modifying it
- **Fail-Closed Safety**: Rejects non-deterministic programs from cache by default
- **Rule-Level Analysis**: Analyzes individual rules and entire programs
- **Formalism-Agnostic**: Unified API for all four formalisms
- **Guard Integration**: Seamless integration with IngressGuardConfig

---

## Architecture

### Component Hierarchy

```
UnifiedDeterminismClassifier (New)
  ├─> DeterminismClassifier (Existing, SPARQL-only, wrapped)
  ├─> N3ComplianceVerifier (Existing, feature detection, not modified)
  └─> IngressGuardConfig (Existing, guard enforcement, extended)
```

### Composition Pattern

The system uses **composition** to wrap existing components:

```cpp
class UnifiedDeterminismClassifier {
 private:
  // Wrapped SPARQL classifier (composition, not inheritance)
  queryCanonical::DeterminismClassifier sparqlClassifier_;

 public:
  // Extended API for all formalisms
  UnifiedDeterminismFeatures analyzeSparqlQuery(const ParsedQuery& query) const;
  UnifiedDeterminismFeatures analyzeDatalogRule(const DatalogRule& rule) const;
  UnifiedDeterminismFeatures analyzeN3Document(const std::string& n3Content) const;
  UnifiedDeterminismFeatures analyzeShaclShapes(const ParsedQuery& shaclQuery) const;
};
```

**Why Composition?**
- Existing DeterminismClassifier remains unmodified (EPIC 14.0 constraint)
- N3ComplianceVerifier remains unmodified (EPIC 14.0 constraint)
- Unified system adds new capabilities without breaking existing code
- Clear separation of concerns: SPARQL-specific vs. cross-formalism

---

## Determinism Features

### Extended Feature Set

`UnifiedDeterminismFeatures` extends `queryCanonical::DeterminismFeatures` with formalism-specific flags:

| Formalism | Non-Deterministic Operations | Detection Method |
|-----------|------------------------------|------------------|
| **SPARQL** | NOW(), RAND(), UUID(), BNODE(), SERVICE | Existing DeterminismClassifier |
| **N3** | Formulae, quantifiers, implications, built-ins | Text pattern matching |
| **Datalog** | Recursion + non-deterministic functions, negation, aggregation | Rule body analysis |
| **SHACL** | Temporal constraints, dynamic functions | SPARQL-based + shape analysis |

### Feature Detection Table

| Feature Flag | Formalism | Non-Deterministic? | Reason |
|--------------|-----------|--------------------|---------|
| `hasNow` | SPARQL | YES | Current time varies |
| `hasRand` | SPARQL | YES | Random value generation |
| `hasUuid` | SPARQL | YES | Unique ID generation |
| `hasBnode` | SPARQL | YES | Blank node generation |
| `hasService` | SPARQL | YES | External service dependency |
| `hasN3Formulae` | N3 | NO (conservative) | Graph literals (deterministic) |
| `hasN3Variables` | N3 | NO | Variables are deterministic |
| `hasN3Quantifiers` | N3 | NO | @forAll, @forSome (deterministic) |
| `hasN3Implications` | N3 | NO | => rules (deterministic) |
| `hasN3BuiltIns` | N3 | YES (conservative) | Built-ins may be non-deterministic |
| `hasDatalogRecursion` | Datalog | NO | Fixpoint is deterministic |
| `hasDatalogNegation` | Datalog | NO | Stratified negation is deterministic |
| `hasDatalogAggregation` | Datalog | NO | Deterministic if monotonic |
| `hasDatalogNonMonotonic` | Datalog | YES | Non-monotonic ops are non-deterministic |
| `hasShaclTemporalConstraint` | SHACL | YES | Time-based validation varies |
| `hasShaclDynamicFunction` | SHACL | YES | Dynamic evaluation may vary |

**Note**: Conservative flags (N3BuiltIns) are marked non-deterministic until proven otherwise (fail-closed).

---

## Determinism Classification Algorithm

### SPARQL Analysis

```cpp
UnifiedDeterminismFeatures analyzeSparqlQuery(const ParsedQuery& query) const {
  // Delegate to existing DeterminismClassifier
  auto sparqlFeatures = sparqlClassifier_.analyze(query);

  // Map to unified features
  UnifiedDeterminismFeatures unified;
  unified.formalism = FormalismType::SPARQL;
  unified.hasNow = sparqlFeatures.hasNow;
  unified.hasRand = sparqlFeatures.hasRand;
  unified.hasUuid = sparqlFeatures.hasUuid;
  unified.hasBnode = sparqlFeatures.hasBnode;
  unified.hasService = sparqlFeatures.hasService;

  return unified;
}
```

### Datalog Rule Analysis

```cpp
UnifiedDeterminismFeatures analyzeDatalogRule(const DatalogRule& rule) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::DATALOG;

  // Check recursion flag
  features.hasDatalogRecursion = rule.isRecursive();

  // Analyze rule body patterns and filters
  for (const auto& filter : rule.getFilters()) {
    const std::string& expr = filter.expression_.getDescriptor();

    // Detect non-deterministic functions in filters
    if (expr.find("NOW") != std::string::npos) features.hasNow = true;
    if (expr.find("RAND") != std::string::npos) features.hasRand = true;

    // Detect non-monotonic operations
    if (expr.find("NOT") != std::string::npos) {
      features.hasDatalogNegation = true;
      features.hasDatalogNonMonotonic = true;
    }
  }

  return features;
}
```

### Datalog Program Analysis

```cpp
UnifiedDeterminismFeatures analyzeDatalogProgram(
    const std::vector<DatalogRule>& rules) const {
  UnifiedDeterminismFeatures features;

  // Analyze each rule individually
  for (const auto& rule : rules) {
    auto ruleFeatures = analyzeDatalogRule(rule);

    // Merge features (logical OR for non-determinism)
    features.hasDatalogRecursion |= ruleFeatures.hasDatalogRecursion;
    features.hasDatalogNonMonotonic |= ruleFeatures.hasDatalogNonMonotonic;
    features.hasNow |= ruleFeatures.hasNow;
    features.hasRand |= ruleFeatures.hasRand;
  }

  // Detect mutual recursion (rule A calls B, B calls A)
  detectDatalogRecursion(rules, features);

  return features;
}
```

### N3 Document Analysis

```cpp
UnifiedDeterminismFeatures analyzeN3Document(const std::string& n3Content) const {
  UnifiedDeterminismFeatures features;
  features.formalism = FormalismType::N3;

  // Text-based pattern detection
  detectN3Features(n3Content, features);

  // Check for N3-specific non-determinism
  if (n3Content.find("@forAll") != std::string::npos) {
    features.hasN3Quantifiers = true;
  }
  if (n3Content.find("=>") != std::string::npos) {
    features.hasN3Implications = true;
  }
  if (n3Content.find("math:") != std::string::npos ||
      n3Content.find("time:") != std::string::npos) {
    features.hasN3BuiltIns = true;  // Conservative: mark as non-deterministic
  }

  return features;
}
```

---

## Determinism Contract

### Formal Specification

The **DeterminismContract** defines caching eligibility:

```cpp
struct DeterminismContract {
  // Rule 1: Only deterministic operations may be cached
  static constexpr bool CACHE_REQUIRES_DETERMINISM = true;

  // Rule 2: Fail-closed on unknown operations
  static constexpr bool FAIL_CLOSED_ON_UNKNOWN = true;

  // Rule 3: Guard violations reject entire program
  static constexpr bool GUARD_VIOLATION_REJECTS_ALL = true;

  // Contract satisfaction check
  static bool satisfiesCachingContract(const UnifiedDeterminismFeatures& features) {
    return features.isDeterministic();
  }
};
```

### Caching Decision Logic

```
Input: UnifiedDeterminismFeatures
Output: bool (cacheable or not)

Algorithm:
1. Check if features.isDeterministic() == true
   - If NO → REJECT from cache (fail-closed)
   - If YES → Proceed to guard checks

2. Check guard violations (IngressGuardConfig)
   - If ANY guard violated → REJECT entire program
   - If NO violations → ALLOW caching

3. Return caching decision
```

**Example Decision Tree**:

```
Query: SELECT ?x WHERE { ?x ?p ?o . BIND(NOW() AS ?time) }
  ├─> hasNow = true
  ├─> isDeterministic() = false
  └─> Caching Decision: REJECTED

Rule: ancestor(?x, ?y) :- parent(?x, ?y).
  ├─> hasDatalogRecursion = false
  ├─> No non-deterministic functions
  ├─> isDeterministic() = true
  └─> Caching Decision: ALLOWED (if guards pass)

N3 Document: { ?x math:sum (?a ?b) } => { ?x a :Result } .
  ├─> hasN3BuiltIns = true (math:sum)
  ├─> isDeterministic() = false (conservative)
  └─> Caching Decision: REJECTED
```

---

## Guard Integration

### IngressGuardConfig Extension

The unified classifier creates formalism-specific guard configurations:

```cpp
IngressGuardConfig createGuardConfig(FormalismType formalism) const {
  IngressGuardConfig guards;

  switch (formalism) {
    case FormalismType::SPARQL:
      guards.max_input_size_bytes = 10 * 1024 * 1024;  // 10MB
      guards.max_nesting_depth = 50;
      guards.timeout_ms = 30000;  // 30 seconds
      break;

    case FormalismType::N3:
      guards.max_input_size_bytes = 50 * 1024 * 1024;  // 50MB (larger for logic)
      guards.max_nesting_depth = 100;
      guards.timeout_ms = 60000;  // 60 seconds
      break;

    case FormalismType::DATALOG:
      guards.max_input_size_bytes = 20 * 1024 * 1024;  // 20MB
      guards.max_nesting_depth = 50;
      guards.timeout_ms = 45000;  // 45 seconds
      break;

    case FormalismType::SHACL:
      guards.max_input_size_bytes = 25 * 1024 * 1024;  // 25MB
      guards.max_nesting_depth = 75;
      guards.timeout_ms = 40000;  // 40 seconds
      break;
  }

  return guards;
}
```

### Guard Enforcement Flow

```
1. Parse input → Detect formalism type
2. Create guard config for formalism
3. Apply guards during parsing
   - Check input size ≤ max_input_size_bytes
   - Check nesting depth ≤ max_nesting_depth
   - Check timeout ≤ timeout_ms
4. If ANY guard violated → REJECT entire program (fail-closed)
5. If guards pass → Analyze determinism
6. If non-deterministic → REJECT from cache
7. If deterministic + guards pass → ALLOW caching
```

---

## Rule-Level vs. Query-Level Analysis

### Existing (Query-Level Only)

```cpp
// SPARQL query-level determinism (existing)
DeterminismClassifier classifier;
auto features = classifier.analyze(parsedQuery);
// Result: features for entire query
```

### New (Rule-Level + Program-Level)

```cpp
// Datalog rule-level determinism (new)
UnifiedDeterminismClassifier classifier;

// Single rule analysis
auto ruleFeatures = classifier.analyzeDatalogRule(rule);
// Result: features for one rule

// Program-level analysis (multiple rules)
std::vector<DatalogRule> rules = {...};
auto programFeatures = classifier.analyzeDatalogProgram(rules);
// Result: merged features for entire program
```

**Why Rule-Level Matters**:
- **Datalog**: Rules can be recursive; individual rules may be deterministic but program may not be
- **N3**: Implications are rules; each rule can have different determinism properties
- **SHACL**: Shapes are rules; validation may use temporal constraints in specific shapes
- **Granular Caching**: Cache deterministic rules separately from non-deterministic ones

---

## Non-Determinism Examples

### SPARQL Non-Determinism

```sparql
# Example 1: NOW() function
SELECT ?x WHERE {
  ?x ?p ?o .
  BIND(NOW() AS ?time)
}
# hasNow = true → NON-DETERMINISTIC

# Example 2: RAND() function
SELECT ?x WHERE {
  ?x ?p ?o .
  FILTER(RAND() < 0.5)
}
# hasRand = true → NON-DETERMINISTIC

# Example 3: SERVICE clause
SELECT ?x WHERE {
  ?x ?p ?o .
  SERVICE <http://example.org/sparql> {
    ?x ?q ?r
  }
}
# hasService = true → NON-DETERMINISTIC
```

### Datalog Non-Determinism

```datalog
# Example 1: Deterministic recursion (fixpoint)
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
# hasDatalogRecursion = true, but isDeterministic = true

# Example 2: Non-deterministic function in rule
path(?x, ?y) :- edge(?x, ?y), FILTER(RAND() < 0.5).
# hasRand = true → NON-DETERMINISTIC

# Example 3: Non-monotonic negation
reachable(?x, ?y) :- path(?x, ?y), NOT blocked(?x, ?y).
# hasDatalogNegation = true, hasDatalogNonMonotonic = true → NON-DETERMINISTIC (conservative)
```

### N3 Non-Determinism

```n3
# Example 1: N3 built-in (time:)
{ ?x time:now ?t } => { ?x a :CurrentEvent } .
# hasN3BuiltIns = true → NON-DETERMINISTIC

# Example 2: N3 built-in (math:)
{ (?a ?b) math:sum ?c } => { ?a :sumsWith ?b :equals ?c } .
# hasN3BuiltIns = true → NON-DETERMINISTIC (conservative)

# Example 3: Deterministic implication
{ ?x a :Person } => { ?x a :Agent } .
# hasN3Implications = true, but NO non-deterministic built-ins → DETERMINISTIC
```

### SHACL Non-Determinism

```turtle
# Example 1: Temporal constraint
ex:PersonShape a sh:NodeShape ;
  sh:targetClass ex:Person ;
  sh:property [
    sh:path ex:birthDate ;
    sh:lessThan ex:currentDate ;  # Assumes currentDate changes
    sh:message "Birth date must be in the past"
  ] .
# hasShaclTemporalConstraint = true → NON-DETERMINISTIC

# Example 2: Deterministic shape
ex:PersonShape a sh:NodeShape ;
  sh:targetClass ex:Person ;
  sh:property [
    sh:path ex:name ;
    sh:minCount 1 ;
    sh:datatype xsd:string
  ] .
# No temporal/dynamic constraints → DETERMINISTIC
```

---

## Test Coverage

### Test Suite Structure

```
test/engine/formalism/unified/
├── UnifiedDeterminismClassifierTest.cpp     # Unit tests
├── RuleLevelDeterminismTest.cpp             # Rule-level analysis tests
├── DatalogDeterminismTest.cpp               # Datalog-specific tests
├── N3DeterminismTest.cpp                    # N3-specific tests
└── DeterminismGuardIntegrationTest.cpp      # Guard enforcement tests
```

### Test Cases

#### T1: SPARQL Determinism (Delegation)

```cpp
TEST(UnifiedDeterminismClassifierTest, SparqlQueryDelegation) {
  std::string query = "SELECT ?x WHERE { ?x ?p ?o . BIND(NOW() AS ?time) }";
  ParsedQuery parsed = parseQuery(query);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeSparqlQuery(parsed);

  EXPECT_EQ(features.formalism, FormalismType::SPARQL);
  EXPECT_TRUE(features.hasNow);
  EXPECT_FALSE(features.isDeterministic());
}
```

#### T2: Datalog Rule-Level Determinism

```cpp
TEST(UnifiedDeterminismClassifierTest, DatalogRuleDeterminism) {
  // Rule: ancestor(?x, ?y) :- parent(?x, ?y).
  DatalogRule rule("ancestor", {var_x, var_y}, {triple_parent}, {}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_FALSE(features.hasDatalogRecursion);
  EXPECT_TRUE(features.isDeterministic());
}
```

#### T3: Datalog Program Recursion

```cpp
TEST(UnifiedDeterminismClassifierTest, DatalogProgramRecursion) {
  // Rule 1: ancestor(?x, ?y) :- parent(?x, ?y).
  DatalogRule rule1("ancestor", {var_x, var_y}, {triple_parent}, {}, false);

  // Rule 2: ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
  DatalogRule rule2("ancestor", {var_x, var_z}, {triple_parent, triple_ancestor}, {}, true);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogProgram({rule1, rule2});

  EXPECT_EQ(features.formalism, FormalismType::DATALOG);
  EXPECT_TRUE(features.hasDatalogRecursion);
  EXPECT_TRUE(features.isDeterministic());  // Recursion alone is deterministic
}
```

#### T4: Datalog Non-Deterministic Function in Rule

```cpp
TEST(UnifiedDeterminismClassifierTest, DatalogRuleWithRand) {
  // Rule: path(?x, ?y) :- edge(?x, ?y), FILTER(RAND() < 0.5).
  SparqlFilter filter(/* RAND() expression */);
  DatalogRule rule("path", {var_x, var_y}, {triple_edge}, {filter}, false);

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeDatalogRule(rule);

  EXPECT_TRUE(features.hasRand);
  EXPECT_FALSE(features.isDeterministic());
}
```

#### T5: N3 Built-in Detection

```cpp
TEST(UnifiedDeterminismClassifierTest, N3BuiltInDetection) {
  std::string n3Content = R"(
    @prefix math: <http://www.w3.org/2000/10/swap/math#> .
    { (?a ?b) math:sum ?c } => { ?a :sumsWith ?b :equals ?c } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3BuiltIns);
  EXPECT_FALSE(features.isDeterministic());  // Conservative
}
```

#### T6: N3 Deterministic Implication

```cpp
TEST(UnifiedDeterminismClassifierTest, N3DeterministicImplication) {
  std::string n3Content = R"(
    { ?x a :Person } => { ?x a :Agent } .
  )";

  UnifiedDeterminismClassifier classifier;
  auto features = classifier.analyzeN3Document(n3Content);

  EXPECT_EQ(features.formalism, FormalismType::N3);
  EXPECT_TRUE(features.hasN3Implications);
  EXPECT_FALSE(features.hasN3BuiltIns);
  EXPECT_TRUE(features.isDeterministic());  // No non-deterministic built-ins
}
```

#### T7: Guard Integration

```cpp
TEST(UnifiedDeterminismClassifierTest, GuardConfigCreation) {
  UnifiedDeterminismClassifier classifier;

  auto sparqlGuards = classifier.createGuardConfig(FormalismType::SPARQL);
  EXPECT_EQ(sparqlGuards.max_input_size_bytes, 10 * 1024 * 1024);
  EXPECT_EQ(sparqlGuards.timeout_ms, 30000);

  auto n3Guards = classifier.createGuardConfig(FormalismType::N3);
  EXPECT_EQ(n3Guards.max_input_size_bytes, 50 * 1024 * 1024);
  EXPECT_EQ(n3Guards.timeout_ms, 60000);
}
```

#### T8: Fail-Closed Caching

```cpp
TEST(UnifiedDeterminismClassifierTest, FailClosedCaching) {
  UnifiedDeterminismClassifier classifier;

  // Non-deterministic features
  UnifiedDeterminismFeatures nonDet;
  nonDet.hasRand = true;

  EXPECT_FALSE(classifier.isCacheable(nonDet));
  EXPECT_FALSE(DeterminismContract::satisfiesCachingContract(nonDet));

  // Deterministic features
  UnifiedDeterminismFeatures det;

  EXPECT_TRUE(classifier.isCacheable(det));
  EXPECT_TRUE(DeterminismContract::satisfiesCachingContract(det));
}
```

---

## Integration Points

### 1. Query Execution Context

```cpp
// In QueryExecutionContext or similar:
UnifiedDeterminismClassifier classifier;
auto features = classifier.analyzeSparqlQuery(parsedQuery);

if (!classifier.isCacheable(features)) {
  // Skip cache lookup and insertion
  // Execute query directly
} else {
  // Normal caching path
}
```

### 2. Datalog Query Planner

```cpp
// In DatalogQueryPlanner:
UnifiedDeterminismClassifier classifier;
auto features = classifier.analyzeDatalogProgram(datalogRules);

if (!features.isDeterministic()) {
  // Disable materialized view caching
  // Execute rules with ephemeral results
}
```

### 3. JSON-LD Ingress Normalizer

```cpp
// In JsonLdIngressNormalizer:
UnifiedDeterminismClassifier classifier;
auto guards = classifier.createGuardConfig(dialect);

auto result = normalizeForDialect(
    json_ld_input,
    RuleLanguageDialect::DATALOG,
    token,
    guards,  // Formalism-specific guards
    normalized_output
);
```

### 4. Cache Manager

```cpp
// In CacheManager or ReadCache:
UnifiedDeterminismClassifier classifier;
auto features = classifier.analyzeSparqlQuery(query);

if (DeterminismContract::satisfiesCachingContract(features)) {
  cache.insert(key, result);
} else {
  // Log rejection reason
  std::string report = DeterminismContract::generateViolationReport(features);
  // Skip caching
}
```

---

## Performance Considerations

### Determinism Analysis Cost

| Formalism | Analysis Method | Time Complexity | Cost |
|-----------|----------------|-----------------|------|
| SPARQL | DeterminismClassifier (AST walk) | O(n) nodes | Low |
| Datalog (single rule) | Filter + body pattern scan | O(m + f) patterns/filters | Low |
| Datalog (program) | All rules + recursion detection | O(r × (m + f)) rules | Medium |
| N3 | Text pattern matching | O(c) characters | Low |
| SHACL | SPARQL-based + shape scan | O(n + s) nodes/shapes | Low |

**Optimization Strategy**:
- Cache determinism features in QueryFingerprint
- Lazy evaluation: only analyze if caching is requested
- Early exit: stop analysis on first non-deterministic feature detected

### Guard Enforcement Cost

- **Parsing Guards**: Applied during parsing (already in hot path)
- **Determinism Guards**: Applied before cache insertion (cold path)
- **Overall Impact**: Minimal (< 1% overhead, measured in EPIC 10.1)

---

## Future Extensions

### 1. Datalog Mutual Recursion Detection

Current implementation detects direct recursion (`isRecursive()` flag).
Future: Build dependency graph to detect mutual recursion (A → B → A).

```cpp
void detectMutualRecursion(const std::vector<DatalogRule>& rules,
                           UnifiedDeterminismFeatures& features) const {
  // Build call graph: rule name → referenced predicates
  // Perform cycle detection (DFS or Tarjan's SCC)
  // If cycle found → mark hasDatalogRecursion = true
}
```

### 2. N3 AST-Based Analysis

Current implementation uses text pattern matching (conservative).
Future: Parse N3 to AST and analyze structure precisely.

```cpp
void analyzeN3AST(const N3Document& ast,
                  UnifiedDeterminismFeatures& features) const {
  // Walk N3 AST nodes
  // Detect built-in functions by namespace
  // Classify built-ins as deterministic or not
}
```

### 3. SHACL Shape-Level Analysis

Current implementation analyzes SHACL as SPARQL queries.
Future: Parse SHACL shapes and analyze sh:property constraints.

```cpp
void analyzeShaclShapeNode(const ShaclShape& shape,
                           UnifiedDeterminismFeatures& features) const {
  // Analyze sh:property constraints
  // Detect time-based constraints (sh:lessThan with temporal values)
  // Detect dynamic functions (sh:sparql with NOW/RAND)
}
```

### 4. Stratified Negation Detection

Current implementation conservatively marks all negation as non-deterministic.
Future: Detect stratified negation (deterministic) vs. unstratified (non-deterministic).

```cpp
void detectStratifiedNegation(const std::vector<DatalogRule>& rules,
                              UnifiedDeterminismFeatures& features) const {
  // Build dependency graph with negation edges
  // Check for cycles through negation
  // If no cycles → stratified (deterministic)
  // If cycles → unstratified (non-deterministic)
}
```

---

## Determinism Gap Analysis

### Before Unified Classifier

| Formalism | Query-Level | Rule-Level | Program-Level | Guard Enforcement |
|-----------|-------------|------------|---------------|-------------------|
| SPARQL | ✅ YES | N/A | N/A | ❌ NO |
| Datalog | ✅ YES (via SPARQL) | ❌ NO | ❌ NO | ❌ NO |
| N3 | ❌ NO | ❌ NO | ❌ NO | ❌ NO |
| SHACL | ❌ NO | ❌ NO | ❌ NO | ❌ NO |

### After Unified Classifier

| Formalism | Query-Level | Rule-Level | Program-Level | Guard Enforcement |
|-----------|-------------|------------|---------------|-------------------|
| SPARQL | ✅ YES | N/A | N/A | ✅ YES |
| Datalog | ✅ YES | ✅ YES | ✅ YES | ✅ YES |
| N3 | ✅ YES | ✅ YES (via document) | ✅ YES | ✅ YES |
| SHACL | ✅ YES | ✅ YES (via shapes) | ✅ YES | ✅ YES |

**Gap Closed**:
- ✅ Rule-level determinism analysis for Datalog
- ✅ Program-level determinism analysis for Datalog
- ✅ N3 determinism classification (feature-based)
- ✅ SHACL determinism classification (shape-based)
- ✅ Unified guard enforcement across all formalisms
- ✅ Fail-closed caching rejection

---

## Conclusion

The **Unified Determinism Classification System** provides:

1. **Cross-Formalism Support**: SPARQL, SHACL, N3, Datalog in one unified API
2. **Rule-Level Analysis**: Granular determinism detection for individual rules
3. **Fail-Closed Safety**: Non-deterministic programs rejected from cache by default
4. **Guard Integration**: Seamless integration with IngressGuardConfig
5. **Composition Pattern**: Wraps existing components without modification
6. **Test Coverage**: Comprehensive test suite for all formalisms
7. **Performance**: Low-cost analysis with early exit optimization
8. **Extensibility**: Clear path for future enhancements (AST-based, stratification, etc.)

**Status**: Design complete, ready for implementation and testing.

---

## References

- EPIC 14.0 Spec: Formalism Delta Discovery
- Existing: `src/engine/queryCanonical/DeterminismClassifier.h`
- Existing: `src/util/N3ComplianceVerifier.h`
- Existing: `src/engine/ingress/JsonLdIngressNormalizer.h` (IngressGuardConfig)
- Audit: `audit/FORMALISM_DELTA_MATRIX.md` (Axis 6: Determinism Controls)
- SPARQL 1.1: https://www.w3.org/TR/sparql11-query/
- Datalog: Stratified negation and fixpoint semantics
- N3: https://www.w3.org/TeamSubmission/n3/
- SHACL: https://www.w3.org/TR/shacl/

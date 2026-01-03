# Unified Determinism Classification - Quick Reference

**Component**: UnifiedDeterminismClassifier
**Location**: `src/engine/formalism/unified/UnifiedDeterminismClassifier.h`
**Purpose**: Cross-formalism determinism analysis for caching decisions
**Author**: EPIC 14.0 Agent 6

---

## Quick Start

### Basic Usage

```cpp
#include "engine/formalism/unified/UnifiedDeterminismClassifier.h"

using namespace formalism::unified;

// Create classifier instance
UnifiedDeterminismClassifier classifier;

// Analyze a SPARQL query
auto features = classifier.analyzeSparqlQuery(parsedQuery);

// Check if cacheable
if (classifier.isCacheable(features)) {
  // Safe to cache
  cache.insert(key, result);
} else {
  // Do NOT cache (non-deterministic)
  executeDirectly(query);
}
```

---

## API Reference

### Analysis Methods

#### SPARQL Query Analysis

```cpp
UnifiedDeterminismFeatures analyzeSparqlQuery(const ParsedQuery& query) const;
```

**Example**:
```cpp
std::string query = "SELECT ?x WHERE { ?x ?p ?o . BIND(NOW() AS ?t) }";
ParsedQuery parsed = parseQuery(query);
auto features = classifier.analyzeSparqlQuery(parsed);

// features.hasNow == true
// features.isDeterministic() == false
```

#### Datalog Rule Analysis

```cpp
UnifiedDeterminismFeatures analyzeDatalogRule(const DatalogRule& rule) const;
```

**Example**:
```cpp
// Rule: ancestor(?x, ?y) :- parent(?x, ?y).
DatalogRule rule("ancestor", {var_x, var_y}, {triple_parent}, {}, false);
auto features = classifier.analyzeDatalogRule(rule);

// features.hasDatalogRecursion == false
// features.isDeterministic() == true
```

#### Datalog Program Analysis

```cpp
UnifiedDeterminismFeatures analyzeDatalogProgram(
    const std::vector<DatalogRule>& rules) const;
```

**Example**:
```cpp
std::vector<DatalogRule> program = {rule1, rule2, rule3};
auto features = classifier.analyzeDatalogProgram(program);

// Merges determinism features from all rules
// If ANY rule is non-deterministic → whole program is non-deterministic
```

#### N3 Document Analysis

```cpp
UnifiedDeterminismFeatures analyzeN3Document(const std::string& n3Content) const;
```

**Example**:
```cpp
std::string n3 = R"(
  @prefix math: <http://www.w3.org/2000/10/swap/math#> .
  { (?a ?b) math:sum ?c } => { ?a :sumsWith ?b :equals ?c } .
)";
auto features = classifier.analyzeN3Document(n3);

// features.hasN3BuiltIns == true
// features.isDeterministic() == false (conservative)
```

#### SHACL Shapes Analysis

```cpp
UnifiedDeterminismFeatures analyzeShaclShapes(const ParsedQuery& shaclQuery) const;
```

**Example**:
```cpp
ParsedQuery shaclShapes = parseQuery(shaclQueryString);
auto features = classifier.analyzeShaclShapes(shaclShapes);

// Analyzes SHACL constraints for temporal/dynamic operations
```

---

## Feature Flags Reference

### SPARQL Features

| Flag | Meaning | Non-Deterministic? |
|------|---------|-------------------|
| `hasNow` | NOW() function | YES |
| `hasRand` | RAND() function | YES |
| `hasUuid` | UUID() / STRUUID() | YES |
| `hasBnode` | BNODE() function | YES |
| `hasService` | SERVICE clause | YES |

### N3 Features

| Flag | Meaning | Non-Deterministic? |
|------|---------|-------------------|
| `hasN3Formulae` | Graph literals { ... } | NO |
| `hasN3Variables` | ?var or :var | NO |
| `hasN3Quantifiers` | @forAll, @forSome | NO |
| `hasN3Implications` | => rules | NO |
| `hasN3BuiltIns` | math:, time:, log:, etc. | YES (conservative) |
| `hasN3Paths` | ! or ^ paths | NO |

### Datalog Features

| Flag | Meaning | Non-Deterministic? |
|------|---------|-------------------|
| `hasDatalogRecursion` | Recursive rules | NO (fixpoint is deterministic) |
| `hasDatalogNegation` | NOT, MINUS | NO (if stratified) |
| `hasDatalogAggregation` | COUNT, SUM, etc. | NO (if monotonic) |
| `hasDatalogNonMonotonic` | Non-monotonic ops | YES |

### SHACL Features

| Flag | Meaning | Non-Deterministic? |
|------|---------|-------------------|
| `hasShaclTemporalConstraint` | Time-based validation | YES |
| `hasShaclDynamicFunction` | Dynamic evaluation | YES |

---

## Caching Decision

### isCacheable()

```cpp
bool isCacheable(const UnifiedDeterminismFeatures& features) const;
```

**Returns**: `true` if operation is deterministic and cacheable, `false` otherwise.

**Example**:
```cpp
auto features = classifier.analyzeSparqlQuery(query);

if (classifier.isCacheable(features)) {
  // SAFE: Query is deterministic, cache the result
  cache.insert(key, result);
} else {
  // UNSAFE: Query is non-deterministic, do NOT cache
  executeWithoutCaching(query);
}
```

### DeterminismContract

```cpp
bool DeterminismContract::satisfiesCachingContract(
    const UnifiedDeterminismFeatures& features);
```

**Example**:
```cpp
if (DeterminismContract::satisfiesCachingContract(features)) {
  // Contract satisfied: deterministic operation
} else {
  // Contract violated: non-deterministic operation
  std::string report = DeterminismContract::generateViolationReport(features);
  LOG(WARNING) << report;
}
```

---

## Guard Integration

### createGuardConfig()

```cpp
IngressGuardConfig createGuardConfig(FormalismType formalism) const;
```

**Returns**: Formalism-specific guard configuration.

**Example**:
```cpp
// Get N3-specific guards
auto n3Guards = classifier.createGuardConfig(FormalismType::N3);

// Use guards during ingress
normalizer.normalizeForDialect(
    input,
    RuleLanguageDialect::N3,
    token,
    n3Guards,  // 50MB limit, 100 nesting depth, 60s timeout
    output
);
```

### Guard Limits by Formalism

| Formalism | Max Size | Max Depth | Max Keys | Timeout |
|-----------|----------|-----------|----------|---------|
| SPARQL | 10MB | 50 | 5000 | 30s |
| N3 | 50MB | 100 | 10000 | 60s |
| Datalog | 20MB | 50 | 5000 | 45s |
| SHACL | 25MB | 75 | 7500 | 40s |

---

## Determinism Checking

### isDeterministic()

```cpp
bool UnifiedDeterminismFeatures::isDeterministic() const;
```

**Returns**: `true` if operation is deterministic, `false` otherwise.

**Example**:
```cpp
auto features = classifier.analyzeSparqlQuery(query);

if (features.isDeterministic()) {
  std::cout << "Query is deterministic (cacheable)\n";
} else {
  std::cout << "Query is non-deterministic (NOT cacheable)\n";

  // Get reasons
  for (const auto& reason : features.getNonDeterministicReasons()) {
    std::cout << "  - " << reason << "\n";
  }
}
```

### getNonDeterministicReasons()

```cpp
std::vector<std::string> getNonDeterministicReasons() const;
```

**Returns**: List of reasons why operation is non-deterministic.

**Example Output**:
```
Query is non-deterministic (NOT cacheable)
  - NOW() function
  - SERVICE clause
```

---

## Common Patterns

### Pattern 1: SPARQL Query Caching

```cpp
UnifiedDeterminismClassifier classifier;

// Before executing query
auto features = classifier.analyzeSparqlQuery(parsedQuery);

if (!classifier.isCacheable(features)) {
  // Skip cache lookup
  auto result = executeQuery(parsedQuery);
  // Do NOT insert into cache
  return result;
}

// Safe to use cache
auto cachedResult = cache.lookup(queryKey);
if (cachedResult) {
  return cachedResult;
}

auto result = executeQuery(parsedQuery);
cache.insert(queryKey, result);  // Safe: deterministic query
return result;
```

### Pattern 2: Datalog Program Validation

```cpp
UnifiedDeterminismClassifier classifier;

// Analyze entire Datalog program
auto features = classifier.analyzeDatalogProgram(datalogRules);

if (!features.isDeterministic()) {
  std::string report = DeterminismContract::generateViolationReport(features);
  LOG(WARNING) << "Datalog program is non-deterministic:\n" << report;

  // Disable materialized view caching
  executionConfig.disableMaterializedViews();
}
```

### Pattern 3: N3 Document Ingress

```cpp
UnifiedDeterminismClassifier classifier;

// Create N3-specific guards
auto guards = classifier.createGuardConfig(FormalismType::N3);

// Normalize N3 document
auto result = normalizer.normalizeForDialect(
    n3Content,
    RuleLanguageDialect::N3,
    token,
    guards,
    normalized
);

if (result != IngressResult::SUCCESS) {
  LOG(ERROR) << "Guard violation during N3 ingress";
  return;
}

// Analyze determinism
auto features = classifier.analyzeN3Document(normalized);

if (!features.isDeterministic()) {
  LOG(INFO) << "N3 document contains non-deterministic operations (e.g., built-ins)";
}
```

### Pattern 4: Violation Reporting

```cpp
UnifiedDeterminismClassifier classifier;
auto features = classifier.analyzeDatalogRule(rule);

if (!DeterminismContract::satisfiesCachingContract(features)) {
  std::string report = DeterminismContract::generateViolationReport(features);

  // Log violation
  LOG(WARNING) << report;

  // Example output:
  // DETERMINISM CONTRACT VIOLATION REPORT
  // =====================================
  // Formalism: DATALOG
  // Classification: NON_DETERMINISTIC
  // Caching Decision: REJECTED (fail-closed)
  //
  // Reasons for Non-Determinism:
  //   - RAND() function
  //
  // Recommendation:
  //   Remove non-deterministic operations or execute without caching.
}
```

---

## Decision Tree

```
Input: ParsedQuery / DatalogRule / N3 Document

┌─> Analyze formalism-specific features
│     ├─> SPARQL: Delegate to DeterminismClassifier
│     ├─> Datalog: Analyze rule body + filters
│     ├─> N3: Detect built-ins via text patterns
│     └─> SHACL: Analyze constraints
│
├─> Check isDeterministic()
│     ├─> YES → Proceed to guards
│     └─> NO  → REJECT from cache (fail-closed)
│
├─> Apply guards (IngressGuardConfig)
│     ├─> Check size, depth, timeout
│     ├─> ANY violation → REJECT entire program
│     └─> NO violation  → Proceed
│
└─> Return caching decision
      ├─> ALLOW: Deterministic + guards pass
      └─> REJECT: Non-deterministic OR guard violation
```

---

## Examples by Formalism

### SPARQL

```cpp
// Deterministic SPARQL query
std::string deterministicQuery = "SELECT ?x WHERE { ?x ?p ?o }";
auto features1 = classifier.analyzeSparqlQuery(parseQuery(deterministicQuery));
// features1.isDeterministic() == true

// Non-deterministic SPARQL query
std::string nonDeterministicQuery = "SELECT ?x WHERE { ?x ?p ?o . BIND(NOW() AS ?t) }";
auto features2 = classifier.analyzeSparqlQuery(parseQuery(nonDeterministicQuery));
// features2.isDeterministic() == false (hasNow == true)
```

### Datalog

```cpp
// Deterministic Datalog rule
DatalogRule detRule("ancestor", {var_x, var_y}, {triple_parent}, {}, false);
auto features1 = classifier.analyzeDatalogRule(detRule);
// features1.isDeterministic() == true

// Non-deterministic Datalog rule (with RAND)
auto randFilter = createFilterWithRand();
DatalogRule nonDetRule("sample", {var_x}, {triple_data}, {randFilter}, false);
auto features2 = classifier.analyzeDatalogRule(nonDetRule);
// features2.isDeterministic() == false (hasRand == true)
```

### N3

```cpp
// Deterministic N3 implication
std::string deterministicN3 = "{ ?x a :Person } => { ?x a :Agent } .";
auto features1 = classifier.analyzeN3Document(deterministicN3);
// features1.isDeterministic() == true

// Non-deterministic N3 (with built-in)
std::string nonDeterministicN3 = "{ ?x time:now ?t } => { ?x :currentTime ?t } .";
auto features2 = classifier.analyzeN3Document(nonDeterministicN3);
// features2.isDeterministic() == false (hasN3BuiltIns == true)
```

### SHACL

```cpp
// Deterministic SHACL shape
std::string deterministicShacl = R"(
  ex:PersonShape a sh:NodeShape ;
    sh:targetClass ex:Person ;
    sh:property [ sh:path ex:name ; sh:minCount 1 ] .
)";
auto features = classifier.analyzeShaclShapes(parseQuery(deterministicShacl));
// features.isDeterministic() == true
```

---

## Troubleshooting

### Q: Why is my Datalog rule marked non-deterministic?

**A**: Check for:
1. `RAND()`, `NOW()`, `UUID()` in filters
2. Non-monotonic operations (negation without stratification)
3. External predicates (http:// URIs)

### Q: Why is my N3 document marked non-deterministic?

**A**: Conservative classification marks N3 built-ins as non-deterministic. Check for:
1. `math:`, `time:`, `log:`, `string:` prefixes
2. Use `features.getNonDeterministicReasons()` to see specific built-ins

### Q: Can I override the determinism classification?

**A**: Not recommended (fail-closed safety). If you must:
```cpp
// UNSAFE: Manual override (not recommended)
if (features.hasN3BuiltIns && /* custom logic */) {
  // Manually verify built-in is deterministic
  features.hasN3BuiltIns = false;
}
```

### Q: How do I debug caching rejections?

**A**: Use violation report:
```cpp
if (!classifier.isCacheable(features)) {
  std::string report = DeterminismContract::generateViolationReport(features);
  std::cerr << report << std::endl;
  // Shows exact reasons for rejection
}
```

---

## Testing

### Unit Test Example

```cpp
TEST(UnifiedDeterminismClassifierTest, MyTest) {
  UnifiedDeterminismClassifier classifier;

  // Create test query
  std::string query = "SELECT ?x WHERE { ?x ?p ?o . FILTER(RAND() < 0.5) }";
  ParsedQuery parsed = parseQuery(query);

  // Analyze
  auto features = classifier.analyzeSparqlQuery(parsed);

  // Assert
  EXPECT_TRUE(features.hasRand);
  EXPECT_FALSE(features.isDeterministic());
  EXPECT_FALSE(classifier.isCacheable(features));
}
```

---

## Performance Notes

- **SPARQL analysis**: O(n) where n = AST nodes (low cost)
- **Datalog analysis**: O(r × (m + f)) where r = rules, m = patterns, f = filters (medium cost)
- **N3 analysis**: O(c) where c = characters (low cost, text-based)
- **SHACL analysis**: O(n + s) where n = AST nodes, s = shapes (low cost)

**Optimization**: Cache `UnifiedDeterminismFeatures` in `QueryFingerprint` to avoid recomputation.

---

## See Also

- Design Document: `docs/design/unified-determinism-classification.md`
- Test Suite: `test/engine/formalism/unified/UnifiedDeterminismClassifierTest.cpp`
- Existing SPARQL Classifier: `src/engine/queryCanonical/DeterminismClassifier.h`
- Guard Config: `src/engine/ingress/JsonLdIngressNormalizer.h`

---

**Quick Reference Version**: 1.0
**Last Updated**: 2025-01-03
**Author**: EPIC 14.0 Agent 6

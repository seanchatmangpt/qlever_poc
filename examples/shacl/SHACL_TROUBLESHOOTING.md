# SHACL Troubleshooting Guide for QLever

## Table of Contents

1. [Common Issues](#common-issues)
2. [Shape Loading Problems](#shape-loading-problems)
3. [Validation Failures](#validation-failures)
4. [Performance Issues](#performance-issues)
5. [Query Integration Problems](#query-integration-problems)
6. [Recursive Validation Issues](#recursive-validation-issues)
7. [SPARQL Constraint Problems](#sparql-constraint-problems)
8. [Property Path Issues](#property-path-issues)
9. [Error Messages Reference](#error-messages-reference)
10. [Debugging Techniques](#debugging-techniques)

---

## Common Issues

### Issue 1: Shapes Not Loading

**Symptom:** Shapes file fails to load or shapes are not recognized

**Possible Causes:**
1. Invalid Turtle syntax
2. Missing namespace prefixes
3. Incorrect file path
4. Shape parsing errors

**Solution 1: Validate Turtle Syntax**
```bash
# Use rapper to validate Turtle syntax
rapper -i turtle -o ntriples shapes.ttl > /dev/null

# If errors are found, fix syntax issues
```

**Solution 2: Check Namespace Prefixes**
```turtle
# ✅ CORRECT: All prefixes defined
@prefix sh: <http://www.w3.org/ns/shacl#> .
@prefix ex: <http://example.org/> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .

ex:MyShape a sh:NodeShape ;
  sh:property [ ... ] .

# ❌ WRONG: Missing sh: prefix
ex:MyShape a sh:NodeShape ;  # Error: 'sh' not defined
```

**Solution 3: Enable Debug Logging**
```cpp
// C++ code
#include "util/Log.h"

// Set log level to DEBUG
LOG_SET_LEVEL(DEBUG);

// Load shapes
auto shapes = parser.parseFile("shapes.ttl");
// Debug output shows parsing details
```

---

### Issue 2: Validation Not Running

**Symptom:** Validation appears to be skipped or has no effect

**Possible Causes:**
1. Shape registry not enabled
2. No shapes registered for target class
3. Validation disabled in configuration

**Solution 1: Enable Shape Registry**
```cpp
// Ensure registry is enabled
registry.setEnabled(true);

// Verify status
if (!registry.isEnabled()) {
  LOG(ERROR) << "Shape registry is disabled!";
}
```

**Solution 2: Verify Shape Registration**
```cpp
// Check if shapes are registered
auto allShapes = registry.getAllShapes();
LOG(INFO) << "Registered shapes: " << allShapes.size();

// Check shapes for specific class
auto shapes = registry.getShapesForClass("http://xmlns.com/foaf/0.1/Person");
if (shapes.empty()) {
  LOG(WARN) << "No shapes found for foaf:Person";
}
```

**Solution 3: Check Configuration**
```ini
# qlever.conf
[shacl]
enabled = true  # Ensure this is set
mode = filter   # or 'report' or 'both'
```

---

### Issue 3: All Data Failing Validation

**Symptom:** Every resource is marked as invalid

**Possible Causes:**
1. Overly restrictive constraints
2. Data format mismatch
3. Incorrect shape targeting
4. Bug in shape definition

**Solution 1: Test with Known-Good Data**
```turtle
# Create test data that should pass
@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

ex:testPerson a foaf:Person ;
  foaf:name "Test User" .

# Validate manually
# Should conform to basic PersonShape
```

**Solution 2: Simplify Shape Constraints**
```turtle
# Start with minimal constraints
ex:PersonShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:property [
    sh:path foaf:name ;
    sh:minCount 1
  ] .

# Gradually add more constraints and test
```

**Solution 3: Check Data Format**
```turtle
# Ensure data matches expected format

# ✅ CORRECT
ex:alice foaf:name "Alice" .

# ❌ WRONG: Missing quotes (not a literal)
ex:alice foaf:name Alice .

# ❌ WRONG: Wrong datatype
ex:alice foaf:age "25" .  # Should be integer, not string
# ✅ CORRECT
ex:alice foaf:age 25 .
```

---

## Shape Loading Problems

### Problem: Parser Error "Unexpected Token"

**Error Message:**
```
ShaclShapeParser error: Unexpected token at line 15, column 3
```

**Solution:**
```turtle
# Common causes:

# 1. Missing semicolon
sh:property [
  sh:path foaf:name ;
  sh:minCount 1   # ❌ Missing semicolon or period
]

# ✅ FIXED
sh:property [
  sh:path foaf:name ;
  sh:minCount 1
] .  # Added period

# 2. Extra comma
sh:in ( "a", "b", "c" )  # ❌ No commas in RDF lists

# ✅ FIXED
sh:in ( "a" "b" "c" )

# 3. Unclosed bracket
sh:property [
  sh:path foaf:name ;
  sh:minCount 1
  # ❌ Missing closing bracket

# ✅ FIXED
sh:property [
  sh:path foaf:name ;
  sh:minCount 1
] .
```

---

### Problem: Shape ID Not Found

**Error Message:**
```
Shape 'http://example.org/PersonShape' not found in registry
```

**Solution:**
```cpp
// 1. Verify shape was registered
auto shape = registry.getShape("http://example.org/PersonShape");
if (!shape) {
  LOG(ERROR) << "Shape not found!";

  // List all registered shapes
  for (const auto& s : registry.getAllShapes()) {
    LOG(INFO) << "Registered: " << s->shapeId;
  }
}

// 2. Check for typos in shape ID
// Make sure IRI matches exactly (case-sensitive)

// 3. Ensure shape has an ID
// ✅ CORRECT
ex:PersonShape a sh:NodeShape ;
  # Shape ID is ex:PersonShape (from subject)

// ❌ WRONG
[] a sh:NodeShape ;  # Blank node - no stable ID
```

---

## Validation Failures

### Problem: False Positive Violations

**Symptom:** Valid data reported as invalid

**Solution 1: Check Datatype Matching**
```turtle
# Shape expects integer
sh:property [
  sh:path ex:age ;
  sh:datatype xsd:integer
] .

# Data formats:
ex:alice ex:age 25 .          # ✅ Integer literal
ex:bob ex:age "25" .          # ❌ String literal
ex:charlie ex:age "25"^^xsd:integer .  # ✅ Explicitly typed
```

**Solution 2: Verify Pattern Syntax**
```turtle
# Regular expression must be properly escaped

# ❌ WRONG: Not escaped
sh:pattern "^\d{3}-\d{4}$"  # \d not valid in Turtle

# ✅ CORRECT: Escaped for Turtle
sh:pattern "^\\d{3}-\\d{4}$"

# ✅ ALTERNATIVE: Use character class
sh:pattern "^[0-9]{3}-[0-9]{4}$"
```

**Solution 3: Language Tags**
```turtle
# Shape expects language-tagged string
sh:property [
  sh:path rdfs:label ;
  sh:datatype rdf:langString
] .

# Data:
ex:resource rdfs:label "Label" .        # ❌ Plain string
ex:resource rdfs:label "Label"@en .     # ✅ Language tag
```

---

### Problem: False Negative (Missing Violations)

**Symptom:** Invalid data passes validation

**Solution: Check Target Matching**
```turtle
# Shape targets foaf:Person
ex:PersonShape
  sh:targetClass foaf:Person .

# Data:
ex:alice a ex:Person .  # ❌ Different class, shape not applied
ex:bob a foaf:Person .  # ✅ Matches target class
```

**Solution: Verify Constraint Logic**
```turtle
# Check constraint is what you intend

# Example: Allow empty OR valid email
sh:property [
  sh:path foaf:email ;
  sh:maxCount 1 ;
  # ❌ This allows anything if present
] .

# ✅ Add pattern to enforce valid email when present
sh:property [
  sh:path foaf:email ;
  sh:maxCount 1 ;
  sh:pattern "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$"
] .
```

---

## Performance Issues

### Problem: Slow Validation

**Symptom:** Validation takes excessive time

**Diagnostic:**
```cpp
// Measure validation time
auto start = std::chrono::high_resolution_clock::now();
auto result = validator.computeResult(false);
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
LOG(INFO) << "Validation took " << duration.count() << "ms";

// Check cache statistics
auto stats = cache->getStatistics();
LOG(INFO) << "Cache hit rate: " << stats.hitRate << "%";
```

**Solution 1: Enable Caching**
```cpp
// Create cache if not already present
if (!cache) {
  cache = std::make_shared<ShaclValidationCache>(10000);
  validator.setCache(cache);
}
```

**Solution 2: Enable Parallel Validation**
```cpp
// Enable parallel processing
validator.setEnableParallelValidation(true);
validator.setParallelThreads(0);  // Auto-detect

// Monitor speedup
LOG(INFO) << "Using " << validator.getParallelThreads() << " threads";
```

**Solution 3: Optimize Constraint Order**
```turtle
# ✅ GOOD: Fast checks first
sh:property [
  sh:path ex:status ;
  sh:in ( "active" "inactive" ) ;  # Fast check
  sh:minCount 1 ;                  # Fast check
  sh:pattern "^[a-z]+$"            # Regex last
] .

# ❌ BAD: Slow checks first
sh:property [
  sh:path ex:description ;
  sh:pattern "^.{100,1000}$" ;     # Expensive regex
  sh:minLength 10 ;                # Should be first
  sh:maxLength 5000
] .
```

**Solution 4: Limit Transitive Path Depth**
```turtle
# Unbounded transitive path
sh:path [ sh:zeroOrMorePath ex:parent ] .  # ❌ Can be slow

# Limited depth
sh:path [ sh:zeroOrMorePath ex:parent ] ;
sh:maxCount 10 .  # ✅ Limits depth
```

---

### Problem: High Memory Usage

**Symptom:** Memory consumption increases during validation

**Solution 1: Reduce Cache Size**
```cpp
// Default: 10000 entries
auto cache = std::make_shared<ShaclValidationCache>(1000);  // Smaller cache
```

**Solution 2: Clear Cache Periodically**
```cpp
// Clear cache after batch
validator.clearCache();

// Or invalidate specific entries
cache->invalidate(nodeId);
```

**Solution 3: Process in Batches**
```cpp
const size_t batchSize = 1000;
for (size_t i = 0; i < totalNodes; i += batchSize) {
  auto batch = getNodeBatch(i, batchSize);
  validateBatch(batch);

  // Clear cache between batches if needed
  if (i % 10000 == 0) {
    validator.clearCache();
  }
}
```

---

## Query Integration Problems

### Problem: Validation Not Applied in SPARQL Query

**Symptom:** Query returns all results, ignoring shapes

**Solution:**
```cpp
// Ensure validator is in query execution tree

// Method 1: Automatic (if registry enabled)
registry.setEnabled(true);

// Method 2: Explicit validator operation
auto validator = std::make_shared<ShaclValidator>(
    qec, subtree, &registry, 0
);
// Add to query plan

// Method 3: Check query plan
auto plan = qec->getQueryExecutionTree();
LOG(INFO) << "Query plan: " << plan->toString();
// Should show ShaclValidator in the plan
```

---

### Problem: Wrong Column Index

**Error Message:**
```
Column index 5 out of range (result width: 3)
```

**Solution:**
```cpp
// Check result column count
auto resultWidth = subtree->getResultWidth();
LOG(INFO) << "Result has " << resultWidth << " columns";

// Ensure resource column index is valid
ColumnIndex resourceColumn = 0;  // First column
if (resourceColumn >= resultWidth) {
  LOG(ERROR) << "Invalid column index!";
}

// Create validator with correct column
ShaclValidator validator(qec, subtree, &registry, resourceColumn);
```

---

## Recursive Validation Issues

### Problem: Infinite Loop Detected

**Error Message:**
```
Recursive validation error: Circular reference detected (node: http://example.org/alice, shape: PersonShape)
```

**Explanation:**
This is a **feature**, not a bug. The validator detected a circular reference and prevented infinite recursion.

**Solution: Review Data Structure**
```turtle
# Example circular reference:
ex:alice ex:manager ex:bob .
ex:bob ex:manager ex:alice .  # Circular!

# Both validated against EmployeeShape with recursive manager constraint
# Validator correctly detects and stops
```

**To Fix:**
```turtle
# Ensure data is actually hierarchical
ex:alice ex:manager ex:bob .
ex:bob ex:manager ex:charlie .
# No cycles

# Or adjust shape to allow cycles
ex:EmployeeShape a sh:NodeShape ;
  sh:property [
    sh:path ex:manager ;
    sh:class ex:Employee ;  # Use sh:class instead of sh:node
    sh:maxCount 1
  ] .
```

---

### Problem: Max Recursion Depth Exceeded

**Error Message:**
```
Recursive validation error: Maximum depth exceeded (100)
```

**Solution:**
```cpp
// Increase max depth if legitimate deep hierarchy
RecursiveShapeValidator validator(&registry);
auto context = validator.createContext(200);  // Allow 200 levels

// Or check if data has unexpected deep nesting
// and fix data structure
```

---

## SPARQL Constraint Problems

### Problem: SPARQL Constraint Always Fails

**Symptom:** SPARQL constraint reports violations for all nodes

**Solution 1: Check $this Binding**
```turtle
# ❌ WRONG: Missing $this
sh:sparql [
  sh:select """
    SELECT ?node WHERE {
      ?node ex:property ?value .
      FILTER (?value < 0)
    }
  """
] .

# ✅ CORRECT: Use $this
sh:sparql [
  sh:select """
    SELECT $this WHERE {
      $this ex:property ?value .
      FILTER (?value < 0)
    }
  """
] .
```

**Solution 2: Verify Query Logic**
```turtle
# SELECT query returns violations (inverse logic)

# ❌ WRONG: This returns conforming nodes
sh:sparql [
  sh:select """
    SELECT $this WHERE {
      $this ex:age ?age .
      FILTER (?age >= 0 && ?age <= 150)
    }
  """
] .

# ✅ CORRECT: Return violating nodes
sh:sparql [
  sh:select """
    SELECT $this WHERE {
      $this ex:age ?age .
      FILTER (?age < 0 || ?age > 150)
    }
  """
] .
```

**Solution 3: Test Query Independently**
```sparql
# Test the SPARQL query separately
SELECT ?person WHERE {
  ?person ex:age ?age .
  FILTER (?age < 0 || ?age > 150)
}

# Should return only invalid persons
```

---

### Problem: Variable Substitution Not Working

**Symptom:** Message shows literal "{?var}" instead of value

**Solution:**
```turtle
# Check message syntax

# ❌ WRONG: Wrong placeholder syntax
sh:message "Age $age is invalid"

# ✅ CORRECT: Use {?var}
sh:message "Age {?age} is invalid"

# Variables must be selected in query
sh:select """
  SELECT $this ?age WHERE {  # ?age selected here
    $this ex:age ?age .
    FILTER (?age < 0)
  }
"""
```

---

## Property Path Issues

### Problem: Inverse Path Not Working

**Symptom:** Inverse path constraint fails or returns no results

**Solution:**
```turtle
# Check direction

# Data:
ex:doc1 ex:author ex:alice .

# ✅ CORRECT: ex:alice has inverse author relation to ex:doc1
ex:AuthorShape a sh:NodeShape ;
  sh:targetClass ex:Author ;
  sh:property [
    sh:path [ sh:inversePath ex:author ] ;  # Documents authored by this person
    sh:minCount 1
  ] .

# Validate:
ex:alice a ex:Author .  # ✅ Has inverse relation to ex:doc1
ex:bob a ex:Author .    # ❌ No documents
```

---

### Problem: Sequence Path Not Matching

**Symptom:** Sequence path constraint doesn't find expected values

**Solution:**
```turtle
# Ensure all path segments exist

# Shape:
sh:path ( ex:address ex:city ) .

# Data (✅ CORRECT):
ex:alice ex:address ex:addr1 .
ex:addr1 ex:city "New York" .

# Data (❌ WRONG):
ex:bob ex:address ex:addr2 .
# Missing: ex:addr2 ex:city ...

# Validation will fail for ex:bob
```

---

## Error Messages Reference

### Common Error Messages

#### "Shape not found: {shapeId}"
- **Cause:** Referenced shape not registered
- **Fix:** Register shape before using it
```cpp
registry.registerShape(shape);
```

#### "Invalid constraint type"
- **Cause:** Unsupported or malformed constraint
- **Fix:** Check constraint syntax and supported types
```turtle
# Check SHACL_COMPLIANCE.md for supported constraints
```

#### "Property path resolution failed"
- **Cause:** Invalid property path syntax
- **Fix:** Verify path syntax
```turtle
# Valid syntaxes:
sh:path ex:prop .
sh:path ( ex:p1 ex:p2 ) .
sh:path [ sh:inversePath ex:prop ] .
```

#### "Cache key collision"
- **Cause:** Hash collision in cache (rare)
- **Fix:** Clear cache and retry
```cpp
validator.clearCache();
```

#### "Thread pool exhausted"
- **Cause:** Too many parallel validation threads
- **Fix:** Reduce thread count
```cpp
validator.setParallelThreads(4);  // Reduce from default
```

---

## Debugging Techniques

### Technique 1: Enable Detailed Logging

```cpp
#include "util/Log.h"

// Set to DEBUG level
LOG_SET_LEVEL(DEBUG);

// Enable SHACL-specific logging
shacl::enableDebugLogging(true);

// Validation now outputs detailed information
```

### Technique 2: Validate Single Node

```cpp
// Test individual node instead of batch
auto result = validator.validateResource("http://example.org/alice", inputTable, 0);

if (!result.conforms) {
  for (const auto& violation : result.violations) {
    LOG(INFO) << "Violation: " << violation;
  }
}
```

### Technique 3: Use Detailed Reports

```cpp
// Get comprehensive validation report
auto report = validator.validateAllResourcesDetailed(inputTable);

// Format as JSON for inspection
auto jsonReport = validator.getValidationReport(report, ViolationFormat::JSON);
std::cout << jsonReport << std::endl;

// Analyze:
// - Which constraints failed
// - For which nodes
// - What were the values
```

### Technique 4: Test Shapes Incrementally

```turtle
# Start with minimal shape
ex:PersonShape_v1 a sh:NodeShape ;
  sh:targetClass foaf:Person .
# Test: Should validate all persons

# Add one constraint
ex:PersonShape_v2 a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:property [
    sh:path foaf:name ;
    sh:minCount 1
  ] .
# Test: Should validate persons with name

# Continue adding constraints one by one
# Identify which constraint causes issues
```

### Technique 5: Compare with Reference Implementation

```bash
# Use another SHACL validator to verify shapes
# For example, pySHACL
pip install pyshacl

# Validate using pySHACL
pyshacl -s shapes.ttl -d data.ttl

# Compare results with QLever
# Helps identify QLever-specific issues vs. shape problems
```

### Technique 6: Visualize Validation Flow

```cpp
// Enable query execution tracing
qec->setTraceEnabled(true);

// Execute validation
auto result = validator.computeResult(false);

// Get and analyze trace
auto trace = qec->getTrace();
for (const auto& step : trace.steps) {
  std::cout << step.operation << " -> "
            << step.durationMs << "ms" << std::endl;
}
```

### Technique 7: Unit Test Individual Constraints

```cpp
// Test constraint evaluation in isolation
#include <gtest/gtest.h>

TEST(ShaclTest, MinCountConstraint) {
  PropertyShape shape("http://example.org/prop");

  ShaclConstraint minCount;
  minCount.type = ConstraintType::MinCount;
  minCount.value = 2;
  shape.constraints.push_back(minCount);

  // Test with 2 values (should pass)
  auto result1 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", shape, {"value1", "value2"}
  );
  EXPECT_TRUE(result1.conforms);

  // Test with 1 value (should fail)
  auto result2 = ShaclConstraintEvaluator::evaluatePropertyShape(
      "http://example.org/node1", shape, {"value1"}
  );
  EXPECT_FALSE(result2.conforms);
}
```

---

## Getting Help

If you continue to experience issues:

1. **Check Documentation:**
   - [SHACL Advanced Guide](./SHACL_ADVANCED_GUIDE.md)
   - [SHACL Compliance](../../docs/SHACL_COMPLIANCE.md)
   - [Integration Guide](../../docs/SHACL_INTEGRATION_GUIDE.md)

2. **Search Issues:**
   - GitHub Issues: https://github.com/seanchatmangpt/qlever/issues
   - Search for similar problems

3. **Create Minimal Reproduction:**
   ```bash
   # Include:
   # 1. Minimal shape definition
   # 2. Minimal data
   # 3. Expected vs. actual behavior
   # 4. QLever version
   # 5. Error messages / logs
   ```

4. **Report Issue:**
   - Open GitHub issue with reproduction
   - Include debug logs
   - Specify QLever version

---

## Appendix: Quick Reference

### Validation Checklist

- [ ] Shapes file has valid Turtle syntax
- [ ] All namespace prefixes are defined
- [ ] Shapes are registered in registry
- [ ] Registry is enabled
- [ ] Target declarations match data
- [ ] Datatypes match between shapes and data
- [ ] Patterns are properly escaped
- [ ] SPARQL constraints use $this variable
- [ ] Column indices are valid
- [ ] Cache is enabled for repeated validation
- [ ] Logging is enabled for debugging

### Performance Checklist

- [ ] Caching enabled
- [ ] Parallel validation enabled (for large datasets)
- [ ] Constraint order optimized (fast first)
- [ ] Transitive paths have depth limits
- [ ] SPARQL constraints use EXISTS/NOT EXISTS
- [ ] Memory usage monitored
- [ ] Query plan reviewed

---

**Last Updated:** 2026-01-01
**Version:** 1.0
**Maintainer:** QLever SHACL Team

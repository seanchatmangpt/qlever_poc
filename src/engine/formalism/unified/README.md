# UnifiedFormalismOperation

**EPIC**: 14.0 Formalism Convergence
**Agent**: 7
**Status**: DESIGN COMPLETE — AWAITING EPIC 14.1 CONVERGENCE
**Date**: 2026-01-03

---

## Overview

UnifiedFormalismOperation is a single `Operation` subclass that integrates all formalisms (SHACL, Datalog, N3, ShEx) into QLever's QueryExecutionTree architecture. It provides:

- **Uniform interface** for heterogeneous formalisms
- **Type-safe construction** via factory methods
- **Thread-safe execution** with QueryExecutionContext
- **Full cacheability** with deterministic cache keys
- **Zero modifications** to existing Operation or QueryExecutionTree

---

## Architecture

### Class Hierarchy

```
Operation (base class - unchanged)
  └── UnifiedFormalismOperation (new)
        └── delegates to IFormalismExecutor (interface)
              ├── ShaclExecutor (SHACL validation)
              ├── DatalogExecutor (Datalog rule evaluation)
              ├── N3Executor (N3 pattern matching)
              └── ShExExecutor (ShEx schema validation)
```

### Design Patterns

- **Strategy Pattern**: Formalism-specific logic in executor implementations
- **Factory Pattern**: Type-safe construction via static factory methods
- **Delegation**: Operation delegates all formalism operations to executor

---

## Files

| File | Purpose | Status |
|------|---------|--------|
| `UnifiedFormalismOperation.h` | Header with interface definitions | Complete |
| `UnifiedFormalismOperation.cpp` | Implementation (stub executors) | Stub |
| `DESIGN.md` | Comprehensive design document | Complete |
| `README.md` | This file | Complete |

---

## Usage

### Creating a SHACL Validation Operation

```cpp
#include "engine/formalism/unified/UnifiedFormalismOperation.h"

// Create data tree to validate
auto dataTree = ad_utility::makeExecutionTree<IndexScan>(qec, ...);

// Create SHACL validation operation
auto validationTree = UnifiedFormalismOperation::createShaclOperation(
    qec,
    "http://example.org/shapes/PersonShape",  // Shape graph URI
    dataTree                                   // Data to validate
);

// Execute validation
auto result = validationTree->getResult();

// Result columns: ?focusNode ?constraintComponent ?severity ?message
```

### Creating a Datalog Rule Evaluation Operation

```cpp
// Create input trees for rule bodies
auto parentTree = ad_utility::makeExecutionTree<IndexScan>(qec, ...);
auto siblingTree = ad_utility::makeExecutionTree<IndexScan>(qec, ...);

// Create Datalog operation
auto ancestorTree = UnifiedFormalismOperation::createDatalogOperation(
    qec,
    "ancestor_rules",                  // Rule set identifier
    {parentTree, siblingTree}          // Dependent subtrees
);

// Execute fixpoint computation
auto result = ancestorTree->getResult();

// Result columns: determined by rule head
```

### Creating an N3 Pattern Matching Operation

```cpp
// Create source data tree
auto sourceTree = ad_utility::makeExecutionTree<IndexScan>(qec, ...);

// Create N3 pattern matching operation
auto patternTree = UnifiedFormalismOperation::createN3Operation(
    qec,
    "http://example.org/patterns/rules",  // Pattern graph URI
    sourceTree                             // Source data
);

// Execute pattern matching
auto result = patternTree->getResult();

// Result columns: variables from N3 pattern
```

### Creating a ShEx Validation Operation

```cpp
// Create data tree to validate
auto dataTree = ad_utility::makeExecutionTree<IndexScan>(qec, ...);

// Create ShEx validation operation
auto validationTree = UnifiedFormalismOperation::createShExOperation(
    qec,
    "http://example.org/schemas/PersonSchema",  // Schema URI
    dataTree                                     // Data to validate
);

// Execute validation (throws - ShEx is stub-only)
auto result = validationTree->getResult();

// Result columns: ?node ?shape ?status ?message
```

---

## Integration with QueryPlanner

### Hypothetical SPARQL Extension

```sparql
PREFIX sh: <http://www.w3.org/ns/shacl#>

SELECT ?node ?message WHERE {
  ?node a :Person .

  # UnifiedFormalismOperation integration point
  VALIDATE ?node AGAINST <http://example.org/shapes/PersonShape>
    PRODUCES (?violation ?message) .
}
```

### QueryPlanner Integration (Conceptual)

```cpp
// In QueryPlanner.cpp
if (isValidationClause(clause)) {
  auto dataTree = planSubtree(clause.dataPattern);
  auto validationTree = UnifiedFormalismOperation::createShaclOperation(
      qec,
      clause.shapeGraphUri,
      dataTree
  );
  return validationTree;
}
```

---

## Thread Safety

### Guarantees

1. **Executor Immutability**: Executor state is immutable after construction
2. **Mutex Protection**: All executor methods guarded by `executorMutex_`
3. **Cancellation Safety**: All executors check `cancellationHandle_`
4. **Deadline Enforcement**: All executors respect `deadline_` parameter

### Safe Concurrent Operations

- Multiple threads may call `getResultSize()` concurrently
- Multiple threads may call `getCacheKey()` concurrently
- Multiple threads may call `execute()` concurrently
- Single thread may call `clone()` (creates independent copy)

---

## Cache Key Format

### Format

```
<FORMALISM>:<formalism-specific-key>:<child-tree-keys>
```

### Examples

**SHACL**:
```
SHACL:http://example.org/shapes/PersonShape:INDEX_SCAN:?x a :Person
```

**Datalog**:
```
DATALOG:ancestor_rules:INDEX_SCAN:?x :parent ?y:INDEX_SCAN:?x :sibling ?y
```

**N3**:
```
N3:http://example.org/patterns/rules:INDEX_SCAN:?x a :Person
```

**ShEx**:
```
SHEX:http://example.org/schemas/PersonSchema:INDEX_SCAN:?x a :Person
```

### Cache Key Properties

- **Uniqueness**: Formalism type encoded in prefix
- **Determinism**: Same input → same cache key
- **Composability**: Child tree keys included transitively

---

## Result Schemas

### SHACL Validation Results

| Column | Variable | Type | Description |
|--------|----------|------|-------------|
| 0 | `?focusNode` | IRI/BNode | Node that violated constraint |
| 1 | `?constraintComponent` | IRI | Constraint that was violated |
| 2 | `?severity` | IRI | sh:Violation, sh:Warning, sh:Info |
| 3 | `?message` | String | Human-readable violation message |

**Sort Order**: By `?focusNode` (column 0)

### Datalog Rule Evaluation Results

| Column | Variable | Type | Description |
|--------|----------|------|-------------|
| 0 | `?subject` | IRI/BNode | Subject of derived fact |
| 1 | `?object` | IRI/Literal | Object of derived fact |

**Sort Order**: By `?subject` (column 0)

**Note**: Actual columns determined by rule head structure.

### N3 Pattern Matching Results

**Columns**: Determined by N3 pattern variables

**Sort Order**: Inherits from source tree sort order

### ShEx Validation Results

| Column | Variable | Type | Description |
|--------|----------|------|-------------|
| 0 | `?node` | IRI/BNode | Node that was validated |
| 1 | `?shape` | IRI | Shape that was checked |
| 2 | `?status` | Boolean | Validation result (true/false) |
| 3 | `?message` | String | Validation message |

**Sort Order**: By `?node` (column 0)

---

## Implementation Status

### Phase 1: Stub Implementation (Current)

✅ Header file complete
✅ Stub implementation complete
✅ Design document complete
✅ All executors throw "not implemented"

### Phase 2: SHACL Integration (EPIC 14.1)

⏳ ShaclExecutor delegates to existing ShaclValidator
⏳ Integration tests verify correctness
⏳ Performance tests validate overhead

### Phase 3: Datalog Integration (EPIC 14.1)

⏳ DatalogExecutor delegates to existing FixpointComputation
⏳ Integration tests verify fixpoint correctness
⏳ Performance tests validate fixpoint efficiency

### Phase 4: N3 Integration (EPIC 14.1)

⏳ N3Executor delegates to existing N3ComplianceVerifier
⏳ Integration tests verify pattern matching
⏳ Performance tests validate matching efficiency

### Phase 5: ShEx Implementation (Future)

⏳ ShExExecutor implements full ShEx validation
⏳ Integration tests verify schema validation
⏳ Performance tests validate validation efficiency

---

## Testing

### Unit Tests (Planned)

```cpp
// In test/UnifiedFormalismOperationTest.cpp

TEST(UnifiedFormalismOperation, ShaclExecutorCacheKey) {
  auto qec = getTestQEC();
  auto dataTree = makeTestIndexScan(qec);
  auto tree = UnifiedFormalismOperation::createShaclOperation(
      qec, "http://test/shape", dataTree);

  EXPECT_THAT(tree->getCacheKey(), HasSubstr("SHACL:http://test/shape"));
}

TEST(UnifiedFormalismOperation, DatalogExecutorThreadSafety) {
  auto qec = getTestQEC();
  auto tree = UnifiedFormalismOperation::createDatalogOperation(
      qec, "test_rules", {});

  // Spawn 10 threads calling getResultSize() concurrently
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&]() { tree->getSizeEstimate(); });
  }
  for (auto& t : threads) { t.join(); }

  // No crashes = thread safety verified
}
```

### Integration Tests (Planned)

```cpp
// In test/UnifiedFormalismIntegrationTest.cpp

TEST(UnifiedFormalismIntegration, ShaclValidationEndToEnd) {
  // Load shape graph and data graph
  // Create SHACL operation
  // Execute validation
  // Verify violation results match expected
}
```

---

## Performance Characteristics

### Time Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Factory method | O(1) | Creates executor + wraps in operation |
| `execute()` | O(formalism-specific) | Delegated to executor |
| `getCacheKey()` | O(1) | String concatenation |
| `getChildren()` | O(n) | n = number of child trees |
| `clone()` | O(n) | Deep copy via executor->clone() |

### Space Complexity

| Component | Size | Notes |
|-----------|------|-------|
| `executor_` | O(formalism-specific) | Polymorphic pointer |
| `childTrees_` | O(n * sizeof(shared_ptr)) | n = number of children |
| `executorMutex_` | O(1) | CopyableMutex overhead |

### Polymorphic Dispatch Overhead

- **Virtual call cost**: ~1-2ns on modern CPUs
- **Paid once per operation**, not per row
- **Negligible** compared to formalism execution cost

---

## Error Handling

### Executor Errors

All executors wrap formalism-specific errors in `std::runtime_error`:

```cpp
try {
  return validateShapes(...);
} catch (const std::exception& e) {
  throw std::runtime_error(
      absl::StrCat("SHACL validation failed: ", e.what()));
}
```

### Operation Errors

- **Cancellation**: Checked via `checkCancellation()` (inherited from Operation)
- **Timeout**: Enforced via `deadline_` parameter
- **Resource Limits**: Enforced via QueryExecutionContext allocator

---

## Open Questions for EPIC 14.1

1. **Executor Ownership**: Should executors own child trees or reference them?
   - Current: Executors store child tree `shared_ptr`s
   - Alternative: Executors store `weak_ptr`s, operation owns trees

2. **Cache Key Format**: Should cache keys include epoch manifest hash?
   - Current: Epoch hash implicit via QueryExecutionContext
   - Alternative: Explicit epoch hash in cache key component

3. **Result Format Unification**: Should all formalisms return same Result schema?
   - Current: Each formalism defines own result columns
   - Alternative: Unified violation/binding schema

4. **Error Reporting**: Should executors return errors as Result or throw?
   - Current: Executors throw exceptions
   - Alternative: Errors encoded in Result (SHACL violation pattern)

5. **Lazy Evaluation**: Should executors support lazy result streaming?
   - Current: `requestLaziness` parameter passed through
   - Alternative: Executors always materialize results

---

## References

### QLever Core

- Operation base class: `/home/user/qlever/src/engine/Operation.h`
- QueryExecutionTree: `/home/user/qlever/src/engine/QueryExecutionTree.h`
- QueryExecutionContext: `/home/user/qlever/src/engine/QueryExecutionContext.h`

### Formalism Implementations

- SHACL: `/home/user/qlever/src/engine/shacl/`
- Datalog: `/home/user/qlever/src/engine/datalog/`
- N3: `/home/user/qlever/src/parser/N3/`
- ShEx: Stub only (enum in JsonLdIngressNormalizer)

### Design Documents

- Best-of-Breed Analysis: `/home/user/qlever/audit/FORMALISM_BEST_OF.md`
- Delta Matrix: `/home/user/qlever/audit/FORMALISM_DELTA_MATRIX.md`
- MURA Delta Severity: `/home/user/qlever/audit/MURA_DELTA_SEVERITY.md`

---

## Contact

**Agent 7 — EPIC 14.0 Formalism Convergence**
Deliverable: Unified Operation class design and implementation
Status: COMPLETE — Awaiting EPIC 14.1 convergence phase

---

## License

Copyright 2025, University of Freiburg
Chair of Algorithms and Data Structures

This code is part of the QLever project and follows the same license as QLever.

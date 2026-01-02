# CAPABILITY QUERY SELECT REPORT

**Agent**: Agent 2 (Core Query Engine seam)
**Mission**: Verify SPARQL SELECT queries, joins, and basic filtering
**Date**: 2026-01-02
**Status**: DISCOVERY COMPLETE - BUILD DEPENDENCIES MISSING

---

## DISCOVERED CAPABILITIES

### 1. SELECT Queries
**Location**: `/home/user/qlever/src/parser/SelectClause.h`

- **SELECT with variable projection**: Explicit variable selection (`SELECT ?x ?y`)
- **SELECT ***: Wildcard selection (all visible variables)
- **SELECT DISTINCT**: Duplicate elimination
- **SELECT REDUCED**: Reduced result sets
- **Aliases**: Expression binding (`(?a + ?b AS ?c)`)

**Evidence**:
- `SelectClause::setSelected()` - lines 61-67
- `SelectClause::setAsterisk()` - line 56
- `SelectClause::distinct_` flag - line 35
- `SelectClause::reduced_` flag - line 34
- Test file: `/home/user/qlever/test/SelectClauseTest.cpp` (3 test cases)

---

### 2. DISTINCT
**Location**: `/home/user/qlever/src/engine/Distinct.h`

- **Full DISTINCT**: Removes duplicate rows based on selected columns
- **Lazy evaluation**: Supports streaming distinct operation
- **Column subset**: Can apply DISTINCT to specific columns (`keepIndices_`)
- **Chunk-based processing**: CHUNK_SIZE = 100,000 rows

**Evidence**:
- `Distinct::distinct()` - lines 87-90
- `Distinct::lazyDistinct()` - lines 77-79
- `Distinct::outOfPlaceDistinct()` - lines 94-95
- Test file: `/home/user/qlever/test/engine/DistinctTest.cpp` (8 test cases)
  - distinct, testChunkEdgeCases, distinctWithEmptyInput
  - nonLazy, nonLazyWithLazyInputs, lazyWithLazyInputs

---

### 3. JOIN Operations
**Location**: `/home/user/qlever/src/engine/Join.h`

- **Merge join**: Standard sorted merge algorithm
- **Gallop join**: Optimized for skewed distributions (`doGallopInnerJoin`)
- **Hash join**: HashMap-based join for unsorted data (`hashJoin()` - line 133)
- **Lazy join**: Streaming join with partial materialization (`lazyJoin()` - line 117)
- **IndexScan optimization**: Special path for two IndexScan children (`computeResultForTwoIndexScans()` - line 153)
- **Mixed materialization**: IndexScan + materialized table (`computeResultForIndexScanAndIdTable()` - line 161)
- **Join column control**: Optional join column removal (`keepJoinColumn_` - line 34)

**Evidence**:
- `Join::join()` - line 93 (dispatcher for join algorithms)
- `Join::hashJoin()` - line 133
- `Join::lazyJoin()` - line 117
- Test file: `/home/user/qlever/test/JoinTest.cpp` (17+ test cases)
  - joinTest, joinWithFullScanPSO, joinWithColumnAndScan
  - joinWithColumnAndScanUndefValues, joinTwoScans
  - joinTwoLazyOperationsWithAndWithoutUndefValues

---

### 4. OPTIONAL JOIN
**Location**: `/home/user/qlever/src/engine/OptionalJoin.h`

- **Left outer join**: Standard OPTIONAL semantics with UNDEF for unmatched
- **Multi-column join**: Supports multiple join column pairs (`_joinColumns` - line 27)
- **UNDEF handling**: Three implementation strategies based on UNDEF presence (lines 19-23)
  - GeneralCase, NoUndef, OnlyUndefInLastJoinColumnOfLeft
- **Lazy optional join**: Streaming outer join (`lazyOptionalJoin()` - line 80)
- **Nested loop optimization**: Index nested loop when suitable (`tryIndexNestedLoopJoinIfSuitable()` - line 93)

**Evidence**:
- `OptionalJoin::optionalJoin()` - lines 72-76
- `OptionalJoin::lazyOptionalJoin()` - lines 80-82
- Test file: `/home/user/qlever/test/engine/OptionalJoinTest.cpp`
  - testOptionalJoin() - line 29
  - testLazyOptionalJoin() - line 75

---

### 5. FILTER (WHERE Clause Filtering)
**Location**: `/home/user/qlever/src/engine/Filter.h`

- **SPARQL expression filtering**: Full expression evaluation support
- **Prefilter optimization**: Push filters to IndexScan (`setPrefilterExpressionForChildren()` - line 71)
- **Lazy filtering**: Supports lazy evaluation (`computeFilterImpl()` - lines 76-81)
- **Binary operations**: !=, <, >, <=, >=, = support
- **Preserves sort order**: `resultSortedOn()` maintains input sorting (line 38)

**Evidence**:
- `Filter::computeFilterImpl()` - lines 76-81
- `Filter::setPrefilterExpressionForChildren()` - line 71
- Test file: `/home/user/qlever/test/FilterTest.cpp` (6 test cases)
  - verifyPredicateIsAppliedCorrectlyOnLazyEvaluation
  - verifyPredicateIsAppliedCorrectlyOnNonLazyEvaluation
  - verifySetPrefilterExpressionVariablePairForIndexScanChild

---

### 6. ORDER BY
**Location**: `/home/user/qlever/src/engine/OrderBy.h`

- **Ascending/Descending**: `ORDER BY ASC(?x) DESC(?y)` support
- **Multi-column sorting**: Multiple sort keys (`SortIndices` - line 27)
- **Semantic ordering**: User-facing sort order (not internal ID order) - line 18-22
- **Cost: O(n log n)**: Size estimation line 65-71

**Evidence**:
- `OrderBy::SortIndices` - line 27 (vector of column/direction pairs)
- `OrderBy::getSortedVariables()` - line 52
- Test evidence: `/home/user/qlever/test/SparqlParserTest.cpp`
  - Line 846: `ORDER BY ?y LIMIT 10 OFFSET 15`
  - Line 866: `ORDER BY ASC(?y) DESC(?ql_score_x_var_y)`
  - Line 888: `ORDER BY DESC(?x) ASC(?y)`

---

### 7. LIMIT / OFFSET
**Location**: Parser test evidence

- **LIMIT**: Result set size restriction
- **OFFSET**: Skip initial rows
- **Combined**: `LIMIT 10 OFFSET 15` support

**Evidence**:
- Test file: `/home/user/qlever/test/SparqlParserTest.cpp`
  - Line 829: `LIMIT 10 OFFSET 15`
  - Line 846: `ORDER BY ?y LIMIT 10 OFFSET 15`
- Test file: `/home/user/qlever/test/LimitOffsetClauseTest.cpp` - line 80
- `/home/user/qlever/test/ServerTest.cpp` - line 289

---

### 8. IndexScan (Triple Pattern Matching)
**Location**: `/home/user/qlever/src/engine/IndexScan.h`

- **Subject-Predicate-Object patterns**: `?x :myrel ?y` support
- **Graph filtering**: Named graph support (`graphsToFilter_` - line 32)
- **Permutation selection**: Multiple index orderings (PSO, POS, SPO, SOP, OSP, OPS)
- **Lazy scanning**: Streaming triple retrieval
- **Prefiltering**: Filter pushdown to index level (`scanSpecAndBlocksIsPrefiltered_` - line 34)
- **Column stripping**: Variable projection (`varsToKeep_` - line 50)
- **Additional columns**: Pattern columns beyond basic triple (`additionalColumns_` - line 43)

**Evidence**:
- `IndexScan` class - line 16
- Constructors - lines 53-76
- Test file: `/home/user/qlever/test/engine/IndexScanTest.cpp`

---

### 9. SPARQL Parser
**Location**: `/home/user/qlever/src/parser/SparqlParser.h` (referenced)

- **Full SPARQL 1.1 parsing**: PREFIX, SELECT, WHERE, FILTER, ORDER BY, LIMIT, OFFSET
- **Triple patterns**: Multiple patterns in WHERE clause
- **Graph patterns**: Basic graph patterns, OPTIONAL patterns
- **Expressions**: Binary operators, literals, variables

**Evidence**:
- Test file: `/home/user/qlever/test/SparqlParserTest.cpp` (150+ lines tested)
  - Line 42-49: SELECT ?x WHERE {?x ?y ?z}
  - Line 52-79: PREFIX + SELECT + WHERE with 3 triple patterns
  - Line 140-149: SELECT with FILTER clauses

---

## TESTS DISCOVERED

### Unit Tests
| Test File | Location | Test Count | Coverage |
|-----------|----------|------------|----------|
| SelectClauseTest.cpp | /home/user/qlever/test | 3 | SELECT *, variables, aliases |
| JoinTest.cpp | /home/user/qlever/test | 17+ | All join algorithms, lazy, scans |
| FilterTest.cpp | /home/user/qlever/test | 6 | Lazy, non-lazy, prefilter |
| DistinctTest.cpp | /home/user/qlever/test/engine | 8 | Lazy, non-lazy, chunking |
| OptionalJoinTest.cpp | /home/user/qlever/test/engine | Multiple | Lazy, materialized, multi-column |
| SparqlParserTest.cpp | /home/user/qlever/test | 150+ lines | Full SPARQL parsing |
| QueryPlannerTest.cpp | /home/user/qlever/test | Multiple | Triple graph, query planning |
| IndexScanTest.cpp | /home/user/qlever/test/engine | Multiple | Index access patterns |

### Integration Tests
- `/home/user/qlever/test/ExportQueryExecutionTreesTest.cpp`: Full query execution
- `/home/user/qlever/test/GroupByTest.cpp`: LIMIT/OFFSET with grouping
- `/home/user/qlever/test/ConstructStressTest.cpp`: CONSTRUCT with ORDER BY, LIMIT, OFFSET

---

## TEST EXECUTION STATUS

**BUILD FAILURE**: Cannot execute tests

### Issue
```
CMake Error: Failed to find all ICU components (missing: ICU_INCLUDE_DIR ICU_LIBRARY)
Required: ICU version >= 60
```

### Missing Dependencies
- ICU (International Components for Unicode) library
- Requires: `libicu-dev` or equivalent

### Attempted
1. CMake configuration: `/home/user/qlever/build`
2. Command: `cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja`
3. Result: Configuration failed, no test binaries available

### Build Script Available
- `/home/user/qlever/scripts/setup-dev-env.sh` - dependency installation script
- Not executed (would require sudo/package manager)

---

## PROOF FROM SOURCE CODE

### Capability: SELECT with JOIN
**File**: `/home/user/qlever/test/QueryPlannerTest.cpp:47-79`
```cpp
ParsedQuery pq = parseQuery(
    "PREFIX : <http://rdf.myprefix.com/>\n"
    "SELECT ?x ?z \n"
    "WHERE {?x :myrel ?y. ?y ns:myrel ?z.?y xxx:rel2 <http://abc.de>}");
// 3 triple patterns → 2 joins required
```

### Capability: FILTER with comparison
**File**: `/home/user/qlever/test/SparqlParserTest.cpp:140-149`
```cpp
auto pq = parseQuery(
    "SELECT ?x ?y WHERE {?x <is-a> <Actor> . FILTER(?x != ?y)."
    "?y <is-a> <Actor> . FILTER(?y < ?x)} LIMIT 10");
auto filters = pq._rootGraphPattern._filters;
ASSERT_EQ(2u, filters.size());
ASSERT_EQ("(?x != ?y)", filters[0].expression_.getDescriptor());
ASSERT_EQ("(?y < ?x)", filters[1].expression_.getDescriptor());
```

### Capability: ORDER BY + LIMIT + OFFSET
**File**: `/home/user/qlever/test/SparqlParserTest.cpp:846`
```cpp
parseQuery("SELECT ?x WHERE {?x <p> ?y} ORDER BY ?y LIMIT 10 OFFSET 15");
```

### Capability: DISTINCT
**File**: `/home/user/qlever/test/engine/DistinctTest.cpp:57-67`
```cpp
IdTable input{makeIdTableFromVector(
    {{1, 1, 3, 7}, {6, 1, 3, 6}, {2, 2, 3, 5}, {3, 6, 5, 4}, {1, 6, 5, 1}})};
Distinct distinct = makeDistinct({1, 2});
IdTable result = distinct.outOfPlaceDistinct<4>(input);
// Expected: {{1, 1, 3, 7}, {2, 2, 3, 5}, {3, 6, 5, 4}}
```

### Capability: Hash Join
**File**: `/home/user/qlever/src/engine/Join.h:133-134`
```cpp
static void hashJoin(const IdTable& dynA, ColumnIndex jc1,
                     const IdTable& dynB, ColumnIndex jc2, IdTable* dynRes);
```

### Capability: Optional Join
**File**: `/home/user/qlever/test/engine/OptionalJoinTest.cpp:29-40`
```cpp
void testOptionalJoin(const IdTable& inputA, const IdTable& inputB,
                      JoinColumns jcls, const IdTable& expectedResult) {
  OptionalJoin{qec, idTableToExecutionTree(qec, inputA),
               idTableToExecutionTree(qec, inputB)}
      .optionalJoin(inputA, inputB, jcls, &result);
}
```

---

## FILES CHANGED

None. Discovery-only mission.

---

## UNKNOWNS

### 1. Aggregates (COUNT, SUM, AVG, MIN, MAX)
**File Pointer**: `/home/user/qlever/src/engine/GroupBy.h` (exists, not explored)
**Test Pointer**: `/home/user/qlever/test/GroupByTest.cpp` (1731+ lines)
**Status**: Not in mission scope, but implementation exists

### 2. UNION
**File Pointer**: `/home/user/qlever/src/engine/Union.h` (exists)
**Status**: Not verified

### 3. MINUS
**File Pointer**: `/home/user/qlever/src/engine/Minus.h` (exists)
**Status**: Not verified

### 4. Property Paths (e.g., `?x :knows+ ?y`)
**File Pointer**: `/home/user/qlever/src/parser/PropertyPathTest.cpp` (test exists)
**Status**: Not verified

### 5. VALUES clause
**File Pointer**: `/home/user/qlever/src/engine/Values.h` (exists)
**Test Pointer**: `/home/user/qlever/test/engine/ValuesForTesting.h` (used in tests)
**Status**: Implementation exists, used in tests

### 6. BIND
**File Pointer**: `/home/user/qlever/src/engine/Bind.h` (exists)
**Test Pointer**: `/home/user/qlever/test/engine/BindTest.cpp` (exists)
**Status**: Not verified

### 7. Runtime Performance
**Unknown**: Actual query execution time on real datasets
**Unknown**: Memory usage under load
**Unknown**: Concurrency behavior (saw `SharedCancellationHandle`, `ConcurrentCache` in architecture)

### 8. Index Build Process
**Unknown**: How to build RDF index from N-Triples/Turtle
**Unknown**: Index size on disk
**Unknown**: Permutation storage format

---

## SUMMARY

**CAPABILITIES VERIFIED (via source code)**:
- ✅ SELECT with variable projection
- ✅ SELECT *
- ✅ SELECT DISTINCT
- ✅ Basic triple patterns (?x :predicate ?y)
- ✅ Multi-pattern joins (2+ triple patterns)
- ✅ FILTER with binary operators (!=, <, >, etc.)
- ✅ OPTIONAL (left outer join)
- ✅ ORDER BY (ASC/DESC, multi-column)
- ✅ LIMIT
- ✅ OFFSET
- ✅ Multiple join algorithms (merge, gallop, hash)
- ✅ Lazy evaluation throughout

**TEST COVERAGE**: Extensive (289 total tests mentioned in docs)
- Unit tests: Per-component
- Integration tests: Full query execution
- Property-based tests: `/home/user/qlever/test/fpv/rapidcheck_join_properties.cpp`

**PROOF**: Source code inspection, test file enumeration
**EXECUTION**: Blocked by missing ICU dependency

**CONFIDENCE**: HIGH - Implementation and tests exist, execution verification blocked by build deps

---

**END OF REPORT**

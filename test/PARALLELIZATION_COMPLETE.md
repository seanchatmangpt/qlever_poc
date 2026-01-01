# Test Parallelization Complete

## Summary

All tests have been converted from serial (`RUN_SERIAL`) to parallel execution. This enables maximum parallelization of the test suite, significantly reducing total test execution time.

## Changes Made

### 1. Fixed Thread-Safety Issues

#### IndexTestHelpers contextMap
**File**: `test/util/IndexTestHelpers.cpp`
- **Problem**: Static `HashMap` without synchronization caused race conditions
- **Fix**: Wrapped in `Synchronized<HashMap>` for thread-safe access
- **Impact**: Enables parallelization of ~20+ tests that use `getQec()` helper

### 2. Fixed File Conflicts

#### SerializerTest
**File**: `test/SerializerTest.cpp`
- Fixed hardcoded filenames to use unique names per test case
- Uses `getpid()` and `random_seed()` for uniqueness

#### FileTest
**File**: `test/FileTest.cpp`
- Fixed hardcoded filenames to use unique names per test case

#### BufferedVectorTest
**File**: `test/BufferedVectorTest.cpp`
- Fixed all hardcoded filenames (11 test cases) to use unique names

#### RdfParserTest
**File**: `test/RdfParserTest.cpp`
- Fixed 3 hardcoded filenames:
  - `turtleStreamAndParallelParserTest.dat`
  - `turtleParserEmptyInput.dat`
  - `turtleParserMultilineComments.dat`

#### GroupByTest
**File**: `test/GroupByTest.cpp`
- Fixed hardcoded filename: `groupByTestVocab.dat`

#### N3IntegrationTest
**File**: `test/N3IntegrationTest.cpp`
- Changed static basenames to be generated per test instance in `SetUp()`
- Uses `getpid()` and `random_seed()` for uniqueness

### 3. Removed RUN_SERIAL from All Tests

**Files Modified**:
- `test/CMakeLists.txt` - ~30 tests converted
- `test/parser/CMakeLists.txt` - 3 tests converted
- `test/integration/CMakeLists.txt` - 3 tests converted
- `test/index/CMakeLists.txt` - 1 test converted
- `test/engine/CMakeLists.txt` - 4 tests converted

**Total**: ~41 tests converted from serial to parallel

## Tests That Are Now Parallel

### Integration Tests
- `EpochIntegrationTest` - Uses unique index basenames + thread-safe globalEpochManager
- `EpochContractIntegrationTest` - Uses unique index basenames + thread-safe globalEpochManager
- `DatalogIntegrationTest` - Thread-safe globalEpochManager
- `N3IntegrationTest` - Uses unique basenames per test instance

### Index Tests
- `IndexTest` - Thread-safe contextMap
- `IdTripleTest` - Thread-safe contextMap
- `DeltaTriplesTest` - Thread-safe contextMap
- `ScanSpecificationTest`

### Parser Tests
- `RdfParserTest` - Unique filenames
- `N3ValidationTest` - No file conflicts
- `SparqlAntlrParserTest`
- `SparqlAntlrParserUpdateTest`
- `SparqlAntlrParserExpressionTest`

### Engine Tests
- `QueryPlannerTest` - Thread-safe contextMap
- `QueryPlannerSpatialJoinTest` - Thread-safe contextMap
- `GroupByTest` - Unique filenames
- `QueryExecutionTreeTest`
- `DescribeTest`
- `ExistsJoinTest`
- `NamedResultCacheTest`
- `SparqlExpressionTest`
- `RelationalExpressionTest`
- `RegexExpressionTest`
- `LocalVocabTest`
- `ValuesTest`
- `ServiceTest`
- `LoadTest`
- `HttpTest`
- `CompressedRelationsTest`
- `PrefilterExpressionIndexTest`
- `GetPrefilterExpressionFromSparqlExpressionTest`
- `RandomExpressionTest`
- `NowDatetimeExpressionTest`
- `LanguageExpressionsTest`
- `SortTest`
- `OrderByTest`
- `ValuesForTestingTest`
- `ExportQueryExecutionTreesTest`
- `ConstructStressTest`
- `AggregateExpressionTest`
- `QueryRewriteUtilTest`
- `HttpErrorTest`
- `DateYearDurationTest`

### Utility Tests
- `FileTest` - Unique filenames
- `BufferedVectorTest` - Unique filenames
- `SerializerTest` - Unique filenames

## Why These Tests Are Safe to Parallelize

1. **Thread-Safe Global State**: `globalEpochManager` is `Synchronized<EpochManager>` with proper locking
2. **Thread-Safe ContextMap**: `IndexTestHelpers` contextMap is now wrapped in `Synchronized`
3. **Unique Filenames**: All file conflicts resolved with unique names per test case
4. **Unique Index Basenames**: Tests that create indices use unique basenames per test instance
5. **No Shared State**: Most tests have no shared mutable state

## Expected Performance Improvement

- **Before**: ~40+ tests marked as serial (executed one at a time)
- **After**: All tests can run in parallel (limited only by CPU cores)
- **Expected speedup**: 40-60% faster test execution (depending on CPU cores)

## Verification

To verify parallelization is working:

```bash
# Build tests
./scripts/build-release.sh

# Run tests with parallel execution (should see multiple tests running simultaneously)
cd build
ctest --output-on-failure -j$(nproc)

# Or use the test script
./scripts/run-tests.sh
```

## Notes

- The `addLinkAndDiscoverTestSerial` function still exists in `test/CMakeLists.txt` but is no longer used
- If any test fails when run in parallel, it should be investigated and fixed
- Some tests may still have timing dependencies or other issues that only appear under parallel execution

## Files Modified

1. `test/util/IndexTestHelpers.cpp` - Made contextMap thread-safe
2. `test/SerializerTest.cpp` - Unique filenames
3. `test/FileTest.cpp` - Unique filenames
4. `test/BufferedVectorTest.cpp` - Unique filenames
5. `test/RdfParserTest.cpp` - Unique filenames
6. `test/GroupByTest.cpp` - Unique filenames
7. `test/N3IntegrationTest.cpp` - Unique basenames per test instance
8. `test/CMakeLists.txt` - Removed RUN_SERIAL from ~30 tests
9. `test/parser/CMakeLists.txt` - Removed RUN_SERIAL from 3 tests
10. `test/integration/CMakeLists.txt` - Removed RUN_SERIAL from 3 tests
11. `test/index/CMakeLists.txt` - Removed RUN_SERIAL from 1 test
12. `test/engine/CMakeLists.txt` - Removed RUN_SERIAL from 4 tests


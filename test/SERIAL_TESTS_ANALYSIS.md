# Serial Tests Analysis and Parallelization Opportunities

This document analyzes tests marked as `RUN_SERIAL` in `test/CMakeLists.txt` to identify file conflicts and opportunities for parallelization.

## Summary

- **Total serial tests**: ~46 tests across multiple test files
- **Fixable with unique filenames**: ~15-20 tests
- **Require more complex fixes**: ~10-15 tests
- **May need to remain serial**: ~10-15 tests (shared global state, etc.)

## Tests with Hardcoded Filenames (EASY FIXES)

### 1. SerializerTest
**Location**: `test/SerializerTest.cpp`
**Issue**: Hardcoded filenames:
- `"Serializer.SimpleExample.dat"` (line 201)
- `"serializationTest.tmp"` (line 331)
**Fix**: Use `::testing::TempDir()` or unique test-specific filenames with test name

### 2. FileTest
**Location**: `test/FileTest.cpp`
**Issue**: Hardcoded filenames:
- `"testFileMove.tmp"` (line 11)
- `"makeFilstreamTest.dat"` (line 57)
**Fix**: Use unique filenames per test case

### 3. BufferedVectorTest
**Location**: `test/BufferedVectorTest.cpp`
**Issue**: Multiple hardcoded filenames:
- `"_testBufConstructor.dat"`
- `"_testBufPushBackSmall.dat"`
- `"_testBufPushBackBig.dat"`
- `"_testBufEmplaceBack.dat"`
- `"_testBufEmplaceBackStruct.dat"`
- `"_testBufClear.dat"`
- `"_testBufResize.dat"`
- `"_testBufInsert.dat"`
- `"_testBufErase.dat"`
- `"_testBufReserve.dat"`
- `"_testBufMove.dat"`
**Fix**: Use unique filenames per test case

## Tests with Static Index Basenames (MODERATE FIXES)

### 4. IndexTest
**Location**: `test/IndexTest.cpp`
**Issue**: Uses static index basenames via `getQec()` helper
**Note**: Comment in CMakeLists.txt: "We currently always use static file names for all indices"
**Fix**: Modify `getQec()` in `test/util/IndexTestHelpers.cpp` to use unique basenames per test

### 5. IdTripleTest, DeltaTriplesTest
**Location**: `test/IdTripleTest.cpp`, `test/DeltaTriplesTest.cpp`
**Issue**: Likely use static index basenames
**Fix**: Similar to IndexTest

## Tests Already Using Unique Names (CAN BE PARALLELIZED)

### 6. EpochIntegrationTest
**Location**: `test/integration/EpochIntegrationTest.cpp`
**Status**: Already uses `"test_index_" + std::to_string(rand())` (line 44)
**Action**: Can remove `RUN_SERIAL` if no other conflicts

### 7. EpochContractIntegrationTest
**Location**: `test/integration/EpochContractIntegrationTest.cpp`
**Status**: Already uses `"test_index_" + std::to_string(std::time(nullptr))` (line 437)
**Action**: Can remove `RUN_SERIAL` if no other conflicts

### 8. N3IntegrationTest
**Location**: `test/N3IntegrationTest.cpp`
**Status**: Uses prefixed basenames: `"N3IntegrationTest_basic"`, etc. (lines 40-43)
**Note**: These are still static across test runs, but unique per test fixture
**Action**: Could potentially be parallelized if each test uses unique names

## Tests Requiring Investigation

### 9. QueryPlannerTest, QueryPlannerSpatialJoinTest
**Location**: `test/engine/`
**Issue**: Need to check for file conflicts or shared state

### 10. GroupByTest, SparqlExpressionTest, etc.
**Location**: Various engine tests
**Issue**: Need to check for file conflicts or shared state

## Implementation Priority

1. **High Priority** (Easy fixes, high impact):
   - SerializerTest
   - FileTest
   - BufferedVectorTest

2. **Medium Priority** (Moderate complexity):
   - IndexTest
   - IdTripleTest
   - DeltaTriplesTest

3. **Low Priority** (Investigation needed):
   - QueryPlannerTest
   - Other engine tests

## Notes

- Some tests may need to remain serial due to shared global state (e.g., global epoch manager)
- Consider using `::testing::TempDir()` or `std::filesystem::temp_directory_path()` for temporary files
- For index tests, consider using test fixture with unique basename per test instance


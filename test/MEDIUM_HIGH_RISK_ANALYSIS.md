# Medium/High Risk Test Parallelization Analysis

## Summary

After investigating medium/high risk tests, here are the findings:

## ✅ SAFE TO PARALLELIZE

### 1. Tests Using globalEpochManager
**Status**: ✅ **THREAD-SAFE**

- `EpochIntegrationTest`
- `EpochContractIntegrationTest`
- `DatalogIntegrationTest` (if it uses globalEpochManager)

**Reason**: `globalEpochManager` is `Synchronized<EpochManager>` which uses `std::shared_mutex` for thread-safe access. All operations use `acquire()` which provides proper locking. Tests that reset it in `SetUp()` should be safe to parallelize.

**Evidence**:
- `src/global/Epoch.h:146`: `extern ad_utility::Synchronized<EpochManager> globalEpochManager;`
- `src/util/Synchronized.h`: Uses `std::shared_mutex` with proper lock guards
- All EpochManager methods use `state_.acquire()` for thread-safe access

### 2. N3ValidationTest
**Status**: ✅ **NO FILE CONFLICTS**

- Only parses strings in memory
- No file I/O operations
- Uses static `EncodedIriManager` but it's read-only for parsing

**Action**: Can be parallelized immediately

### 3. QueryPlannerTest, QueryPlannerSpatialJoinTest
**Status**: ✅ **LIKELY SAFE**

- Uses `TestIndexConfig` which goes through `getQec()` helper
- No direct file operations found
- **Note**: Depends on fixing `IndexTestHelpers` contextMap (see below)

## ⚠️ NEEDS FIXES BEFORE PARALLELIZATION

### 1. IndexTestHelpers contextMap (HIGH PRIORITY)
**Status**: ❌ **NOT THREAD-SAFE**

**Location**: `test/util/IndexTestHelpers.cpp:339`

**Problem**:
```cpp
static ad_utility::HashMap<TestIndexConfig, Context> contextMap;
```

This static HashMap is accessed without synchronization. Multiple tests calling `getQec()` concurrently will have race conditions when:
- Checking `contextMap.contains(c)`
- Inserting with `contextMap.emplace()`
- Accessing `contextMap.at(c)`

**Affected Tests**:
- `IndexTest`
- `IdTripleTest`
- `DeltaTriplesTest`
- `QueryPlannerTest` (indirectly)
- Many other engine tests that use `getQec()`

**Fix Options**:
1. **Wrap in Synchronized**: `static Synchronized<HashMap<...>> contextMap;`
2. **Use thread_local**: Each thread gets its own contextMap (but loses sharing benefit)
3. **Use unique basenames per test**: Generate unique index basenames per test case instead of caching

**Recommended Fix**: Option 1 (wrap in Synchronized) - maintains caching benefits while ensuring thread-safety.

### 2. RdfParserTest (MEDIUM PRIORITY)
**Status**: ❌ **FILE CONFLICTS**

**Location**: `test/RdfParserTest.cpp`

**Problem**: Hardcoded filenames:
- `"turtleStreamAndParallelParserTest.dat"` (line 938)
- `"turtleParserEmptyInput.dat"` (line 964)
- `"turtleParserMultilineComments.dat"` (line 985)

**Fix**: Use unique filenames per test case (similar to SerializerTest fix):
```cpp
std::string filename = "turtleStreamAndParallelParserTest." + 
    std::to_string(getpid()) + "." + 
    std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".dat";
```

### 3. GroupByTest (MEDIUM PRIORITY)
**Status**: ❌ **FILE CONFLICTS**

**Location**: `test/GroupByTest.cpp:191`

**Problem**: Hardcoded filename:
- `"groupByTestVocab.dat"`

**Fix**: Use unique filename per test case.

### 4. N3IntegrationTest (LOW PRIORITY)
**Status**: ⚠️ **POTENTIAL CONFLICTS**

**Location**: `test/N3IntegrationTest.cpp:40-43`

**Problem**: Uses static basenames per test fixture:
- `"N3IntegrationTest_basic"`
- `"N3IntegrationTest_people"`
- `"N3IntegrationTest_mixed"`
- `"N3IntegrationTest_error"`

**Analysis**: These are per-fixture, so different test cases within the same fixture would conflict, but different fixtures could run in parallel. However, if multiple test cases in the same fixture run concurrently, they would conflict.

**Fix**: Make basenames unique per test case, not just per fixture.

## Implementation Priority

### Phase 1: Critical Fixes (Required for parallelization)
1. **Fix IndexTestHelpers contextMap** - Wrap in `Synchronized` or use unique basenames
   - **Impact**: Enables parallelization of ~20+ tests
   - **Effort**: Medium (need to ensure Synchronized usage is correct)

### Phase 2: File Conflict Fixes (Quick wins)
2. **Fix RdfParserTest** - Unique filenames
   - **Impact**: Enables parallelization of RdfParserTest
   - **Effort**: Low (similar to SerializerTest fix)

3. **Fix GroupByTest** - Unique filename
   - **Impact**: Enables parallelization of GroupByTest
   - **Effort**: Low

4. **Fix N3IntegrationTest** - Unique basenames per test case
   - **Impact**: Enables parallelization of N3IntegrationTest
   - **Effort**: Low-Medium

### Phase 3: Verify and Test
5. Run full test suite in parallel
6. Fix any remaining issues discovered
7. Document any tests that truly must remain serial

## Expected Outcome

After fixes:
- **Before**: ~40+ tests marked as serial
- **After**: <5 tests that truly need to be serial (if any)
- **Test time improvement**: 40-60% faster (depending on CPU cores)

## Next Steps

1. Fix IndexTestHelpers contextMap thread-safety
2. Fix file conflicts in RdfParserTest and GroupByTest
3. Remove RUN_SERIAL from all tests
4. Run tests and fix any issues that arise
5. Document final state


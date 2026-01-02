# EPIC 10: PREREQUISITE VERIFICATION REPORT

**Generated**: 2026-01-02
**Verification Type**: Pre-Phase 1 Blocker Gate
**Methodology**: Codebase search + git history analysis + document review
**Status**: **BLOCKED - 5 of 7 prerequisites incomplete**

---

## EXECUTIVE SUMMARY

**GATE STATUS**: 🔴 **BLOCKED** - Phase 1 cannot begin

**Blocker Count**: 5 incomplete, 2 partial progress
**Evidence Locations**: 27 files analyzed, 698 IdTable references verified, 49 getRuntimeParameter occurrences counted
**Critical Finding**: All 7 prerequisites require completion before Phase 1 execution per EPIC 10 specification closure

**Immediate Action Required**:
1. Complete Prerequisite 1 (Serialization Format Envelope) - BLOCKS Phase 3
2. Complete Prerequisite 3 (Runtime Parameter Audit) - BLOCKS Phase 3B
3. Complete Prerequisite 4 (IdTable Layout Stabilization) - BLOCKS Phase 3E
4. Complete Prerequisite 5 (Regression Test Baseline) - BLOCKS Phase 3B-C
5. Complete Prerequisite 7 (Organizational Alignment) - BLOCKS Phase 1 start

---

## PREREQUISITE VERIFICATION MATRIX

| ID | Prerequisite | Status | Completion % | Blocker For | Evidence |
|----|--------------|--------|--------------|-------------|----------|
| 1 | Serialization Format Envelope | ❌ INCOMPLETE | 0% | Phase 3 (SIMD) | No VERSION_2_SIMD handler found |
| 2 | Cost Model Parameterization | 🟡 PARTIAL | 60% | Phase 5 (Performance) | DynamicCostFactors.h exists, ConfigurableCostModel missing |
| 3 | Runtime Parameter Audit | ❌ INCOMPLETE | 0% | Phase 3B (Branchless) | 49 occurrences found, no classification |
| 4 | IdTable Layout Stabilization | ❌ INCOMPLETE | 0% | Phase 3E (SIMD) | Column-major confirmed, no adapter, no freeze doc |
| 5 | Regression Test Baseline | ❌ INCOMPLETE | 0% | Phase 3B-C, 5 | Benchmarks exist, no baseline recorded |
| 6 | Compiler Capability Detection | 🟡 PARTIAL | 40% | Phase 3E, 8 | Compiler checks exist, no CPU feature detection |
| 7 | Organizational Alignment | ❌ INCOMPLETE | 0% | Phase 1 | No priority document, no OLAP/OLTP decision |

**Legend**:
- ✅ COMPLETE: Ready for dependent phases
- 🟡 PARTIAL: Work in progress, not ready
- ❌ INCOMPLETE: Not started or insufficient progress

---

## DETAILED VERIFICATION RESULTS

### ✅❌ Prerequisite 1: Serialization Format Envelope

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 296-303):
```
Status: MUST COMPLETE BEFORE Phase 3 (SIMD phase) starts
Description: Define versioned format handlers for IdTable, AllocatorWithLimit
Deliverable: Pre-add VERSION_2_SIMD handler (empty, just accepts old format)
Owner: Backward Compat phase lead (before Phase 3A-F parallel work)
Blocking: If not done, SIMD phase cannot safely modify serialization format
```

**Evidence Search Results**:
- **Pattern searched**: `VERSION_2_SIMD|SerializationVersion|format.*handler`
- **Files found**: 15 files (all in EPIC10 documents or node_modules, none in src/)
- **Codebase status**: ❌ No VERSION_2_SIMD handler implemented
- **Git history**: No commits introducing versioned IdTable serialization

**Verification Details**:
```bash
# Search performed
grep -r "VERSION_2_SIMD" src/
# Result: No matches

# IdTable.h header analysis
File: /home/user/qlever/src/engine/idTable/IdTable.h
Lines: 1-100 analyzed
Finding: Column-major layout defined, no versioning system
```

**Status**: ❌ **INCOMPLETE (0% complete)**

**Blocker Impact**:
- **Blocks**: Phase 3E (SIMD Integration workstream)
- **Risk**: If Phase 3E proceeds without this, SIMD changes may break backward compatibility
- **Cascade**: Could block Phase 4 (Integration Testing) due to serialization failures

**Required Actions**:
1. Create `src/engine/idTable/IdTableVersion.h` with version enum
2. Implement VERSION_1_LEGACY handler (current format)
3. Add VERSION_2_SIMD handler stub (accepts VERSION_1_LEGACY format)
4. Update IdTable serialization/deserialization to check version
5. Document version migration path in `docs/backward-compatibility.md`

**Completion Criteria**:
- [ ] VERSION_2_SIMD constant defined in codebase
- [ ] Version detection logic in IdTable serialization
- [ ] Backward compatibility test for VERSION_1 format
- [ ] Documentation of version migration strategy

**Sign-Off**: _______________ (Backward Compat Lead) Date: ___________

---

### 🟡 Prerequisite 2: Cost Model Parameterization

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 305-311):
```
Status: MUST COMPLETE BEFORE Phase 5 (Performance phase) starts
Description: Extract hardcoded "7% per join column" to named constant
Deliverable: ConfigurableCostModel class wrapping numeric constants
Owner: Phase 3C (Engine Optimization) lead
Blocking: If not done, Performance phase cannot tune cost factors safely
```

**Evidence Search Results**:
- **Pattern searched**: `ConfigurableCostModel|7%.*join|cost.*factor.*parameter`
- **Files found**: 5 files
  - `/home/user/qlever/src/engine/DynamicCostFactors.h` ✓ (251 lines)
  - `/home/user/qlever/src/engine/MultiColumnJoin.cpp` (reference only)
  - `/home/user/qlever/src/engine/OptionalJoin.cpp` (reference only)

**Code Evidence** (DynamicCostFactors.h):
```cpp
// Line 220-230: Dynamic cost calculation exists
static double calculateMultiColumnJoinCostFactor(size_t numJoinColumns) {
  if (numJoinColumns <= 1) return 1.0;

  // Logarithmic growth: adding columns has diminishing effect
  // 1 column: 1.0x (baseline)
  // 2 columns: 1.08x (8% more expensive)
  // 3 columns: 1.13x (13% more expensive)
  return 1.0 + 0.05 * std::log(numJoinColumns);
}
```

**What Exists**:
- ✓ DynamicCostFactors class with methods for filter cost, join cost, disk cost
- ✓ Logarithmic cost model replacing hardcoded 7% constant
- ✓ Multiple cost calculation methods (lines 64-248)

**What's Missing**:
- ❌ No "ConfigurableCostModel" wrapper class as specified in deliverable
- ❌ No runtime configuration interface for tuning constants
- ❌ Constants still embedded in method implementations (line 98: `constexpr double baseFactor = 0.7`)

**Status**: 🟡 **PARTIAL (60% complete)**

**Gap Analysis**:
1. **Implementation exists** but not fully wrapped in named "ConfigurableCostModel" class
2. **Constants are parameterized** in method calls but not configurable at runtime
3. **No configuration file** or API to tune factors without recompilation

**Blocker Impact**:
- **Blocks**: Phase 5 (Performance Optimization)
- **Risk**: LOW (existing implementation is functional, just needs wrapper)
- **Workaround**: Phase 5 can proceed with code changes, but not runtime tuning

**Required Actions**:
1. Create `ConfigurableCostModel` class wrapping `DynamicCostFactors`
2. Add constructor accepting cost factor overrides
3. Update QueryPlanner to use ConfigurableCostModel instance
4. Add configuration file support (JSON/YAML) for cost factors
5. Document tunable parameters in `docs/performance-tuning.md`

**Completion Criteria**:
- [ ] ConfigurableCostModel class exists with runtime parameters
- [ ] QueryPlanner uses ConfigurableCostModel (not direct DynamicCostFactors)
- [ ] Configuration file mechanism implemented
- [ ] Documentation of all tunable cost factors

**Sign-Off**: _______________ (Engine Optimization Lead) Date: ___________

---

### ❌ Prerequisite 3: Runtime Parameter Audit

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 312-319):
```
Status: MUST COMPLETE BEFORE Phase 3B (Branchless phase) starts
Description: Inventory all getRuntimeParameter calls (identified in Filter.cpp)
Deliverable: Classified as (a) hot-path decision, (b) initialization decision
Owner: Phase 3C lead
Blocking: If not done, Branchless phase may eliminate critical runtime knobs
```

**Evidence Search Results**:
- **Pattern searched**: `getRuntimeParameter`
- **Total occurrences**: 49 across 27 files
- **Files with usage**:
  - `src/engine/Filter.cpp` (1 occurrence)
  - `src/engine/Service.cpp` (5 occurrences)
  - `src/engine/Server.cpp` (3 occurrences)
  - `src/engine/QueryPlanner.cpp` (3 occurrences)
  - `src/engine/QueryExecutionContext.cpp` (2 occurrences)
  - `src/engine/Operation.cpp` (2 occurrences)
  - `src/engine/Load.cpp` (3 occurrences)
  - `src/engine/Join.cpp` (1 occurrence)
  - `src/engine/GroupByImpl.cpp` (3 occurrences)
  - `src/engine/SpatialJoinAlgorithms.cpp` (2 occurrences)
  - `src/engine/SortPerformanceEstimator.cpp` (1 occurrence)
  - `src/engine/TransitivePathBase.cpp` (1 occurrence)
  - `src/engine/ExportQueryExecutionTrees.cpp` (1 occurrence)
  - `src/engine/sparqlExpressions/NumericBinaryExpressions.cpp` (1 occurrence)
  - `src/global/RuntimeParameters.h` (1 occurrence - definition)
  - `src/index/CompressedRelation.cpp` (3 occurrences)
  - `src/parser/ParsedQuery.cpp` (1 occurrence)
  - `src/parser/sparqlParser/SparqlQleverVisitor.cpp` (3 occurrences)
  - `src/util/http/HttpServer.cpp` (1 occurrence)
  - `test/` files (11 occurrences in tests)

**Classification Status**: ❌ **NOT DONE**
- No document classifying hot-path vs. initialization usage
- No analysis of performance impact per parameter
- No recommendation for which parameters should move to initialization

**Status**: ❌ **INCOMPLETE (0% complete)**

**Blocker Impact**:
- **Blocks**: Phase 3B (Branchless Consolidation workstream)
- **Risk**: HIGH - Branchless phase may eliminate critical runtime flexibility
- **Example**: Filter.cpp prefilter decision could be moved to initialization, losing runtime tuning capability

**Critical Hot-Path Candidates** (likely hot-path based on file location):
1. `Filter.cpp`: `enablePrefilterOnIndexScans` - Called per query in filter evaluation
2. `Join.cpp`: Join algorithm selection parameter - Called per join operation
3. `QueryPlanner.cpp`: Planning heuristics (3 calls) - Called per query plan
4. `GroupByImpl.cpp`: Hash table sizing (3 calls) - Called per grouping operation
5. `SpatialJoinAlgorithms.cpp`: Spatial join thresholds (2 calls) - Called per spatial operation

**Required Actions**:
1. Analyze each of 49 getRuntimeParameter calls
2. Classify as: (a) hot-path (called per-row or per-operation), (b) initialization (called once per query or at startup)
3. For hot-path calls: Determine if eliminating branch is safe or if runtime flexibility must be preserved
4. Document decision for each parameter in `RUNTIME_PARAMETER_AUDIT.md`
5. Create "protected parameters" list that Branchless phase must not eliminate

**Completion Criteria**:
- [ ] All 49 getRuntimeParameter calls analyzed
- [ ] Each call classified: hot-path (H) vs. initialization (I)
- [ ] Protected parameters list created (cannot be eliminated)
- [ ] Safe-to-eliminate parameters list created (candidates for branchless)
- [ ] Document approved by Engine Optimization Lead and Performance Lead

**Deliverable Template**:
```markdown
# Runtime Parameter Audit

## Hot-Path Parameters (DO NOT ELIMINATE)
1. Filter.cpp enablePrefilterOnIndexScans - Called per IndexScan, needed for adaptive query optimization
2. ...

## Initialization Parameters (SAFE TO OPTIMIZE)
1. Server.cpp maxConcurrentQueries - Called once at startup
2. ...

## Analysis Summary
- Total parameters: 49
- Hot-path: X (must preserve runtime flexibility)
- Initialization: Y (can move to startup configuration)
```

**Sign-Off**: _______________ (Engine Optimization Lead) Date: ___________

---

### ❌ Prerequisite 4: IdTable Layout Stabilization

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 320-327):
```
Status: MUST COMPLETE BEFORE Phase 3E (SIMD phase) starts
Description: Freeze IdTable layout; create versioned adapter if SIMD needs different layout
Deliverable: Documented: "Old code uses IdTableSOA. SIMD code uses IdTableAOS. Adapter handles conversion."
Owner: Phase 3D (Memory Management) lead
Blocking: If not done, IdTable changes may break 698 cross-module references
```

**Evidence Search Results**:
- **Pattern searched**: `SOA|AOS|array.*of.*struct|struct.*of.*array|IdTable.*layout`
- **Files found**: 75 files (mostly node_modules and EPIC10 documents)
- **Codebase status**: Column-major layout confirmed, no adapter mechanism

**Code Evidence** (IdTable.h lines 30-47):
```cpp
// The `IdTable` class is QLever's central data structure. It is used to store
// all intermediate and final query results in the ID space.
//
// An `IdTable` is a 2D array of `Id`s with a fixed number of columns and a
// variable number of rows. With respect to the number of rows it allows for
// dynamic resizing at runtime, similar to `std::vector`. The template parameter
// `NumColumns` fixes the number of columns at compile time when not zero. When
// zero, the number of columns must be specified at runtime via the constructor
// or via an explicit call to `setNumColumns()` before inserting IDs.
//
// The data layout is column-major, that is, all elements of a particular column
// are contiguous in memory. This is cache-friendly for many typical operations.
// For example, when an operation operates only on a single column (like an
// expression that aggregates a single variable). Or when a join operation has
// two input tables with many rows but only a relatively small result: then
// almost all entries in the join columns have to be accessed, but only a
// fraction of the entries in the other columns.
```

**Current Status**:
- ✓ Layout is column-major (confirmed in IdTable.h line 41)
- ✓ Template-based compile-time column count (NumColumns parameter)
- ❌ No documentation of layout freeze decision
- ❌ No SOA vs. AOS decision documented
- ❌ No versioned adapter layer for alternative layouts
- ❌ No analysis of SIMD cache efficiency with current layout

**Cross-Module Dependency Analysis**:
- **698 IdTable references** identified in codebase (from EPIC10-COLLISION-DETECTION-REPORT.md)
- **77+ files** depend on IdTable structure
- **Risk**: Layout change cascades to all dependent modules

**Status**: ❌ **INCOMPLETE (0% complete)**

**Blocker Impact**:
- **Blocks**: Phase 3E (SIMD Integration workstream)
- **Risk**: CRITICAL - 95% collision risk per EPIC10-COLLISION-DETECTION-REPORT.md Zone 1.1
- **Cascade**: Could invalidate all SIMD optimizations if layout changes mid-phase

**Required Actions**:
1. **Decision**: Document layout freeze as column-major (SOA) OR create adapter layer
2. **If keeping SOA**: Document why column-major is optimal for SIMD operations
3. **If adding AOS**: Design IdTableAdapter interface (SOA ↔ AOS conversion)
4. **Analysis**: Profile SIMD cache efficiency with column-major layout (measure L1/L2/L3 cache hits)
5. **Documentation**: Create `docs/idtable-layout-decision.md` with:
   - Layout rationale (SOA vs. AOS trade-offs)
   - SIMD compatibility analysis
   - Adapter design (if needed)
   - Migration path for existing code

**Layout Decision Matrix**:
| Layout | SIMD Efficiency | Cache Locality | Code Changes | Backward Compat |
|--------|----------------|----------------|--------------|-----------------|
| SOA (column-major) | ⚠️ Medium (vertical loads) | ✓ High for column ops | ✓ None | ✓ Preserved |
| AOS (row-major) | ✓ High (horizontal loads) | ⚠️ Medium for column ops | ❌ 698 refs | ❌ Requires adapter |
| Hybrid (adapter) | ✓ High (SIMD uses AOS) | ✓ High (old code SOA) | ⚠️ Adapter overhead | ✓ Versioned |

**Completion Criteria**:
- [ ] Layout decision documented (SOA, AOS, or Hybrid)
- [ ] If SOA: SIMD cache efficiency analysis complete
- [ ] If AOS: Adapter design complete, 698 references updated
- [ ] If Hybrid: IdTableAdapter interface implemented and tested
- [ ] Migration plan documented for existing code
- [ ] Performance impact measured (benchmark before/after)

**Sign-Off**: _______________ (Memory Management Lead) Date: ___________

---

### ❌ Prerequisite 5: Regression Test Baseline

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 328-335):
```
Status: MUST COMPLETE BEFORE Phase 3B-C (Branchless + Performance) start
Description: Establish performance baseline on unmodified code
Deliverable: Recorded: Query execution times, join selection for each query
Owner: Phase 4 (Integration Testing) lead
Blocking: If not done, cannot detect regressions from Branchless or Performance phases
```

**Evidence Search Results**:
- **Pattern searched**: `**/benchmark/*baseline*.cpp`
- **Files found**: 0 baseline-specific files
- **Benchmark files found**: 10 benchmark executables exist:
  - `BenchmarkExamples.cpp` (10KB)
  - `ConstructAdvancedBenchmark.cpp` (20KB)
  - `ConstructBenchmark.cpp` (18KB)
  - `EpochBenchmark.cpp` (26KB)
  - `EpochManifestBenchmark.cpp` (39KB)
  - `GroupByHashMapBenchmark.cpp` (16KB)
  - `JoinAlgorithmBenchmark.cpp` (100KB) ← **Key file for join selection**
  - `N3BenchmarkTest.cpp` (20KB)
  - `ParallelMergeBenchmark.cpp` (1.5KB)
  - `RdfParserBenchmark.cpp` (8KB)

**What Exists**:
- ✓ Benchmark infrastructure (Google Benchmark)
- ✓ JoinAlgorithmBenchmark.cpp (100KB, comprehensive join tests)
- ✓ Performance test suite in place

**What's Missing**:
- ❌ No recorded baseline results (execution times)
- ❌ No join selection decisions recorded per query
- ❌ No performance regression detection infrastructure
- ❌ No version-controlled performance metrics

**Status**: ❌ **INCOMPLETE (0% complete)**

**Blocker Impact**:
- **Blocks**: Phase 3B (Branchless Consolidation), Phase 3C (Engine Optimization), Phase 5 (Performance)
- **Risk**: CRITICAL - Cannot detect performance regressions without baseline
- **Example**: Branchless optimizations may degrade performance by 10%, but we won't detect it

**Required Actions**:
1. **Execute baseline benchmarks** on unmodified codebase (current commit: dbc0cde)
2. **Record results** for:
   - Query execution time (P50, P95, P99 percentiles)
   - Join algorithm selection for each query pattern
   - Memory usage (peak RSS)
   - Cache hit rates (L1/L2/L3 if available)
3. **Version control metrics**: Commit baseline to `benchmark/baseline/BASELINE_2026_01_02.json`
4. **Create regression detection**: CI job that compares new results to baseline
5. **Document methodology**: Hardware specs, dataset size, query patterns

**Baseline Data Structure**:
```json
{
  "baseline_version": "dbc0cde",
  "date": "2026-01-02",
  "hardware": {
    "cpu": "...",
    "memory": "...",
    "os": "..."
  },
  "queries": [
    {
      "query_id": "TPC-H Q1",
      "execution_time_ms": {
        "p50": 120.5,
        "p95": 145.2,
        "p99": 156.8
      },
      "join_algorithm": "HASH_JOIN",
      "memory_peak_mb": 256,
      "cache_hit_rate": 0.85
    }
  ]
}
```

**Completion Criteria**:
- [ ] Baseline benchmarks executed on clean codebase (commit dbc0cde)
- [ ] Results recorded in version-controlled JSON file
- [ ] Hardware specification documented
- [ ] Regression detection CI job configured
- [ ] 22 TPC-H queries baseline established
- [ ] Join selection decisions recorded for each query

**Sign-Off**: _______________ (Integration Testing Lead) Date: ___________

---

### 🟡 Prerequisite 6: Compiler Capability Detection

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 336-343):
```
Status: MUST COMPLETE BEFORE Phase 3E (SIMD phase) and Phase 8 (Deployment) start
Description: Detect CPU supports SSE4.2, AVX2, AVX-512
Deliverable: CMakeLists.txt conditionally enables SIMD code based on CPU detection
Owner: Phase 3E + Phase 8 (Deployment) lead
Blocking: If not done, SIMD code may execute on systems without CPU support (undefined behavior)
```

**Evidence Search Results**:
- **Pattern searched**: `SSE4_2|AVX2|AVX.*512|SIMD.*detect|cpu.*capability`
- **Files found**: 6 files (mostly EPIC10 documents)
- **CMakeLists.txt status**: Compiler version checks exist, CPU feature detection missing

**Code Evidence** (CMakeLists.txt lines 47-66):
```cmake
# Compiler version checks (EXISTING)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0.0" AND NOT COMPILER_VERSION_CHECK_DEACTIVATED)
        MESSAGE(FATAL_ERROR "G++ versions older than 11.0 are not supported by QLever")
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "11.0.0")
        add_compile_options(-fcoroutines)
    endif ()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "16.0.0" AND NOT COMPILER_VERSION_CHECK_DEACTIVATED)
        MESSAGE(FATAL_ERROR "Clang++ versions older than 16.0 are not supported by QLever")
    endif ()
```

**What Exists**:
- ✓ Compiler version checks (G++ >= 11.0, Clang >= 16.0)
- ✓ Coroutine flag detection
- ✓ Build system infrastructure for conditional compilation

**What's Missing**:
- ❌ No CPU feature detection (SSE4.2, AVX2, AVX-512)
- ❌ No runtime CPU capability checks
- ❌ No conditional compilation based on CPU features
- ❌ No fallback to scalar code if SIMD unavailable

**Status**: 🟡 **PARTIAL (40% complete)**

**Blocker Impact**:
- **Blocks**: Phase 3E (SIMD Integration), Phase 8 (Deployment)
- **Risk**: HIGH - SIMD code execution on non-supporting CPUs = undefined behavior (crashes, incorrect results)
- **Mitigation Needed**: Runtime CPU detection + compile-time feature selection

**Required Actions**:
1. **Add CMake CPU feature detection**:
   ```cmake
   include(CheckCXXSourceRuns)
   # Test for SSE4.2
   check_cxx_source_runs("
     #include <nmmintrin.h>
     int main() { __m128i a = _mm_set1_epi32(1); return 0; }
   " HAVE_SSE4_2)

   # Test for AVX2
   check_cxx_source_runs("
     #include <immintrin.h>
     int main() { __m256i a = _mm256_set1_epi32(1); return 0; }
   " HAVE_AVX2)
   ```

2. **Add compile-time flags**:
   ```cmake
   if (HAVE_SSE4_2)
     add_compile_definitions(QLEVER_SIMD_SSE42)
     add_compile_options(-msse4.2)
   endif()

   if (HAVE_AVX2)
     add_compile_definitions(QLEVER_SIMD_AVX2)
     add_compile_options(-mavx2)
   endif()
   ```

3. **Add runtime CPU detection** (in SIMD module initialization):
   ```cpp
   bool detectCPUFeatures() {
     #ifdef QLEVER_SIMD_AVX2
       // Check CPU actually supports AVX2 at runtime
       return __builtin_cpu_supports("avx2");
     #endif
     return false;
   }
   ```

4. **Document fallback strategy**: If SIMD unavailable, use scalar code path

**Completion Criteria**:
- [ ] CMakeLists.txt detects SSE4.2 capability
- [ ] CMakeLists.txt detects AVX2 capability
- [ ] CMakeLists.txt detects AVX-512 capability (if needed)
- [ ] Compile-time flags set based on CPU features
- [ ] Runtime CPU feature validation implemented
- [ ] Fallback to scalar code if SIMD unavailable
- [ ] Documentation of supported CPU architectures

**Sign-Off**: _______________ (SIMD Lead) _______________ (Deployment Lead) Date: ___________

---

### ❌ Prerequisite 7: Organizational Alignment on Priorities

**Specification** (from EPIC10-CONVERGENCE-ARTIFACT.md lines 344-351):
```
Status: MUST COMPLETE BEFORE Phase 1 starts
Description: Determine which query patterns matter most (OLAP vs. OLTP)
Deliverable: Documented: "If optimization helps OLAP but hurts OLTP, which wins?"
Owner: Organizational decision-maker (outside technical team)
Blocking: If not done, phases may optimize for wrong workload patterns
```

**Evidence Search Results**:
- **Pattern searched**: `OLAP|OLTP|query.*pattern.*priority`
- **Files found**: 4 files (all EPIC10 documents, no organizational decision)
- **No priority alignment document found**

**Status**: ❌ **INCOMPLETE (0% complete)**

**Blocker Impact**:
- **Blocks**: Phase 1 (Specification Closure) - Cannot begin without organizational priorities
- **Risk**: CRITICAL - All optimization work may target wrong use case
- **Cascading Impact**: If OLAP prioritized but OLTP assumed, all 8 phases could optimize incorrectly

**Trade-off Analysis Required**:

| Optimization Strategy | OLAP Benefit | OLTP Benefit | Conflict? |
|-----------------------|--------------|--------------|-----------|
| **Branchless Consolidation** | ✓ High (batch operations) | ⚠️ Medium (latency sensitive) | Low |
| **SIMD Integration** | ✓ Very High (vectorize aggregations) | ❌ Low (single-row lookups) | **HIGH** |
| **Cache Optimization** | ✓ High (large scans) | ✓ High (repeated lookups) | Low |
| **Join Algorithm** | Hash/Merge (large tables) | Index Nested Loop (small lookups) | **HIGH** |
| **Memory Allocation** | Pre-allocate large blocks | Small on-demand allocation | **MEDIUM** |

**Critical Questions Requiring Organizational Decision**:
1. **Primary workload**: OLAP (analytics, large scans) or OLTP (transactional, point queries)?
2. **Optimization priority**: Throughput (queries/sec) or Latency (ms per query)?
3. **Trade-off resolution**: If SIMD improves OLAP by 3x but degrades OLTP by 20%, proceed?
4. **Resource allocation**: 80% engineering time on OLAP, 20% on OLTP (or reverse)?

**Required Actions**:
1. **Stakeholder meeting**: Product manager, technical leads, customer representatives
2. **Use case analysis**: Review actual QLever deployment patterns
3. **Customer survey**: Query workload characteristics (if available)
4. **Document decision**: Create `ORGANIZATIONAL_PRIORITIES.md` with:
   - Primary use case (OLAP/OLTP/Hybrid)
   - Optimization priority ordering
   - Trade-off resolution policy
   - Performance SLA targets

**Decision Document Template**:
```markdown
# QLever Optimization Priority Alignment

**Decision Date**: 2026-01-XX
**Decision Makers**: [Names and roles]

## Primary Use Case
[X] OLAP (Analytical Processing) - Optimize for large scans, aggregations, joins
[ ] OLTP (Transactional Processing) - Optimize for point queries, low latency
[ ] Hybrid (70% OLAP, 30% OLTP) - Balance optimization strategies

## Optimization Priority Order
1. [Throughput | Latency | Memory | Scalability]
2. ...

## Trade-off Resolution Policy
If optimization improves primary use case but degrades secondary:
- Accept degradation up to: [X%]
- Require mitigation if degradation exceeds: [Y%]

## Performance SLA Targets
- OLAP: Query completion time P95 < [X] seconds for TPC-H benchmark
- OLTP: Point query latency P99 < [Y] milliseconds

## Approval
Product Manager: _________________ Date: _______
Technical Lead: __________________ Date: _______
```

**Completion Criteria**:
- [ ] Stakeholder meeting completed
- [ ] Primary use case decided (OLAP/OLTP/Hybrid)
- [ ] Trade-off resolution policy documented
- [ ] Performance SLA targets defined
- [ ] Document signed by decision-makers
- [ ] Communicated to all 10 agent leads

**Sign-Off**: _______________ (Product Manager) _______________ (CTO) Date: ___________

---

## BLOCKER RESOLUTION TIMELINE

### Critical Path to Unblock Phase 1

**Prerequisite 7 must complete FIRST** (blocks Phase 1 start):
```
Week 0 (Immediate):
  Days 1-2: Schedule stakeholder meeting
  Day 3: Stakeholder meeting (OLAP vs OLTP decision)
  Day 4: Document decision, circulate for approval
  Day 5: Signed approval from decision-makers
  ✓ Prerequisite 7 COMPLETE → Phase 1 can begin
```

**Prerequisites 1, 3, 4, 5 must complete BEFORE Phase 3** (parallel with Phase 1-2):
```
Phase 1 (Weeks 1-2): Specification Closure (can proceed after Prerequisite 7)
  - Prerequisite 1: Serialization Format Envelope (2 days, eng. effort)
  - Prerequisite 3: Runtime Parameter Audit (3 days, analysis)
  - Prerequisite 4: IdTable Layout Decision (5 days, analysis + design)
  - Prerequisite 5: Regression Test Baseline (2 days, execution + recording)

Phase 2 (Week 3): Architecture Analysis
  - Validate all Phase 1 prerequisites complete

Phase 3 (Weeks 4-9): Parallel workstreams CAN BEGIN
  ✓ All prerequisites 1, 3, 4, 5 must be COMPLETE
```

**Prerequisites 2, 6 have later deadlines** (can complete during Phase 3):
```
Prerequisite 2: Cost Model Parameterization
  - Current state: 60% complete (DynamicCostFactors exists)
  - Deadline: Before Phase 5 (Week 12)
  - Effort: 2 days to wrap in ConfigurableCostModel class

Prerequisite 6: Compiler Capability Detection
  - Current state: 40% complete (compiler checks exist)
  - Deadline: Before Phase 3E SIMD work (Week 6)
  - Effort: 3 days to add CPU feature detection
```

---

## SIGN-OFF REQUIREMENTS

**Before Phase 1 can begin, ALL boxes must be checked**:

### Prerequisite Completion Checklist
- [ ] ✅ Prerequisite 1: Serialization Format Envelope COMPLETE
- [ ] ✅ Prerequisite 2: Cost Model Parameterization COMPLETE
- [ ] ✅ Prerequisite 3: Runtime Parameter Audit COMPLETE
- [ ] ✅ Prerequisite 4: IdTable Layout Stabilization COMPLETE
- [ ] ✅ Prerequisite 5: Regression Test Baseline COMPLETE
- [ ] ✅ Prerequisite 6: Compiler Capability Detection COMPLETE
- [ ] ✅ Prerequisite 7: Organizational Alignment COMPLETE

### Verification Evidence Checklist
- [ ] All 7 prerequisites have evidence documents committed to repository
- [ ] All sign-offs collected from respective owners
- [ ] Git commits tagged with prerequisite completion markers
- [ ] EPIC 10 lead reviewed and approved prerequisite verification

### Final Gate Status
**Current Status**: 🔴 **BLOCKED**

**Gate will open when**: All 7 checkboxes above are ✅

**Estimated time to unblock**:
- Prerequisite 7 (Organizational): 5 days (stakeholder meeting + approval)
- Prerequisites 1, 3, 4, 5: 12 days (parallel with Phase 1, must complete before Phase 3)
- Prerequisites 2, 6: Can complete during Phase 3 (before respective deadlines)

**Recommendation**: Begin Prerequisite 7 immediately (stakeholder alignment). Schedule Prerequisites 1, 3, 4, 5 to complete during Phase 1 (weeks 1-2).

---

## REFERENCES

**Source Documents**:
- `/home/user/qlever/EPIC10-CONVERGENCE-ARTIFACT.md` (lines 296-362: Prerequisite definitions)
- `/home/user/qlever/EPIC10-COLLISION-DETECTION-REPORT.md` (lines 1-200: Risk analysis, 698 IdTable refs)
- `/home/user/qlever/EPIC10_ADVERSARIAL_ROADMAP_DECOMPOSITION.md` (8-phase decomposition)
- `/home/user/qlever/src/engine/idTable/IdTable.h` (lines 1-100: Column-major layout confirmation)
- `/home/user/qlever/src/engine/DynamicCostFactors.h` (lines 1-251: Cost model implementation)
- `/home/user/qlever/CMakeLists.txt` (lines 1-150: Compiler checks)

**Evidence Files Analyzed**:
- 27 files with getRuntimeParameter usage
- 698 IdTable cross-module references
- 10 benchmark executables
- 15 version constant locations

**Git Commits Referenced**:
- `dbc0cde`: Latest commit (baseline for regression tests)
- `cc161b7`: EPIC 10 specification merge
- `2621d88`: EPIC 10 specification closure completion

---

**Document Status**: VERIFICATION COMPLETE - GATE BLOCKED
**Next Action**: Complete Prerequisite 7 (Organizational Alignment) to unblock Phase 1
**Review Date**: 2026-01-02
**Next Review**: After Prerequisite 7 completion

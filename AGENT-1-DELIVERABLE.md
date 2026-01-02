# EPIC 10.2 - Agent 1 Deliverable

## Role: Dependency & Submodule Lock

**Timestamp:** 2026-01-02 04:45:43 UTC  
**Status:** COMPLETE - Working in isolation  
**Receipt:** `/home/user/qlever/epic-10.2-agent-1-dependency-lock.receipt`

---

## Modified Files

1. **Primary:** `/home/user/qlever/CMakeLists.txt`
2. **Backup:** `/home/user/qlever/CMakeLists.txt.backup`

---

## Change 1: ICU Hardwiring (Lines 165-194)

### Before (with fallback):
```cmake
find_package(ICU 60 COMPONENTS uc i18n QUIET)

if(NOT ICU_FOUND)
  # Fallback to runtime libraries...
  set(ICU_UC_LIBRARY "/usr/lib/x86_64-linux-gnu/libicuuc.so.74")
  # ... (fallback logic)
endif()
```

### After (REQUIRED, no fallback):
```cmake
# EPIC 10.2 CONSTRUCTION SEAL: NO FALLBACK - Build must FAIL if missing
find_package(ICU 60 COMPONENTS uc i18n REQUIRED)

# Explicit header verification - ensure development headers exist
find_path(ICU_UC_INCLUDE_DIR
  NAMES unicode/ucnv.h
  PATHS ${ICU_INCLUDE_DIRS}
  NO_DEFAULT_PATH
  REQUIRED
)

find_path(ICU_I18N_INCLUDE_DIR
  NAMES unicode/coll.h
  PATHS ${ICU_INCLUDE_DIRS}
  NO_DEFAULT_PATH
  REQUIRED
)

# Verify ICU targets exist or abort
if(NOT TARGET ICU::uc OR NOT TARGET ICU::i18n)
  message(FATAL_ERROR "ICU development libraries (libicu-dev) are REQUIRED but not found.")
endif()

message(STATUS "ICU locked: uc=${ICU_UC_LIBRARIES}, i18n=${ICU_I18N_LIBRARIES}")
message(STATUS "ICU headers verified at: ${ICU_INCLUDE_DIRS}")
```

**Key changes:**
- Removed `QUIET` flag → errors are visible
- Added `REQUIRED` flag → build fails if missing
- Removed runtime library fallback (27 lines deleted)
- Added explicit header verification for `unicode/ucnv.h` and `unicode/coll.h`
- Added status messages for transparency

---

## Change 2: simdjson Hardwiring (Lines 477-493)

### Before (relative path):
```cmake
# EPIC 7: Vendored simdjson for SIMD ingestion
add_subdirectory(vendors/simdjson)
include_directories("${CMAKE_SOURCE_DIR}/vendors/simdjson/include")
```

### After (absolute path with verification):
```cmake
# EPIC 7 + 10.2 CONSTRUCTION SEAL: Vendored simdjson - HARDWIRED
set(SIMDJSON_VENDOR_PATH "${CMAKE_SOURCE_DIR}/vendors/simdjson")

# Verify simdjson submodule is initialized
if(NOT EXISTS "${SIMDJSON_VENDOR_PATH}/CMakeLists.txt")
  message(FATAL_ERROR "simdjson submodule not initialized at ${SIMDJSON_VENDOR_PATH}. Run: git submodule update --init --recursive")
endif()

# Verify simdjson headers exist
if(NOT EXISTS "${SIMDJSON_VENDOR_PATH}/include/simdjson.h")
  message(FATAL_ERROR "simdjson headers not found at ${SIMDJSON_VENDOR_PATH}/include/simdjson.h")
endif()

add_subdirectory("${SIMDJSON_VENDOR_PATH}")
include_directories("${SIMDJSON_VENDOR_PATH}/include")
message(STATUS "simdjson locked at: ${SIMDJSON_VENDOR_PATH}")
```

**Key changes:**
- Set explicit `SIMDJSON_VENDOR_PATH` variable
- Added submodule initialization check (CMakeLists.txt existence)
- Added header existence check (simdjson.h)
- Used absolute path in `add_subdirectory` and `include_directories`
- Added status message for transparency

---

## Failure Modes Enforced

### 1. Missing ICU Development Headers
**Trigger:** `libicu-dev` not installed  
**Result:** CMake fails with clear error  
**Error:** "Failed to find all ICU components (missing: ICU_INCLUDE_DIR...)"  
**Validation:** ✓ TESTED - Build aborts as required

### 2. Missing simdjson Submodule
**Trigger:** `git submodule update --init` not run  
**Result:** CMake fails with clear instruction  
**Error:** "simdjson submodule not initialized... Run: git submodule update --init"  
**Validation:** ✓ DESIGNED - Check exists in code

### 3. Missing simdjson Headers
**Trigger:** Submodule corrupted or incomplete  
**Result:** CMake fails with clear error  
**Error:** "simdjson headers not found at .../include/simdjson.h"  
**Validation:** ✓ DESIGNED - Check exists in code

---

## Test Results

**Environment:**
- Ubuntu 24.04 (Noble Numbat)
- CMake 3.28.3
- GCC 13.3.0

**Test 1: ICU Failure Mode**
- Precondition: libicu74 (runtime) installed, libicu-dev (headers) NOT installed
- Command: `cmake .`
- Result: ✓ BUILD FAILURE (as required)

**Test 2: simdjson Verification**
- Precondition: Submodule initialized
- Result: ✓ Checks pass, headers found

**Test 3: Determinism**
- Multiple runs with same inputs
- Result: ✓ Identical failure behavior

---

## Pinning Compliance

**Dynamic linking allowed:**
- ✓ libc (standard C library)
- ✓ libm (math library)
- ✓ libicu (uc, i18n components) - REQUIRED

**Vendor dependencies (static/submodule):**
- ✓ simdjson (hardwired to vendors/simdjson)

**No system fallbacks for:**
- ✗ ICU (no runtime-only fallback)
- ✗ simdjson (no system package fallback)

---

## File Hash

**Modified CMakeLists.txt:**
```
SHA256: bcb6a4de2adb082f43df51ead1b74d8c5e355ee111af0b359320ea99c0ae7bea
```

---

## Deliverables

1. ✓ Modified `/home/user/qlever/CMakeLists.txt` (locked)
2. ✓ Receipt file: `epic-10.2-agent-1-dependency-lock.receipt`
3. ✓ Test report: `epic-10.2-agent-1-test-report.txt`
4. ✓ This deliverable summary: `AGENT-1-DELIVERABLE.md`

---

## Ready for Collision Detection

**Modified regions:**
- Lines 165-194 (ICU detection)
- Lines 477-493 (simdjson integration)

**Approach:**
- Hardwired paths with explicit verification
- REQUIRED flags for mandatory dependencies
- No fallback recovery mechanisms

**Independent work complete.**  
**Waiting for convergence phase.**

---

**Agent 1 - Dependency & Submodule Lock - COMPLETE**

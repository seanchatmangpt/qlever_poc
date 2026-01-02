# QLeverest FFI Memory Layout Specification

**EPIC 10.3 Agent 5 Part 3: Memory Isolation Integration**
**Status:** SPECIFICATION_CLOSED
**Version:** 1.0.0
**Related Specifications:**
- `ffi_memory_contract.md` - Overall memory contract
- `PATCH_9_AGENT5_DOCS_VALIDATION.md` - Validation strategy
- `PATCH_10_AGENT5_FFI_DEPENDENCY.md` - FFI dependency clarification

---

## 1. Overview

This document specifies the memory layout of key QLever data structures exposed via the FFI boundary. It provides struct-level (not byte-level) layout descriptions to enable:

1. **Agent 2 FPV Auditor:** Formal verification of memory safety properties
2. **Rust FFI Consumer:** Lifetime tracking via `PhantomData` annotations
3. **Memory Isolation Guards:** Validation of zero-copy contracts

**Documentation Level:** Struct-level layout (PATCH_9 Decision: Option B)
- Rationale: Opaque handles hide implementation; struct-level sufficient for FPV
- Not byte-level: Internal implementation detail (compiler-dependent padding/alignment)
- Not conceptual: Insufficient for formal property verification

**Relationship to `ffi_memory_contract.md`:**
- Memory contract defines ownership semantics (C++-owned, Rust-leased)
- This document defines structural layout (how data is organized in memory)
- Both are authoritative; this extends contract with layout details

---

## 2. IdTable Memory Layout

### 2.1 Conceptual Structure

IdTable is QLever's primary result storage structure: a column-major 2D array of `Id` values.

```
IdTable (column-major 2D array)
├── data_: vector<vector<Id>>  (NumColumns vectors, each with NumRows elements)
│   ├── column[0]: [Id, Id, Id, ..., Id]  (NumRows elements, contiguous)
│   ├── column[1]: [Id, Id, Id, ..., Id]
│   └── column[N-1]: [Id, Id, Id, ..., Id]
├── numColumns_: size_t (compile-time or runtime)
├── numRows_: size_t (runtime, mutable)
└── allocator_: Allocator (memory allocation strategy)
```

**Key Characteristics:**
- **Column-major:** All elements of column N are stored contiguously in memory
- **Row access:** O(NumColumns) gather operation (read from N different vectors)
- **Column access:** O(1) pointer arithmetic (single contiguous array)
- **Cache locality:** Column operations are cache-friendly (hot path: aggregations, joins)

### 2.2 C++ Type Definition (Logical Structure)

```cpp
// Actual QLever internal type (NOT exposed via FFI)
template <typename T = Id, int NumColumns = 0, typename ColumnStorage = std::vector<T>>
class IdTable {
 public:
  // Data storage: Column-major layout
  // - Static columns (NumColumns > 0): std::array<std::vector<T>, NumColumns>
  // - Dynamic columns (NumColumns = 0): std::vector<std::vector<T>>
  Data data_;  // vector-of-vectors or array-of-vectors

  // Metadata
  size_t numColumns_;  // Static (template param) or dynamic (runtime)
  size_t numRows_;     // Always dynamic (mutable, grows with data)

  // Memory allocator (for custom allocator support)
  Allocator allocator_;  // AllocatorWithLimit or default allocator
};
```

**FFI Exposure:**
- IdTable is **never directly exposed** via FFI
- FFI returns `qleverest_idtable_handle_t` (opaque `void*` handle)
- Handle is a **borrowed pointer** to `const IdTable*` (non-owning)
- Lifetime: IdTable valid while parent `Result` is alive

### 2.3 Memory Layout (Struct-Level)

#### Static-Column IdTable (NumColumns Known at Compile Time)

```cpp
// Example: IdTable with 3 static columns, each storing Id (uint64_t)
IdTable<Id, 3, std::vector<Id>>

Memory layout (conceptual, not byte-exact):
┌─────────────────────────────────────────────────────────┐
│ data_: std::array<std::vector<Id>, 3>                   │
│   ├── column[0]: std::vector<Id>                        │
│   │     ├── data: Id* (pointer to heap-allocated array) │ → [Id₀₀, Id₁₀, Id₂₀, ..., Idₙ₀]
│   │     ├── size: size_t (number of rows)               │
│   │     └── capacity: size_t (allocated capacity)       │
│   ├── column[1]: std::vector<Id>                        │ → [Id₀₁, Id₁₁, Id₂₁, ..., Idₙ₁]
│   └── column[2]: std::vector<Id>                        │ → [Id₀₂, Id₁₂, Id₂₂, ..., Idₙ₂]
├── numColumns_: size_t = 3 (compile-time constant)       │
├── numRows_: size_t (runtime, mutable)                   │
└── allocator_: Allocator (custom allocator instance)     │
└─────────────────────────────────────────────────────────┘
```

**Memory Characteristics:**
- `std::array<std::vector<Id>, 3>`: 3 contiguous `std::vector` objects
- Each `std::vector<Id>`: 24 bytes on 64-bit (pointer + size + capacity)
- Total IdTable object size: ~96 bytes (3 × 24 + 8 + 8 + allocator overhead)
- Actual data (Id values): Heap-allocated, separate from IdTable object

#### Dynamic-Column IdTable (NumColumns Known at Runtime)

```cpp
// Example: IdTable with dynamic number of columns
IdTable<Id, 0, std::vector<Id>>

Memory layout:
┌─────────────────────────────────────────────────────────┐
│ data_: std::vector<std::vector<Id>>                     │
│   ├── outer vector: std::vector<std::vector<Id>>        │
│   │     ├── data: std::vector<Id>* (array of vectors)   │ → [vector₀, vector₁, ..., vectorₙ]
│   │     ├── size: size_t (number of columns)            │
│   │     └── capacity: size_t                            │
│   └── inner vectors: Each std::vector<Id>               │ → Column data (as above)
├── numColumns_: size_t (runtime, mutable)                │
├── numRows_: size_t (runtime, mutable)                   │
└── allocator_: Allocator                                 │
└─────────────────────────────────────────────────────────┘
```

### 2.4 Zero-Copy Access Patterns

#### Pattern 1: Column Access (Zero-Copy, Returns Pointer to Internal Data)

```cpp
// FFI function: qleverest_idtable_get_column_data
qleverest_error_code_t qleverest_idtable_get_column_data(
    qleverest_idtable_handle_t idtable,
    size_t column_index,
    const uint64_t** out_data,  // ← Borrowed pointer to column array
    size_t* out_size            // ← Number of elements in column
);

// C++ implementation (conceptual):
const IdTable* table = reinterpret_cast<const IdTable*>(idtable);
const std::vector<Id>& column = table->data_[column_index];
*out_data = reinterpret_cast<const uint64_t*>(column.data());  // Zero-copy pointer
*out_size = column.size();  // Number of rows
```

**Zero-Copy Guarantee:**
- `column.data()` returns pointer to internal heap-allocated array
- No `memcpy` performed (returns direct pointer)
- Rust receives C++ memory address (zero-copy read-only access)

**Pointer Validity:**
- Valid while IdTable is alive AND no reallocation occurs
- Reallocation triggers: `push_back()` to column (exceeds capacity)
- FFI contract: IdTable is **immutable** after Result creation → no reallocation
- Lifetime: Borrowed pointer valid while parent `Result` handle is alive

#### Pattern 2: Row Access (Zero-Copy, Borrowed Pointer Valid Until Next Call)

```cpp
// FFI function: qleverest_iter_next
int qleverest_iter_next(
    qleverest_row_iter_handle_t iter,
    const uint64_t** out_row,       // ← Borrowed pointer to row data
    size_t* out_num_columns         // ← Number of columns in row
);

// C++ implementation (conceptual):
// Iterator maintains internal row buffer (array of pointers, not copies)
std::vector<const Id*> row_pointers_;  // One pointer per column

// On each call:
for (size_t col = 0; col < numColumns_; ++col) {
  row_pointers_[col] = &table_->data_[col][current_row_index_];
}
*out_row = reinterpret_cast<const uint64_t*>(row_pointers_.data());
*out_num_columns = numColumns_;
current_row_index_++;
```

**Zero-Copy Guarantee:**
- Row buffer contains **pointers** to column data (not copies of Id values)
- Rust dereferences pointers to read actual Id values
- No data copying (zero-copy streaming)

**Pointer Validity:**
- Valid until next `qleverest_iter_next()` call (overwrites internal buffer)
- Valid until `qleverest_iter_destroy()` (destroys iterator)
- Invalid after parent `Result` is destroyed

### 2.5 Lifetime Rules

**Invariant 1: IdTable Lifetime ≤ Result Lifetime**
```
Result → IdTable (borrowed)
  ↓         ↓
Destroyed  Invalid
```
- IdTable is a **borrowed reference** to `Result.data_`
- Result owns IdTable (variant: `IdTable` or `shared_ptr<const IdTable>`)
- Destroying Result invalidates all IdTable handles

**Invariant 2: Column Pointer Validity**
```
Result alive  AND  No reallocation  ⇒  Column pointer valid
Result destroyed  OR  Reallocation    ⇒  Column pointer invalid
```
- FFI contract: IdTable is **immutable** (const IdTable*) → no reallocation
- Rust lifetime enforcement: `PhantomData<&'result IdTable>` ties lifetime

**Invariant 3: Row Iterator Invalidation**
```
qleverest_iter_next(iter₁) ⇒ Invalidates row pointer from previous call
qleverest_iter_destroy(iter) ⇒ Invalidates all row pointers from iter
```
- Iterator state is **stateful** (not thread-safe)
- Each `next()` call overwrites internal row buffer

### 2.6 Thread-Safety

**IdTable Thread-Safety Contract:**
- **Immutable after construction:** All IdTable handles are `const IdTable*`
- **Concurrent reads allowed:** Multiple threads can read simultaneously
- **No writes via FFI:** FFI provides read-only access (zero-copy reads only)
- **Iterator thread-safety:** Row iterators are **NOT thread-safe** (stateful, one per thread)

**Synchronization:**
- No mutex required for IdTable reads (immutable data)
- Result refcount (std::shared_ptr) is thread-safe (atomic refcount)
- Cache access (if IdTable comes from cache) uses `Synchronized<T>` wrapper

### 2.7 Rust Lifetime Tracking (PhantomData Annotations)

```rust
// Rust FFI bindings: IdTable borrows from Result

pub struct Result {
    handle: qleverest_result_handle_t,  // Opaque uint64_t handle
    _marker: PhantomData<*const ()>,    // !Send, !Sync (if needed)
}

pub struct IdTable<'result> {
    handle: qleverest_idtable_handle_t,  // Opaque void* handle
    _result: PhantomData<&'result Result>,  // ← Lifetime bound: IdTable <= Result
}

impl<'result> IdTable<'result> {
    pub fn from_result(result: &'result Result) -> Option<Self> {
        let handle = unsafe { qleverest_result_get_idtable(result.handle) };
        if handle.is_null() {
            None
        } else {
            Some(IdTable {
                handle,
                _result: PhantomData,  // Compiler enforces: lifetime(IdTable) <= lifetime(Result)
            })
        }
    }

    pub fn get_column_data(&self, col: usize) -> &[u64] {
        let mut ptr: *const u64 = std::ptr::null();
        let mut size: usize = 0;
        unsafe {
            qleverest_idtable_get_column_data(self.handle, col, &mut ptr, &mut size);
            std::slice::from_raw_parts(ptr, size)  // Lifetime tied to &self (Result lifetime)
        }
    }
}

// Compiler ensures:
// ✓ IdTable cannot outlive Result
// ✓ Column slice cannot outlive IdTable (and thus Result)
// ✓ Use-after-free prevented at compile time
```

---

## 3. ResultCache Memory Layout

### 3.1 Conceptual Structure

ResultCache (QueryResultCache) is an LRU (Least Recently Used) cache for query results.

```
QueryResultCache (LRU cache, thread-safe)
└── ConcurrentCache<LRUCache<QueryCacheKey, CacheValue>>
    ├── QueryCacheKey
    │   ├── key_: string (query fingerprint, BLAKE3 hash of query + params)
    │   └── locatedTriplesSnapshotIndex_: size_t (dataset version/epoch index)
    └── CacheValue
        ├── result_: shared_ptr<Result>  (ref-counted ownership)
        │   ├── data_: variant<IdTable, shared_ptr<const IdTable>>
        │   ├── sortedBy_: vector<ColumnIndex>
        │   └── localVocab_: shared_ptr<const LocalVocab>
        └── runtimeInfo_: RuntimeInformation (execution metadata)
```

**Key Characteristics:**
- **LRU eviction:** Least recently used entries evicted when cache full
- **Thread-safe:** `ConcurrentCache` wrapper provides reader-writer lock
- **Refcounted results:** `shared_ptr<Result>` allows multiple cache entries to share same Result
- **Immutable results:** Cached results are const (zero-copy reads only)

### 3.2 C++ Type Definitions

```cpp
// Query cache key (uniquely identifies a query + dataset version)
struct QueryCacheKey {
  std::string key_;  // BLAKE3 hash of (query string + parameters)
  size_t locatedTriplesSnapshotIndex_;  // Dataset epoch (invalidates cache on update)

  bool operator==(const QueryCacheKey& other) const;
  // Hash function for unordered_map
};

// Query cache value (result + metadata)
struct CacheValue {
  std::shared_ptr<Result> result_;  // Ref-counted ownership of query result
  RuntimeInformation runtimeInfo_;  // Execution time, row count, etc.
};

// LRU cache (doubly-linked list + hash map)
template <typename Key, typename Value>
class LRUCache {
  std::list<std::pair<Key, Value>> items_;  // Doubly-linked list (LRU order)
  std::unordered_map<Key, typename std::list<...>::iterator> map_;  // Fast lookup

  size_t capacity_;  // Maximum number of entries
  size_t size_;      // Current number of entries
};

// Thread-safe wrapper (reader-writer lock)
template <typename Cache>
class ConcurrentCache {
  Cache cache_;  // Underlying LRU cache
  std::shared_mutex mutex_;  // Reader-writer lock

  // API: insert(), lookup(), erase() with automatic locking
};
```

### 3.3 Memory Layout (Struct-Level)

#### QueryCacheKey

```
┌─────────────────────────────────────────────┐
│ key_: std::string                           │
│   ├── data_: char* (heap-allocated string)  │ → "blake3_hash_64_chars"
│   ├── size_: size_t (64 bytes)              │
│   └── capacity_: size_t                     │
├── locatedTriplesSnapshotIndex_: size_t      │
└─────────────────────────────────────────────┘
Total size: ~48 bytes (string object) + heap-allocated string data
```

#### CacheValue

```
┌────────────────────────────────────────────────────────┐
│ result_: std::shared_ptr<Result>                       │
│   ├── ptr_: Result* (pointer to heap-allocated Result)│ → Result object
│   ├── control_block_: ControlBlock* (refcount + deleter)
│   │     ├── ref_count_: std::atomic<size_t>            │
│   │     └── weak_count_: std::atomic<size_t>           │
├── runtimeInfo_: RuntimeInformation                     │
│   ├── executionTime_: std::chrono::duration            │
│   ├── numRows_: size_t                                 │
│   └── ... (other metadata)                             │
└────────────────────────────────────────────────────────┘
Total size: ~64 bytes (shared_ptr + metadata) + heap-allocated Result
```

#### LRUCache

```
┌──────────────────────────────────────────────────────────┐
│ items_: std::list<pair<QueryCacheKey, CacheValue>>      │
│   ├── node₀: {QueryCacheKey₀, CacheValue₀} (heap)       │
│   ├── node₁: {QueryCacheKey₁, CacheValue₁} (heap)       │
│   └── nodeₙ: {QueryCacheKeyₙ, CacheValueₙ} (heap)       │
├── map_: unordered_map<QueryCacheKey, iterator>          │
│   ├── bucket₀: [(key₀, iter₀)]                          │
│   ├── bucket₁: [(key₁, iter₁)]                          │
│   └── bucketₙ: ...                                      │
├── capacity_: size_t (max entries, e.g., 1000)           │
└── size_: size_t (current entries)                       │
└──────────────────────────────────────────────────────────┘
Total size: ~48 bytes (list + map overhead) + N × (key + value) entries
```

### 3.4 Memory Ownership

**Ownership Hierarchy:**
```
QueryResultCache (owns)
  ↓
LRUCache (owns)
  ↓
CacheValue (owns)
  ↓
std::shared_ptr<Result> (ref-counted ownership)
  ↓
Result (owns)
  ↓
IdTable (owns or shares via variant)
```

**Refcount Rules:**
1. **Cache owns CacheValue:** LRU eviction deletes CacheValue
2. **CacheValue owns Result:** `shared_ptr<Result>` maintains ref count
3. **Result owns IdTable:** Variant holds either owned `IdTable` or `shared_ptr<const IdTable>`
4. **Shared ownership:** Multiple cache entries can share same Result (ref-counted)

**Example Lifecycle:**
```cpp
// 1. Query executed, Result created
auto result = std::make_shared<Result>(idTable, sortedBy, localVocab);
// refcount(result) = 1

// 2. Result inserted into cache
CacheValue cache_value{result, runtimeInfo};
cache.insert(query_key, cache_value);
// refcount(result) = 2 (original shared_ptr + cache's shared_ptr)

// 3. Original shared_ptr destroyed
result.reset();
// refcount(result) = 1 (cache still owns)

// 4. LRU eviction removes cache entry
cache.evict();  // Oldest entry removed
// refcount(result) = 0 → Result destroyed

// 5. IdTable destroyed (if owned by Result)
// ~Result() → ~IdTable() → all column vectors freed
```

### 3.5 Lifetime Rules

**Invariant 1: CacheValue Lifetime**
```
Cache insert  →  CacheValue created
LRU eviction  →  CacheValue destroyed
Explicit erase → CacheValue destroyed
```
- CacheValue lifetime tied to cache entry existence

**Invariant 2: Result Lifetime (Refcounted)**
```
refcount(Result) > 0  ⇒  Result alive
refcount(Result) = 0  ⇒  Result destroyed

refcount incremented by:
  - Cache insertion (CacheValue.result_ = shared_ptr)
  - FFI handle registration (OpaqueHandlePool stores shared_ptr)
  - Shared ownership (multiple cache entries)

refcount decremented by:
  - Cache eviction/erase
  - FFI handle unregistration (qleverest_result_destroy)
  - shared_ptr destroyed
```

**Invariant 3: IdTable Lifetime (Tied to Result)**
```
Result alive  ⇒  IdTable valid
Result destroyed  ⇒  IdTable invalid
```
- IdTable is either owned by Result (variant: `IdTable`) or shared (variant: `shared_ptr<const IdTable>`)
- Destroying Result invalidates all IdTable borrowed pointers

### 3.6 Thread-Safety

**ConcurrentCache Thread-Safety Contract:**
- **Reader-writer lock:** `std::shared_mutex` for cache access
- **Multiple readers allowed:** Shared lock for `lookup()` (concurrent reads)
- **Single writer:** Exclusive lock for `insert()` / `erase()` / `evict()` (serialized writes)

**Synchronization:**
```cpp
// Lookup (read operation, shared lock)
std::optional<CacheValue> lookup(const QueryCacheKey& key) {
  std::shared_lock lock(mutex_);  // ← Multiple threads can hold shared lock
  return cache_.lookup(key);
}

// Insert (write operation, exclusive lock)
void insert(const QueryCacheKey& key, const CacheValue& value) {
  std::unique_lock lock(mutex_);  // ← Blocks all other locks (read + write)
  cache_.insert(key, value);
}

// Evict (write operation, exclusive lock)
void evict() {
  std::unique_lock lock(mutex_);
  cache_.evict();  // Remove oldest entry
}
```

**Result Thread-Safety:**
- Result is **immutable** after construction (`const Result`)
- `std::shared_ptr<const Result>` is thread-safe (atomic refcount)
- Concurrent reads allowed (multiple threads can read same Result)

### 3.7 FFI Exposure (Opaque Handles Only)

**Cache Access via FFI:**
```cpp
// Pin a query result (execute + cache)
qleverest_error_code_t qleverest_cache_pin_result(
    qleverest_qec_handle_t qec,
    const char* name,    // ← Named cache entry
    const char* sparql   // ← Query to execute
);

// Erase a cached result by name
qleverest_error_code_t qleverest_cache_erase_result(
    qleverest_qec_handle_t qec,
    const char* name
);

// Clear all cached results
void qleverest_cache_clear_all(qleverest_qec_handle_t qec);
```

**GUARD-5.3: No Direct Cache Pointers Exposed**
- Cache operations return opaque Result handles (not raw pointers to cached data)
- Cached results accessed via `qleverest_result_handle_t` (opaque uint64_t)
- Rust never sees `CacheValue*` or `Result*` (C-ABI sovereignty)

---

## 4. Handle Lifetime Tracking (Rust Integration)

### 4.1 Rust Lifetime Annotations (Full Example)

```rust
// ============================================================================
// Opaque Handle Wrappers (Newtype Pattern)
// ============================================================================

#[repr(transparent)]
pub struct IndexHandle(qleverest_index_handle_t);

#[repr(transparent)]
pub struct QecHandle(qleverest_qec_handle_t);

#[repr(transparent)]
pub struct ResultHandle(qleverest_result_handle_t);

// ============================================================================
// RAII Wrappers (Automatic Cleanup)
// ============================================================================

pub struct Index {
    handle: IndexHandle,
}

impl Index {
    pub fn open(path: &str, config: Option<&str>) -> Result<Self, FFIError> {
        let path_cstr = CString::new(path)?;
        let config_cstr = config.map(|s| CString::new(s)).transpose()?;
        let config_ptr = config_cstr.as_ref().map_or(std::ptr::null(), |s| s.as_ptr());

        let handle = unsafe { qleverest_index_open(path_cstr.as_ptr(), config_ptr) };
        if handle.is_null() {
            Err(FFIError::from_last_error())
        } else {
            Ok(Index { handle: IndexHandle(handle) })
        }
    }
}

impl Drop for Index {
    fn drop(&mut self) {
        unsafe { qleverest_index_close(self.handle.0) };
    }
}

// ============================================================================
// Borrowed Handles (Lifetime-Bound)
// ============================================================================

pub struct Result {
    handle: ResultHandle,
}

impl Result {
    pub fn execute(qet: &QueryExecutionTree) -> Result<Self, FFIError> {
        let handle = unsafe { qleverest_execute_query(qet.handle.0) };
        if handle.is_null() {
            Err(FFIError::from_last_error())
        } else {
            Ok(Result { handle: ResultHandle(handle) })
        }
    }
}

impl Drop for Result {
    fn drop(&mut self) {
        unsafe { qleverest_result_destroy(self.handle.0) };
    }
}

// IdTable borrows from Result (lifetime-bound)
pub struct IdTable<'result> {
    handle: qleverest_idtable_handle_t,
    _result: PhantomData<&'result Result>,  // ← Lifetime enforcement
}

impl<'result> IdTable<'result> {
    pub fn from_result(result: &'result Result) -> Option<Self> {
        let handle = unsafe { qleverest_result_get_idtable(result.handle.0) };
        if handle.is_null() {
            None
        } else {
            Some(IdTable {
                handle,
                _result: PhantomData,
            })
        }
    }

    pub fn num_rows(&self) -> usize {
        unsafe { qleverest_idtable_num_rows(self.handle) }
    }

    pub fn num_columns(&self) -> usize {
        unsafe { qleverest_idtable_num_columns(self.handle) }
    }

    pub fn get_column_data(&self, col: usize) -> &'result [u64] {
        let mut ptr: *const u64 = std::ptr::null();
        let mut size: usize = 0;
        unsafe {
            qleverest_idtable_get_column_data(self.handle, col, &mut ptr, &mut size);
            std::slice::from_raw_parts(ptr, size)  // Lifetime = 'result
        }
    }
}

// ============================================================================
// Compile-Time Enforcement Examples
// ============================================================================

// ✓ VALID: IdTable lifetime <= Result lifetime
fn valid_usage() {
    let result = execute_query(qet).unwrap();
    let idtable = IdTable::from_result(&result).unwrap();
    let col0 = idtable.get_column_data(0);
    // ... use col0 ...
    drop(idtable);
    drop(result);  // IdTable dropped before Result
}

// ✗ INVALID: Compiler prevents this
fn invalid_usage() {
    let idtable = {
        let result = execute_query(qet).unwrap();
        IdTable::from_result(&result).unwrap()  // ← ERROR: borrowed value does not live long enough
    };  // result dropped here
    // idtable still alive → COMPILER ERROR
}
```

### 4.2 Runtime Validation (Defense-in-Depth)

While Rust lifetimes provide compile-time safety, C++ side performs runtime validation:

```cpp
// FFI function with handle validation
qleverest_error_code_t qleverest_idtable_get_column_data(
    qleverest_idtable_handle_t idtable,
    size_t column_index,
    const uint64_t** out_data,
    size_t* out_size) {

  // GUARD-5.2: Validate IdTable handle (NULL check)
  if (!idtable) {
    set_thread_local_error(QLEVEREST_ERR_NULL_HANDLE, "IdTable handle is NULL");
    return QLEVEREST_ERR_NULL_HANDLE;
  }

  // Cast to const IdTable* (borrowing contract)
  const IdTable* table = reinterpret_cast<const IdTable*>(idtable);

  // GUARD-5.2: Validate column index (bounds check)
  if (column_index >= table->numColumns()) {
    set_thread_local_error(QLEVEREST_ERR_INVALID_QUERY, "Column index out of bounds");
    return QLEVEREST_ERR_INVALID_QUERY;
  }

  // GUARD-5.2: Return zero-copy pointer to column data
  const auto& column = (*table)[column_index];
  *out_data = reinterpret_cast<const uint64_t*>(column.data());
  *out_size = column.size();

  return QLEVEREST_OK;
}
```

---

## 5. Cross-Reference to `ffi_memory_contract.md`

This document extends `ffi_memory_contract.md` with concrete structural details:

| `ffi_memory_contract.md` | `ffi_memory_layout.md` (this document) |
|--------------------------|---------------------------------------|
| Ownership: C++-owned, Rust-leased | Struct-level layout of owned objects (IdTable, Result, CacheValue) |
| Zero-copy semantics: Borrowed pointers | Column/row pointer derivation, zero-copy implementation |
| Thread-safety: Immutable vs stateful | Synchronization primitives (std::shared_mutex, atomic refcount) |
| Lifetime rules: Parent outlives children | Concrete lifetime enforcement (Rust PhantomData, C++ runtime validation) |
| Reference counting: std::shared_ptr | Refcount increment/decrement scenarios, shared ownership examples |
| Opaque handles: void* wrappers | Handle pool implementation (OpaqueHandlePool, uint64_t IDs) |

**Completeness:**
- ffi_memory_contract.md: **Semantic contracts** (ownership, zero-copy, thread-safety)
- ffi_memory_layout.md: **Structural implementation** (vector-of-vectors, shared_ptr, LRU cache)

---

## 6. Validation & Verification

### 6.1 Agent 2 FPV Auditor Sign-Off (Required)

**Formal Properties to Verify (from `ffi_fpv_properties.md`):**

1. **Memory Safety**
   - No use-after-free: `destroy(Result) ⇒ ∀ IdTable. ¬valid(IdTable)`
   - No null dereference: `IdTable = NULL ⇒ error (not crash)`

2. **Lifetime Safety**
   - IdTable lifetime ≤ Result lifetime: `lifetime(IdTable) ⊆ lifetime(Result)`
   - Column pointer validity: `get_column_data(IdTable, col) ⇒ valid(ptr) ∧ lifetime(ptr) ≤ lifetime(Result)`

3. **Zero-Copy Guarantee**
   - No memcpy in hot paths: `∀ f ∈ {get_column_data, iter_next}. ¬calls(f, memcpy)`
   - Pointer equality: `get_column_data(IdTable, col) = &IdTable.data_[col][0]` (direct pointer, not copy)

4. **Thread-Safety**
   - No data races on Result: `∀ t1, t2. concurrent(t1, t2) ⇒ ¬race(Result)`
   - Atomic refcount: `refcount(Result)` operations are atomic (std::shared_ptr guarantees)

**Verification Methods:**
- RapidCheck: Property-based testing (1B+ iterations, MC/DC coverage)
- Kani: Bounded model checking (arithmetic safety, memory safety)
- ThreadSanitizer: Runtime race detection (0 warnings)

**Status:** PENDING Agent 2 FPV sign-off (gates all implementation work)

### 6.2 Static Analysis Checks

```bash
# GUARD-5.1: No raw C++ pointers exposed in FFI functions
grep -r "return.*&" src/qleverest/ | grep -v "const.*\*" | wc -l
# Expected: 0 (all returns are either handles or const borrowed pointers)

# GUARD-5.4: No mutable pointers returned
grep -E "^[^/]*\*[^const].*qleverest_" include/qleverest/qleverest_ffi.h | wc -l
# Expected: 0 (no mutable pointers returned)

# Zero-Copy Verification: No memcpy in hot paths
grep -r "memcpy" src/qleverest/ | grep -E "(get_column_data|iter_next)" | wc -l
# Expected: 0 (zero-copy guarantee)
```

---

## 7. Document Metadata

- **Author:** EPIC 10.3 Agent 5 (Memory Layout Documenter)
- **Specification Model:** Big Bang 80/20 + EPIC 9 Atomic Cognitive Cycle
- **Status:** SPECIFICATION_CLOSED
- **Version:** 1.0.0
- **Documentation Level:** Struct-level layout (not byte-level, not conceptual)
- **Related Specifications:**
  - `ffi_memory_contract.md` (semantic contracts)
  - `PATCH_8_AGENT5_DESIGN.md` (OpaqueHandlePool design)
  - `PATCH_9_AGENT5_DOCS_VALIDATION.md` (documentation format decision)
  - `PATCH_10_AGENT5_FFI_DEPENDENCY.md` (FFI dependency clarification)
- **Next Gate:** Agent 2 FPV Auditor verification (gates implementation)
- **Deterministic Receipt:** BLAKE3(ffi_memory_layout.md) = ABI contract extension

---

**END OF MEMORY LAYOUT SPECIFICATION**

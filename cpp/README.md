# QLeverest FFI - C++ to Rust Foreign Function Interface

**EPIC 13 Phase 4 - Agent 6 Implementation**

## Overview

This directory contains the C++ implementation of FFI bindings for QLever, enabling Rust code to call QLever's C++ query engine directly.

## Files

- **qleverest_ffi_impl.cpp** - Implementation of the FFI interface defined in `/include/qleverest/qleverest_ffi.h`
- **test_ffi.cpp** - Test executable to verify FFI bindings work correctly
- **CMakeLists.txt** - Build configuration for the FFI library
- **README.md** - This file

## Architecture

### FFI Design Principles

1. **C ABI Compatibility**: All exposed functions use C linkage (`extern "C"`)
2. **Opaque Handles**: C++ objects are wrapped in opaque `void*` pointers
3. **Error Handling**: Thread-local error state with `qleverest_get_last_error()`
4. **Zero-Copy**: Direct pointer access to IdTable data where possible
5. **Memory Safety**: Clear ownership rules (caller owns handles, must destroy)

### Implementation Details

The FFI wrapper implements the following QLever workflow:

```
1. Open Index         -> qleverest_index_open()
2. Create QEC         -> qleverest_qec_create()
3. Parse Query        -> qleverest_parse_query()
4. Plan Query         -> qleverest_plan_query()
5. Execute Query      -> qleverest_execute_query()
6. Access Results     -> qleverest_result_get_idtable()
7. Read Data          -> qleverest_idtable_get_column_data()
8. Cleanup            -> qleverest_*_destroy()
```

### Actual QLever C++ APIs Used

- **Index**: `src/index/Index.h`
  - `createFromOnDiskIndex()` - Opens index from disk
  - `numTriples()`, `numDistinctSubjects()`, etc. - Statistics

- **QueryExecutionContext**: `src/engine/QueryExecutionContext.h`
  - Constructor with Index, allocator, cache
  - Manages query execution lifecycle

- **ParsedQuery**: `src/parser/ParsedQuery.h` (via SparqlParser)
  - `SparqlParser::parseQuery()` - Parses SPARQL strings
  - `hasSelectClause()`, `hasConstructClause()` - Query type detection

- **QueryExecutionTree**: `src/engine/QueryExecutionTree.h`
  - `QueryPlanner::createExecutionTree()` - Plans query execution
  - `getSizeEstimate()`, `getCostEstimate()` - Query metrics
  - `getResult()` - Executes query

- **Result**: `src/engine/Result.h`
  - `idTable()` - Access to result data
  - `numRows()`, `numColumns()` - Result dimensions

- **IdTable**: `src/engine/idTable/IdTable.h`
  - `getColumn()` - Zero-copy column access
  - `operator()(row, col)` - Cell access

## Building

### Prerequisites

The FFI library requires all QLever dependencies:
- CMake 3.27+
- C++20 compiler (GCC 11+ or Clang 16+)
- ICU libraries
- Boost
- All other QLever dependencies

### Build Commands

```bash
# From QLever root directory
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make qleverest_ffi
make test_ffi
```

### Testing

```bash
# Run FFI test
./build/cpp/test_ffi

# Expected output:
# Testing QLeverest FFI bindings...
# ABI Version: 1.0.0
# ✓ Error handling works
# ✓ Query parsed successfully
# ✓ Index open correctly failed with error
```

## Rust Integration

The corresponding Rust bindings are in:
`/rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs`

### Rust Usage Example

```rust
use qlever_kernel_runner::ffi_bindings::*;

// Open index
let index = Index::open("/path/to/index")?;

// Get statistics
let stats = index.get_stats()?;
println!("Triples: {}", stats.num_triples);

// Create query execution context
let qec = QueryExecutionContext::new(&index)?;

// Parse query
let parsed = ParsedQuery::parse("SELECT * WHERE { ?s ?p ?o } LIMIT 10")?;

// Plan query
let qet = QueryExecutionTree::plan(&qec, &parsed)?;
println!("Size estimate: {}", qet.size_estimate());

// Execute query
let result = QueryResult::execute(&qet)?;
let idtable = result.idtable()?;

// Access results (zero-copy)
println!("Results: {} rows x {} cols",
    idtable.num_rows(), idtable.num_columns());

for row in 0..idtable.num_rows() {
    for col in 0..idtable.num_columns() {
        let value = idtable.get_cell(row, col)?;
        println!("  [{}, {}] = {}", row, col, value);
    }
}
```

## Memory Safety Guarantees

### Ownership Rules

1. **Index Handle**: Owned by Rust, destroyed by `qleverest_index_close()`
2. **QEC Handle**: Owned by Rust, destroyed by `qleverest_qec_destroy()`
3. **ParsedQuery Handle**: Owned by Rust, destroyed by `qleverest_parsed_query_destroy()`
4. **QET Handle**: Owned by Rust, destroyed by `qleverest_qet_destroy()`
5. **Result Handle**: Owned by Rust, destroyed by `qleverest_result_destroy()`
6. **IdTable Handle**: Borrowed from Result, valid while Result is alive

### Safety Invariants

- All C++ exceptions are caught and converted to error codes
- Thread-local error storage prevents error state leakage between threads
- Null pointer checks on all handle accesses
- Bounds checking on IdTable row/column access

## Testing Strategy

### Unit Tests

1. **ABI Version Check**: Verify version string is "1.0.0"
2. **Error Handling**: Verify error state is properly managed
3. **Query Parsing**: Test valid and invalid SPARQL queries
4. **Null Safety**: Verify NULL handle handling

### Integration Tests

1. **Full Query Workflow**: Index open -> Parse -> Plan -> Execute -> Results
2. **Zero-Copy Access**: Verify column data access without copying
3. **Multi-Query**: Verify multiple queries on same index
4. **Error Recovery**: Verify error doesn't corrupt state

### Test Data

Tests require a small QLever index. Create test index:

```bash
# Create tiny test index
echo "<s1> <p1> <o1> ." > test.nt
echo "<s2> <p2> <o2> ." >> test.nt
IndexBuilderMain -i test.nt -F nt -f test_index
```

## Known Limitations (Phase 4 v1.0)

1. **Row Iterator**: Not yet implemented (stub returns error)
2. **Cache Management**: Cache pinning functions not implemented
3. **Vocabulary String Resolution**: `vocab_string_to_id` not implemented
4. **Result Serialization**: `qleverest_query_json` convenience function not implemented

These are non-blocking for initial FFI validation and can be implemented in subsequent phases.

## Verification

To verify the implementation is working:

```bash
# Build
make qleverest_ffi test_ffi

# Test
./build/cpp/test_ffi

# Link test (should link without errors)
g++ -o ffi_link_test test_ffi.cpp -lqleverest_ffi -lstdc++
```

## Next Steps

1. Add row iterator implementation
2. Implement cache management functions
3. Add vocabulary string-to-ID conversion
4. Implement JSON serialization convenience function
5. Add comprehensive integration tests with real QLever indices
6. Benchmark zero-copy performance vs. copying
7. Add FFI stability tests (ABI versioning validation)

## References

- QLever Index: `src/index/Index.h`
- Query Execution: `src/engine/QueryExecutionContext.h`
- Query Planning: `src/engine/QueryPlanner.h`
- Results: `src/engine/Result.h`
- FFI Header: `include/qleverest/qleverest_ffi.h`
- Rust Bindings: `rust/qleverest-validation/crates/qlever-kernel-runner/src/ffi_bindings.rs`

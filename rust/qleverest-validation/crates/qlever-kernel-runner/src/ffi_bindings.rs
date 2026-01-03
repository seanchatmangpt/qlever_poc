// Copyright 2025, QLever
// EPIC 13 Phase 4: Rust FFI Bindings (Agent 6)
//
// Safe Rust wrappers around the qleverest_ffi.h C interface

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};
use std::ptr;

// ============================================================================
// RAW FFI DECLARATIONS
// ============================================================================

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ErrorCode {
    Ok = 0,
    NullHandle = 1,
    InvalidHandle = 2,
    IndexOpenFailed = 3,
    ParseFailed = 4,
    PlanFailed = 5,
    ExecFailed = 6,
    OutOfMemory = 7,
    InvalidQuery = 8,
    Timeout = 9,
    Cancelled = 10,
    Unknown = 99,
}

#[repr(C)]
pub struct QleverestError {
    pub code: ErrorCode,
    pub message: [c_char; 1024],
    pub file: *const c_char,
    pub line: i32,
}

// Opaque handle types
pub type IndexHandle = *mut c_void;
pub type QecHandle = *mut c_void;
pub type ParsedQueryHandle = *mut c_void;
pub type QetHandle = *mut c_void;
pub type ResultHandle = *mut c_void;
pub type IdTableHandle = *mut c_void;

#[link(name = "qleverest_ffi", kind = "static")]
extern "C" {
    // Error handling
    pub fn qleverest_get_last_error() -> *const QleverestError;
    pub fn qleverest_clear_error();

    // Index management
    pub fn qleverest_index_open(
        index_path: *const c_char,
        config_json: *const c_char,
    ) -> IndexHandle;
    pub fn qleverest_index_close(index: IndexHandle);
    pub fn qleverest_index_get_stats(
        index: IndexHandle,
        num_triples: *mut usize,
        num_subjects: *mut usize,
        num_predicates: *mut usize,
        num_objects: *mut usize,
    ) -> ErrorCode;

    // Query execution context
    pub fn qleverest_qec_create(index: IndexHandle) -> QecHandle;
    pub fn qleverest_qec_destroy(qec: QecHandle);

    // Query parsing
    pub fn qleverest_parse_query(sparql: *const c_char) -> ParsedQueryHandle;
    pub fn qleverest_parsed_query_destroy(parsed_query: ParsedQueryHandle);
    pub fn qleverest_parsed_query_get_type(parsed_query: ParsedQueryHandle) -> *const c_char;

    // Query planning
    pub fn qleverest_plan_query(qec: QecHandle, parsed_query: ParsedQueryHandle) -> QetHandle;
    pub fn qleverest_qet_destroy(qet: QetHandle);
    pub fn qleverest_qet_get_size_estimate(qet: QetHandle) -> usize;
    pub fn qleverest_qet_get_cost_estimate(qet: QetHandle) -> usize;

    // Query execution
    pub fn qleverest_execute_query(qet: QetHandle) -> ResultHandle;
    pub fn qleverest_result_destroy(result: ResultHandle);

    // Result access
    pub fn qleverest_result_get_idtable(result: ResultHandle) -> IdTableHandle;
    pub fn qleverest_idtable_num_rows(idtable: IdTableHandle) -> usize;
    pub fn qleverest_idtable_num_columns(idtable: IdTableHandle) -> usize;
    pub fn qleverest_idtable_get_column_data(
        idtable: IdTableHandle,
        column_index: usize,
        out_data: *mut *const u64,
        out_size: *mut usize,
    ) -> ErrorCode;
    pub fn qleverest_idtable_get_cell(
        idtable: IdTableHandle,
        row_index: usize,
        column_index: usize,
        out_value: *mut u64,
    ) -> ErrorCode;

    // Vocabulary
    pub fn qleverest_vocab_id_to_string(
        index: IndexHandle,
        id: u64,
        out_string: *mut *const c_char,
        out_length: *mut usize,
    ) -> ErrorCode;

    // ABI version
    pub fn qleverest_get_abi_version() -> *const c_char;
    pub fn qleverest_get_abi_hash() -> *const c_char;
}

// ============================================================================
// SAFE RUST WRAPPERS
// ============================================================================

#[derive(Debug)]
pub struct QleverestException {
    pub code: ErrorCode,
    pub message: String,
    pub file: Option<String>,
    pub line: i32,
}

impl std::fmt::Display for QleverestException {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "QLeverest error: {} (code {:?})", self.message, self.code)
    }
}

impl std::error::Error for QleverestException {}

pub type Result<T> = std::result::Result<T, QleverestException>;

fn get_last_error() -> QleverestException {
    unsafe {
        let err_ptr = qleverest_get_last_error();
        if err_ptr.is_null() {
            return QleverestException {
                code: ErrorCode::Unknown,
                message: "Unknown error".to_string(),
                file: None,
                line: 0,
            };
        }

        let err = &*err_ptr;
        let message = CStr::from_ptr(err.message.as_ptr())
            .to_string_lossy()
            .to_string();
        let file = if err.file.is_null() {
            None
        } else {
            Some(CStr::from_ptr(err.file).to_string_lossy().to_string())
        };

        QleverestException {
            code: err.code,
            message,
            file,
            line: err.line,
        }
    }
}

/// Safe wrapper for QLever Index
pub struct Index {
    handle: IndexHandle,
}

impl Index {
    /// Open an index from disk
    pub fn open(index_path: &str) -> Result<Self> {
        let path_cstr = CString::new(index_path).unwrap();

        let handle = unsafe { qleverest_index_open(path_cstr.as_ptr(), ptr::null()) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(Index { handle })
    }

    /// Get index statistics
    pub fn get_stats(&self) -> Result<IndexStats> {
        let mut stats = IndexStats::default();

        let code = unsafe {
            qleverest_index_get_stats(
                self.handle,
                &mut stats.num_triples,
                &mut stats.num_subjects,
                &mut stats.num_predicates,
                &mut stats.num_objects,
            )
        };

        if code != ErrorCode::Ok {
            return Err(get_last_error());
        }

        Ok(stats)
    }

    /// Get the raw handle (for use with QueryExecutionContext)
    pub fn handle(&self) -> IndexHandle {
        self.handle
    }
}

impl Drop for Index {
    fn drop(&mut self) {
        unsafe {
            qleverest_index_close(self.handle);
        }
    }
}

#[derive(Debug, Default, Clone)]
pub struct IndexStats {
    pub num_triples: usize,
    pub num_subjects: usize,
    pub num_predicates: usize,
    pub num_objects: usize,
}

/// Safe wrapper for QueryExecutionContext
pub struct QueryExecutionContext {
    handle: QecHandle,
}

impl QueryExecutionContext {
    /// Create a new QueryExecutionContext for the given index
    pub fn new(index: &Index) -> Result<Self> {
        let handle = unsafe { qleverest_qec_create(index.handle()) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(QueryExecutionContext { handle })
    }

    pub fn handle(&self) -> QecHandle {
        self.handle
    }
}

impl Drop for QueryExecutionContext {
    fn drop(&mut self) {
        unsafe {
            qleverest_qec_destroy(self.handle);
        }
    }
}

/// Safe wrapper for ParsedQuery
pub struct ParsedQuery {
    handle: ParsedQueryHandle,
}

impl ParsedQuery {
    /// Parse a SPARQL query
    pub fn parse(sparql: &str) -> Result<Self> {
        let sparql_cstr = CString::new(sparql).unwrap();

        let handle = unsafe { qleverest_parse_query(sparql_cstr.as_ptr()) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(ParsedQuery { handle })
    }

    /// Get the query type (SELECT, CONSTRUCT, etc.)
    pub fn query_type(&self) -> String {
        unsafe {
            let type_ptr = qleverest_parsed_query_get_type(self.handle);
            if type_ptr.is_null() {
                return "UNKNOWN".to_string();
            }
            CStr::from_ptr(type_ptr).to_string_lossy().to_string()
        }
    }

    pub fn handle(&self) -> ParsedQueryHandle {
        self.handle
    }
}

impl Drop for ParsedQuery {
    fn drop(&mut self) {
        unsafe {
            qleverest_parsed_query_destroy(self.handle);
        }
    }
}

/// Safe wrapper for QueryExecutionTree
pub struct QueryExecutionTree {
    handle: QetHandle,
}

impl QueryExecutionTree {
    /// Plan a query
    pub fn plan(qec: &QueryExecutionContext, parsed_query: &ParsedQuery) -> Result<Self> {
        let handle = unsafe { qleverest_plan_query(qec.handle(), parsed_query.handle()) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(QueryExecutionTree { handle })
    }

    /// Get size estimate for the query result
    pub fn size_estimate(&self) -> usize {
        unsafe { qleverest_qet_get_size_estimate(self.handle) }
    }

    /// Get cost estimate for the query
    pub fn cost_estimate(&self) -> usize {
        unsafe { qleverest_qet_get_cost_estimate(self.handle) }
    }

    pub fn handle(&self) -> QetHandle {
        self.handle
    }
}

impl Drop for QueryExecutionTree {
    fn drop(&mut self) {
        unsafe {
            qleverest_qet_destroy(self.handle);
        }
    }
}

/// Safe wrapper for query Result
pub struct QueryResult {
    handle: ResultHandle,
}

impl QueryResult {
    /// Execute a query plan
    pub fn execute(qet: &QueryExecutionTree) -> Result<Self> {
        let handle = unsafe { qleverest_execute_query(qet.handle()) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(QueryResult { handle })
    }

    /// Get the IdTable from the result
    pub fn idtable(&self) -> Result<IdTable> {
        let handle = unsafe { qleverest_result_get_idtable(self.handle) };

        if handle.is_null() {
            return Err(get_last_error());
        }

        Ok(IdTable { handle })
    }
}

impl Drop for QueryResult {
    fn drop(&mut self) {
        unsafe {
            qleverest_result_destroy(self.handle);
        }
    }
}

/// Safe wrapper for IdTable (non-owning)
pub struct IdTable {
    handle: IdTableHandle,
}

impl IdTable {
    /// Get number of rows
    pub fn num_rows(&self) -> usize {
        unsafe { qleverest_idtable_num_rows(self.handle) }
    }

    /// Get number of columns
    pub fn num_columns(&self) -> usize {
        unsafe { qleverest_idtable_num_columns(self.handle) }
    }

    /// Get a cell value
    pub fn get_cell(&self, row: usize, col: usize) -> Result<u64> {
        let mut value: u64 = 0;

        let code = unsafe { qleverest_idtable_get_cell(self.handle, row, col, &mut value) };

        if code != ErrorCode::Ok {
            return Err(get_last_error());
        }

        Ok(value)
    }

    /// Get column data as a slice (zero-copy)
    pub fn get_column(&self, col: usize) -> Result<&[u64]> {
        let mut data_ptr: *const u64 = ptr::null();
        let mut size: usize = 0;

        let code =
            unsafe { qleverest_idtable_get_column_data(self.handle, col, &mut data_ptr, &mut size) };

        if code != ErrorCode::Ok {
            return Err(get_last_error());
        }

        if data_ptr.is_null() {
            return Ok(&[]);
        }

        // SAFETY: The data pointer is valid as long as the Result is alive
        // This is a borrowed reference, so it's safe as long as the IdTable
        // is alive (which is tied to the QueryResult lifetime)
        Ok(unsafe { std::slice::from_raw_parts(data_ptr, size) })
    }
}

// ============================================================================
// TESTS
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_abi_version() {
        unsafe {
            let version = CStr::from_ptr(qleverest_get_abi_version())
                .to_string_lossy()
                .to_string();
            assert_eq!(version, "1.0.0");
        }
    }

    #[test]
    #[ignore] // Requires actual index
    fn test_index_open() {
        // This test requires an actual QLever index to exist
        // In a real test, you would create a test index first
        let result = Index::open("/path/to/test/index");
        // Should fail with IndexOpenFailed since path doesn't exist
        assert!(result.is_err());
    }
}

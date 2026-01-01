//! Raw C FFI bindings to libqlever
//!
//! This module declares the C interface to the QLever C++ library.
//! All pointers are opaque (never dereferenced from Rust).
//! Safe Rust wrappers in `types.rs` provide the public API.

use std::ffi::c_char;
use std::ffi::c_uint;

/// Opaque type for Qlever engine instance
/// Never dereferenced from Rust - ownership managed by QleverHandle wrapper
pub struct QleverOpaque;

/// Opaque type for query execution plans
/// Never dereferenced from Rust - ownership managed by QueryPlan wrapper
pub struct QueryPlanOpaque;

/// Media type enum for result formats
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum MediaType {
    SparqlJson = 0,
    SparqlXml = 1,
    TurtleFormat = 2,
    NTriplesFormat = 3,
    CsvFormat = 4,
    TsvFormat = 5,
    QLeverJson = 6,
}

#[link(name = "qlever_ffi")]
extern "C" {
    // ================== Qlever Instance ==================

    /// Create a new Qlever engine instance
    ///
    /// # Arguments
    /// * `config_json` - JSON configuration string (null-terminated)
    ///
    /// # Returns
    /// Opaque pointer to Qlever instance, or NULL on error.
    /// Call `qlever_get_last_error()` to get error message.
    pub fn qlever_new(config_json: *const c_char) -> *mut QleverOpaque;

    /// Free a Qlever engine instance
    ///
    /// # Safety
    /// The pointer must be valid (created by qlever_new).
    /// After calling this, the pointer becomes invalid.
    pub fn qlever_free(ptr: *mut QleverOpaque);

    // ================== Query Execution ==================

    /// Execute a SPARQL query
    ///
    /// # Arguments
    /// * `ptr` - Valid Qlever instance pointer
    /// * `query` - SPARQL query string (null-terminated)
    /// * `format` - Result format (MediaType enum)
    ///
    /// # Returns
    /// Result string as JSON/Turtle/etc (caller must call qlever_free_string).
    /// Returns NULL on error. Call `qlever_get_last_error()` for details.
    pub fn qlever_query(
        ptr: *mut QleverOpaque,
        query: *const c_char,
        format: u32,
    ) -> *const c_char;

    /// Parse and create execution plan for a SPARQL query
    ///
    /// # Arguments
    /// * `ptr` - Valid Qlever instance pointer
    /// * `query` - SPARQL query string (null-terminated)
    ///
    /// # Returns
    /// Opaque query plan pointer (use with `qlever_execute_plan`).
    /// Returns NULL on error. Call `qlever_get_last_error()` for details.
    pub fn qlever_parse_and_plan(
        ptr: *mut QleverOpaque,
        query: *const c_char,
    ) -> *mut QueryPlanOpaque;

    /// Execute a pre-compiled query plan
    ///
    /// # Arguments
    /// * `ptr` - Valid Qlever instance pointer
    /// * `plan` - Query plan from `qlever_parse_and_plan`
    /// * `format` - Result format (MediaType enum)
    ///
    /// # Returns
    /// Result string (caller must call qlever_free_string).
    /// Returns NULL on error.
    pub fn qlever_execute_plan(
        ptr: *mut QleverOpaque,
        plan: *mut QueryPlanOpaque,
        format: u32,
    ) -> *const c_char;

    /// Free a query plan
    pub fn qlever_plan_free(plan: *mut QueryPlanOpaque);

    // ================== Named Result Caching ==================

    /// Cache a query result with a name
    ///
    /// # Returns
    /// 0 on success, -1 on error
    pub fn qlever_query_and_pin(
        ptr: *mut QleverOpaque,
        name: *const c_char,
        query: *const c_char,
    ) -> c_uint;

    /// Retrieve a cached result by name
    ///
    /// # Returns
    /// Cached result as JSON (caller must call qlever_free_string).
    /// Returns NULL if not found or on error.
    pub fn qlever_get_pinned_result(
        ptr: *mut QleverOpaque,
        name: *const c_char,
    ) -> *const c_char;

    /// Clear a cached result
    pub fn qlever_clear_pinned(
        ptr: *mut QleverOpaque,
        name: *const c_char,
    ) -> c_uint;

    // ================== Materialized Views ==================

    /// Create and write a materialized view
    ///
    /// # Returns
    /// 0 on success, -1 on error
    pub fn qlever_write_materialized_view(
        ptr: *mut QleverOpaque,
        name: *const c_char,
        query: *const c_char,
    ) -> c_uint;

    /// Load a materialized view
    ///
    /// # Returns
    /// 0 on success, -1 on error
    pub fn qlever_load_materialized_view(
        ptr: *mut QleverOpaque,
        name: *const c_char,
    ) -> c_uint;

    /// List all available materialized views
    ///
    /// # Returns
    /// JSON array of view names (caller must call qlever_free_string).
    /// Returns NULL on error.
    pub fn qlever_list_materialized_views(
        ptr: *mut QleverOpaque,
    ) -> *const c_char;

    // ================== Statistics ==================

    /// Get server statistics
    ///
    /// # Returns
    /// JSON string with server stats (caller must call qlever_free_string).
    /// Returns NULL on error.
    pub fn qlever_server_stats(ptr: *mut QleverOpaque) -> *const c_char;

    /// Get cache statistics
    ///
    /// # Returns
    /// JSON string with cache stats (caller must call qlever_free_string).
    /// Returns NULL on error.
    pub fn qlever_cache_stats(ptr: *mut QleverOpaque) -> *const c_char;

    /// Get index statistics
    ///
    /// # Returns
    /// JSON string with index stats (caller must call qlever_free_string).
    /// Returns NULL on error.
    pub fn qlever_index_stats(ptr: *mut QleverOpaque) -> *const c_char;

    // ================== Memory Management ==================

    /// Free a string allocated by C++
    ///
    /// # Safety
    /// The pointer must be from a C FFI function result.
    pub fn qlever_free_string(ptr: *const c_char);

    /// Get the last error message
    ///
    /// # Returns
    /// Null-terminated error string. Valid only until next FFI call.
    /// Do not free this string.
    pub fn qlever_get_last_error() -> *const c_char;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_media_type_values() {
        assert_eq!(MediaType::SparqlJson as u32, 0);
        assert_eq!(MediaType::SparqlXml as u32, 1);
        assert_eq!(MediaType::TurtleFormat as u32, 2);
    }
}

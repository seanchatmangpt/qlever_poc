//! Low-level FFI bindings to libqlever C wrapper
//!
//! This module provides raw C function bindings. Use the safe `Store` wrapper instead
//! for a proper Rust interface.

use std::os::raw::c_char;

pub type QleverHandle = *mut std::ffi::c_void;
pub type QueryPlanHandle = *mut std::ffi::c_void;

extern "C" {
    /// Opens a QLever index from disk.
    pub fn qlever_open(index_path: *const c_char, config_json: *const c_char) -> QleverHandle;

    /// Executes a SPARQL query and returns JSON results.
    pub fn qlever_query_json(
        h: QleverHandle,
        sparql: *const c_char,
        detailed_timings: i32,
    ) -> *mut c_char;

    /// Parses and plans a SPARQL query separately.
    pub fn qlever_parse_and_plan(h: QleverHandle, sparql: *const c_char) -> QueryPlanHandle;

    /// Executes a previously parsed and planned query.
    pub fn qlever_execute_plan(
        h: QleverHandle,
        plan: QueryPlanHandle,
        detailed_timings: i32,
    ) -> *mut c_char;

    /// Frees a query plan handle.
    pub fn qlever_free_plan(plan: QueryPlanHandle);

    /// Caches a query result with a name.
    pub fn qlever_pin_result(h: QleverHandle, name: *const c_char, sparql: *const c_char);

    /// Clears a named cached result.
    pub fn qlever_erase_result(h: QleverHandle, name: *const c_char);

    /// Clears all cached results.
    pub fn qlever_clear_cache(h: QleverHandle);

    /// Writes a materialized view.
    pub fn qlever_write_materialized_view(
        h: QleverHandle,
        name: *const c_char,
        sparql: *const c_char,
    );

    /// Loads a materialized view.
    pub fn qlever_load_materialized_view(h: QleverHandle, name: *const c_char);

    /// Full-text text search.
    pub fn qlever_text_search(h: QleverHandle, text_query: *const c_char, limit: i32) -> *mut c_char;

    /// Frees a string returned by qlever functions.
    pub fn qlever_free_string(s: *mut c_char);

    /// Closes a QLever handle.
    pub fn qlever_close(h: QleverHandle);
}


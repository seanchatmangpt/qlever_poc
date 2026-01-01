//! Low-level FFI bindings to libqlever C wrapper
//!
//! This module provides raw C function bindings. Use the safe `Store` wrapper instead
//! for a proper Rust interface.

use std::os::raw::c_char;

pub type QleverHandle = *mut std::ffi::c_void;

extern "C" {
    /// Opens a QLever index from disk.
    pub fn qlever_open(index_path: *const c_char, config_json: *const c_char) -> QleverHandle;

    /// Executes a SPARQL query and returns JSON results.
    pub fn qlever_query_json(
        h: QleverHandle,
        sparql: *const c_char,
        detailed_timings: i32,
    ) -> *mut c_char;

    /// Frees a string returned by qlever_query_json.
    pub fn qlever_free_string(s: *mut c_char);

    /// Closes a QLever handle.
    pub fn qlever_close(h: QleverHandle);
}

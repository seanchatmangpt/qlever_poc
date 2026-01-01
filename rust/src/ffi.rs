//! Raw FFI bindings to QLever C++ engine
//!
//! This module provides zero-overhead C bindings to the QLever library.
//! All functions are marked unsafe because they directly interface with C++.
//! The caller is responsible for memory safety.
//!
//! # Design
//!
//! - **No type conversion**: Raw pointers and C types only
//! - **No copying**: All data is shared via pointers
//! - **C++ owns memory**: Rust just holds references
//! - **Minimal marshaling**: Direct pointer passing

use std::os::raw::{c_char, c_uint};

/// Opaque pointer to a QLever Store instance
pub type QleverStore = std::ffi::c_void;

/// Opaque pointer to a QLever Query Result
pub type QleverResult = std::ffi::c_void;

/// Raw RDF triple representation (C-compatible struct)
#[repr(C)]
pub struct CTriple {
    /// Subject URI as C string
    pub subject: *const c_char,
    /// Predicate URI as C string
    pub predicate: *const c_char,
    /// Object (URI, blank node, or literal) as C string
    pub object: *const c_char,
}

/// Raw query result binding
#[repr(C)]
pub struct CBinding {
    /// Variable name as C string
    pub var: *const c_char,
    /// Bound value as C string
    pub value: *const c_char,
}

extern "C" {
    /// Creates a new empty QLever store
    /// Returns opaque pointer to store, or null on failure
    pub fn qlever_store_new() -> *mut QleverStore;

    /// Loads an RDF file into the store
    pub fn qlever_store_load(
        store: *mut QleverStore,
        filepath: *const c_char,
    ) -> i32;

    /// Inserts a single triple into the store
    /// Returns 0 on success, -1 on error
    pub fn qlever_store_insert(
        store: *mut QleverStore,
        subject: *const c_char,
        predicate: *const c_char,
        object: *const c_char,
    ) -> i32;

    /// Executes a SPARQL query
    /// Returns opaque pointer to result, or null on error
    pub fn qlever_query(
        store: *const QleverStore,
        sparql: *const c_char,
    ) -> *mut QleverResult;

    /// Gets the number of result rows
    pub fn qlever_result_count(result: *const QleverResult) -> c_uint;

    /// Gets the number of variables in result
    pub fn qlever_result_vars(result: *const QleverResult) -> c_uint;

    /// Gets the variable names from result
    /// Returns pointer to array of C strings
    pub fn qlever_result_var_names(result: *const QleverResult) -> *const *const c_char;

    /// Gets a triple from the result at index
    /// Returns pointer to CTriple struct, or null if index out of bounds
    pub fn qlever_result_get_triple(
        result: *const QleverResult,
        index: c_uint,
    ) -> *const CTriple;

    /// Gets bindings for a row
    /// Returns pointer to array of CBinding, count in out parameter
    pub fn qlever_result_get_bindings(
        result: *const QleverResult,
        row: c_uint,
        count: *mut c_uint,
    ) -> *const CBinding;

    /// Gets all triples in store
    /// Sets count to number of triples, returns pointer to array
    pub fn qlever_store_get_all_triples(
        store: *const QleverStore,
        count: *mut c_uint,
    ) -> *const CTriple;

    /// Gets triple count
    pub fn qlever_store_triple_count(store: *const QleverStore) -> c_uint;

    /// Frees a result object
    pub fn qlever_result_free(result: *mut QleverResult);

    /// Frees a store object
    pub fn qlever_store_free(store: *mut QleverStore);

    /// Frees an array of C strings
    pub fn qlever_free_string_array(arr: *mut *mut c_char);

    /// Frees a generic pointer allocated by QLever
    pub fn qlever_free(ptr: *mut std::ffi::c_void);
}

// Tests require C++ library linking - see build.rs for linking configuration

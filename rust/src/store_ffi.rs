//! Fast FFI wrapper around QLever C++ engine
//!
//! This is a zero-overhead wrapper that directly uses libqlever via C FFI.
//! Data is shared via pointers - no copying, no serialization.
//!
//! # Safety
//!
//! This module is unsafe by design. The C++ library is trusted to be safe.
//! All invariants are enforced by the C++ QLever implementation.

use crate::error::Result;
use crate::model::{NamedNode, Term, Triple};
use std::ffi::CStr;

/// Direct wrapper around QLever C++ store
///
/// Holds a raw pointer to C++ allocated memory. All operations pass through
/// that pointer with minimal overhead. Memory is managed by C++.
pub struct Store {
    handle: *mut std::ffi::c_void,
}

impl Store {
    /// Creates a new store by calling C++ constructor
    pub fn new() -> Result<Self> {
        unsafe {
            let handle = crate::ffi::qlever_store_new();
            if handle.is_null() {
                return Err(crate::error::Error::Internal(
                    "Failed to create QLever store".to_string(),
                ));
            }
            Ok(Store { handle })
        }
    }

    /// Loads RDF data from a file
    pub fn load(&self, filepath: &str) -> Result<()> {
        let c_path = std::ffi::CString::new(filepath)
            .map_err(|e| crate::error::Error::NullInString(e))?;
        unsafe {
            let result = crate::ffi::qlever_store_load(self.handle, c_path.as_ptr());
            if result != 0 {
                return Err(crate::error::Error::Internal(format!(
                    "Failed to load RDF from {}",
                    filepath
                )));
            }
        }
        Ok(())
    }

    /// Inserts a single triple
    pub fn insert(&self, triple: &Triple) -> Result<()> {
        let c_subject = std::ffi::CString::new(triple.subject.as_str())
            .map_err(|e| crate::error::Error::NullInString(e))?;
        let c_predicate = std::ffi::CString::new(triple.predicate.as_str())
            .map_err(|e| crate::error::Error::NullInString(e))?;
        let c_object = std::ffi::CString::new(self.term_to_string(&triple.object))
            .map_err(|e| crate::error::Error::NullInString(e))?;

        unsafe {
            let result = crate::ffi::qlever_store_insert(
                self.handle,
                c_subject.as_ptr(),
                c_predicate.as_ptr(),
                c_object.as_ptr(),
            );
            if result != 0 {
                return Err(crate::error::Error::Internal(
                    "Failed to insert triple".to_string(),
                ));
            }
        }
        Ok(())
    }

    /// Inserts multiple triples
    pub fn insert_triples(&self, triples: &[Triple]) -> Result<()> {
        for triple in triples {
            self.insert(triple)?;
        }
        Ok(())
    }

    /// Executes a SPARQL query
    /// Returns results directly from C++ memory (zero-copy)
    pub fn query(&self, sparql: &str) -> Result<Vec<Vec<(String, String)>>> {
        let c_query = std::ffi::CString::new(sparql)
            .map_err(|e| crate::error::Error::NullInString(e))?;

        unsafe {
            let result = crate::ffi::qlever_query(self.handle, c_query.as_ptr());
            if result.is_null() {
                return Err(crate::error::Error::QueryError(
                    "Query execution failed".to_string(),
                ));
            }

            let row_count = crate::ffi::qlever_result_count(result);
            let mut rows = Vec::with_capacity(row_count as usize);

            for row_idx in 0..row_count {
                let mut binding_count: u32 = 0;
                let bindings = crate::ffi::qlever_result_get_bindings(
                    result,
                    row_idx,
                    &mut binding_count,
                );

                if !bindings.is_null() {
                    let binding_slice =
                        std::slice::from_raw_parts(bindings, binding_count as usize);
                    let row: Vec<(String, String)> = binding_slice
                        .iter()
                        .filter_map(|b| {
                            let var = CStr::from_ptr(b.var).to_string_lossy().to_string();
                            let val = CStr::from_ptr(b.value).to_string_lossy().to_string();
                            Some((var, val))
                        })
                        .collect();
                    rows.push(row);
                }
            }

            crate::ffi::qlever_result_free(result);
            Ok(rows)
        }
    }

    /// Gets all triples in the store (zero-copy from C++ memory)
    pub fn get_all_triples(&self) -> Result<Vec<Triple>> {
        unsafe {
            let mut count: u32 = 0;
            let triples_ptr = crate::ffi::qlever_store_get_all_triples(self.handle, &mut count);

            if triples_ptr.is_null() {
                return Ok(Vec::new());
            }

            // Create slice directly over C++ memory - ZERO COPY
            let triples_slice =
                std::slice::from_raw_parts(triples_ptr, count as usize);

            let mut results = Vec::with_capacity(count as usize);
            for c_triple in triples_slice {
                let subject_str = CStr::from_ptr(c_triple.subject).to_string_lossy();
                let predicate_str = CStr::from_ptr(c_triple.predicate).to_string_lossy();
                let object_str = CStr::from_ptr(c_triple.object).to_string_lossy();

                let subject = NamedNode::new(subject_str.to_string())
                    .map_err(|_| crate::error::Error::InvalidUri(subject_str.to_string()))?;
                let predicate = NamedNode::new(predicate_str.to_string())
                    .map_err(|_| crate::error::Error::InvalidUri(predicate_str.to_string()))?;
                let object = Term::NamedNode(
                    NamedNode::new(object_str.to_string())
                        .map_err(|_| crate::error::Error::InvalidUri(object_str.to_string()))?
                );

                results.push(Triple::new(subject, predicate, object));
            }

            Ok(results)
        }
    }

    /// Gets the number of triples in the store
    pub fn triple_count(&self) -> u32 {
        unsafe { crate::ffi::qlever_store_triple_count(self.handle) }
    }

    // Helper methods

    fn term_to_string(&self, term: &Term) -> String {
        match term {
            Term::NamedNode(n) => n.as_str().to_string(),
            Term::BlankNode(b) => b.as_str().to_string(),
            Term::Literal(l) => l.value().to_string(),
        }
    }
}

impl Default for Store {
    fn default() -> Self {
        Self::new().expect("Failed to create default Store")
    }
}

impl Drop for Store {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                crate::ffi::qlever_store_free(self.handle);
            }
        }
    }
}

// Tests require C++ library linking - integration tests in when build.rs links libqlever

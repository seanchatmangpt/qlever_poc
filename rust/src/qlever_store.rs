//! Safe Rust wrapper around QLever C FFI
//!
//! This module provides the `Store` type which wraps the libqlever C interface
//! with a safe, ergonomic Rust API.

use crate::error::{Error, Result};
use crate::model::{NamedNode, Term};
use crate::query::QuerySolution;
use std::ffi::CString;
use std::path::Path;
use std::ptr::NonNull;
use serde_json::{json, Value};
use std::collections::BTreeMap;

/// Safe handle to a QLever database
pub struct Store {
    handle: NonNull<std::ffi::c_void>,
}

impl Store {
    /// Opens a QLever index from disk
    pub fn open<P: AsRef<Path>>(index_path: P) -> Result<Self> {
        let path = index_path.as_ref().to_string_lossy().to_string();
        let c_path = CString::new(path)?;

        unsafe {
            let handle = crate::ffi::qlever_open(c_path.as_ptr(), std::ptr::null());
            let handle =
                NonNull::new(handle).ok_or_else(|| Error::Internal("Failed to open QLever index".to_string()))?;

            Ok(Store { handle })
        }
    }

    /// Opens a QLever index with custom configuration
    pub fn open_with_config<P: AsRef<Path>>(index_path: P, config: &str) -> Result<Self> {
        let path = index_path.as_ref().to_string_lossy().to_string();
        let c_path = CString::new(path)?;
        let c_config = CString::new(config)?;

        unsafe {
            let handle = crate::ffi::qlever_open(c_path.as_ptr(), c_config.as_ptr());
            let handle =
                NonNull::new(handle).ok_or_else(|| Error::Internal("Failed to open QLever index".to_string()))?;

            Ok(Store { handle })
        }
    }

    /// Executes a SPARQL query and returns bindings
    pub fn query(&self, sparql: &str) -> Result<Vec<QuerySolution>> {
        let c_query = CString::new(sparql)?;

        unsafe {
            let json_result =
                crate::ffi::qlever_query_json(self.handle.as_ptr(), c_query.as_ptr(), 0);

            if json_result.is_null() {
                return Err(Error::QueryError("Query execution failed".to_string()));
            }

            // Convert C string to Rust string
            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            // Free the C string
            crate::ffi::qlever_free_string(json_result);

            // Parse JSON result
            let json: Value = serde_json::from_str(&result_str)?;

            // Extract bindings from SPARQL JSON results format
            let mut solutions = Vec::new();

            if let Some(bindings) = json.get("results")
                .and_then(|r| r.get("bindings"))
                .and_then(|b| b.as_array())
            {
                for binding in bindings {
                    let mut solution_map = BTreeMap::new();

                    if let Some(obj) = binding.as_object() {
                        for (var, value) in obj.iter() {
                            let term_str = value.get("value")
                                .and_then(|v| v.as_str())
                                .unwrap_or("");

                            // Simple term parsing - in production use proper RDF parser
                            let term = if term_str.starts_with("http://") || term_str.starts_with("https://") {
                                Term::NamedNode(NamedNode::new(term_str.to_string())?)
                            } else {
                                Term::Literal(crate::model::Literal::new_simple(term_str))
                            };

                            solution_map.insert(var.clone(), term);
                        }
                    }

                    solutions.push(QuerySolution::new(solution_map));
                }
            }

            Ok(solutions)
        }
    }

    /// Executes a SPARQL query with detailed timing information
    pub fn query_with_timings(&self, sparql: &str) -> Result<(Vec<QuerySolution>, Value)> {
        let c_query = CString::new(sparql)?;

        unsafe {
            let json_result =
                crate::ffi::qlever_query_json(self.handle.as_ptr(), c_query.as_ptr(), 1);

            if json_result.is_null() {
                return Err(Error::QueryError("Query execution failed".to_string()));
            }

            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            crate::ffi::qlever_free_string(json_result);

            let json: Value = serde_json::from_str(&result_str)?;

            // Extract timings
            let timings = json.get("timings").cloned().unwrap_or(json!({}));

            // Parse bindings (same as query())
            let mut solutions = Vec::new();

            if let Some(bindings) = json.get("results")
                .and_then(|r| r.get("bindings"))
                .and_then(|b| b.as_array())
            {
                for binding in bindings {
                    let mut solution_map = BTreeMap::new();

                    if let Some(obj) = binding.as_object() {
                        for (var, value) in obj.iter() {
                            let term_str = value.get("value")
                                .and_then(|v| v.as_str())
                                .unwrap_or("");

                            let term = if term_str.starts_with("http://") || term_str.starts_with("https://") {
                                Term::NamedNode(NamedNode::new(term_str.to_string())?)
                            } else {
                                Term::Literal(crate::model::Literal::new_simple(term_str))
                            };

                            solution_map.insert(var.clone(), term);
                        }
                    }

                    solutions.push(QuerySolution::new(solution_map));
                }
            }

            Ok((solutions, timings))
        }
    }

    /// Gets the raw JSON response for a SPARQL query
    pub fn query_raw_json(&self, sparql: &str) -> Result<Value> {
        let c_query = CString::new(sparql)?;

        unsafe {
            let json_result =
                crate::ffi::qlever_query_json(self.handle.as_ptr(), c_query.as_ptr(), 0);

            if json_result.is_null() {
                return Err(Error::QueryError("Query execution failed".to_string()));
            }

            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            crate::ffi::qlever_free_string(json_result);

            let json = serde_json::from_str(&result_str)?;
            Ok(json)
        }
    }
}

impl Drop for Store {
    fn drop(&mut self) {
        unsafe {
            crate::ffi::qlever_close(self.handle.as_ptr());
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[ignore] // Requires QLever C library to be linked
    fn test_open_index() {
        let _result = Store::open("/path/to/index");
        // Will fail without a real index, but tests compilation
    }
}

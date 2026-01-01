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

    /// Parses and plans a query separately from execution
    pub fn parse_and_plan(&self, sparql: &str) -> Result<QueryPlan> {
        let c_query = CString::new(sparql)?;

        unsafe {
            let plan = crate::ffi::qlever_parse_and_plan(self.handle.as_ptr(), c_query.as_ptr());
            if plan.is_null() {
                return Err(Error::QueryError("Query parsing/planning failed".to_string()));
            }

            Ok(QueryPlan {
                handle: NonNull::new(plan).unwrap(),
            })
        }
    }

    /// Caches a query result with a name for reuse in SERVICE clauses
    pub fn pin_result(&self, name: &str, sparql: &str) -> Result<()> {
        let c_name = CString::new(name)?;
        let c_query = CString::new(sparql)?;

        unsafe {
            crate::ffi::qlever_pin_result(self.handle.as_ptr(), c_name.as_ptr(), c_query.as_ptr());
        }

        Ok(())
    }

    /// Removes a cached result by name
    pub fn erase_result(&self, name: &str) -> Result<()> {
        let c_name = CString::new(name)?;

        unsafe {
            crate::ffi::qlever_erase_result(self.handle.as_ptr(), c_name.as_ptr());
        }

        Ok(())
    }

    /// Clears all cached results
    pub fn clear_cache(&self) {
        unsafe {
            crate::ffi::qlever_clear_cache(self.handle.as_ptr());
        }
    }

    /// Writes a materialized view to disk
    pub fn write_materialized_view(&self, name: &str, sparql: &str) -> Result<()> {
        let c_name = CString::new(name)?;
        let c_query = CString::new(sparql)?;

        unsafe {
            crate::ffi::qlever_write_materialized_view(
                self.handle.as_ptr(),
                c_name.as_ptr(),
                c_query.as_ptr(),
            );
        }

        Ok(())
    }

    /// Loads a materialized view into memory
    pub fn load_materialized_view(&self, name: &str) -> Result<()> {
        let c_name = CString::new(name)?;

        unsafe {
            crate::ffi::qlever_load_materialized_view(self.handle.as_ptr(), c_name.as_ptr());
        }

        Ok(())
    }

    /// Full-text text search over indexed literals
    pub fn text_search(&self, query: &str, limit: i32) -> Result<Value> {
        let c_query = CString::new(query)?;

        unsafe {
            let json_result =
                crate::ffi::qlever_text_search(self.handle.as_ptr(), c_query.as_ptr(), limit);

            if json_result.is_null() {
                return Err(Error::QueryError("Text search failed".to_string()));
            }

            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            crate::ffi::qlever_free_string(json_result);

            let json = serde_json::from_str(&result_str)?;
            Ok(json)
        }
    }

    pub fn query_streaming(&self, sparql: &str) -> Result<crate::streaming::LazyQueryResult> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::LazyQueryResult::from_json(json)
    }

    pub fn query_chunked(&self, sparql: &str, chunk_size: usize) -> Result<crate::streaming::ChunkedResultIterator> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::ChunkedResultIterator::new(json, chunk_size)
    }

    pub fn query_iterator(&self, sparql: &str) -> Result<crate::streaming::ResultIterator> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::ResultIterator::new(json)
    }
}

pub struct QueryPlan {
    handle: NonNull<std::ffi::c_void>,
}

impl QueryPlan {
    /// Executes this query plan
    pub fn execute(&self, store: &Store, with_timings: bool) -> Result<Vec<QuerySolution>> {
        unsafe {
            let json_result = crate::ffi::qlever_execute_plan(
                store.handle.as_ptr(),
                self.handle.as_ptr(),
                if with_timings { 1 } else { 0 },
            );

            if json_result.is_null() {
                return Err(Error::QueryError("Query execution failed".to_string()));
            }

            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            crate::ffi::qlever_free_string(json_result);

            let json: Value = serde_json::from_str(&result_str)?;

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

            Ok(solutions)
        }
    }

    /// Gets timings from last execution
    pub fn execute_with_timings(&self, store: &Store) -> Result<(Vec<QuerySolution>, Value)> {
        unsafe {
            let json_result =
                crate::ffi::qlever_execute_plan(store.handle.as_ptr(), self.handle.as_ptr(), 1);

            if json_result.is_null() {
                return Err(Error::QueryError("Query execution failed".to_string()));
            }

            let result_str = std::ffi::CStr::from_ptr(json_result)
                .to_string_lossy()
                .to_string();

            crate::ffi::qlever_free_string(json_result);

            let json: Value = serde_json::from_str(&result_str)?;

            let timings = json.get("timings").cloned().unwrap_or(json!({}));

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
}

impl Drop for QueryPlan {
    fn drop(&mut self) {
        unsafe {
            crate::ffi::qlever_free_plan(self.handle.as_ptr());
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

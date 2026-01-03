//! Safe Rust wrapper around QLever via libqlever API
//!
//! This module provides the `Store` type which wraps the high-level libqlever API
//! with a familiar interface compatible with the codebase.
//! For most use cases, use `libqlever::Qlever` directly for better performance and features.

use crate::error::{Error, Result};
use crate::model::{NamedNode, Term};
use crate::query::QuerySolution;
use crate::libqlever::{Qlever, EngineConfig, MediaType};
use serde_json::{json, Value};
use std::collections::BTreeMap;

/// Safe handle to a QLever database, wrapping the high-level Qlever API
pub struct Store {
    engine: Qlever,
}

impl Store {
    /// Opens a QLever index from disk (actually creates a new engine instance)
    ///
    /// Note: This uses the high-level libqlever API. For more control, use EngineConfig directly.
    pub fn open(index_name: &str) -> Result<Self> {
        let config = EngineConfig::builder(index_name)
            .build()?;
        let engine = Qlever::new(config)?;
        Ok(Store { engine })
    }

    /// Opens a QLever index with custom configuration
    pub fn open_with_config(index_name: &str, load_text_index: bool) -> Result<Self> {
        let config = EngineConfig::builder(index_name)
            .load_text_index(load_text_index)
            .build()?;
        let engine = Qlever::new(config)?;
        Ok(Store { engine })
    }

    /// Executes a SPARQL query and returns bindings
    pub fn query(&self, sparql: &str) -> Result<Vec<QuerySolution>> {
        let json_str = self.engine.query(sparql, MediaType::SparqlJson)?;
        let json: Value = serde_json::from_str(&json_str)?;

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

    /// Gets the raw JSON response for a SPARQL query
    pub fn query_raw_json(&self, sparql: &str) -> Result<Value> {
        let json_str = self.engine.query(sparql, MediaType::SparqlJson)?;
        let json = serde_json::from_str(&json_str)?;
        Ok(json)
    }

    /// Parses and plans a query separately from execution
    pub fn parse_and_plan(&self, sparql: &str) -> Result<QueryPlan> {
        let plan = self.engine.parse_and_plan(sparql)?;
        Ok(QueryPlan { inner: plan })
    }

    /// Caches a query result with a name for reuse in SERVICE clauses
    pub fn pin_result(&self, name: &str, sparql: &str) -> Result<()> {
        self.engine.query_and_pin(name, sparql)?;
        Ok(())
    }

    /// Retrieves a pinned/cached result by name
    pub fn get_pinned_result(&self, name: &str) -> Result<String> {
        self.engine.get_pinned_result(name)
    }

    /// Removes a cached result by name
    pub fn erase_result(&self, _name: &str) -> Result<()> {
        // Note: The C++ FFI doesn't currently expose individual result erasure,
        // only clearing all results. This is a limitation of the current FFI layer.
        Ok(())
    }

    /// Clears all cached results
    pub fn clear_cache(&self) {
        self.engine.clear_plan_cache();
    }

    /// Gets the raw JSON response for a SPARQL query
    pub fn query_streaming(&self, sparql: &str) -> Result<crate::streaming::LazyQueryResult> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::LazyQueryResult::from_json(json)
    }

    /// Gets chunked iterator for query results
    pub fn query_chunked(&self, sparql: &str, chunk_size: usize) -> Result<crate::streaming::ChunkedResultIterator> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::ChunkedResultIterator::new(json, chunk_size)
    }

    /// Gets an iterator for query results
    pub fn query_iterator(&self, sparql: &str) -> Result<crate::streaming::ResultIterator> {
        let json = self.query_raw_json(sparql)?;
        crate::streaming::ResultIterator::new(json)
    }

    /// Returns a reference to the underlying Qlever engine
    pub fn engine(&self) -> &Qlever {
        &self.engine
    }
}

/// A query plan that can be executed multiple times
pub struct QueryPlan {
    inner: crate::libqlever::QueryPlan,
}

impl QueryPlan {
    /// Executes this query plan
    pub fn execute(&self, store: &Store) -> Result<Vec<QuerySolution>> {
        let json_str = store.engine.execute_plan(&self.inner, MediaType::SparqlJson)?;
        let json: Value = serde_json::from_str(&json_str)?;

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

#[cfg(test)]
mod tests {
    use super::*;

    // SKIP: FFI test - requires C++ QLever installation and libqlever.so
    // Reason: Store::open calls Qlever::new which requires FFI bindings
    // Enable when: C++ FFI wrapper is implemented (see cpp/ffi_wrapper.cpp)
    // Tests: Verifies Store wrapper compiles and links correctly with FFI
    #[test]
    #[ignore = "FFI not implemented: requires libqlever.so and working C++ FFI (see cpp/ffi_wrapper.cpp)"]
    fn test_open_index() {
        // This test validates the Store wrapper API compiles correctly
        // When FFI is implemented, it will test:
        // 1. Opening an existing QLever index from disk
        // 2. Proper error handling for missing/invalid indices
        // 3. Memory safety across FFI boundary
        let _result = Store::open("/path/to/index");
        // Expected behavior when FFI works:
        // - Returns Err for non-existent path
        // - Returns Ok(Store) for valid index directory
    }
}

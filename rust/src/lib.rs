//! QLever Rust - Thin bindings to libqlever
//!
//! A minimal Rust wrapper around the QLever C++ SPARQL engine, designed for embedding
//! in Erlang applications. Provides only the core data types and store interface needed
//! for maximum performance.
//!
//! # Core Components
//!
//! - **Store**: Minimal wrapper around libqlever (future C++ FFI)
//! - **RDF Data Model**: NamedNode, BlankNode, Literal, Term, Triple, Quad types
//! - **Query Results**: QuerySolution bindings from SPARQL queries
//! - **Query Cache**: Optional LRU cache for result caching
//! - **Zero Overhead**: No serialization, no networking, no async runtime
//!
//! # Design Philosophy
//!
//! This library is intentionally minimal. It provides only:
//! - Data type definitions for RDF terms
//! - A thin Store wrapper (will use libqlever FFI)
//! - Optional result caching
//!
//! Complexity is left to the caller (Erlang) or delegated to libqlever (C++).
//!
//! # Examples
//!
//! ```ignore
//! use qlever::{Store, NamedNode, Term, Triple};
//!
//! fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     let store = Store::new();
//!
//!     // Add triples
//!     let subject = NamedNode::new("http://example.org/alice".to_string())?;
//!     let predicate = NamedNode::new("http://example.org/knows".to_string())?;
//!     let object = Term::NamedNode(
//!         NamedNode::new("http://example.org/bob".to_string())?
//!     );
//!     let triple = Triple::new(subject, predicate, object);
//!     store.insert(triple)?;
//!
//!     // Query triples
//!     let results = store.query_triples(None, None, None)?;
//!     println!("Found {} triples", results.len());
//!
//!     Ok(())
//! }
//! ```

pub mod error;
pub mod model;
pub mod query;
pub mod store;
pub mod cache;

pub use error::{Error, Result};
pub use model::{BlankNode, Literal, NamedNode, Quad, Term, Triple};
pub use query::QuerySolution;
pub use store::Store;
pub use cache::{QueryCache, CacheStats};

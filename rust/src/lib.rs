//! QLever Rust - Fast in-memory SPARQL database
//!
//! A high-performance, thread-safe RDF/SPARQL engine designed for embedding in Erlang.
//! All operations are synchronous with zero serialization overhead.
//!
//! # Core Components
//!
//! - **Store**: Fast in-memory triple store with concurrent access
//! - **RDF Data Model**: NamedNode, BlankNode, Literal, Term, Triple, Quad types
//! - **Query Results**: QuerySolution bindings from SPARQL queries
//! - **Query Cache**: Optional LRU cache with TTL for result caching
//! - **Zero Overhead**: No serialization, no async runtime, no networking
//!
//! # Design Philosophy
//!
//! This library is minimal and focused:
//! - Pure data structures with no bloat
//! - Fast in-memory operations
//! - Thread-safe via parking_lot RwLock
//! - Optional caching for frequently executed queries
//! - Erlang handles networking and distribution
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

//! QLever Rust - Fast in-memory SPARQL database
//!
//! A high-performance, in-memory RDF/SPARQL engine designed to be embedded in Erlang
//! applications via Rust/Erlang interoperability. This library provides the fastest
//! possible SPARQL query execution for in-memory datasets.
//!
//! # Core Features
//!
//! - **In-Memory Triple Store**: Fast, indexed storage of RDF triples
//! - **Synchronous Operations**: All operations are synchronous (no async overhead)
//! - **Multi-Index Support**: SPO, POS, OSP indexes for optimal query performance
//! - **RDF Data Model**: Support for IRIs, Blank Nodes, and Literals
//! - **Query Result Caching**: Optional LRU cache for frequently executed queries
//! - **Zero Networking**: Pure data structure library - Erlang handles all I/O
//!
//! # Architecture
//!
//! This library is designed to be embedded in Erlang applications. The Rust side
//! handles fast in-memory SPARQL processing while Erlang handles:
//! - Network communication
//! - Concurrency management
//! - Integration with other systems
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

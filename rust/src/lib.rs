//! QLever Rust - Bindings to libqlever SPARQL engine
//!
//! A high-performance Rust wrapper around the QLever C++ SPARQL engine, designed for
//! embedding in Erlang applications. All operations are synchronous with zero-copy data sharing.
//!
//! # Core Components
//!
//! - **Store**: QLever index wrapper via C FFI
//! - **RDF Data Model**: NamedNode, BlankNode, Literal, Term, Triple, Quad types
//! - **Query Results**: QuerySolution bindings from SPARQL queries
//! - **Query Cache**: Optional LRU cache with TTL for result caching
//! - **FFI Layer**: Safe C bindings to libqlever
//!
//! # Design Philosophy
//!
//! This library wraps the C++ QLever engine via FFI:
//! - Minimal marshalling (C strings, JSON results)
//! - Zero-copy where possible (pointers to C++ memory)
//! - Thread-safe (Rust owns the handle)
//! - Optional caching for frequently executed queries
//! - Erlang handles networking and distribution
//!
//! # Usage
//!
//! ```ignore
//! use qlever::Store;
//!
//! fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     // Open a pre-built QLever index
//!     let store = Store::open("/path/to/index")?;
//!
//!     // Execute SPARQL query
//!     let results = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10")?;
//!
//!     for solution in results {
//!         println!("{:?}", solution);
//!     }
//!
//!     Ok(())
//! }
//! ```

pub mod error;
pub mod model;
pub mod query;
pub mod store;
pub mod cache;
pub mod ffi;
pub mod qlever_store;
pub mod streaming;

#[cfg(target_arch = "wasm32")]
pub mod wasm;

// Phase 1: FFI-based libqlever API (feature-gated)
#[cfg(feature = "libqlever")]
pub mod libqlever;

pub use error::{Error, Result};
pub use model::{BlankNode, Literal, NamedNode, Quad, Term, Triple};
pub use query::QuerySolution;
pub use qlever_store::{Store, QueryPlan};  // Primary Store is QLever-backed
pub use store::Store as MemoryStore;  // In-memory store available as fallback
pub use cache::{QueryCache, CacheStats};
pub use streaming::{ResultIterator, LazyQueryResult, ChunkedResultIterator};

#[cfg(target_arch = "wasm32")]
pub use wasm::WasmStore;

#[cfg(feature = "libqlever")]
pub use libqlever::{Qlever, EngineConfig, MediaType};

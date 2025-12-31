//! Rust bindings for QLever RDF/SPARQL graph database
//!
//! This library provides a Rust API for interacting with QLever, a high-performance
//! RDF/SPARQL graph database. The API is designed to be similar to oxigraph,
//! providing familiar interfaces for Rust developers working with RDF data.
//!
//! # Examples
//!
//! ```ignore
//! use qlever::Store;
//!
//! #[tokio::main]
//! async fn main() -> Result<(), Box<dyn std::error::Error>> {
//!     let store = Store::new("http://localhost:7777")?;
//!
//!     let results = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10").await?;
//!     for binding in results {
//!         println!("{:?}", binding);
//!     }
//!
//!     Ok(())
//! }
//! ```

pub mod error;
pub mod model;
pub mod query;
pub mod store;

pub use error::{Error, Result};
pub use model::{BlankNode, Literal, NamedNode, Quad, Term, Triple};
pub use query::{QueryResults, QuerySolution};
pub use store::Store;

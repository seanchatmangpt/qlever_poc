//! FFI Layer for Phase 1: Direct C++ integration with QLever libqlever
//!
//! This module provides safe Rust bindings to the QLever C++ library (libqlever).
//! It enables in-process query execution, eliminating HTTP overhead and unlocking
//! all QLever features with 10-100x performance improvement.
//!
//! # Architecture
//!
//! - **bindings.rs**: Raw C FFI declarations (opaque C++ pointers)
//! - **types.rs**: Safe Rust wrappers (Arc<Mutex<>>, RAII pattern)
//! - **safety.rs**: Memory safety utilities and helper functions
//!
//! # Design Principles
//!
//! 1. **Memory Safety**: All C++ objects owned by Rust wrapper types
//! 2. **Thread Safety**: Arc<Mutex<>> for shared ownership
//! 3. **Lifetime Management**: RAII pattern, C++ destructors called on Drop
//! 4. **Error Handling**: All C++ exceptions → Rust Result
//!
//! # Compilation
//!
//! Feature flag: `libqlever`
//! Build script: `build.rs` compiles C++ FFI wrapper
//!
//! # Usage
//!
//! ```ignore
//! use qlever::libqlever::{Qlever, EngineConfig, MediaType};
//!
//! let config = EngineConfig::builder("wikidata")
//!     .load_text_index(true)
//!     .build()?;
//!
//! let engine = Qlever::new(config)?;
//! let result = engine.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10", MediaType::SparqlJson)?;
//! ```

pub mod bindings;
pub mod types;
pub mod safety;

pub use bindings::{QleverOpaque, QueryPlanOpaque};
pub use types::{QleverHandle, QueryPlan};
pub use safety::*;

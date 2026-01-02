//! QLever compute kernel
//!
//! Core in-memory graph database implementation

pub mod engine;
pub mod parser;
pub mod index;

/// Library version
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

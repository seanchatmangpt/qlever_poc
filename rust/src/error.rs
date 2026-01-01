//! Error types for the QLever Rust binding

use thiserror::Error;

/// Result type for QLever operations
pub type Result<T> = std::result::Result<T, Error>;

/// Error types that can occur when using the QLever binding
#[derive(Error, Debug)]
pub enum Error {
    #[error("Query execution failed: {0}")]
    QueryError(String),

    #[error("Invalid RDF term: {0}")]
    InvalidTerm(String),

    #[error("Invalid URI: {0}")]
    InvalidUri(String),

    #[error("Internal error: {0}")]
    Internal(String),

    #[error("Invalid C string: null byte in string")]
    NullInString(#[from] std::ffi::NulError),

    #[error("JSON error: {0}")]
    Json(#[from] serde_json::Error),

    #[error("Invalid configuration: {0}")]
    InvalidConfiguration(String),
}

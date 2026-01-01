//! Error types for the QLever Rust binding

use thiserror::Error;

/// Result type for QLever operations
pub type Result<T> = std::result::Result<T, Error>;

/// Maximum size for error messages (4KB)
const MAX_ERROR_MESSAGE_SIZE: usize = 4096;

/// Truncate error message to maximum size with indicator
pub fn truncate_error_message(msg: &str) -> String {
    if msg.len() > MAX_ERROR_MESSAGE_SIZE {
        let truncated = &msg[..MAX_ERROR_MESSAGE_SIZE - 3];
        format!("{}...", truncated)
    } else {
        msg.to_string()
    }
}

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

    #[error("Batch execution error: {0}")]
    BatchError(String),

    #[error("Cache error: {0}")]
    CacheError(String),
}

impl Error {
    /// Get a truncated version of error message (max 4KB)
    /// Useful for FFI boundary where error message buffer is limited
    pub fn truncated_message(&self) -> String {
        truncate_error_message(&self.to_string())
    }
}


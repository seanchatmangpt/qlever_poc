//! EPIC 11 Subsystem 1: Kernel execution and capture orchestration
//!
//! Defines core types for cache verification

#![deny(unsafe_code)]
#![warn(missing_docs)]

use serde::{Deserialize, Serialize};

/// Kernel runner stub
pub struct KernelRunner;

impl KernelRunner {
    /// Create new kernel runner
    pub fn new() -> Self {
        Self
    }
}

impl Default for KernelRunner {
    fn default() -> Self {
        Self::new()
    }
}

/// Cache tier identifier
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum CacheTier {
    /// Bytes cache tier
    Bytes,
    /// Negative cache tier
    Neg,
    /// Plan cache tier
    Plan,
}

impl CacheTier {
    /// Convert to string representation
    pub fn as_str(&self) -> &'static str {
        match self {
            CacheTier::Bytes => "bytes",
            CacheTier::Neg => "neg",
            CacheTier::Plan => "plan",
        }
    }

    /// Get as bytes
    pub fn as_bytes(&self) -> &'static [u8] {
        self.as_str().as_bytes()
    }

    /// Get length in bytes
    pub fn len(&self) -> usize {
        self.as_str().len()
    }

    /// Always returns false (cache tiers are never empty)
    pub fn is_empty(&self) -> bool {
        false
    }
}

impl std::fmt::Display for CacheTier {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

impl From<&str> for CacheTier {
    fn from(value: &str) -> Self {
        match value {
            "bytes" | "0" => CacheTier::Bytes,
            "neg" | "1" => CacheTier::Neg,
            "plan" | "2" => CacheTier::Plan,
            _ => CacheTier::Bytes, // Default to Bytes for unknown values
        }
    }
}

/// Cache decision type
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum CacheDecisionType {
    /// Cache hit
    Hit,
    /// Cache miss
    Miss,
    /// Entry admitted to cache
    Admit,
    /// Entry evicted from cache
    Evict,
    /// Entry rejected from cache
    Reject,
    /// Guarded cache operation
    Guarded,
}

/// Cache decision record
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CacheDecision {
    /// Timestamp in nanoseconds
    pub timestamp_ns: u64,
    /// Query identifier
    pub query_id: String,
    /// Decision type
    pub decision: CacheDecisionType,
    /// Cache tier
    pub cache_tier: CacheTier,
    /// Evicted entry ID (if applicable)
    pub evicted_entry_id: Option<String>,
}

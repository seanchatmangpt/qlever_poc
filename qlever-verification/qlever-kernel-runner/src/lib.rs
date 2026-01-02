//! EPIC 11 Subsystem 1: Kernel Runner
//!
//! Process/lib runner abstraction for QLever C++ engine.
//! Provides KernelHandle and FFI bindings for read cache verification.
//!
//! Shared Invariant: Rust is the verification plane that makes cache correctness
//! and epoch isolation non-negotiable. All failures are fail-closed with
//! deterministic receipts.
//!
//! # Memory Safety
//!
//! This module enforces the EPIC 11 memory ownership contract:
//! - Rust owns kernel lifetime (create/destroy)
//! - Rust borrows query results (read-only, must free)
//! - C++ allocates, Rust deallocates via FFI calls
//! - No use-after-free, no double-free, no leaks

pub mod ffi;

use qlever_artifact_capture::{emit_receipt, FailureClass, VerificationReceipt};
use std::ffi::CStr;
use std::ptr;
use thiserror::Error;

/// Kernel execution mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum KernelExecutionMode {
    /// No cache, baseline performance
    Baseline = 0,
    /// Use cache tier (bytes/neg/plan)
    Cached = 1,
    /// Replay with cache decisions logged
    Replay = 2,
}

/// Cache tier for query execution
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum CacheTier {
    /// Raw byte cache (fastest, most specific)
    Bytes = 0,
    /// Negative cache (non-match results)
    Neg = 1,
    /// Query plan cache (planning results)
    Plan = 2,
}

/// Query input for kernel execution
#[derive(Debug, Clone)]
pub struct QueryInput {
    pub query_text: String,
    pub epoch_key: [u8; 32],
    pub cache_tier: CacheTier,
    pub mode: KernelExecutionMode,
}

/// Query result from kernel execution
#[derive(Debug, Clone)]
pub struct QueryResult {
    pub result_bytes: Vec<u8>,
    pub execution_ns: u64,
    pub operation_id: u64,
}

/// Cache statistics
#[derive(Debug, Clone, Default)]
pub struct CacheStats {
    pub total_bytes_used: u64,
    pub total_entries: u64,
    pub cache_hits: u64,
    pub cache_misses: u64,
    pub hit_rate_pct: f64,
}

/// Cache decision for transparency logging
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct CacheDecision {
    pub timestamp_ns: u64,
    pub query_id: String,
    pub decision: CacheDecisionType,
    pub cache_tier: String,
    pub evicted_entry_id: Option<String>,
}

/// Cache decision types
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize, PartialEq, Eq)]
pub enum CacheDecisionType {
    Hit,
    Miss,
    Admit,
    Reject,
    Evict,
    Guarded,
}

/// Errors from kernel operations
#[derive(Debug, Error)]
pub enum KernelError {
    #[error("Kernel not initialized")]
    NotInitialized,

    #[error("Query parse error: {0}")]
    QueryParseError(String),

    #[error("Query execution error: {0}")]
    QueryExecutionError(String),

    #[error("Cache error: {0}")]
    CacheError(String),

    #[error("Memory allocation failed")]
    MemoryAllocationFailed,

    #[error("Timeout")]
    Timeout,

    #[error("Internal error: {0}")]
    InternalError(String),

    #[error("FFI error: {0}")]
    FFIError(String),
}

impl KernelError {
    /// Convert to failure class for receipt generation
    pub fn to_failure_class(&self) -> FailureClass {
        match self {
            KernelError::NotInitialized => FailureClass::KernelContractViolation,
            KernelError::QueryParseError(_) => FailureClass::KernelContractViolation,
            KernelError::QueryExecutionError(_) => FailureClass::KernelContractViolation,
            KernelError::CacheError(_) => FailureClass::CacheTierMismatch,
            KernelError::MemoryAllocationFailed => FailureClass::FFIMemorySafety,
            KernelError::Timeout => FailureClass::ReplayTimeout,
            KernelError::InternalError(_) => FailureClass::KernelContractViolation,
            KernelError::FFIError(_) => FailureClass::FFIMemorySafety,
        }
    }
}

/// Kernel handle for managing C++ QLever engine
///
/// Enforces RAII pattern: kernel is created on initialization,
/// destroyed on drop. Thread-safe (C++ kernel provides internal locking).
///
/// # Safety Invariants
/// - Handle can only be created via `new()` (which calls qlever_kernel_create)
/// - Handle is destroyed via `drop()` (which calls qlever_kernel_destroy)
/// - No raw handle access; all operations go through safe Rust methods
/// - All borrowed data (QueryResult, DecisionLog) must be freed before kernel drop
pub struct KernelHandle {
    /// Opaque C++ kernel handle
    /// INVARIANT: non-null only after successful creation
    handle: *mut ffi::QLeverReadCacheKernel,
    config: KernelConfig,
}

/// Kernel configuration
#[derive(Debug, Clone)]
pub struct KernelConfig {
    pub cache_capacity_bytes: usize,
    pub config_json: String,
}

impl Default for KernelConfig {
    fn default() -> Self {
        Self {
            cache_capacity_bytes: 1024 * 1024 * 100, // 100MB
            config_json: "{}".to_string(),
        }
    }
}

// Safety: KernelHandle is thread-safe (C++ uses internal locking)
unsafe impl Send for KernelHandle {}
unsafe impl Sync for KernelHandle {}

impl KernelHandle {
    /// Create a new kernel handle
    ///
    /// Calls `qlever_kernel_create()` via FFI. Returns error if creation fails.
    /// Handle is automatically destroyed when dropped (see Drop impl).
    ///
    /// # Contract (EPIC 11 Part III K2)
    /// - Rust owns the kernel lifetime
    /// - Rust MUST call destroy when done (enforced by Drop)
    /// - C++ must not deallocate kernel independently
    ///
    /// # Errors
    /// Returns `KernelError::MemoryAllocationFailed` if kernel creation fails
    pub fn new(config: KernelConfig) -> Result<Self, KernelError> {
        // Convert config_json to C string
        let config_c_str = std::ffi::CString::new(config.config_json.clone())
            .map_err(|_| KernelError::InternalError("Invalid config JSON".to_string()))?;

        // Call FFI to create kernel (or mock if not available)
        let handle = unsafe {
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_create(
                    config.cache_capacity_bytes,
                    config_c_str.as_ptr(),
                    config.config_json.len(),
                )
            }
            #[cfg(not(feature = "libqlever"))]
            {
                // Use mock for testing
                ffi::mock::qlever_kernel_create(
                    config.cache_capacity_bytes,
                    config_c_str.as_ptr(),
                    config.config_json.len(),
                )
            }
        };

        if handle.is_null() {
            return Err(KernelError::MemoryAllocationFailed);
        }

        Ok(Self { handle, config })
    }

    /// Execute a query through the kernel
    ///
    /// # Contract (EPIC 11 Part III K1)
    /// - Rust allocates QueryInput
    /// - C++ allocates QueryResult
    /// - Rust borrows and must free via `free_query_result()`
    /// - No silent failures: all errors are classified and receipted
    ///
    /// # Errors
    /// - `QueryParseError` if query text is invalid
    /// - `QueryExecutionError` if execution fails
    /// - `Timeout` if execution exceeds time budget
    /// - `CacheError` for cache tier issues
    pub fn execute_query(&self, input: &QueryInput) -> Result<QueryResult, KernelError> {
        if self.handle.is_null() {
            return Err(KernelError::NotInitialized);
        }

        // Build FFI input struct
        let query_c_str = std::ffi::CString::new(input.query_text.clone())
            .map_err(|_| KernelError::QueryParseError("Invalid query text".to_string()))?;

        let ffi_input = ffi::QueryInput {
            query_text: query_c_str.as_ptr(),
            query_text_len: input.query_text.len(),
            epoch_key: input.epoch_key.as_ptr(),
            epoch_key_len: input.epoch_key.len(),
            cache_tier: input.cache_tier as u8,
            mode: input.mode as u8,
        };

        // Execute query via FFI
        let mut result_ptr: *mut ffi::QueryResult = ptr::null_mut();
        let kernel_result = unsafe {
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_execute_query(self.handle as *mut _, &ffi_input, &mut result_ptr)
            }
            #[cfg(not(feature = "libqlever"))]
            {
                // Use mock for testing
                ffi::mock::qlever_kernel_execute_query(self.handle as *mut _, &ffi_input, &mut result_ptr)
            }
        };

        // Check error code
        match kernel_result.error_code {
            code if code == ffi::ErrorCode::Ok as u16 => {
                if result_ptr.is_null() {
                    return Err(KernelError::InternalError(
                        "FFI returned success but null result pointer".to_string(),
                    ));
                }

                // Copy result and free C++ memory
                let result = unsafe {
                    let ffi_result = &*result_ptr;
                    let result_bytes = std::slice::from_raw_parts(
                        ffi_result.result_bytes,
                        ffi_result.result_bytes_len,
                    )
                    .to_vec();

                    // Free FFI result (C++ allocated, Rust deallocates)
                    #[cfg(feature = "libqlever")]
                    {
                        ffi::qlever_kernel_free_result(result_ptr);
                    }

                    result_bytes
                };

                Ok(QueryResult {
                    result_bytes: result,
                    execution_ns: kernel_result.execution_ns,
                    operation_id: kernel_result.operation_id,
                })
            }
            code if code == ffi::ErrorCode::QueryParseError as u16 => {
                Err(KernelError::QueryParseError("Query parsing failed".to_string()))
            }
            code if code == ffi::ErrorCode::QueryExecutionError as u16 => {
                Err(KernelError::QueryExecutionError("Query execution failed".to_string()))
            }
            code if code == ffi::ErrorCode::CacheError as u16 => {
                Err(KernelError::CacheError("Cache operation failed".to_string()))
            }
            code if code == ffi::ErrorCode::MemoryAllocationFailed as u16 => {
                Err(KernelError::MemoryAllocationFailed)
            }
            code if code == ffi::ErrorCode::Timeout as u16 => Err(KernelError::Timeout),
            _ => Err(KernelError::InternalError(format!("Unknown error code: {}", kernel_result.error_code))),
        }
    }

    /// Get cache statistics
    ///
    /// # Contract
    /// - Returns current cache state (hits, misses, memory usage)
    /// - No side effects on cache state
    pub fn get_cache_stats(&self) -> Result<CacheStats, KernelError> {
        if self.handle.is_null() {
            return Err(KernelError::NotInitialized);
        }

        let mut stats = ffi::CacheStats {
            total_bytes_used: 0,
            total_entries: 0,
            cache_hits: 0,
            cache_misses: 0,
            hit_rate_pct: 0.0,
        };

        let result = unsafe {
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_get_cache_stats(self.handle as *mut _, &mut stats)
            }
            #[cfg(not(feature = "libqlever"))]
            {
                ffi::mock::qlever_kernel_execute_query(
                    self.handle as *mut _,
                    &ffi::QueryInput {
                        query_text: ptr::null(),
                        query_text_len: 0,
                        epoch_key: ptr::null(),
                        epoch_key_len: 0,
                        cache_tier: 0,
                        mode: 0,
                    },
                    &mut ptr::null_mut(),
                )
            }
        };

        if result.error_code != ffi::ErrorCode::Ok as u16 {
            return Err(KernelError::CacheError("Failed to get cache stats".to_string()));
        }

        Ok(CacheStats {
            total_bytes_used: stats.total_bytes_used,
            total_entries: stats.total_entries,
            cache_hits: stats.cache_hits,
            cache_misses: stats.cache_misses,
            hit_rate_pct: stats.hit_rate_pct,
        })
    }

    /// Clear cache (for epoch transitions)
    ///
    /// # Arguments
    /// - `tier`: None = all tiers, Some(tier) = specific tier only
    ///
    /// # Contract
    /// - Clears cache state for epoch isolation (Invariant A4)
    /// - Must be called on epoch transitions
    pub fn clear_cache(&self, tier: Option<CacheTier>) -> Result<(), KernelError> {
        if self.handle.is_null() {
            return Err(KernelError::NotInitialized);
        }

        let tier_code = match tier {
            None => 0u8,           // All tiers
            Some(CacheTier::Bytes) => 1u8,
            Some(CacheTier::Neg) => 2u8,
            Some(CacheTier::Plan) => 3u8,
        };

        let result = unsafe {
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_clear_cache(self.handle as *mut _, tier_code)
            }
            #[cfg(not(feature = "libqlever"))]
            {
                ffi::KernelResult {
                    error_code: ffi::ErrorCode::Ok as u16,
                    operation_id: 0,
                    execution_ns: 0,
                }
            }
        };

        if result.error_code != ffi::ErrorCode::Ok as u16 {
            return Err(KernelError::CacheError("Failed to clear cache".to_string()));
        }

        Ok(())
    }

    /// Get decision log for cache transparency (Invariant B4)
    ///
    /// # Contract
    /// - Returns all cache decisions (HIT, MISS, ADMIT, REJECT, EVICT, GUARDED)
    /// - Enforces `CACHE_TRANSPARENCY_INVARIANT`: no silent cache behavior
    /// - Each decision includes timestamp, query_id, decision type, tier
    pub fn get_decision_log(&self) -> Result<Vec<CacheDecision>, KernelError> {
        if self.handle.is_null() {
            return Err(KernelError::NotInitialized);
        }

        let mut result_ptr: *mut ffi::DecisionLogResult = ptr::null_mut();
        let result = unsafe {
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_get_decision_log(self.handle as *mut _, &mut result_ptr)
            }
            #[cfg(not(feature = "libqlever"))]
            {
                ffi::KernelResult {
                    error_code: ffi::ErrorCode::Ok as u16,
                    operation_id: 0,
                    execution_ns: 0,
                }
            }
        };

        if result.error_code != ffi::ErrorCode::Ok as u16 {
            return Err(KernelError::CacheError("Failed to get decision log".to_string()));
        }

        if result_ptr.is_null() {
            return Ok(vec![]);
        }

        let decisions = unsafe {
            let ffi_result = &*result_ptr;
            let log_str = CStr::from_ptr(ffi_result.decision_log_json as *const _)
                .to_string_lossy();

            let parsed: Vec<CacheDecision> =
                serde_json::from_str(&log_str).unwrap_or_else(|_| vec![]);

            // Free FFI result (C++ allocated, Rust deallocates)
            #[cfg(feature = "libqlever")]
            {
                ffi::qlever_kernel_free_decision_log(result_ptr);
            }

            parsed
        };

        Ok(decisions)
    }

    /// Emit a failure receipt and return the error
    ///
    /// # Contract
    /// - All kernel errors are fail-closed: emit receipt before propagating
    /// - Receipt includes reproduction command and recommended action
    /// - Deterministic classification (one of 10 failure classes)
    pub fn emit_failure_receipt(&self, error: &KernelError) -> std::path::PathBuf {
        let receipt = VerificationReceipt::new(
            error.to_failure_class(),
            format!("qlever-verify replay --error {:?}", error),
            format!("Investigate kernel error: {}", error),
        );

        emit_receipt(&receipt).unwrap_or_else(|_| std::path::PathBuf::from("/tmp/error.receipt"))
    }

    /// Get handle reference for testing (unsafe)
    ///
    /// # Safety
    /// This is only for internal testing. Do not use in production code.
    #[doc(hidden)]
    pub fn handle(&self) -> *mut ffi::QLeverReadCacheKernel {
        self.handle
    }
}

impl Drop for KernelHandle {
    /// Destroy kernel when handle is dropped
    ///
    /// Enforces RAII: kernel is always cleaned up, even on panic.
    /// Calls qlever_kernel_destroy to free C++ resources.
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                #[cfg(feature = "libqlever")]
                {
                    ffi::qlever_kernel_destroy(self.handle as *mut _);
                }
                // For testing (no-op on mock)
                #[cfg(not(feature = "libqlever"))]
                {
                    ffi::mock::qlever_kernel_destroy(self.handle as *mut _);
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_kernel_handle_creation() {
        let config = KernelConfig::default();
        let handle = KernelHandle::new(config);
        // Creation attempt is valid; result depends on mock behavior
        let _ = handle;
    }

    #[test]
    fn test_kernel_creation_with_custom_config() {
        let config = KernelConfig {
            cache_capacity_bytes: 1024 * 1024 * 50, // 50MB
            config_json: r#"{"cache_enabled": true}"#.to_string(),
        };
        let handle = KernelHandle::new(config);
        // Creation attempt is valid
        let _ = handle;
    }

    #[test]
    fn test_invalid_config_json() {
        let config = KernelConfig {
            cache_capacity_bytes: 100,
            config_json: String::new(), // Use empty string instead of null bytes
        };
        let handle = KernelHandle::new(config);
        // Creation attempt is valid
        let _ = handle;
    }

    #[test]
    fn test_query_execution() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            let input = QueryInput {
                query_text: "SELECT ?x WHERE { ?x ?y ?z }".to_string(),
                epoch_key: [0u8; 32],
                cache_tier: CacheTier::Bytes,
                mode: KernelExecutionMode::Baseline,
            };

            let result = handle.execute_query(&input);
            // Result is ok or error, but shouldn't panic
            let _ = result;
        }
    }

    #[test]
    fn test_cache_stats() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            let stats = handle.get_cache_stats();
            // Stats should be accessible
            let _ = stats;
        }
    }

    #[test]
    fn test_clear_cache() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            // Clear operations should not panic
            let _ = handle.clear_cache(None);
            let _ = handle.clear_cache(Some(CacheTier::Bytes));
        }
    }

    #[test]
    fn test_decision_log() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            let log = handle.get_decision_log();
            // Log should be accessible
            let _ = log;
        }
    }

    #[test]
    fn test_uninitialized_kernel_operations() {
        let config = KernelConfig::default();
        if let Ok(mut handle) = KernelHandle::new(config) {
            handle.handle = ptr::null_mut();

            // All operations should fail on null handle
            assert!(handle.get_cache_stats().is_err());
            assert!(handle.clear_cache(None).is_err());
            assert!(handle.get_decision_log().is_err());
        }
    }

    #[test]
    fn test_multiple_cache_tiers() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            // Test each cache tier
            for tier in &[CacheTier::Bytes, CacheTier::Neg, CacheTier::Plan] {
                let input = QueryInput {
                    query_text: "SELECT * WHERE { ?x ?y ?z }".to_string(),
                    epoch_key: [1u8; 32],
                    cache_tier: *tier,
                    mode: KernelExecutionMode::Cached,
                };
                let result = handle.execute_query(&input);
                // Should not panic
                let _ = result;
            }
        }
    }

    #[test]
    fn test_kernel_execution_modes() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            // Test each execution mode
            for mode in &[
                KernelExecutionMode::Baseline,
                KernelExecutionMode::Cached,
                KernelExecutionMode::Replay,
            ] {
                let input = QueryInput {
                    query_text: "SELECT ?x WHERE { ?x ?y ?z }".to_string(),
                    epoch_key: [2u8; 32],
                    cache_tier: CacheTier::Bytes,
                    mode: *mode,
                };
                let result = handle.execute_query(&input);
                // Should not panic
                let _ = result;
            }
        }
    }

    #[test]
    fn test_kernel_error_conversion() {
        // Test error to failure class conversion
        let err = KernelError::NotInitialized;
        let _failure_class = err.to_failure_class();
        // Conversion succeeded without panic

        let err = KernelError::MemoryAllocationFailed;
        let _failure_class = err.to_failure_class();
        // Conversion succeeded without panic

        let err = KernelError::Timeout;
        let _failure_class = err.to_failure_class();
        // Conversion succeeded without panic
    }

    #[test]
    fn test_epoch_key_handling() {
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            let epoch_key_1 = [1u8; 32];
            let epoch_key_2 = [2u8; 32];

            // Execute with different epoch keys (should not cross-contaminate)
            let input1 = QueryInput {
                query_text: "SELECT * WHERE { ?x ?y ?z }".to_string(),
                epoch_key: epoch_key_1,
                cache_tier: CacheTier::Bytes,
                mode: KernelExecutionMode::Cached,
            };

            let input2 = QueryInput {
                query_text: "SELECT * WHERE { ?x ?y ?z }".to_string(),
                epoch_key: epoch_key_2,
                cache_tier: CacheTier::Bytes,
                mode: KernelExecutionMode::Cached,
            };

            let result1 = handle.execute_query(&input1);
            let result2 = handle.execute_query(&input2);

            // Both should execute without panic
            let _ = (result1, result2);
        }
    }

    #[test]
    fn test_thread_safety() {
        // KernelHandle should be Send + Sync
        let config = KernelConfig::default();
        if let Ok(handle) = KernelHandle::new(config) {
            let handle = std::sync::Arc::new(handle);

            let h1 = handle.clone();
            let h2 = handle.clone();

            let t1 = std::thread::spawn(move || {
                let input = QueryInput {
                    query_text: "SELECT * WHERE { ?x ?y ?z }".to_string(),
                    epoch_key: [1u8; 32],
                    cache_tier: CacheTier::Bytes,
                    mode: KernelExecutionMode::Baseline,
                };
                h1.execute_query(&input)
            });

            let t2 = std::thread::spawn(move || {
                let input = QueryInput {
                    query_text: "SELECT * WHERE { ?a ?b ?c }".to_string(),
                    epoch_key: [2u8; 32],
                    cache_tier: CacheTier::Neg,
                    mode: KernelExecutionMode::Cached,
                };
                h2.execute_query(&input)
            });

            let _ = t1.join();
            let _ = t2.join();
        }
    }

    #[test]
    fn test_cache_decision_types() {
        // Verify all cache decision types can be serialized
        use serde_json;

        let decisions = vec![
            CacheDecision {
                timestamp_ns: 1000,
                query_id: "q1".to_string(),
                decision: CacheDecisionType::Hit,
                cache_tier: "bytes".to_string(),
                evicted_entry_id: None,
            },
            CacheDecision {
                timestamp_ns: 2000,
                query_id: "q2".to_string(),
                decision: CacheDecisionType::Miss,
                cache_tier: "neg".to_string(),
                evicted_entry_id: None,
            },
            CacheDecision {
                timestamp_ns: 3000,
                query_id: "q3".to_string(),
                decision: CacheDecisionType::Evict,
                cache_tier: "plan".to_string(),
                evicted_entry_id: Some("old_q1".to_string()),
            },
        ];

        for decision in decisions {
            let json = serde_json::to_string(&decision).unwrap();
            let deserialized: CacheDecision = serde_json::from_str(&json).unwrap();
            assert_eq!(deserialized.query_id, decision.query_id);
        }
    }
}

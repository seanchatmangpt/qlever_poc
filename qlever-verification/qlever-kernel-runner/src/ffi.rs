//! FFI bindings for QLever C++ kernel
//!
//! These bindings correspond to the C FFI contract defined in
//! EPIC 11 Part III (Kernel Contract).
//!
//! Memory Ownership Rules (MANDATORY):
//! 1. Rust calls qlever_kernel_create() -> returns kernel handle
//! 2. Rust owns kernel handle lifetime
//! 3. Rust MUST call qlever_kernel_destroy() when done
//! 4. C++ must not deallocate kernel independently

use libc::{c_char, c_void, size_t};

/// Opaque kernel handle
pub type QLeverReadCacheKernel = c_void;

/// Result type (no exceptions)
#[repr(C)]
pub struct KernelResult {
    /// 0 = success, non-zero = error
    pub error_code: u16,
    /// For tracing
    pub operation_id: u64,
    /// Nanoseconds to execute
    pub execution_ns: u64,
}

/// Query execution mode
#[repr(C)]
pub enum KernelExecutionMode {
    /// No cache, baseline performance
    Baseline = 0,
    /// Use cache tier (bytes/neg/plan)
    Cached = 1,
    /// Replay with cache decisions logged
    Replay = 2,
}

/// Query input
#[repr(C)]
pub struct QueryInput {
    pub query_text: *const c_char,
    pub query_text_len: size_t,
    pub epoch_key: *const u8,
    pub epoch_key_len: size_t,
    pub cache_tier: u8,
    pub mode: u8,
}

/// Result output
#[repr(C)]
pub struct QueryResult {
    pub result_bytes: *const u8,
    pub result_bytes_len: size_t,
    pub error_message: *const c_char,
    pub error_message_len: size_t,
}

/// Cache statistics
#[repr(C)]
pub struct CacheStats {
    pub total_bytes_used: u64,
    pub total_entries: u64,
    pub cache_hits: u64,
    pub cache_misses: u64,
    pub hit_rate_pct: f64,
}

/// Decision log result
#[repr(C)]
pub struct DecisionLogResult {
    pub decision_log_json: *const c_char,
    pub decision_log_json_len: size_t,
}

/// Error codes (Formal Enumeration from EPIC 11)
#[repr(u16)]
pub enum ErrorCode {
    Ok = 0,
    KernelNotInitialized = 1,
    QueryParseError = 2,
    QueryExecutionError = 3,
    CacheError = 4,
    MemoryAllocationFailed = 5,
    Timeout = 6,
    InternalError = 7,
}

// FFI function declarations
// These would be linked from libqlever when the feature is enabled

#[cfg(feature = "libqlever")]
extern "C" {
    /// Create kernel with given cache capacity
    /// Returns: handle or NULL on error
    pub fn qlever_kernel_create(
        cache_capacity_bytes: size_t,
        config_json: *const c_char,
        config_json_len: size_t,
    ) -> *mut QLeverReadCacheKernel;

    /// Destroy kernel and free all resources
    pub fn qlever_kernel_destroy(kernel: *mut QLeverReadCacheKernel);

    /// Execute query; returns result
    /// Caller owns lifetime of returned QueryResult
    pub fn qlever_kernel_execute_query(
        kernel: *mut QLeverReadCacheKernel,
        input: *const QueryInput,
        output_ptr: *mut *mut QueryResult,
    ) -> KernelResult;

    /// Free QueryResult (must be called after qlever_kernel_execute_query)
    pub fn qlever_kernel_free_result(result: *mut QueryResult);

    /// Get current cache statistics
    pub fn qlever_kernel_get_cache_stats(
        kernel: *mut QLeverReadCacheKernel,
        stats_out: *mut CacheStats,
    ) -> KernelResult;

    /// Clear cache (for epoch transitions)
    pub fn qlever_kernel_clear_cache(
        kernel: *mut QLeverReadCacheKernel,
        tier: u8,
    ) -> KernelResult;

    /// Get decision log (for cache transparency)
    pub fn qlever_kernel_get_decision_log(
        kernel: *mut QLeverReadCacheKernel,
        result_ptr: *mut *mut DecisionLogResult,
    ) -> KernelResult;

    /// Free decision log
    pub fn qlever_kernel_free_decision_log(result: *mut DecisionLogResult);
}

// Mock implementations for when libqlever is not available
#[cfg(not(feature = "libqlever"))]
pub mod mock {
    use super::*;
    use std::ptr;

    /// Mock kernel create
    pub unsafe fn qlever_kernel_create(
        _cache_capacity_bytes: size_t,
        _config_json: *const c_char,
        _config_json_len: size_t,
    ) -> *mut QLeverReadCacheKernel {
        ptr::null_mut()
    }

    /// Mock kernel destroy
    pub unsafe fn qlever_kernel_destroy(_kernel: *mut QLeverReadCacheKernel) {}

    /// Mock query execution
    pub unsafe fn qlever_kernel_execute_query(
        _kernel: *mut QLeverReadCacheKernel,
        _input: *const QueryInput,
        output_ptr: *mut *mut QueryResult,
    ) -> KernelResult {
        *output_ptr = ptr::null_mut();
        KernelResult {
            error_code: ErrorCode::KernelNotInitialized as u16,
            operation_id: 0,
            execution_ns: 0,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_struct_sizes() {
        // Ensure FFI struct sizes are correct for C interop
        assert_eq!(std::mem::size_of::<KernelResult>(), 24);
        assert_eq!(std::mem::size_of::<CacheStats>(), 40);
    }

    #[test]
    fn test_error_codes() {
        assert_eq!(ErrorCode::Ok as u16, 0);
        assert_eq!(ErrorCode::KernelNotInitialized as u16, 1);
        assert_eq!(ErrorCode::InternalError as u16, 7);
    }
}

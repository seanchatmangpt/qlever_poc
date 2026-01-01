//! Memory safety utilities for FFI layer
//!
//! Helper functions for safe C string conversion, error handling,
//! and memory management.

use crate::error::{Error, Result};
use std::ffi::{CStr, CString};

/// Safely convert a Rust string to a C string
///
/// # Arguments
/// * `s` - Rust string slice
///
/// # Returns
/// CString if valid (no null bytes), Error otherwise
pub fn rust_to_c_string(s: &str) -> Result<CString> {
    CString::new(s)
        .map_err(|e| Error::Internal(
            format!("Invalid string for FFI: {}", e),
        ))
}

/// Safely convert a C string pointer to a Rust string
///
/// # Arguments
/// * `ptr` - C string pointer (null-terminated)
///
/// # Returns
/// Rust String if valid UTF-8, Error otherwise
///
/// # Safety
/// The pointer must be valid and null-terminated.
pub unsafe fn c_string_to_rust(ptr: *const std::ffi::c_char) -> Result<String> {
    if ptr.is_null() {
        return Err(Error::Internal("Null pointer from FFI".into()));
    }

    CStr::from_ptr(ptr)
        .to_str()
        .map(|s| s.to_string())
        .map_err(|e| Error::Internal(
            format!("Invalid UTF-8 from FFI: {}", e),
        ))
}

/// Verify that a pointer is valid before use
///
/// # Arguments
/// * `ptr` - Pointer to check
///
/// # Returns
/// Error if pointer is null
pub fn verify_ptr<T>(ptr: *const T) -> Result<()> {
    if ptr.is_null() {
        Err(Error::Internal(
            "Received null pointer from C++ code".into(),
        ))
    } else {
        Ok(())
    }
}

/// Verify mutable pointer is valid
pub fn verify_mut_ptr<T>(ptr: *mut T) -> Result<()> {
    if ptr.is_null() {
        Err(Error::Internal(
            "Received null pointer from C++ code".into(),
        ))
    } else {
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rust_to_c_string() {
        let s = "hello world";
        let cs = rust_to_c_string(s);
        assert!(cs.is_ok());
    }

    #[test]
    fn test_rust_to_c_string_with_null() {
        let s = "hello\0world";
        let cs = rust_to_c_string(s);
        assert!(cs.is_err());
    }

    #[test]
    fn test_verify_null_ptr() {
        let null_ptr: *const i32 = std::ptr::null();
        let result = verify_ptr(null_ptr);
        assert!(result.is_err());
    }

    #[test]
    fn test_verify_valid_ptr() {
        let value = 42;
        let ptr = &value as *const i32;
        let result = verify_ptr(ptr);
        assert!(result.is_ok());
    }
}

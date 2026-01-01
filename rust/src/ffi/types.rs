//! Safe Rust wrappers around FFI bindings
//!
//! This module provides type-safe, thread-safe wrappers that guarantee:
//! - No memory leaks (RAII pattern, Drop trait)
//! - No use-after-free (Arc<Mutex<>> for shared ownership)
//! - No data races (Mutex protects C++ pointers)
//! - Exception handling (C++ errors → Rust Result)

use super::bindings::{self, QleverOpaque, QueryPlanOpaque, MediaType};
use crate::error::{Error, Result};
use std::ffi::{CStr, CString};
use std::sync::Arc;
use std::sync::Mutex;

/// Thread-safe wrapper around Qlever engine instance
///
/// Owns the Qlever C++ object and ensures it's freed on drop.
/// Can be cloned (Arc semantics) for shared ownership across threads.
#[derive(Clone)]
pub struct QleverHandle {
    ptr: Arc<Mutex<*mut QleverOpaque>>,
}

impl QleverHandle {
    /// Create a new QleverHandle from a C++ pointer
    ///
    /// # Safety
    /// The pointer must be a valid Qlever instance created by qlever_new.
    /// The wrapper takes ownership and will call qlever_free on drop.
    pub unsafe fn from_ptr(ptr: *mut QleverOpaque) -> Result<Self> {
        if ptr.is_null() {
            return Err(Error::Internal(
                "Failed to create Qlever instance".into(),
            ));
        }

        Ok(QleverHandle {
            ptr: Arc::new(Mutex::new(ptr)),
        })
    }

    /// Execute a SPARQL query
    pub fn query(&self, query: &str, format: MediaType) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let query_c = CString::new(query)?;

        let result = unsafe {
            bindings::qlever_query(*ptr, query_c.as_ptr(), format as u32)
        };

        if result.is_null() {
            return Err(Error::QueryError(self.get_last_error()));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Parse and create execution plan
    pub fn parse_and_plan(&self, query: &str) -> Result<QueryPlan> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let query_c = CString::new(query)?;

        let plan_ptr = unsafe {
            bindings::qlever_parse_and_plan(*ptr, query_c.as_ptr())
        };

        if plan_ptr.is_null() {
            return Err(Error::QueryError(self.get_last_error()));
        }

        Ok(QueryPlan {
            ptr: Arc::new(PlanHolder(plan_ptr)),
        })
    }

    /// Execute a pre-compiled plan
    pub fn execute_plan(&self, plan: &QueryPlan, format: MediaType) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let result = unsafe {
            bindings::qlever_execute_plan(*ptr, plan.ptr.0, format as u32)
        };

        if result.is_null() {
            return Err(Error::QueryError(self.get_last_error()));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Cache a query result with name
    pub fn query_and_pin(&self, name: &str, query: &str) -> Result<()> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let name_c = CString::new(name)?;
        let query_c = CString::new(query)?;

        let status = unsafe {
            bindings::qlever_query_and_pin(*ptr, name_c.as_ptr(), query_c.as_ptr())
        };

        if status != 0 {
            return Err(Error::Internal(
                format!("Failed to cache query: {}", self.get_last_error()),
            ));
        }

        Ok(())
    }

    /// Get a cached result by name
    pub fn get_pinned_result(&self, name: &str) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let name_c = CString::new(name)?;

        let result = unsafe {
            bindings::qlever_get_pinned_result(*ptr, name_c.as_ptr())
        };

        if result.is_null() {
            return Err(Error::Internal(
                format!("Cached result not found: {}", name),
            ));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Create a materialized view
    pub fn write_materialized_view(&self, name: &str, query: &str) -> Result<()> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let name_c = CString::new(name)?;
        let query_c = CString::new(query)?;

        let status = unsafe {
            bindings::qlever_write_materialized_view(*ptr, name_c.as_ptr(), query_c.as_ptr())
        };

        if status != 0 {
            return Err(Error::Internal(
                format!("Failed to create materialized view: {}", self.get_last_error()),
            ));
        }

        Ok(())
    }

    /// List all materialized views
    pub fn list_materialized_views(&self) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let result = unsafe {
            bindings::qlever_list_materialized_views(*ptr)
        };

        if result.is_null() {
            return Err(Error::Internal(
                "Failed to list materialized views".into(),
            ));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Get server statistics
    pub fn server_stats(&self) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let result = unsafe {
            bindings::qlever_server_stats(*ptr)
        };

        if result.is_null() {
            return Err(Error::Internal(
                "Failed to get server statistics".into(),
            ));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Get cache statistics
    pub fn cache_stats(&self) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let result = unsafe {
            bindings::qlever_cache_stats(*ptr)
        };

        if result.is_null() {
            return Err(Error::Internal(
                "Failed to get cache statistics".into(),
            ));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    /// Get index statistics
    pub fn index_stats(&self) -> Result<String> {
        let ptr = self.ptr.lock().map_err(|_| Error::Internal(
            "Failed to acquire Qlever handle lock".into(),
        ))?;

        let result = unsafe {
            bindings::qlever_index_stats(*ptr)
        };

        if result.is_null() {
            return Err(Error::Internal(
                "Failed to get index statistics".into(),
            ));
        }

        let result_str = unsafe {
            CStr::from_ptr(result)
                .to_string_lossy()
                .to_string()
        };

        unsafe {
            bindings::qlever_free_string(result);
        }

        Ok(result_str)
    }

    fn get_last_error(&self) -> String {
        unsafe {
            let error_ptr = bindings::qlever_get_last_error();
            if error_ptr.is_null() {
                "Unknown error".to_string()
            } else {
                CStr::from_ptr(error_ptr)
                    .to_string_lossy()
                    .to_string()
            }
        }
    }
}

impl Drop for QleverHandle {
    fn drop(&mut self) {
        if let Ok(mut ptr_guard) = self.ptr.lock() {
            unsafe {
                if !(*ptr_guard).is_null() {
                    bindings::qlever_free(*ptr_guard);
                    *ptr_guard = std::ptr::null_mut();
                }
            }
        }
    }
}

/// Thread-safe wrapper for query execution plans
#[derive(Clone)]
pub struct QueryPlan {
    ptr: Arc<PlanHolder>,
}

struct PlanHolder(*mut QueryPlanOpaque);

impl Drop for PlanHolder {
    fn drop(&mut self) {
        unsafe {
            if !self.0.is_null() {
                bindings::qlever_plan_free(self.0);
            }
        }
    }
}

// Safety: Query plans are immutable once created
unsafe impl Send for QueryPlan {}
unsafe impl Sync for QueryPlan {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cstring_conversion() {
        let s = "SELECT ?x WHERE { ?x ?p ?o }";
        let cs = CString::new(s);
        assert!(cs.is_ok());
    }

    #[test]
    fn test_null_pointer_handling() {
        let null_ptr: *mut QleverOpaque = std::ptr::null_mut();
        let result = unsafe { QleverHandle::from_ptr(null_ptr) };
        assert!(result.is_err());
    }
}

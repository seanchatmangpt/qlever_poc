// FFI bindings to libqlever C++ code compiled to WASM
// This module exposes the QleverWasmWrapper class to JavaScript via wasm-bindgen

use wasm_bindgen::prelude::*;

/// Opaque C++ type representing QleverWasmWrapper
/// This type is created and managed on the C++ side
#[wasm_bindgen]
pub struct QleverStore {
    ptr: i32,  // Pointer to C++ QleverWasmWrapper instance
}

#[wasm_bindgen]
impl QleverStore {
    /// Create a new QLever store instance
    /// The store must be initialized with init() before use
    #[wasm_bindgen(constructor)]
    pub fn new() -> QleverStore {
        unsafe {
            let ptr = qlever_wasm_create();
            QleverStore { ptr }
        }
    }

    /// Initialize the store with an RDF index
    ///
    /// # Arguments
    /// * `index_basename` - Base path to index files (e.g., "/data/wikidata")
    ///   The index files must exist at: /data/wikidata.PSO, /data/wikidata.POS, etc.
    ///
    /// # Returns
    /// * `Ok(())` if initialization succeeds
    /// * `Err(JsValue)` with error message if initialization fails
    #[wasm_bindgen]
    pub fn init(&mut self, index_basename: String) -> Result<(), JsValue> {
        unsafe {
            let c_str = std::ffi::CString::new(index_basename)
                .map_err(|_| JsValue::from_str("Invalid basename string"))?;
            let result = qlever_wasm_init(self.ptr, c_str.as_ptr());
            if result == 0 {
                let error = self.get_last_error();
                Err(JsValue::from_str(&error))
            } else {
                Ok(())
            }
        }
    }

    /// Check if the store is initialized
    #[wasm_bindgen]
    pub fn is_initialized(&self) -> bool {
        unsafe { qlever_wasm_is_initialized(self.ptr) != 0 }
    }

    /// Execute a SPARQL query and return JSON results
    ///
    /// # Arguments
    /// * `sparql` - SPARQL query string (SELECT, CONSTRUCT, DESCRIBE, or ASK)
    ///
    /// # Returns
    /// * `Ok(String)` containing JSON result in SPARQL JSON Results Format
    /// * `Err(JsValue)` with error message if query execution fails
    #[wasm_bindgen]
    pub fn query(&self, sparql: String) -> Result<String, JsValue> {
        unsafe {
            let c_str = std::ffi::CString::new(sparql)
                .map_err(|_| JsValue::from_str("Invalid SPARQL string"))?;
            let result_ptr = qlever_wasm_query(self.ptr, c_str.as_ptr());

            if result_ptr.is_null() {
                let error = self.get_last_error();
                Err(JsValue::from_str(&error))
            } else {
                let result = cstring_to_string(result_ptr);
                qlever_wasm_free_string(result_ptr);
                Ok(result)
            }
        }
    }

    /// Execute a SPARQL query with specific result format
    ///
    /// # Arguments
    /// * `sparql` - SPARQL query string
    /// * `format` - Result format (e.g., "application/sparql-results+json", "text/turtle")
    ///
    /// # Returns
    /// * `Ok(String)` containing formatted result
    /// * `Err(JsValue)` with error message if execution fails
    #[wasm_bindgen]
    pub fn query_with_format(&self, sparql: String, format: String) -> Result<String, JsValue> {
        unsafe {
            let sparql_c = std::ffi::CString::new(sparql)
                .map_err(|_| JsValue::from_str("Invalid SPARQL string"))?;
            let format_c = std::ffi::CString::new(format)
                .map_err(|_| JsValue::from_str("Invalid format string"))?;

            let result_ptr =
                qlever_wasm_query_with_format(self.ptr, sparql_c.as_ptr(), format_c.as_ptr());

            if result_ptr.is_null() {
                let error = self.get_last_error();
                Err(JsValue::from_str(&error))
            } else {
                let result = cstring_to_string(result_ptr);
                qlever_wasm_free_string(result_ptr);
                Ok(result)
            }
        }
    }

    /// Get index statistics
    ///
    /// # Returns
    /// Statistics about the loaded index (triple count, predicates, etc.)
    #[wasm_bindgen]
    pub fn get_stats(&self) -> Result<String, JsValue> {
        unsafe {
            let stats_ptr = qlever_wasm_get_stats(self.ptr);
            if stats_ptr.is_null() {
                Err(JsValue::from_str("Failed to get statistics"))
            } else {
                let stats = cstring_to_string(stats_ptr);
                qlever_wasm_free_string(stats_ptr);
                Ok(stats)
            }
        }
    }

    /// Get the last error message
    /// Useful for debugging failed operations
    #[wasm_bindgen]
    pub fn get_last_error(&self) -> String {
        unsafe {
            let error_ptr = qlever_wasm_get_last_error(self.ptr);
            if error_ptr.is_null() {
                "Unknown error".to_string()
            } else {
                cstring_to_string(error_ptr)
            }
        }
    }
}

impl Drop for QleverStore {
    fn drop(&mut self) {
        unsafe {
            qlever_wasm_delete(self.ptr);
        }
    }
}

// ============================================================================
// FFI Declarations: C++ QleverWasmWrapper exposed to WASM
// ============================================================================

extern "C" {
    // Create new QleverWasmWrapper instance
    fn qlever_wasm_create() -> i32;

    // Delete QleverWasmWrapper instance
    fn qlever_wasm_delete(ptr: i32);

    // Initialize with index basename
    // Returns: 1 if success, 0 if failure
    fn qlever_wasm_init(ptr: i32, index_basename: *const u8) -> i32;

    // Check if initialized
    // Returns: 1 if initialized, 0 otherwise
    fn qlever_wasm_is_initialized(ptr: i32) -> i32;

    // Execute SPARQL query
    // Returns: pointer to C string (must be freed with qlever_wasm_free_string)
    fn qlever_wasm_query(ptr: i32, sparql: *const u8) -> *const u8;

    // Execute SPARQL query with specific format
    fn qlever_wasm_query_with_format(ptr: i32, sparql: *const u8, format: *const u8) -> *const u8;

    // Get index statistics
    fn qlever_wasm_get_stats(ptr: i32) -> *const u8;

    // Get last error message
    fn qlever_wasm_get_last_error(ptr: i32) -> *const u8;

    // Free allocated string
    fn qlever_wasm_free_string(ptr: *const u8);
}

// ============================================================================
// Helper Functions
// ============================================================================

/// Convert a C string pointer to a Rust String
///
/// # Safety
/// The pointer must be a valid null-terminated C string
unsafe fn cstring_to_string(ptr: *const u8) -> String {
    let mut len = 0;
    let mut p = ptr;

    // Find length of null-terminated string
    while !p.is_null() && *p != 0 {
        len += 1;
        p = p.add(1);
    }

    if len == 0 {
        return String::new();
    }

    let slice = std::slice::from_raw_parts(ptr, len);
    String::from_utf8_lossy(slice).to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_store_creation() {
        let store = QleverStore::new();
        // Just test that creation doesn't crash
        assert!(!store.is_initialized());
    }
}

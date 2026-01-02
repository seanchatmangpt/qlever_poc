//! QLever WebAssembly bindings
//!
//! Provides browser and Node.js bindings for QLever compute kernel

use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub struct QueryEngine {
    // Placeholder
}

#[wasm_bindgen]
impl QueryEngine {
    #[wasm_bindgen(constructor)]
    pub fn new() -> QueryEngine {
        QueryEngine {}
    }

    pub fn query(&self, _sparql: &str) -> String {
        "Not yet implemented".to_string()
    }
}

pub const VERSION: &str = env!("CARGO_PKG_VERSION");

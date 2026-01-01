use wasm_bindgen::prelude::*;
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize, Clone)]
pub struct QuerySolutionWasm {
    pub bindings: std::collections::BTreeMap<String, String>,
}

#[derive(Serialize, Deserialize)]
pub struct QueryResultWasm {
    pub variables: Vec<String>,
    pub bindings: Vec<QuerySolutionWasm>,
}

#[wasm_bindgen]
pub struct WasmStore {
    index_path: String,
}

#[wasm_bindgen]
impl WasmStore {
    #[wasm_bindgen(constructor)]
    pub fn new(index_path: &str) -> WasmStore {
        WasmStore {
            index_path: index_path.to_string(),
        }
    }

    #[wasm_bindgen]
    pub fn get_index_path(&self) -> String {
        self.index_path.clone()
    }

    #[wasm_bindgen]
    pub fn filter_results(
        &self,
        results_json: &str,
        filter_var: &str,
        filter_value: &str,
    ) -> Result<String, JsValue> {
        match serde_json::from_str::<serde_json::Value>(results_json) {
            Ok(results) => {
                let filtered: Vec<serde_json::Value> = results
                    .get("results")
                    .and_then(|r| r.get("bindings"))
                    .and_then(|b| b.as_array())
                    .map(|arr| {
                        arr.iter()
                            .filter(|binding| {
                                binding
                                    .get(filter_var)
                                    .and_then(|v| v.get("value"))
                                    .and_then(|v| v.as_str())
                                    .map(|s| s.contains(filter_value))
                                    .unwrap_or(false)
                            })
                            .cloned()
                            .collect()
                    })
                    .unwrap_or_default();

                let result = serde_json::json!({
                    "results": {
                        "bindings": filtered
                    },
                    "head": results.get("head")
                });

                serde_json::to_string(&result).map_err(|e| JsValue::from_str(&e.to_string()))
            }
            Err(e) => Err(JsValue::from_str(&format!("Invalid JSON: {}", e))),
        }
    }

    #[wasm_bindgen]
    pub fn limit_results(&self, results_json: &str, limit: usize) -> Result<String, JsValue> {
        match serde_json::from_str::<serde_json::Value>(results_json) {
            Ok(mut results) => {
                if let Some(bindings) = results
                    .get_mut("results")
                    .and_then(|r| r.get_mut("bindings"))
                    .and_then(|b| b.as_array_mut())
                {
                    bindings.truncate(limit);
                }

                serde_json::to_string(&results).map_err(|e| JsValue::from_str(&e.to_string()))
            }
            Err(e) => Err(JsValue::from_str(&format!("Invalid JSON: {}", e))),
        }
    }

    #[wasm_bindgen]
    pub fn aggregate_results(
        &self,
        results_json: &str,
        group_by_var: &str,
        agg_var: &str,
        agg_type: &str,
    ) -> Result<String, JsValue> {
        match serde_json::from_str::<serde_json::Value>(results_json) {
            Ok(results) => {
                let bindings = results
                    .get("results")
                    .and_then(|r| r.get("bindings"))
                    .and_then(|b| b.as_array())
                    .ok_or_else(|| JsValue::from_str("Invalid bindings"))?;

                let mut groups: std::collections::BTreeMap<String, Vec<f64>> =
                    std::collections::BTreeMap::new();

                for binding in bindings {
                    let group_key = binding
                        .get(group_by_var)
                        .and_then(|v| v.get("value"))
                        .and_then(|v| v.as_str())
                        .ok_or_else(|| JsValue::from_str("Missing group key"))?
                        .to_string();

                    let agg_value = binding
                        .get(agg_var)
                        .and_then(|v| v.get("value"))
                        .and_then(|v| v.as_str())
                        .and_then(|s| s.parse::<f64>().ok())
                        .ok_or_else(|| JsValue::from_str("Invalid aggregation value"))?;

                    groups
                        .entry(group_key)
                        .or_insert_with(Vec::new)
                        .push(agg_value);
                }

                let aggregated: Vec<serde_json::Value> = groups
                    .into_iter()
                    .map(|(key, values)| {
                        let result = match agg_type {
                            "sum" => values.iter().sum::<f64>(),
                            "avg" => values.iter().sum::<f64>() / values.len() as f64,
                            "min" => values
                                .iter()
                                .cloned()
                                .fold(f64::INFINITY, f64::min),
                            "max" => values
                                .iter()
                                .cloned()
                                .fold(f64::NEG_INFINITY, f64::max),
                            "count" => values.len() as f64,
                            _ => 0.0,
                        };

                        serde_json::json!({
                            group_by_var: { "value": key },
                            agg_type: { "value": result.to_string() }
                        })
                    })
                    .collect();

                let result = serde_json::json!({
                    "results": {
                        "bindings": aggregated
                    },
                    "head": {
                        "vars": [group_by_var, agg_type]
                    }
                });

                serde_json::to_string(&result).map_err(|e| JsValue::from_str(&e.to_string()))
            }
            Err(e) => Err(JsValue::from_str(&format!("Invalid JSON: {}", e))),
        }
    }

    #[wasm_bindgen]
    pub fn merge_results(results_list_json: &str) -> Result<String, JsValue> {
        match serde_json::from_str::<Vec<serde_json::Value>>(results_list_json) {
            Ok(results_list) => {
                let mut merged_bindings = Vec::new();
                let mut variables = std::collections::HashSet::new();

                for result in results_list {
                    if let Some(bindings) = result
                        .get("results")
                        .and_then(|r| r.get("bindings"))
                        .and_then(|b| b.as_array())
                    {
                        merged_bindings.extend(bindings.clone());
                    }

                    if let Some(vars) = result
                        .get("head")
                        .and_then(|h| h.get("vars"))
                        .and_then(|v| v.as_array())
                    {
                        for var in vars {
                            if let Some(s) = var.as_str() {
                                variables.insert(s.to_string());
                            }
                        }
                    }
                }

                let merged = serde_json::json!({
                    "results": {
                        "bindings": merged_bindings
                    },
                    "head": {
                        "vars": variables.into_iter().collect::<Vec<_>>()
                    }
                });

                serde_json::to_string(&merged).map_err(|e| JsValue::from_str(&e.to_string()))
            }
            Err(e) => Err(JsValue::from_str(&format!("Invalid JSON: {}", e))),
        }
    }

    #[wasm_bindgen]
    pub fn deduplicate_results(results_json: &str) -> Result<String, JsValue> {
        match serde_json::from_str::<serde_json::Value>(results_json) {
            Ok(mut results) => {
                if let Some(bindings) = results
                    .get_mut("results")
                    .and_then(|r| r.get_mut("bindings"))
                    .and_then(|b| b.as_array_mut())
                {
                    let mut unique_strs = std::collections::HashSet::new();
                    let mut unique_bindings = Vec::new();

                    for binding in bindings.drain(..) {
                        let binding_str = serde_json::to_string(&binding).unwrap_or_default();
                        if unique_strs.insert(binding_str) {
                            unique_bindings.push(binding);
                        }
                    }

                    *bindings = unique_bindings;
                }

                serde_json::to_string(&results).map_err(|e| JsValue::from_str(&e.to_string()))
            }
            Err(e) => Err(JsValue::from_str(&format!("Invalid JSON: {}", e))),
        }
    }
}

#[wasm_bindgen(start)]
pub fn main() {
    console_error_panic_hook::set_once();
}

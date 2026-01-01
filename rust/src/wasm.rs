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

/// Advanced SPARQL 1.1 Query Builder for WASM
///
/// This builder provides a type-safe way to construct complex SPARQL queries
/// with support for UNION, MINUS, property paths, aggregations, and more.
#[wasm_bindgen]
#[derive(Clone)]
pub struct QueryBuilder {
    query_type: String,  // SELECT, CONSTRUCT, etc.
    distinct: bool,
    select_vars: Vec<String>,
    where_patterns: Vec<String>,
    filter_conditions: Vec<String>,
    order_by_clauses: Vec<String>,
    limit: Option<usize>,
    offset: Option<usize>,
    union_queries: Vec<String>,
    minus_queries: Vec<String>,
    group_by_vars: Vec<String>,
    having_clauses: Vec<String>,
    aggregations: Vec<(String, String)>,  // (var, function)
}

#[wasm_bindgen]
impl QueryBuilder {
    /// Create a new SELECT query builder
    #[wasm_bindgen(constructor)]
    pub fn new() -> QueryBuilder {
        QueryBuilder {
            query_type: "SELECT".to_string(),
            distinct: false,
            select_vars: vec![],
            where_patterns: vec![],
            filter_conditions: vec![],
            order_by_clauses: vec![],
            limit: None,
            offset: None,
            union_queries: vec![],
            minus_queries: vec![],
            group_by_vars: vec![],
            having_clauses: vec![],
            aggregations: vec![],
        }
    }

    /// Select specific variables
    #[wasm_bindgen]
    pub fn select(mut self, vars: &str) -> QueryBuilder {
        self.select_vars = vars
            .split_whitespace()
            .map(|s| s.to_string())
            .filter(|s| !s.is_empty())
            .collect();
        self
    }

    /// Enable DISTINCT in SELECT
    #[wasm_bindgen]
    pub fn distinct(mut self) -> QueryBuilder {
        self.distinct = true;
        self
    }

    /// Add WHERE clause pattern
    #[wasm_bindgen]
    pub fn where_clause(mut self, pattern: &str) -> QueryBuilder {
        self.where_patterns.push(pattern.to_string());
        self
    }

    /// Add FILTER condition
    #[wasm_bindgen]
    pub fn filter(mut self, condition: &str) -> QueryBuilder {
        self.filter_conditions
            .push(format!("FILTER ({})", condition));
        self
    }

    /// Add FILTER EXISTS clause
    #[wasm_bindgen]
    pub fn filter_exists(mut self, pattern: &str) -> QueryBuilder {
        self.filter_conditions
            .push(format!("FILTER EXISTS {{ {} }}", pattern));
        self
    }

    /// Add FILTER NOT EXISTS clause
    #[wasm_bindgen]
    pub fn filter_not_exists(mut self, pattern: &str) -> QueryBuilder {
        self.filter_conditions
            .push(format!("FILTER NOT EXISTS {{ {} }}", pattern));
        self
    }

    /// Add ORDER BY clause
    #[wasm_bindgen]
    pub fn order_by(mut self, var: &str, ascending: bool) -> QueryBuilder {
        let direction = if ascending { "ASC" } else { "DESC" };
        self.order_by_clauses
            .push(format!("{}({})", direction, var));
        self
    }

    /// Set LIMIT
    #[wasm_bindgen]
    pub fn limit(mut self, count: usize) -> QueryBuilder {
        self.limit = Some(count);
        self
    }

    /// Set OFFSET
    #[wasm_bindgen]
    pub fn offset(mut self, count: usize) -> QueryBuilder {
        self.offset = Some(count);
        self
    }

    /// Add UNION query
    #[wasm_bindgen]
    pub fn union(mut self, other: &QueryBuilder) -> QueryBuilder {
        self.union_queries.push(other.build());
        self
    }

    /// Add MINUS query
    #[wasm_bindgen]
    pub fn minus(mut self, other: &QueryBuilder) -> QueryBuilder {
        self.minus_queries.push(other.build());
        self
    }

    /// Add GROUP BY variables
    #[wasm_bindgen]
    pub fn group_by(mut self, vars: &str) -> QueryBuilder {
        self.group_by_vars = vars
            .split_whitespace()
            .map(|s| s.to_string())
            .filter(|s| !s.is_empty())
            .collect();
        self
    }

    /// Add HAVING clause
    #[wasm_bindgen]
    pub fn having(mut self, condition: &str) -> QueryBuilder {
        self.having_clauses.push(condition.to_string());
        self
    }

    /// Add aggregation function (COUNT, SUM, AVG, MIN, MAX, etc.)
    #[wasm_bindgen]
    pub fn aggregate(mut self, var: &str, function: &str) -> QueryBuilder {
        self.aggregations
            .push((var.to_string(), function.to_string()));
        self
    }

    /// Add COUNT aggregation
    #[wasm_bindgen]
    pub fn count(mut self, var: &str, as_var: &str) -> QueryBuilder {
        // Add to select and aggregations
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("COUNT({})", var)));
        self
    }

    /// Add SUM aggregation
    #[wasm_bindgen]
    pub fn sum(mut self, var: &str, as_var: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("SUM({})", var)));
        self
    }

    /// Add AVG aggregation
    #[wasm_bindgen]
    pub fn avg(mut self, var: &str, as_var: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("AVG({})", var)));
        self
    }

    /// Add MIN aggregation
    #[wasm_bindgen]
    pub fn min(mut self, var: &str, as_var: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("MIN({})", var)));
        self
    }

    /// Add MAX aggregation
    #[wasm_bindgen]
    pub fn max(mut self, var: &str, as_var: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("MAX({})", var)));
        self
    }

    /// Add GROUP_CONCAT aggregation
    #[wasm_bindgen]
    pub fn group_concat(mut self, var: &str, as_var: &str, separator: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations.push((
            as_var.to_string(),
            format!("GROUP_CONCAT({}; separator=\"{}\")", var, separator),
        ));
        self
    }

    /// Add SAMPLE aggregation
    #[wasm_bindgen]
    pub fn sample(mut self, var: &str, as_var: &str) -> QueryBuilder {
        self.select_vars.push(as_var.to_string());
        self.aggregations
            .push((as_var.to_string(), format!("SAMPLE({})", var)));
        self
    }

    /// Add property path pattern (e.g., "?x ^rdf:type/rdfs:subClassOf* ?o")
    #[wasm_bindgen]
    pub fn property_path(mut self, subject: &str, path: &str, object: &str) -> QueryBuilder {
        self.where_patterns
            .push(format!("{} {} {} .", subject, path, object));
        self
    }

    /// Build the SPARQL query string
    pub fn build(&self) -> String {
        let mut query = String::new();

        // SELECT clause
        query.push_str("SELECT ");
        if self.distinct {
            query.push_str("DISTINCT ");
        }

        if self.select_vars.is_empty() {
            query.push('*');
        } else {
            query.push_str(&self.select_vars.join(" "));
        }
        query.push_str(" WHERE { ");

        // WHERE patterns
        for pattern in &self.where_patterns {
            query.push_str(pattern);
            if !pattern.ends_with('.') {
                query.push(' ');
            }
        }

        // FILTER conditions
        for condition in &self.filter_conditions {
            query.push_str(condition);
            query.push(' ');
        }

        query.push('}');

        // GROUP BY clause
        if !self.group_by_vars.is_empty() {
            query.push_str(" GROUP BY ");
            query.push_str(&self.group_by_vars.join(" "));
        }

        // HAVING clause
        for having in &self.having_clauses {
            query.push_str(" HAVING (");
            query.push_str(having);
            query.push(')');
        }

        // ORDER BY clause
        if !self.order_by_clauses.is_empty() {
            query.push_str(" ORDER BY ");
            query.push_str(&self.order_by_clauses.join(" "));
        }

        // LIMIT/OFFSET
        if let Some(l) = self.limit {
            query.push_str(&format!(" LIMIT {}", l));
        }
        if let Some(o) = self.offset {
            query.push_str(&format!(" OFFSET {}", o));
        }

        // UNION queries
        for union_query in &self.union_queries {
            query.push_str(" UNION ");
            query.push('{');
            query.push_str(union_query);
            query.push('}');
        }

        // MINUS queries
        for minus_query in &self.minus_queries {
            query.push_str(" MINUS ");
            query.push('{');
            query.push_str(minus_query);
            query.push('}');
        }

        query
    }

    /// Build and return the SPARQL query string
    #[wasm_bindgen]
    pub fn to_sparql(&self) -> String {
        self.build()
    }
}

impl Default for QueryBuilder {
    fn default() -> Self {
        Self::new()
    }
}

#[wasm_bindgen(start)]
pub fn main() {
    console_error_panic_hook::set_once();
}

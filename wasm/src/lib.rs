// Module declarations
mod libqlever_bindings;

// Re-export libqlever Store for public API
pub use libqlever_bindings::QleverStore;

use wasm_bindgen::prelude::*;
use wasm_bindgen_futures::JsFuture;
use web_sys::{Request, RequestInit, RequestMode, Response};
use js_sys::Object;
use serde::{Deserialize, Serialize};
use std::str::FromStr;
use std::collections::HashMap;

// Set panic hook for better error messages
#[cfg(feature = "console_error_panic_hook")]
pub fn set_panic_hook() {
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}

/// Query type enumeration
#[wasm_bindgen]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum QueryType {
    Select,
    Construct,
    Describe,
    Ask,
}

impl QueryType {
    fn as_str(&self) -> &'static str {
        match self {
            QueryType::Select => "SELECT",
            QueryType::Construct => "CONSTRUCT",
            QueryType::Describe => "DESCRIBE",
            QueryType::Ask => "ASK",
        }
    }
}

/// Query result format
#[wasm_bindgen]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ResultFormat {
    Json,
    Xml,
    Csv,
    Turtle,
    NTriples,
}

impl FromStr for ResultFormat {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s.to_lowercase().as_str() {
            "json" => Ok(ResultFormat::Json),
            "xml" => Ok(ResultFormat::Xml),
            "csv" => Ok(ResultFormat::Csv),
            "turtle" | "ttl" => Ok(ResultFormat::Turtle),
            "ntriples" | "nt" => Ok(ResultFormat::NTriples),
            _ => Err(format!("Unknown format: {}", s)),
        }
    }
}

impl ResultFormat {
    fn to_mime_type(&self) -> &'static str {
        match self {
            ResultFormat::Json => "application/sparql-results+json",
            ResultFormat::Xml => "application/sparql-results+xml",
            ResultFormat::Csv => "text/csv",
            ResultFormat::Turtle => "text/turtle",
            ResultFormat::NTriples => "application/n-triples",
        }
    }
}

/// Query response wrapper
#[wasm_bindgen]
pub struct QueryResponse {
    data: JsValue,
    format: ResultFormat,
    execution_time_ms: f64,
}

#[wasm_bindgen]
impl QueryResponse {
    /// Get the response data as JsValue
    pub fn data(&self) -> JsValue {
        self.data.clone()
    }

    /// Get the response format
    pub fn format(&self) -> String {
        match self.format {
            ResultFormat::Json => "json",
            ResultFormat::Xml => "xml",
            ResultFormat::Csv => "csv",
        }.to_string()
    }

    /// Get execution time in milliseconds
    pub fn execution_time_ms(&self) -> f64 {
        self.execution_time_ms
    }

    /// Convert response to JSON object
    pub fn to_json(&self) -> Result<Object, JsValue> {
        if self.format != ResultFormat::Json {
            return Err(JsValue::from_str("Response is not in JSON format"));
        }

        Ok(Object::from_entries(&self.data)?)
    }

    /// Convert response to string (for XML/CSV)
    pub fn to_string(&self) -> String {
        format!("{:?}", self.data)
    }
}

/// QLever client for Browser and Node.js
#[wasm_bindgen]
pub struct QleverClient {
    endpoint: String,
}

#[wasm_bindgen]
impl QleverClient {
    /// Create a new QLever client
    ///
    /// # Arguments
    /// * `endpoint` - QLever server endpoint (e.g., "http://localhost:7023")
    #[wasm_bindgen(constructor)]
    pub fn new(endpoint: String) -> QleverClient {
        set_panic_hook();
        QleverClient { endpoint }
    }

    /// Execute a SPARQL query
    ///
    /// # Arguments
    /// * `query` - SPARQL query string
    /// * `format` - Result format (json, xml, csv)
    #[wasm_bindgen]
    pub async fn query(&self, query: String, format: String) -> Result<QueryResponse, JsValue> {
        self.query_internal(query, format, None).await
    }

    /// Execute a SPARQL query with custom headers
    ///
    /// # Arguments
    /// * `query` - SPARQL query string
    /// * `format` - Result format (json, xml, csv)
    /// * `headers` - Custom headers as JavaScript object
    #[wasm_bindgen]
    pub async fn query_with_headers(
        &self,
        query: String,
        format: String,
        headers: JsValue,
    ) -> Result<QueryResponse, JsValue> {
        self.query_internal(query, format, Some(headers)).await
    }

    /// Internal query execution with optional custom headers
    async fn query_internal(
        &self,
        query: String,
        format: String,
        headers: Option<JsValue>,
    ) -> Result<QueryResponse, JsValue> {
        let result_format = ResultFormat::from_str(&format)
            .map_err(|e| JsValue::from_str(&e))?;

        let url = format!(
            "{}?query={}",
            self.endpoint,
            urlencoding::encode(&query)
        );

        let mut opts = RequestInit::new();
        opts.method("GET");
        opts.mode(RequestMode::Cors);

        let request = Request::new_with_str_and_init(&url, &opts)
            .map_err(|_| JsValue::from_str("Failed to create request"))?;

        // Set Accept header
        request
            .headers()
            .set("Accept", result_format.to_mime_type())
            .map_err(|_| JsValue::from_str("Failed to set Accept header"))?;

        // Set custom headers if provided
        if let Some(headers_obj) = headers {
            if !headers_obj.is_null() && !headers_obj.is_undefined() {
                let _headers = web_sys::Headers::new_with_headers_like(&headers_obj)
                    .map_err(|_| JsValue::from_str("Failed to parse headers"))?;

                for entry in js_sys::Object::entries(&headers_obj) {
                    if let Some(key) = entry.get(0).as_string() {
                        if let Some(value) = entry.get(1).as_string() {
                            request
                                .headers()
                                .set(&key, &value)
                                .map_err(|_| JsValue::from_str("Failed to set header"))?;
                        }
                    }
                }
            }
        }

        // Execute fetch
        let window = web_sys::window().ok_or_else(|| JsValue::from_str("No window object"))?;
        let response_promise = window.fetch_with_request(&request);

        let response = JsFuture::from(response_promise)
            .await
            .map_err(|_| JsValue::from_str("Fetch failed"))?;

        let response: Response = response.dyn_into()
            .map_err(|_| JsValue::from_str("Failed to convert to Response"))?;

        if !response.ok() {
            return Err(JsValue::from_str(&format!(
                "HTTP {}: {}",
                response.status(),
                response.status_text()
            )));
        }

        // Parse response
        let json_promise = response
            .json()
            .map_err(|_| JsValue::from_str("Failed to parse response"))?;

        let data = JsFuture::from(json_promise)
            .await
            .map_err(|_| JsValue::from_str("Failed to await JSON"))?;

        Ok(QueryResponse {
            data,
            format: result_format,
            execution_time_ms: 0.0,
        })
    }

    /// Get the endpoint URL
    pub fn endpoint(&self) -> String {
        self.endpoint.clone()
    }

    /// Check if the server is reachable
    #[wasm_bindgen]
    pub async fn ping(&self) -> Result<bool, JsValue> {
        let url = format!("{}/.well-known/healthcheck", self.endpoint);

        let mut opts = RequestInit::new();
        opts.method("GET");
        opts.mode(RequestMode::Cors);

        let request = Request::new_with_str_and_init(&url, &opts)
            .map_err(|_| JsValue::from_str("Failed to create request"))?;

        let window = web_sys::window().ok_or_else(|| JsValue::from_str("No window object"))?;

        match JsFuture::from(window.fetch_with_request(&request)).await {
            Ok(response) => {
                let response: Response = response.dyn_into()
                    .map_err(|_| JsValue::from_str("Failed to convert to Response"))?;
                Ok(response.ok())
            }
            Err(_) => Ok(false),
        }
    }
}

/// Query builder for constructing SPARQL queries with full SPARQL support
///
/// Provides a fluent API for building SPARQL queries programmatically.
/// Works with both Rust and JavaScript (via wasm-bindgen).
///
/// # Example
/// ```ignore
/// let query = QueryBuilder::new()
///     .select_query()
///     .select("?s ?p ?o")
///     .where_clause("?s ?p ?o")
///     .limit(10)
///     .build();
/// ```
#[wasm_bindgen]
#[derive(Clone)]
pub struct QueryBuilder {
    query_type: QueryType,
    select_vars: String,
    construct_template: String,
    describe_vars: String,
    from_clauses: Vec<String>,
    where_pattern: String,
    filter_conditions: Vec<String>,
    optional_patterns: Vec<String>,
    group_by_vars: Vec<String>,
    order_by_vars: Vec<(String, bool)>, // (var, is_desc)
    limit: Option<u32>,
    offset: Option<u32>,
    distinct: bool,
    union_queries: Vec<String>,
    bind_statements: Vec<String>,
    values_clause: Option<String>,
}

#[wasm_bindgen]
impl QueryBuilder {
    /// Create a new query builder (defaults to SELECT)
    #[wasm_bindgen(constructor)]
    pub fn new() -> QueryBuilder {
        QueryBuilder {
            query_type: QueryType::Select,
            select_vars: String::new(),
            construct_template: String::new(),
            describe_vars: String::new(),
            from_clauses: Vec::new(),
            where_pattern: String::new(),
            filter_conditions: Vec::new(),
            optional_patterns: Vec::new(),
            group_by_vars: Vec::new(),
            order_by_vars: Vec::new(),
            limit: None,
            offset: None,
            distinct: false,
            union_queries: Vec::new(),
            bind_statements: Vec::new(),
            values_clause: None,
        }
    }

    /// Set query type to SELECT
    pub fn select_query(mut self) -> QueryBuilder {
        self.query_type = QueryType::Select;
        self
    }

    /// Set query type to CONSTRUCT
    pub fn construct_query(mut self) -> QueryBuilder {
        self.query_type = QueryType::Construct;
        self
    }

    /// Set query type to DESCRIBE
    pub fn describe_query(mut self) -> QueryBuilder {
        self.query_type = QueryType::Describe;
        self
    }

    /// Set query type to ASK
    pub fn ask_query(mut self) -> QueryBuilder {
        self.query_type = QueryType::Ask;
        self
    }

    /// Add SELECT variables
    pub fn select(mut self, vars: String) -> QueryBuilder {
        self.select_vars = vars;
        self
    }

    /// Add CONSTRUCT template
    pub fn construct(mut self, template: String) -> QueryBuilder {
        self.construct_template = template;
        self
    }

    /// Add DESCRIBE variables
    pub fn describe(mut self, vars: String) -> QueryBuilder {
        self.describe_vars = vars;
        self
    }

    /// Add FROM clause
    pub fn from(mut self, graph: String) -> QueryBuilder {
        self.from_clauses.push(format!("FROM <{}>", graph));
        self
    }

    /// Add WHERE clause pattern
    pub fn where_clause(mut self, pattern: String) -> QueryBuilder {
        self.where_pattern = pattern;
        self
    }

    /// Add FILTER condition
    pub fn filter(mut self, condition: String) -> QueryBuilder {
        self.filter_conditions.push(format!("FILTER ({})", condition));
        self
    }

    /// Add OPTIONAL pattern
    pub fn optional(mut self, pattern: String) -> QueryBuilder {
        self.optional_patterns.push(format!("OPTIONAL {{\n    {}\n  }}", pattern));
        self
    }

    /// Add BIND statement
    pub fn bind(mut self, expression: String, var: String) -> QueryBuilder {
        self.bind_statements.push(format!("BIND ({} AS {})", expression, var));
        self
    }

    /// Add GROUP BY clause
    pub fn group_by(mut self, vars: String) -> QueryBuilder {
        self.group_by_vars = vars.split_whitespace()
            .map(|s| s.to_string())
            .collect();
        self
    }

    /// Add ORDER BY clause (ascending)
    pub fn order_by(mut self, vars: String) -> QueryBuilder {
        for var in vars.split_whitespace() {
            self.order_by_vars.push((var.to_string(), false));
        }
        self
    }

    /// Add ORDER BY DESC clause
    pub fn order_by_desc(mut self, vars: String) -> QueryBuilder {
        for var in vars.split_whitespace() {
            self.order_by_vars.push((var.to_string(), true));
        }
        self
    }

    /// Add LIMIT clause
    pub fn limit(mut self, count: u32) -> QueryBuilder {
        self.limit = Some(count);
        self
    }

    /// Add OFFSET clause
    pub fn offset(mut self, count: u32) -> QueryBuilder {
        self.offset = Some(count);
        self
    }

    /// Add DISTINCT modifier
    pub fn distinct(mut self) -> QueryBuilder {
        self.distinct = true;
        self
    }

    /// Add VALUES clause
    pub fn values(mut self, clause: String) -> QueryBuilder {
        self.values_clause = Some(clause);
        self
    }

    /// Build the final query string
    pub fn build(&self) -> String {
        let mut query = String::new();

        // Build query prefix
        match self.query_type {
            QueryType::Select => {
                query.push_str("SELECT ");
                if self.distinct {
                    query.push_str("DISTINCT ");
                }
                query.push_str(&self.select_vars);
                query.push('\n');
            }
            QueryType::Construct => {
                query.push_str("CONSTRUCT {\n");
                query.push_str(&self.construct_template);
                query.push_str("\n}\n");
            }
            QueryType::Describe => {
                query.push_str("DESCRIBE ");
                query.push_str(&self.describe_vars);
                query.push('\n');
            }
            QueryType::Ask => {
                query.push_str("ASK\n");
            }
        }

        // Add FROM clauses
        for from in &self.from_clauses {
            query.push_str(from);
            query.push('\n');
        }

        // Build WHERE clause
        query.push_str("WHERE {\n");
        query.push_str(&self.where_pattern);

        // Add OPTIONAL patterns
        for optional in &self.optional_patterns {
            query.push('\n');
            query.push_str("  ");
            query.push_str(optional);
        }

        // Add FILTER conditions
        for filter in &self.filter_conditions {
            query.push('\n');
            query.push_str("  ");
            query.push_str(filter);
        }

        // Add BIND statements
        for bind in &self.bind_statements {
            query.push('\n');
            query.push_str("  ");
            query.push_str(bind);
        }

        // Add VALUES clause
        if let Some(values) = &self.values_clause {
            query.push('\n');
            query.push_str("  ");
            query.push_str(values);
        }

        query.push_str("\n}\n");

        // Add GROUP BY
        if !self.group_by_vars.is_empty() {
            query.push_str("GROUP BY ");
            query.push_str(&self.group_by_vars.join(" "));
            query.push('\n');
        }

        // Add ORDER BY
        if !self.order_by_vars.is_empty() {
            query.push_str("ORDER BY ");
            for (var, is_desc) in &self.order_by_vars {
                if *is_desc {
                    query.push_str("DESC(");
                    query.push_str(var);
                    query.push_str(") ");
                } else {
                    query.push_str(var);
                    query.push(' ');
                }
            }
            query.push('\n');
        }

        // Add LIMIT
        if let Some(limit) = self.limit {
            query.push_str(&format!("LIMIT {}\n", limit));
        }

        // Add OFFSET
        if let Some(offset) = self.offset {
            query.push_str(&format!("OFFSET {}\n", offset));
        }

        query
    }

}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_result_format_json() {
        let format = ResultFormat::Json;
        assert_eq!(format.to_mime_type(), "application/sparql-results+json");
    }

    #[test]
    fn test_result_format_turtle() {
        let format = ResultFormat::Turtle;
        assert_eq!(format.to_mime_type(), "text/turtle");
    }

    #[test]
    fn test_query_type_select() {
        let qt = QueryType::Select;
        assert_eq!(qt.as_str(), "SELECT");
    }

    #[test]
    fn test_query_type_construct() {
        let qt = QueryType::Construct;
        assert_eq!(qt.as_str(), "CONSTRUCT");
    }

    #[test]
    fn test_query_builder_select() {
        let builder = QueryBuilder::new()
            .select("?s ?p ?o".to_string())
            .where_clause("?s ?p ?o".to_string());

        let query = builder.build();
        assert!(query.contains("SELECT ?s ?p ?o"));
        assert!(query.contains("WHERE"));
    }

    #[test]
    fn test_query_builder_with_filter() {
        let builder = QueryBuilder::new()
            .select("?s".to_string())
            .where_clause("?s ?p ?o".to_string())
            .filter("?s = <http://example.org>".to_string());

        let query = builder.build();
        assert!(query.contains("FILTER"));
        assert!(query.contains("?s = <http://example.org>"));
    }

    #[test]
    fn test_query_builder_with_limit_offset() {
        let builder = QueryBuilder::new()
            .select("?s".to_string())
            .where_clause("?s ?p ?o".to_string())
            .limit(10)
            .offset(5);

        let query = builder.build();
        assert!(query.contains("LIMIT 10"));
        assert!(query.contains("OFFSET 5"));
    }

    #[test]
    fn test_query_builder_construct() {
        let builder = QueryBuilder::new()
            .construct_query()
            .construct("?s ?p ?o".to_string())
            .where_clause("?s ?p ?o".to_string());

        let query = builder.build();
        assert!(query.contains("CONSTRUCT"));
        assert!(query.contains("?s ?p ?o"));
    }

    #[test]
    fn test_query_builder_distinct() {
        let builder = QueryBuilder::new()
            .select("?s".to_string())
            .distinct()
            .where_clause("?s ?p ?o".to_string());

        let query = builder.build();
        assert!(query.contains("DISTINCT"));
    }
}

// Module initialization
#[wasm_bindgen]
pub async fn init() -> Result<(), JsValue> {
    // Initialize panic hook for better error messages
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();

    // TODO: Initialize libqlever WASM module when available
    Ok(())
}

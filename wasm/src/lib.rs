use wasm_bindgen::prelude::*;
use wasm_bindgen_futures::JsFuture;
use web_sys::{Request, RequestInit, RequestMode, Response};
use js_sys::Object;
use serde::{Deserialize, Serialize};
use std::str::FromStr;

// Set panic hook for better error messages
#[cfg(feature = "console_error_panic_hook")]
pub fn set_panic_hook() {
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}

/// Query result format
#[wasm_bindgen]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ResultFormat {
    Json,
    Xml,
    Csv,
}

impl FromStr for ResultFormat {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s.to_lowercase().as_str() {
            "json" => Ok(ResultFormat::Json),
            "xml" => Ok(ResultFormat::Xml),
            "csv" => Ok(ResultFormat::Csv),
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

        request
            .headers()
            .set("Accept", result_format.to_mime_type())
            .map_err(|_| JsValue::from_str("Failed to set headers"))?;

        let window = web_sys::window().ok_or_else(|| JsValue::from_str("No window object"))?;
        let response_promise = window.fetch_with_request(&request);

        let response = JsFuture::from(response_promise)
            .await
            .map_err(|_| JsValue::from_str("Fetch failed"))?;

        let response: Response = response.dyn_into()
            .map_err(|_| JsValue::from_str("Failed to convert to Response"))?;

        if !response.ok() {
            return Err(JsValue::from_str(&format!("HTTP {}: {}", response.status(), response.status_text())));
        }

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

        // Set default header
        request
            .headers()
            .set("Accept", result_format.to_mime_type())
            .map_err(|_| JsValue::from_str("Failed to set Accept header"))?;

        // Set custom headers from object
        if !headers.is_null() && !headers.is_undefined() {
            let headers_obj = web_sys::Headers::new_with_headers_like(&headers)
                .map_err(|_| JsValue::from_str("Failed to parse headers"))?;

            for entry in js_sys::Object::entries(&headers) {
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

        let window = web_sys::window().ok_or_else(|| JsValue::from_str("No window object"))?;
        let response_promise = window.fetch_with_request(&request);

        let response = JsFuture::from(response_promise)
            .await
            .map_err(|_| JsValue::from_str("Fetch failed"))?;

        let response: Response = response.dyn_into()
            .map_err(|_| JsValue::from_str("Failed to convert to Response"))?;

        if !response.ok() {
            return Err(JsValue::from_str(&format!("HTTP {}: {}", response.status(), response.status_text())));
        }

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

/// Query builder for constructing SPARQL queries
#[wasm_bindgen]
pub struct QueryBuilder {
    query: String,
}

#[wasm_bindgen]
impl QueryBuilder {
    /// Create a new query builder
    #[wasm_bindgen(constructor)]
    pub fn new() -> QueryBuilder {
        QueryBuilder {
            query: String::new(),
        }
    }

    /// Add SELECT clause
    pub fn select(&mut self, vars: String) -> QueryBuilder {
        self.query.push_str(&format!("SELECT {}\n", vars));
        QueryBuilder {
            query: self.query.clone(),
        }
    }

    /// Add FROM clause
    pub fn from(&mut self, graph: String) -> QueryBuilder {
        self.query.push_str(&format!("FROM <{}>\n", graph));
        QueryBuilder {
            query: self.query.clone(),
        }
    }

    /// Add WHERE clause
    pub fn where_clause(&mut self, pattern: String) -> QueryBuilder {
        self.query.push_str(&format!("WHERE {{\n  {}\n}}\n", pattern));
        QueryBuilder {
            query: self.query.clone(),
        }
    }

    /// Build the final query string
    pub fn build(&self) -> String {
        self.query.clone()
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
    fn test_query_builder() {
        let mut builder = QueryBuilder::new();
        let query = builder
            .select("?s ?p ?o".to_string())
            .where_clause("?s ?p ?o".to_string())
            .build();

        assert!(query.contains("SELECT ?s ?p ?o"));
        assert!(query.contains("WHERE"));
    }
}

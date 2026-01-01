//! Store implementation for QLever
//!
//! The Store is the main entry point for interacting with a QLever server,
//! providing methods for querying RDF data and executing SPARQL operations.
//! The API is designed to be similar to oxigraph's Store interface.

use crate::error::{Error, Result};
use crate::model::{NamedNode, Quad, Term, Triple};
use crate::query::{QueryResults, QuerySolution};
use reqwest::Client;
use std::sync::Arc;
use url::Url;

/// Configuration for Store connection
///
/// Use the builder pattern for fluent configuration:
///
/// ```ignore
/// let config = StoreConfig::builder()
///     .url("http://localhost:7777")
///     .timeout_secs(60)
///     .build()?;
/// ```
#[derive(Debug, Clone)]
pub struct StoreConfig {
    url: String,
    timeout_secs: u64,
}

impl StoreConfig {
    /// Creates a new builder for StoreConfig
    pub fn builder() -> StoreConfigBuilder {
        StoreConfigBuilder::default()
    }

    /// Creates a new store configuration with default timeout
    pub fn new(url: impl Into<String>) -> Self {
        StoreConfig {
            url: url.into(),
            timeout_secs: 30,
        }
    }

    /// Sets the timeout for requests (builder pattern)
    pub fn with_timeout(mut self, secs: u64) -> Self {
        self.timeout_secs = secs;
        self
    }

    /// Gets the configured URL
    pub fn url(&self) -> &str {
        &self.url
    }

    /// Gets the configured timeout in seconds
    pub fn timeout_secs(&self) -> u64 {
        self.timeout_secs
    }
}

/// Builder for StoreConfig with fluent interface
#[derive(Debug, Default)]
pub struct StoreConfigBuilder {
    url: Option<String>,
    timeout_secs: u64,
}

impl StoreConfigBuilder {
    /// Sets the endpoint URL
    pub fn url(mut self, url: impl Into<String>) -> Self {
        self.url = Some(url.into());
        self
    }

    /// Sets the timeout duration in seconds (default: 30)
    pub fn timeout_secs(mut self, secs: u64) -> Self {
        self.timeout_secs = secs;
        self
    }

    /// Builds the StoreConfig, validating the URL
    pub fn build(self) -> Result<StoreConfig> {
        let url = self.url.ok_or_else(|| Error::InvalidUrl("URL is required".to_string()))?;
        // Validate URL format
        Url::parse(&url).map_err(|_| Error::InvalidUrl(url.clone()))?;

        Ok(StoreConfig {
            url,
            timeout_secs: self.timeout_secs,
        })
    }
}

/// A QLever RDF store
///
/// The Store provides methods for executing SPARQL queries and managing RDF data
/// in a QLever server. It communicates with QLever via HTTP.
pub struct Store {
    client: Arc<Client>,
    endpoint_url: String,
}

impl Store {
    /// Creates a new Store connected to a QLever server
    ///
    /// # Arguments
    ///
    /// * `url` - The base URL of the QLever server (e.g., "http://localhost:7777")
    ///
    /// # Errors
    ///
    /// Returns an error if the URL is invalid or the connection cannot be established.
    ///
    /// # Example
    ///
    /// ```ignore
    /// let store = Store::new("http://localhost:7777")?;
    /// let results = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10").await?;
    /// ```
    pub fn new(url: impl Into<String>) -> Result<Self> {
        let url = url.into();
        Self::with_config(StoreConfig::new(url))
    }

    /// Creates a new Store with custom configuration
    ///
    /// # Example
    ///
    /// ```ignore
    /// let config = StoreConfig::builder()
    ///     .url("http://localhost:7777")
    ///     .timeout_secs(60)
    ///     .build()?;
    /// let store = Store::with_config(config)?;
    /// ```
    pub fn with_config(config: StoreConfig) -> Result<Self> {
        // Validate the URL
        Url::parse(config.url()).map_err(|_| Error::InvalidUrl(config.url().to_string()))?;

        let timeout = std::time::Duration::from_secs(config.timeout_secs());
        let client = Client::builder()
            .timeout(timeout)
            .build()
            .map_err(Error::Http)?;

        // Ensure endpoint URL ends with /
        let endpoint_url = if config.url().ends_with('/') {
            config.url().to_string()
        } else {
            format!("{}/", config.url())
        };

        Ok(Store {
            client: Arc::new(client),
            endpoint_url,
        })
    }

    /// Executes a SPARQL SELECT query
    ///
    /// # Arguments
    ///
    /// * `query` - The SPARQL query string
    ///
    /// # Returns
    ///
    /// Returns a vector of query solutions (variable bindings)
    pub async fn query(&self, query: &str) -> Result<Vec<QuerySolution>> {
        let query_url = format!("{}api/sparql", self.endpoint_url);

        let response = self
            .client
            .post(&query_url)
            .header("Accept", "application/sparql-results+json")
            .form(&[("query", query)])
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            return Err(Error::QueryError(format!(
                "Query failed with status {}: {}",
                status, text
            )));
        }

        let results: QueryResults = response.json().await.map_err(Error::Http)?;

        Ok(results.results.bindings)
    }

    /// Executes a SPARQL CONSTRUCT query
    ///
    /// # Arguments
    ///
    /// * `query` - The SPARQL CONSTRUCT query string
    ///
    /// # Returns
    ///
    /// Returns a vector of triples
    pub async fn construct(&self, query: &str) -> Result<Vec<Triple>> {
        let query_url = format!("{}api/sparql", self.endpoint_url);

        let response = self
            .client
            .post(&query_url)
            .header("Accept", "text/turtle")
            .form(&[("query", query)])
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            return Err(Error::QueryError(format!(
                "Query failed with status {}: {}",
                status, text
            )));
        }

        let rdf_text = response.text().await?;
        Self::parse_turtle(&rdf_text)
    }

    /// Executes a SPARQL ASK query
    ///
    /// # Arguments
    ///
    /// * `query` - The SPARQL ASK query string
    ///
    /// # Returns
    ///
    /// Returns true if the query matches any results, false otherwise
    pub async fn ask(&self, query: &str) -> Result<bool> {
        let query_url = format!("{}api/sparql", self.endpoint_url);

        let response = self
            .client
            .post(&query_url)
            .header("Accept", "application/sparql-results+json")
            .form(&[("query", query)])
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            return Err(Error::QueryError(format!(
                "Query failed with status {}: {}",
                status, text
            )));
        }

        let json: serde_json::Value = response.json().await?;
        let result = json
            .get("boolean")
            .and_then(|v| v.as_bool())
            .unwrap_or(false);

        Ok(result)
    }

    /// Executes a SPARQL DESCRIBE query
    ///
    /// # Arguments
    ///
    /// * `query` - The SPARQL DESCRIBE query string
    ///
    /// # Returns
    ///
    /// Returns a vector of quads describing the resources
    pub async fn describe(&self, query: &str) -> Result<Vec<Quad>> {
        let query_url = format!("{}api/sparql", self.endpoint_url);

        let response = self
            .client
            .post(&query_url)
            .header("Accept", "application/n-quads")
            .form(&[("query", query)])
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            return Err(Error::QueryError(format!(
                "Query failed with status {}: {}",
                status, text
            )));
        }

        let rdf_text = response.text().await?;
        Self::parse_nquads(&rdf_text)
    }

    /// Gets server statistics
    ///
    /// # Returns
    ///
    /// Returns a JSON object containing server statistics
    pub async fn stats(&self) -> Result<serde_json::Value> {
        let stats_url = format!("{}api/stats", self.endpoint_url);

        let response = self
            .client
            .get(&stats_url)
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            return Err(Error::QueryError(format!(
                "Failed to get stats: status {}",
                status
            )));
        }

        let json = response.json().await?;
        Ok(json)
    }

    /// Gets autocompletion suggestions for a partial SPARQL query
    ///
    /// # Arguments
    ///
    /// * `query` - The partial SPARQL query
    /// * `cursor_position` - The cursor position in the query
    ///
    /// # Returns
    ///
    /// Returns a vector of autocompletion suggestions
    pub async fn autocomplete(
        &self,
        query: &str,
        cursor_position: Option<usize>,
    ) -> Result<Vec<String>> {
        let mut autocomplete_url = format!("{}api/autocomplete", self.endpoint_url);

        let query_params = if let Some(pos) = cursor_position {
            format!(
                "?query={}&cursorPosition={}",
                urlencoding::encode(query),
                pos
            )
        } else {
            format!("?query={}", urlencoding::encode(query))
        };

        autocomplete_url.push_str(&query_params);

        let response = self
            .client
            .get(&autocomplete_url)
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            return Err(Error::QueryError(format!(
                "Failed to get autocompletion: status {}",
                status
            )));
        }

        let suggestions: Vec<String> = response.json().await?;
        Ok(suggestions)
    }

    /// Helper method for making SPARQL queries and handling responses
    async fn execute_sparql_request(
        &self,
        query: &str,
        accept_header: &str,
    ) -> Result<String> {
        let query_url = format!("{}api/sparql", self.endpoint_url);

        let response = self
            .client
            .post(&query_url)
            .header("Accept", accept_header)
            .form(&[("query", query)])
            .send()
            .await
            .map_err(Error::Http)?;

        if !response.status().is_success() {
            let status = response.status();
            let text = response.text().await.unwrap_or_default();
            return Err(Error::QueryError(format!(
                "Query failed with status {}: {}",
                status, text
            )));
        }

        response.text().await.map_err(Error::Http)
    }

    // Helper methods for parsing RDF formats

    /// Parses Turtle format RDF
    fn parse_turtle(turtle: &str) -> Result<Vec<Triple>> {
        // Simple basic parsing - in production, use a proper RDF parser
        let mut triples = Vec::new();

        for line in turtle.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            // Very basic triple parsing (subject predicate object .)
            if let Some(dot_idx) = line.rfind('.') {
                let triple_str = &line[..dot_idx].trim();
                let parts: Vec<&str> = triple_str.split_whitespace().collect();

                if parts.len() >= 3 {
                    if let (Ok(subj), Ok(pred)) = (
                        NamedNode::new(Self::clean_uri(parts[0]).to_string()),
                        NamedNode::new(Self::clean_uri(parts[1]).to_string()),
                    ) {
                        let obj_str = parts[2..].join(" ");
                        if let Ok(obj) = Term::from_str(&obj_str) {
                            triples.push(Triple::new(subj, pred, obj));
                        }
                    }
                }
            }
        }

        Ok(triples)
    }

    /// Parses N-Quads format RDF
    fn parse_nquads(nquads: &str) -> Result<Vec<Quad>> {
        let mut quads = Vec::new();

        for line in nquads.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            // N-Quads format: subject predicate object [graphLabel] .
            if let Some(dot_idx) = line.rfind('.') {
                let quad_str = &line[..dot_idx].trim();
                let parts: Vec<&str> = quad_str.split_whitespace().collect();

                if parts.len() >= 3 {
                    if let (Ok(subj), Ok(pred)) = (
                        NamedNode::new(Self::clean_uri(parts[0]).to_string()),
                        NamedNode::new(Self::clean_uri(parts[1]).to_string()),
                    ) {
                        let (obj_str, graph_name) = if parts.len() > 3 {
                            (
                                parts[2..parts.len() - 1].join(" "),
                                NamedNode::new(Self::clean_uri(parts[parts.len() - 1]).to_string())
                                    .ok(),
                            )
                        } else {
                            (parts[2..].join(" "), None)
                        };

                        if let Ok(obj) = Term::from_str(&obj_str) {
                            quads.push(Quad::new(subj, pred, obj, graph_name));
                        }
                    }
                }
            }
        }

        Ok(quads)
    }

    /// Cleans up URI by removing angle brackets if present
    fn clean_uri(uri: &str) -> &str {
        if uri.starts_with('<') && uri.ends_with('>') {
            &uri[1..uri.len() - 1]
        } else {
            uri
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_store_config_new() {
        let config = StoreConfig::new("http://localhost:7777");
        assert_eq!(config.url(), "http://localhost:7777");
        assert_eq!(config.timeout_secs(), 30);
    }

    #[test]
    fn test_store_config_with_timeout() {
        let config = StoreConfig::new("http://localhost:7777").with_timeout(60);
        assert_eq!(config.timeout_secs(), 60);
    }

    #[test]
    fn test_store_config_builder() {
        let config = StoreConfig::builder()
            .url("http://localhost:7777")
            .timeout_secs(45)
            .build()
            .expect("Builder should succeed");

        assert_eq!(config.url(), "http://localhost:7777");
        assert_eq!(config.timeout_secs(), 45);
    }

    #[test]
    fn test_store_config_builder_missing_url() {
        let result = StoreConfig::builder().timeout_secs(30).build();
        assert!(result.is_err());
    }

    #[test]
    fn test_store_config_builder_invalid_url() {
        let result = StoreConfig::builder()
            .url("not a valid url")
            .build();
        assert!(result.is_err());
    }

    #[test]
    fn test_clean_uri() {
        assert_eq!(
            Store::clean_uri("<http://example.org>"),
            "http://example.org"
        );
        assert_eq!(Store::clean_uri("http://example.org"), "http://example.org");
    }
}

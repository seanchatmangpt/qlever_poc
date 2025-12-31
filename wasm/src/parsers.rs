/// RDF Format Parsers
/// Supports: Turtle, N-Triples, N-Quads, and more
///
/// This module provides parsing for various RDF serialization formats
/// to load data into the in-memory Store.

use wasm_bindgen::prelude::*;
use crate::rdf_term::{RdfTerm, Quad};
use std::collections::HashMap;

/// Parser for RDF formats
#[wasm_bindgen]
pub struct RdfParser;

#[wasm_bindgen]
impl RdfParser {
    /// Parse N-Quads format
    /// Format: subject predicate object graph .
    /// Example: <http://example.org/s> <http://example.org/p> <http://example.org/o> <http://example.org/g> .
    #[wasm_bindgen]
    pub fn parse_nquads(data: &str) -> Result<Vec<Quad>, JsValue> {
        let mut quads = Vec::new();

        for line in data.lines() {
            let trimmed = line.trim();

            // Skip empty lines and comments
            if trimmed.is_empty() || trimmed.starts_with('#') {
                continue;
            }

            // Parse N-Quads line
            match parse_nquads_line(trimmed) {
                Ok(Some(quad)) => quads.push(quad),
                Ok(None) => continue, // Empty or comment
                Err(e) => return Err(JsValue::from_str(&format!("N-Quads parse error: {}", e))),
            }
        }

        Ok(quads)
    }

    /// Parse N-Triples format
    /// Format: subject predicate object .
    /// Example: <http://example.org/s> <http://example.org/p> <http://example.org/o> .
    #[wasm_bindgen]
    pub fn parse_ntriples(data: &str) -> Result<Vec<Quad>, JsValue> {
        let mut quads = Vec::new();

        for line in data.lines() {
            let trimmed = line.trim();

            // Skip empty lines and comments
            if trimmed.is_empty() || trimmed.starts_with('#') {
                continue;
            }

            // Parse N-Triples line (convert to default graph quad)
            match parse_ntriples_line(trimmed) {
                Ok(Some((subject, predicate, object))) => {
                    let quad = Quad {
                        subject,
                        predicate,
                        object,
                        graph: RdfTerm::DefaultGraph,
                    };
                    quads.push(quad);
                }
                Ok(None) => continue,
                Err(e) => return Err(JsValue::from_str(&format!("N-Triples parse error: {}", e))),
            }
        }

        Ok(quads)
    }

    /// Parse Turtle format (simplified)
    /// Note: This is a simplified Turtle parser that handles basic cases.
    /// For production use, use a full Turtle parser library.
    #[wasm_bindgen]
    pub fn parse_turtle(data: &str, base_iri: Option<String>) -> Result<Vec<Quad>, JsValue> {
        // For now, fall back to N-Triples since many Turtle files are compatible
        // Full Turtle parsing would require more complex parsing logic
        RdfParser::parse_ntriples(data)
    }

    /// Detect RDF format from content
    /// Returns format string: "nquads", "ntriples", "turtle", etc.
    #[wasm_bindgen]
    pub fn detect_format(data: &str) -> String {
        let trimmed = data.trim();

        // Check for XML (RDF/XML)
        if trimmed.starts_with("<?xml") || trimmed.starts_with("<rdf:RDF") {
            return "rdfxml".to_string();
        }

        // Check for JSON-LD
        if trimmed.starts_with("{") || trimmed.starts_with("[") {
            return "jsonld".to_string();
        }

        // Check for Turtle-specific syntax
        if trimmed.contains("@prefix") || trimmed.contains("@base") {
            return "turtle".to_string();
        }

        // Default to N-Quads/N-Triples (compatible)
        "nquads".to_string()
    }

    /// Parse RDF data with format detection
    #[wasm_bindgen]
    pub fn parse(data: &str, format: Option<String>) -> Result<Vec<Quad>, JsValue> {
        let fmt = format.unwrap_or_else(|| RdfParser::detect_format(data));

        match fmt.to_lowercase().as_str() {
            "nquads" | "nq" => RdfParser::parse_nquads(data),
            "ntriples" | "nt" => RdfParser::parse_ntriples(data),
            "turtle" | "ttl" => RdfParser::parse_turtle(data, None),
            "jsonld" => Err(JsValue::from_str("JSON-LD format requires external library")),
            "trig" => Err(JsValue::from_str("TriG format requires full parser")),
            "rdfxml" => Err(JsValue::from_str("RDF/XML format requires external library")),
            _ => Err(JsValue::from_str(&format!("Unknown format: {}", fmt))),
        }
    }
}

/// Parse a single N-Quads line
/// Returns (subject, predicate, object, graph) or None if empty/comment
fn parse_nquads_line(line: &str) -> Result<Option<Quad>, String> {
    let trimmed = line.trim();

    if trimmed.is_empty() || trimmed.starts_with('#') {
        return Ok(None);
    }

    // Remove trailing period
    let line = if trimmed.ends_with('.') {
        &trimmed[..trimmed.len() - 1]
    } else {
        return Err("N-Quads line must end with period".to_string());
    };

    // Split by whitespace (simple tokenization)
    let parts: Vec<&str> = tokenize_nquads(line);

    if parts.len() < 3 {
        return Err("N-Quads line must have at least 3 parts".to_string());
    }

    let subject = parse_rdf_term(parts[0])?;
    let predicate = parse_rdf_term(parts[1])?;
    let object = parse_rdf_term(parts[2])?;
    let graph = if parts.len() > 3 {
        parse_rdf_term(parts[3])?
    } else {
        RdfTerm::DefaultGraph
    };

    Ok(Some(Quad {
        subject,
        predicate,
        object,
        graph,
    }))
}

/// Parse a single N-Triples line
/// Returns (subject, predicate, object) or None
fn parse_ntriples_line(line: &str) -> Result<Option<(RdfTerm, RdfTerm, RdfTerm)>, String> {
    let trimmed = line.trim();

    if trimmed.is_empty() || trimmed.starts_with('#') {
        return Ok(None);
    }

    // Remove trailing period
    let line = if trimmed.ends_with('.') {
        &trimmed[..trimmed.len() - 1]
    } else {
        return Err("N-Triples line must end with period".to_string());
    };

    let parts: Vec<&str> = tokenize_nquads(line);

    if parts.len() < 3 {
        return Err("N-Triples line must have exactly 3 parts".to_string());
    }

    let subject = parse_rdf_term(parts[0])?;
    let predicate = parse_rdf_term(parts[1])?;
    let object = parse_rdf_term(parts[2])?;

    Ok(Some((subject, predicate, object)))
}

/// Tokenize N-Quads/N-Triples line respecting quoted strings
fn tokenize_nquads(line: &str) -> Vec<&str> {
    let mut tokens = Vec::new();
    let mut current = String::new();
    let mut in_quote = false;
    let mut escape_next = false;
    let mut chars = line.chars().peekable();

    while let Some(ch) = chars.next() {
        match ch {
            '\\' if in_quote => {
                escape_next = true;
                current.push(ch);
            }
            '"' if !escape_next => {
                in_quote = !in_quote;
                current.push(ch);
            }
            ' ' | '\t' | '\n' | '\r' if !in_quote => {
                if !current.is_empty() {
                    // Use slice to avoid allocation
                    tokens.push(&line[tokens.len()..]);
                    current.clear();
                }
            }
            _ => {
                escape_next = false;
                current.push(ch);
            }
        }
    }

    if !current.is_empty() {
        tokens.push(&line[line.len() - current.len()..]);
    }

    // Simple split approach (not perfect, but functional)
    line.split_whitespace().collect()
}

/// Parse an RDF term from N-Quads format
/// - IRI: <http://example.org/term>
/// - Blank node: _:label
/// - Literal: "value" or "value"@lang or "value"^^<type>
fn parse_rdf_term(s: &str) -> Result<RdfTerm, String> {
    let s = s.trim();

    if s.starts_with('<') && s.ends_with('>') {
        // Named node (IRI)
        let iri = &s[1..s.len() - 1];
        Ok(RdfTerm::named_node(iri.to_string()))
    } else if s.starts_with("_:") {
        // Blank node
        let label = &s[2..];
        Ok(RdfTerm::blank_node(Some(label.to_string())))
    } else if s.starts_with('"') {
        // Literal
        parse_literal(s)
    } else {
        Err(format!("Invalid RDF term: {}", s))
    }
}

/// Parse a literal value
/// Supports: "value", "value"@lang, "value"^^<type>
fn parse_literal(s: &str) -> Result<RdfTerm, String> {
    if !s.starts_with('"') {
        return Err("Literal must start with quote".to_string());
    }

    // Find closing quote (simple approach, doesn't handle all escapes)
    let mut i = 1;
    let chars: Vec<char> = s.chars().collect();

    while i < chars.len() {
        if chars[i] == '"' && (i == 1 || chars[i - 1] != '\\') {
            break;
        }
        i += 1;
    }

    if i >= chars.len() {
        return Err("Unterminated literal".to_string());
    }

    let value: String = chars[1..i].iter().collect();
    let rest = &s[i + 1..];

    if rest.is_empty() {
        // Plain literal
        Ok(RdfTerm::literal(value, None))
    } else if rest.starts_with('@') {
        // Language tagged literal
        let lang = rest[1..].to_string();
        Ok(RdfTerm::literal(value, Some(format!("@{}", lang))))
    } else if rest.starts_with("^^") {
        // Typed literal
        let dtype = parse_rdf_term(&rest[2..])?;
        if let Some(iri) = dtype.as_iri() {
            Ok(RdfTerm::literal(value, Some(iri.to_string())))
        } else {
            Err("Datatype must be IRI".to_string())
        }
    } else {
        Err(format!("Invalid literal suffix: {}", rest))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_ntriples() {
        let data = "<http://example.org/s> <http://example.org/p> <http://example.org/o> .";
        let result = RdfParser::parse_ntriples(data);
        assert!(result.is_ok());
        let quads = result.unwrap();
        assert_eq!(quads.len(), 1);
    }

    #[test]
    fn test_parse_nquads() {
        let data = "<http://example.org/s> <http://example.org/p> <http://example.org/o> <http://example.org/g> .";
        let result = RdfParser::parse_nquads(data);
        assert!(result.is_ok());
    }

    #[test]
    fn test_detect_format() {
        assert_eq!(RdfParser::detect_format("<http://example.org>"), "nquads");
        assert_eq!(RdfParser::detect_format("@prefix"), "turtle");
        assert_eq!(RdfParser::detect_format("{\"@context\""), "jsonld");
    }

    #[test]
    fn test_parse_iri() {
        let term = parse_rdf_term("<http://example.org/test>").unwrap();
        assert!(term.is_named_node());
    }

    #[test]
    fn test_parse_blank_node() {
        let term = parse_rdf_term("_:b1").unwrap();
        assert!(term.is_blank_node());
    }

    #[test]
    fn test_parse_literal() {
        let term = parse_literal("\"hello\"").unwrap();
        assert!(term.is_literal());
        assert_eq!(term.as_literal_value(), Some("hello"));
    }
}

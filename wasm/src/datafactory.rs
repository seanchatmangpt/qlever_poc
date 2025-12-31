/// RDF/JS DataFactory Implementation
/// Implements the RDF/JS specification DataFactory interface

use wasm_bindgen::prelude::*;
use crate::rdf_term::{RdfTerm, Triple, Quad, QuadPattern};

/// DataFactory for creating RDF terms, triples, and quads
#[wasm_bindgen]
pub struct DataFactory;

#[wasm_bindgen]
impl DataFactory {
    /// Create a named node from an IRI
    #[wasm_bindgen]
    pub fn named_node(iri: String) -> RdfTerm {
        RdfTerm::named_node(iri)
    }

    /// Create a blank node with optional label
    #[wasm_bindgen]
    pub fn blank_node(label: Option<String>) -> RdfTerm {
        RdfTerm::blank_node(label)
    }

    /// Create a literal with optional language tag or datatype
    /// Examples:
    /// - literal("hello") -> plain literal
    /// - literal("hello", Some("@en")) -> language-tagged literal
    /// - literal("42", Some("http://www.w3.org/2001/XMLSchema#integer")) -> typed literal
    #[wasm_bindgen]
    pub fn literal(value: String, lang_or_datatype: Option<String>) -> RdfTerm {
        RdfTerm::literal(value, lang_or_datatype)
    }

    /// Create a triple (subject, predicate, object)
    #[wasm_bindgen]
    pub fn triple(subject: String, predicate: String, object: String) -> Triple {
        Triple::new(&subject, &predicate, &object)
    }

    /// Create a quad (subject, predicate, object, graph)
    #[wasm_bindgen]
    pub fn quad(
        subject: String,
        predicate: String,
        object: String,
        graph: Option<String>,
    ) -> Quad {
        Quad::new(&subject, &predicate, &object, graph)
    }

    /// Create a pattern for matching quads (wildcards for None values)
    #[wasm_bindgen]
    pub fn quad_pattern(
        subject: Option<String>,
        predicate: Option<String>,
        object: Option<String>,
        graph: Option<String>,
    ) -> QuadPattern {
        QuadPattern::new(subject, predicate, object, graph)
    }

    /// Get the default graph term
    #[wasm_bindgen]
    pub fn default_graph() -> RdfTerm {
        RdfTerm::DefaultGraph
    }

    /// Convert a term to its N-Quads string representation
    #[wasm_bindgen]
    pub fn term_to_string(term: &RdfTerm) -> String {
        term.to_n_quads()
    }

    /// Get the IRI of a named node
    #[wasm_bindgen]
    pub fn get_iri(term: &RdfTerm) -> Option<String> {
        term.as_iri().map(|s| s.to_string())
    }

    /// Get the value of a literal
    #[wasm_bindgen]
    pub fn get_literal_value(term: &RdfTerm) -> Option<String> {
        term.as_literal_value().map(|s| s.to_string())
    }
}

/// Export factory as a singleton for convenience
#[wasm_bindgen]
pub fn get_data_factory() -> DataFactory {
    DataFactory
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_named_node_factory() {
        let node = DataFactory::named_node("http://example.org/test".to_string());
        assert!(node.is_named_node());
        assert_eq!(DataFactory::get_iri(&node), Some("http://example.org/test".to_string()));
    }

    #[test]
    fn test_blank_node_factory() {
        let bn = DataFactory::blank_node(Some("b1".to_string()));
        assert!(bn.is_blank_node());
    }

    #[test]
    fn test_literal_factory() {
        let lit = DataFactory::literal("hello".to_string(), None);
        assert!(lit.is_literal());
        assert_eq!(DataFactory::get_literal_value(&lit), Some("hello".to_string()));
    }

    #[test]
    fn test_triple_factory() {
        let triple = DataFactory::triple(
            "http://example.org/s".to_string(),
            "http://example.org/p".to_string(),
            "http://example.org/o".to_string(),
        );
        assert_eq!(
            DataFactory::get_iri(&triple.subject),
            Some("http://example.org/s".to_string())
        );
    }

    #[test]
    fn test_quad_factory() {
        let quad = DataFactory::quad(
            "http://example.org/s".to_string(),
            "http://example.org/p".to_string(),
            "http://example.org/o".to_string(),
            Some("http://example.org/g".to_string()),
        );
        assert_eq!(
            DataFactory::get_iri(&quad.subject),
            Some("http://example.org/s".to_string())
        );
    }
}

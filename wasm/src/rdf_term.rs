/// RDF/JS DataFactory types and implementations
/// Implements the RDF/JS specification for term representation

use wasm_bindgen::prelude::*;
use serde::{Deserialize, Serialize};
use std::fmt;
use std::hash::{Hash, Hasher};

/// Represents an RDF term (Node or Literal)
#[wasm_bindgen]
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum RdfTerm {
    NamedNode(String), // IRI
    BlankNode(String), // Label
    Literal(String, Option<String>, Option<String>), // value, datatype, language
    DefaultGraph,
}

impl RdfTerm {
    /// Create a named node (IRI)
    pub fn named_node(iri: String) -> Self {
        RdfTerm::NamedNode(canonicalize_iri(&iri))
    }

    /// Create a blank node with optional label
    pub fn blank_node(label: Option<String>) -> Self {
        let label = label.unwrap_or_else(|| generate_blank_node_label());
        RdfTerm::BlankNode(label)
    }

    /// Create a literal with optional datatype or language tag
    pub fn literal(value: String, lang_or_datatype: Option<String>) -> Self {
        match lang_or_datatype {
            Some(tag) if tag.starts_with('@') => {
                // Language tag (e.g., "@en")
                let lang = tag.strip_prefix('@').unwrap_or("").to_string();
                RdfTerm::Literal(value, None, Some(lang))
            }
            Some(datatype) => {
                // Datatype (e.g., "http://www.w3.org/2001/XMLSchema#integer")
                RdfTerm::Literal(value, Some(datatype), None)
            }
            None => {
                // Plain literal (xsd:string by default in SPARQL)
                RdfTerm::Literal(value, None, None)
            }
        }
    }

    /// Get the string representation (for N-Quads/SPARQL)
    pub fn to_n_quads(&self) -> String {
        match self {
            RdfTerm::NamedNode(iri) => format!("<{}>", iri),
            RdfTerm::BlankNode(label) => format!("_:{}", label),
            RdfTerm::Literal(value, datatype, language) => {
                if let Some(lang) = language {
                    format!("\"{}\"@{}", escape_string(value), lang)
                } else if let Some(dtype) = datatype {
                    format!("\"{}\"^^<{}>", escape_string(value), dtype)
                } else {
                    format!("\"{}\"", escape_string(value))
                }
            }
            RdfTerm::DefaultGraph => "".to_string(),
        }
    }

    /// Check if this is a named node
    pub fn is_named_node(&self) -> bool {
        matches!(self, RdfTerm::NamedNode(_))
    }

    /// Check if this is a blank node
    pub fn is_blank_node(&self) -> bool {
        matches!(self, RdfTerm::BlankNode(_))
    }

    /// Check if this is a literal
    pub fn is_literal(&self) -> bool {
        matches!(self, RdfTerm::Literal(_, _, _))
    }

    /// Get IRI value if named node
    pub fn as_iri(&self) -> Option<&str> {
        match self {
            RdfTerm::NamedNode(iri) => Some(iri),
            _ => None,
        }
    }

    /// Get value as string (for literals)
    pub fn as_literal_value(&self) -> Option<&str> {
        match self {
            RdfTerm::Literal(value, _, _) => Some(value),
            _ => None,
        }
    }
}

impl Hash for RdfTerm {
    fn hash<H: Hasher>(&self, state: &mut H) {
        match self {
            RdfTerm::NamedNode(iri) => {
                0u8.hash(state);
                iri.hash(state);
            }
            RdfTerm::BlankNode(label) => {
                1u8.hash(state);
                label.hash(state);
            }
            RdfTerm::Literal(value, datatype, language) => {
                2u8.hash(state);
                value.hash(state);
                datatype.hash(state);
                language.hash(state);
            }
            RdfTerm::DefaultGraph => {
                3u8.hash(state);
            }
        }
    }
}

impl Display for RdfTerm {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.to_n_quads())
    }
}

use std::fmt::Display;

/// Represents an RDF Triple (subject, predicate, object)
#[wasm_bindgen]
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Triple {
    pub subject: RdfTerm,
    pub predicate: RdfTerm,
    pub object: RdfTerm,
}

#[wasm_bindgen]
impl Triple {
    #[wasm_bindgen(constructor)]
    pub fn new(subject: &str, predicate: &str, object: &str) -> Triple {
        Triple {
            subject: RdfTerm::named_node(subject.to_string()),
            predicate: RdfTerm::named_node(predicate.to_string()),
            object: RdfTerm::named_node(object.to_string()),
        }
    }

    pub fn to_n_triples(&self) -> String {
        format!("{} {} {} .", self.subject, self.predicate, self.object)
    }
}

/// Represents an RDF Quad (subject, predicate, object, graph)
#[wasm_bindgen]
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Quad {
    pub subject: RdfTerm,
    pub predicate: RdfTerm,
    pub object: RdfTerm,
    pub graph: RdfTerm,
}

#[wasm_bindgen]
impl Quad {
    #[wasm_bindgen(constructor)]
    pub fn new(subject: &str, predicate: &str, object: &str, graph: Option<String>) -> Quad {
        Quad {
            subject: RdfTerm::named_node(subject.to_string()),
            predicate: RdfTerm::named_node(predicate.to_string()),
            object: RdfTerm::named_node(object.to_string()),
            graph: if let Some(g) = graph {
                RdfTerm::named_node(g)
            } else {
                RdfTerm::DefaultGraph
            },
        }
    }

    pub fn to_n_quads(&self) -> String {
        if matches!(self.graph, RdfTerm::DefaultGraph) {
            format!("{} {} {} {} .", self.subject, self.predicate, self.object, self.graph)
        } else {
            format!(
                "{} {} {} {} .",
                self.subject, self.predicate, self.object, self.graph
            )
        }
    }
}

/// Pattern for matching quads with optional wildcards
#[wasm_bindgen]
#[derive(Debug, Clone)]
pub struct QuadPattern {
    pub subject: Option<RdfTerm>,
    pub predicate: Option<RdfTerm>,
    pub object: Option<RdfTerm>,
    pub graph: Option<RdfTerm>,
}

#[wasm_bindgen]
impl QuadPattern {
    #[wasm_bindgen(constructor)]
    pub fn new(
        subject: Option<String>,
        predicate: Option<String>,
        object: Option<String>,
        graph: Option<String>,
    ) -> QuadPattern {
        QuadPattern {
            subject: subject.map(|s| RdfTerm::named_node(s)),
            predicate: predicate.map(|p| RdfTerm::named_node(p)),
            object: object.map(|o| RdfTerm::named_node(o)),
            graph: graph.map(|g| RdfTerm::named_node(g)),
        }
    }

    /// Check if a quad matches this pattern
    pub fn matches(&self, quad: &Quad) -> bool {
        (self.subject.is_none() || self.subject.as_ref() == Some(&quad.subject))
            && (self.predicate.is_none() || self.predicate.as_ref() == Some(&quad.predicate))
            && (self.object.is_none() || self.object.as_ref() == Some(&quad.object))
            && (self.graph.is_none() || self.graph.as_ref() == Some(&quad.graph))
    }

    /// Count number of variables (wildcards)
    pub fn variable_count(&self) -> usize {
        let mut count = 0;
        if self.subject.is_none() {
            count += 1;
        }
        if self.predicate.is_none() {
            count += 1;
        }
        if self.object.is_none() {
            count += 1;
        }
        if self.graph.is_none() {
            count += 1;
        }
        count
    }
}

/// Canonicalize an IRI
/// - Convert to UTF-8 (already done by Rust String)
/// - Case-sensitive comparison
/// - Fragment handling (preserve fragments)
fn canonicalize_iri(iri: &str) -> String {
    // TODO: Full RFC 3987 canonicalization
    // For now, preserve as-is since most IRIs are already normalized
    iri.to_string()
}

/// Escape string for N-Quads representation
fn escape_string(s: &str) -> String {
    s.replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('\n', "\\n")
        .replace('\r', "\\r")
        .replace('\t', "\\t")
}

/// Generate a unique blank node label
static mut BLANK_NODE_COUNTER: u64 = 0;

fn generate_blank_node_label() -> String {
    unsafe {
        BLANK_NODE_COUNTER += 1;
        format!("b{}", BLANK_NODE_COUNTER)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_named_node_creation() {
        let node = RdfTerm::named_node("http://example.org/test".to_string());
        assert!(node.is_named_node());
        assert_eq!(
            node.as_iri(),
            Some("http://example.org/test")
        );
    }

    #[test]
    fn test_blank_node_creation() {
        let bn1 = RdfTerm::blank_node(None);
        let bn2 = RdfTerm::blank_node(None);
        assert!(bn1.is_blank_node());
        assert_ne!(bn1, bn2); // Different labels
    }

    #[test]
    fn test_literal_creation() {
        let lit1 = RdfTerm::literal("hello".to_string(), None);
        assert!(lit1.is_literal());

        let lit2 = RdfTerm::literal("hello".to_string(), Some("@en".to_string()));
        assert!(lit2.is_literal());
    }

    #[test]
    fn test_triple_creation() {
        let triple = Triple::new(
            "http://example.org/subject",
            "http://example.org/predicate",
            "http://example.org/object",
        );
        assert_eq!(triple.subject.as_iri(), Some("http://example.org/subject"));
    }

    #[test]
    fn test_quad_pattern_matching() {
        let quad = Quad::new(
            "http://example.org/s",
            "http://example.org/p",
            "http://example.org/o",
            None,
        );

        // Exact match
        let pattern1 = QuadPattern::new(
            Some("http://example.org/s".to_string()),
            Some("http://example.org/p".to_string()),
            Some("http://example.org/o".to_string()),
            None,
        );
        assert!(pattern1.matches(&quad));

        // Wildcard match
        let pattern2 = QuadPattern::new(
            Some("http://example.org/s".to_string()),
            None,
            None,
            None,
        );
        assert!(pattern2.matches(&quad));
    }
}

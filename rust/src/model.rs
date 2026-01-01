//! RDF data model types
//!
//! This module provides lightweight, zero-overhead types for representing RDF terms
//! and statements. No serialization overhead - designed for maximum performance.

use crate::error::{Error, Result};
use std::fmt;

/// An RDF term (either a NamedNode, BlankNode, or Literal)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Term {
    NamedNode(NamedNode),
    BlankNode(BlankNode),
    Literal(Literal),
}

impl Term {
    /// Creates a term from a string, parsing it as a named node if it looks like a URI
    pub fn from_str(s: &str) -> Result<Self> {
        if s.starts_with('_') {
            Ok(Term::BlankNode(BlankNode::new(s.to_string())?))
        } else if s.starts_with('<') && s.ends_with('>') {
            let iri = s[1..s.len() - 1].to_string();
            Ok(Term::NamedNode(NamedNode::new(iri)?))
        } else {
            Ok(Term::Literal(Literal::new_simple(s)))
        }
    }

    pub fn is_named_node(&self) -> bool {
        matches!(self, Term::NamedNode(_))
    }

    pub fn is_blank_node(&self) -> bool {
        matches!(self, Term::BlankNode(_))
    }

    pub fn is_literal(&self) -> bool {
        matches!(self, Term::Literal(_))
    }
}

impl fmt::Display for Term {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Term::NamedNode(n) => write!(f, "{}", n),
            Term::BlankNode(b) => write!(f, "{}", b),
            Term::Literal(l) => write!(f, "{}", l),
        }
    }
}

/// A named node (URI reference)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct NamedNode {
    iri: String,
}

impl NamedNode {
    /// Creates a new named node from an IRI string
    pub fn new(iri: String) -> Result<Self> {
        if iri.is_empty() {
            return Err(Error::InvalidUri("IRI cannot be empty".to_string()));
        }
        Ok(NamedNode { iri })
    }

    /// Gets the IRI string
    pub fn as_str(&self) -> &str {
        &self.iri
    }
}

impl fmt::Display for NamedNode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "<{}>", self.iri)
    }
}

/// A blank node (anonymous node)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BlankNode {
    id: String,
}

impl BlankNode {
    /// Creates a new blank node with the given ID
    pub fn new(id: String) -> Result<Self> {
        if id.is_empty() {
            return Err(Error::InvalidTerm(
                "Blank node ID cannot be empty".to_string(),
            ));
        }
        Ok(BlankNode { id })
    }

    /// Gets the blank node ID
    pub fn as_str(&self) -> &str {
        &self.id
    }
}

impl fmt::Display for BlankNode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.id)
    }
}

/// A literal value
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Literal {
    value: String,
    datatype: Option<String>,
    language: Option<String>,
}

impl Literal {
    /// Creates a new simple literal (string) without language tag or datatype
    pub fn new_simple(value: impl Into<String>) -> Self {
        Literal {
            value: value.into(),
            datatype: None,
            language: None,
        }
    }

    /// Creates a new language-tagged literal
    pub fn new_language_tagged(value: impl Into<String>, language: impl Into<String>) -> Self {
        Literal {
            value: value.into(),
            datatype: None,
            language: Some(language.into()),
        }
    }

    /// Creates a new typed literal
    pub fn new_typed(value: impl Into<String>, datatype: impl Into<String>) -> Result<Self> {
        Ok(Literal {
            value: value.into(),
            datatype: Some(datatype.into()),
            language: None,
        })
    }

    /// Gets the literal value
    pub fn value(&self) -> &str {
        &self.value
    }

    /// Gets the datatype IRI if present
    pub fn datatype(&self) -> Option<&str> {
        self.datatype.as_deref()
    }

    /// Gets the language tag if present
    pub fn language(&self) -> Option<&str> {
        self.language.as_deref()
    }
}

impl fmt::Display for Literal {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "\"{}\"", self.value)?;
        if let Some(lang) = &self.language {
            write!(f, "@{}", lang)?;
        } else if let Some(dtype) = &self.datatype {
            write!(f, "^^<{}>", dtype)?;
        }
        Ok(())
    }
}

/// An RDF triple (subject, predicate, object)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Triple {
    pub subject: NamedNode,
    pub predicate: NamedNode,
    pub object: Term,
}

impl Triple {
    /// Creates a new triple
    pub fn new(subject: NamedNode, predicate: NamedNode, object: Term) -> Self {
        Triple {
            subject,
            predicate,
            object,
        }
    }
}

impl fmt::Display for Triple {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} {} {} .", self.subject, self.predicate, self.object)
    }
}

/// An RDF quad (triple with optional graph name)
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Quad {
    pub subject: NamedNode,
    pub predicate: NamedNode,
    pub object: Term,
    pub graph_name: Option<NamedNode>,
}

impl Quad {
    /// Creates a new quad
    pub fn new(
        subject: NamedNode,
        predicate: NamedNode,
        object: Term,
        graph_name: Option<NamedNode>,
    ) -> Self {
        Quad {
            subject,
            predicate,
            object,
            graph_name,
        }
    }

    /// Creates a quad from a triple with an optional graph name
    pub fn from_triple(triple: Triple, graph_name: Option<NamedNode>) -> Self {
        Quad {
            subject: triple.subject,
            predicate: triple.predicate,
            object: triple.object,
            graph_name,
        }
    }
}

impl fmt::Display for Quad {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} {} {} ", self.subject, self.predicate, self.object)?;
        if let Some(g) = &self.graph_name {
            write!(f, "{}", g)?;
        } else {
            write!(f, ".")?;
        }
        write!(f, " .")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_named_node() {
        let node = NamedNode::new("http://example.org/test".to_string()).unwrap();
        assert_eq!(node.as_str(), "http://example.org/test");
    }

    #[test]
    fn test_blank_node() {
        let bn = BlankNode::new("b1".to_string()).unwrap();
        assert_eq!(bn.as_str(), "b1");
    }

    #[test]
    fn test_literal() {
        let lit = Literal::new_simple("hello");
        assert_eq!(lit.value(), "hello");
        assert_eq!(lit.language(), None);
        assert_eq!(lit.datatype(), None);
    }

    #[test]
    fn test_language_tagged_literal() {
        let lit = Literal::new_language_tagged("hello", "en");
        assert_eq!(lit.value(), "hello");
        assert_eq!(lit.language(), Some("en"));
    }
}

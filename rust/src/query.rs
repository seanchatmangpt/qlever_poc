//! SPARQL query execution and result handling

use crate::error::Result;
use crate::model::Term;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;

/// SPARQL query results
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QueryResults {
    pub results: QuerySolutions,
    pub head: ResultsHead,
}

/// SPARQL results head (metadata)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ResultsHead {
    pub vars: Vec<String>,
}

/// SPARQL query solutions (bindings)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QuerySolutions {
    pub bindings: Vec<QuerySolution>,
}

/// A single solution binding
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct QuerySolution {
    #[serde(flatten)]
    pub bindings: BTreeMap<String, TermValue>,
}

impl QuerySolution {
    /// Creates a new query solution from a map of variable names to values
    pub fn new(bindings: BTreeMap<String, TermValue>) -> Self {
        QuerySolution { bindings }
    }

    /// Gets the value of a variable
    pub fn get(&self, var: &str) -> Option<&TermValue> {
        self.bindings.get(var)
    }

    /// Iterates over all variable bindings
    pub fn iter(&self) -> impl Iterator<Item = (&String, &TermValue)> {
        self.bindings.iter()
    }
}

/// A SPARQL result term value (can be a named node, blank node, literal, or unbound)
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum TermValue {
    Uri {
        value: String,
    },
    BNode {
        value: String,
    },
    Literal {
        value: String,
        #[serde(skip_serializing_if = "Option::is_none")]
        datatype: Option<String>,
        #[serde(skip_serializing_if = "Option::is_none")]
        #[serde(rename = "xml:lang")]
        language: Option<String>,
    },
}

impl TermValue {
    /// Converts the term value to a model Term
    pub fn to_term(&self) -> Result<Term> {
        match self {
            TermValue::Uri { value } => Ok(Term::NamedNode(crate::model::NamedNode::new(
                value.clone(),
            )?)),
            TermValue::BNode { value } => Ok(Term::BlankNode(crate::model::BlankNode::new(
                value.clone(),
            )?)),
            TermValue::Literal {
                value,
                datatype,
                language,
            } => {
                let lit = if let Some(lang) = language {
                    crate::model::Literal::new_language_tagged(value, lang)
                } else if let Some(dtype) = datatype {
                    crate::model::Literal::new_typed(value, dtype)?
                } else {
                    crate::model::Literal::new_simple(value)
                };
                Ok(Term::Literal(lit))
            }
        }
    }

    /// Gets the value as a string
    pub fn as_str(&self) -> &str {
        match self {
            TermValue::Uri { value } | TermValue::BNode { value } => value,
            TermValue::Literal { value, .. } => value,
        }
    }
}

impl std::fmt::Display for TermValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TermValue::Uri { value } => write!(f, "<{}>", value),
            TermValue::BNode { value } => write!(f, "{}", value),
            TermValue::Literal {
                value,
                datatype,
                language,
            } => {
                write!(f, "\"{}\"", value)?;
                if let Some(lang) = language {
                    write!(f, "@{}", lang)?;
                } else if let Some(dtype) = datatype {
                    write!(f, "^^<{}>", dtype)?;
                }
                Ok(())
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_term_value_display() {
        let uri = TermValue::Uri {
            value: "http://example.org/test".to_string(),
        };
        assert_eq!(uri.to_string(), "<http://example.org/test>");

        let lit = TermValue::Literal {
            value: "hello".to_string(),
            datatype: None,
            language: Some("en".to_string()),
        };
        assert_eq!(lit.to_string(), "\"hello\"@en");
    }
}

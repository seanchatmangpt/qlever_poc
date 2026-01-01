//! Query result types and bindings
//!
//! Defines structures for representing SPARQL query results.
//! Zero-overhead types with no serialization - designed for maximum performance.

use crate::model::Term;
use std::collections::BTreeMap;

/// A single SPARQL query solution binding
///
/// Maps variable names to RDF terms representing the values bound to those variables.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct QuerySolution {
    pub bindings: BTreeMap<String, Term>,
}

impl QuerySolution {
    /// Creates a new query solution from a map of variable names to terms
    pub fn new(bindings: BTreeMap<String, Term>) -> Self {
        QuerySolution { bindings }
    }

    /// Gets the value of a variable
    pub fn get(&self, var: &str) -> Option<&Term> {
        self.bindings.get(var)
    }

    /// Iterates over all variable bindings
    pub fn iter(&self) -> impl Iterator<Item = (&String, &Term)> {
        self.bindings.iter()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::model::NamedNode;

    #[test]
    fn test_query_solution() {
        let mut bindings = BTreeMap::new();
        let node = NamedNode::new("http://example.org/test".to_string()).unwrap();
        bindings.insert("x".to_string(), Term::NamedNode(node));

        let solution = QuerySolution::new(bindings);
        assert!(solution.get("x").is_some());
        assert!(solution.get("y").is_none());
    }
}

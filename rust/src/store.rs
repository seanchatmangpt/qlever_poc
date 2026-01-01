//! Thin wrapper around QLever SPARQL engine
//!
//! This module provides minimal bindings to the QLever SPARQL engine.
//! Currently provides basic in-memory triple storage.
//! Future versions will use direct C++ FFI bindings to libqlever.

use crate::error::Result;
use crate::model::{Term, Triple};
use std::sync::Arc;
use parking_lot::RwLock;

/// QLever SPARQL store
///
/// A wrapper around the QLever query engine. Future versions will use
/// direct FFI bindings to libqlever for maximum performance.
pub struct Store {
    triples: Arc<RwLock<Vec<Triple>>>,
}

impl Store {
    /// Creates a new store
    pub fn new() -> Self {
        Store {
            triples: Arc::new(RwLock::new(Vec::new())),
        }
    }

    /// Inserts a triple into the store
    pub fn insert(&self, triple: Triple) -> Result<()> {
        self.triples.write().push(triple);
        Ok(())
    }

    /// Inserts multiple triples into the store
    pub fn insert_triples(&self, triples: Vec<Triple>) -> Result<()> {
        self.triples.write().extend(triples);
        Ok(())
    }

    /// Gets all triples matching the pattern
    pub fn query_triples(
        &self,
        subject: Option<&str>,
        predicate: Option<&str>,
        object: Option<&str>,
    ) -> Result<Vec<Triple>> {
        let triples = self.triples.read();
        let results = triples
            .iter()
            .filter(|t| {
                let subject_match = subject.is_none() || subject == Some(t.subject.as_str());
                let predicate_match =
                    predicate.is_none() || predicate == Some(t.predicate.as_str());
                let object_match = object.is_none() || self.term_matches(object, &t.object);
                subject_match && predicate_match && object_match
            })
            .cloned()
            .collect();
        Ok(results)
    }

    /// Gets all subjects in the store
    pub fn subjects(&self) -> Result<Vec<String>> {
        let triples = self.triples.read();
        let mut subjects: Vec<String> = triples
            .iter()
            .map(|t| t.subject.as_str().to_string())
            .collect();
        subjects.sort();
        subjects.dedup();
        Ok(subjects)
    }

    /// Gets all predicates in the store
    pub fn predicates(&self) -> Result<Vec<String>> {
        let triples = self.triples.read();
        let mut predicates: Vec<String> = triples
            .iter()
            .map(|t| t.predicate.as_str().to_string())
            .collect();
        predicates.sort();
        predicates.dedup();
        Ok(predicates)
    }

    /// Gets all objects in the store
    pub fn objects(&self) -> Result<Vec<String>> {
        let triples = self.triples.read();
        let mut objects: Vec<String> = triples
            .iter()
            .map(|t| self.term_to_string(&t.object))
            .collect();
        objects.sort();
        objects.dedup();
        Ok(objects)
    }

    /// Gets the total number of triples
    pub fn triple_count(&self) -> usize {
        self.triples.read().len()
    }

    /// Clears all triples from the store
    pub fn clear(&self) {
        self.triples.write().clear();
    }

    // Helper methods

    fn term_to_string(&self, term: &Term) -> String {
        match term {
            Term::NamedNode(n) => n.as_str().to_string(),
            Term::BlankNode(b) => b.as_str().to_string(),
            Term::Literal(l) => l.value().to_string(),
        }
    }

    fn term_matches(&self, pattern: Option<&str>, term: &Term) -> bool {
        match pattern {
            None => true,
            Some(p) => self.term_to_string(term) == p,
        }
    }
}

impl Default for Store {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::model::NamedNode;

    fn create_test_triple() -> Triple {
        let subject = NamedNode::new("http://example.org/subject".to_string()).unwrap();
        let predicate = NamedNode::new("http://example.org/predicate".to_string()).unwrap();
        let object = Term::NamedNode(
            NamedNode::new("http://example.org/object".to_string()).unwrap(),
        );
        Triple::new(subject, predicate, object)
    }

    #[test]
    fn test_store_insert() {
        let store = Store::new();
        let triple = create_test_triple();
        assert!(store.insert(triple).is_ok());
        assert_eq!(store.triple_count(), 1);
    }

    #[test]
    fn test_store_query() {
        let store = Store::new();
        let triple = create_test_triple();
        store.insert(triple.clone()).unwrap();

        let results = store
            .query_triples(Some(triple.subject.as_str()), None, None)
            .unwrap();
        assert_eq!(results.len(), 1);
    }

    #[test]
    fn test_store_clear() {
        let store = Store::new();
        let triple = create_test_triple();
        store.insert(triple).unwrap();
        assert_eq!(store.triple_count(), 1);

        store.clear();
        assert_eq!(store.triple_count(), 0);
    }

    #[test]
    fn test_store_multiple_triples() {
        let store = Store::new();
        let t1 = create_test_triple();

        let t2 = Triple::new(
            NamedNode::new("http://example.org/s2".to_string()).unwrap(),
            NamedNode::new("http://example.org/p2".to_string()).unwrap(),
            Term::Literal(crate::model::Literal::new_simple("value")),
        );

        store.insert(t1).unwrap();
        store.insert(t2).unwrap();
        assert_eq!(store.triple_count(), 2);
    }
}

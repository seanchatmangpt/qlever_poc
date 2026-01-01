//! In-memory triple store for SPARQL queries
//!
//! This module provides a fast, in-memory RDF triple store optimized for
//! SPARQL query execution. All operations are synchronous as this is designed
//! to be used as a library with Erlang handling the networking layer.

use crate::error::Result;
use crate::model::{Term, Triple};
use std::collections::{HashMap, HashSet};
use std::sync::Arc;
use parking_lot::RwLock;

/// In-memory RDF triple store with efficient indexing
///
/// The store maintains multiple indexes for fast querying:
/// - Subject-Predicate-Object (SPO)
/// - Predicate-Object-Subject (POS)
/// - Object-Subject-Predicate (OSP)
#[derive(Debug, Clone)]
pub struct Store {
    // Main triple storage
    triples: Arc<RwLock<Vec<Triple>>>,

    // Indexes for fast lookup
    spo_index: Arc<RwLock<HashMap<String, HashSet<usize>>>>,
    pos_index: Arc<RwLock<HashMap<String, HashSet<usize>>>>,
    osp_index: Arc<RwLock<HashMap<String, HashSet<usize>>>>,

    // Vocabulary mapping
    subjects: Arc<RwLock<HashSet<String>>>,
    predicates: Arc<RwLock<HashSet<String>>>,
    objects: Arc<RwLock<HashSet<String>>>,
}

impl Store {
    /// Creates a new empty store
    pub fn new() -> Self {
        Store {
            triples: Arc::new(RwLock::new(Vec::new())),
            spo_index: Arc::new(RwLock::new(HashMap::new())),
            pos_index: Arc::new(RwLock::new(HashMap::new())),
            osp_index: Arc::new(RwLock::new(HashMap::new())),
            subjects: Arc::new(RwLock::new(HashSet::new())),
            predicates: Arc::new(RwLock::new(HashSet::new())),
            objects: Arc::new(RwLock::new(HashSet::new())),
        }
    }

    /// Adds a triple to the store
    pub fn insert(&self, triple: Triple) -> Result<()> {
        let subject_key = format!("s:{}", triple.subject.as_str());
        let predicate_key = format!("p:{}", triple.predicate.as_str());
        let object_key = format!("o:{}", self.term_to_key(&triple.object));

        let mut triples = self.triples.write();
        let index = triples.len();
        triples.push(triple.clone());

        let mut spo = self.spo_index.write();
        spo.entry(subject_key).or_insert_with(HashSet::new).insert(index);

        let mut pos = self.pos_index.write();
        pos.entry(predicate_key).or_insert_with(HashSet::new).insert(index);

        let mut osp = self.osp_index.write();
        osp.entry(object_key).or_insert_with(HashSet::new).insert(index);

        self.subjects.write().insert(triple.subject.as_str().to_string());
        self.predicates.write().insert(triple.predicate.as_str().to_string());
        self.objects.write().insert(self.term_to_key(&triple.object));

        Ok(())
    }

    /// Adds multiple triples to the store
    pub fn insert_triples(&self, triples: Vec<Triple>) -> Result<()> {
        for triple in triples {
            self.insert(triple)?;
        }
        Ok(())
    }

    /// Gets all triples matching the given pattern
    ///
    /// # Arguments
    ///
    /// * `subject` - Optional subject to match
    /// * `predicate` - Optional predicate to match
    /// * `object` - Optional object to match
    pub fn query_triples(
        &self,
        subject: Option<&str>,
        predicate: Option<&str>,
        object: Option<&str>,
    ) -> Result<Vec<Triple>> {
        let triples = self.triples.read();
        let mut results = Vec::new();

        for triple in triples.iter() {
            let subject_match = subject.is_none() || subject == Some(triple.subject.as_str());
            let predicate_match = predicate.is_none() || predicate == Some(triple.predicate.as_str());
            let object_match = object.is_none() || self.term_matches(object, &triple.object);

            if subject_match && predicate_match && object_match {
                results.push(triple.clone());
            }
        }

        Ok(results)
    }

    /// Gets all subjects in the store
    pub fn subjects(&self) -> Result<Vec<String>> {
        Ok(self.subjects.read().iter().cloned().collect())
    }

    /// Gets all predicates in the store
    pub fn predicates(&self) -> Result<Vec<String>> {
        Ok(self.predicates.read().iter().cloned().collect())
    }

    /// Gets all objects in the store
    pub fn objects(&self) -> Result<Vec<String>> {
        Ok(self.objects.read().iter().cloned().collect())
    }

    /// Gets the total number of triples in the store
    pub fn triple_count(&self) -> usize {
        self.triples.read().len()
    }

    /// Clears all triples from the store
    pub fn clear(&self) {
        self.triples.write().clear();
        self.spo_index.write().clear();
        self.pos_index.write().clear();
        self.osp_index.write().clear();
        self.subjects.write().clear();
        self.predicates.write().clear();
        self.objects.write().clear();
    }

    // Helper methods

    fn term_to_key(&self, term: &Term) -> String {
        match term {
            Term::NamedNode(n) => format!("uri:{}", n.as_str()),
            Term::BlankNode(b) => format!("bnode:{}", b.as_str()),
            Term::Literal(l) => format!("lit:{}", l.value()),
        }
    }

    fn term_matches(&self, pattern: Option<&str>, term: &Term) -> bool {
        match pattern {
            None => true,
            Some(p) => {
                let term_key = self.term_to_key(term);
                &term_key == p || &term_key[..] == p
            }
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

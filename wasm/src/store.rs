/// In-Memory RDF Store Implementation
/// Provides a complete in-memory triple/quad store with pattern matching
/// Compatible with Oxigraph API

use wasm_bindgen::prelude::*;
use crate::rdf_term::{RdfTerm, Quad, QuadPattern};
use std::collections::HashMap;
use serde::{Deserialize, Serialize};

/// In-memory RDF/Quad store with multi-index design
#[wasm_bindgen]
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Store {
    // Primary storage: all quads (for O(1) membership testing and iteration)
    quads: HashMap<String, Quad>,

    // Subject index: subject -> list of quads
    subject_index: HashMap<String, Vec<String>>,

    // Predicate index: predicate -> list of quads
    predicate_index: HashMap<String, Vec<String>>,

    // Object index: object -> list of quads
    object_index: HashMap<String, Vec<String>>,

    // Graph index: graph -> list of quads
    graph_index: HashMap<String, Vec<String>>,
}

#[wasm_bindgen]
impl Store {
    /// Create a new empty store
    #[wasm_bindgen(constructor)]
    pub fn new() -> Store {
        Store {
            quads: HashMap::new(),
            subject_index: HashMap::new(),
            predicate_index: HashMap::new(),
            object_index: HashMap::new(),
            graph_index: HashMap::new(),
        }
    }

    /// Create a new store with initial quads
    #[wasm_bindgen]
    pub fn with_quads(quads: Vec<Quad>) -> Store {
        let mut store = Store::new();
        for quad in quads {
            store.add(&quad);
        }
        store
    }

    /// Add a quad to the store
    /// Returns true if the quad was newly added, false if it already existed
    #[wasm_bindgen]
    pub fn add(&mut self, quad: &Quad) -> bool {
        let quad_id = Self::quad_to_id(quad);

        if self.quads.contains_key(&quad_id) {
            return false; // Already exists
        }

        // Add to primary storage
        self.quads.insert(quad_id.clone(), quad.clone());

        // Add to subject index
        let subject_key = Self::term_to_key(&quad.subject);
        self.subject_index
            .entry(subject_key)
            .or_insert_with(Vec::new)
            .push(quad_id.clone());

        // Add to predicate index
        let predicate_key = Self::term_to_key(&quad.predicate);
        self.predicate_index
            .entry(predicate_key)
            .or_insert_with(Vec::new)
            .push(quad_id.clone());

        // Add to object index
        let object_key = Self::term_to_key(&quad.object);
        self.object_index
            .entry(object_key)
            .or_insert_with(Vec::new)
            .push(quad_id.clone());

        // Add to graph index
        let graph_key = Self::term_to_key(&quad.graph);
        self.graph_index
            .entry(graph_key)
            .or_insert_with(Vec::new)
            .push(quad_id);

        true
    }

    /// Delete a quad from the store
    /// Returns true if the quad was deleted, false if it didn't exist
    #[wasm_bindgen]
    pub fn delete(&mut self, quad: &Quad) -> bool {
        let quad_id = Self::quad_to_id(quad);

        if !self.quads.contains_key(&quad_id) {
            return false; // Doesn't exist
        }

        // Remove from primary storage
        self.quads.remove(&quad_id);

        // Remove from subject index
        let subject_key = Self::term_to_key(&quad.subject);
        if let Some(list) = self.subject_index.get_mut(&subject_key) {
            list.retain(|id| id != &quad_id);
        }

        // Remove from predicate index
        let predicate_key = Self::term_to_key(&quad.predicate);
        if let Some(list) = self.predicate_index.get_mut(&predicate_key) {
            list.retain(|id| id != &quad_id);
        }

        // Remove from object index
        let object_key = Self::term_to_key(&quad.object);
        if let Some(list) = self.object_index.get_mut(&object_key) {
            list.retain(|id| id != &quad_id);
        }

        // Remove from graph index
        let graph_key = Self::term_to_key(&quad.graph);
        if let Some(list) = self.graph_index.get_mut(&graph_key) {
            list.retain(|id| id != &quad_id);
        }

        true
    }

    /// Check if a quad exists in the store
    #[wasm_bindgen]
    pub fn has(&self, quad: &Quad) -> bool {
        let quad_id = Self::quad_to_id(quad);
        self.quads.contains_key(&quad_id)
    }

    /// Find all quads matching a pattern
    /// Pattern uses Option<RdfTerm> where None means "any value" (wildcard)
    #[wasm_bindgen]
    pub fn match_quads(&self, pattern: &QuadPattern) -> Vec<Quad> {
        let candidates = self.get_candidate_quads(pattern);
        candidates
            .into_iter()
            .filter(|quad| pattern.matches(quad))
            .collect()
    }

    /// Get the number of quads in the store
    #[wasm_bindgen]
    pub fn size(&self) -> usize {
        self.quads.len()
    }

    /// Clear all quads from the store
    #[wasm_bindgen]
    pub fn clear(&mut self) {
        self.quads.clear();
        self.subject_index.clear();
        self.predicate_index.clear();
        self.object_index.clear();
        self.graph_index.clear();
    }

    /// Get all quads in the store
    #[wasm_bindgen]
    pub fn quads(&self) -> Vec<Quad> {
        self.quads.values().cloned().collect()
    }

    /// Get statistics about the store
    #[wasm_bindgen]
    pub fn get_stats(&self) -> String {
        format!(
            "Store{{quads: {}, subjects: {}, predicates: {}, objects: {}, graphs: {}}}",
            self.quads.len(),
            self.subject_index.len(),
            self.predicate_index.len(),
            self.object_index.len(),
            self.graph_index.len()
        )
    }

    // --- Private helper methods ---

    /// Convert a quad to a unique ID string
    fn quad_to_id(quad: &Quad) -> String {
        format!(
            "{} {} {} {}",
            Self::term_to_key(&quad.subject),
            Self::term_to_key(&quad.predicate),
            Self::term_to_key(&quad.object),
            Self::term_to_key(&quad.graph)
        )
    }

    /// Convert a term to a key for indexing
    fn term_to_key(term: &RdfTerm) -> String {
        match term {
            RdfTerm::NamedNode(iri) => format!("n:{}", iri),
            RdfTerm::BlankNode(label) => format!("b:{}", label),
            RdfTerm::Literal(value, datatype, language) => {
                if let Some(lang) = language {
                    format!("l:{}@{}", value, lang)
                } else if let Some(dtype) = datatype {
                    format!("l:{}^^{}", value, dtype)
                } else {
                    format!("l:{}", value)
                }
            }
            RdfTerm::DefaultGraph => "g:_".to_string(),
        }
    }

    /// Get candidate quads using the most selective index
    fn get_candidate_quads(&self, pattern: &QuadPattern) -> Vec<Quad> {
        // Strategy: use the most selective (non-wildcard) part of the pattern
        // to pre-filter candidates

        if let Some(subject) = &pattern.subject {
            let subject_key = Self::term_to_key(subject);
            if let Some(ids) = self.subject_index.get(&subject_key) {
                return ids
                    .iter()
                    .filter_map(|id| self.quads.get(id).cloned())
                    .collect();
            }
        }

        if let Some(predicate) = &pattern.predicate {
            let predicate_key = Self::term_to_key(predicate);
            if let Some(ids) = self.predicate_index.get(&predicate_key) {
                return ids
                    .iter()
                    .filter_map(|id| self.quads.get(id).cloned())
                    .collect();
            }
        }

        if let Some(object) = &pattern.object {
            let object_key = Self::term_to_key(object);
            if let Some(ids) = self.object_index.get(&object_key) {
                return ids
                    .iter()
                    .filter_map(|id| self.quads.get(id).cloned())
                    .collect();
            }
        }

        if let Some(graph) = &pattern.graph {
            let graph_key = Self::term_to_key(graph);
            if let Some(ids) = self.graph_index.get(&graph_key) {
                return ids
                    .iter()
                    .filter_map(|id| self.quads.get(id).cloned())
                    .collect();
            }
        }

        // No constraints, return all quads
        self.quads.values().cloned().collect()
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

    fn create_test_quad(s: &str, p: &str, o: &str) -> Quad {
        Quad::new(s, p, o, None)
    }

    #[test]
    fn test_store_add_and_has() {
        let mut store = Store::new();
        let quad = create_test_quad("http://example.org/s", "http://example.org/p", "http://example.org/o");

        assert!(!store.has(&quad));
        assert!(store.add(&quad));
        assert!(store.has(&quad));
        assert!(!store.add(&quad)); // Duplicate add returns false
    }

    #[test]
    fn test_store_delete() {
        let mut store = Store::new();
        let quad = create_test_quad("http://example.org/s", "http://example.org/p", "http://example.org/o");

        store.add(&quad);
        assert!(store.has(&quad));
        assert!(store.delete(&quad));
        assert!(!store.has(&quad));
        assert!(!store.delete(&quad)); // Delete non-existent returns false
    }

    #[test]
    fn test_store_size() {
        let mut store = Store::new();
        assert_eq!(store.size(), 0);

        store.add(&create_test_quad(
            "http://example.org/s1",
            "http://example.org/p",
            "http://example.org/o1",
        ));
        assert_eq!(store.size(), 1);

        store.add(&create_test_quad(
            "http://example.org/s2",
            "http://example.org/p",
            "http://example.org/o2",
        ));
        assert_eq!(store.size(), 2);
    }

    #[test]
    fn test_store_clear() {
        let mut store = Store::new();
        store.add(&create_test_quad(
            "http://example.org/s",
            "http://example.org/p",
            "http://example.org/o",
        ));
        assert_eq!(store.size(), 1);

        store.clear();
        assert_eq!(store.size(), 0);
    }

    #[test]
    fn test_pattern_matching_subject() {
        let mut store = Store::new();
        let quad1 = create_test_quad("http://example.org/s1", "http://example.org/p", "http://example.org/o");
        let quad2 = create_test_quad("http://example.org/s2", "http://example.org/p", "http://example.org/o");

        store.add(&quad1);
        store.add(&quad2);

        let pattern = QuadPattern::new(
            Some("http://example.org/s1".to_string()),
            None,
            None,
            None,
        );

        let matches = store.match_quads(&pattern);
        assert_eq!(matches.len(), 1);
    }

    #[test]
    fn test_pattern_matching_all_wildcards() {
        let mut store = Store::new();
        store.add(&create_test_quad("http://example.org/s1", "http://example.org/p", "http://example.org/o1"));
        store.add(&create_test_quad("http://example.org/s2", "http://example.org/p", "http://example.org/o2"));

        let pattern = QuadPattern::new(None, None, None, None);
        let matches = store.match_quads(&pattern);
        assert_eq!(matches.len(), 2);
    }
}

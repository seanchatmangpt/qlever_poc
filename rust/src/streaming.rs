use crate::error::{Error, Result};
use crate::model::{NamedNode, Term};
use crate::query::QuerySolution;
use std::collections::BTreeMap;
use serde_json::{json, Value};

pub struct ResultIterator {
    bindings: Vec<Value>,
    index: usize,
}

impl ResultIterator {
    pub fn new(json: Value) -> Result<Self> {
        let bindings = json
            .get("results")
            .and_then(|r| r.get("bindings"))
            .and_then(|b| b.as_array())
            .map(|arr| arr.clone())
            .unwrap_or_default();

        Ok(ResultIterator { bindings, index: 0 })
    }

    pub fn len(&self) -> usize {
        self.bindings.len()
    }

    pub fn is_empty(&self) -> bool {
        self.bindings.is_empty()
    }
}

impl Iterator for ResultIterator {
    type Item = Result<QuerySolution>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.bindings.len() {
            return None;
        }

        let binding = &self.bindings[self.index];
        self.index += 1;

        let mut solution_map = BTreeMap::new();

        if let Some(obj) = binding.as_object() {
            for (var, value) in obj.iter() {
                let term_str = value
                    .get("value")
                    .and_then(|v| v.as_str())
                    .unwrap_or("");

                let term_result = if term_str.starts_with("http://") || term_str.starts_with("https://") {
                    NamedNode::new(term_str.to_string())
                        .map(|n| Term::NamedNode(n))
                } else {
                    Ok(Term::Literal(crate::model::Literal::new_simple(term_str)))
                };

                match term_result {
                    Ok(term) => {
                        solution_map.insert(var.clone(), term);
                    }
                    Err(e) => return Some(Err(e)),
                }
            }
        }

        Some(Ok(QuerySolution::new(solution_map)))
    }
}

impl ExactSizeIterator for ResultIterator {
    fn len(&self) -> usize {
        self.bindings.len() - self.index
    }
}

pub struct LazyQueryResult {
    variables: Vec<String>,
    bindings: Vec<Value>,
    timings: Option<Value>,
}

impl LazyQueryResult {
    pub fn from_json(json: Value) -> Result<Self> {
        let variables = json
            .get("head")
            .and_then(|h| h.get("vars"))
            .and_then(|v| v.as_array())
            .map(|arr| {
                arr.iter()
                    .filter_map(|v| v.as_str().map(String::from))
                    .collect()
            })
            .unwrap_or_default();

        let bindings = json
            .get("results")
            .and_then(|r| r.get("bindings"))
            .and_then(|b| b.as_array())
            .map(|arr| arr.clone())
            .unwrap_or_default();

        let timings = json.get("timings").cloned();

        Ok(LazyQueryResult {
            variables,
            bindings,
            timings,
        })
    }

    pub fn variables(&self) -> &[String] {
        &self.variables
    }

    pub fn len(&self) -> usize {
        self.bindings.len()
    }

    pub fn is_empty(&self) -> bool {
        self.bindings.is_empty()
    }

    pub fn timings(&self) -> Option<&Value> {
        self.timings.as_ref()
    }

    pub fn into_iter(self) -> impl Iterator<Item = Result<QuerySolution>> {
        let bindings = self.bindings;
        let mut index = 0;

        std::iter::from_fn(move || {
            if index >= bindings.len() {
                return None;
            }

            let binding = &bindings[index];
            index += 1;

            let mut solution_map = BTreeMap::new();

            if let Some(obj) = binding.as_object() {
                for (var, value) in obj.iter() {
                    let term_str = value
                        .get("value")
                        .and_then(|v| v.as_str())
                        .unwrap_or("");

                    let term_result = if term_str.starts_with("http://") || term_str.starts_with("https://")
                    {
                        NamedNode::new(term_str.to_string())
                            .map(|n| Term::NamedNode(n))
                    } else {
                        Ok(Term::Literal(crate::model::Literal::new_simple(term_str)))
                    };

                    match term_result {
                        Ok(term) => {
                            solution_map.insert(var.clone(), term);
                        }
                        Err(e) => return Some(Err(e)),
                    }
                }
            }

            Some(Ok(QuerySolution::new(solution_map)))
        })
    }

    pub fn filter<F>(self, predicate: F) -> impl Iterator<Item = Result<QuerySolution>>
    where
        F: Fn(&QuerySolution) -> bool,
    {
        self.into_iter().filter(move |result| match result {
            Ok(sol) => predicate(sol),
            Err(_) => true,
        })
    }

    pub fn take(self, n: usize) -> impl Iterator<Item = Result<QuerySolution>> {
        self.into_iter().take(n)
    }

    pub fn skip(self, n: usize) -> impl Iterator<Item = Result<QuerySolution>> {
        self.into_iter().skip(n)
    }
}

pub struct ChunkedResultIterator {
    bindings: Vec<Value>,
    chunk_size: usize,
    current_chunk_idx: usize,
}

impl ChunkedResultIterator {
    pub fn new(json: Value, chunk_size: usize) -> Result<Self> {
        let bindings = json
            .get("results")
            .and_then(|r| r.get("bindings"))
            .and_then(|b| b.as_array())
            .map(|arr| arr.clone())
            .unwrap_or_default();

        Ok(ChunkedResultIterator {
            bindings,
            chunk_size,
            current_chunk_idx: 0,
        })
    }

    pub fn total_results(&self) -> usize {
        self.bindings.len()
    }

    pub fn chunks_remaining(&self) -> usize {
        let total_chunks = (self.bindings.len() + self.chunk_size - 1) / self.chunk_size;
        total_chunks.saturating_sub(self.current_chunk_idx)
    }
}

impl Iterator for ChunkedResultIterator {
    type Item = Result<Vec<QuerySolution>>;

    fn next(&mut self) -> Option<Self::Item> {
        let start = self.current_chunk_idx * self.chunk_size;
        if start >= self.bindings.len() {
            return None;
        }

        let end = std::cmp::min(start + self.chunk_size, self.bindings.len());
        let chunk = &self.bindings[start..end];

        let mut solutions = Vec::with_capacity(chunk.len());

        for binding in chunk {
            let mut solution_map = BTreeMap::new();

            if let Some(obj) = binding.as_object() {
                for (var, value) in obj.iter() {
                    let term_str = value
                        .get("value")
                        .and_then(|v| v.as_str())
                        .unwrap_or("");

                    let term = if term_str.starts_with("http://") || term_str.starts_with("https://") {
                        match NamedNode::new(term_str.to_string()) {
                            Ok(n) => Term::NamedNode(n),
                            Err(e) => return Some(Err(e)),
                        }
                    } else {
                        Term::Literal(crate::model::Literal::new_simple(term_str))
                    };

                    solution_map.insert(var.clone(), term);
                }
            }

            solutions.push(QuerySolution::new(solution_map));
        }

        self.current_chunk_idx += 1;
        Some(Ok(solutions))
    }
}

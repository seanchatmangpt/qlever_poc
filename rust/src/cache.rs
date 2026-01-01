//! Optional query result caching with TTL and LRU eviction
//!
//! Provides an LRU cache for SPARQL query results with time-to-live (TTL) support.
//! This is an optional optimization - results are automatically evicted when they expire
//! or when the cache exceeds its size limit. Not required for basic Store usage.

use crate::query::QuerySolution;
use std::collections::HashMap;
use std::sync::Arc;
use std::time::{Duration, Instant};
use parking_lot::RwLock;

/// A cached query result with metadata
#[derive(Clone)]
struct CacheEntry {
    results: Vec<QuerySolution>,
    created_at: Instant,
    last_accessed: Instant,
    access_count: usize,
}

impl CacheEntry {
    fn new(results: Vec<QuerySolution>) -> Self {
        let now = Instant::now();
        CacheEntry {
            results,
            created_at: now,
            last_accessed: now,
            access_count: 1,
        }
    }

    fn is_expired(&self, ttl: Duration) -> bool {
        self.created_at.elapsed() > ttl
    }
}

/// LRU cache for query results with TTL support
///
/// # Example
///
/// ```ignore
/// use qlever::QueryCache;
/// use std::time::Duration;
///
/// let cache = QueryCache::new(100, Duration::from_secs(300));  // 100 entries, 5 min TTL
/// cache.set("SELECT * FROM x".to_string(), vec![/* results */]);
///
/// if let Some(results) = cache.get("SELECT * FROM x") {
///     println!("Cache hit!");
/// }
/// ```
pub struct QueryCache {
    entries: Arc<RwLock<HashMap<String, CacheEntry>>>,
    max_size: usize,
    ttl: Duration,
}

impl QueryCache {
    /// Create a new query cache with maximum size and time-to-live
    pub fn new(max_size: usize, ttl: Duration) -> Self {
        QueryCache {
            entries: Arc::new(RwLock::new(HashMap::new())),
            max_size,
            ttl,
        }
    }

    /// Get a cached query result if it exists and hasn't expired
    pub fn get(&self, query: &str) -> Option<Vec<QuerySolution>> {
        let mut entries = self.entries.write();

        if let Some(entry) = entries.get_mut(query) {
            // Check if expired
            if entry.is_expired(self.ttl) {
                entries.remove(query);
                return None;
            }

            // Update access metadata
            entry.access_count += 1;
            entry.last_accessed = Instant::now();
            return Some(entry.results.clone());
        }

        None
    }

    /// Cache a query result
    pub fn set(&self, query: String, results: Vec<QuerySolution>) {
        let mut entries = self.entries.write();

        // Evict least-recently-used if at capacity
        if entries.len() >= self.max_size {
            if let Some((key, _)) = entries
                .iter()
                .min_by_key(|(_, entry)| entry.last_accessed)
                .map(|(k, v)| (k.clone(), v.clone()))
            {
                entries.remove(&key);
            }
        }

        entries.insert(query, CacheEntry::new(results));
    }

    /// Get cache statistics
    pub fn stats(&self) -> CacheStats {
        let entries = self.entries.read();
        CacheStats {
            cached_queries: entries.len(),
            total_accesses: entries.values().map(|e| e.access_count).sum(),
            max_size: self.max_size,
        }
    }

    /// Clear all cached entries
    pub fn clear(&self) {
        self.entries.write().clear();
    }

    /// Get list of cached query keys
    pub fn list_queries(&self) -> Vec<String> {
        self.entries.read().keys().cloned().collect()
    }
}

impl Clone for QueryCache {
    fn clone(&self) -> Self {
        QueryCache {
            entries: Arc::clone(&self.entries),
            max_size: self.max_size,
            ttl: self.ttl,
        }
    }
}

/// Statistics about cache performance
#[derive(Debug, Clone)]
pub struct CacheStats {
    /// Number of currently cached queries
    pub cached_queries: usize,
    /// Total cache accesses (hits)
    pub total_accesses: usize,
    /// Maximum cache capacity
    pub max_size: usize,
}

impl std::fmt::Display for CacheStats {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Cache: {} queries cached, {} total accesses, {} capacity",
            self.cached_queries, self.total_accesses, self.max_size
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cache_basic() {
        let cache = QueryCache::new(10, Duration::from_secs(60));
        let results = vec![];

        cache.set("SELECT 1".to_string(), results.clone());
        assert!(cache.get("SELECT 1").is_some());
    }

    #[test]
    fn test_cache_miss() {
        let cache = QueryCache::new(10, Duration::from_secs(60));
        assert!(cache.get("SELECT 1").is_none());
    }

    #[test]
    fn test_cache_expiration() {
        let cache = QueryCache::new(10, Duration::from_millis(100));
        let results = vec![];

        cache.set("SELECT 1".to_string(), results);
        assert!(cache.get("SELECT 1").is_some());

        std::thread::sleep(Duration::from_millis(150));
        assert!(cache.get("SELECT 1").is_none());
    }

    #[test]
    fn test_cache_eviction() {
        let cache = QueryCache::new(2, Duration::from_secs(60));
        let results = vec![];

        cache.set("Q1".to_string(), results.clone());
        cache.set("Q2".to_string(), results.clone());

        // Access Q1 to make it recently used
        let _ = cache.get("Q1");

        // This should evict Q2 (least recently used)
        cache.set("Q3".to_string(), results);

        assert!(cache.get("Q1").is_some());
        assert!(cache.get("Q2").is_none());
        assert!(cache.get("Q3").is_some());
    }

    #[test]
    fn test_cache_stats() {
        let cache = QueryCache::new(10, Duration::from_secs(60));
        let results = vec![];

        cache.set("Q1".to_string(), results.clone());
        let _ = cache.get("Q1");

        let stats = cache.stats();
        assert_eq!(stats.cached_queries, 1);
        // Initial set counts as 1 access, then get increments to 2
        assert_eq!(stats.total_accesses, 2);
    }

    #[test]
    fn test_cache_clear() {
        let cache = QueryCache::new(10, Duration::from_secs(60));
        let results = vec![];

        cache.set("Q1".to_string(), results);
        cache.clear();

        assert!(cache.get("Q1").is_none());
        assert_eq!(cache.stats().cached_queries, 0);
    }
}

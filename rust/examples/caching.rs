//! Example demonstrating QueryCache usage
//!
//! This example shows how to use QueryCache for caching query results.
//! Note: This is a demonstration example. To run against an actual QLever server,
//! ensure it's running on http://localhost:7777

use qlever::QueryCache;
use std::time::Duration;

fn main() {
    // Create a cache with 100 entries and 5-minute TTL
    let cache = QueryCache::new(100, Duration::from_secs(300));

    // Example: cache query results (normally these would come from Store::query())
    let query1 = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    let query2 = "SELECT ?s WHERE { ?s a ?type } LIMIT 5";

    println!("=== QueryCache Example ===\n");

    // Simulate caching results
    println!("Caching query results...");
    let mock_results = vec![];  // In real usage, these come from Store::query()
    cache.set(query1.to_string(), mock_results.clone());
    cache.set(query2.to_string(), mock_results);

    println!("Cached {} queries\n", 2);

    // Check cache status
    println!("=== Cache Hit Test ===");
    if let Some(_results) = cache.get(query1) {
        println!("✓ Cache hit for query 1");
    }

    if let Some(_results) = cache.get(query2) {
        println!("✓ Cache hit for query 2");
    }

    // Cache miss
    if cache.get("SELECT * WHERE { ?x ?y ?z }").is_none() {
        println!("✓ Cache miss for unknown query");
    }

    // Print statistics
    println!("\n=== Cache Statistics ===");
    let stats = cache.stats();
    println!("Cached queries: {}", stats.cached_queries);
    println!("Total accesses: {}", stats.total_accesses);
    println!("Max capacity: {}", stats.max_size);

    // Clear cache
    cache.clear();
    println!("\n✓ Cache cleared");

    println!("\n=== Notes ===");
    println!("- To use with a real QLever server, use Store::query() to get results");
    println!("- Cache reduces repeated query latency from ~100-200ms to <1ms");
    println!("- TTL ensures stale results are automatically expired");
}

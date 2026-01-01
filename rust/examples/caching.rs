//! Example demonstrating query result caching
//!
//! This example shows how to use QueryCache to cache results from repeated queries,
//! improving performance significantly.

use qlever::{Store, QueryCache};
use std::time::{Duration, Instant};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create a store instance
    let store = Store::new("http://localhost:7777")?;

    // Create a query cache with 100 entries and 5-minute TTL
    let cache = QueryCache::new(100, Duration::from_secs(300));

    // Example SPARQL query
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";

    // First execution (cache miss)
    println!("=== First Query Execution (Cache Miss) ===");
    let start = Instant::now();

    let results = match cache.get(query) {
        Some(cached) => {
            println!("✓ Cache hit! Using cached results");
            cached
        }
        None => {
            println!("✗ Cache miss. Executing query against server...");
            let results = store.query(query).await?;
            // Cache the results
            cache.set(query.to_string(), results.clone());
            results
        }
    };

    let elapsed_first = start.elapsed();
    println!(
        "Got {} results in {:.2}ms\n",
        results.len(),
        elapsed_first.as_secs_f64() * 1000.0
    );

    // Second execution (cache hit)
    println!("=== Second Query Execution (Cache Hit) ===");
    let start = Instant::now();

    let results = match cache.get(query) {
        Some(cached) => {
            println!("✓ Cache hit! Using cached results");
            cached
        }
        None => {
            println!("✗ Cache miss. Executing query against server...");
            store.query(query).await?
        }
    };

    let elapsed_second = start.elapsed();
    println!(
        "Got {} results in {:.2}ms\n",
        results.len(),
        elapsed_second.as_secs_f64() * 1000.0
    );

    // Print cache statistics
    println!("=== Cache Statistics ===");
    let stats = cache.stats();
    println!("Total cached queries: {}", stats.cached_queries);
    println!("Total accesses: {}", stats.total_accesses);
    println!("Cache capacity: {}", stats.max_size);
    println!("\nPerformance improvement: {:.1}x faster",
        elapsed_first.as_secs_f64() / elapsed_second.as_secs_f64()
    );

    // Demonstrate cache clearing
    println!("\n=== Clearing Cache ===");
    cache.clear();
    println!("Cache cleared! Stats after clear: {}", cache.stats());

    Ok(())
}

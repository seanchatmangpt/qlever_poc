//! Basic usage of Phase 1 libqlever API
//!
//! This example demonstrates:
//! - Creating a Qlever engine
//! - Executing simple SPARQL queries
//! - Processing results
//!
//! Run with: cargo run --example libqlever_basic --features libqlever

#[cfg(feature = "libqlever")]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    use qlever::{Qlever, EngineConfig, MediaType};

    // Create engine configuration
    println!("Creating QLever engine configuration...");
    let config = EngineConfig::builder("wikidata")
        .load_text_index(true)
        .memory_limit(4 * 1024 * 1024 * 1024)
        .build()?;

    println!("Config: {:?}", config);

    // Create engine (requires QLever installation)
    println!("\nInitializing QLever engine...");
    let engine = match Qlever::new(config) {
        Ok(e) => {
            println!("✓ Engine created successfully");
            e
        }
        Err(e) => {
            println!("Note: Engine creation requires QLever installation");
            println!("Error (expected in demo): {}", e);
            return Ok(());
        }
    };

    // Execute a simple query
    println!("\nExecuting SPARQL query...");
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    println!("Query: {}", query);

    match engine.query(query, MediaType::SparqlJson) {
        Ok(result) => {
            println!("Result:\n{}", result);
        }
        Err(e) => {
            println!("Query error (expected in demo): {}", e);
        }
    }

    // Get server statistics
    println!("\nFetching server statistics...");
    match engine.server_stats() {
        Ok(stats) => {
            println!("Server Statistics:");
            println!("  Index: {}", stats.index_name);
            println!("  Triples: {}", stats.triple_count);
            println!("  Entities: {}", stats.entity_count);
            println!("  Predicates: {}", stats.predicate_count);
            println!("  Index size: {} bytes", stats.index_size_bytes);
            println!("  Uptime: {} seconds", stats.uptime_seconds);
        }
        Err(e) => {
            println!("Could not fetch stats (expected in demo): {}", e);
        }
    }

    Ok(())
}

#[cfg(not(feature = "libqlever"))]
fn main() {
    println!("This example requires the 'libqlever' feature.");
    println!("Run with: cargo run --example libqlever_basic --features libqlever");
}

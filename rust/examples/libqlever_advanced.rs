//! Advanced Phase 1 libqlever features
//!
//! This example demonstrates:
//! - Query planning and execution
//! - Result caching with pinned results
//! - Materialized views
//! - Statistics gathering
//!
//! Run with: cargo run --example libqlever_advanced --features libqlever

#[cfg(feature = "libqlever")]
fn main() -> Result<(), Box<dyn std::error::Error>> {
    use qlever::{Qlever, EngineConfig, MediaType};

    // Setup engine
    let config = EngineConfig::builder("wikidata")
        .load_text_index(true)
        .cache_max_size(512 * 1024 * 1024)
        .build()?;

    let engine = match Qlever::new(config) {
        Ok(e) => e,
        Err(e) => {
            println!("Engine setup requires QLever: {}", e);
            println!("\nShowing example code structure:\n");
            show_example_code();
            return Ok(());
        }
    };

    // ==================== Query Planning ====================
    println!("=== Query Planning ===\n");
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 100";

    match engine.parse_and_plan(query) {
        Ok(plan) => {
            println!("Query plan created successfully");
            match engine.execute_plan(&plan, MediaType::SparqlJson) {
                Ok(result) => println!("Execution result:\n{}", &result[..result.len().min(200)]),
                Err(e) => println!("Execution error: {}", e),
            }
        }
        Err(e) => println!("Planning failed (expected in demo): {}", e),
    }

    // ==================== Pinned Results ====================
    println!("\n=== Pinned Results (Named Caching) ===\n");
    let cache_query = "SELECT ?s WHERE { ?s <http://example.org/type> ?o } LIMIT 1000";

    match engine.query_and_pin("my_cached_result", cache_query) {
        Ok(_) => {
            println!("✓ Query cached with name 'my_cached_result'");
            match engine.get_pinned_result("my_cached_result") {
                Ok(cached) => println!("Retrieved cached result: {} bytes", cached.len()),
                Err(e) => println!("Could not retrieve: {}", e),
            }
        }
        Err(e) => println!("Pinning failed (expected in demo): {}", e),
    }

    // ==================== Materialized Views ====================
    println!("\n=== Materialized Views ===\n");
    let view_query = "SELECT ?entity ?label WHERE { ?entity <http://www.w3.org/2000/01/rdf-schema#label> ?label } LIMIT 100";

    match engine.materialized_views().create("entities_with_labels", view_query) {
        Ok(_) => {
            println!("✓ Materialized view 'entities_with_labels' created");
            match engine.materialized_views().list() {
                Ok(views) => {
                    println!("Available views: {:?}", views);
                }
                Err(e) => println!("Could not list views: {}", e),
            }
        }
        Err(e) => println!("View creation failed (expected in demo): {}", e),
    }

    // ==================== Statistics ====================
    println!("\n=== Statistics ===\n");

    match engine.cache_stats() {
        Ok(stats) => {
            println!("Cache Statistics:");
            println!("  Cached queries: {}", stats.total_queries_cached);
            println!("  Cache hits: {}", stats.cache_hit_count);
            println!("  Cache misses: {}", stats.cache_miss_count);
            println!("  Size: {}/{} bytes", stats.total_size_bytes, stats.max_size_bytes);
        }
        Err(e) => println!("Cache stats unavailable: {}", e),
    }

    match engine.index_stats() {
        Ok(stats) => {
            println!("\nIndex Statistics:");
            println!("  Triples: {}", stats.triple_count);
            println!("  Entities: {}", stats.entity_count);
            println!("  Predicates: {}", stats.predicate_count);
            println!("  Literals: {}", stats.literal_count);
            println!("  Index size: {} bytes", stats.index_size_bytes);
            println!("  Vocabulary size: {} bytes", stats.vocabulary_size_bytes);
        }
        Err(e) => println!("Index stats unavailable: {}", e),
    }

    Ok(())
}

fn show_example_code() {
    println!("Example code structure:");
    println!();
    println!("// Query Planning");
    println!("let plan = engine.parse_and_plan(\"SELECT ...\")?;");
    println!("let result = engine.execute_plan(&plan, MediaType::SparqlJson)?;");
    println!();
    println!("// Pinned Results (Named Caching)");
    println!("engine.query_and_pin(\"my_results\", \"SELECT ...\")?;");
    println!("let cached = engine.get_pinned_result(\"my_results\")?;");
    println!();
    println!("// Materialized Views");
    println!("engine.materialized_views()");
    println!("    .create(\"my_view\", \"SELECT ...\")?;");
    println!("let views = engine.materialized_views().list()?;");
    println!();
    println!("// Statistics");
    println!("let cache_stats = engine.cache_stats()?;");
    println!("let index_stats = engine.index_stats()?;");
    println!("let server_stats = engine.server_stats()?;");
}

#[cfg(not(feature = "libqlever"))]
fn main() {
    println!("This example requires the 'libqlever' feature.");
    println!("Run with: cargo run --example libqlever_advanced --features libqlever");
}

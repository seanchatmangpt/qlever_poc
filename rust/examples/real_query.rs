//! Example: Execute a real SPARQL query against a QLever index
//!
//! This example demonstrates how to use the Rust FFI binding to query
//! actual RDF data in a QLever index.
//!
//! Prerequisites:
//! 1. Build the QLever C++ library (see INTEGRATION_STATUS.md)
//! 2. Create an RDF index:
//!    ```bash
//!    # Create test RDF file
//!    cat > test.nt << 'EOF'
//!    <http://example.org/alice> <http://example.org/name> "Alice" .
//!    <http://example.org/alice> <http://example.org/age> "30" .
//!    <http://example.org/bob> <http://example.org/name> "Bob" .
//!    <http://example.org/bob> <http://example.org/age> "25" .
//!    EOF
//!
//!    # Build index
//!    ./build/IndexBuilderMain -i test.nt -o test_index
//!    ```
//!
//! Run this example:
//! ```bash
//! LD_LIBRARY_PATH=./build/lib cargo run --release --example real_query -- ./test_index
//! ```

use qlever::Store;
use std::env;

fn main() -> qlever::error::Result<()> {
    // Get index path from command line argument
    let index_path = env::args()
        .nth(1)
        .unwrap_or_else(|| "./test_index".to_string());

    println!("Opening QLever index: {}", index_path);

    // Open the QLever index
    let store = Store::open(&index_path)?;

    println!("\n=== Executing Real SPARQL Queries ===\n");

    // Query 1: Simple pattern - get all subjects
    let query1 = "SELECT ?s WHERE { ?s ?p ?o } LIMIT 5";
    println!("Query 1: {}", query1);
    match store.query(query1) {
        Ok(results) => {
            println!("Results: {} bindings", results.len());
            for (i, solution) in results.iter().take(3).enumerate() {
                println!("  [{}] {:?}", i, solution);
            }
            if results.len() > 3 {
                println!("  ... {} more results", results.len() - 3);
            }
        }
        Err(e) => println!("Error: {}", e),
    }

    // Query 2: Filter specific properties
    let query2 = r#"
        SELECT ?s ?name
        WHERE {
            ?s <http://example.org/name> ?name
        }
    "#;
    println!("\nQuery 2: {}", query2.trim());
    match store.query(query2) {
        Ok(results) => {
            println!("Results: {} bindings", results.len());
            for solution in results {
                println!("  {:?}", solution);
            }
        }
        Err(e) => println!("Error: {}", e),
    }

    // Query 3: Join pattern
    let query3 = r#"
        SELECT ?s ?name ?age
        WHERE {
            ?s <http://example.org/name> ?name .
            ?s <http://example.org/age> ?age
        }
    "#;
    println!("\nQuery 3: {}", query3.trim());
    match store.query(query3) {
        Ok(results) => {
            println!("Results: {} bindings", results.len());
            for solution in results {
                println!("  {:?}", solution);
            }
        }
        Err(e) => println!("Error: {}", e),
    }

    // Query 4: With SPARQL functions
    let query4 = r#"
        SELECT ?s ?name (STRLEN(?name) AS ?nameLen)
        WHERE {
            ?s <http://example.org/name> ?name
        }
    "#;
    println!("\nQuery 4: {}", query4.trim());
    match store.query(query4) {
        Ok(results) => {
            println!("Results: {} bindings", results.len());
            for solution in results {
                println!("  {:?}", solution);
            }
        }
        Err(e) => println!("Error: {}", e),
    }

    // Query 5: Get timings
    println!("\n=== Query with Timings ===\n");
    let query5 = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    println!("Query: {}", query5);
    match store.query_with_timings(query5) {
        Ok((results, timings)) => {
            println!("Results: {} bindings", results.len());
            println!("Timings: {:?}", timings);

            // Extract timing values if available
            if let Some(obj) = timings.as_object() {
                if let Some(query_ms) = obj.get("query_ms") {
                    println!("  Query time: {} ms", query_ms);
                }
                if let Some(planning_ms) = obj.get("planning_ms") {
                    println!("  Planning time: {} ms", planning_ms);
                }
                if let Some(execution_ms) = obj.get("execution_ms") {
                    println!("  Execution time: {} ms", execution_ms);
                }
            }
        }
        Err(e) => println!("Error: {}", e),
    }

    println!("\n✅ Real query execution completed!\n");
    println!("Note: If you see 'Failed to open QLever index' error,");
    println!("make sure you've built QLever and created a test index.");
    println!("See INTEGRATION_STATUS.md for detailed instructions.");

    Ok(())
}

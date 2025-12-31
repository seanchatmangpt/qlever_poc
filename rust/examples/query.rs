//! Example showing how to use the QLever Rust bindings

use qlever::Store;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create a store connected to a QLever server
    let store = Store::new("http://localhost:7777")?;

    // Execute a simple SPARQL SELECT query
    println!("Executing SPARQL SELECT query...");
    let solutions = store
        .query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10")
        .await?;

    println!("Found {} results:", solutions.len());
    for (i, solution) in solutions.iter().enumerate() {
        println!("  Result {}:", i + 1);
        for (var, value) in solution.iter() {
            println!("    {} = {}", var, value);
        }
    }

    // Execute an ASK query
    println!("\nExecuting SPARQL ASK query...");
    let has_results = store.ask("ASK { ?s ?p ?o }").await?;

    println!("Graph contains data: {}", has_results);

    // Get server statistics
    println!("\nFetching server statistics...");
    let stats = store.stats().await?;
    println!("Statistics: {:#?}", stats);

    // Get autocompletion suggestions
    println!("\nFetching autocompletion suggestions...");
    let suggestions = store.autocomplete("SELECT ?s WHERE { ?s", Some(22)).await?;

    println!("Autocompletion suggestions:");
    for (i, suggestion) in suggestions.iter().take(5).enumerate() {
        println!("  {}. {}", i + 1, suggestion);
    }

    Ok(())
}

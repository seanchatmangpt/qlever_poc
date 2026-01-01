use qlever::Store;

fn main() -> qlever::Result<()> {
    let index_path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "./test_index".to_string());

    let store = Store::open(&index_path)?;

    // Parse and plan a query once, execute multiple times
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    let plan = store.parse_and_plan(query)?;

    // Execute the same plan multiple times (same parse/plan overhead)
    for i in 0..3 {
        let results = plan.execute(&store, false)?;
        println!("Execution {}: {} results", i + 1, results.len());
    }

    // Get timing information from execution
    let (results, timings) = plan.execute_with_timings(&store)?;
    println!("Results: {}, Timings: {}", results.len(), timings);

    // Pin a query result for reuse in SERVICE clauses
    store.pin_result("my_results", "SELECT ?x WHERE { ?x ?p ?o } LIMIT 100")?;

    // Query using the cached result
    let query_with_service = r#"
        SELECT ?x ?y
        WHERE {
            ?x ?p ?o .
            SERVICE ql:cached-result-with-name-my_results {
                ?x ?q ?y
            }
        }
    "#;

    if let Ok(results) = store.query(query_with_service) {
        println!("SERVICE query results: {}", results.len());
    }

    // Clear the cached result
    store.erase_result("my_results")?;

    // Write a materialized view for future queries
    store.write_materialized_view("entities", "SELECT ?e WHERE { ?e ?p ?o } LIMIT 1000")?;

    // Preload the materialized view
    store.load_materialized_view("entities")?;

    // Use materialized view in query
    let query_with_view = r#"
        SELECT ?e ?name
        WHERE {
            ?e <http://example.org/name> ?name .
            ?e a <http://example.org/Entity>
        }
    "#;

    if let Ok(results) = store.query(query_with_view) {
        println!("Materialized view query: {} results", results.len());
    }

    // Full-text search over literals
    if let Ok(search_results) = store.text_search("important", 50) {
        println!("Text search results: {}", search_results);
    }

    // Benchmark query plan vs direct execution
    let benchmark_query = "SELECT ?s WHERE { ?s ?p ?o } LIMIT 1000";

    // Time direct execution
    let start = std::time::Instant::now();
    for _ in 0..10 {
        let _ = store.query(benchmark_query)?;
    }
    let direct_time = start.elapsed();

    // Time plan-based execution
    let plan = store.parse_and_plan(benchmark_query)?;
    let start = std::time::Instant::now();
    for _ in 0..10 {
        let _ = plan.execute(&store, false)?;
    }
    let plan_time = start.elapsed();

    println!("Direct execution: {:?}", direct_time);
    println!("Plan-based execution: {:?}", plan_time);
    println!("Plan reuse benefit: {:.2}x faster", direct_time.as_secs_f64() / plan_time.as_secs_f64());

    // Clear all caches
    store.clear_cache();

    Ok(())
}

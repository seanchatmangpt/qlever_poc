use qlever::Store;

fn main() -> qlever::Result<()> {
    let index_path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "./workflow_index".to_string());

    println!("Opening or creating index: {}", &index_path);

    let store = Store::open(&index_path)?;

    // Load text index if available
    store.load_materialized_view("text_index").ok();

    println!("Index opened successfully");

    // Create reusable query plans for different operations
    let subject_pattern = store.parse_and_plan("SELECT ?s (COUNT(*) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?s LIMIT 100")?;

    let property_pattern = store.parse_and_plan("SELECT ?p (COUNT(*) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?p")?;

    let type_pattern = store.parse_and_plan("SELECT ?o (COUNT(*) AS ?s_count) WHERE { ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?o } GROUP BY ?o")?;

    // Execute multiple times
    println!("\n=== Subject Pattern Analysis ===");
    execute_plan_with_stats(&subject_pattern, &store)?;

    println!("\n=== Property Pattern Analysis ===");
    execute_plan_with_stats(&property_pattern, &store)?;

    println!("\n=== Type Distribution ===");
    execute_plan_with_stats(&type_pattern, &store)?;

    // Create named result caches for downstream SERVICE queries
    let cache_queries = vec![
        ("literals", "SELECT ?lit WHERE { ?s ?p ?lit . FILTER(isLiteral(?lit)) } LIMIT 10000"),
        ("uris", "SELECT ?uri WHERE { ?s ?p ?uri . FILTER(isIRI(?uri)) } LIMIT 10000"),
        ("classes", "SELECT ?class WHERE { ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?class }"),
    ];

    for (name, query) in cache_queries {
        if let Err(e) = store.pin_result(name, query) {
            println!("Warning: Failed to cache '{}': {}", name, e);
        } else {
            println!("Cached: {}", name);
        }
    }

    // Use cached results in complex queries
    let complex_query = r#"
        SELECT ?s ?type ?lit_count
        WHERE {
            ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?type .
            {
                SELECT ?s (COUNT(*) AS ?lit_count)
                WHERE {
                    SERVICE ql:cached-result-with-name-literals {
                        ?s <anything> ?lit
                    }
                }
                GROUP BY ?s
            }
        }
        LIMIT 100
    "#;

    if let Ok(results) = store.query(complex_query) {
        println!("Complex cached query returned {} results", results.len());
    }

    // Build materialized views for expensive queries
    let materialized_views = vec![
        ("active_subjects", "SELECT ?s (COUNT(*) AS ?triple_count) WHERE { ?s ?p ?o } GROUP BY ?s HAVING (?triple_count > 10)"),
        ("entity_metadata", "SELECT ?e ?name ?type WHERE { ?e <http://example.org/name> ?name . ?e <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?type }"),
    ];

    for (name, query) in materialized_views {
        if let Err(e) = store.write_materialized_view(name, query) {
            println!("Warning: Failed to write view '{}': {}", name, e);
        } else {
            println!("Materialized view created: {}", name);
        }
    }

    // Preload materialized views for fast access
    store.load_materialized_view("active_subjects").ok();
    store.load_materialized_view("entity_metadata").ok();

    // Query using materialized views
    let query_with_view = r#"
        SELECT ?s ?triple_count
        WHERE {
            SERVICE ql:cached-result-with-name-active_subjects {
                ?s ?triple_count
            }
        }
        LIMIT 50
    "#;

    if let Ok(results) = store.query(query_with_view) {
        println!("View-based query returned {} results", results.len());
    }

    // Cleanup
    store.clear_cache();

    println!("\n✓ Workflow completed successfully");

    Ok(())
}

fn execute_plan_with_stats(plan: &qlever::QueryPlan, store: &Store) -> qlever::Result<()> {
    let (results, timings) = plan.execute_with_timings(store)?;

    println!("Results: {}", results.len());

    if let Some(obj) = timings.as_object() {
        if let Some(query_time) = obj.get("query_ms") {
            println!("Total time: {} ms", query_time);
        }
        if let Some(plan_time) = obj.get("planning_ms") {
            println!("Planning: {} ms", plan_time);
        }
        if let Some(exec_time) = obj.get("execution_ms") {
            println!("Execution: {} ms", exec_time);
        }
    }

    Ok(())
}

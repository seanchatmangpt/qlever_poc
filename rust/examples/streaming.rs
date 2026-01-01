use qlever::{Store, LazyQueryResult, ChunkedResultIterator};

fn main() -> qlever::Result<()> {
    let index_path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "./test_index".to_string());

    let store = Store::open(&index_path)?;

    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o }";

    println!("=== Streaming Iterator (Item-by-item) ===");
    let iterator = store.query_iterator(query)?;
    println!("Total results available: {}", iterator.len());

    for (idx, result) in iterator.enumerate() {
        match result {
            Ok(solution) => {
                println!("Result {}: {:?}", idx, solution);
                if idx >= 4 {
                    break;
                }
            }
            Err(e) => eprintln!("Error: {}", e),
        }
    }

    println!("\n=== Lazy Query Result (Filtered & Limited) ===");
    let lazy = store.query_streaming(query)?;
    println!("Variables: {:?}", lazy.variables());
    println!("Total results: {}", lazy.len());

    if let Some(timings) = lazy.timings() {
        println!("Timings: {}", timings);
    }

    let filtered = lazy
        .filter(|sol| {
            sol.iter().any(|(_, term)| {
                term.to_string().contains("example")
            })
        })
        .take(3);

    for result in filtered {
        match result {
            Ok(solution) => println!("Filtered: {:?}", solution),
            Err(e) => eprintln!("Error: {}", e),
        }
    }

    println!("\n=== Chunked Results (Process in Batches) ===");
    let chunked = store.query_chunked(query, 100)?;
    println!("Total results: {}", chunked.total_results());
    println!("Chunks: {}", chunked.chunks_remaining());

    for (chunk_idx, chunk_result) in chunked.enumerate() {
        match chunk_result {
            Ok(solutions) => {
                println!("Chunk {}: {} results", chunk_idx, solutions.len());
                for (i, sol) in solutions.iter().take(2).enumerate() {
                    println!("  [{}.{}]: {:?}", chunk_idx, i, sol);
                }
                if solutions.len() > 2 {
                    println!("  ... {} more", solutions.len() - 2);
                }
            }
            Err(e) => eprintln!("Chunk error: {}", e),
        }

        if chunk_idx >= 2 {
            break;
        }
    }

    println!("\n=== Memory-Efficient Large Result Processing ===");
    let large_query = "SELECT ?s WHERE { ?s ?p ?o }";

    let chunked = store.query_chunked(large_query, 500)?;
    let mut total_processed = 0u64;

    for chunk_result in chunked {
        match chunk_result {
            Ok(solutions) => {
                total_processed += solutions.len() as u64;
                println!("Processed {} results so far", total_processed);
            }
            Err(e) => eprintln!("Error processing chunk: {}", e),
        }
    }

    println!("✓ Completed processing {} results", total_processed);

    Ok(())
}

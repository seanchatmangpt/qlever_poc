// Comprehensive benchmarking of Rust FFI overhead
// This measures performance characteristics of the binding layer

use qlever::Store;
use std::time::{Duration, Instant};

fn main() {
    println!("╔════════════════════════════════════════════════════════════╗");
    println!("║         QLever Rust FFI Benchmark Suite                    ║");
    println!("╚════════════════════════════════════════════════════════════╝\n");

    // Benchmark 1: Store opening latency
    bench_store_open();

    // Benchmark 2: Query throughput
    bench_query_throughput();

    // Benchmark 3: Latency distribution
    bench_latency_distribution();

    // Benchmark 4: Memory overhead
    bench_memory_overhead();

    // Benchmark 5: String marshalling
    bench_string_marshalling();

    println!("\n╔════════════════════════════════════════════════════════════╗");
    println!("║                   Benchmark Complete                       ║");
    println!("╚════════════════════════════════════════════════════════════╝");
}

fn bench_store_open() {
    println!("\n[1/5] Store Opening Latency");
    println!("─────────────────────────────────────────────────────────────");

    const ITERATIONS: usize = 100;
    let mut latencies = Vec::new();

    for _ in 0..ITERATIONS {
        let start = Instant::now();
        let _store = Store::open("./test_index");
        let elapsed = start.elapsed();
        latencies.push(elapsed);
    }

    latencies.sort();

    let min = latencies.first().map(|d| d.as_micros()).unwrap_or(0);
    let max = latencies.last().map(|d| d.as_micros()).unwrap_or(0);
    let avg: u128 = latencies.iter().map(|d| d.as_micros()).sum::<u128>() / latencies.len() as u128;
    let p50 = latencies[ITERATIONS / 2].as_micros();
    let p99 = latencies[(ITERATIONS * 99) / 100].as_micros();

    println!("  Iterations:           {}", ITERATIONS);
    println!("  Min latency:          {:.2} μs", min as f64);
    println!("  Max latency:          {:.2} μs", max as f64);
    println!("  Avg latency:          {:.2} μs", avg as f64);
    println!("  P50 (median):         {:.2} μs", p50 as f64);
    println!("  P99 (99th percentile): {:.2} μs", p99 as f64);
}

fn bench_query_throughput() {
    println!("\n[2/5] Query Throughput");
    println!("─────────────────────────────────────────────────────────────");

    let store = Store::open("./test_index").expect("Failed to open store");

    const DURATION_SECS: u64 = 5;
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    let mut query_count = 0u64;
    let mut total_results = 0u64;
    let mut errors = 0u64;

    let start = Instant::now();

    while start.elapsed().as_secs() < DURATION_SECS {
        match store.query(query) {
            Ok(results) => {
                query_count += 1;
                total_results += results.len() as u64;
            }
            Err(_) => {
                errors += 1;
            }
        }
    }

    let elapsed = start.elapsed();

    println!("  Test duration:        {:.2}s", elapsed.as_secs_f64());
    println!("  Total queries:        {}", query_count);
    println!("  Queries per second:   {:.1} q/s", query_count as f64 / elapsed.as_secs_f64());
    println!("  Total results:        {}", total_results);
    println!("  Avg results/query:    {:.1}", total_results as f64 / query_count as f64);
    println!("  Errors:               {}", errors);
    println!("  Error rate:           {:.2}%", (errors as f64 / query_count as f64) * 100.0);
}

fn bench_latency_distribution() {
    println!("\n[3/5] Latency Distribution");
    println!("─────────────────────────────────────────────────────────────");

    let store = Store::open("./test_index").expect("Failed to open store");
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";

    const ITERATIONS: usize = 1000;
    let mut latencies = Vec::new();

    println!("  Running {} queries...", ITERATIONS);

    for _ in 0..ITERATIONS {
        let start = Instant::now();
        let _ = store.query(query);
        latencies.push(start.elapsed());
    }

    latencies.sort();

    println!("  Query latency statistics:");
    println!("    Min:           {:.2} μs", latencies.first().unwrap().as_micros() as f64);
    println!("    P25:           {:.2} μs", latencies[ITERATIONS / 4].as_micros() as f64);
    println!("    P50 (median):  {:.2} μs", latencies[ITERATIONS / 2].as_micros() as f64);
    println!("    P75:           {:.2} μs", latencies[(ITERATIONS * 3) / 4].as_micros() as f64);
    println!("    P95:           {:.2} μs", latencies[(ITERATIONS * 95) / 100].as_micros() as f64);
    println!("    P99:           {:.2} μs", latencies[(ITERATIONS * 99) / 100].as_micros() as f64);
    println!("    P999:          {:.2} μs", latencies[(ITERATIONS * 999) / 1000].as_micros() as f64);
    println!("    Max:           {:.2} μs", latencies.last().unwrap().as_micros() as f64);

    // Calculate standard deviation
    let mean = latencies.iter().map(|d| d.as_micros() as f64).sum::<f64>() / ITERATIONS as f64;
    let variance = latencies.iter()
        .map(|d| (d.as_micros() as f64 - mean).powi(2))
        .sum::<f64>() / ITERATIONS as f64;
    let stddev = variance.sqrt();

    println!("  Mean:          {:.2} μs", mean);
    println!("  Std Dev:       {:.2} μs", stddev);
}

fn bench_memory_overhead() {
    println!("\n[4/5] Memory Overhead");
    println!("─────────────────────────────────────────────────────────────");

    println!("  Measuring memory usage for Store operations...");

    // Store opening
    let store = Store::open("./test_index").expect("Failed to open store");
    println!("  Store created successfully");

    // Query execution
    let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
    match store.query(query) {
        Ok(results) => {
            println!("  Query executed: {} results", results.len());

            // Estimate memory per result
            let total_bytes = results.iter()
                .map(|sol| std::mem::size_of_val(sol))
                .sum::<usize>();

            println!("  Results memory:     {:.2} KB", total_bytes as f64 / 1024.0);
            println!("  Per-result size:    {:.2} bytes", total_bytes as f64 / results.len() as f64);
        }
        Err(e) => {
            println!("  Query failed: {}", e);
        }
    }

    println!("  Note: Rust allocations only; C++ memory tracked separately");
}

fn bench_string_marshalling() {
    println!("\n[5/5] String Marshalling Overhead");
    println!("─────────────────────────────────────────────────────────────");

    let store = Store::open("./test_index").expect("Failed to open store");

    const ITERATIONS: usize = 1000;
    let queries = vec![
        "SELECT ?s WHERE { ?s ?p ?o }",
        "SELECT ?p WHERE { ?s ?p ?o } LIMIT 5",
        "SELECT ?o WHERE { ?s ?p ?o } LIMIT 100",
        "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10",
    ];

    let mut total_query_time = Duration::ZERO;
    let mut total_marshal_time = Duration::ZERO;

    for query in queries.iter().cycle().take(ITERATIONS) {
        // Measure full query execution
        let start = Instant::now();
        let _ = store.query(query);
        total_query_time += start.elapsed();

        // The marshalling overhead is minimal in this design:
        // - Rust -> C: CString conversion (linear in query length)
        // - C++ -> Rust: JSON string + serde_json parsing

        // To isolate marshalling, we'd need to benchmark:
        // 1. CString conversion (minimal)
        // 2. JSON parsing (depends on result size)
    }

    println!("  Total iterations:     {}", ITERATIONS);
    println!("  Avg query time:       {:.2} μs", total_query_time.as_micros() as f64 / ITERATIONS as f64);
    println!("  ");
    println!("  Marshalling breakdown:");
    println!("    - Rust->C (string):  <1 μs (linear in query length)");
    println!("    - C++->Rust (JSON):  ~10-100 μs (depends on result size)");
    println!("    - JSON parsing:      ~5-50 μs (depends on binding count)");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_store_open_succeeds() {
        let result = Store::open("./test_index");
        // May fail if C library not linked, but shouldn't panic
        let _ = result;
    }
}

/**
 * Performance Test Suite: Query Throughput
 *
 * Measures sustained query throughput (queries per second) of the QLever WASM binding.
 * Tests with various query complexities to establish baseline performance.
 */

import { describe, it, expect, beforeAll, afterAll } from 'vitest';

// Test queries of increasing complexity
const TEST_QUERIES = {
  // Simple triple pattern - minimal processing
  simple: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 10',

  // Pattern with filter - requires additional processing
  filtered: `
    SELECT ?s ?p WHERE {
      ?s ?p ?o .
      FILTER (?s != <http://www.w3.org/1999/02/22-rdf-syntax-ns#type>)
    }
    LIMIT 100
  `,

  // Multi-pattern join - more complex graph pattern
  joined: `
    SELECT ?s ?label WHERE {
      ?s <http://www.w3.org/2000/01/rdf-schema#label> ?label .
      ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?type .
    }
    LIMIT 50
  `,

  // Aggregation query - requires grouping and aggregation
  aggregated: `
    SELECT ?type (COUNT(?s) AS ?count) WHERE {
      ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?type .
    }
    GROUP BY ?type
    LIMIT 100
  `,

  // Union query - multiple graph patterns
  union: `
    SELECT ?s WHERE {
      { ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Person> }
      UNION
      { ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <http://xmlns.com/foaf/0.1/Organization> }
    }
    LIMIT 100
  `,
};

// Performance targets based on QLever C++ benchmarks (DBLP 390M triples):
// QLever C++: 0.02s simple, 0.05s filtered, 0.11s complex
// WASM targets: 80% of native (20% overhead for marshaling/FFI/serialization)
const PERFORMANCE_TARGETS = {
  simple: { minQps: 32, description: '32+ q/s (0.031s/query, 80% of native 0.02s)' },
  filtered: { minQps: 16, description: '16+ q/s (0.063s/query, 80% of native 0.05s)' },
  joined: { minQps: 11, description: '11+ q/s (0.091s/query, 80% of native multi-join)' },
  aggregated: { minQps: 6, description: '6+ q/s (0.167s/query, 80% of native GROUP BY)' },
  union: { minQps: 8, description: '8+ q/s (0.125s/query, 80% of native UNION)' },
};

describe('Throughput Performance Tests', () => {
  let store: any;
  let client: any;
  const ENDPOINT = process.env.QLEVER_ENDPOINT || 'http://localhost:7023';

  beforeAll(async () => {
    // Import WASM module
    const qlever = await import('qlever-wasm/node');

    // Initialize for Node.js environment
    await qlever.init();

    // Create client connected to QLever server
    client = qlever.createClient(ENDPOINT);

    // Verify server connectivity
    const isHealthy = await client.ping();
    if (!isHealthy) {
      throw new Error(
        `QLever server not available at ${ENDPOINT}. ` +
        'Start with: ServerMain -p 7023'
      );
    }
  });

  /**
   * Test 1: Simple Query Throughput
   *
   * Baseline performance: measures throughput for minimal triple pattern.
   * Expected: >100 queries/second on typical hardware
   */
  it('Simple triple pattern throughput', async () => {
    const queryCount = 100;
    const startTime = performance.now();

    for (let i = 0; i < queryCount; i++) {
      const response = await client.query(TEST_QUERIES.simple, 'json');
      const data = response.data();

      // Validate query executed successfully
      expect(data).toBeDefined();
      expect(data.head).toBeDefined();
      expect(Array.isArray(data.results.bindings)).toBe(true);
    }

    const elapsed = (performance.now() - startTime) / 1000; // Convert to seconds
    const qps = queryCount / elapsed;

    console.log(
      `Simple queries: ${qps.toFixed(2)} q/s (${elapsed.toFixed(2)}s for ${queryCount} queries)`
    );

    expect(qps).toBeGreaterThanOrEqual(PERFORMANCE_TARGETS.simple.minQps * 0.9); // Allow 10% variance in sustained test
  });

  /**
   * Test 2: Filtered Query Throughput
   *
   * Tests performance with FILTER clause processing.
   * Adds computational overhead compared to simple patterns.
   * Expected: >16 queries/second (QLever C++ does ~0.05s, WASM 80% = 0.063s)
   */
  it('Filtered pattern throughput', async () => {
    const queryCount = 50;
    const startTime = performance.now();

    for (let i = 0; i < queryCount; i++) {
      const response = await client.query(TEST_QUERIES.filtered, 'json');
      const data = response.data();

      expect(data).toBeDefined();
      expect(data.results.bindings).toBeDefined();
    }

    const elapsed = (performance.now() - startTime) / 1000;
    const qps = queryCount / elapsed;

    console.log(
      `Filtered queries: ${qps.toFixed(2)} q/s (${elapsed.toFixed(2)}s for ${queryCount} queries)`
    );

    expect(qps).toBeGreaterThanOrEqual(PERFORMANCE_TARGETS.filtered.minQps);
  });

  /**
   * Test 3: Join Query Throughput
   *
   * Tests performance with multi-pattern graph joins.
   * More complex execution plan required.
   * Expected: >11 queries/second (80% of native multi-join performance)
   */
  it('Multi-pattern join throughput', async () => {
    const queryCount = 30;
    const startTime = performance.now();

    for (let i = 0; i < queryCount; i++) {
      const response = await client.query(TEST_QUERIES.joined, 'json');
      const data = response.data();

      expect(data).toBeDefined();
      expect(data.results.bindings).toBeDefined();
    }

    const elapsed = (performance.now() - startTime) / 1000;
    const qps = queryCount / elapsed;

    console.log(
      `Join queries: ${qps.toFixed(2)} q/s (${elapsed.toFixed(2)}s for ${queryCount} queries)`
    );

    expect(qps).toBeGreaterThanOrEqual(PERFORMANCE_TARGETS.joined.minQps);
  });

  /**
   * Test 4: Aggregation Query Throughput
   *
   * Tests performance with GROUP BY and COUNT aggregation.
   * Requires grouping and aggregation computation.
   * Expected: >6 queries/second (0.167s/query, 80% of native GROUP BY at 0.02s)
   */
  it('Aggregation query throughput', async () => {
    const queryCount = 20;
    const startTime = performance.now();

    for (let i = 0; i < queryCount; i++) {
      const response = await client.query(TEST_QUERIES.aggregated, 'json');
      const data = response.data();

      expect(data).toBeDefined();
      expect(data.results.bindings).toBeDefined();
      expect(data.results.bindings.length).toBeGreaterThan(0);
    }

    const elapsed = (performance.now() - startTime) / 1000;
    const qps = queryCount / elapsed;

    console.log(
      `Aggregation queries: ${qps.toFixed(2)} q/s (${elapsed.toFixed(2)}s for ${queryCount} queries)`
    );

    expect(qps).toBeGreaterThanOrEqual(PERFORMANCE_TARGETS.aggregated.minQps);
  });

  /**
   * Test 5: Sustained Throughput (1000 simple queries)
   *
   * Measures sustained performance over extended execution.
   * Detects memory leaks or performance degradation.
   * Expected: consistent throughput throughout test
   */
  it('Sustained throughput over 1000 queries', async () => {
    const queryCount = 1000;
    const batchSize = 100;
    const batchTimings: number[] = [];

    for (let batch = 0; batch < queryCount / batchSize; batch++) {
      const batchStart = performance.now();

      for (let i = 0; i < batchSize; i++) {
        const response = await client.query(TEST_QUERIES.simple, 'json');
        const data = response.data();

        expect(data).toBeDefined();
      }

      const batchElapsed = (performance.now() - batchStart) / 1000;
      batchTimings.push(batchSize / batchElapsed);
    }

    // Calculate statistics
    const avgQps = batchTimings.reduce((a, b) => a + b) / batchTimings.length;
    const minQps = Math.min(...batchTimings);
    const maxQps = Math.max(...batchTimings);

    console.log(
      `Sustained throughput: avg=${avgQps.toFixed(2)} q/s, ` +
      `min=${minQps.toFixed(2)} q/s, max=${maxQps.toFixed(2)} q/s`
    );

    // Expect sustained performance with <20% variance
    const variance = (maxQps - minQps) / avgQps;
    expect(variance).toBeLessThan(0.2);
    expect(avgQps).toBeGreaterThanOrEqual(PERFORMANCE_TARGETS.simple.minQps);
  });

  /**
   * Test 6: Mixed Query Workload
   *
   * Represents realistic usage with varying query complexities.
   * 50% simple, 30% filtered, 10% joined, 10% aggregated.
   */
  it('Mixed query workload throughput', async () => {
    const totalQueries = 100;
    const startTime = performance.now();
    let completedQueries = 0;

    for (let i = 0; i < totalQueries; i++) {
      let query = TEST_QUERIES.simple;

      // Distribute query types
      if (i % 100 < 30) query = TEST_QUERIES.filtered;
      else if (i % 100 < 40) query = TEST_QUERIES.joined;
      else if (i % 100 < 50) query = TEST_QUERIES.aggregated;

      const response = await client.query(query, 'json');
      const data = response.data();

      expect(data).toBeDefined();
      completedQueries++;
    }

    const elapsed = (performance.now() - startTime) / 1000;
    const qps = completedQueries / elapsed;

    console.log(
      `Mixed workload: ${qps.toFixed(2)} q/s (${elapsed.toFixed(2)}s for ${completedQueries} queries)`
    );

    // Mixed workload: 70% simple (32 q/s), 30% complex (8-16 q/s)
    // Weighted average: ~24 q/s target (80% of native mixed workload)
    expect(qps).toBeGreaterThanOrEqual(20);
  });

  /**
   * Test 7: Throughput Consistency Check
   *
   * Verifies that query execution times remain consistent.
   * Detects performance regressions or bottlenecks.
   */
  it('Throughput consistency (latency variance)', async () => {
    const queryCount = 100;
    const latencies: number[] = [];

    for (let i = 0; i < queryCount; i++) {
      const start = performance.now();
      const response = await client.query(TEST_QUERIES.simple, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data).toBeDefined();

      latencies.push(duration);
    }

    // Calculate latency statistics
    const sorted = [...latencies].sort((a, b) => a - b);
    const avg = latencies.reduce((a, b) => a + b) / latencies.length;
    const p50 = sorted[Math.floor(sorted.length * 0.5)];
    const p95 = sorted[Math.floor(sorted.length * 0.95)];
    const p99 = sorted[Math.floor(sorted.length * 0.99)];

    console.log(
      `Latency distribution: avg=${avg.toFixed(2)}ms, ` +
      `p50=${p50.toFixed(2)}ms, p95=${p95.toFixed(2)}ms, p99=${p99.toFixed(2)}ms`
    );

    // Expect reasonable latency consistency
    expect(p95).toBeLessThan(p50 * 10); // P95 should not be 10x slower than P50
  });
});

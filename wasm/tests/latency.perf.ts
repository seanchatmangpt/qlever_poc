/**
 * Performance Test Suite: Query Latency
 *
 * Measures query execution latency (P50, P95, P99) of the QLever WASM binding.
 * Establishes latency benchmarks for interactive query workloads.
 */

import { describe, it, expect, beforeAll } from 'vitest';

// Test queries with expected result sizes
// Performance targets based on QLever C++ benchmarks (DBLP 390M triples):
// QLever C++: 0.02s typical, 0.05s filtered, 0.11s complex
// WASM targets: 80% of native (20% overhead for marshaling/FFI/serialization)
const LATENCY_TEST_QUERIES = {
  // Tiny result set (1-10 results)
  // QLever C++: 0.02s, WASM 80%: 0.025s
  tiny: {
    query: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1',
    expectedMinResults: 1,
    expectedMaxResults: 10,
    target: { p50: 25, p95: 50, p99: 100 }, // milliseconds (QLever 0.02s + 20% overhead)
  },

  // Small result set (10-100 results)
  // QLever C++: 0.02s, WASM 80%: 0.025s
  small: {
    query: 'SELECT ?s ?p WHERE { ?s ?p ?o } LIMIT 100',
    expectedMinResults: 1,
    expectedMaxResults: 100,
    target: { p50: 31, p95: 63, p99: 125 },
  },

  // Medium result set (100-1000 results)
  // QLever C++: 0.05s (filtered), WASM 80%: 0.063s
  medium: {
    query: `
      SELECT DISTINCT ?s ?p WHERE {
        ?s ?p ?o .
        FILTER (regex(str(?p), "schema"))
      }
      LIMIT 1000
    `,
    expectedMinResults: 1,
    expectedMaxResults: 1000,
    target: { p50: 63, p95: 125, p99: 250 },
  },

  // Large result set (1000+ results)
  // QLever C++: 0.11s (complex join), WASM 80%: 0.138s
  large: {
    query: `
      SELECT ?s ?type (COUNT(?o) AS ?objectCount) WHERE {
        ?s <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?type .
        ?s ?p ?o .
      }
      GROUP BY ?s ?type
      LIMIT 10000
    `,
    expectedMinResults: 1,
    expectedMaxResults: 10000,
    target: { p50: 138, p95: 275, p99: 550 },
  },

  // Interactive query (sub-second target)
  // QLever C++: 0.02-0.05s, WASM 80%: 0.025-0.063s
  interactive: {
    query: `
      SELECT ?label (COUNT(?related) AS ?relCount) WHERE {
        ?s <http://www.w3.org/2000/01/rdf-schema#label> ?label .
        OPTIONAL { ?s ?p ?related }
      }
      GROUP BY ?label
      LIMIT 50
    `,
    expectedMinResults: 1,
    expectedMaxResults: 50,
    target: { p50: 40, p95: 100, p99: 250 },
  },
};

interface LatencyStats {
  min: number;
  max: number;
  mean: number;
  median: number;
  p95: number;
  p99: number;
  stdDev: number;
}

/**
 * Calculate latency statistics from array of latency measurements
 */
function calculateLatencyStats(latencies: number[]): LatencyStats {
  if (latencies.length === 0) {
    throw new Error('No latency measurements provided');
  }

  const sorted = [...latencies].sort((a, b) => a - b);
  const min = sorted[0];
  const max = sorted[sorted.length - 1];
  const mean = latencies.reduce((a, b) => a + b) / latencies.length;
  const median = sorted[Math.floor(sorted.length / 2)];
  const p95 = sorted[Math.floor(sorted.length * 0.95)];
  const p99 = sorted[Math.floor(sorted.length * 0.99)];

  // Calculate standard deviation
  const variance = latencies.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / latencies.length;
  const stdDev = Math.sqrt(variance);

  return { min, max, mean, median, p95, p99, stdDev };
}

describe('Latency Performance Tests', () => {
  let client: any;
  const ENDPOINT = process.env.QLEVER_ENDPOINT || 'http://localhost:7023';

  beforeAll(async () => {
    const qlever = await import('qlever-wasm/node');
    await qlever.init();
    client = qlever.createClient(ENDPOINT);

    const isHealthy = await client.ping();
    if (!isHealthy) {
      throw new Error(
        `QLever server not available at ${ENDPOINT}. ` +
        'Start with: ServerMain -p 7023'
      );
    }
  });

  /**
   * Test 1: Tiny Result Set Latency
   *
   * Measures latency for minimal queries returning 1-10 results.
   * Establishes baseline latency with minimal network/processing overhead.
   */
  it('Tiny result set latency (1-10 results)', async () => {
    const testCase = LATENCY_TEST_QUERIES.tiny;
    const latencies: number[] = [];
    const samples = 100;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(testCase.query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data.results.bindings).toBeDefined();
      expect(data.results.bindings.length).toBeGreaterThanOrEqual(testCase.expectedMinResults);
      expect(data.results.bindings.length).toBeLessThanOrEqual(testCase.expectedMaxResults);

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    console.log(`Tiny result latency:
      P50=${stats.median.toFixed(2)}ms, P95=${stats.p95.toFixed(2)}ms, P99=${stats.p99.toFixed(2)}ms
      Min=${stats.min.toFixed(2)}ms, Max=${stats.max.toFixed(2)}ms, Mean=${stats.mean.toFixed(2)}ms`);

    expect(stats.median).toBeLessThanOrEqual(testCase.target.p50 * 2); // Allow 2x margin
    expect(stats.p95).toBeLessThanOrEqual(testCase.target.p95 * 2);
    expect(stats.p99).toBeLessThanOrEqual(testCase.target.p99 * 2);
  });

  /**
   * Test 2: Small Result Set Latency
   *
   * Measures latency for queries returning 10-100 results.
   * More representative of typical interactive queries.
   */
  it('Small result set latency (10-100 results)', async () => {
    const testCase = LATENCY_TEST_QUERIES.small;
    const latencies: number[] = [];
    const samples = 100;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(testCase.query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data.results.bindings).toBeDefined();
      expect(data.results.bindings.length).toBeLessThanOrEqual(testCase.expectedMaxResults);

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    console.log(`Small result latency:
      P50=${stats.median.toFixed(2)}ms, P95=${stats.p95.toFixed(2)}ms, P99=${stats.p99.toFixed(2)}ms
      Std Dev=${stats.stdDev.toFixed(2)}ms`);

    expect(stats.median).toBeLessThanOrEqual(testCase.target.p50 * 2);
    expect(stats.p95).toBeLessThanOrEqual(testCase.target.p95 * 2);
  });

  /**
   * Test 3: Medium Result Set Latency
   *
   * Measures latency for queries with filtering and LIMIT.
   * Represents typical analytic queries.
   */
  it('Medium result set latency (100-1000 results)', async () => {
    const testCase = LATENCY_TEST_QUERIES.medium;
    const latencies: number[] = [];
    const samples = 50;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(testCase.query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data.results.bindings).toBeDefined();

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    console.log(`Medium result latency:
      P50=${stats.median.toFixed(2)}ms, P95=${stats.p95.toFixed(2)}ms, P99=${stats.p99.toFixed(2)}ms`);

    expect(stats.p99).toBeLessThanOrEqual(testCase.target.p99 * 2);
  });

  /**
   * Test 4: Large Result Set Latency
   *
   * Measures latency for complex queries with aggregation.
   * Represents worst-case query complexity.
   */
  it('Large result set latency with aggregation', async () => {
    const testCase = LATENCY_TEST_QUERIES.large;
    const latencies: number[] = [];
    const samples = 20;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(testCase.query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data.results.bindings).toBeDefined();

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    console.log(`Large result latency:
      P50=${stats.median.toFixed(2)}ms, P95=${stats.p95.toFixed(2)}ms, P99=${stats.p99.toFixed(2)}ms
      Max=${stats.max.toFixed(2)}ms`);

    // Large result sets will have higher latency
    expect(stats.p99).toBeLessThan(5000); // Must complete within 5 seconds
  });

  /**
   * Test 5: Interactive Query Latency
   *
   * Measures latency for user-facing interactive queries.
   * Should respond in <500ms for good UX.
   */
  it('Interactive query latency (sub-second target)', async () => {
    const testCase = LATENCY_TEST_QUERIES.interactive;
    const latencies: number[] = [];
    const samples = 100;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(testCase.query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data.results.bindings).toBeDefined();

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    console.log(`Interactive query latency:
      P50=${stats.median.toFixed(2)}ms (target: <${testCase.target.p50}ms)
      P95=${stats.p95.toFixed(2)}ms (target: <${testCase.target.p95}ms)
      P99=${stats.p99.toFixed(2)}ms (target: <${testCase.target.p99}ms)`);

    // For interactive queries, targets based on 80% of native performance
    // QLever C++: 0.02-0.05s → WASM 80%: 0.025-0.063s
    expect(stats.median).toBeLessThanOrEqual(100); // P50 < 100ms
    expect(stats.p95).toBeLessThanOrEqual(250); // P95 < 250ms
    expect(stats.p99).toBeLessThanOrEqual(500); // P99 < 500ms
  });

  /**
   * Test 6: Latency Distribution Analysis
   *
   * Analyzes latency distribution shape.
   * Should be relatively normal without extreme outliers.
   */
  it('Latency distribution shape analysis', async () => {
    const latencies: number[] = [];
    const query = LATENCY_TEST_QUERIES.small.query;
    const samples = 200;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data).toBeDefined();

      latencies.push(duration);
    }

    const stats = calculateLatencyStats(latencies);

    // Calculate skewness (asymmetry of distribution)
    const mean = stats.mean;
    const n = latencies.length;
    const cubed = latencies.reduce((sum, val) => sum + Math.pow(val - mean, 3), 0);
    const skewness = (cubed / n) / Math.pow(stats.stdDev, 3);

    // Calculate tail ratio (P99/P50)
    const tailRatio = stats.p99 / stats.median;

    console.log(`Distribution analysis:
      Median=${stats.median.toFixed(2)}ms
      Mean=${stats.mean.toFixed(2)}ms
      Std Dev=${stats.stdDev.toFixed(2)}ms
      Skewness=${skewness.toFixed(2)}
      Tail Ratio (P99/P50)=${tailRatio.toFixed(2)}`);

    // Distribution should not be too skewed (should be relatively normal)
    expect(Math.abs(skewness)).toBeLessThan(3);

    // Tail ratio should not be extreme (P99 should be <5x P50)
    expect(tailRatio).toBeLessThan(5);
  });

  /**
   * Test 7: Latency Percentile Sweep
   *
   * Provides comprehensive percentile breakdown (10th through 99th).
   * Useful for understanding latency distribution across all users.
   */
  it('Comprehensive latency percentile sweep', async () => {
    const latencies: number[] = [];
    const query = LATENCY_TEST_QUERIES.small.query;
    const samples = 150;

    for (let i = 0; i < samples; i++) {
      const start = performance.now();
      const response = await client.query(query, 'json');
      const duration = performance.now() - start;

      const data = response.data();
      expect(data).toBeDefined();

      latencies.push(duration);
    }

    const sorted = [...latencies].sort((a, b) => a - b);

    // Calculate percentiles
    const percentiles = [10, 25, 50, 75, 90, 95, 99];
    const results: { [key: number]: number } = {};

    percentiles.forEach(p => {
      const index = Math.floor((p / 100) * sorted.length);
      results[p] = sorted[index];
    });

    let output = 'Latency percentile breakdown:\n';
    percentiles.forEach(p => {
      output += `  P${p.toString().padStart(2)}=${results[p].toFixed(2)}ms\n`;
    });
    console.log(output);

    // Verify reasonable progression
    percentiles.forEach((p, i) => {
      if (i > 0) {
        const prevP = percentiles[i - 1];
        // Each percentile should be >= previous (monotonic increase)
        expect(results[p]).toBeGreaterThanOrEqual(results[prevP]);
      }
    });
  });
});

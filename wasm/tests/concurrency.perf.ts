/**
 * Performance Test Suite: Concurrency & Parallelism
 *
 * Measures the QLever WASM binding's performance under concurrent load.
 * Tests scaling with multiple simultaneous queries.
 */

import { describe, it, expect, beforeAll } from 'vitest';

interface ConcurrencyMetrics {
  totalQueries: number;
  totalDuration: number;
  avgThroughput: number;
  avgLatency: number;
  minLatency: number;
  maxLatency: number;
  p95Latency: number;
  successCount: number;
  failureCount: number;
  scalingFactor: number; // Throughput / sequential throughput
}

describe('Concurrency Performance Tests', () => {
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
   * Helper: Execute queries concurrently and gather metrics
   */
  async function runConcurrentLoad(
    concurrency: number,
    queriesPerClient: number,
    query: string
  ): Promise<ConcurrencyMetrics> {
    const startTime = performance.now();
    const latencies: number[] = [];
    let successCount = 0;
    let failureCount = 0;

    // Create concurrent tasks
    const clients = Array(concurrency)
      .fill(null)
      .map(async () => {
        for (let i = 0; i < queriesPerClient; i++) {
          try {
            const queryStart = performance.now();
            const response = await client.query(query, 'json');
            const queryDuration = performance.now() - queryStart;

            const data = response.data();
            if (data.results && data.results.bindings) {
              latencies.push(queryDuration);
              successCount++;
            } else {
              failureCount++;
            }
          } catch (error) {
            failureCount++;
          }
        }
      });

    // Wait for all clients to complete
    await Promise.all(clients);
    const totalDuration = (performance.now() - startTime) / 1000;

    // Calculate metrics
    const totalQueries = successCount + failureCount;
    const avgThroughput = totalQueries / totalDuration;
    const avgLatency = latencies.reduce((a, b) => a + b, 0) / latencies.length;

    const sorted = [...latencies].sort((a, b) => a - b);
    const minLatency = sorted[0];
    const maxLatency = sorted[sorted.length - 1];
    const p95Index = Math.floor(sorted.length * 0.95);
    const p95Latency = sorted[p95Index];

    return {
      totalQueries,
      totalDuration,
      avgThroughput,
      avgLatency,
      minLatency,
      maxLatency,
      p95Latency,
      successCount,
      failureCount,
      scalingFactor: 1, // Will be calculated relative to sequential
    };
  }

  /**
   * Test 1: Sequential Baseline (1 query at a time)
   *
   * Establishes baseline throughput for comparison with concurrent tests.
   */
  it('Sequential baseline performance', async () => {
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const metrics = await runConcurrentLoad(1, 100, query);

    console.log(`Sequential baseline:
      Throughput: ${metrics.avgThroughput.toFixed(2)} q/s
      Avg latency: ${metrics.avgLatency.toFixed(2)}ms
      P95 latency: ${metrics.p95Latency.toFixed(2)}ms
      Duration: ${metrics.totalDuration.toFixed(2)}s`);

    expect(metrics.successCount).toBe(100);
    expect(metrics.failureCount).toBe(0);
    expect(metrics.avgThroughput).toBeGreaterThan(10); // At least 10 q/s sequential
  });

  /**
   * Test 2: Dual Concurrent Queries (2 parallel)
   *
   * Tests performance with 2 concurrent queries.
   * Baseline for parallelism benefits.
   */
  it('Dual concurrent queries (2 parallel)', async () => {
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const metrics = await runConcurrentLoad(2, 50, query);

    console.log(`Dual concurrent (2 parallel):
      Total throughput: ${metrics.avgThroughput.toFixed(2)} q/s
      Avg latency: ${metrics.avgLatency.toFixed(2)}ms
      P95 latency: ${metrics.p95Latency.toFixed(2)}ms`);

    expect(metrics.successCount).toBeGreaterThan(90); // Allow 1-2 failures
    expect(metrics.avgThroughput).toBeGreaterThan(20); // Should be > 2x sequential if parallel
  });

  /**
   * Test 3: Multiple Concurrent Queries (5 parallel)
   *
   * Tests with 5 concurrent queries.
   * Shows scaling behavior.
   */
  it('Multiple concurrent queries (5 parallel)', async () => {
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const metrics = await runConcurrentLoad(5, 20, query);

    console.log(`Multiple concurrent (5 parallel):
      Total throughput: ${metrics.avgThroughput.toFixed(2)} q/s
      Avg latency: ${metrics.avgLatency.toFixed(2)}ms
      Success rate: ${((metrics.successCount / metrics.totalQueries) * 100).toFixed(1)}%`);

    expect(metrics.successCount).toBeGreaterThan(90); // 90% success rate
    expect(metrics.avgThroughput).toBeGreaterThan(40); // 40+ q/s with 5 clients
  });

  /**
   * Test 4: High Concurrency (10 parallel)
   *
   * Tests performance under higher concurrent load.
   * Identifies bottlenecks or contention.
   */
  it('High concurrency (10 parallel queries)', async () => {
    const query = 'SELECT ?s ?p WHERE { ?s ?p ?o } LIMIT 50';
    const metrics = await runConcurrentLoad(10, 10, query);

    console.log(`High concurrency (10 parallel):
      Total throughput: ${metrics.avgThroughput.toFixed(2)} q/s
      Avg latency: ${metrics.avgLatency.toFixed(2)}ms
      Max latency: ${metrics.maxLatency.toFixed(2)}ms
      P95 latency: ${metrics.p95Latency.toFixed(2)}ms`);

    expect(metrics.successCount).toBeGreaterThan(80); // 80%+ success
    expect(metrics.avgThroughput).toBeGreaterThan(50); // Sustained >50 q/s
  });

  /**
   * Test 5: Extreme Concurrency (20 parallel)
   *
   * Tests performance under extreme concurrent load.
   * Identifies when system becomes saturated.
   */
  it('Extreme concurrency (20 parallel queries)', async () => {
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 20';
    const metrics = await runConcurrentLoad(20, 5, query);

    console.log(`Extreme concurrency (20 parallel):
      Total throughput: ${metrics.avgThroughput.toFixed(2)} q/s
      Avg latency: ${metrics.avgLatency.toFixed(2)}ms
      P95 latency: ${metrics.p95Latency.toFixed(2)}ms
      Success rate: ${((metrics.successCount / metrics.totalQueries) * 100).toFixed(1)}%`);

    // Under extreme load, success rate may drop slightly
    expect(metrics.successCount).toBeGreaterThan(70); // 70%+ success

    // System should still handle meaningful throughput
    expect(metrics.avgThroughput).toBeGreaterThan(30);
  });

  /**
   * Test 6: Mixed Complexity Under Concurrency
   *
   * Tests concurrent execution with varying query complexities.
   * More realistic than homogeneous query workload.
   */
  it('Mixed complexity concurrent workload', async () => {
    const startTime = performance.now();
    const latencies: number[] = [];

    const simpleQuery = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const complexQuery = `
      SELECT ?s (COUNT(?o) AS ?count) WHERE {
        ?s ?p ?o .
        FILTER (regex(str(?s), "wiki"))
      }
      GROUP BY ?s
      LIMIT 50
    `;

    // Mix 70% simple, 30% complex queries
    const queries = [
      ...Array(70).fill(simpleQuery),
      ...Array(30).fill(complexQuery),
    ];

    // Shuffle queries
    for (let i = queries.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [queries[i], queries[j]] = [queries[j], queries[i]];
    }

    // Execute concurrently in batches of 5
    const concurrency = 5;
    for (let i = 0; i < queries.length; i += concurrency) {
      const batch = queries.slice(i, i + concurrency);

      const promises = batch.map(async query => {
        try {
          const qStart = performance.now();
          const response = await client.query(query, 'json');
          const duration = performance.now() - qStart;

          const data = response.data();
          if (data.results && data.results.bindings) {
            latencies.push(duration);
            return true;
          }
          return false;
        } catch {
          return false;
        }
      });

      await Promise.all(promises);
    }

    const totalDuration = (performance.now() - startTime) / 1000;
    const throughput = queries.length / totalDuration;
    const avgLatency = latencies.length > 0 ? latencies.reduce((a, b) => a + b) / latencies.length : 0;

    console.log(`Mixed complexity (70% simple, 30% complex):
      Total queries: ${queries.length}
      Throughput: ${throughput.toFixed(2)} q/s
      Avg latency: ${avgLatency.toFixed(2)}ms
      Duration: ${totalDuration.toFixed(2)}s`);

    expect(latencies.length).toBeGreaterThan(80); // 80%+ success rate
    expect(throughput).toBeGreaterThan(30);
  });

  /**
   * Test 7: Sustained Concurrent Load (30 sec test)
   *
   * Measures sustained performance over extended period.
   * Detects performance degradation or resource exhaustion.
   */
  it('Sustained concurrent load (30 seconds)', async () => {
    const concurrency = 5;
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const testDuration = 30000; // 30 seconds
    const startTime = performance.now();
    const latencies: number[] = [];
    let successCount = 0;
    let failureCount = 0;

    // Track throughput by second
    const secondlyThroughput: number[] = [];
    let currentSecondQueries = 0;
    let lastSecondMark = startTime;

    const workers = Array(concurrency)
      .fill(null)
      .map(async () => {
        while (performance.now() - startTime < testDuration) {
          try {
            const qStart = performance.now();
            const response = await client.query(query, 'json');
            const duration = performance.now() - qStart;

            const data = response.data();
            if (data.results && data.results.bindings) {
              latencies.push(duration);
              successCount++;
              currentSecondQueries++;
            } else {
              failureCount++;
            }

            // Track second-by-second throughput
            const now = performance.now();
            if (now - lastSecondMark >= 1000) {
              secondlyThroughput.push(currentSecondQueries);
              currentSecondQueries = 0;
              lastSecondMark = now;
            }
          } catch {
            failureCount++;
          }
        }
      });

    await Promise.all(workers);
    const actualDuration = (performance.now() - startTime) / 1000;

    const totalQueries = successCount + failureCount;
    const avgThroughput = totalQueries / actualDuration;
    const avgLatency = latencies.length > 0 ? latencies.reduce((a, b) => a + b) / latencies.length : 0;

    const minThroughput = Math.min(...secondlyThroughput);
    const maxThroughput = Math.max(...secondlyThroughput);
    const avgSecondThroughput = secondlyThroughput.reduce((a, b) => a + b) / secondlyThroughput.length;

    console.log(`Sustained load (30s, ${concurrency} concurrent):
      Total queries: ${totalQueries}
      Avg throughput: ${avgThroughput.toFixed(2)} q/s
      Avg latency: ${avgLatency.toFixed(2)}ms
      Throughput range: ${minThroughput}-${maxThroughput} q/s per second
      Success rate: ${((successCount / totalQueries) * 100).toFixed(1)}%`);

    expect(successCount).toBeGreaterThan(failureCount); // More successes than failures
    expect(avgThroughput).toBeGreaterThan(25); // At least 25 q/s sustained
    expect(Math.abs(maxThroughput - minThroughput)).toBeLessThan(maxThroughput * 0.5); // <50% variance
  });

  /**
   * Test 8: Scalability Analysis
   *
   * Tests throughput scaling across different concurrency levels.
   * Identifies optimal concurrency and saturation point.
   */
  it('Scalability across concurrency levels', async () => {
    const concurrencyLevels = [1, 2, 4, 8];
    const results: { concurrency: number; throughput: number }[] = [];

    for (const concurrency of concurrencyLevels) {
      const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 50';
      const metrics = await runConcurrentLoad(concurrency, Math.ceil(50 / concurrency), query);

      results.push({
        concurrency,
        throughput: metrics.avgThroughput,
      });

      console.log(`Concurrency ${concurrency}: ${metrics.avgThroughput.toFixed(2)} q/s`);
    }

    // Verify scaling (throughput should increase with concurrency)
    for (let i = 1; i < results.length; i++) {
      // Allow small variations but should generally scale up
      const prevThroughput = results[i - 1].throughput;
      const currentThroughput = results[i].throughput;

      // Should not degrade significantly
      expect(currentThroughput).toBeGreaterThan(prevThroughput * 0.8);
    }
  });
});

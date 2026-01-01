/**
 * Performance Test Suite: Memory Efficiency
 *
 * Measures memory usage and detects memory leaks in the QLever WASM binding.
 * Tracks heap memory growth during extended query execution.
 */

import { describe, it, expect, beforeAll } from 'vitest';

interface MemorySnapshot {
  timestamp: number;
  heapUsed: number;
  heapTotal: number;
  external: number;
  rss: number;
}

/**
 * Take a memory snapshot and force garbage collection if available
 */
function takeMemorySnapshot(): MemorySnapshot {
  // Force garbage collection if available
  if (global.gc) {
    global.gc();
  }

  const mem = process.memoryUsage();
  return {
    timestamp: Date.now(),
    heapUsed: mem.heapUsed,
    heapTotal: mem.heapTotal,
    external: mem.external || 0,
    rss: mem.rss,
  };
}

/**
 * Calculate memory statistics
 */
interface MemoryStats {
  initialHeap: number;
  peakHeap: number;
  finalHeap: number;
  growthMB: number;
  avgHeap: number;
  leakEstimate: number; // Estimated memory leak in MB
}

function analyzeMemoryGrowth(snapshots: MemorySnapshot[]): MemoryStats {
  const heaps = snapshots.map(s => s.heapUsed);
  const minHeap = Math.min(...heaps);
  const maxHeap = Math.max(...heaps);
  const avgHeap = heaps.reduce((a, b) => a + b) / heaps.length;

  const initialHeap = heaps[0];
  const finalHeap = heaps[heaps.length - 1];
  const peakHeap = maxHeap;

  // Estimate memory leak as growth in minimum heap over time
  const startMin = heaps.slice(0, Math.floor(heaps.length * 0.2));
  const endMin = heaps.slice(Math.ceil(heaps.length * 0.8));
  const minStart = Math.min(...startMin);
  const minEnd = Math.min(...endMin);
  const leakEstimate = Math.max(0, (minEnd - minStart) / (1024 * 1024));

  return {
    initialHeap: initialHeap / (1024 * 1024),
    peakHeap: peakHeap / (1024 * 1024),
    finalHeap: finalHeap / (1024 * 1024),
    growthMB: (finalHeap - initialHeap) / (1024 * 1024),
    avgHeap: avgHeap / (1024 * 1024),
    leakEstimate,
  };
}

describe('Memory Efficiency Tests', () => {
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
   * Test 1: Baseline Memory Usage
   *
   * Establishes baseline heap memory without any queries.
   * Used as reference for leak detection.
   */
  it('Baseline memory usage', async () => {
    const snapshot = takeMemorySnapshot();

    const heapMB = snapshot.heapUsed / (1024 * 1024);
    const totalMB = snapshot.heapTotal / (1024 * 1024);

    console.log(`Baseline memory:
      Heap used: ${heapMB.toFixed(2)}MB
      Heap total: ${totalMB.toFixed(2)}MB
      RSS: ${(snapshot.rss / (1024 * 1024)).toFixed(2)}MB`);

    // Heap should be reasonable size for WASM module
    expect(heapMB).toBeLessThan(100); // Should use <100MB baseline
  });

  /**
   * Test 2: Single Query Memory Impact
   *
   * Measures memory usage delta for a single query execution.
   * Detects if queries leak memory on a per-query basis.
   */
  it('Single query memory impact', async () => {
    const before = takeMemorySnapshot();

    const query = 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000';
    const response = await client.query(query, 'json');
    const data = response.data();

    expect(data.results.bindings).toBeDefined();

    const after = takeMemorySnapshot();

    const deltaHeap = (after.heapUsed - before.heapUsed) / (1024 * 1024);
    const peakHeap = (after.heapTotal / (1024 * 1024));

    console.log(`Single query memory impact:
      Delta: ${deltaHeap > 0 ? '+' : ''}${deltaHeap.toFixed(2)}MB
      Heap total: ${peakHeap.toFixed(2)}MB
      Result size: ${data.results.bindings.length} bindings`);

    // Expect reasonable memory per query
    // Even large result sets should not grow heap by >50MB
    expect(Math.abs(deltaHeap)).toBeLessThan(50);
  });

  /**
   * Test 3: Memory Stability Over Repeated Queries
   *
   * Executes 100 identical queries and monitors memory.
   * Detects gradual memory leaks across query batches.
   */
  it('Memory stability over 100 repeated queries', async () => {
    const snapshots: MemorySnapshot[] = [];
    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';

    // Initial snapshot
    snapshots.push(takeMemorySnapshot());

    // Execute 100 queries, snapshot every 10
    for (let i = 0; i < 100; i++) {
      const response = await client.query(query, 'json');
      const data = response.data();

      expect(data.results.bindings).toBeDefined();

      if ((i + 1) % 10 === 0) {
        snapshots.push(takeMemorySnapshot());
      }
    }

    const stats = analyzeMemoryGrowth(snapshots);

    console.log(`Memory stability (100 queries):
      Initial: ${stats.initialHeap.toFixed(2)}MB
      Peak: ${stats.peakHeap.toFixed(2)}MB
      Final: ${stats.finalHeap.toFixed(2)}MB
      Growth: ${stats.growthMB > 0 ? '+' : ''}${stats.growthMB.toFixed(2)}MB
      Estimated leak: ${stats.leakEstimate.toFixed(2)}MB`);

    // Total growth should be minimal (<10MB)
    expect(stats.growthMB).toBeLessThan(10);

    // Estimated leak should be near zero (<2MB)
    expect(stats.leakEstimate).toBeLessThan(2);
  });

  /**
   * Test 4: Memory Efficiency with Varying Result Sizes
   *
   * Tests memory usage with different query result sizes (10 to 10000 results).
   * Ensures memory growth scales appropriately with result size.
   */
  it('Memory scaling with result size', async () => {
    const resultSizes = [10, 100, 1000, 10000];
    const memoryPerResult: number[] = [];

    for (const limit of resultSizes) {
      const before = takeMemorySnapshot();

      const query = `SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT ${limit}`;
      const response = await client.query(query, 'json');
      const data = response.data();

      const after = takeMemorySnapshot();

      const memUsedMB = (after.heapUsed - before.heapUsed) / (1024 * 1024);
      const resultCount = data.results.bindings.length;
      const memPerResult = resultCount > 0 ? memUsedMB / resultCount : 0;

      memoryPerResult.push(memPerResult);

      console.log(`Result size ${resultCount}: ${memUsedMB.toFixed(2)}MB (${(memPerResult * 1000).toFixed(2)}KB/result)`);

      expect(memUsedMB).toBeLessThan(100); // Each result set <100MB
    }

    // Memory per result should be relatively consistent
    // (not grow exponentially with result size)
    const avgMemPerResult = memoryPerResult.reduce((a, b) => a + b) / memoryPerResult.length;
    memoryPerResult.forEach((mem, i) => {
      // Allow up to 3x variation from average
      expect(mem).toBeLessThan(avgMemPerResult * 3);
    });
  });

  /**
   * Test 5: Memory Leak Detection Over Extended Session
   *
   * Simulates extended usage with 500 varied queries.
   * Sensitive detector for gradual memory leaks.
   */
  it('Extended session memory leak detection', async () => {
    const snapshots: MemorySnapshot[] = [];
    const queries = [
      'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100',
      'SELECT DISTINCT ?p WHERE { ?s ?p ?o } LIMIT 50',
      'SELECT ?s (COUNT(?o) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?s LIMIT 200',
    ];

    snapshots.push(takeMemorySnapshot());

    for (let i = 0; i < 500; i++) {
      const query = queries[i % queries.length];
      const response = await client.query(query, 'json');
      const data = response.data();

      expect(data.results.bindings).toBeDefined();

      // Snapshot every 50 queries
      if ((i + 1) % 50 === 0) {
        snapshots.push(takeMemorySnapshot());
      }
    }

    const stats = analyzeMemoryGrowth(snapshots);

    console.log(`Extended session (500 queries):
      Initial heap: ${stats.initialHeap.toFixed(2)}MB
      Peak heap: ${stats.peakHeap.toFixed(2)}MB
      Final heap: ${stats.finalHeap.toFixed(2)}MB
      Total growth: ${stats.growthMB > 0 ? '+' : ''}${stats.growthMB.toFixed(2)}MB
      Leak estimate: ${stats.leakEstimate.toFixed(2)}MB
      Avg heap: ${stats.avgHeap.toFixed(2)}MB`);

    // Heap growth should be linear or decreasing (with GC)
    // Allow up to 20MB growth for 500 queries
    expect(stats.growthMB).toBeLessThan(20);

    // Estimated leak should be minimal
    expect(stats.leakEstimate).toBeLessThan(5);
  });

  /**
   * Test 6: Result Set Retention
   *
   * Verifies that result objects don't retain memory unnecessarily.
   * Tests if multiple large queries can be executed without accumulation.
   */
  it('Result set memory release', async () => {
    const snapshots: MemorySnapshot[] = [];

    snapshots.push(takeMemorySnapshot());

    // Execute large query, then discard result
    for (let i = 0; i < 20; i++) {
      const query = 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5000';
      const response = await client.query(query, 'json');
      const data = response.data();

      expect(data.results.bindings.length).toBeGreaterThan(0);
      // Let garbage collection happen
    }

    // Force GC and take final snapshot
    if (global.gc) global.gc();
    snapshots.push(takeMemorySnapshot());

    const stats = analyzeMemoryGrowth(snapshots);

    console.log(`Result retention test:
      Initial: ${stats.initialHeap.toFixed(2)}MB
      Final: ${stats.finalHeap.toFixed(2)}MB
      Growth: ${stats.growthMB > 0 ? '+' : ''}${stats.growthMB.toFixed(2)}MB`);

    // After GC, heap should return close to baseline
    // Allow 5MB variance
    expect(stats.finalHeap).toBeLessThan(stats.initialHeap + 5);
  });

  /**
   * Test 7: Concurrent Query Memory Impact
   *
   * Tests memory behavior when executing multiple queries concurrently.
   * Ensures WASM module handles concurrent memory allocation properly.
   */
  it('Concurrent query memory impact', async () => {
    const before = takeMemorySnapshot();

    const query = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';
    const concurrentCount = 10;

    // Fire off concurrent queries
    const promises = Array(concurrentCount)
      .fill(null)
      .map(() =>
        client.query(query, 'json').then((response: any) => response.data())
      );

    const results = await Promise.all(promises);

    const after = takeMemorySnapshot();

    results.forEach(data => {
      expect(data.results.bindings).toBeDefined();
    });

    const peakHeapMB = (after.heapTotal / (1024 * 1024));
    const usedHeapMB = (after.heapUsed / (1024 * 1024));
    const deltaHeapMB = (after.heapUsed - before.heapUsed) / (1024 * 1024);

    console.log(`Concurrent queries (${concurrentCount}):
      Heap used: ${usedHeapMB.toFixed(2)}MB
      Heap total: ${peakHeapMB.toFixed(2)}MB
      Delta: ${deltaHeapMB > 0 ? '+' : ''}${deltaHeapMB.toFixed(2)}MB`);

    // Concurrent queries should not cause excessive memory growth
    // 10 queries should not require >100MB additional memory
    expect(deltaHeapMB).toBeLessThan(100);
  });

  /**
   * Test 8: Memory per WASM Module Instance
   *
   * Estimates memory footprint of the WASM module itself.
   * Useful for deployment planning.
   */
  it('WASM module memory footprint', async () => {
    const wasmModule = await import('qlever-wasm/node');

    const before = takeMemorySnapshot();

    // Module is already loaded, but measure current state
    const snapshot = takeMemorySnapshot();

    const moduleSizeMB = (snapshot.heapUsed / (1024 * 1024));
    const externalMB = (snapshot.external / (1024 * 1024));

    console.log(`WASM module footprint:
      Heap used: ${moduleSizeMB.toFixed(2)}MB
      External (WASM): ${externalMB.toFixed(2)}MB
      Total: ${(moduleSizeMB + externalMB).toFixed(2)}MB`);

    // WASM module should have reasonable footprint
    expect(moduleSizeMB).toBeLessThan(500); // <500MB heap
  });
});

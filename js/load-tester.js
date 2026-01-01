const axios = require('axios');
const { ResilienceHandler } = require('./resilience-layer');
const { ContractEnforcer } = require('./contract-enforcer');

class LoadTester {
  constructor(options = {}) {
    this.config = {
      duration: options.duration || 30000,
      concurrency: options.concurrency || 10,
      querySize: options.querySize || 10,
      thinkTime: options.thinkTime || 100
    };

    this.resilience = new ResilienceHandler({
      maxRetries: 3,
      retryDelay: 50
    });

    this.enforcer = new ContractEnforcer();

    this.metrics = {
      startTime: null,
      endTime: null,
      totalRequests: 0,
      successfulRequests: 0,
      failedRequests: 0,
      contractViolations: 0,
      averageLatency: 0,
      maxLatency: 0,
      minLatency: Infinity,
      latencies: [],
      failuresByType: {},
      throughput: 0,
      errors: []
    };

    this.http = axios.create({
      baseURL: 'http://localhost:3000',
      timeout: 10000
    });
  }

  async generateLoad() {
    this.metrics.startTime = Date.now();
    const endTime = this.metrics.startTime + this.config.duration;

    const workers = [];
    for (let i = 0; i < this.config.concurrency; i++) {
      workers.push(this.worker(endTime));
    }

    await Promise.allSettled(workers);
    this.metrics.endTime = Date.now();

    this.calculateMetrics();
  }

  async worker(endTime) {
    const handle = await this.openSession();
    if (!handle) return;

    while (Date.now() < endTime) {
      try {
        await this.executeQuery(handle);
        await this.delay(this.config.thinkTime);
      } catch (error) {
        if (!error.message.includes('timeout')) {
          console.error('Worker error:', error.message);
        }
      }
    }

    await this.closeSession(handle);
  }

  async openSession() {
    try {
      const res = await this.resilience.executeWithResilience(
        () => this.http.post('/api/open', { indexPath: './index' }),
        'open-session'
      );
      return res.data.handle;
    } catch (error) {
      console.error('Failed to open session:', error.message);
      return null;
    }
  }

  async closeSession(handle) {
    try {
      await this.http.post('/api/close', { handle });
    } catch (error) {
      console.error('Failed to close session:', error.message);
    }
  }

  async executeQuery(handle) {
    const startTime = Date.now();
    this.metrics.totalRequests++;

    try {
      const res = await this.resilience.executeWithResilience(
        () => this.http.post('/api/query', {
          handle,
          sparql: `SELECT ?s WHERE { ?s ?p ?o } LIMIT ${this.config.querySize}`,
          timings: 1
        }),
        'execute-query'
      );

      const latency = Date.now() - startTime;
      this.metrics.latencies.push(latency);
      this.metrics.maxLatency = Math.max(this.metrics.maxLatency, latency);
      this.metrics.minLatency = Math.min(this.metrics.minLatency, latency);

      try {
        this.enforcer.assertStrict('HTTP.Response.Structure', res.data);
        this.metrics.successfulRequests++;
      } catch (violation) {
        this.metrics.contractViolations++;
        this.metrics.failedRequests++;
      }
    } catch (error) {
      this.metrics.failedRequests++;
      const failureType = this.resilience.classifyFailure(error);
      this.metrics.failuresByType[failureType] = (this.metrics.failuresByType[failureType] || 0) + 1;
      this.metrics.errors.push(error.message);
    }
  }

  calculateMetrics() {
    const totalTime = this.metrics.endTime - this.metrics.startTime;
    this.metrics.averageLatency =
      this.metrics.latencies.length > 0
        ? this.metrics.latencies.reduce((a, b) => a + b, 0) / this.metrics.latencies.length
        : 0;

    this.metrics.throughput = (this.metrics.successfulRequests / (totalTime / 1000)).toFixed(2);
  }

  printReport() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║              LOAD TEST REPORT                          ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const totalTime = this.metrics.endTime - this.metrics.startTime;
    const successRate = ((this.metrics.successfulRequests / this.metrics.totalRequests) * 100).toFixed(1);

    console.log(`   Duration: ${totalTime}ms`);
    console.log(`   Total Requests: ${this.metrics.totalRequests}`);
    console.log(`   Successful: ${this.metrics.successfulRequests}`);
    console.log(`   Failed: ${this.metrics.failedRequests}`);
    console.log(`   Contract Violations: ${this.metrics.contractViolations}`);
    console.log(`   Success Rate: ${successRate}%\n`);

    console.log('   Latency:');
    console.log(`     Min: ${this.metrics.minLatency}ms`);
    console.log(`     Max: ${this.metrics.maxLatency}ms`);
    console.log(`     Avg: ${this.metrics.averageLatency.toFixed(2)}ms\n`);

    console.log(`   Throughput: ${this.metrics.throughput} req/s\n`);

    if (Object.keys(this.metrics.failuresByType).length > 0) {
      console.log('   Failures by Type:');
      for (const [type, count] of Object.entries(this.metrics.failuresByType)) {
        console.log(`     ${type}: ${count}`);
      }
      console.log();
    }

    const percentiles = this.calculatePercentiles();
    if (percentiles) {
      console.log('   Latency Percentiles:');
      console.log(`     P50: ${percentiles.p50}ms`);
      console.log(`     P95: ${percentiles.p95}ms`);
      console.log(`     P99: ${percentiles.p99}ms\n`);
    }

    console.log(`   Resilience Recovery: ${this.resilience.recoveryLog.length} recoveries`);
    console.log(`   System Health: ${successRate >= 95 ? '✓ EXCELLENT' : successRate >= 80 ? '✓ GOOD' : '⚠ DEGRADED'}\n`);

    const fs = require('fs');
    fs.writeFileSync('load-test-report.json', JSON.stringify(this.metrics, null, 2));
  }

  calculatePercentiles() {
    if (this.metrics.latencies.length === 0) return null;

    const sorted = this.metrics.latencies.sort((a, b) => a - b);
    const p50Idx = Math.floor(sorted.length * 0.5);
    const p95Idx = Math.floor(sorted.length * 0.95);
    const p99Idx = Math.floor(sorted.length * 0.99);

    return {
      p50: sorted[p50Idx],
      p95: sorted[p95Idx],
      p99: sorted[p99Idx]
    };
  }

  async run() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║           LOAD TESTING FRAMEWORK                       ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    console.log(`   Configuration:`);
    console.log(`     Duration: ${this.config.duration}ms`);
    console.log(`     Concurrency: ${this.config.concurrency} workers`);
    console.log(`     Query Size: ${this.config.querySize} results`);
    console.log(`     Think Time: ${this.config.thinkTime}ms\n`);

    console.log('   Starting load test...\n');

    try {
      await this.generateLoad();
      this.printReport();
      this.printResilienceReport();
    } catch (error) {
      console.error('Load test error:', error);
    }
  }

  printResilienceReport() {
    console.log('╔════════════════════════════════════════════════════════╗');
    console.log('║         RESILIENCE BEHAVIOR DURING LOAD                ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    this.resilience.printReport();
  }

  delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
}

const tester = new LoadTester({
  duration: 30000,
  concurrency: 5,
  querySize: 10,
  thinkTime: 100
});

tester.run().catch(console.error);

module.exports = LoadTester;

const ProtocolValidator = require('./protocol-validator');
const StressTester = require('./stress-tester');
const { ContractEnforcer } = require('./contract-enforcer');
const axios = require('axios');

class SystemDiagnostics {
  constructor() {
    this.diagnostics = {
      timestamp: new Date().toISOString(),
      systemHealth: {
        protocols: {},
        contracts: {},
        boundaries: {},
        criticalPaths: []
      },
      failures: [],
      recommendations: []
    };
  }

  async diagnoseProtocolHealth() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║          SYSTEM DIAGNOSTICS - PROTOCOL LAYER            ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const protocols = [
      { name: 'HTTP', url: 'http://localhost:3000/health', port: 3000 },
      { name: 'WebSocket', url: 'http://localhost:3002/health', port: 3002 },
      { name: 'GraphQL', url: 'http://localhost:3003/health', port: 3003 },
      { name: 'gRPC', port: 50051, protocol: 'grpc' }
    ];

    for (const proto of protocols) {
      const health = {
        name: proto.name,
        available: false,
        latency: null,
        responseTime: null,
        errors: []
      };

      try {
        const start = Date.now();
        if (proto.protocol === 'grpc') {
          await this.checkGrpcHealth(proto.port);
        } else {
          const res = await axios.get(proto.url, { timeout: 5000 });
          health.responseTime = Date.now() - start;
          health.available = res.status === 200;
        }
        health.latency = Date.now() - start;
        console.log(`   ✓ ${proto.name}: ${health.latency}ms`);
      } catch (error) {
        health.errors.push(error.message);
        console.log(`   ✗ ${proto.name}: ${error.message}`);
        this.diagnostics.failures.push({
          layer: 'protocol',
          protocol: proto.name,
          error: error.message
        });
      }

      this.diagnostics.systemHealth.protocols[proto.name] = health;
    }
  }

  async checkGrpcHealth(port) {
    const GrpcClient = require('./grpc-client');
    const client = new GrpcClient(`localhost:${port}`);
    await client.openIndex('./test');
    await client.close().catch(() => {});
  }

  async diagnoseBoundaries() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║       SYSTEM DIAGNOSTICS - LANGUAGE BOUNDARIES          ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const boundaries = [
      {
        name: 'JavaScript ↔ HTTP Server',
        test: async () => {
          const res = await axios.post('http://localhost:3000/api/open', { indexPath: './index' });
          return {
            data: res.data,
            serializable: typeof res.data === 'object',
            schema: Object.keys(res.data)
          };
        }
      },
      {
        name: 'HTTP ↔ C++ FFI',
        test: async () => {
          const res = await axios.post('http://localhost:3000/api/query', {
            handle: 1,
            sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1',
            timings: 1
          });
          return {
            dataTypes: {
              bindings: typeof res.data.results.bindings[0],
              timings: typeof res.data.timings?.query_ms
            },
            schema: {
              head: Object.keys(res.data.head),
              results: Object.keys(res.data.results)
            }
          };
        }
      }
    ];

    for (const boundary of boundaries) {
      try {
        const result = await boundary.test();
        console.log(`   ✓ ${boundary.name}`);
        console.log(`     Schema: ${JSON.stringify(result.schema || result.dataTypes).substring(0, 50)}...`);
        this.diagnostics.systemHealth.boundaries[boundary.name] = { status: 'ok', ...result };
      } catch (error) {
        console.log(`   ✗ ${boundary.name}: ${error.message}`);
        this.diagnostics.systemHealth.boundaries[boundary.name] = { status: 'failed', error: error.message };
        this.diagnostics.failures.push({
          layer: 'boundary',
          boundary: boundary.name,
          error: error.message
        });
      }
    }
  }

  async diagnoseCriticalPaths() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║       SYSTEM DIAGNOSTICS - CRITICAL PATHS                ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const criticalPaths = [
      {
        name: 'Index Lifecycle',
        operations: [
          { op: 'open', call: () => axios.post('http://localhost:3000/api/open', { indexPath: './index' }) },
          { op: 'query', call: (h) => axios.post('http://localhost:3000/api/query', { handle: h, sparql: 'SELECT ?s WHERE { ?s ?p ?o }', timings: 1 }) },
          { op: 'close', call: (h) => axios.post('http://localhost:3000/api/close', { handle: h }) }
        ]
      },
      {
        name: 'Query Planning Pipeline',
        operations: [
          { op: 'open', call: () => axios.post('http://localhost:3000/api/open', { indexPath: './index' }) },
          { op: 'plan', call: (h) => axios.post('http://localhost:3000/api/parse-and-plan', { handle: h, sparql: 'SELECT ?s WHERE { ?s ?p ?o }' }) },
          { op: 'execute', call: (h, p) => axios.post('http://localhost:3000/api/execute-plan', { handle: h, planId: p, timings: 1 }) },
          { op: 'close', call: (h) => axios.post('http://localhost:3000/api/close', { handle: h }) }
        ]
      }
    ];

    for (const path of criticalPaths) {
      const pathHealth = {
        name: path.name,
        steps: [],
        status: 'ok',
        totalTime: 0
      };

      let handle = null;
      let planId = null;
      let startTime = Date.now();

      for (const { op, call } of path.operations) {
        try {
          const opStart = Date.now();
          const result = await call(handle, planId);
          const opTime = Date.now() - opStart;

          if (op === 'open') {
            handle = result.data.handle;
          } else if (op === 'plan') {
            planId = result.data.planId;
          }

          pathHealth.steps.push({
            operation: op,
            time: opTime,
            status: 'ok'
          });

          console.log(`   ✓ ${path.name} → ${op} (${opTime}ms)`);
        } catch (error) {
          pathHealth.steps.push({
            operation: op,
            status: 'failed',
            error: error.message
          });
          pathHealth.status = 'failed';

          console.log(`   ✗ ${path.name} → ${op}: ${error.message}`);
          this.diagnostics.failures.push({
            layer: 'critical-path',
            path: path.name,
            operation: op,
            error: error.message
          });

          break;
        }
      }

      pathHealth.totalTime = Date.now() - startTime;
      this.diagnostics.systemHealth.criticalPaths.push(pathHealth);
    }
  }

  analyzeFailures() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║           FAILURE ANALYSIS & RECOMMENDATIONS            ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    if (this.diagnostics.failures.length === 0) {
      console.log('   ✓ NO FAILURES DETECTED\n');
      return;
    }

    const byLayer = {};
    for (const failure of this.diagnostics.failures) {
      if (!byLayer[failure.layer]) {
        byLayer[failure.layer] = [];
      }
      byLayer[failure.layer].push(failure);
    }

    for (const [layer, failures] of Object.entries(byLayer)) {
      console.log(`   ${layer.toUpperCase()} (${failures.length} issues)`);

      for (const failure of failures) {
        const details = failure.protocol || failure.boundary || failure.path;
        console.log(`     - ${details}: ${failure.error}`);

        let recommendation = this.getRecommendation(failure);
        if (recommendation) {
          console.log(`       → ${recommendation}`);
          this.diagnostics.recommendations.push({
            failure: details,
            recommendation
          });
        }
      }
    }
  }

  getRecommendation(failure) {
    const error = failure.error.toLowerCase();

    if (error.includes('econnrefused') || error.includes('timeout')) {
      return `Service not running on expected port. Check if ${failure.protocol || failure.boundary} is started.`;
    }

    if (error.includes('invalid handle') || error.includes('invalid')) {
      return `Input validation failed. Ensure handle/query parameters are correct format.`;
    }

    if (error.includes('serializ')) {
      return `Data serialization failed. Check that messages match expected schema.`;
    }

    if (error.includes('memory') || error.includes('oom')) {
      return `Out of memory condition detected. Reduce query size or increase available memory.`;
    }

    if (error.includes('timeout')) {
      return `Operation timed out. Consider increasing timeout or optimizing query complexity.`;
    }

    return null;
  }

  printSummary() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║                DIAGNOSTIC SUMMARY                       ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const protocolCount = Object.keys(this.diagnostics.systemHealth.protocols).length;
    const availableProtocols = Object.values(this.diagnostics.systemHealth.protocols).filter(p => p.available).length;

    console.log(`   Protocols: ${availableProtocols}/${protocolCount} available`);
    console.log(`   Failures detected: ${this.diagnostics.failures.length}`);
    console.log(`   Recommendations: ${this.diagnostics.recommendations.length}`);

    const systemHealth = availableProtocols === protocolCount && this.diagnostics.failures.length === 0;
    console.log(`\n   System Health: ${systemHealth ? '✓ HEALTHY' : '✗ DEGRADED'}\n`);

    const fs = require('fs');
    fs.writeFileSync(
      'system-diagnostics.json',
      JSON.stringify(this.diagnostics, null, 2)
    );
    console.log('   Full diagnostics saved to system-diagnostics.json\n');
  }

  async runDiagnostics() {
    try {
      await this.diagnoseProtocolHealth();
      await this.diagnoseBoundaries();
      await this.diagnoseCriticalPaths();
      this.analyzeFailures();
      this.printSummary();
    } catch (e) {
      console.error('Diagnostic error:', e);
    }
  }
}

const diagnostics = new SystemDiagnostics();
diagnostics.runDiagnostics().catch(console.error);

module.exports = SystemDiagnostics;

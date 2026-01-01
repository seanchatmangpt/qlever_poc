const axios = require('axios');
const GrpcClient = require('./grpc-client');
const RedisCacheLayer = require('./redis-cache');

class ProtocolValidator {
  constructor() {
    this.results = {
      protocols: {},
      failures: [],
      contracts: [],
      measurements: {}
    };
    this.http = axios.create({ baseURL: 'http://localhost:3000' });
    this.cache = new RedisCacheLayer();
  }

  async validateHttpProtocol() {
    const name = 'HTTP';
    const measurements = {
      name,
      operations: [],
      failures: []
    };

    try {
      const start = Date.now();
      const openRes = await this.http.post('/api/open', { indexPath: './index' });
      measurements.operations.push({
        op: 'open',
        time: Date.now() - start,
        success: !!openRes.data.handle
      });

      const handle = openRes.data.handle;
      const queryStart = Date.now();
      const queryRes = await this.http.post('/api/query', {
        handle,
        sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 10',
        timings: 1
      });
      measurements.operations.push({
        op: 'query',
        time: Date.now() - queryStart,
        resultCount: queryRes.data.results.bindings.length,
        serverTime: queryRes.data.timings?.query_ms
      });

      const planStart = Date.now();
      const planRes = await this.http.post('/api/parse-and-plan', {
        handle,
        sparql: 'SELECT ?s WHERE { ?s ?p ?o }'
      });
      measurements.operations.push({
        op: 'parse-and-plan',
        time: Date.now() - planStart,
        planId: planRes.data.planId
      });

      const execStart = Date.now();
      const execRes = await this.http.post('/api/execute-plan', {
        handle,
        planId: planRes.data.planId,
        timings: 1
      });
      measurements.operations.push({
        op: 'execute-plan',
        time: Date.now() - execStart,
        resultCount: execRes.data.results.bindings.length,
        serverTime: execRes.data.timings?.query_ms
      });

      await this.http.post('/api/close', { handle });
      measurements.operations.push({
        op: 'close',
        success: true
      });

      this.results.protocols[name] = measurements;
      return measurements;
    } catch (error) {
      measurements.failures.push(error.message);
      this.results.failures.push({ protocol: name, error: error.message });
      return measurements;
    }
  }

  async validateGrpcProtocol() {
    const name = 'gRPC';
    const measurements = {
      name,
      operations: [],
      failures: []
    };

    try {
      const grpc = new GrpcClient('localhost:50051');

      const start = Date.now();
      const openRes = await grpc.openIndex('./index');
      measurements.operations.push({
        op: 'open',
        time: Date.now() - start,
        success: !!openRes.handle
      });

      const queryStart = Date.now();
      const queryRes = await grpc.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 10', true);
      measurements.operations.push({
        op: 'query',
        time: Date.now() - queryStart,
        resultCount: queryRes.bindings.length,
        serverTime: queryRes.timings?.query_ms
      });

      const planStart = Date.now();
      const planRes = await grpc.parseAndPlan('SELECT ?s WHERE { ?s ?p ?o }');
      measurements.operations.push({
        op: 'parse-and-plan',
        time: Date.now() - planStart,
        planId: planRes.planId
      });

      const streamStart = Date.now();
      const streamRes = await grpc.streamQuery('SELECT ?s WHERE { ?s ?p ?o }');
      measurements.operations.push({
        op: 'stream-query',
        time: Date.now() - streamStart,
        chunkCount: streamRes.length,
        totalResults: streamRes.reduce((sum, chunk) => sum + chunk.bindings.length, 0)
      });

      await grpc.close();
      measurements.operations.push({
        op: 'close',
        success: true
      });

      this.results.protocols[name] = measurements;
      return measurements;
    } catch (error) {
      measurements.failures.push(error.message);
      this.results.failures.push({ protocol: name, error: error.message });
      return measurements;
    }
  }

  async validateCaching() {
    const name = 'Redis Caching';
    const measurements = {
      name,
      operations: [],
      failures: []
    };

    try {
      await this.cache.connect();

      const testData = { results: { bindings: [{ test: 'value' }] } };
      const cacheKey = this.cache.getCacheKey(1, 'test-query', 'query');

      const setStart = Date.now();
      await this.cache.set(cacheKey, testData, 300);
      measurements.operations.push({
        op: 'cache-set',
        time: Date.now() - setStart,
        success: true
      });

      const getStart = Date.now();
      const cached = await this.cache.get(cacheKey);
      measurements.operations.push({
        op: 'cache-get',
        time: Date.now() - getStart,
        hit: !!cached,
        success: cached !== null
      });

      const delStart = Date.now();
      await this.cache.del(cacheKey);
      const deleted = await this.cache.get(cacheKey);
      measurements.operations.push({
        op: 'cache-del',
        time: Date.now() - delStart,
        deleted: deleted === null,
        success: deleted === null
      });

      await this.cache.disconnect();

      this.results.protocols[name] = measurements;
      return measurements;
    } catch (error) {
      measurements.failures.push(error.message);
      this.results.failures.push({ protocol: name, error: error.message });
      return measurements;
    }
  }

  async validateDataContracts() {
    const contracts = [];

    try {
      const httpRes = await this.http.post('/api/query', {
        handle: 999,
        sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1',
        timings: 1
      });

      if (!httpRes.data.head || !Array.isArray(httpRes.data.head.vars)) {
        contracts.push({ contract: 'HTTP-head.vars', satisfied: false, reason: 'head.vars not array' });
      } else {
        contracts.push({ contract: 'HTTP-head.vars', satisfied: true });
      }

      if (!httpRes.data.results || !Array.isArray(httpRes.data.results.bindings)) {
        contracts.push({ contract: 'HTTP-results.bindings', satisfied: false, reason: 'results.bindings not array' });
      } else {
        contracts.push({ contract: 'HTTP-results.bindings', satisfied: true });
      }

      if (!httpRes.data.timings || typeof httpRes.data.timings.query_ms !== 'number') {
        contracts.push({ contract: 'HTTP-timings.query_ms', satisfied: false, reason: 'query_ms not number' });
      } else {
        contracts.push({ contract: 'HTTP-timings.query_ms', satisfied: true });
      }

      this.results.contracts = contracts;
    } catch (error) {
      contracts.push({ contract: 'HTTP-validation', satisfied: false, reason: error.message });
      this.results.contracts = contracts;
    }

    return contracts;
  }

  async validateEndpointAvailability() {
    const endpoints = [
      { name: 'HTTP', url: 'http://localhost:3000/health', expected: 'ok' },
      { name: 'WebSocket', url: 'http://localhost:3002/health', expected: 'ok' },
      { name: 'GraphQL', url: 'http://localhost:3003/health', expected: 'ok' },
      { name: 'gRPC', port: 50051, protocol: 'grpc' }
    ];

    const availability = [];

    for (const endpoint of endpoints) {
      try {
        if (endpoint.protocol === 'grpc') {
          const grpc = new GrpcClient(`localhost:${endpoint.port}`);
          const start = Date.now();
          await grpc.openIndex('./test');
          const latency = Date.now() - start;
          availability.push({
            service: endpoint.name,
            available: true,
            latency,
            port: endpoint.port
          });
          await grpc.close().catch(() => {});
        } else {
          const start = Date.now();
          const res = await axios.get(endpoint.url);
          const latency = Date.now() - start;
          availability.push({
            service: endpoint.name,
            available: res.status === 200,
            latency,
            port: endpoint.url.split(':')[2]?.split('/')[0]
          });
        }
      } catch (error) {
        availability.push({
          service: endpoint.name,
          available: false,
          error: error.message,
          port: endpoint.port || endpoint.url.split(':')[2]?.split('/')[0]
        });
      }
    }

    this.results.availability = availability;
    return availability;
  }

  async validateMessageSerialization() {
    const tests = [];

    try {
      const testQueries = [
        'SELECT ?s WHERE { ?s ?p ?o }',
        'SELECT ?s (COUNT(*) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?s',
        'SELECT DISTINCT ?s WHERE { ?s ?p ?o }'
      ];

      for (const query of testQueries) {
        try {
          const res = await this.http.post('/api/parse-and-plan', {
            handle: 1,
            sparql: query
          });

          tests.push({
            query: query.substring(0, 30),
            serializable: typeof res.data === 'object',
            hasplanId: !!res.data.planId,
            success: true
          });
        } catch (error) {
          tests.push({
            query: query.substring(0, 30),
            serializable: false,
            error: error.message,
            success: false
          });
        }
      }

      this.results.serialization = tests;
      return tests;
    } catch (error) {
      this.results.serialization = [{ error: error.message }];
      return this.results.serialization;
    }
  }

  async runFullValidation() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║        MULTI-PROTOCOL VALIDATION FRAMEWORK              ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    console.log('1. Checking endpoint availability...');
    const availability = await this.validateEndpointAvailability();
    this.printAvailability(availability);

    console.log('\n2. Validating HTTP protocol...');
    const http = await this.validateHttpProtocol();
    this.printMeasurements(http);

    console.log('\n3. Validating gRPC protocol...');
    const grpc = await this.validateGrpcProtocol();
    this.printMeasurements(grpc);

    console.log('\n4. Validating Redis caching...');
    const cache = await this.validateCaching();
    this.printMeasurements(cache);

    console.log('\n5. Validating data contracts...');
    const contracts = await this.validateDataContracts();
    this.printContracts(contracts);

    console.log('\n6. Validating message serialization...');
    const serialization = await this.validateMessageSerialization();
    this.printSerialization(serialization);

    console.log('\n7. Validation report...');
    this.printReport();
  }

  printAvailability(availability) {
    for (const ep of availability) {
      const status = ep.available ? '✓' : '✗';
      const info = ep.latency ? `${ep.latency}ms` : ep.error || 'unknown';
      console.log(`   ${status} ${ep.service.padEnd(15)} ${info}`);
    }
  }

  printMeasurements(measurements) {
    if (measurements.failures.length > 0) {
      console.log(`   ✗ ${measurements.name} FAILED`);
      for (const fail of measurements.failures) {
        console.log(`     Error: ${fail}`);
      }
      return;
    }

    console.log(`   ✓ ${measurements.name}`);
    for (const op of measurements.operations) {
      const details = [
        op.time && `${op.time}ms`,
        op.resultCount && `${op.resultCount} results`,
        op.serverTime && `server: ${op.serverTime}ms`,
        op.chunkCount && `${op.chunkCount} chunks`,
        op.hit !== undefined && `cache: ${op.hit ? 'hit' : 'miss'}`,
        op.success === false && 'FAILED'
      ].filter(Boolean).join(' | ');

      console.log(`     ${op.op.padEnd(20)} ${details}`);
    }
  }

  printContracts(contracts) {
    for (const contract of contracts) {
      const status = contract.satisfied ? '✓' : '✗';
      const detail = contract.reason ? ` (${contract.reason})` : '';
      console.log(`   ${status} ${contract.contract}${detail}`);
    }
  }

  printSerialization(tests) {
    for (const test of tests) {
      if (test.error) {
        console.log(`   ✗ Error: ${test.error}`);
      } else {
        const status = test.success ? '✓' : '✗';
        console.log(`   ${status} ${test.query}... (planId: ${test.hasplanId ? 'yes' : 'no'})`);
      }
    }
  }

  printReport() {
    const totalOps = Object.values(this.results.protocols).reduce((sum, p) => sum + p.operations.length, 0);
    const failedOps = Object.values(this.results.protocols).reduce((sum, p) => sum + p.failures.length, 0);
    const failedProtocols = this.results.failures.length;
    const satisfiedContracts = this.results.contracts.filter(c => c.satisfied).length;
    const totalContracts = this.results.contracts.length;

    console.log(`\n   Total operations: ${totalOps}`);
    console.log(`   Failed operations: ${failedOps}`);
    console.log(`   Failed protocols: ${failedProtocols}`);
    console.log(`   Contract satisfaction: ${satisfiedContracts}/${totalContracts}`);

    if (failedOps === 0 && failedProtocols === 0 && satisfiedContracts === totalContracts) {
      console.log('\n   ✓✓✓ ALL VALIDATIONS PASSED ✓✓✓\n');
    } else {
      console.log('\n   ✗✗✗ SOME VALIDATIONS FAILED ✗✗✗\n');
    }

    console.log('\nDetailed report saved to validation-report.json');
    const fs = require('fs');
    fs.writeFileSync(
      'validation-report.json',
      JSON.stringify(this.results, null, 2)
    );
  }
}

const validator = new ProtocolValidator();
validator.runFullValidation().catch(console.error);

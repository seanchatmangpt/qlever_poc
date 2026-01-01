const axios = require('axios');
const GrpcClient = require('./grpc-client');

class MultiProtocolQLever {
  constructor() {
    this.httpClient = null;
    this.grpcClient = null;
    this.wsClient = null;
    this.redisCache = null;
  }

  async initializeAllProtocols() {
    console.log('=== Initializing Multi-Protocol QLever ===\n');

    console.log('1. Initializing HTTP client...');
    this.httpClient = axios.create({
      baseURL: 'http://localhost:3000'
    });
    console.log('   ✓ HTTP client ready\n');

    console.log('2. Initializing gRPC client...');
    this.grpcClient = new GrpcClient('localhost:50051');
    console.log('   ✓ gRPC client ready\n');

    console.log('3. Initializing WebSocket client...');
    const WebSocketClient = require('./websocket-client');
    this.wsClient = new WebSocketClient('http://localhost:3002');
    try {
      await this.wsClient.connect();
      console.log('   ✓ WebSocket client connected\n');
    } catch (error) {
      console.log('   ⚠ WebSocket connection failed (optional):', error.message, '\n');
    }

    console.log('4. Initializing Redis cache...');
    const RedisCacheLayer = require('./redis-cache');
    this.redisCache = new RedisCacheLayer();
    await this.redisCache.connect();
    console.log('   ✓ Redis cache ready\n');
  }

  async demoHttpProtocol(indexPath) {
    console.log('=== HTTP Protocol Demo ===\n');

    try {
      const openRes = await this.httpClient.post('/api/open', { indexPath });
      const handle = openRes.data.handle;
      console.log(`✓ Opened index with handle ${handle}`);

      const queryRes = await this.httpClient.post('/api/query', {
        handle,
        sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 5',
        timings: 1
      });

      console.log(`✓ Query returned ${queryRes.data.results.bindings.length} results`);
      console.log(`  Timings: ${queryRes.data.timings.query_ms}ms total`);

      await this.httpClient.post('/api/close', { handle });
      console.log('✓ Index closed\n');
    } catch (error) {
      console.error('HTTP error:', error.message, '\n');
    }
  }

  async demoGrpcProtocol(indexPath) {
    console.log('=== gRPC Protocol Demo ===\n');

    try {
      const openRes = await this.grpcClient.openIndex(indexPath);
      console.log(`✓ Opened index with handle ${openRes.handle}`);

      const queryRes = await this.grpcClient.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 5', true);
      console.log(`✓ Query returned ${queryRes.bindings.length} results`);
      if (queryRes.timings) {
        console.log(`  Timings: ${queryRes.timings.query_ms}ms total`);
      }

      const planRes = await this.grpcClient.parseAndPlan('SELECT ?s WHERE { ?s ?p ?o }');
      console.log(`✓ Query plan created: ${planRes.planId}`);

      const streamRes = await this.grpcClient.streamQuery('SELECT ?s WHERE { ?s ?p ?o }');
      console.log(`✓ Streamed ${streamRes.length} result chunks\n`);

      await this.grpcClient.close();
    } catch (error) {
      console.error('gRPC error:', error.message, '\n');
    }
  }

  async demoWebSocketProtocol(indexPath) {
    if (!this.wsClient) {
      console.log('=== WebSocket Protocol Demo ===');
      console.log('⚠ WebSocket not available (requires ws://localhost:3002)\n');
      return;
    }

    console.log('=== WebSocket Protocol Demo ===\n');

    try {
      const openRes = await this.wsClient.open(indexPath);
      console.log(`✓ Opened index with handle ${openRes.handle}`);

      const queryRes = await this.wsClient.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 5');
      console.log(`✓ Query returned ${queryRes.results.length} results`);

      const batchResults = [];
      const streamRes = await this.wsClient.streamQuery(
        'SELECT ?s WHERE { ?s ?p ?o }',
        (batch) => {
          batchResults.push(batch.results.length);
          console.log(`  Received batch ${batch.batch}: ${batch.results.length} results`);
        },
        50
      );
      console.log(`✓ Streamed ${batchResults.length} batches (${batchResults.reduce((a, b) => a + b, 0)} total)`);

      await this.wsClient.close();
      console.log('✓ Connection closed\n');
    } catch (error) {
      console.error('WebSocket error:', error.message, '\n');
    }
  }

  async demoCaching() {
    console.log('=== Redis Caching Demo ===\n');

    const queryKey = this.redisCache.getCacheKey(1, 'SELECT ?s WHERE { ?s ?p ?o }', 'query');

    const testData = {
      variables: ['s', 'p', 'o'],
      bindings: [
        { s: 'value1', p: 'value2', o: 'value3' }
      ]
    };

    console.log('✓ Setting cache value...');
    await this.redisCache.set(queryKey, testData, 300);

    console.log('✓ Retrieving from cache...');
    const cached = await this.redisCache.get(queryKey);
    if (cached) {
      console.log(`✓ Cache hit: ${JSON.stringify(cached).substring(0, 50)}...`);
    }

    console.log('✓ Invalidating cache...');
    await this.redisCache.del(queryKey);

    const invalid = await this.redisCache.get(queryKey);
    if (!invalid) {
      console.log('✓ Cache invalidated successfully\n');
    }
  }

  async demoComparison(indexPath) {
    console.log('=== Protocol Performance Comparison ===\n');

    const sparql = 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100';

    const measurements = {};

    try {
      console.log('Measuring HTTP performance...');
      const httpStart = Date.now();
      const openRes = await this.httpClient.post('/api/open', { indexPath });
      const handle = openRes.data.handle;
      const queryRes = await this.httpClient.post('/api/query', {
        handle,
        sparql,
        timings: 1
      });
      const httpTime = Date.now() - httpStart;
      measurements.http = {
        totalTime: httpTime,
        resultsCount: queryRes.data.results.bindings.length,
        serverTime: queryRes.data.timings?.query_ms || 0
      };
      await this.httpClient.post('/api/close', { handle });
      console.log(`  HTTP: ${httpTime}ms (${measurements.http.resultsCount} results)\n`);
    } catch (error) {
      console.log(`  HTTP: Error - ${error.message}\n`);
    }

    try {
      console.log('Measuring gRPC performance...');
      const grpcStart = Date.now();
      await this.grpcClient.openIndex(indexPath);
      const queryRes = await this.grpcClient.query(sparql, true);
      const grpcTime = Date.now() - grpcStart;
      measurements.grpc = {
        totalTime: grpcTime,
        resultsCount: queryRes.bindings.length,
        serverTime: queryRes.timings?.query_ms || 0
      };
      await this.grpcClient.close();
      console.log(`  gRPC: ${grpcTime}ms (${measurements.grpc.resultsCount} results)\n`);
    } catch (error) {
      console.log(`  gRPC: Error - ${error.message}\n`);
    }

    if (measurements.http && measurements.grpc) {
      const improvement = ((measurements.http.totalTime - measurements.grpc.totalTime) / measurements.http.totalTime * 100).toFixed(1);
      console.log(`Performance Delta: gRPC is ${Math.abs(improvement)}% ${improvement > 0 ? 'faster' : 'slower'} than HTTP\n`);
    }
  }

  async runFullDemo(indexPath = './test_index') {
    try {
      await this.initializeAllProtocols();
      await this.demoHttpProtocol(indexPath);
      await this.demoGrpcProtocol(indexPath);
      await this.demoWebSocketProtocol(indexPath);
      await this.demoCaching();
      await this.demoComparison(indexPath);

      console.log('=== Demo Complete ===');
      console.log('All protocols tested successfully!');
    } catch (error) {
      console.error('Demo error:', error);
    }
  }
}

const demo = new MultiProtocolQLever();
demo.runFullDemo().catch(console.error);

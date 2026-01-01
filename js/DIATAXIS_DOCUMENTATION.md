# QLever JavaScript Client & Server Documentation

**Diataxis-based documentation for the QLever JavaScript ecosystem**

---

## Table of Contents

1. [Tutorials](#tutorials) - Learning by doing
2. [How-to Guides](#how-to-guides) - Solve specific problems
3. [Reference](#reference) - API and module documentation
4. [Explanation](#explanation) - Understanding architecture and design

---

# Tutorials

*Learning-oriented guides to get you started with the essentials*

**Before starting:** This module provides Node.js clients and servers for QLever. There are no npm scripts - all servers and clients are run directly with `node`. Make sure the C++ QLever engine is running on port 3000 (HTTP server) before starting any of these tutorials.

## Tutorial 1: First SPARQL Query (5 minutes)

Learn to execute your first SPARQL query against QLever.

### Prerequisites
- Node.js 16+
- Running QLever HTTP server (`node js/server.js`)

### Steps

1. **Create a simple client:**

```javascript
const QleverClient = require('./client');

async function main() {
  const client = new QleverClient('http://localhost:3000');

  // Open an index
  await client.open('./test_index');

  // Execute a SPARQL query
  const result = await client.query(
    `SELECT ?s ?o WHERE { ?s <http://example.org/name> ?o } LIMIT 10`
  );

  console.log(`Found ${result.results.bindings.length} results`);
  console.log(`Columns: ${result.head.vars.join(', ')}`);

  await client.close();
}

main().catch(console.error);
```

2. **Understanding the response:**
- `head.vars` - Array of variable names (column headers)
- `results.bindings` - Array of binding objects (result rows)
- Each binding is an object: `{ ?var1: "value", ?var2: "value2", ... }`

### Key Concepts
- **Handle**: Unique identifier for your index session (auto-managed)
- **SPARQL**: Query language for RDF data
- **Index**: Pre-computed data structure enabling fast queries

---

## Tutorial 2: Real-time Data Streaming via WebSocket (10 minutes)

Stream large results without loading everything into memory.

### Prerequisites
- Running WebSocket server (`node js/websocket-server.js`)
- Basic knowledge of async/await

### Steps

1. **Connect and stream results:**

```javascript
const io = require('socket.io-client');

const socket = io('http://localhost:3002');

socket.on('connect', () => {
  console.log('Connected to QLever WebSocket server');

  // Step 1: Open an index
  socket.emit('open', { indexPath: './test_index' });
});

socket.on('opened', (data) => {
  console.log('Index opened with handle:', data.handle);

  // Step 2: Start streaming query
  socket.emit('stream-query', {
    handle: data.handle,
    sparql: 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1000000'
  });
});

// Receive results in batches
socket.on('stream-batch', (batch) => {
  console.log(`Received batch with ${batch.bindings.length} rows`);
  batch.bindings.forEach(binding => {
    console.log(binding); // Each binding is { ?var1: val1, ?var2: val2, ... }
  });
});

// Streaming complete
socket.on('stream-complete', (summary) => {
  console.log(`Streaming finished. Total rows: ${summary.totalRows}`);
});

// Error handling
socket.on('error', (err) => {
  console.error('WebSocket error:', err.message);
});
```

2. **Key events:**
- `opened` - Index ready, returns `{ handle }`
- `stream-batch` - Results batch arrives with `{ bindings, batchSize }`
- `stream-complete` - Query finished with `{ totalRows, totalTime }`
- `error` - Error occurred with `{ message, errorType }`

---

## Tutorial 3: Setting Up Multi-Protocol Server Stack (15 minutes)

Run all 4 protocol servers (HTTP, WebSocket, GraphQL, gRPC) together.

### Steps

1. **Start the production stack:**

```bash
node js/start-production-stack.js
```

2. **Verify all servers are running:**

```bash
# HTTP
curl http://localhost:3000/health

# WebSocket
nc -zv localhost 3002

# GraphQL
curl -X POST http://localhost:3003/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ health { status } }"}'

# gRPC
# Requires gRPC client tool
```

3. **What each server provides:**
- **HTTP (3000)**: REST API, easiest for simple queries
- **WebSocket (3002)**: Real-time streaming, best for large results
- **GraphQL (3003)**: Flexible schema, introspection, type safety
- **gRPC (50051)**: High-performance binary protocol

---

# How-to Guides

*Task-oriented guides for common operations*

## How to: Execute a Query and Get Metrics

```javascript
const QleverClient = require('./client');

async function executeWithMetrics() {
  const client = new QleverClient('http://localhost:3000');
  await client.open('./test_index');

  // Request timing information (second parameter = true)
  const result = await client.query(
    `SELECT ?s WHERE { ?s ?p ?o }`,
    true  // timings = true
  );

  console.log('Query parsing time:', result.timings.query_ms, 'ms');
  console.log('Planning time:', result.timings.planning_ms, 'ms');
  console.log('Execution time:', result.timings.execution_ms, 'ms');
  console.log('Results found:', result.results.bindings.length);

  await client.close();
}
```

**Key timing fields in response:**
- `timings.query_ms` - Time to parse SPARQL
- `timings.planning_ms` - Time to plan execution
- `timings.execution_ms` - Time to execute query
- `results.bindings.length` - Number of result rows

---

## How to: Stream Results Without Memory Overload

Use the WebSocket server for large result sets:

```javascript
const io = require('socket.io-client');
const fs = require('fs');

async function streamToFile(sparql, outputFile) {
  const socket = io('http://localhost:3002');
  const output = fs.createWriteStream(outputFile);

  return new Promise((resolve, reject) => {
    let handle;

    socket.on('connect', () => {
      socket.emit('open', { indexPath: './test_index' });
    });

    socket.on('opened', (data) => {
      handle = data.handle;
      // Start streaming query
      socket.emit('stream-query', { handle, sparql });
    });

    socket.on('stream-batch', (batch) => {
      // Write each binding (row) as JSON
      batch.bindings.forEach(binding => {
        output.write(JSON.stringify(binding) + '\n');
      });
    });

    socket.on('stream-complete', () => {
      output.end();
      socket.disconnect();
      resolve();
    });

    socket.on('error', reject);
  });
}

// Usage
streamToFile(
  'SELECT ?s ?p ?o WHERE { ?s ?p ?o }',
  'results.jsonl'
).catch(console.error);
```

---

## How to: Cache Expensive Query Results

Use materialized views to cache results:

```javascript
const QleverClient = require('./client');

async function createCache() {
  const client = new QleverClient();
  await client.open('/path/to/index');

  // Define a materialized view
  const query = `
    SELECT ?entity ?label
    WHERE {
      ?entity <http://www.w3.org/2000/01/rdf-schema#label> ?label
    }
  `;

  // Create the view (one-time operation)
  await client.writeMaterializedView('entity_labels', query);

  // Load it for faster subsequent queries
  await client.loadMaterializedView('entity_labels');

  // Now queries using the cached data are faster
  const result = await client.query('SELECT * FROM entity_labels LIMIT 10');
}
```

---

## How to: Monitor System Health

```javascript
const axios = require('axios');

async function monitorHealth() {
  const client = axios.create({ baseURL: 'http://localhost:3000' });

  // Get overall system health
  const health = await client.get('/health');
  console.log('Status:', health.data.status);

  // Get detailed metrics
  const metrics = await client.get('/system/metrics');
  console.log('Operation counts:', metrics.data.operations);
  console.log('Error rate:', metrics.data.errorRate);

  // Get current status
  const status = await client.get('/system/status');
  console.log('Uptime:', status.data.uptime);
  console.log('Memory usage:', status.data.memoryUsage);
}
```

---

## How to: Handle Network Failures Gracefully

The `ResilienceHandler` is built-in but you can customize it:

```javascript
const { ResilienceHandler } = require('./resilience-layer');

const resilience = new ResilienceHandler({
  maxRetries: 5,           // Retry up to 5 times
  retryDelay: 100,         // Wait 100ms between retries
  exponentialBackoff: true // Double wait time each retry
});

async function queryWithResilience(client, sparql) {
  return resilience.executeWithResilience(
    () => client.query(sparql),
    'sparql_query'
  );
}
```

---

## How to: Validate Response Contracts

Ensure responses conform to expected structure:

```javascript
const { ContractEnforcer } = require('./contract-enforcer');

const enforcer = new ContractEnforcer();

async function validateResponse(response) {
  try {
    // Validate against strict HTTP.Response contract
    enforcer.assertStrict('HTTP.Response.Structure', response);
    console.log('✓ Response valid');
  } catch (violation) {
    console.error('✗ Contract violation:', violation.message);
  }
}
```

---

## How to: Use GraphQL for Type-Safe Queries

```javascript
const axios = require('axios');

async function graphqlQuery() {
  const client = axios.create({ baseURL: 'http://localhost:3003' });

  const query = `
    query {
      sparqlQuery(query: "SELECT ?s WHERE { ?s ?p ?o LIMIT 10 }") {
        results {
          s
        }
        resultSize
        computationTime
      }
    }
  `;

  const response = await client.post('/graphql', {
    query: query
  });

  console.log(response.data);
}
```

---

## How to: Load Test Your Server

```javascript
const LoadTester = require('./load-tester');

async function runLoadTest() {
  const tester = new LoadTester({
    duration: 30000,        // Run for 30 seconds
    concurrency: 10,        // 10 concurrent clients
    querySize: 10,          // Queries returning ~10 results
    thinkTime: 100          // Wait 100ms between requests
  });

  // Generate load
  await tester.generateLoad();

  // Get results
  const metrics = tester.metrics;
  console.log('Total requests:', metrics.totalRequests);
  console.log('Successful:', metrics.successfulRequests);
  console.log('Failed:', metrics.failedRequests);
  console.log('Avg latency:', metrics.averageLatency, 'ms');
  console.log('Max latency:', metrics.maxLatency, 'ms');
  console.log('Min latency:', metrics.minLatency, 'ms');
  console.log('Throughput:', metrics.throughput, 'req/sec');
}

runLoadTest().catch(console.error);
```

**Metrics available:**
- `totalRequests` - Total operations
- `successfulRequests` - Operations that succeeded
- `failedRequests` - Operations that failed
- `contractViolations` - Response format violations
- `averageLatency` - Mean latency in ms
- `throughput` - Requests per second

---

## How to: Query via gRPC (Binary Protocol)

For maximum performance with strongly-typed queries:

```javascript
const GrpcClient = require('./grpc-client');

async function grpcQuery() {
  const client = new GrpcClient('localhost:50051');

  // Open index
  const openRes = await client.openIndex({
    indexPath: './test_index'
  });
  const handle = openRes.handle;

  // Execute query
  const result = await client.query({
    handle,
    sparql: 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 100',
    timings: 1
  });

  console.log(`Results: ${result.bindings.length}`);
  console.log(`Execution: ${result.timings.execution_ms}ms`);

  await client.closeIndex({ handle });
}

grpcQuery().catch(console.error);
```

**Advantages:**
- Binary protocol (30-50% smaller than JSON)
- Strongly typed messages
- Stream support for large results
- Lower latency than HTTP

---

## How to: Handle Common SPARQL Errors

Proper error detection and recovery:

```javascript
const QleverClient = require('./client');

async function queryWithErrorHandling() {
  const client = new QleverClient('http://localhost:3000');

  try {
    await client.open('./test_index');

    // Invalid SPARQL syntax
    try {
      await client.query('SELECT * FROM invalid');
    } catch (error) {
      console.error('SPARQL Error:', error.message);
      // Error: "SPARQL parse error"
    }

    // Non-existent variable
    try {
      await client.query('SELECT ?x WHERE { ?s ?p ?o }');
      // Returns empty results (valid but no matches)
    } catch (error) {
      console.error('Query Error:', error.message);
    }

    // Timeout on slow query
    try {
      const result = await client.query(
        'SELECT ?s ?p ?o WHERE { ?s ?p ?o }',
        false,
        30000  // 30 second timeout
      );
    } catch (error) {
      if (error.code === 'ECONNABORTED') {
        console.error('Query timeout - try with LIMIT');
      }
    }

  } finally {
    await client.close();
  }
}
```

**Common errors:**
- `Parse error` - Invalid SPARQL syntax
- `Timeout` - Query too slow, add LIMIT or WHERE filters
- `Handle not found` - Index was closed or session expired
- `Connection refused` - Server not running on specified port

---

## How to: Use Text Search

Full-text search over RDF text:

```javascript
const QleverClient = require('./client');

async function textSearch() {
  const client = new QleverClient('http://localhost:3000');
  await client.open('./test_index');

  // Simple text search
  const results = await client.textSearch('berlin', 50);

  console.log(`Found ${results.results.bindings.length} matches`);
  results.results.bindings.forEach(binding => {
    console.log(binding);
  });

  await client.close();
}

textSearch().catch(console.error);
```

**Text search tips:**
- Returns full-text matches (BM25 scoring)
- Useful for discovering data
- Can combine with SPARQL FILTER
- Limit results to avoid huge response sets

---

# Reference

*Complete API documentation for all modules*

## Response Format Reference

All query responses follow the **SPARQL JSON Results Format**:

```javascript
{
  head: {
    vars: ["?variable1", "?variable2", ...]  // Column names
  },
  results: {
    bindings: [
      { "?variable1": "value1", "?variable2": "value2" },  // Row 1
      { "?variable1": "value3", "?variable2": "value4" },  // Row 2
      ...
    ]
  },
  timings: {                          // Optional, if requested with timings=true
    query_ms: 10,                     // Time to parse SPARQL
    planning_ms: 50,                  // Time to plan execution
    execution_ms: 100                 // Time to execute query
  }
}
```

**Key points:**
- Each binding is an object where keys are variable names (include the `?`)
- Values can be strings, numbers, or objects (for RDF terms with type info)
- Order of results matches query, or sorted by `ORDER BY` clause
- `timings` field only present if `timings=true` parameter was passed

---

## Core Clients

### `client.js` - HTTP REST Client

**Class: `QleverClient`**

#### Constructor
```javascript
const client = new QleverClient(baseUrl = 'http://localhost:3000');
```

#### Methods

| Method | Parameters | Returns | Purpose |
|--------|-----------|---------|---------|
| `open(indexPath, config?)` | string, object? | Promise<this> | Open an RDF index |
| `query(sparql, timings?)` | string, boolean? | Promise<Object> | Execute SPARQL query |
| `parseAndPlan(sparql)` | string | Promise<string> | Parse and plan without executing |
| `executePlan(planId, timings?)` | string, boolean? | Promise<Object> | Execute pre-planned query |
| `textSearch(query, limit?)` | string, number? | Promise<Object> | Full-text search |
| `pinResult(name, sparql)` | string, string | Promise<void> | Cache result with name |
| `eraseResult(name)` | string | Promise<void> | Clear cached result |
| `writeMaterializedView(name, sparql)` | string, string | Promise<void> | Create materialized view |
| `loadMaterializedView(name)` | string | Promise<void> | Load materialized view |
| `clearCache()` | none | Promise<void> | Clear all caches |
| `close()` | none | Promise<void> | Close index session |

**Example:**
```javascript
const client = new QleverClient();
await client.open('/index/path');
const result = await client.query('SELECT ?s WHERE { ?s ?p ?o }', true);
await client.close();
```

---

### `browser-client.js` - Browser-Compatible Client

Same API as `QleverClient` but works in browser environments (fetch instead of axios).

```javascript
import QleverClient from './browser-client.js';

const client = new QleverClient('http://localhost:3000');
```

---

### `websocket-client.js` - Socket.io Client

**Class: `WebSocketQleverClient`**

```javascript
const WebSocketQleverClient = require('./websocket-client');

const client = new WebSocketQleverClient('http://localhost:3002');

// Connect to server
await client.connect();

// Open index
await client.open('./test_index');

// Stream query results
await client.streamQuery('SELECT ?s WHERE { ?s ?p ?o }', (batch) => {
  console.log(`Batch: ${batch.bindings.length} rows`);
});
```

**Methods:**
- `connect()` - Connect to WebSocket server
- `open(indexPath)` - Open RDF index
- `streamQuery(sparql, onBatch)` - Stream results
- `query(sparql, timings?)` - Execute single query
- `disconnect()` - Close connection

**Response Events:**
- `opened` - Index ready, data: `{ handle }`
- `stream-batch` - Batch received, data: `{ bindings, batchSize }`
- `query-result` - Single query result
- `error` - Error occurred

---

### `grpc-client.js` - gRPC Client

```javascript
const { createGRPCClient } = require('./grpc-client');

const client = createGRPCClient('localhost:50051');
const result = await client.query({ sparql: '...' });
```

**Supports:** Unary calls and server-side streaming.

---

## Server Implementations

### `server.js` - HTTP/REST Server

**Endpoints:**

| Endpoint | Method | Body | Returns |
|----------|--------|------|---------|
| `/api/open` | POST | `{indexPath, config?}` | `{handle}` |
| `/api/query` | POST | `{handle, sparql, timings?}` | `{head, results, ...}` |
| `/api/parse-and-plan` | POST | `{handle, sparql}` | `{planId}` |
| `/api/execute-plan` | POST | `{handle, planId, timings?}` | `{head, results, ...}` |
| `/api/text-search` | POST | `{handle, query, limit?}` | `{results}` |
| `/api/pin-result` | POST | `{handle, name, sparql}` | `{}` |
| `/api/erase-result` | POST | `{handle, name}` | `{}` |
| `/api/write-materialized-view` | POST | `{handle, name, sparql}` | `{}` |
| `/api/load-materialized-view` | POST | `{handle, name}` | `{}` |
| `/api/clear-cache` | POST | `{handle}` | `{}` |
| `/health` | GET | none | `{status}` |
| `/system/health` | GET | none | `{healthy, details}` |
| `/system/metrics` | GET | none | `{operations, errors, ...}` |
| `/system/status` | GET | none | `{uptime, memory, ...}` |

---

### `websocket-server.js` - WebSocket Server (Port 3002)

**Incoming Events (Client → Server):**

| Event | Data | Response | Purpose |
|-------|------|----------|---------|
| `open` | `{indexPath, config?}` | `opened` | Open RDF index |
| `query` | `{handle, sparql, timings?}` | `query-result` | Execute single query |
| `stream-query` | `{handle, sparql, timings?}` | `stream-batch`, `stream-complete` | Stream large results |
| `plan-and-execute` | `{handle, sparql}` | `plan-executed` | Plan + execute |
| `cache-result` | `{handle, name, sparql}` | `result-cached` | Cache result |
| `close` | none | `closed` | Disconnect |

**Example:**
```javascript
socket.emit('open', { indexPath: './test_index' });
socket.on('opened', (data) => {
  console.log('Handle:', data.handle);
});

socket.emit('stream-query', { handle, sparql: '...' });
socket.on('stream-batch', (data) => {
  console.log('Batch size:', data.bindings.length);
});
socket.on('stream-complete', (data) => {
  console.log('Total rows:', data.totalRows);
});
```

---

### `graphql-server-instrumented.js` - GraphQL Server (Port 3003)

**Endpoint:** `POST /graphql`

**Query Example:**
```graphql
query {
  sparqlQuery(query: "SELECT ?s WHERE { ?s ?p ?o }") {
    results
    resultSize
    computationTime
    health {
      status
      errorRate
    }
  }
}
```

---

### `grpc-server.js` - gRPC Server (Port 50051)

**Service Definition:** See `qlever.proto`

**Methods:**
- `QueryUnary(QueryRequest)` → `QueryResponse`
- `QueryStream(QueryRequest)` → `stream QueryResponse`

---

## Infrastructure Modules

### `protocol-instrumentation.js` - Unified Instrumentation Framework

**Class: `ProtocolInstrumentation`**

```javascript
const ProtocolInstrumentation = require('./protocol-instrumentation');

const instrumentation = new ProtocolInstrumentation('http', {
  maxRetries: 3,
  retryDelay: 50,
  healthUpdateInterval: 5000,
  metricsWindow: 100
});

// Execute operation with automatic instrumentation
const result = await instrumentation.executeWithInstrumentation(
  'query',  // operation name
  () => queryFunction(),  // handler function
  ['HTTP.Response.Structure']  // contracts to validate
);
```

**Methods:**
- `executeWithInstrumentation(operation, handler, contracts)` - Execute with all instrumentation
- `recordMetric(operation, latency, success, error, metadata)` - Manually record metric
- `getMetrics()` - Get operation statistics
- `getHealth()` - Get system health status

**Automatically provides:**
- Metrics (latency percentiles, throughput, error rates)
- Resilience (automatic retry with exponential backoff)
- Contract enforcement (response validation)
- Health tracking (per-operation health scores)

---

### `resilience-layer.js` - Fault Tolerance

**Class: `ResilienceHandler`**

```javascript
const { ResilienceHandler } = require('./resilience-layer');

const resilience = new ResilienceHandler({
  maxRetries: 3,
  retryDelay: 50,
  exponentialBackoff: true
});

const result = await resilience.executeWithResilience(
  () => operation(),
  'operation_name'
);
```

**Configuration:**
- `maxRetries` - Maximum retry attempts
- `retryDelay` - Initial delay between retries (ms)
- `exponentialBackoff` - Double delay each retry

---

### `contract-enforcer.js` - Data Validation

**Class: `ContractEnforcer`**

```javascript
const { ContractEnforcer } = require('./contract-enforcer');

const enforcer = new ContractEnforcer();

try {
  enforcer.assertStrict('HTTP.Response.Structure', response);
} catch (violation) {
  console.error('Invalid response:', violation);
}
```

**Built-in Contracts:**
- `HTTP.Response.Structure` - Valid REST response format
- `SPARQL.Result.Format` - Valid SPARQL result format

---

### `system-health-monitor.js` - Health Tracking

**Class: `SystemHealthMonitor`**

```javascript
const { SystemHealthMonitor } = require('./system-health-monitor');

const monitor = new SystemHealthMonitor({
  updateInterval: 5000
});

monitor.on('status_change', (status) => {
  console.log('System is now:', status);
});

const health = monitor.getHealth();
```

**Tracks:**
- Memory usage
- CPU usage
- Operation error rates
- Response times

---

### `system-diagnostics.js` - Diagnostic Information

```javascript
const { runDiagnostics } = require('./system-diagnostics');

const report = await runDiagnostics({
  baseUrl: 'http://localhost:3000',
  includePerformance: true
});

console.log(report);
```

---

### `protocol-validator.js` - Protocol Compliance

```javascript
const { ProtocolValidator } = require('./protocol-validator');

const validator = new ProtocolValidator();

const isValid = validator.validateResponse(response);
const violations = validator.getViolations();
```

---

## Utility Modules

### `redis-cache.js` - Redis Caching Layer

```javascript
const { RedisCache } = require('./redis-cache');

const cache = new RedisCache({
  host: 'localhost',
  port: 6379
});

await cache.set('key', data, 3600); // 1 hour TTL
const cached = await cache.get('key');
```

---

### `wasm-utils.js` - WebAssembly Utilities

```javascript
const { initWasm } = require('./wasm-utils');

await initWasm();
// Use WASM features in browser
```

---

## Testing & Monitoring Modules

### `load-tester.js` - Load Testing

**Class: `LoadTester`**

```javascript
const LoadTester = require('./load-tester');

const tester = new LoadTester({
  duration: 60000,          // Run for 60 seconds
  concurrency: 20,          // 20 concurrent clients
  querySize: 100,           // Expected result size
  thinkTime: 200            // Delay between requests
});

await tester.generateLoad();

const { totalRequests, successfulRequests, failedRequests,
        averageLatency, maxLatency, minLatency, throughput } = tester.metrics;

console.log(`Throughput: ${throughput.toFixed(2)} req/sec`);
console.log(`Avg latency: ${averageLatency.toFixed(2)}ms`);
```

**Configuration Options:**
- `duration` - Test duration in milliseconds
- `concurrency` - Number of concurrent client threads
- `querySize` - Expected size of results
- `thinkTime` - Delay between requests (ms)

**Metrics Available:**
- `totalRequests` - Total operations
- `successfulRequests` - Passed operations
- `failedRequests` - Failed operations
- `contractViolations` - Response format violations
- `averageLatency` - Mean response time
- `maxLatency` - Longest response time
- `minLatency` - Shortest response time
- `throughput` - Requests per second

---

### `stress-tester.js` - Edge Case Testing

```javascript
const StressTester = require('./stress-tester');

const tester = new StressTester();

// Test edge cases
await tester.testEdgeCases();

// Test boundary violations
await tester.testBoundaryViolations();

// Stress test with high load
await tester.stressTest();

// Get results
const report = tester.results;
console.log('Edge cases:', report.edgeCases);
console.log('Failures:', report.failures);
```

**Tests performed:**
- Empty queries
- Malformed SPARQL
- Invalid handles
- Concurrent operations
- Memory limits
- Timeout conditions

---

### `server-coordinator.js` - Multi-Server Management

```javascript
const ServerCoordinator = require('./server-coordinator');

const coordinator = new ServerCoordinator({
  httpPort: 3000,
  wsPort: 3002,
  graphqlPort: 3003,
  grpcPort: 50051,
  coordinatorPort: 3001,
  healthCheckInterval: 5000
});

// Start health checking all servers
await coordinator.startHealthChecking();

// Get aggregated metrics
const metrics = coordinator.aggregatedMetrics;
console.log('Total requests:', metrics.totalRequests);
console.log('Total errors:', metrics.totalErrors);
```

**Methods:**
- `startHealthChecking()` - Start monitoring all servers
- `performHealthCheck()` - Check server health
- `getAggregatedMetrics()` - Get combined metrics

---

### `integration-test.js` - Protocol Integration Testing

Complete integration test of all protocol servers:

```bash
node js/integration-test.js
```

Tests:
- HTTP endpoint functionality
- WebSocket streaming
- GraphQL queries
- gRPC unary and streaming calls
- Cross-protocol consistency

---

### `coherence-test.js` - Data Coherence Testing

```bash
node js/coherence-test.js
```

Validates:
- Result consistency across protocols
- Binding format compliance
- Timing accuracy
- Error handling consistency

---

### `example-usage.js` - Complete Usage Examples

Comprehensive example showing all major features:

```bash
node js/example-usage.js
```

Demonstrates:
- Opening indices
- Basic queries
- Materialized views
- Text search
- Cache management
- Closing sessions

---

### `multi-protocol-example.js` - Multi-Protocol Demo

```bash
node js/multi-protocol-example.js
```

Shows how to use all 4 protocols together:
- Initializing all clients
- Running equivalent queries on each
- Comparing results and performance
- Redis caching integration

---

### `start-production-stack.js` - Start All Servers

Starts all 4 protocol servers in one process:

```bash
node js/start-production-stack.js
```

Starts:
1. HTTP server (port 3000)
2. WebSocket server (port 3002)
3. GraphQL server (port 3003)
4. gRPC server (port 50051)

Provides:
- Unified logging
- Health monitoring
- Graceful shutdown

---

# Explanation

*Understanding the design and architecture*

## System Architecture Overview

The QLever JavaScript ecosystem is organized in **4 layers**:

```
┌─────────────────────────────────────────┐
│  Layer 3: Client Applications           │
│  (Your code using clients)              │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  Layer 2: Protocol Servers              │
│  HTTP (3000) / WS (3002) /              │
│  GraphQL (3003) / gRPC (50051)          │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  Layer 1: Instrumentation Framework     │
│  Metrics, Resilience, Contracts, Health │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  Layer 0: QLever C++ Engine             │
│  (Fast RDF/SPARQL execution)            │
└─────────────────────────────────────────┘
```

---

## Design Decisions: Why This Architecture?

### 1. Multi-Protocol Servers (HTTP, WebSocket, GraphQL, gRPC)

**Question:** Why support 4 protocols?

**Answer:** Different use cases have different needs:
- **HTTP/REST**: Simplest, most familiar, stateless
- **WebSocket**: Real-time streaming, connection-persistent
- **GraphQL**: Type safety, introspection, flexible queries
- **gRPC**: High performance, binary protocol, mobile-friendly

Each protocol is a **thin wrapper** around the same core engine, so maintainability is high.

### 2. Instrumentation Framework

**Question:** Why a separate instrumentation layer?

**Answer:** To avoid code duplication. Each protocol server needs:
- Performance metrics (latency, throughput)
- Error tracking and recovery
- Data validation
- Health monitoring

By centralizing this in `ProtocolInstrumentation`, all servers get these features automatically.

### 3. Resilience Handler with Exponential Backoff

**Question:** Why retry failed requests?

**Answer:** Transient failures (network hiccups, temporary load) are common in distributed systems. Automatic retry with exponential backoff:
- Recovers from temporary issues automatically
- Prevents thundering herd (exponential backoff spreads load)
- Improves user experience (transparent recovery)

### 4. Contract Enforcement

**Question:** Why validate responses?

**Answer:** Contracts catch bugs early:
- Ensures responses match expected format
- Catches protocol violations
- Documents expected behavior
- Makes debugging easier (failures caught near source)

### 5. Materialized Views (Caching)

**Question:** Why cache query results?

**Answer:** Some queries are expensive and run repeatedly:
- Pre-compute results once
- Serve from cache instantly
- Dramatically faster for dashboards and analytics
- Trade: Staleness (cache age)

---

## When to Use Each Protocol

### Use HTTP/REST When:
- Simple, one-off queries
- Stateless operations
- Easy to implement (curl, axios, fetch)
- Building web dashboards
- Mobile apps

### Use WebSocket When:
- Streaming large result sets (millions of rows)
- Real-time updates needed
- Persistent connection useful
- Building monitoring dashboards
- Client needs bidirectional communication

### Use GraphQL When:
- Type safety matters
- Introspection/self-discovery needed
- Complex nested queries
- Schema documentation required
- Easy client generation

### Use gRPC When:
- Maximum performance critical
- Binary protocol acceptable
- High-volume requests
- Mobile/low-bandwidth important
- Services need strong typing

---

## Error Handling Strategy

The system uses a **layered error approach**:

```
Client App
    ↓
ResilienceHandler (retries)
    ↓
ContractEnforcer (validates)
    ↓
Protocol Server (formats)
    ↓
QLever Engine (executes)
```

Each layer handles specific error categories:
1. **Engine**: Semantic errors (bad SPARQL)
2. **Server**: Protocol errors (malformed requests)
3. **Enforcer**: Contract violations (invalid response)
4. **Resilience**: Transient failures (timeout, 503)

---

## Performance Characteristics

### Query Execution Time Breakdown

For a typical query on a large RDF index:

```
Parsing:      10-50ms   (once per query)
Planning:     50-200ms  (depends on complexity)
Execution:    100-5000ms (depends on data size)
Serialization: 10-100ms  (depends on result size)
─────────────────────────
Total:        170-5350ms
```

### Memory Usage

- Per-client session: ~1-10MB (index metadata)
- Per-query result: ~1MB per million rows (rough)
- Server overhead: ~50MB (caches, indices)

### Throughput

- HTTP sequential: ~100-500 queries/sec (depends on query complexity)
- Concurrent (10 clients): ~500-2000 queries/sec
- WebSocket streaming: Limited by network bandwidth

---

## Testing Strategy

The codebase includes:

1. **Unit Tests** (`*Test.js`): Test individual modules
2. **Integration Tests** (`integration-test.js`): Test protocol servers
3. **Load Tests** (`load-tester.js`): Performance benchmarking
4. **System Tests** (`coherence-test.js`): End-to-end validation
5. **Health Checks** (Continuous): Monitor production health

---

## Security Considerations

### Current Implementation

- **Input validation**: SPARQL queries validated by parser
- **Authentication**: Not implemented (assume trusted network)
- **Encryption**: Use HTTPS/TLS in production
- **Rate limiting**: Not implemented (add if needed)

### Production Deployment

1. **Run behind reverse proxy** (nginx, Apache)
2. **Enable HTTPS/TLS**
3. **Add authentication** (JWT tokens, API keys)
4. **Implement rate limiting** (prevent abuse)
5. **Monitor logs** (error tracking)
6. **Setup alerts** (health monitor + alerting)

---

## Extending the System

### Adding a New Protocol

1. **Create server file**: `js/newprotocol-server.js`
2. **Use `ProtocolInstrumentation`** for metrics/resilience
3. **Create client file**: `js/newprotocol-client.js`
4. **Add example**: `js/newprotocol-example.js`
5. **Update architecture doc**: `SYSTEM_ARCHITECTURE.js`

### Adding New Metrics

1. Edit `protocol-instrumentation.js`
2. Add to `getMetrics()` response
3. Update clients to display new metrics

### Custom Contracts

1. Extend `contract-enforcer.js`
2. Define contract specification
3. Implement validation logic
4. Document contract in reference

---

## Troubleshooting Common Issues

### "Index not opened" Error

**Cause:** Forgot to call `client.open()` before querying

**Fix:**
```javascript
const client = new QleverClient();
await client.open('./test_index'); // Don't forget this!
await client.query('...');
```

---

### "Connection refused" on Port 3000

**Cause:** HTTP server not running or not accessible

**Fix:**
```bash
# Start the server
node js/server.js

# Or start all servers
node js/start-production-stack.js

# Verify it's running
curl http://localhost:3000/health
```

---

### WebSocket "Invalid handle" Error

**Cause:** Using handle from different session or index closed

**Fix:**
```javascript
socket.on('opened', (data) => {
  // Use the handle returned from 'opened' event
  const handle = data.handle;
  socket.emit('stream-query', { handle, sparql: '...' });
});
```

---

### SPARQL Parse Error

**Cause:** Syntax error in SPARQL query

**Fix:** Validate SPARQL syntax:
```javascript
// Use /api/parse-and-plan to get error details
const response = await client.parseAndPlan('SELECT * FROM invalid');
// Will throw with error message

// Valid SPARQL
const result = await client.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 10');
```

**Common SPARQL mistakes:**
- Missing `WHERE` clause: `SELECT ?s` (should be `SELECT ?s WHERE { ... }`)
- Unmatched curly braces: `{ ?s ?p ?o }`
- Undefined variables: `SELECT ?x WHERE { ?s ?p ?o }`
- Invalid prefixes: `SELECT ?s WHERE { ex:subject ex:prop ?o }` (undefined `ex` prefix)

---

### WebSocket Results Arriving Very Slowly

**Cause:** Batch size too small, network latency, or query complexity

**Fix:**
```javascript
// Increase concurrency on large queries
const socket = io('http://localhost:3002');

socket.emit('stream-query', {
  handle,
  sparql: 'SELECT ?s ?p ?o WHERE { ?s ?p ?o }', // Add LIMIT
  batchSize: 10000  // If supported
});
```

---

### Memory Usage Growing Over Time

**Cause:** Results not being cleared, cache growing unbounded

**Fix:**
```javascript
// Clear specific cached results
await client.eraseResult('cached_name');

// Clear all caches
await client.clearCache();

// Or manually manage sessions
const client = new QleverClient();
await client.open('./test_index');
// ... do work ...
await client.close();  // Always close!
```

---

### gRPC "Unimplemented" Error

**Cause:** gRPC server not running or proto file mismatch

**Fix:**
```bash
# Start gRPC server
node js/grpc-server.js

# Verify it's running
grpcurl -plaintext localhost:50051 list

# Check service definition
cat js/qlever.proto
```

---

### GraphQL Introspection Failing

**Cause:** GraphQL server not running or schema issue

**Fix:**
```bash
# Start GraphQL server
node js/graphql-server-instrumented.js

# Test introspection
curl -X POST http://localhost:3003/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __schema { types { name } } }"}'
```

---

### Redis Cache Connection Failed

**Cause:** Redis server not running

**Fix:**
```bash
# Start Redis (if installed)
redis-server

# Or disable Redis caching in multi-protocol example
# Comment out RedisCache initialization
```

---

### Timeout on Large Queries

**Cause:** Query too complex or result set too large

**Fix:**
```javascript
// Option 1: Add LIMIT clause
await client.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 10000');

// Option 2: Use WebSocket streaming instead
socket.emit('stream-query', { handle, sparql: '...' });

// Option 3: Increase axios timeout
const client = new QleverClient('http://localhost:3000');
client.client.defaults.timeout = 60000; // 60 seconds
```

---

### Contract Violation Error

**Cause:** Response doesn't match expected format

**Debug:**
```javascript
try {
  enforcer.assertStrict('HTTP.Response.Structure', response);
} catch (violation) {
  console.error('Invalid field:', violation.field);
  console.error('Expected:', violation.expected);
  console.error('Got:', violation.actual);
}
```

---

## Production Deployment Guide

### 1. Environment Setup

```bash
# Create production directory
mkdir -p /opt/qlever-js
cd /opt/qlever-js

# Copy source
cp -r /path/to/qlever/js/* .
npm install --production

# Set permissions
chmod 755 *.js
```

### 2. Configure Services

```bash
# Create .env file
cat > .env << EOF
NODE_ENV=production
HTTP_PORT=3000
WS_PORT=3002
GRAPHQL_PORT=3003
GRPC_PORT=50051
REDIS_HOST=localhost
REDIS_PORT=6379
LOG_LEVEL=info
EOF
```

### 3. Start with Supervisor

```bash
# Create supervisor config
cat > /etc/supervisor/conf.d/qlever.conf << EOF
[program:qlever-stack]
command=node /opt/qlever-js/start-production-stack.js
directory=/opt/qlever-js
autostart=true
autorestart=true
redirect_stderr=true
stdout_logfile=/var/log/qlever.log
EOF

supervisorctl reread
supervisorctl update
supervisorctl start qlever-stack
```

### 4. Reverse Proxy Setup (nginx)

```nginx
upstream qlever_http {
  server localhost:3000;
}

upstream qlever_ws {
  server localhost:3002;
}

server {
  listen 80;
  server_name qlever.example.com;

  # HTTP API
  location /api {
    proxy_pass http://qlever_http;
    proxy_set_header X-Forwarded-For $remote_addr;
  }

  # WebSocket
  location /socket.io {
    proxy_pass http://qlever_ws;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
  }

  # GraphQL
  location /graphql {
    proxy_pass http://localhost:3003;
  }
}
```

### 5. Monitoring & Logging

```javascript
// Log to file
const fs = require('fs');
const logStream = fs.createWriteStream('/var/log/qlever-metrics.log');

// Periodically log metrics
setInterval(async () => {
  const health = await axios.get('http://localhost:3000/health');
  logStream.write(JSON.stringify(health.data) + '\n');
}, 60000);
```

### 6. Health Checks

```bash
# Add to monitoring system
curl http://localhost:3000/health | jq .status
curl http://localhost:3002/health | jq .status
curl http://localhost:3003/health | jq .status
```

### 7. Backup Strategy

```bash
# Back up indices regularly
cron: 0 2 * * * /opt/qlever-js/backup.sh

# Script
#!/bin/bash
tar -czf /backup/qlever-$(date +%Y%m%d).tar.gz ./index/
```

---

## 80/20 Focus: Core Concepts You Need to Know

**20% of concepts that give you 80% of capability:**

1. **QleverClient**: How to open index and execute queries
2. **Result format**: Head (columns) + Results (rows)
3. **Protocol choice**: Use HTTP for simple cases, WebSocket for large results
4. **Materialized views**: Cache expensive query results
5. **Health monitoring**: Check `/health` endpoint regularly
6. **Error handling**: Resilience layer handles transients automatically
7. **Contract validation**: Response format is validated automatically
8. **Performance metrics**: Available at `/system/metrics`

**Master these 8 concepts and you can build almost anything with QLever.**

---

## Next Steps

1. **Start with Tutorial 1** to get a simple example working
2. **Browse How-to Guides** for your specific use case
3. **Check Reference** when you need API details
4. **Read Explanation** when you want to understand design decisions
5. **Run examples**: `node js/example-usage.js`
6. **Check your queries**: Use `/api/parse-and-plan` to see execution plan

---

## Additional Resources

- **SPARQL Specification**: https://www.w3.org/TR/sparql11-query/
- **RDF Concepts**: https://www.w3.org/RDF/
- **QLever C++ Docs**: See `CLAUDE.md` in root
- **Example Data**: Check `/examples` directory
- **System Architecture**: See `SYSTEM_ARCHITECTURE.js`

---

**Document Version:** 1.0
**Last Updated:** 2025-01-01
**Status:** Ready for Production

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

## Tutorial 1: First SPARQL Query (5 minutes)

Learn to execute your first SPARQL query against QLever.

### Prerequisites
- Node.js 16+
- Running QLever HTTP server (`npm run server`)

### Steps

1. **Create a simple client:**

```javascript
const QleverClient = require('./client');

async function main() {
  const client = new QleverClient('http://localhost:3000');

  // Open an index
  await client.open('/path/to/index');

  // Execute a SPARQL query
  const result = await client.query(
    `SELECT ?s ?o WHERE { ?s <http://example.org/name> ?o } LIMIT 10`
  );

  console.log(result);
  await client.close();
}

main().catch(console.error);
```

2. **Understanding the response:**
- `head` - Column names (variables)
- `results` - Array of rows (arrays)
- `resultSize` - Number of results

### Key Concepts
- **Handle**: Unique identifier for your index session (auto-managed)
- **SPARQL**: Query language for RDF data
- **Index**: Pre-computed data structure enabling fast queries

---

## Tutorial 2: Real-time Data Streaming via WebSocket (10 minutes)

Stream large results without loading everything into memory.

### Prerequisites
- Running WebSocket server (`npm run ws-server`)
- Basic knowledge of async/await

### Steps

1. **Connect and stream results:**

```javascript
const io = require('socket.io-client');

const socket = io('http://localhost:3002');

socket.on('connect', async () => {
  console.log('Connected to QLever');

  socket.emit('open', { indexPath: '/path/to/index' });

  socket.on('opened', (data) => {
    socket.emit('query', {
      sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1000000'
    });
  });

  // Receive results as they stream
  socket.on('result_batch', (batch) => {
    console.log(`Received ${batch.rows.length} rows`);
  });

  socket.on('query_complete', (summary) => {
    console.log(`Total rows: ${summary.totalRows}`);
  });
});
```

2. **Key events:**
- `opened` - Index ready
- `result_batch` - Results arrive in batches
- `query_complete` - Query finished
- `error` - Error occurred

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
  const client = new QleverClient();
  await client.open('/path/to/index');

  // Request timing information
  const result = await client.query(
    `SELECT ?s WHERE { ?s ?p ?o }`,
    true  // timings = true
  );

  console.log('Query time:', result.computationTimeMs);
  console.log('Total time:', result.totalTimeMs);
  console.log('Results:', result.resultSize);
}
```

**Key fields in response:**
- `computationTimeMs` - Time to execute query
- `totalTimeMs` - Total time (parsing + execution)
- `resultSize` - Number of results returned

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
    socket.on('connect', () => {
      socket.emit('open', { indexPath: '/path/to/index' });
    });

    socket.on('result_batch', (batch) => {
      batch.rows.forEach(row => {
        output.write(JSON.stringify(row) + '\n');
      });
    });

    socket.on('query_complete', () => {
      output.end();
      socket.disconnect();
      resolve();
    });

    socket.on('error', reject);
  });
}
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
const { loadTest } = require('./load-tester');

async function runLoadTest() {
  const results = await loadTest({
    baseUrl: 'http://localhost:3000',
    concurrency: 10,
    requestsPerClient: 100,
    sparqlQueries: [
      'SELECT ?s WHERE { ?s ?p ?o }',
      'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 100'
    ]
  });

  console.log('Throughput:', results.requestsPerSecond);
  console.log('Avg latency:', results.avgLatency);
  console.log('P99 latency:', results.p99Latency);
  console.log('Error rate:', results.errorRate);
}
```

---

# Reference

*Complete API documentation for all modules*

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

**Functions:**
```javascript
const { createWebSocketClient } = require('./websocket-client');

const client = createWebSocketClient('http://localhost:3002');
client.on('opened', () => console.log('Ready'));
client.on('result_batch', (batch) => console.log(batch));
```

**Events:**
- `opened` - Index loaded
- `result_batch` - Data chunk received
- `query_complete` - All results received
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

**Socket Events:**

| Event | Data | Response Event | Purpose |
|-------|------|---|---------|
| `open` | `{indexPath}` | `opened` | Initialize index |
| `query` | `{sparql}` | `result_batch`, `query_complete` | Stream results |
| `close` | none | `closed` | Disconnect |

**Example:**
```javascript
socket.emit('open', { indexPath: '/index' });
socket.on('result_batch', (data) => {
  console.log('Batch size:', data.rows.length);
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
const { ProtocolInstrumentation } = require('./protocol-instrumentation');

const instrumentation = new ProtocolInstrumentation();

// Automatically tracks:
// - Metrics (latency, count, errors)
// - Resilience (retries, backoff)
// - Contracts (data validation)
// - Health (per-operation status)
```

**Provides:**
- `getMetrics()` - Operation statistics
- `getHealth()` - System health status
- `getContracts()` - Active contracts

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
await client.open('/path/to/index'); // Don't forget this!
await client.query('...');
```

### "Connection refused" on Port 3000

**Cause:** HTTP server not running

**Fix:**
```bash
node js/server.js
```

### WebSocket Results Arriving Very Slowly

**Cause:** Batch size too small or network latency

**Fix:** Check batch size in `websocket-server.js`, or increase client concurrency

### Memory Usage Growing Over Time

**Cause:** Results not being cleared, cache growing

**Fix:**
```javascript
await client.eraseResult(name);  // Clear specific cache
await client.clearCache();       // Clear all caches
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

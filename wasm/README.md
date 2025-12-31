# QLever WASM Bindings

High-performance WebAssembly bindings for QLever, enabling full SPARQL 1.1 queries from JavaScript/Node.js in both browser and server environments.

**📦 Published on npm as [`qlever-wasm`](https://www.npmjs.com/package/qlever-wasm)**
**📖 Full documentation available in [README_NPM.md](./README_NPM.md)**

## Features

- ✅ **Cross-Platform**: Works in browsers and Node.js
- ✅ **Type-Safe**: Full TypeScript support with type definitions
- ✅ **High-Performance**: Compiled Rust to WebAssembly
- ✅ **Easy to Use**: Simple API for SPARQL queries
- ✅ **Query Builder**: Fluent API for constructing SPARQL queries
- ✅ **Async/Await**: Modern async/await support
- ✅ **Error Handling**: Comprehensive error messages

## Installation

```bash
npm install qlever-wasm
# or
yarn add qlever-wasm
```

## Quick Start

### Browser Usage

```html
<script type="module">
  import * as qlever from 'qlever-wasm/browser';

  const client = new qlever.QleverClient('http://localhost:7023');

  const response = await client.query(
    'SELECT * WHERE { ?s ?p ?o } LIMIT 10',
    'json'
  );

  console.log(response.data());
</script>
```

### Node.js Usage

```javascript
const qlever = require('qlever-wasm/node');

const client = new qlever.QleverClient('http://localhost:7023');

async function query() {
  const response = await client.query(
    'SELECT * WHERE { ?s ?p ?o } LIMIT 10',
    'json'
  );
  console.log(response.data());
}

query().catch(console.error);
```

## API Documentation

### QleverClient

Main client for executing SPARQL queries.

#### Constructor

```typescript
const client = new QleverClient(endpoint: string);
```

- `endpoint`: QLever server endpoint (e.g., `http://localhost:7023`)

#### Methods

**`query(query: string, format: string): Promise<QueryResponse>`**

Execute a SPARQL query.

```typescript
const response = await client.query(
  'SELECT * WHERE { ?s ?p ?o } LIMIT 10',
  'json'
);
```

Parameters:
- `query`: SPARQL query string
- `format`: Result format (`'json'`, `'xml'`, `'csv'`)

**`query_with_headers(query: string, format: string, headers: Record<string, string>): Promise<QueryResponse>`**

Execute a query with custom headers.

```typescript
const response = await client.query_with_headers(
  'SELECT * WHERE { ?s ?p ?o }',
  'json',
  {
    'Authorization': 'Bearer token',
    'Custom-Header': 'value'
  }
);
```

**`ping(): Promise<boolean>`**

Check if the server is reachable.

```typescript
const isHealthy = await client.ping();
console.log(isHealthy ? 'Server OK' : 'Server unreachable');
```

**`endpoint(): string`**

Get the endpoint URL.

```typescript
console.log(client.endpoint());
```

### QueryResponse

Represents the response from a query.

#### Methods

**`data(): any`**

Get the response data.

```typescript
const results = response.data();
```

**`format(): string`**

Get the response format.

```typescript
console.log(response.format()); // 'json', 'xml', or 'csv'
```

**`execution_time_ms(): number`**

Get the execution time in milliseconds.

```typescript
console.log(`Query took ${response.execution_time_ms()}ms`);
```

**`to_json(): any`**

Convert response to JSON object (for JSON format only).

```typescript
const jsonData = response.to_json();
```

**`to_string(): string`**

Convert response to string (for XML/CSV).

```typescript
const csvData = response.to_string();
```

### QueryBuilder

Fluent API for building SPARQL queries.

```typescript
const query = new QueryBuilder()
  .select('?subject ?predicate ?object')
  .where_clause('?subject ?predicate ?object')
  .build();

console.log(query);
// Output:
// SELECT ?subject ?predicate ?object
// WHERE {
//   ?subject ?predicate ?object
// }
```

#### Methods

**`select(vars: string): QueryBuilder`**

Add SELECT clause.

**`from(graph: string): QueryBuilder`**

Add FROM clause.

**`where_clause(pattern: string): QueryBuilder`**

Add WHERE clause.

**`build(): string`**

Build the final query string.

## Examples

See [examples/](./examples/) directory for complete examples:
- [browser.html](./examples/browser.html) - Browser-based interactive interface
- [node.js](./examples/node.js) - Node.js server-side usage
- [advanced.js](./examples/advanced.js) - Complete SPARQL 1.1 feature demonstrations

### Quick Example: Simple Query

```typescript
import * as qlever from 'qlever-wasm/browser';

const client = new qlever.QleverClient('http://localhost:7023');

const response = await client.query(
  'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 100',
  'json'
);
console.log(response.data());
```

### Quick Example: Using Query Builder

```typescript
const builder = new qlever.QueryBuilder()
  .select('?subject ?label')
  .where_clause('?subject rdfs:label ?label')
  .filter('STRLEN(?label) > 3')
  .limit(100)
  .build();

const response = await client.query(builder.build(), 'json');
console.log(response.data());
```

### Advanced Features Examples

**FILTER with Expressions**
```typescript
new qlever.QueryBuilder()
  .select('?name ?age')
  .where_clause('?person foaf:name ?name ; foaf:age ?age')
  .filter('?age > 21')
  .filter('CONTAINS(?name, "John")')
```

**OPTIONAL Patterns**
```typescript
new qlever.QueryBuilder()
  .select('?person ?name ?email')
  .where_clause('?person foaf:name ?name')
  .optional('?person foaf:mbox ?email')  // Email is optional
```

**GROUP BY Aggregation**
```typescript
new qlever.QueryBuilder()
  .select('?author COUNT(?book) AS ?bookCount')
  .where_clause('?book dc:creator ?author')
  .group_by('?author')
  .order_by_desc('?bookCount')
```

**CONSTRUCT Query (Create RDF)**
```typescript
new qlever.QueryBuilder()
  .construct_query()
  .construct('?person foaf:knows ?friend')
  .where_clause('?person foaf:knows ?friend')
```

See [README_NPM.md](./README_NPM.md) for 20+ detailed examples covering all SPARQL features.

## Building from Source

### Prerequisites

- Rust 1.56+ ([Install](https://rustup.rs/))
- Node.js 14+ and npm
- wasm-pack (`cargo install wasm-pack`)

### Build Steps

```bash
# Install dependencies
npm install

# Build WASM module
npm run build

# Build browser bundle
npm run build:browser

# Build Node.js bundle
npm run build:node

# Build all
npm run build:all
```

### Development Build

For faster development builds without optimizations:

```bash
npm run build:dev
```

## Testing

```bash
# Run tests in browser
npm test

# Run Node.js tests
npm run test:node
```

## Configuration

### Environment Variables

- `QLEVER_ENDPOINT`: Set the default QLever endpoint

```bash
export QLEVER_ENDPOINT=http://example.com:7023
npm run test:node
```

## Performance Tips

1. **Reuse client instances**: Create one client and reuse it for multiple queries
2. **Use JSON format**: JSON is faster than XML or CSV
3. **Limit results**: Use LIMIT clause in queries to reduce data transfer
4. **Batch queries**: Use batch operations for multiple queries
5. **Enable compression**: Configure server-side compression for large responses

## Browser Compatibility

- Chrome/Edge 74+
- Firefox 79+
- Safari 14.1+
- Modern Node.js (14+)

## Troubleshooting

### "WebAssembly is not supported"

Ensure your browser supports WebAssembly. Check at [caniuse.com](https://caniuse.com/wasm).

### "Failed to fetch"

Ensure:
1. QLever server is running
2. CORS is properly configured
3. Endpoint URL is correct
4. Server allows requests from your origin

### "Query execution failed"

Check:
1. SPARQL query syntax
2. Server logs for errors
3. Query timeout settings

## Building for Production

```bash
npm run build
npm run prepublishOnly
```

This will:
1. Create optimized WASM module
2. Generate type definitions
3. Bundle for browser and Node.js
4. Prepare for npm publishing

## License

MIT - See LICENSE file

## Support

- [QLever GitHub](https://github.com/ad-freiburg/qlever)
- [SPARQL Documentation](https://www.w3.org/TR/sparql11-query/)
- [WebAssembly](https://webassembly.org/)

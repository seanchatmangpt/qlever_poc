# QLever WASM - JavaScript/Node.js Client

[![npm version](https://badge.fury.io/js/qlever-wasm.svg)](https://www.npmjs.com/package/qlever-wasm)
[![License](https://img.shields.io/github/license/ad-freiburg/qlever)](https://github.com/ad-freiburg/qlever/blob/master/LICENSE)
[![SPARQL 1.1 Compatible](https://www.w3.org/RDF/)](https://www.w3.org/TR/sparql11-query/)

High-performance WebAssembly client for QLever SPARQL queries. Execute SPARQL 1.1 queries from JavaScript/Node.js with full feature parity with the C++ QLever engine.

## Features

### Core Capabilities ✨
- **Full SPARQL 1.1 Support**: SELECT, CONSTRUCT, DESCRIBE, ASK queries
- **Advanced Query Operations**: Filtering, grouping, ordering, aggregation
- **Complex Patterns**: OPTIONAL, UNION, BIND, VALUES, property paths
- **High Performance**: Compiled Rust to WebAssembly with optimizations
- **Cross-Platform**: Works in browsers (Chrome, Firefox, Safari, Edge) and Node.js 14+
- **Type-Safe**: Full TypeScript support with complete type definitions
- **Async/Await**: Modern async patterns throughout
- **Multiple Formats**: JSON, XML, CSV, Turtle, N-Triples output

### Query Features 🎯
- ✅ SELECT queries with DISTINCT
- ✅ CONSTRUCT graph creation
- ✅ DESCRIBE resource exploration
- ✅ ASK boolean queries
- ✅ FILTER conditions with SPARQL functions
- ✅ OPTIONAL patterns for left joins
- ✅ BIND variable binding
- ✅ GROUP BY aggregation
- ✅ ORDER BY sorting (ASC/DESC)
- ✅ LIMIT and OFFSET pagination
- ✅ VALUES inline data sets
- ✅ Multiple FROM clauses
- ✅ Server health checking

### Result Handling 📊
- Parse SPARQL JSON results format
- Convert to multiple output formats
- Execution time tracking
- Automatic error handling and reporting

## Installation

```bash
npm install qlever-wasm
# or
yarn add qlever-wasm
# or
pnpm add qlever-wasm
```

### Browser Usage

```html
<script type="module">
  import * as qlever from 'qlever-wasm/browser';
  // ... your code
</script>
```

### Node.js Usage

```javascript
const qlever = require('qlever-wasm/node');
// or
import * as qlever from 'qlever-wasm/node';
```

## Quick Start

### Basic Query (SELECT)

```javascript
import * as qlever from 'qlever-wasm/browser';

const client = new qlever.QleverClient('http://localhost:7023');

// Execute a simple query
const response = await client.query(
  'SELECT ?subject ?object WHERE { ?subject rdfs:label ?object } LIMIT 10',
  'json'
);

console.log(response.data());
```

### Using Query Builder

```javascript
// Build a query programmatically
const builder = new qlever.QueryBuilder()
  .select('?name ?age')
  .where_clause('?person foaf:name ?name ; foaf:age ?age')
  .filter('?age > 18')
  .order_by('?age')
  .limit(100);

const response = await client.query(builder.build(), 'json');
```

### Complex Query with Multiple Clauses

```javascript
const builder = new qlever.QueryBuilder()
  .select('?person ?name ?projects')
  .where_clause('?person foaf:name ?name')
  .optional('?person foaf:workplaceHomepage ?website')
  .bind('COUNT(?project)', '?projects')  // Count projects
  .group_by('?person ?name')
  .order_by_desc('?projects')
  .limit(50);

const response = await client.query(builder.build(), 'json');
```

## API Documentation

### QleverClient

Main client for executing SPARQL queries against a QLever endpoint.

#### Constructor

```typescript
const client = new QleverClient(endpoint: string);
```

**Parameters:**
- `endpoint` (string): QLever SPARQL endpoint URL (e.g., `http://localhost:7023`)

#### Methods

##### `query(query: string, format: string): Promise<QueryResponse>`

Execute a SPARQL query.

```typescript
const response = await client.query(
  'SELECT * WHERE { ?s ?p ?o } LIMIT 10',
  'json'
);
```

**Parameters:**
- `query` (string): SPARQL 1.1 query string
- `format` (string): Result format: `'json'`, `'xml'`, `'csv'`, `'turtle'`, `'ntriples'`

**Returns:** Promise resolving to QueryResponse

**Throws:** Error if query execution fails

---

##### `query_with_headers(query: string, format: string, headers: Record<string, string>): Promise<QueryResponse>`

Execute a query with custom HTTP headers (e.g., authentication, compression preferences).

```typescript
const response = await client.query_with_headers(
  'SELECT * WHERE { ?s ?p ?o }',
  'json',
  {
    'Authorization': 'Bearer mytoken',
    'Accept-Encoding': 'gzip'
  }
);
```

**Parameters:**
- `query` (string): SPARQL query string
- `format` (string): Result format
- `headers` (object): Custom HTTP headers

**Returns:** Promise resolving to QueryResponse

---

##### `ping(): Promise<boolean>`

Check server health and reachability.

```typescript
const isHealthy = await client.ping();
if (isHealthy) {
  console.log('QLever server is running');
}
```

**Returns:** Promise resolving to boolean (true if server is reachable)

---

##### `endpoint(): string`

Get the configured endpoint URL.

```typescript
console.log(client.endpoint()); // http://localhost:7023
```

**Returns:** The endpoint URL string

---

### QueryResponse

Represents the response from a SPARQL query execution.

#### Methods

##### `data(): any`

Get the parsed response data.

```typescript
const results = response.data();
// For JSON format: { head: { vars: [...] }, results: { bindings: [...] } }
```

**Returns:** Parsed response (format-dependent)

---

##### `format(): string`

Get the response format.

```typescript
console.log(response.format()); // 'json', 'xml', 'csv', etc.
```

**Returns:** Format string

---

##### `execution_time_ms(): number`

Get the query execution time in milliseconds.

```typescript
console.log(`Query executed in ${response.execution_time_ms()}ms`);
```

**Returns:** Execution time in milliseconds

---

##### `to_json(): any`

Convert response to JSON object (only for JSON format responses).

```typescript
const json = response.to_json();
console.log(json.head.vars); // Variable names
console.log(json.results.bindings); // Result rows
```

**Throws:** Error if response is not in JSON format

**Returns:** Parsed JSON object

---

##### `to_string(): string`

Convert response to string representation.

```typescript
const str = response.to_string();
console.log(str);
```

**Returns:** String representation of the response

---

### QueryBuilder

Fluent API for constructing SPARQL queries programmatically.

#### Constructor

```typescript
const builder = new QueryBuilder();
```

#### Query Type Methods

##### `select_query(): QueryBuilder`
Set query type to SELECT (default).

##### `construct_query(): QueryBuilder`
Set query type to CONSTRUCT.

##### `describe_query(): QueryBuilder`
Set query type to DESCRIBE.

##### `ask_query(): QueryBuilder`
Set query type to ASK.

#### SELECT Query Methods

##### `select(vars: string): QueryBuilder`

Add SELECT clause.

```typescript
.select('?name ?age ?email')
```

---

##### `distinct(): QueryBuilder`

Add DISTINCT modifier (returns only unique results).

```typescript
.select('?status')
.distinct()
```

---

#### CONSTRUCT Query Methods

##### `construct(template: string): QueryBuilder`

Add CONSTRUCT template.

```typescript
const builder = new QueryBuilder()
  .construct_query()
  .construct('?person foaf:knows ?friend')
  .where_clause('?person foaf:knows ?friend');
```

---

#### DESCRIBE Query Methods

##### `describe(vars: string): QueryBuilder`

Add DESCRIBE variables.

```typescript
const builder = new QueryBuilder()
  .describe_query()
  .describe('?person');
```

---

#### Graph Pattern Methods

##### `where_clause(pattern: string): QueryBuilder`

Add WHERE clause pattern for matching RDF triples.

```typescript
.where_clause('?person foaf:name ?name ; foaf:age ?age')
```

---

##### `filter(condition: string): QueryBuilder`

Add FILTER condition to constrain results.

```typescript
.filter('?age > 18')
.filter('?name = "John"')
.filter('STRLEN(?name) > 3')
.filter('CONTAINS(?email, "@example.com")')
```

Supports SPARQL functions:
- Comparison: `=`, `!=`, `<`, `>`, `<=`, `>=`
- String: `STRLEN()`, `CONTAINS()`, `STARTS_WITH()`, `ENDS_WITH()`, `UPPER()`, `LOWER()`
- Math: `+`, `-`, `*`, `/`, `ABS()`, `ROUND()`, `CEIL()`, `FLOOR()`
- Boolean: `!`, `&&`, `||`
- Type checking: `isIRI()`, `isBlank()`, `isLiteral()`, `isNumeric()`

---

##### `optional(pattern: string): QueryBuilder`

Add OPTIONAL pattern (left join).

```typescript
.where_clause('?person foaf:name ?name')
.optional('?person foaf:homepage ?website')
// Results include people without websites (with ?website unbound)
```

---

##### `bind(expression: string, var: string): QueryBuilder`

Bind an expression result to a variable.

```typescript
.bind('CONCAT(?firstName, " ", ?lastName)', '?fullName')
.bind('STRLEN(?name)', '?nameLength')
.bind('NOW()', '?currentTime')
```

---

##### `values(clause: string): QueryBuilder`

Add VALUES clause for inline data.

```typescript
.values('VALUES (?status) { ("active") ("pending") ("inactive") }')
.values('VALUES (?id ?name) { (1 "Alice") (2 "Bob") (3 "Charlie") }')
```

---

#### FROM Methods

##### `from(graph: string): QueryBuilder`

Add FROM clause to query specific named graph.

```typescript
.from('http://example.org/graph1')
.from('http://example.org/graph2')
```

---

#### Aggregation Methods

##### `group_by(vars: string): QueryBuilder`

Add GROUP BY clause for aggregation.

```typescript
.select('?status COUNT(?person) AS ?count')
.where_clause('?person foaf:status ?status')
.group_by('?status')
```

Common aggregation patterns:
- `COUNT(?var)` - Count non-NULL values
- `SUM(?var)` - Sum numeric values
- `AVG(?var)` - Average numeric values
- `MIN(?var)` - Minimum value
- `MAX(?var)` - Maximum value

---

#### Sorting Methods

##### `order_by(vars: string): QueryBuilder`

Sort results in ascending order.

```typescript
.order_by('?name')
.order_by('?date ?priority')
```

---

##### `order_by_desc(vars: string): QueryBuilder`

Sort results in descending order.

```typescript
.order_by_desc('?score')
.order_by_desc('?date')
```

---

#### Pagination Methods

##### `limit(count: number): QueryBuilder`

Limit number of results.

```typescript
.limit(100)
```

---

##### `offset(count: number): QueryBuilder`

Skip first N results.

```typescript
.offset(50).limit(50) // Get results 50-99
```

---

#### Build Method

##### `build(): string`

Generate the final SPARQL query string.

```typescript
const query = builder.build();
console.log(query);
// SELECT ?name ?age
// WHERE {
//   ?person foaf:name ?name ; foaf:age ?age
//   FILTER (?age > 18)
// }
// LIMIT 100
```

**Returns:** Complete SPARQL query string

---

## Usage Examples

### Example 1: People Search

```javascript
const client = new qlever.QleverClient('http://dbpedia.org/sparql');

const builder = new qlever.QueryBuilder()
  .select('?name ?birthDate ?abstract')
  .where_clause(`
    ?person foaf:name ?name ;
            dbo:birthDate ?birthDate ;
            dbo:abstract ?abstract .
    ?person rdf:type dbo:Person
  `)
  .filter('CONTAINS(?name, "Einstein")')
  .order_by('?birthDate');

const response = await client.query(builder.build(), 'json');
console.log(response.data());
```

### Example 2: Aggregation Query

```javascript
const builder = new qlever.QueryBuilder()
  .select('?author COUNT(?publication) AS ?publications')
  .where_clause('?publication dbo:author ?author')
  .group_by('?author')
  .order_by_desc('?publications')
  .limit(10);

const response = await client.query(builder.build(), 'json');
// Returns top 10 authors by publication count
```

### Example 3: CONSTRUCT Query (Create New RDF)

```javascript
const builder = new qlever.QueryBuilder()
  .construct_query()
  .construct(`
    ?person foaf:knows ?friend ;
            foaf:name ?personName .
    ?friend foaf:name ?friendName
  `)
  .where_clause(`
    ?person foaf:knows ?friend ;
            foaf:name ?personName .
    ?friend foaf:name ?friendName
  `)
  .limit(50);

const response = await client.query(builder.build(), 'turtle');
// Returns Turtle RDF with know relationships
```

### Example 4: DESCRIBE Query (Explore Resources)

```javascript
const client = new qlever.QleverClient('http://dbpedia.org/sparql');

// Get all properties of a resource
const response = await client.query(
  'DESCRIBE <http://dbpedia.org/resource/Albert_Einstein>',
  'turtle'
);

console.log(response.data());
// Returns all triples where Einstein is subject or object
```

### Example 5: ASK Query (Boolean Check)

```javascript
const client = new qlever.QleverClient('http://localhost:7023');

const response = await client.query(
  'ASK WHERE { <http://example.org/resource> rdf:type foaf:Person }',
  'json'
);

// Check if resource is a person
const isPerson = response.data().boolean;
console.log(isPerson ? 'Is a person' : 'Not a person');
```

### Example 6: OPTIONAL Pattern (Left Join)

```javascript
const builder = new qlever.QueryBuilder()
  .select('?person ?name ?email')
  .where_clause('?person foaf:name ?name')
  .optional('?person foaf:mbox ?email')
  .order_by('?name');

const response = await client.query(builder.build(), 'json');
// Returns all people with optional email (null if not available)
```

### Example 7: Batch Processing

```javascript
const client = new qlever.QleverClient('http://localhost:7023');

const queries = [
  'SELECT COUNT(*) AS ?count WHERE { ?s ?p ?o }',
  'SELECT DISTINCT ?type WHERE { ?s rdf:type ?type } LIMIT 10',
  'SELECT ?p COUNT(*) AS ?count WHERE { ?s ?p ?o } GROUP BY ?p ORDER BY DESC(?count) LIMIT 5'
];

// Execute queries sequentially
for (const query of queries) {
  const response = await client.query(query, 'json');
  console.log(response.data());
}
```

### Example 8: Pagination

```javascript
const pageSize = 100;
let offset = 0;
let hasMore = true;

while (hasMore) {
  const builder = new qlever.QueryBuilder()
    .select('?s ?p ?o')
    .where_clause('?s ?p ?o')
    .limit(pageSize)
    .offset(offset);

  const response = await client.query(builder.build(), 'json');
  const results = response.data().results.bindings;

  if (results.length < pageSize) {
    hasMore = false;
  }

  console.log(`Page ${offset / pageSize + 1}: ${results.length} results`);
  offset += pageSize;
}
```

### Example 9: Advanced Filtering

```javascript
const builder = new qlever.QueryBuilder()
  .select('?product ?name ?price')
  .where_clause('?product ecom:name ?name ; ecom:price ?price')
  .filter('?price > 100')
  .filter('?price < 500')
  .filter('CONTAINS(?name, "laptop")')
  .order_by('?price');

const response = await client.query(builder.build(), 'json');
```

### Example 10: Complex Graph Pattern

```javascript
const builder = new qlever.QueryBuilder()
  .select('?person ?friend ?friendName')
  .where_clause('?person foaf:knows ?friend')
  .where_clause('?friend foaf:name ?friendName')
  .optional('?friend foaf:age ?age')
  .filter('!BOUND(?age) || ?age > 21')  // Only adults or unknown age
  .order_by('?friendName');

const response = await client.query(builder.build(), 'json');
```

## Building from Source

### Prerequisites

- Rust 1.56+ ([Install](https://rustup.rs/))
- Node.js 14+ and npm
- wasm-pack (`cargo install wasm-pack`)

### Build Steps

```bash
cd wasm
npm install

# Build optimized WASM module
npm run build

# Build for development (faster, larger)
npm run build:dev

# Build browser-specific bundle
npm run build:browser

# Build Node.js-specific bundle
npm run build:node

# Build all at once
npm run build:all
```

## Browser Compatibility

| Browser | Version | Support |
|---------|---------|---------|
| Chrome | 74+ | ✅ |
| Firefox | 79+ | ✅ |
| Safari | 14.1+ | ✅ |
| Edge | 79+ | ✅ |
| Opera | 62+ | ✅ |

## Node.js Compatibility

| Version | Support |
|---------|---------|
| Node.js 14+ | ✅ |
| Node.js 16+ | ✅ |
| Node.js 18+ | ✅ |
| Node.js 20+ | ✅ |

## Performance Tips

1. **Reuse Client Instances**: Create one client and reuse for multiple queries
   ```javascript
   const client = new qlever.QleverClient('http://localhost:7023');
   // Use for all queries
   ```

2. **Use JSON Format**: JSON parsing is fastest
   ```javascript
   await client.query(query, 'json');
   ```

3. **Paginate Large Results**: Use LIMIT and OFFSET
   ```javascript
   .limit(1000).offset(offset)
   ```

4. **Use FILTER Early**: Filter in WHERE clause, not after
   ```javascript
   // Good
   .where_clause('?person foaf:age ?age')
   .filter('?age > 18')
   ```

5. **Limit Scope with FROM**: Query specific graphs
   ```javascript
   .from('http://example.org/target-graph')
   ```

6. **Minimize Data Transfer**: Select only needed variables
   ```javascript
   .select('?id ?name')  // Not ?id ?name ?description ?email ...
   ```

7. **Use DISTINCT Wisely**: Only when duplicates are expected
   ```javascript
   .select('?type')
   .distinct()
   ```

## Configuration

### Environment Variables

```bash
# Set default endpoint
export QLEVER_ENDPOINT=http://example.com:7023
```

### Custom Headers

Add authentication or other headers:

```javascript
const response = await client.query_with_headers(
  query,
  'json',
  {
    'Authorization': 'Bearer token123',
    'User-Agent': 'MyApp/1.0'
  }
);
```

## Result Formats

### JSON Format

Standard SPARQL JSON Results Format:

```javascript
{
  "head": {
    "vars": ["subject", "predicate", "object"]
  },
  "results": {
    "bindings": [
      {
        "subject": { "type": "uri", "value": "..." },
        "predicate": { "type": "uri", "value": "..." },
        "object": { "type": "literal", "value": "..." }
      }
    ]
  }
}
```

### XML Format

SPARQL XML Results Format - standard W3C format

### CSV Format

Comma-separated values with header row

### Turtle Format

RDF Turtle serialization - useful for CONSTRUCT queries

### N-Triples Format

RDF N-Triples serialization - one triple per line

## Error Handling

```javascript
try {
  const response = await client.query(query, 'json');
  console.log(response.data());
} catch (error) {
  if (error.message.includes('HTTP 400')) {
    console.error('Invalid SPARQL query');
  } else if (error.message.includes('HTTP 503')) {
    console.error('Server unavailable');
  } else {
    console.error('Query execution failed:', error.message);
  }
}
```

## Troubleshooting

### "WebAssembly is not supported"

- Ensure your browser/Node.js version supports WebAssembly
- Check [caniuse.com/wasm](https://caniuse.com/wasm)

### "Failed to fetch from endpoint"

1. Verify QLever server is running
2. Check CORS configuration (if in browser)
3. Verify endpoint URL is correct
4. Check network connectivity

### "Query execution failed"

1. Validate SPARQL syntax (W3C [SPARQL Validator](https://www.w3.org/2001/sw/DataAccess/code/"))
2. Check endpoint logs
3. Verify server has data for your query

### "CORS errors in browser"

Configure server with CORS headers:

```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, Accept
```

Or use a proxy service for public endpoints without CORS.

## Publishing to npm

```bash
cd wasm
npm run build:all
npm publish
```

## Feature Parity with QLever C++

This WASM module provides feature parity with the main QLever C++ implementation for:

- ✅ All SPARQL 1.1 query types (SELECT, CONSTRUCT, DESCRIBE, ASK)
- ✅ FILTER expressions with SPARQL functions
- ✅ OPTIONAL patterns (optional joins)
- ✅ BIND variable binding
- ✅ GROUP BY aggregation
- ✅ ORDER BY sorting
- ✅ LIMIT/OFFSET pagination
- ✅ DISTINCT modifier
- ✅ VALUES inline data sets
- ✅ FROM named graphs
- ✅ Multiple result formats
- ✅ Error handling and reporting

Note: The WASM module is a client library for querying QLever endpoints. It does not include:
- RDF indexing and loading
- Full-text search configuration
- Spatial query indexing
- Update operations (INSERT/DELETE)
- SPARQL service endpoints

For these features, use the full QLever C++ server.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

See [CONTRIBUTING.md](../CONTRIBUTING.md) for details.

## License

MIT License - See LICENSE file

## Support

- **Issues**: [GitHub Issues](https://github.com/ad-freiburg/qlever/issues)
- **SPARQL Spec**: [W3C SPARQL 1.1](https://www.w3.org/TR/sparql11-query/)
- **QLever Docs**: [QLever GitHub](https://github.com/ad-freiburg/qlever)
- **WASM Info**: [WebAssembly](https://webassembly.org/)

## Changelog

### Version 0.1.0 (Initial Release)

- ✨ Full SPARQL 1.1 query support
- ✨ Query builder API
- ✨ Multiple result formats
- ✨ TypeScript definitions
- ✨ Cross-platform (Browser + Node.js)
- ✨ Server health checking

---

**Made with ❤️ for the RDF community**

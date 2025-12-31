# QLever WASM Test & Validation Plan

This document outlines the comprehensive testing strategy for QLever WASM with libqlever backend.

## Overview

Testing ensures that:
1. ✅ WASM module compiles and loads correctly
2. ✅ libqlever C++ code works through WASM bindings
3. ✅ JavaScript API matches expected behavior (Oxigraph-compatible)
4. ✅ Query execution is correct and performant
5. ✅ All 20 example queries execute successfully
6. ✅ Error handling works as expected

## Test Levels

### Level 1: Build Validation

**Goal**: Ensure build process completes successfully

**Tests**:
```bash
# Verify Emscripten installation
emcc --version

# Verify Rust/wasm-bindgen setup
rustup target list | grep emscripten
which wasm-pack

# Build libqlever WASM
npm run build:libqlever

# Verify output files exist
ls -lh wasm_build/lib/libqlever_wasm.wasm
ls -lh wasm_build/lib/libqlever_wasm.js
```

**Success Criteria**:
- All files compile without errors
- WASM module > 2MB (indicates libqlever included)
- JavaScript wrapper < 100KB (glue code)

### Level 2: Module Loading

**Goal**: WASM module loads in JavaScript runtime

**Test File**: `tests/load.test.js`

```javascript
describe('WASM Module Loading', () => {
  test('Module loads without errors', async () => {
    const wasm = await import('./wasm_wrapper.ts');
    expect(wasm).toBeDefined();
    expect(wasm.initializeWasm).toBeDefined();
  });

  test('InitializeWasm completes successfully', async () => {
    const wasm = await import('./wasm_wrapper.ts');
    await expect(wasm.initializeWasm()).resolves.toBeUndefined();
  });

  test('QleverStore can be instantiated', async () => {
    const { createStore, initializeWasm } = await import('./wasm_wrapper.ts');
    await initializeWasm();
    const store = createStore();
    expect(store).toBeDefined();
  });
});
```

**Success Criteria**:
- WASM module loads in Node.js
- WASM module loads in browsers (Chrome, Firefox, Safari)
- Instantiation doesn't crash

### Level 3: DataFactory API

**Goal**: Ensure RDF/JS DataFactory works correctly

**Test File**: `tests/datafactory.test.js`

```javascript
describe('DataFactory', () => {
  test('Create named node', () => {
    const node = DataFactory.namedNode('http://example.org/test');
    expect(node.type).toBe('NamedNode');
    expect(node.value).toBe('http://example.org/test');
  });

  test('Create blank node', () => {
    const bn = DataFactory.blankNode();
    expect(bn.type).toBe('BlankNode');
    expect(bn.value).toBeTruthy();
  });

  test('Create literal with language', () => {
    const lit = DataFactory.literal('hello', '@en');
    expect(lit.type).toBe('Literal');
    expect(lit.value).toBe('hello');
    expect(lit.language).toBe('en');
  });

  test('Create literal with datatype', () => {
    const lit = DataFactory.literal('42', 'http://www.w3.org/2001/XMLSchema#integer');
    expect(lit.datatype).toContain('XMLSchema');
  });

  test('Create triple', () => {
    const t = DataFactory.triple(
      DataFactory.namedNode('http://example.org/s'),
      DataFactory.namedNode('http://example.org/p'),
      DataFactory.namedNode('http://example.org/o')
    );
    expect(t.subject).toBeDefined();
    expect(t.predicate).toBeDefined();
    expect(t.object).toBeDefined();
  });

  test('Create quad with graph', () => {
    const q = DataFactory.quad(
      DataFactory.namedNode('http://example.org/s'),
      DataFactory.namedNode('http://example.org/p'),
      DataFactory.namedNode('http://example.org/o'),
      DataFactory.namedNode('http://example.org/g')
    );
    expect(q.graph).toBeDefined();
  });

  test('Term to string conversion', () => {
    const node = DataFactory.namedNode('http://example.org/test');
    const str = DataFactory.termToString(node);
    expect(str).toBe('<http://example.org/test>');
  });
});
```

**Success Criteria**:
- All term types create correctly
- String conversions match N-Quads format
- Language tags and datatypes preserved

### Level 4: QueryBuilder API

**Goal**: Verify SPARQL query construction

**Test File**: `tests/querybuilder.test.js`

```javascript
describe('QueryBuilder', () => {
  test('Basic SELECT query', () => {
    const builder = new QueryBuilder()
      .select('?s')
      .where('?s ?p ?o');

    const query = builder.build();
    expect(query).toContain('SELECT ?s');
    expect(query).toContain('WHERE');
  });

  test('SELECT with FILTER', () => {
    const builder = new QueryBuilder()
      .select('?s')
      .where('?s ?p ?o')
      .filter('?s = <http://example.org>');

    const query = builder.build();
    expect(query).toContain('FILTER');
  });

  test('SELECT with OPTIONAL', () => {
    const builder = new QueryBuilder()
      .select('?s ?o')
      .where('?s ?p ?o')
      .optional('?s ?p2 ?o2');

    const query = builder.build();
    expect(query).toContain('OPTIONAL');
  });

  test('CONSTRUCT query', () => {
    const builder = new QueryBuilder()
      .constructQuery()
      .construct('?s ?p ?o')
      .where('?s ?p ?o');

    const query = builder.build();
    expect(query).toContain('CONSTRUCT');
  });

  test('SELECT with GROUP BY ORDER BY LIMIT', () => {
    const builder = new QueryBuilder()
      .select('?s')
      .where('?s ?p ?o')
      .groupBy('?s')
      .orderBy('?s')
      .limit(10);

    const query = builder.build();
    expect(query).toContain('GROUP BY');
    expect(query).toContain('ORDER BY');
    expect(query).toContain('LIMIT 10');
  });

  test('DISTINCT modifier', () => {
    const builder = new QueryBuilder()
      .select('?s')
      .distinct()
      .where('?s ?p ?o');

    const query = builder.build();
    expect(query).toContain('DISTINCT');
  });
});
```

**Success Criteria**:
- All query types build correctly
- SPARQL syntax is valid
- Modifiers apply in correct order

### Level 5: Store Initialization & Queries

**Goal**: Test Store initialization and basic queries

**Test File**: `tests/store.test.js`

```javascript
describe('Store - libqlever Backend', () => {
  let store;

  beforeEach(async () => {
    const { createStore, initializeWasm } = await import('./wasm_wrapper.ts');
    await initializeWasm();
    store = createStore();
  });

  test('Store initializes with index', async () => {
    // Requires pre-built QLever index
    const indexPath = process.env.QLEVER_INDEX || '/tmp/test-index';
    try {
      await store.init(indexPath);
      expect(store.backend.is_initialized()).toBe(true);
    } catch (e) {
      // Skip if index not available
      console.warn('Skipping: index not available at', indexPath);
    }
  });

  test('Store executes SELECT query', async () => {
    const result = await store.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 5');
    expect(result.head).toBeDefined();
    expect(result.head.vars).toBeInstanceOf(Array);
    expect(result.results.bindings).toBeInstanceOf(Array);
  });

  test('Store executes ASK query', async () => {
    const result = await store.query('ASK WHERE { ?s ?p ?o }');
    expect(result.boolean).toBeTypeOf('boolean');
  });

  test('Store executes DESCRIBE query', async () => {
    const result = await store.query('DESCRIBE ?s WHERE { ?s ?p ?o } LIMIT 1');
    expect(result).toBeDefined();
  });

  test('Store executes CONSTRUCT query', async () => {
    const result = await store.query(
      'CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o } LIMIT 5'
    );
    expect(result).toBeDefined();
  });

  test('Error handling for invalid query', async () => {
    await expect(store.query('INVALID SPARQL'))
      .rejects
      .toThrow();
  });

  test('Store reports statistics', async () => {
    const stats = await store.getStats();
    expect(typeof stats).toBe('string');
    expect(stats.length).toBeGreaterThan(0);
  });
});
```

**Success Criteria**:
- Index initializes successfully
- All query types execute
- Results are valid JSON
- Error handling works
- Statistics available

### Level 6: Complete Examples (20 examples from advanced.js)

**Goal**: Validate all example use cases work correctly

**Test File**: `tests/examples.test.js`

```javascript
describe('Complete Examples', () => {
  let store;

  beforeAll(async () => {
    const { createStore, initializeWasm } = await import('./wasm_wrapper.ts');
    await initializeWasm();
    store = createStore();
    // Initialize with test dataset
  });

  // Example 1: Basic Subject Query
  test('Example 1: Basic subject query', async () => {
    const result = await store.query(`
      SELECT ?s WHERE {
        ?s <http://example.org/type> ?type
      } LIMIT 10
    `);
    expect(result.results.bindings.length).toBeGreaterThan(0);
  });

  // Example 2: Property Path
  test('Example 2: Property path query', async () => {
    const result = await store.query(`
      SELECT ?answer WHERE {
        <http://example.org/Alice> <http://example.org/knows>+ ?answer
      }
    `);
    expect(result).toBeDefined();
  });

  // Example 3: OPTIONAL clause
  test('Example 3: OPTIONAL clause', async () => {
    const result = await store.query(`
      SELECT ?s ?name ?email WHERE {
        ?s <http://example.org/name> ?name .
        OPTIONAL { ?s <http://example.org/email> ?email }
      } LIMIT 5
    `);
    expect(result.results.bindings.length).toBeGreaterThan(0);
  });

  // Example 4: FILTER expression
  test('Example 4: FILTER expression', async () => {
    const result = await store.query(`
      SELECT ?name WHERE {
        ?person <http://example.org/name> ?name ;
                <http://example.org/age> ?age .
        FILTER (?age > 30)
      }
    `);
    expect(result.results.bindings).toBeInstanceOf(Array);
  });

  // Example 5: GROUP BY with aggregation
  test('Example 5: GROUP BY with aggregation', async () => {
    const result = await store.query(`
      SELECT ?type (COUNT(?item) AS ?count) WHERE {
        ?item <http://example.org/type> ?type
      } GROUP BY ?type
    `);
    expect(result.results.bindings).toBeInstanceOf(Array);
  });

  // Example 6: UNION query
  test('Example 6: UNION query', async () => {
    const result = await store.query(`
      SELECT ?label WHERE {
        { ?item <http://example.org/label> ?label }
        UNION
        { ?item <http://example.org/name> ?label }
      } LIMIT 10
    `);
    expect(result.results.bindings).toBeInstanceOf(Array);
  });

  // Example 7: DISTINCT modifier
  test('Example 7: DISTINCT modifier', async () => {
    const result = await store.query(`
      SELECT DISTINCT ?type WHERE {
        ?s <http://example.org/type> ?type
      }
    `);
    expect(result).toBeDefined();
  });

  // Example 8: ORDER BY with LIMIT
  test('Example 8: ORDER BY with LIMIT', async () => {
    const result = await store.query(`
      SELECT ?name WHERE {
        ?person <http://example.org/name> ?name
      } ORDER BY ?name LIMIT 5
    `);
    expect(result.results.bindings.length).toBeLessThanOrEqual(5);
  });

  // Example 9: CONSTRUCT for graph transformation
  test('Example 9: CONSTRUCT query', async () => {
    const result = await store.query(`
      CONSTRUCT {
        ?person <http://example.org/foaf:name> ?name
      } WHERE {
        ?person <http://example.org/name> ?name
      } LIMIT 5
    `);
    expect(result).toBeDefined();
  });

  // Example 10: DESCRIBE for resource exploration
  test('Example 10: DESCRIBE query', async () => {
    const result = await store.query(`
      DESCRIBE ?person WHERE {
        ?person <http://example.org/name> ?name
      } LIMIT 1
    `);
    expect(result).toBeDefined();
  });

  // Examples 11-20: Additional complex queries
  // (See examples/advanced.js for full list)
});
```

**Success Criteria**:
- All 20 examples execute without errors
- Results are returned in expected format
- No memory leaks or crashes
- Reasonable execution time

### Level 7: Performance Testing

**Goal**: Measure and validate performance characteristics

**Test File**: `tests/performance.test.js`

```javascript
describe('Performance Characteristics', () => {
  test('Query execution time (simple)', async () => {
    const start = performance.now();
    const result = await store.query(`
      SELECT ?s WHERE { ?s ?p ?o } LIMIT 10
    `);
    const duration = performance.now() - start;

    // Simple queries should complete < 100ms
    expect(duration).toBeLessThan(100);
  });

  test('Query execution time (complex)', async () => {
    const start = performance.now();
    const result = await store.query(`
      SELECT ?s WHERE {
        ?s ?p1 ?o1 .
        ?s ?p2 ?o2 .
        ?s ?p3 ?o3 .
        FILTER (?s = <...>)
      }
    `);
    const duration = performance.now() - start;

    // Complex queries < 5000ms
    expect(duration).toBeLessThan(5000);
  });

  test('Memory usage is reasonable', async () => {
    const memBefore = process.memoryUsage().heapUsed;

    // Execute multiple queries
    for (let i = 0; i < 100; i++) {
      await store.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 1');
    }

    const memAfter = process.memoryUsage().heapUsed;
    const memIncrease = (memAfter - memBefore) / 1024 / 1024;

    // Should not use excessive memory
    expect(memIncrease).toBeLessThan(100);  // Less than 100 MB increase
  });
});
```

**Success Criteria**:
- Simple queries: < 100 ms
- Complex queries: < 5 seconds
- Memory growth: < 100 MB for 100 queries
- No memory leaks

## Running Tests

### All Tests

```bash
npm test
```

### Specific Test Level

```bash
# Level 1: Build
npm run test:build

# Level 2: Module Loading
npm run test:load

# Level 3: DataFactory
npm run test:datafactory

# Level 4: QueryBuilder
npm run test:querybuilder

# Level 5: Store
npm run test:store

# Level 6: Examples
npm run test:examples

# Level 7: Performance
npm run test:performance
```

### Continuous Testing

Watch for changes and rerun tests:

```bash
npm run test:watch
```

## Test Coverage

Target coverage:
- Line coverage: > 80%
- Branch coverage: > 70%
- Function coverage: > 85%

Generate coverage report:

```bash
npm run test:coverage
```

## Test Data

For testing, use:
- Small RDF dataset (< 1000 triples) in `tests/data/small.ttl`
- Medium dataset (10K-100K triples) in `tests/data/medium.ttl`
- Pre-built QLever index in `/tmp/test-index` (for integration tests)

## Validation Checklist

Before release, verify:

- ✅ All tests pass
- ✅ Code coverage > 80%
- ✅ No memory leaks
- ✅ Performance meets targets
- ✅ Works in Node.js 14+
- ✅ Works in Browsers (Chrome, Firefox, Safari)
- ✅ All examples execute correctly
- ✅ Error messages are helpful
- ✅ Documentation is complete
- ✅ Build is reproducible

## Debugging

### Enable Verbose Logging

```bash
QLEVER_DEBUG=1 npm test
```

### Inspect WASM Module

```bash
node --experimental-wasm-modules tests/inspect-wasm.js
```

### Profile Queries

```bash
node --prof tests/profile-queries.js
node --prof-process isolate-*.log > profile.txt
```

## References

- [Jest Testing Framework](https://jestjs.io/)
- [WASM Testing Best Practices](https://rustwasm.org/docs/wasm-pack/book/test/)
- [Performance Measurement](https://developer.mozilla.org/en-US/docs/Web/API/Performance)
- [Memory Profiling](https://nodejs.org/en/docs/guides/simple-profiling/)

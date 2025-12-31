/**
 * QLever WASM Node.js Example
 *
 * This example demonstrates how to use QLever WASM bindings in Node.js
 *
 * Usage:
 *   npm run build
 *   node examples/node.js
 */

const qlever = require('../pkg/qlever_wasm');

// Simple example queries
const EXAMPLE_QUERIES = [
  'SELECT * WHERE { ?s ?p ?o } LIMIT 1',
  'SELECT DISTINCT ?p WHERE { ?s ?p ?o } LIMIT 5',
  'SELECT ?s WHERE { ?s a ?type } LIMIT 10',
];

/**
 * Execute a single query and display results
 */
async function executeQuery(endpoint, query, format = 'json') {
  try {
    console.log('\n📝 Query:', query);
    console.log('🔗 Endpoint:', endpoint);
    console.log('📊 Format:', format);
    console.log('---');

    const client = new qlever.QleverClient(endpoint);
    const startTime = Date.now();
    const response = await client.query(query, format);
    const endTime = Date.now();

    const data = response.data();
    console.log('✅ Success!');
    console.log('⏱️  Execution time:', (endTime - startTime) + 'ms');
    console.log('📋 Results:');
    console.log(JSON.stringify(data, null, 2));

    return {
      success: true,
      data,
      executionTime: endTime - startTime,
    };
  } catch (error) {
    console.error('❌ Error:', error.message);
    return {
      success: false,
      error: error.message,
    };
  }
}

/**
 * Check server health
 */
async function checkServerHealth(endpoint) {
  try {
    console.log('\n🏥 Checking server health...');
    console.log('🔗 Endpoint:', endpoint);
    console.log('---');

    const client = new qlever.QleverClient(endpoint);
    const isHealthy = await client.ping();

    if (isHealthy) {
      console.log('✅ Server is reachable');
      return true;
    } else {
      console.log('❌ Server is not responding');
      return false;
    }
  } catch (error) {
    console.error('❌ Error:', error.message);
    return false;
  }
}

/**
 * Batch query execution
 */
async function batchQueries(endpoint, queries, format = 'json') {
  console.log('\n📦 Executing batch queries...');
  console.log('🔗 Endpoint:', endpoint);
  console.log(`📝 Queries: ${queries.length}`);
  console.log('---');

  const results = [];
  for (let i = 0; i < queries.length; i++) {
    const query = queries[i];
    console.log(`\n[${i + 1}/${queries.length}] Executing query...`);

    try {
      const client = new qlever.QleverClient(endpoint);
      const startTime = Date.now();
      const response = await client.query(query, format);
      const endTime = Date.now();

      console.log(`✅ Success (${endTime - startTime}ms)`);
      results.push({
        success: true,
        query,
        data: response.data(),
        executionTime: endTime - startTime,
      });
    } catch (error) {
      console.log(`❌ Failed: ${error.message}`);
      results.push({
        success: false,
        query,
        error: error.message,
      });
    }
  }

  console.log('\n📊 Batch Summary:');
  const successful = results.filter(r => r.success).length;
  console.log(`  ✅ Successful: ${successful}/${results.length}`);
  console.log(`  ❌ Failed: ${results.length - successful}/${results.length}`);

  return results;
}

/**
 * Query builder example
 */
function queryBuilderExample() {
  console.log('\n🏗️  Query Builder Example');
  console.log('---');

  const builder = new qlever.QueryBuilder();
  const query = builder
    .select('?subject ?predicate')
    .where_clause('?subject ?predicate ?object')
    .build();

  console.log('Built query:');
  console.log(query);

  return query;
}

/**
 * Main example execution
 */
async function main() {
  console.log('🚀 QLever WASM Node.js Example');
  console.log('================================\n');

  // Configuration
  const ENDPOINT = process.env.QLEVER_ENDPOINT || 'http://localhost:7023';

  // Initialize panic hook
  try {
    qlever.set_panic_hook?.();
  } catch (e) {
    console.warn('Warning: Could not set panic hook:', e.message);
  }

  // Check if server is available
  const isHealthy = await checkServerHealth(ENDPOINT);

  if (!isHealthy) {
    console.log('\n⚠️  Server is not available. Please make sure QLever is running at:', ENDPOINT);
    console.log('   Start QLever with: ServerMain -p 7023');
    return;
  }

  // Run examples
  console.log('\n═════════════════════════════════════');
  console.log('EXAMPLE 1: Single Query');
  console.log('═════════════════════════════════════');
  await executeQuery(ENDPOINT, EXAMPLE_QUERIES[0]);

  console.log('\n═════════════════════════════════════');
  console.log('EXAMPLE 2: Query Builder');
  console.log('═════════════════════════════════════');
  const builtQuery = queryBuilderExample();

  console.log('\n═════════════════════════════════════');
  console.log('EXAMPLE 3: Batch Queries');
  console.log('═════════════════════════════════════');
  await batchQueries(ENDPOINT, EXAMPLE_QUERIES.slice(0, 2));

  console.log('\n═════════════════════════════════════');
  console.log('✨ All examples completed!');
  console.log('═════════════════════════════════════\n');
}

// Run if executed directly
if (require.main === module) {
  main().catch(error => {
    console.error('Fatal error:', error);
    process.exit(1);
  });
}

module.exports = {
  executeQuery,
  checkServerHealth,
  batchQueries,
  queryBuilderExample,
};

const QleverClient = require('./client');
const assert = require('assert');

let testsPassed = 0;
let testsFailed = 0;

function log(message) {
  console.log(`  ${message}`);
}

function success(testName) {
  console.log(`✓ ${testName}`);
  testsPassed++;
}

function failure(testName, error) {
  console.log(`✗ ${testName}`);
  console.log(`  Error: ${error.message}`);
  testsFailed++;
}

async function testClient() {
  const client = new QleverClient('http://localhost:3000');

  console.log('\n=== Testing QLever Client ===\n');

  // Test 1: Open Index
  try {
    log('Testing: Open Index');
    const openResult = await client.open('./test_index', null);
    assert(openResult.handle !== undefined, 'Handle not returned');
    assert(openResult.success === true, 'Success flag not set');
    success('Open Index');
  } catch (error) {
    failure('Open Index', error);
    return;
  }

  // Test 2: Basic Query
  try {
    log('Testing: Basic Query');
    const queryResult = await client.query('SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5');
    assert(queryResult.results !== undefined, 'Results not returned');
    assert(queryResult.head !== undefined, 'Head not returned');
    assert(Array.isArray(queryResult.head.vars), 'Variables not array');
    assert(Array.isArray(queryResult.results.bindings), 'Bindings not array');
    success('Basic Query');
  } catch (error) {
    failure('Basic Query', error);
  }

  // Test 3: Query with Timings
  try {
    log('Testing: Query with Timings');
    const timedResult = await client.query('SELECT ?s WHERE { ?s ?p ?o }', true);
    assert(timedResult.timings !== undefined, 'Timings not returned');
    assert(timedResult.timings.query_ms !== undefined, 'Query time missing');
    assert(timedResult.timings.planning_ms !== undefined, 'Planning time missing');
    assert(timedResult.timings.execution_ms !== undefined, 'Execution time missing');
    success('Query with Timings');
  } catch (error) {
    failure('Query with Timings', error);
  }

  // Test 4: Parse and Plan
  try {
    log('Testing: Parse and Plan');
    const planResult = await client.parseAndPlan('SELECT ?s ?o WHERE { ?s <http://example.org/prop> ?o }');
    assert(planResult.planId !== undefined, 'Plan ID not returned');
    assert(planResult.success === true, 'Success flag not set');
    success('Parse and Plan');
  } catch (error) {
    failure('Parse and Plan', error);
  }

  // Test 5: Pin Result
  try {
    log('Testing: Pin Result');
    await client.pinResult('test_cache', 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 100');
    success('Pin Result');
  } catch (error) {
    failure('Pin Result', error);
  }

  // Test 6: Erase Result
  try {
    log('Testing: Erase Result');
    await client.eraseResult('test_cache');
    success('Erase Result');
  } catch (error) {
    failure('Erase Result', error);
  }

  // Test 7: Write Materialized View
  try {
    log('Testing: Write Materialized View');
    await client.writeMaterializedView(
      'test_view',
      'SELECT ?s (COUNT(*) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?s'
    );
    success('Write Materialized View');
  } catch (error) {
    failure('Write Materialized View', error);
  }

  // Test 8: Load Materialized View
  try {
    log('Testing: Load Materialized View');
    await client.loadMaterializedView('test_view');
    success('Load Materialized View');
  } catch (error) {
    failure('Load Materialized View', error);
  }

  // Test 9: Text Search
  try {
    log('Testing: Text Search');
    const searchResult = await client.textSearch('example', 50);
    assert(searchResult.results !== undefined, 'Results not returned');
    assert(searchResult.head !== undefined, 'Head not returned');
    success('Text Search');
  } catch (error) {
    failure('Text Search', error);
  }

  // Test 10: Clear Cache
  try {
    log('Testing: Clear Cache');
    await client.clearCache();
    success('Clear Cache');
  } catch (error) {
    failure('Clear Cache', error);
  }

  // Test 11: Close Index
  try {
    log('Testing: Close Index');
    const closeResult = await client.close();
    assert(closeResult.closed === true || closeResult.success === true, 'Close not confirmed');
    success('Close Index');
  } catch (error) {
    failure('Close Index', error);
  }

  // Test 12: Error Handling - Invalid Handle
  try {
    log('Testing: Error Handling - Invalid Handle');
    const badClient = new QleverClient('http://localhost:3000');
    try {
      await badClient.query('SELECT ?s WHERE { ?s ?p ?o }');
      failure('Error Handling - Invalid Handle', new Error('Should have thrown error'));
    } catch (expectedError) {
      if (expectedError.message.includes('opened') || expectedError.message.includes('Index')) {
        success('Error Handling - Invalid Handle');
      } else {
        failure('Error Handling - Invalid Handle', expectedError);
      }
    }
  } catch (error) {
    failure('Error Handling - Invalid Handle', error);
  }

  // Test 13: Multiple Sessions
  try {
    log('Testing: Multiple Sessions');
    const client1 = new QleverClient('http://localhost:3000');
    const client2 = new QleverClient('http://localhost:3000');

    await client1.open('./test_index');
    await client2.open('./test_index_2', null);

    const result1 = await client1.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 1');
    const result2 = await client2.query('SELECT ?s WHERE { ?s ?p ?o } LIMIT 2');

    assert(result1.results.bindings.length <= 1, 'Client 1 limit not respected');
    assert(result2.results.bindings.length <= 2, 'Client 2 limit not respected');

    await client1.close();
    await client2.close();

    success('Multiple Sessions');
  } catch (error) {
    failure('Multiple Sessions', error);
  }

  console.log(`\n=== Test Results ===`);
  console.log(`Passed: ${testsPassed}`);
  console.log(`Failed: ${testsFailed}`);
  console.log(`Total:  ${testsPassed + testsFailed}`);

  if (testsFailed === 0) {
    console.log('\n✓ All tests passed!');
    process.exit(0);
  } else {
    console.log('\n✗ Some tests failed');
    process.exit(1);
  }
}

testClient().catch(error => {
  console.error('Fatal error:', error);
  process.exit(1);
});

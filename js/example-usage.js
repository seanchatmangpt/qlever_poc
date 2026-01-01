const QleverClient = require('./client');

async function main() {
  const client = new QleverClient('http://localhost:3000');

  try {
    console.log('Opening QLever index...');
    await client.open('./test_index');
    console.log('✓ Index opened');

    console.log('\n=== Basic Query ===');
    const results = await client.query('SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10');
    console.log(`Found ${results.results.bindings.length} results`);
    console.log(`Variables: ${results.head.vars.join(', ')}`);

    console.log('\n=== Query with Timings ===');
    const timed = await client.query(
      'SELECT ?s WHERE { ?s ?p ?o }',
      true
    );
    if (timed.timings) {
      console.log(`Query time: ${timed.timings.query_ms}ms`);
      console.log(`Planning: ${timed.timings.planning_ms}ms`);
      console.log(`Execution: ${timed.timings.execution_ms}ms`);
    }

    console.log('\n=== Query Planning ===');
    const plan = await client.parseAndPlan('SELECT ?s ?o WHERE { ?s <http://example.org/prop> ?o }');
    console.log(`Plan ID: ${plan.planId}`);

    console.log('\n=== Caching Results ===');
    await client.pinResult('subjects', 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1000');
    console.log('✓ Result cached as "subjects"');

    console.log('\n=== Materialized Views ===');
    await client.writeMaterializedView(
      'active_subjects',
      'SELECT ?s (COUNT(*) AS ?count) WHERE { ?s ?p ?o } GROUP BY ?s HAVING (?count > 10)'
    );
    console.log('✓ Materialized view created');

    console.log('\n=== Text Search ===');
    const searchResults = await client.textSearch('example', 50);
    console.log(`Search found ${searchResults.results.bindings.length} matches`);

    console.log('\n=== Clear Cache ===');
    await client.clearCache();
    console.log('✓ Cache cleared');

    console.log('\n=== Closing Index ===');
    await client.close();
    console.log('✓ Index closed');

  } catch (error) {
    console.error('Error:', error.message);
  }
}

main();

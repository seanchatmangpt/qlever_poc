/**
 * QLever WASM Advanced Examples
 * Demonstrates all SPARQL 1.1 features with the QueryBuilder API
 */

const qlever = require('../pkg/qlever_wasm.js');

// Initialize client
const client = new qlever.QleverClient('http://localhost:7023');

// ============================================================================
// 1. BASIC SELECT QUERIES
// ============================================================================

async function exampleBasicSelect() {
  console.log('\n=== Example 1: Basic SELECT Query ===');

  const query = `
    SELECT ?subject ?predicate ?object
    WHERE {
      ?subject ?predicate ?object
    }
    LIMIT 10
  `;

  try {
    const response = await client.query(query, 'json');
    console.log('Results:', response.data().results.bindings.length);
  } catch (error) {
    console.error('Error:', error);
  }
}

// ============================================================================
// 2. QUERY BUILDER EXAMPLES
// ============================================================================

async function exampleQueryBuilder() {
  console.log('\n=== Example 2: Query Builder - Simple ===');

  const builder = new qlever.QueryBuilder()
    .select('?name ?age')
    .where_clause('?person foaf:name ?name ; foaf:age ?age')
    .limit(10);

  console.log('Generated Query:');
  console.log(builder.build());

  const response = await client.query(builder.build(), 'json');
  console.log('Results:', response.data());
}

// ============================================================================
// 3. FILTER EXPRESSIONS
// ============================================================================

async function exampleFilter() {
  console.log('\n=== Example 3: FILTER Expressions ===');

  const builder = new qlever.QueryBuilder()
    .select('?name ?age')
    .where_clause('?person foaf:name ?name ; foaf:age ?age')
    .filter('?age > 18')
    .filter('?age < 65')
    .filter('STRLEN(?name) > 3')
    .order_by('?age')
    .limit(20);

  console.log('Generated Query:');
  console.log(builder.build());
}

// ============================================================================
// 4. OPTIONAL PATTERNS (LEFT JOIN)
// ============================================================================

async function exampleOptional() {
  console.log('\n=== Example 4: OPTIONAL Patterns ===');

  const builder = new qlever.QueryBuilder()
    .select('?person ?name ?email ?website')
    .where_clause('?person foaf:name ?name')
    .optional('?person foaf:mbox ?email')
    .optional('?person foaf:homepage ?website')
    .order_by('?name')
    .limit(50);

  console.log('Generated Query:');
  console.log(builder.build());
  console.log('\nNote: Results with NULL values for unbound optional variables');
}

// ============================================================================
// 5. BIND - VARIABLE BINDING
// ============================================================================

async function exampleBind() {
  console.log('\n=== Example 5: BIND Variable Binding ===');

  const builder = new qlever.QueryBuilder()
    .select('?firstName ?lastName ?fullName')
    .where_clause('?person foaf:givenName ?firstName ; foaf:familyName ?lastName')
    .bind('CONCAT(?firstName, " ", ?lastName)', '?fullName')
    .bind('STRLEN(?fullName)', '?nameLength')
    .limit(100);

  console.log('Generated Query:');
  console.log(builder.build());
  console.log('\nSupported BIND expressions:');
  console.log('- String: CONCAT(), STRLEN(), UCASE(), LCASE(), SUBSTR()');
  console.log('- Math: +, -, *, /, ABS(), ROUND(), CEIL(), FLOOR()');
  console.log('- Date: NOW(), YEAR(), MONTH(), DAY()');
  console.log('- Type: STR(), LANG(), DATATYPE()');
}

// ============================================================================
// 6. GROUP BY - AGGREGATION
// ============================================================================

async function exampleGroupBy() {
  console.log('\n=== Example 6: GROUP BY Aggregation ===');

  const builder = new qlever.QueryBuilder()
    .select('?author COUNT(?book) AS ?bookCount SUM(?rating) AS ?totalRating')
    .where_clause('?book dc:creator ?author ; schema:rating ?rating')
    .group_by('?author')
    .order_by_desc('?bookCount')
    .limit(20);

  console.log('Generated Query:');
  console.log(builder.build());
  console.log('\nAvailable aggregate functions:');
  console.log('- COUNT(?var) - Count non-NULL values');
  console.log('- SUM(?var) - Sum numeric values');
  console.log('- AVG(?var) - Average of numeric values');
  console.log('- MIN(?var) - Minimum value');
  console.log('- MAX(?var) - Maximum value');
}

// ============================================================================
// 7. ORDER BY - SORTING
// ============================================================================

async function exampleOrderBy() {
  console.log('\n=== Example 7: ORDER BY Sorting ===');

  const builder1 = new qlever.QueryBuilder()
    .select('?name ?age ?score')
    .where_clause('?person foaf:name ?name ; schema:age ?age ; schema:score ?score')
    .order_by('?name')
    .order_by('?age')
    .limit(100);

  console.log('Ascending Order:');
  console.log(builder1.build());

  const builder2 = new qlever.QueryBuilder()
    .select('?name ?score')
    .where_clause('?person foaf:name ?name ; schema:score ?score')
    .order_by_desc('?score')
    .limit(10);

  console.log('\nDescending Order (Top 10 Scores):');
  console.log(builder2.build());
}

// ============================================================================
// 8. PAGINATION - LIMIT AND OFFSET
// ============================================================================

async function examplePagination() {
  console.log('\n=== Example 8: Pagination with LIMIT and OFFSET ===');

  const pageSize = 50;
  const pageNumber = 2; // Get page 2
  const offset = (pageNumber - 1) * pageSize;

  const builder = new qlever.QueryBuilder()
    .select('?id ?name ?createdDate')
    .where_clause('?resource rdf:type foaf:Person ; foaf:name ?name ; schema:dateCreated ?createdDate')
    .order_by('?createdDate')
    .limit(pageSize)
    .offset(offset);

  console.log(`Fetching page ${pageNumber} (${offset}-${offset + pageSize - 1})`);
  console.log(builder.build());
}

// ============================================================================
// 9. DISTINCT - REMOVE DUPLICATES
// ============================================================================

async function exampleDistinct() {
  console.log('\n=== Example 9: DISTINCT Results ===');

  const builder = new qlever.QueryBuilder()
    .select('?type')
    .distinct()
    .where_clause('?resource rdf:type ?type')
    .order_by('?type')
    .limit(100);

  console.log('Generated Query (with DISTINCT):');
  console.log(builder.build());
}

// ============================================================================
// 10. VALUES - INLINE DATA
// ============================================================================

async function exampleValues() {
  console.log('\n=== Example 10: VALUES Inline Data ===');

  const builder = new qlever.QueryBuilder()
    .select('?status ?count')
    .where_clause('?resource schema:status ?status')
    .values('VALUES (?status) { ("active") ("pending") ("inactive") }')
    .group_by('?status')
    .bind('COUNT(?resource)', '?count');

  console.log('Generated Query:');
  console.log(builder.build());

  const builder2 = new qlever.QueryBuilder()
    .select('?personName ?age')
    .where_clause('?person foaf:name ?personName ; foaf:age ?age')
    .values('VALUES (?age) { (25) (30) (35) (40) }')
    .order_by('?age');

  console.log('\nFiltering by multiple values:');
  console.log(builder2.build());
}

// ============================================================================
// 11. CONSTRUCT QUERY
// ============================================================================

async function exampleConstruct() {
  console.log('\n=== Example 11: CONSTRUCT Query ===');

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

  console.log('Generated CONSTRUCT Query:');
  console.log(builder.build());
  console.log('\nCONSTRUCT creates new RDF data matching the template');
}

// ============================================================================
// 12. DESCRIBE QUERY
// ============================================================================

async function exampleDescribe() {
  console.log('\n=== Example 12: DESCRIBE Query ===');

  const builder = new qlever.QueryBuilder()
    .describe_query()
    .describe('?person');

  console.log('Generated DESCRIBE Query:');
  console.log(builder.build());
  console.log('\nDESCRIBE returns all properties of resources');

  // Or describe a specific resource URI
  const specificQuery = 'DESCRIBE <http://dbpedia.org/resource/Albert_Einstein>';
  console.log('\nDescribe specific resource:');
  console.log(specificQuery);
}

// ============================================================================
// 13. ASK QUERY - BOOLEAN
// ============================================================================

async function exampleAsk() {
  console.log('\n=== Example 13: ASK Query (Boolean) ===');

  const builder = new qlever.QueryBuilder()
    .ask_query()
    .where_clause('?person foaf:name "Alice" ; foaf:age ?age')
    .filter('?age > 21');

  console.log('Generated ASK Query:');
  console.log(builder.build());
  console.log('\nASK returns true/false instead of results');
}

// ============================================================================
// 14. COMPLEX GRAPH PATTERNS
// ============================================================================

async function exampleComplexPatterns() {
  console.log('\n=== Example 14: Complex Graph Patterns ===');

  const builder = new qlever.QueryBuilder()
    .select('?person ?friend ?friendName ?friendAge')
    .where_clause('?person foaf:knows ?friend')
    .where_clause('?friend foaf:name ?friendName')
    .where_clause('?friend foaf:age ?friendAge')
    .optional('?person foaf:name ?personName')
    .filter('?friendAge > 21')
    .filter('CONTAINS(?friendName, "John")')
    .order_by_desc('?friendAge')
    .limit(100);

  console.log('Generated Query:');
  console.log(builder.build());
}

// ============================================================================
// 15. MULTIPLE FILTERS
// ============================================================================

async function exampleMultipleFilters() {
  console.log('\n=== Example 15: Multiple FILTER Conditions ===');

  const builder = new qlever.QueryBuilder()
    .select('?product ?name ?price ?rating')
    .where_clause('?product schema:name ?name ; schema:price ?price ; schema:rating ?rating')
    .filter('?price > 100')
    .filter('?price < 500')
    .filter('?rating > 4.0')
    .filter('CONTAINS(?name, "phone")')
    .order_by_desc('?rating')
    .limit(20);

  console.log('Generated Query:');
  console.log(builder.build());
}

// ============================================================================
// 16. FROM - NAMED GRAPHS
// ============================================================================

async function exampleFromNamedGraphs() {
  console.log('\n=== Example 16: FROM Named Graphs ===');

  const builder = new qlever.QueryBuilder()
    .select('?subject ?predicate ?object')
    .from('http://example.org/graph1')
    .from('http://example.org/graph2')
    .where_clause('?subject ?predicate ?object')
    .limit(50);

  console.log('Generated Query (multiple FROM clauses):');
  console.log(builder.build());
}

// ============================================================================
// 17. SERVER HEALTH CHECK
// ============================================================================

async function exampleHealthCheck() {
  console.log('\n=== Example 17: Server Health Check ===');

  try {
    const isHealthy = await client.ping();
    console.log(`QLever server status: ${isHealthy ? '✓ Healthy' : '✗ Unhealthy'}`);
    console.log(`Endpoint: ${client.endpoint()}`);
  } catch (error) {
    console.error('Health check failed:', error.message);
  }
}

// ============================================================================
// 18. QUERY WITH CUSTOM HEADERS
// ============================================================================

async function exampleCustomHeaders() {
  console.log('\n=== Example 18: Query with Custom Headers ===');

  const query = 'SELECT * WHERE { ?s ?p ?o } LIMIT 10';

  const headers = {
    'Authorization': 'Bearer mytoken123',
    'User-Agent': 'QLever-WASM/1.0',
    'Accept-Language': 'en-US'
  };

  console.log('Sending request with custom headers:');
  console.log(JSON.stringify(headers, null, 2));

  try {
    const response = await client.query_with_headers(query, 'json', headers);
    console.log('Response received');
  } catch (error) {
    console.error('Request failed:', error.message);
  }
}

// ============================================================================
// 19. RESULT FORMAT EXAMPLES
// ============================================================================

async function exampleResultFormats() {
  console.log('\n=== Example 19: Different Result Formats ===');

  const query = 'SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5';

  const formats = ['json', 'xml', 'csv', 'turtle', 'ntriples'];

  for (const format of formats) {
    try {
      const response = await client.query(query, format);
      console.log(`\n${format.toUpperCase()} Format:`);
      console.log(`Response format: ${response.format()}`);
      console.log(`Execution time: ${response.execution_time_ms()}ms`);
    } catch (error) {
      console.log(`${format} format not available: ${error.message}`);
    }
  }
}

// ============================================================================
// 20. BATCH QUERIES
// ============================================================================

async function exampleBatchQueries() {
  console.log('\n=== Example 20: Batch Query Processing ===');

  const queries = [
    'SELECT COUNT(*) AS ?count WHERE { ?s ?p ?o }',
    'SELECT DISTINCT ?type WHERE { ?s rdf:type ?type } LIMIT 10',
    'SELECT ?p COUNT(*) AS ?count WHERE { ?s ?p ?o } GROUP BY ?p ORDER BY DESC(?count) LIMIT 5'
  ];

  console.log(`Processing ${queries.length} queries...`);

  for (let i = 0; i < queries.length; i++) {
    try {
      console.log(`\nQuery ${i + 1}:`);
      const response = await client.query(queries[i], 'json');
      console.log(`✓ Success - ${response.execution_time_ms()}ms`);
    } catch (error) {
      console.log(`✗ Failed - ${error.message}`);
    }
  }
}

// ============================================================================
// RUN ALL EXAMPLES
// ============================================================================

async function runAllExamples() {
  console.log('╔════════════════════════════════════════════════════════════════╗');
  console.log('║         QLever WASM Advanced Examples                           ║');
  console.log('║         Full SPARQL 1.1 Feature Demonstration                  ║');
  console.log('╚════════════════════════════════════════════════════════════════╝');

  await exampleBasicSelect();
  await exampleQueryBuilder();
  await exampleFilter();
  await exampleOptional();
  await exampleBind();
  await exampleGroupBy();
  await exampleOrderBy();
  await examplePagination();
  await exampleDistinct();
  await exampleValues();
  await exampleConstruct();
  await exampleDescribe();
  await exampleAsk();
  await exampleComplexPatterns();
  await exampleMultipleFilters();
  await exampleFromNamedGraphs();
  await exampleHealthCheck();
  await exampleCustomHeaders();
  await exampleResultFormats();
  await exampleBatchQueries();

  console.log('\n╔════════════════════════════════════════════════════════════════╗');
  console.log('║                    Examples Complete!                           ║');
  console.log('╚════════════════════════════════════════════════════════════════╝');
}

// Run examples if executed directly
if (require.main === module) {
  runAllExamples().catch(console.error);
}

module.exports = {
  exampleBasicSelect,
  exampleQueryBuilder,
  exampleFilter,
  exampleOptional,
  exampleBind,
  exampleGroupBy,
  exampleOrderBy,
  examplePagination,
  exampleDistinct,
  exampleValues,
  exampleConstruct,
  exampleDescribe,
  exampleAsk,
  exampleComplexPatterns,
  exampleMultipleFilters,
  exampleFromNamedGraphs,
  exampleHealthCheck,
  exampleCustomHeaders,
  exampleResultFormats,
  exampleBatchQueries,
};

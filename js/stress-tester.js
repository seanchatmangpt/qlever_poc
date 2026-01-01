const axios = require('axios');
const { ContractEnforcer, ContractViolation } = require('./contract-enforcer');

class StressTester {
  constructor() {
    this.enforcer = new ContractEnforcer();
    this.http = axios.create({ baseURL: 'http://localhost:3000' });
    this.results = {
      edgeCases: [],
      boundaryViolations: [],
      stressTests: [],
      failures: []
    };
  }

  async testEdgeCases() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║              EDGE CASE TESTING                          ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const tests = [
      {
        name: 'Empty SPARQL query',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 1, sparql: '' });
            return { pass: false, reason: 'Should reject empty query' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Malformed SPARQL',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 1, sparql: 'SELECT * FROM ???' });
            return { pass: false, reason: 'Should reject malformed SPARQL' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Invalid handle (0)',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 0, sparql: 'SELECT ?s WHERE { ?s ?p ?o }' });
            return { pass: false, reason: 'Should reject handle 0' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Invalid handle (negative)',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: -1, sparql: 'SELECT ?s WHERE { ?s ?p ?o }' });
            return { pass: false, reason: 'Should reject negative handle' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Non-existent handle',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 999999, sparql: 'SELECT ?s WHERE { ?s ?p ?o }' });
            return { pass: false, reason: 'Should reject non-existent handle' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Extremely long SPARQL',
        fn: async () => {
          try {
            const longQuery = 'SELECT ?s WHERE { ' + '?s ?p ?o . '.repeat(1000) + '}';
            await this.http.post('/api/query', { handle: 1, sparql: longQuery });
            return { pass: true, reason: 'Handled long query' };
          } catch (e) {
            return { pass: false, reason: `Rejected: ${e.message}` };
          }
        }
      },
      {
        name: 'Null values in request',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: null, sparql: null });
            return { pass: false, reason: 'Should reject null values' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Missing required fields',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 1 });
            return { pass: false, reason: 'Should reject missing sparql' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Negative limit',
        fn: async () => {
          try {
            await this.http.post('/api/query', { handle: 1, sparql: 'SELECT ?s WHERE { ?s ?p ?o }', limit: -1 });
            return { pass: false, reason: 'Should reject negative limit' };
          } catch (e) {
            return { pass: true, reason: 'Correctly rejected' };
          }
        }
      },
      {
        name: 'Zero limit',
        fn: async () => {
          try {
            const res = await this.http.post('/api/query', { handle: 1, sparql: 'SELECT ?s WHERE { ?s ?p ?o }', limit: 0 });
            return { pass: res.data.results.bindings.length === 0, reason: 'Should return 0 results' };
          } catch (e) {
            return { pass: false, reason: `Error: ${e.message}` };
          }
        }
      }
    ];

    for (const test of tests) {
      try {
        const result = await test.fn();
        console.log(`   ${result.pass ? '✓' : '✗'} ${test.name}`);
        console.log(`     ${result.reason}`);
        this.results.edgeCases.push({ test: test.name, ...result });
      } catch (e) {
        console.log(`   ✗ ${test.name}`);
        console.log(`     Exception: ${e.message}`);
        this.results.failures.push({ test: test.name, error: e.message });
      }
    }
  }

  async testBoundaryViolations() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║           BOUNDARY VIOLATION TESTING                    ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    try {
      const res = await this.http.post('/api/open', { indexPath: './index' });
      const handle = res.data.handle;

      this.enforcer.assert('HTTP.Handle.Validity', handle);
      console.log(`   ✓ HTTP.Handle.Validity: valid handle ${handle}`);

      const queryRes = await this.http.post('/api/query', {
        handle,
        sparql: 'SELECT ?s WHERE { ?s ?p ?o }',
        timings: 1
      });

      try {
        this.enforcer.assertStrict('HTTP.Response.Structure', queryRes.data);
        console.log('   ✓ HTTP.Response.Structure: valid');
      } catch (e) {
        console.log(`   ✗ HTTP.Response.Structure: ${e.violation}`);
      }

      try {
        this.enforcer.assertStrict('HTTP.Timings.Structure', queryRes.data);
        console.log('   ✓ HTTP.Timings.Structure: valid');
      } catch (e) {
        console.log(`   ✗ HTTP.Timings.Structure: ${e.violation}`);
      }

      for (const binding of queryRes.data.results.bindings) {
        try {
          this.enforcer.assertStrict('Result.Binding.Consistency', binding, queryRes.data.head.vars);
        } catch (e) {
          console.log(`   ✗ Result.Binding.Consistency: ${e.violation}`);
          this.results.boundaryViolations.push(e);
        }
      }

      await this.http.post('/api/close', { handle });

      this.enforcer.assert('Index.Handle.Lifecycle', null, 'closed');
      console.log('   ✓ Index.Handle.Lifecycle: handle properly closed');

    } catch (e) {
      console.log(`   ✗ Boundary test failed: ${e.message}`);
      this.results.failures.push({ test: 'boundary-violation', error: e.message });
    }
  }

  async testStress() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║               STRESS TESTING                            ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    try {
      const res = await this.http.post('/api/open', { indexPath: './index' });
      const handle = res.data.handle;

      const stressTests = [
        { name: 'Rapid sequential queries', count: 10 },
        { name: 'Large result set', limit: 10000 },
        { name: 'Plan reuse cycle', count: 5 }
      ];

      for (const test of stressTests) {
        const startTime = Date.now();
        let successCount = 0;
        let failureCount = 0;

        try {
          if (test.count) {
            for (let i = 0; i < test.count; i++) {
              try {
                await this.http.post('/api/query', {
                  handle,
                  sparql: `SELECT ?s WHERE { ?s ?p ?o } LIMIT ${10 + i}`
                });
                successCount++;
              } catch {
                failureCount++;
              }
            }
          } else if (test.limit) {
            await this.http.post('/api/query', {
              handle,
              sparql: `SELECT ?s WHERE { ?s ?p ?o } LIMIT ${test.limit}`
            });
            successCount = 1;
          }

          const elapsed = Date.now() - startTime;
          console.log(`   ✓ ${test.name}`);
          console.log(`     ${successCount} success, ${failureCount} failed, ${elapsed}ms elapsed`);
          this.results.stressTests.push({
            test: test.name,
            successCount,
            failureCount,
            elapsed,
            passed: failureCount === 0
          });
        } catch (e) {
          const elapsed = Date.now() - startTime;
          console.log(`   ✗ ${test.name}: ${e.message}`);
          this.results.stressTests.push({
            test: test.name,
            passed: false,
            error: e.message,
            elapsed
          });
        }
      }

      await this.http.post('/api/close', { handle });

    } catch (e) {
      console.log(`   ✗ Stress test failed: ${e.message}`);
      this.results.failures.push({ test: 'stress', error: e.message });
    }
  }

  async runFullTests() {
    try {
      await this.testEdgeCases();
      await this.testBoundaryViolations();
      await this.testStress();

      console.log('\n╔════════════════════════════════════════════════════════╗');
      console.log('║                TEST SUMMARY                            ║');
      console.log('╚════════════════════════════════════════════════════════╝\n');

      const edgePassed = this.results.edgeCases.filter(e => e.pass).length;
      const stressPassed = this.results.stressTests.filter(s => s.passed).length;
      const contractViolations = this.enforcer.getViolations().length;

      console.log(`   Edge cases: ${edgePassed}/${this.results.edgeCases.length} passed`);
      console.log(`   Stress tests: ${stressPassed}/${this.results.stressTests.length} passed`);
      console.log(`   Contract violations: ${contractViolations}`);
      console.log(`   Uncaught failures: ${this.results.failures.length}\n`);

      if (contractViolations > 0) {
        this.enforcer.printReport();
      }

    } catch (e) {
      console.error('Test runner error:', e);
    }
  }
}

const tester = new StressTester();
tester.runFullTests().catch(console.error);

module.exports = StressTester;

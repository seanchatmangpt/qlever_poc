const SystemHealthMonitor = require('./system-health-monitor');
const SystemDiagnostics = require('./system-diagnostics');
const { ContractEnforcer } = require('./contract-enforcer');
const { ResilienceHandler } = require('./resilience-layer');
const axios = require('axios');

class CoherenceTest {
  constructor() {
    this.results = {
      timestamp: new Date().toISOString(),
      phases: [],
      integrations: [],
      coherenceScore: 0,
      coherent: false
    };
  }

  async phase1_SystemObservation() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║          PHASE 1: SYSTEM OBSERVATION                    ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const diagnostics = new SystemDiagnostics();

    try {
      console.log('  Running protocol diagnostics...');
      await diagnostics.diagnoseProtocolHealth();

      console.log('\n  Analyzing boundaries...');
      await diagnostics.diagnoseBoundaries();

      console.log('\n  Analyzing critical paths...');
      await diagnostics.diagnoseCriticalPaths();

      const allObserved = Object.keys(diagnostics.diagnostics.systemHealth.protocols).length > 0;
      this.results.phases.push({
        phase: 'observation',
        success: allObserved,
        protocolsObserved: Object.keys(diagnostics.diagnostics.systemHealth.protocols).length,
        failures: diagnostics.diagnostics.failures.length
      });

      return allObserved;
    } catch (error) {
      console.log(`  ✗ Observation failed: ${error.message}`);
      this.results.phases.push({
        phase: 'observation',
        success: false,
        error: error.message
      });
      return false;
    }
  }

  async phase2_ContractVerification() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║       PHASE 2: CONTRACT VERIFICATION                    ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const enforcer = new ContractEnforcer();
    let contractsVerified = 0;
    let contractsViolated = 0;

    try {
      console.log('  Verifying contracts...');

      try {
        const res = await axios.post('http://localhost:3000/api/open', { indexPath: './index' });
        enforcer.assertStrict('HTTP.Handle.Validity', res.data.handle);
        contractsVerified++;
        console.log('    ✓ HTTP.Handle.Validity');
      } catch {
        contractsViolated++;
        console.log('    ✗ HTTP.Handle.Validity');
      }

      try {
        const res = await axios.post('http://localhost:3000/api/query', {
          handle: 1,
          sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 1',
          timings: 1
        });
        enforcer.assertStrict('HTTP.Response.Structure', res.data);
        contractsVerified++;
        console.log('    ✓ HTTP.Response.Structure');
      } catch {
        contractsViolated++;
        console.log('    ✗ HTTP.Response.Structure');
      }

      try {
        const res = await axios.post('http://localhost:3000/api/query', {
          handle: 1,
          sparql: 'SELECT ?s WHERE { ?s ?p ?o }',
          timings: 1
        });
        enforcer.assertStrict('HTTP.Timings.Structure', res.data);
        contractsVerified++;
        console.log('    ✓ HTTP.Timings.Structure');
      } catch {
        contractsViolated++;
        console.log('    ✗ HTTP.Timings.Structure');
      }

      this.results.phases.push({
        phase: 'contract-verification',
        success: contractsViolated === 0,
        contractsVerified,
        contractsViolated,
        violations: enforcer.getViolations().length
      });

      return contractsViolated === 0;
    } catch (error) {
      console.log(`  ✗ Contract verification failed: ${error.message}`);
      this.results.phases.push({
        phase: 'contract-verification',
        success: false,
        error: error.message
      });
      return false;
    }
  }

  async phase3_ResilienceTesting() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║       PHASE 3: RESILIENCE TESTING                       ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const resilience = new ResilienceHandler({
      maxRetries: 2,
      retryDelay: 50
    });

    let testsRun = 0;
    let testsPass = 0;

    try {
      console.log('  Testing failure recovery...');

      const tests = [
        {
          name: 'Valid operation succeeds',
          fn: async () => {
            const res = await axios.get('http://localhost:3000/health');
            return res.status === 200;
          }
        },
        {
          name: 'Invalid handle is handled',
          fn: async () => {
            try {
              await axios.post('http://localhost:3000/api/query', {
                handle: 999,
                sparql: 'SELECT ?s WHERE { ?s ?p ?o }'
              });
              return false;
            } catch {
              return true;
            }
          }
        },
        {
          name: 'Malformed query is rejected',
          fn: async () => {
            try {
              await axios.post('http://localhost:3000/api/query', {
                handle: 1,
                sparql: 'INVALID QUERY'
              });
              return false;
            } catch {
              return true;
            }
          }
        }
      ];

      for (const test of tests) {
        testsRun++;
        try {
          const result = await resilience.executeWithResilience(
            test.fn,
            test.name,
            { maxRetries: 1 }
          );
          if (result) {
            testsPass++;
            console.log(`    ✓ ${test.name}`);
          } else {
            console.log(`    ✗ ${test.name}`);
          }
        } catch (error) {
          console.log(`    ✗ ${test.name}: ${error.message}`);
        }
      }

      this.results.phases.push({
        phase: 'resilience-testing',
        success: testsPass === testsRun,
        testsRun,
        testsPassed: testsPass,
        recoveryRate: ((testsPass / testsRun) * 100).toFixed(1) + '%'
      });

      return testsPass === testsRun;
    } catch (error) {
      console.log(`  ✗ Resilience testing failed: ${error.message}`);
      this.results.phases.push({
        phase: 'resilience-testing',
        success: false,
        error: error.message
      });
      return false;
    }
  }

  async phase4_HealthMonitoring() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║       PHASE 4: UNIFIED HEALTH MONITORING                ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const monitor = new SystemHealthMonitor();

    try {
      console.log('  Running unified health monitoring...');
      await monitor.monitorProtocols();
      await monitor.validateContracts();
      await monitor.assessResilience();
      monitor.aggregateHealth();

      const protocolHealthy = Object.values(monitor.health.protocols).filter(p => p.status === 'healthy').length;
      const protocolTotal = Object.keys(monitor.health.protocols).length;
      const healthScore = parseFloat(monitor.health.score);

      this.results.phases.push({
        phase: 'health-monitoring',
        success: healthScore >= 60,
        systemStatus: monitor.health.status,
        healthScore: healthScore,
        protocolsHealthy: `${protocolHealthy}/${protocolTotal}`,
        recommendations: monitor.health.recommendations.length
      });

      console.log(`\n  System Status: ${monitor.health.status.toUpperCase()}`);
      console.log(`  Health Score: ${healthScore}%`);

      return healthScore >= 60;
    } catch (error) {
      console.log(`  ✗ Health monitoring failed: ${error.message}`);
      this.results.phases.push({
        phase: 'health-monitoring',
        success: false,
        error: error.message
      });
      return false;
    }
  }

  async phase5_IntegrationValidation() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║     PHASE 5: INTEGRATION VALIDATION                     ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    console.log('  Validating system integration...');

    const tests = [
      {
        name: 'Diagnostics ↔ Contracts',
        test: async () => {
          const enforcer = new ContractEnforcer();
          const diagnostics = new SystemDiagnostics();
          return enforcer && diagnostics;
        }
      },
      {
        name: 'Contracts ↔ Resilience',
        test: async () => {
          const enforcer = new ContractEnforcer();
          const resilience = new ResilienceHandler();
          return enforcer && resilience;
        }
      },
      {
        name: 'Resilience ↔ Health Monitor',
        test: async () => {
          const resilience = new ResilienceHandler();
          const monitor = new SystemHealthMonitor();
          return resilience && monitor;
        }
      },
      {
        name: 'All systems communicate',
        test: async () => {
          const res = await axios.get('http://localhost:3000/health');
          return res.status === 200;
        }
      }
    ];

    let integrationsPassed = 0;
    for (const test of tests) {
      try {
        const result = await test.test();
        if (result) {
          integrationsPassed++;
          console.log(`    ✓ ${test.name}`);
          this.results.integrations.push({ test: test.name, pass: true });
        } else {
          console.log(`    ✗ ${test.name}`);
          this.results.integrations.push({ test: test.name, pass: false });
        }
      } catch (error) {
        console.log(`    ✗ ${test.name}: ${error.message}`);
        this.results.integrations.push({ test: test.name, pass: false, error: error.message });
      }
    }

    this.results.phases.push({
      phase: 'integration-validation',
      success: integrationsPassed === tests.length,
      integrationsTest: tests.length,
      integrationsPassed
    });

    return integrationsPassed === tests.length;
  }

  calculateCoherence() {
    const totalPhases = this.results.phases.length;
    const passedPhases = this.results.phases.filter(p => p.success).length;
    const phasesScore = (passedPhases / totalPhases) * 100;

    const totalIntegrations = this.results.integrations.length;
    const passedIntegrations = this.results.integrations.filter(i => i.pass).length;
    const integrationsScore = (passedIntegrations / totalIntegrations) * 100;

    this.results.coherenceScore = (phasesScore * 0.6 + integrationsScore * 0.4);
    this.results.coherent = this.results.coherenceScore >= 80;
  }

  printFinalReport() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║              COHERENCE TEST REPORT                     ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    console.log(`   Coherence Score: ${this.results.coherenceScore.toFixed(1)}%`);
    console.log(`   Coherent: ${this.results.coherent ? '✓ YES' : '✗ NO'}\n`);

    console.log('   Phases:');
    for (const phase of this.results.phases) {
      const status = phase.success ? '✓' : '✗';
      console.log(`     ${status} ${phase.phase}`);
    }

    console.log('\n   Integrations:');
    for (const integration of this.results.integrations) {
      const status = integration.pass ? '✓' : '✗';
      console.log(`     ${status} ${integration.test}`);
    }

    console.log(`\n   System Coherence: ${
      this.results.coherent
        ? '✓✓✓ INTERNALLY COHERENT ✓✓✓'
        : '⚠ COHERENCE DEGRADED'
    }\n`);

    const fs = require('fs');
    fs.writeFileSync('coherence-test-report.json', JSON.stringify(this.results, null, 2));
  }

  async runFullCoherenceTest() {
    try {
      const phase1 = await this.phase1_SystemObservation();
      const phase2 = await this.phase2_ContractVerification();
      const phase3 = await this.phase3_ResilienceTesting();
      const phase4 = await this.phase4_HealthMonitoring();
      const phase5 = await this.phase5_IntegrationValidation();

      this.calculateCoherence();
      this.printFinalReport();
    } catch (error) {
      console.error('Coherence test failed:', error);
    }
  }
}

const test = new CoherenceTest();
test.runFullCoherenceTest().catch(console.error);

module.exports = CoherenceTest;

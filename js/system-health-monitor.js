const ProtocolValidator = require('./protocol-validator');
const StressTester = require('./stress-tester');
const SystemDiagnostics = require('./system-diagnostics');
const { ResilienceHandler } = require('./resilience-layer');
const { ContractEnforcer } = require('./contract-enforcer');
const axios = require('axios');

class SystemHealthMonitor {
  constructor() {
    this.enforcer = new ContractEnforcer();
    this.resilience = new ResilienceHandler({
      maxRetries: 3,
      retryDelay: 100
    });
    this.http = axios.create({
      baseURL: 'http://localhost:3000',
      timeout: 5000
    });
    this.health = {
      timestamp: new Date().toISOString(),
      status: 'unknown',
      protocols: {},
      contracts: {},
      resilience: {},
      recommendations: []
    };

    this.httpBreaker = this.resilience.createCircuitBreaker('HTTP', {
      failureThreshold: 5,
      resetTimeout: 30000
    });
    this.grpcBreaker = this.resilience.createCircuitBreaker('gRPC', {
      failureThreshold: 5,
      resetTimeout: 30000
    });
  }

  async monitorProtocols() {
    console.log('\n  Monitoring Protocols...');

    const protocols = [
      { name: 'HTTP', breaker: this.httpBreaker, test: () => axios.get('http://localhost:3000/health') },
      { name: 'WebSocket', test: () => axios.get('http://localhost:3002/health') },
      { name: 'GraphQL', test: () => axios.get('http://localhost:3003/health') }
    ];

    for (const proto of protocols) {
      try {
        const operation = () => proto.breaker ? proto.breaker.execute(proto.test) : proto.test();
        const start = Date.now();
        await operation();
        this.health.protocols[proto.name] = {
          status: 'healthy',
          latency: Date.now() - start,
          circuitBreaker: proto.breaker ? proto.breaker.getState() : 'n/a'
        };
      } catch (error) {
        this.health.protocols[proto.name] = {
          status: 'unhealthy',
          error: error.message,
          circuitBreaker: proto.breaker ? proto.breaker.getState() : 'n/a'
        };
      }
    }
  }

  async validateContracts() {
    console.log('  Validating Contracts...');

    try {
      const openRes = await this.resilience.executeWithResilience(
        () => this.http.post('/api/open', { indexPath: './index' }),
        'open-index'
      );

      const handle = openRes.data.handle;
      this.enforcer.assert('HTTP.Handle.Validity', handle);

      const queryRes = await this.resilience.executeWithResilience(
        () => this.http.post('/api/query', {
          handle,
          sparql: 'SELECT ?s WHERE { ?s ?p ?o } LIMIT 5',
          timings: 1
        }),
        'query-execution'
      );

      const contractTests = [
        'HTTP.Response.Structure',
        'HTTP.Timings.Structure',
        'Index.Handle.Lifecycle'
      ];

      for (const contract of contractTests) {
        const satisfied = this.enforcer.assert(contract, queryRes.data);
        this.health.contracts[contract] = { satisfied };
      }

      await this.http.post('/api/close', { handle });
    } catch (error) {
      console.log(`    Contract validation failed: ${error.message}`);
      this.health.contracts['validation-error'] = { error: error.message };
    }
  }

  async assessResilience() {
    console.log('  Assessing Resilience...');

    const testOperations = [
      {
        name: 'Connection Recovery',
        test: async () => {
          for (let i = 0; i < 3; i++) {
            try {
              await axios.get('http://localhost:3000/health', { timeout: 1000 });
              return true;
            } catch {
              await new Promise(r => setTimeout(r, 100 * Math.pow(2, i)));
            }
          }
          return false;
        }
      },
      {
        name: 'Handle Management',
        test: async () => {
          const res = await this.http.post('/api/open', { indexPath: './index' });
          await this.http.post('/api/close', { handle: res.data.handle });
          return true;
        }
      },
      {
        name: 'Error Propagation',
        test: async () => {
          try {
            await this.http.post('/api/query', { handle: 999, sparql: 'SELECT ?s WHERE { ?s ?p ?o }' });
            return false;
          } catch {
            return true;
          }
        }
      }
    ];

    for (const test of testOperations) {
      try {
        const result = await this.resilience.executeWithResilience(
          test.test,
          `resilience-${test.name}`,
          { maxRetries: 2 }
        );
        this.health.resilience[test.name] = { passed: result };
      } catch (error) {
        this.health.resilience[test.name] = { passed: false, error: error.message };
      }
    }
  }

  aggregateHealth() {
    const protocolHealthy = Object.values(this.health.protocols).filter(p => p.status === 'healthy').length;
    const protocolTotal = Object.keys(this.health.protocols).length;

    const contractsSatisfied = Object.values(this.health.contracts).filter(c => c.satisfied).length;
    const contractsTotal = Object.keys(this.health.contracts).length;

    const resilienceTests = Object.values(this.health.resilience);
    const resilientTests = resilienceTests.filter(r => r.passed).length;

    const healthScore = (
      (protocolHealthy / protocolTotal) * 0.4 +
      (contractsSatisfied / contractsTotal) * 0.3 +
      (resilientTests / resilienceTests.length) * 0.3
    );

    if (healthScore >= 0.95) {
      this.health.status = 'excellent';
    } else if (healthScore >= 0.80) {
      this.health.status = 'good';
    } else if (healthScore >= 0.60) {
      this.health.status = 'degraded';
    } else {
      this.health.status = 'critical';
    }

    this.health.score = (healthScore * 100).toFixed(1);

    this.generateRecommendations(protocolHealthy, contractsSatisfied, resilienceTests);
  }

  generateRecommendations(protocolHealthy, contractsSatisfied, resilienceTests) {
    if (protocolHealthy < Object.keys(this.health.protocols).length) {
      const unhealthy = Object.entries(this.health.protocols)
        .filter(([_, p]) => p.status !== 'healthy')
        .map(([name]) => name);
      this.health.recommendations.push(`Restart unhealthy services: ${unhealthy.join(', ')}`);
    }

    if (contractsSatisfied < Object.keys(this.health.contracts).length) {
      this.health.recommendations.push('Contract violations detected - check data integrity');
    }

    const failedResilience = resilienceTests.filter(r => !r.passed).length;
    if (failedResilience > 0) {
      this.health.recommendations.push(`${failedResilience} resilience tests failed - review error handling`);
    }

    if (Object.values(this.health.protocols).some(p => p.circuitBreaker === 'open')) {
      this.health.recommendations.push('Circuit breakers are OPEN - services may be in recovery');
    }
  }

  printReport() {
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║            SYSTEM HEALTH MONITORING REPORT              ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    const statusIcon = {
      'excellent': '✓✓✓',
      'good': '✓✓',
      'degraded': '⚠',
      'critical': '✗'
    };

    console.log(`   System Status: ${statusIcon[this.health.status]} ${this.health.status.toUpperCase()}`);
    console.log(`   Health Score: ${this.health.score}%\n`);

    console.log('   Protocols:');
    for (const [name, proto] of Object.entries(this.health.protocols)) {
      const icon = proto.status === 'healthy' ? '✓' : '✗';
      const info = proto.latency ? `${proto.latency}ms` : proto.error;
      console.log(`     ${icon} ${name.padEnd(15)} ${info}`);
    }

    console.log('\n   Contracts:');
    let satisfiedCount = 0;
    for (const [contract, data] of Object.entries(this.health.contracts)) {
      if (data.satisfied) {
        satisfiedCount++;
        console.log(`     ✓ ${contract}`);
      } else {
        console.log(`     ✗ ${contract}: ${data.error || 'unsatisfied'}`);
      }
    }

    console.log('\n   Resilience:');
    for (const [test, result] of Object.entries(this.health.resilience)) {
      const icon = result.passed ? '✓' : '✗';
      console.log(`     ${icon} ${test}: ${result.passed ? 'recovered' : result.error}`);
    }

    if (this.health.recommendations.length > 0) {
      console.log('\n   Recommendations:');
      for (const rec of this.health.recommendations) {
        console.log(`     → ${rec}`);
      }
    }

    console.log(`\n   Timestamp: ${this.health.timestamp}\n`);

    const fs = require('fs');
    fs.writeFileSync('health-report.json', JSON.stringify(this.health, null, 2));
  }

  async runMonitoring() {
    try {
      console.log('\n╔════════════════════════════════════════════════════════╗');
      console.log('║         SYSTEM HEALTH MONITORING STARTED               ║');
      console.log('╚════════════════════════════════════════════════════════╝');

      await this.monitorProtocols();
      await this.validateContracts();
      await this.assessResilience();
      this.aggregateHealth();
      this.printReport();

      this.resilience.printReport();
      this.enforcer.printReport();

    } catch (error) {
      console.error('Monitoring error:', error);
    }
  }
}

module.exports = { SystemHealthMonitor };

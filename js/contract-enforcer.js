class ContractViolation extends Error {
  constructor(contract, violation, context = {}) {
    super(`Contract violation: ${contract}`);
    this.contract = contract;
    this.violation = violation;
    this.context = context;
    this.timestamp = Date.now();
  }
}

class ContractEnforcer {
  constructor() {
    this.violations = [];
    this.contracts = {
      'HTTP.Response.Structure': (data) => {
        if (!data || typeof data !== 'object') throw new ContractViolation('HTTP.Response.Structure', 'response not object');
        if (!data.head || !Array.isArray(data.head.vars)) throw new ContractViolation('HTTP.Response.Structure', 'head.vars missing or not array');
        if (!data.results || !Array.isArray(data.results.bindings)) throw new ContractViolation('HTTP.Response.Structure', 'results.bindings missing or not array');
        return true;
      },

      'HTTP.Timings.Structure': (data) => {
        if (!data.timings) throw new ContractViolation('HTTP.Timings.Structure', 'timings missing');
        if (typeof data.timings.query_ms !== 'number') throw new ContractViolation('HTTP.Timings.Structure', 'query_ms not number');
        if (typeof data.timings.planning_ms !== 'number') throw new ContractViolation('HTTP.Timings.Structure', 'planning_ms not number');
        if (typeof data.timings.execution_ms !== 'number') throw new ContractViolation('HTTP.Timings.Structure', 'execution_ms not number');
        return true;
      },

      'HTTP.Handle.Validity': (handle) => {
        if (typeof handle !== 'number' || handle <= 0) throw new ContractViolation('HTTP.Handle.Validity', 'handle must be positive number');
        return true;
      },

      'gRPC.Message.Serialization': (message) => {
        if (!message) throw new ContractViolation('gRPC.Message.Serialization', 'message is null');
        try {
          JSON.stringify(message);
        } catch (e) {
          throw new ContractViolation('gRPC.Message.Serialization', 'message not serializable', { error: e.message });
        }
        return true;
      },

      'Cache.KeyFormat': (key) => {
        if (typeof key !== 'string') throw new ContractViolation('Cache.KeyFormat', 'key must be string');
        if (!key.includes(':')) throw new ContractViolation('Cache.KeyFormat', 'key must contain colon separator');
        const parts = key.split(':');
        if (parts.length < 2) throw new ContractViolation('Cache.KeyFormat', 'key must have at least 2 parts');
        return true;
      },

      'Cache.ValueFormat': (value) => {
        if (!value || typeof value !== 'object') throw new ContractViolation('Cache.ValueFormat', 'value must be object');
        return true;
      },

      'WebSocket.Event.Signature': (event, expectedFields) => {
        for (const field of expectedFields) {
          if (!(field in event)) throw new ContractViolation('WebSocket.Event.Signature', `missing field: ${field}`);
        }
        return true;
      },

      'GraphQL.Query.Structure': (query) => {
        if (typeof query !== 'string') throw new ContractViolation('GraphQL.Query.Structure', 'query must be string');
        if (!query.includes('query') && !query.includes('mutation')) throw new ContractViolation('GraphQL.Query.Structure', 'query must contain query or mutation keyword');
        return true;
      },

      'SPARQL.Query.Syntax': (sparql) => {
        if (typeof sparql !== 'string') throw new ContractViolation('SPARQL.Query.Syntax', 'SPARQL must be string');
        if (!sparql.toUpperCase().includes('SELECT') && !sparql.toUpperCase().includes('ASK') && !sparql.toUpperCase().includes('CONSTRUCT')) {
          throw new ContractViolation('SPARQL.Query.Syntax', 'SPARQL must contain SELECT, ASK, or CONSTRUCT');
        }
        return true;
      },

      'Result.Binding.Consistency': (binding, expectedVars) => {
        if (!binding || typeof binding !== 'object') throw new ContractViolation('Result.Binding.Consistency', 'binding must be object');
        for (const varName of expectedVars) {
          if (!(varName in binding)) {
            throw new ContractViolation('Result.Binding.Consistency', `missing variable: ${varName}`, { binding, expectedVars });
          }
        }
        return true;
      },

      'Index.Handle.Lifecycle': (handle, state) => {
        if (state === 'open' && typeof handle !== 'number') throw new ContractViolation('Index.Handle.Lifecycle', 'open handle must be number');
        if (state === 'closed' && handle !== null && handle !== undefined) throw new ContractViolation('Index.Handle.Lifecycle', 'closed handle must be null/undefined');
        return true;
      },

      'Protocol.Latency.Bound': (latency, protocol, bound) => {
        if (latency > bound) {
          throw new ContractViolation('Protocol.Latency.Bound', `${protocol} exceeded ${bound}ms`, { actual: latency, protocol, bound });
        }
        return true;
      },

      'Message.Roundtrip.Integrity': (sent, received) => {
        if (JSON.stringify(sent) !== JSON.stringify(received)) {
          throw new ContractViolation('Message.Roundtrip.Integrity', 'message corrupted in transit', { sent, received });
        }
        return true;
      }
    };
  }

  assert(contractName, ...args) {
    try {
      const contract = this.contracts[contractName];
      if (!contract) throw new Error(`Unknown contract: ${contractName}`);
      return contract(...args);
    } catch (error) {
      if (error instanceof ContractViolation) {
        this.violations.push(error);
        return false;
      }
      throw error;
    }
  }

  assertStrict(contractName, ...args) {
    const result = this.assert(contractName, ...args);
    if (!result) {
      const violation = this.violations[this.violations.length - 1];
      throw violation;
    }
    return result;
  }

  getViolations() {
    return this.violations;
  }

  clearViolations() {
    this.violations = [];
  }

  getViolationReport() {
    const byContract = {};
    for (const violation of this.violations) {
      if (!byContract[violation.contract]) {
        byContract[violation.contract] = [];
      }
      byContract[violation.contract].push({
        violation: violation.violation,
        context: violation.context,
        timestamp: violation.timestamp
      });
    }
    return byContract;
  }

  printReport() {
    const report = this.getViolationReport();
    console.log('\n╔════════════════════════════════════════════════════════╗');
    console.log('║            CONTRACT VIOLATION REPORT                    ║');
    console.log('╚════════════════════════════════════════════════════════╝\n');

    if (this.violations.length === 0) {
      console.log('   ✓ NO CONTRACT VIOLATIONS\n');
      return;
    }

    let totalViolations = 0;
    for (const [contract, violations] of Object.entries(report)) {
      totalViolations += violations.length;
      console.log(`   ${contract} (${violations.length} violations)`);
      for (const v of violations) {
        console.log(`     - ${v.violation}`);
      }
    }

    console.log(`\n   Total violations: ${totalViolations}`);
    console.log(`   Severity: ${totalViolations > 5 ? 'CRITICAL' : totalViolations > 0 ? 'WARNING' : 'OK'}\n`);
  }
}

module.exports = { ContractEnforcer, ContractViolation };

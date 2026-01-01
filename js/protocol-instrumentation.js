const { ResilienceHandler } = require('./resilience-layer');
const { ContractEnforcer } = require('./contract-enforcer');
const { SystemHealthMonitor } = require('./system-health-monitor');

class ProtocolInstrumentation {
  constructor(protocolName, config = {}) {
    this.protocolName = protocolName;
    this.config = {
      maxRetries: config.maxRetries || 3,
      retryDelay: config.retryDelay || 50,
      healthUpdateInterval: config.healthUpdateInterval || 5000,
      metricsWindow: config.metricsWindow || 100,
      ...config
    };

    this.resilience = new ResilienceHandler({
      maxRetries: this.config.maxRetries,
      retryDelay: this.config.retryDelay
    });

    this.enforcer = new ContractEnforcer();
    this.healthMonitor = new SystemHealthMonitor({
      updateInterval: this.config.healthUpdateInterval
    });

    this.metrics = {
      operations: {},
      errors: [],
      startTime: Date.now(),
      protocolName
    };

    this.circulitBreakersByOperation = new Map();
  }

  recordMetric(operation, latency, success = true, error = null, metadata = {}) {
    if (!this.metrics.operations[operation]) {
      this.metrics.operations[operation] = {
        count: 0,
        errors: 0,
        totalLatency: 0,
        minLatency: Infinity,
        maxLatency: 0,
        lastLatencies: [],
        lastErrors: [],
        metadata: {}
      };
    }

    const op = this.metrics.operations[operation];
    op.count++;
    op.totalLatency += latency;
    op.minLatency = Math.min(op.minLatency, latency);
    op.maxLatency = Math.max(op.maxLatency, latency);
    op.lastLatencies.push(latency);

    if (op.lastLatencies.length > this.config.metricsWindow) {
      op.lastLatencies.shift();
    }

    if (!success) {
      op.errors++;
      if (error) {
        const errorEntry = {
          operation,
          error: error.message,
          timestamp: new Date().toISOString(),
          errorType: this.resilience.classifyFailure(error),
          metadata
        };
        op.lastErrors.push(errorEntry);
        if (op.lastErrors.length > 50) op.lastErrors.shift();

        this.metrics.errors.push(errorEntry);
        if (this.metrics.errors.length > 1000) this.metrics.errors.shift();

        this.healthMonitor.recordFailure(`${this.protocolName}-${operation}`, errorEntry.errorType);
      }
    }

    Object.assign(op.metadata, metadata);
  }

  async executeWithInstrumentation(operation, handler, contracts = []) {
    const startTime = Date.now();
    const operationId = `${this.protocolName}-${operation}`;

    try {
      const result = await this.resilience.executeWithResilience(
        handler,
        operationId
      );

      const latency = Date.now() - startTime;

      if (contracts.length > 0) {
        for (const contract of contracts) {
          try {
            this.enforcer.assertStrict(contract, result);
          } catch (violation) {
            this.healthMonitor.recordViolation(contract, result);
          }
        }
      }

      this.recordMetric(operation, latency, true);
      return result;
    } catch (error) {
      const latency = Date.now() - startTime;
      this.recordMetric(operation, latency, false, error);
      throw error;
    }
  }

  getMetrics(operation = null) {
    if (operation) {
      return this.metrics.operations[operation] || null;
    }

    const stats = {};
    for (const [op, data] of Object.entries(this.metrics.operations)) {
      const avgLatency = data.count > 0 ? data.totalLatency / data.count : 0;
      const successRate = data.count > 0 ? ((data.count - data.errors) / data.count * 100).toFixed(1) : 0;

      stats[op] = {
        count: data.count,
        errors: data.errors,
        successRate,
        avgLatency: avgLatency.toFixed(2),
        minLatency: data.minLatency,
        maxLatency: data.maxLatency,
        recentLatencies: data.lastLatencies,
        recentErrors: data.lastErrors
      };
    }

    return stats;
  }

  getHealth() {
    const operationStats = this.getMetrics();
    const totalOps = Object.values(operationStats).reduce((sum, s) => sum + s.count, 0);
    const totalErrors = Object.values(operationStats).reduce((sum, s) => sum + s.errors, 0);
    const overallSuccessRate = totalOps > 0 ? ((totalOps - totalErrors) / totalOps * 100).toFixed(1) : 0;

    return {
      protocol: this.protocolName,
      uptime: Date.now() - this.metrics.startTime,
      totalOperations: totalOps,
      totalErrors,
      overallSuccessRate,
      operations: operationStats,
      status: parseFloat(overallSuccessRate) >= 95 ? 'excellent'
        : parseFloat(overallSuccessRate) >= 80 ? 'good'
        : parseFloat(overallSuccessRate) >= 60 ? 'degraded'
        : 'critical',
      lastErrors: this.metrics.errors.slice(-10)
    };
  }

  getOperationHealth(operation) {
    const metrics = this.metrics.operations[operation];
    if (!metrics) return null;

    const successRate = metrics.count > 0
      ? ((metrics.count - metrics.errors) / metrics.count * 100).toFixed(1)
      : 0;

    const p50 = metrics.lastLatencies.length > 0
      ? metrics.lastLatencies.sort((a, b) => a - b)[Math.floor(metrics.lastLatencies.length * 0.5)]
      : 0;

    const p95 = metrics.lastLatencies.length > 0
      ? metrics.lastLatencies.sort((a, b) => a - b)[Math.floor(metrics.lastLatencies.length * 0.95)]
      : 0;

    const p99 = metrics.lastLatencies.length > 0
      ? metrics.lastLatencies.sort((a, b) => a - b)[Math.floor(metrics.lastLatencies.length * 0.99)]
      : 0;

    return {
      operation,
      count: metrics.count,
      errors: metrics.errors,
      successRate,
      latency: {
        min: metrics.minLatency,
        max: metrics.maxLatency,
        avg: (metrics.totalLatency / metrics.count).toFixed(2),
        p50,
        p95,
        p99
      },
      recentErrors: metrics.lastErrors.slice(-5)
    };
  }

  addCircuitBreaker(operation, options = {}) {
    if (!this.circulitBreakersByOperation.has(operation)) {
      const cb = this.resilience.createCircuitBreaker(`${this.protocolName}-${operation}`, options);
      this.circulitBreakersByOperation.set(operation, cb);
    }
    return this.circulitBreakersByOperation.get(operation);
  }

  getCircuitBreakerStatus(operation = null) {
    if (operation) {
      const cb = this.circulitBreakersByOperation.get(operation);
      return cb ? cb.getState() : null;
    }

    const status = {};
    for (const [op, cb] of this.circulitBreakersByOperation) {
      status[op] = cb.getState();
    }
    return status;
  }

  createHealthEndpoint() {
    return (req, res) => {
      res.json(this.getHealth());
    };
  }

  createMetricsEndpoint() {
    return (req, res) => {
      const operation = req.query.operation || null;
      if (operation) {
        const metrics = this.getOperationHealth(operation);
        return res.json(metrics || { error: 'Operation not found' });
      }
      res.json(this.getMetrics());
    };
  }

  createOperationStatusEndpoint() {
    return (req, res) => {
      const { operation } = req.params;
      const metrics = this.getOperationHealth(operation);

      if (!metrics) {
        return res.status(404).json({ error: 'Operation not found' });
      }

      const cbStatus = this.getCircuitBreakerStatus(operation);

      res.json({
        operation,
        metrics,
        circuitBreaker: cbStatus,
        timestamp: new Date().toISOString()
      });
    };
  }

  startHealthMonitoring() {
    this.healthMonitor.startMonitoring();
  }

  getRecommendations() {
    const recommendations = [];
    const health = this.getHealth();

    if (health.status === 'critical') {
      recommendations.push({
        level: 'critical',
        message: `${this.protocolName} is in critical health (${health.overallSuccessRate}% success rate)`
      });
    }

    for (const [op, metrics] of Object.entries(health.operations)) {
      if (parseFloat(metrics.successRate) < 80) {
        recommendations.push({
          level: 'warning',
          operation: op,
          message: `Operation ${op} has ${metrics.successRate}% success rate`
        });
      }

      if (parseFloat(metrics.avgLatency) > 1000) {
        recommendations.push({
          level: 'warning',
          operation: op,
          message: `Operation ${op} has high latency (${metrics.avgLatency}ms avg)`
        });
      }
    }

    return recommendations;
  }
}

module.exports = ProtocolInstrumentation;

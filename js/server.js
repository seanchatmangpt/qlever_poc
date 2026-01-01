const express = require('express');
const { spawn } = require('child_process');
const path = require('path');
const { ResilienceHandler } = require('./resilience-layer');
const { ContractEnforcer } = require('./contract-enforcer');
const { SystemHealthMonitor } = require('./system-health-monitor');

const app = express();
app.use(express.json());

const handles = new Map();
const plans = new Map();

let handleCounter = 0;
let planCounter = 0;

const RUST_SERVER = path.join(__dirname, '../rust/target/debug/qlever-http');

const resilience = new ResilienceHandler({
  maxRetries: 3,
  retryDelay: 50
});

const enforcer = new ContractEnforcer();

const healthMonitor = new SystemHealthMonitor({
  updateInterval: 5000
});

const metrics = {
  operations: {},
  errors: [],
  startTime: Date.now()
};

function generateHandle() {
  return ++handleCounter;
}

function generatePlanId() {
  return ++planCounter;
}

function recordMetric(operation, latency, success = true, error = null) {
  if (!metrics.operations[operation]) {
    metrics.operations[operation] = {
      count: 0,
      errors: 0,
      totalLatency: 0,
      minLatency: Infinity,
      maxLatency: 0,
      lastLatencies: []
    };
  }

  const op = metrics.operations[operation];
  op.count++;
  op.totalLatency += latency;
  op.minLatency = Math.min(op.minLatency, latency);
  op.maxLatency = Math.max(op.maxLatency, latency);
  op.lastLatencies.push(latency);
  if (op.lastLatencies.length > 100) op.lastLatencies.shift();

  if (!success) {
    op.errors++;
    if (error) {
      metrics.errors.push({
        operation,
        error: error.message,
        timestamp: new Date().toISOString(),
        errorType: resilience.classifyFailure(error)
      });
      if (metrics.errors.length > 1000) metrics.errors.shift();
    }
  }
}

function wrapEndpoint(operationName, handler) {
  return async (req, res) => {
    const startTime = Date.now();
    try {
      const result = await resilience.executeWithResilience(
        () => handler(req),
        operationName
      );

      const latency = Date.now() - startTime;
      recordMetric(operationName, latency, true);

      try {
        if (result.head && result.results) {
          enforcer.assertStrict('HTTP.Response.Structure', result);
        }
      } catch (violation) {
        healthMonitor.recordViolation('HTTP.Response.Structure', result);
      }

      res.json(result);
    } catch (error) {
      const latency = Date.now() - startTime;
      recordMetric(operationName, latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure(operationName, errorType);
      res.status(500).json({ error: error.message, errorType });
    }
  };
}

app.post('/api/open', wrapEndpoint('open', async (req) => {
  const { indexPath, config } = req.body;

  const handle = generateHandle();
  handles.set(handle, {
    indexPath,
    config,
    createdAt: new Date(),
  });

  return { handle, indexPath, config };
}));

app.post('/api/query', wrapEndpoint('query', async (req) => {
  const { handle, sparql, timings } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  const results = {
    results: {
      bindings: [
        { s: { value: 'http://example.org/1' }, p: { value: 'http://example.org/prop' }, o: { value: 'value1' } },
        { s: { value: 'http://example.org/2' }, p: { value: 'http://example.org/prop' }, o: { value: 'value2' } },
      ]
    },
    head: {
      vars: ['s', 'p', 'o']
    }
  };

  if (timings) {
    results.timings = { query_ms: 10, planning_ms: 2, execution_ms: 8 };
  }

  return results;
}));

app.post('/api/parse-and-plan', wrapEndpoint('parse-and-plan', async (req) => {
  const { handle, sparql } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  const planId = generatePlanId();
  plans.set(planId, {
    handle,
    sparql,
    createdAt: new Date(),
  });

  return { planId };
}));

app.post('/api/execute-plan', wrapEndpoint('execute-plan', async (req) => {
  const { handle, planId, timings } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  if (!plans.has(planId)) {
    throw new Error('Invalid plan ID');
  }

  const plan = plans.get(planId);
  const results = {
    results: {
      bindings: [
        { result: { value: 'planned_result_1' } },
        { result: { value: 'planned_result_2' } },
      ]
    },
    head: {
      vars: ['result']
    }
  };

  if (timings) {
    results.timings = { query_ms: 5, planning_ms: 0, execution_ms: 5 };
  }

  return results;
}));

app.post('/api/pin-result', wrapEndpoint('pin-result', async (req) => {
  const { handle, name, sparql } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return { success: true, name, cached: true };
}));

app.post('/api/erase-result', wrapEndpoint('erase-result', async (req) => {
  const { handle, name } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return { success: true, name, erased: true };
}));

app.post('/api/clear-cache', wrapEndpoint('clear-cache', async (req) => {
  const { handle } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return { success: true, cacheCleared: true };
}));

app.post('/api/write-materialized-view', wrapEndpoint('write-materialized-view', async (req) => {
  const { handle, name, sparql } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return { success: true, view: name, created: true };
}));

app.post('/api/load-materialized-view', wrapEndpoint('load-materialized-view', async (req) => {
  const { handle, name } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return { success: true, view: name, loaded: true };
}));

app.post('/api/text-search', wrapEndpoint('text-search', async (req) => {
  const { handle, query, limit } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return {
    results: {
      bindings: [
        { match: { value: 'text_match_1' }, score: { value: '0.95' } },
        { match: { value: 'text_match_2' }, score: { value: '0.87' } },
      ]
    },
    head: {
      vars: ['match', 'score']
    }
  };
}));

app.post('/api/query-streaming', wrapEndpoint('query-streaming', async (req) => {
  const { handle, sparql } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return {
    results: {
      bindings: [
        { s: { value: 'http://example.org/1' } },
        { s: { value: 'http://example.org/2' } },
        { s: { value: 'http://example.org/3' } },
      ]
    },
    head: {
      vars: ['s']
    },
    timings: { query_ms: 20, planning_ms: 5, execution_ms: 15 }
  };
}));

app.post('/api/query-chunked', wrapEndpoint('query-chunked', async (req) => {
  const { handle, sparql, chunkSize } = req.body;

  if (!handles.has(handle)) {
    throw new Error('Invalid handle');
  }

  return {
    results: {
      bindings: Array.from({ length: chunkSize * 2 }, (_, i) => ({
        item: { value: `item_${i + 1}` }
      }))
    },
    head: {
      vars: ['item']
    }
  };
}));

app.post('/api/close', wrapEndpoint('close', async (req) => {
  const { handle } = req.body;

  if (handles.has(handle)) {
    handles.delete(handle);
  }

  return { success: true, handle, closed: true };
}));

app.get('/health', (req, res) => {
  const operationStats = {};
  for (const [op, stats] of Object.entries(metrics.operations)) {
    const avgLatency = stats.count > 0 ? stats.totalLatency / stats.count : 0;
    const errorRate = stats.count > 0 ? (stats.errors / stats.count) * 100 : 0;
    operationStats[op] = {
      count: stats.count,
      errors: stats.errors,
      avgLatency: avgLatency.toFixed(2),
      minLatency: stats.minLatency,
      maxLatency: stats.maxLatency,
      errorRate: errorRate.toFixed(1)
    };
  }

  res.json({
    status: 'ok',
    uptime: Date.now() - metrics.startTime,
    activeHandles: handles.size,
    activePlans: plans.size,
    totalErrors: metrics.errors.length,
    operations: operationStats
  });
});

app.get('/system/health', async (req, res) => {
  const health = await healthMonitor.calculateHealth();
  const report = healthMonitor.getReport();

  res.json({
    score: health.score,
    status: health.status,
    timestamp: new Date().toISOString(),
    components: {
      protocol: health.protocolHealth,
      contracts: health.contractSatisfaction,
      resilience: health.resilienceCapability
    },
    recommendations: report.recommendations
  });
});

app.get('/system/metrics', (req, res) => {
  const operationStats = {};
  for (const [op, stats] of Object.entries(metrics.operations)) {
    const avgLatency = stats.count > 0 ? stats.totalLatency / stats.count : 0;
    const recentAvg = stats.lastLatencies.length > 0
      ? stats.lastLatencies.reduce((a, b) => a + b) / stats.lastLatencies.length
      : 0;

    operationStats[op] = {
      totalRequests: stats.count,
      failedRequests: stats.errors,
      successRate: ((stats.count - stats.errors) / stats.count * 100).toFixed(1),
      avgLatency: avgLatency.toFixed(2),
      recentAvgLatency: recentAvg.toFixed(2),
      minLatency: stats.minLatency,
      maxLatency: stats.maxLatency,
      percentiles: stats.lastLatencies.length > 0 ? {
        p50: stats.lastLatencies.sort((a, b) => a - b)[Math.floor(stats.lastLatencies.length * 0.5)],
        p95: stats.lastLatencies.sort((a, b) => a - b)[Math.floor(stats.lastLatencies.length * 0.95)],
        p99: stats.lastLatencies.sort((a, b) => a - b)[Math.floor(stats.lastLatencies.length * 0.99)]
      } : null
    };
  }

  res.json({
    timestamp: new Date().toISOString(),
    uptime: Date.now() - metrics.startTime,
    operations: operationStats,
    recentErrors: metrics.errors.slice(-20),
    resilience: {
      circuitBreakers: resilience.getCircuitBreakerStates(),
      recoveryLog: resilience.getRecoveryLog()
    }
  });
});

app.get('/system/status', (req, res) => {
  res.json({
    timestamp: new Date().toISOString(),
    uptime: Date.now() - metrics.startTime,
    state: {
      handles: {
        active: handles.size,
        list: Array.from(handles.entries()).map(([id, info]) => ({
          id,
          indexPath: info.indexPath,
          createdAt: info.createdAt
        }))
      },
      plans: {
        active: plans.size,
        list: Array.from(plans.entries()).map(([id, info]) => ({
          id,
          handle: info.handle,
          createdAt: info.createdAt
        }))
      }
    },
    systemHealth: healthMonitor.getLastScore(),
    resilience: {
      activeCBs: Object.keys(resilience.circuitBreakers || {}).length,
      states: resilience.getCircuitBreakerStates()
    }
  });
});

const PORT = process.env.PORT || 3000;

const server = app.listen(PORT, async () => {
  console.log(`\n╔════════════════════════════════════════════════════════╗`);
  console.log(`║         QLever HTTP Server (Self-Observing)            ║`);
  console.log(`╚════════════════════════════════════════════════════════╝\n`);
  console.log(`   Port: ${PORT}`);
  console.log(`   Health Check: http://localhost:${PORT}/health`);
  console.log(`   System Health: http://localhost:${PORT}/system/health`);
  console.log(`   Metrics: http://localhost:${PORT}/system/metrics`);
  console.log(`   Status: http://localhost:${PORT}/system/status\n`);

  healthMonitor.startMonitoring();
});

process.on('SIGTERM', () => {
  console.log('\nShutting down gracefully...');
  server.close();
});

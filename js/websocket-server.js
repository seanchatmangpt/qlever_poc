const express = require('express');
const http = require('http');
const { Server: SocketIOServer } = require('socket.io');
const axios = require('axios');
const { ResilienceHandler } = require('./resilience-layer');
const { SystemHealthMonitor } = require('./system-health-monitor');

const app = express();
app.use(express.json());
const server = http.createServer(app);
const io = new SocketIOServer(server, {
  cors: { origin: '*' }
});

const sessions = new Map();
let handleCounter = 0;

const resilience = new ResilienceHandler({
  maxRetries: 3,
  retryDelay: 50
});

const healthMonitor = new SystemHealthMonitor({
  updateInterval: 5000
});

const metrics = {
  operations: {},
  errors: [],
  startTime: Date.now()
};

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
    activeSessions: sessions.size,
    wsConnections: io.engine.clientsCount,
    uptime: Date.now() - metrics.startTime,
    operations: operationStats,
    totalErrors: metrics.errors.length
  });
});

io.on('connection', (socket) => {
  console.log(`Client connected: ${socket.id}`);
  socket.data.startTime = Date.now();

  socket.on('open', async (data) => {
    const opStartTime = Date.now();
    const handle = ++handleCounter;
    const indexPath = data.indexPath || './index';

    try {
      const response = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/open', {
          indexPath,
          config: data.config || null
        }),
        'ws-open'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-open', latency, true);

      sessions.set(handle, {
        socketId: socket.id,
        indexPath,
        createdAt: new Date()
      });

      socket.emit('opened', { handle, success: true });
      socket.data.handle = handle;
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-open', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-open', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('query', async (data) => {
    const { handle, sparql, timings } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/query', {
          handle,
          sparql,
          timings: timings ? 1 : 0
        }),
        'ws-query'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-query', latency, true);

      socket.emit('query-result', {
        handle,
        results: response.data.results.bindings,
        variables: response.data.head.vars,
        timings: response.data.timings,
        complete: true
      });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-query', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-query', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('stream-query', async (data) => {
    const { handle, sparql, batchSize } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/query', {
          handle,
          sparql,
          timings: 1
        }),
        'ws-stream-query'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-stream-query', latency, true);

      const bindings = response.data.results.bindings;
      const batch = batchSize || 100;

      for (let i = 0; i < bindings.length; i += batch) {
        socket.emit('stream-batch', {
          handle,
          batch: i / batch,
          results: bindings.slice(i, i + batch),
          variables: response.data.head.vars,
          isLast: i + batch >= bindings.length
        });
      }

      socket.emit('stream-complete', {
        handle,
        totalResults: bindings.length,
        timings: response.data.timings
      });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-stream-query', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-stream-query', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('plan-and-execute', async (data) => {
    const { handle, sparql, reusable } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const planResponse = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/parse-and-plan', {
          handle,
          sparql
        }),
        'ws-plan-and-execute'
      );

      const planId = planResponse.data.planId;

      const execResponse = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/execute-plan', {
          handle,
          planId,
          timings: 1
        }),
        'ws-plan-and-execute'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-plan-and-execute', latency, true);

      if (reusable) {
        sessions.set(handle, Object.assign(sessions.get(handle), {
          lastPlanId: planId,
          lastQuery: sparql
        }));
      }

      socket.emit('plan-executed', {
        handle,
        planId,
        results: execResponse.data.results.bindings,
        variables: execResponse.data.head.vars,
        timings: execResponse.data.timings
      });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-plan-and-execute', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-plan-and-execute', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('cache-result', async (data) => {
    const { handle, name, sparql } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/pin-result', {
          handle,
          name,
          sparql
        }),
        'ws-cache-result'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-cache-result', latency, true);

      socket.emit('result-cached', { handle, name, success: true });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-cache-result', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-cache-result', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('text-search', async (data) => {
    const { handle, query, limit } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      const response = await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/text-search', {
          handle,
          query,
          limit: limit || 50
        }),
        'ws-text-search'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-text-search', latency, true);

      socket.emit('search-results', {
        handle,
        results: response.data.results.bindings,
        variables: response.data.head.vars
      });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-text-search', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-text-search', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('close', async (data) => {
    const { handle } = data;
    const opStartTime = Date.now();

    if (!sessions.has(handle)) {
      socket.emit('error', { message: 'Invalid handle' });
      return;
    }

    try {
      await resilience.executeWithResilience(
        () => axios.post('http://localhost:3000/api/close', { handle }),
        'ws-close'
      );

      const latency = Date.now() - opStartTime;
      recordMetric('ws-close', latency, true);

      sessions.delete(handle);
      socket.emit('closed', { handle, success: true });
    } catch (error) {
      const latency = Date.now() - opStartTime;
      recordMetric('ws-close', latency, false, error);
      const errorType = resilience.classifyFailure(error);
      healthMonitor.recordFailure('ws-close', errorType);
      socket.emit('error', { message: error.message, errorType });
    }
  });

  socket.on('request-health', async (data) => {
    try {
      const health = await healthMonitor.calculateHealth();
      socket.emit('health-update', {
        score: health.score,
        status: health.status,
        timestamp: new Date().toISOString()
      });
    } catch (error) {
      socket.emit('error', { message: 'Health check failed: ' + error.message });
    }
  });

  socket.on('disconnect', () => {
    console.log(`Client disconnected: ${socket.id}`);
    const handle = socket.data.handle;
    if (handle && sessions.has(handle)) {
      sessions.delete(handle);
    }
  });
});

const PORT = process.env.WS_PORT || 3002;

server.listen(PORT, () => {
  console.log(`\n╔════════════════════════════════════════════════════════╗`);
  console.log(`║      QLever WebSocket Server (Self-Observing)         ║`);
  console.log(`╚════════════════════════════════════════════════════════╝\n`);
  console.log(`   Port: ${PORT}`);
  console.log(`   Health Check: http://localhost:${PORT}/health`);
  console.log(`   WebSocket: ws://localhost:${PORT}`);
  console.log(`   Operations: open, query, stream-query, plan-and-execute`);
  console.log(`             cache-result, text-search, close, request-health\n`);

  healthMonitor.startMonitoring();
});

process.on('SIGTERM', () => {
  console.log('\nShutting down gracefully...');
  server.close();
});

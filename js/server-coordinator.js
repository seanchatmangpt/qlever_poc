const axios = require('axios');
const { ResilienceHandler } = require('./resilience-layer');
const { SystemHealthMonitor } = require('./system-health-monitor');

class ServerCoordinator {
  constructor(config = {}) {
    this.config = {
      httpPort: config.httpPort || 3000,
      wsPort: config.wsPort || 3002,
      graphqlPort: config.graphqlPort || 3003,
      grpcPort: config.grpcPort || 50051,
      coordinatorPort: config.coordinatorPort || 3001,
      healthCheckInterval: config.healthCheckInterval || 5000,
      ...config
    };

    this.servers = {
      http: { status: 'unknown', health: null, lastCheck: null },
      websocket: { status: 'unknown', health: null, lastCheck: null },
      graphql: { status: 'unknown', health: null, lastCheck: null },
      grpc: { status: 'unknown', health: null, lastCheck: null }
    };

    this.resilience = new ResilienceHandler({
      maxRetries: 3,
      retryDelay: 50
    });

    this.healthMonitor = new SystemHealthMonitor({
      updateInterval: 5000
    });

    this.aggregatedMetrics = {
      startTime: Date.now(),
      totalRequests: 0,
      totalErrors: 0,
      operations: {}
    };

    this.protocolHealthHistory = [];
  }

  async startHealthChecking() {
    console.log('Starting cross-protocol health monitoring...');
    setInterval(() => this.performHealthCheck(), this.config.healthCheckInterval);
  }

  async performHealthCheck() {
    const checks = [
      this.checkHTTPServer(),
      this.checkWebSocketServer(),
      this.checkGraphQLServer(),
      this.checkGRPCServer()
    ];

    await Promise.allSettled(checks);
    this.aggregateHealth();
  }

  async checkHTTPServer() {
    try {
      const response = await axios.get(`http://localhost:${this.config.httpPort}/health`, {
        timeout: 2000
      });

      this.servers.http.status = 'healthy';
      this.servers.http.health = response.data;
      this.servers.http.lastCheck = Date.now();
      this.updateAggregatedMetrics('http', response.data);
    } catch (error) {
      this.servers.http.status = 'unhealthy';
      this.servers.http.error = error.message;
      this.servers.http.lastCheck = Date.now();
    }
  }

  async checkWebSocketServer() {
    try {
      const response = await axios.get(`http://localhost:${this.config.wsPort}/health`, {
        timeout: 2000
      });

      this.servers.websocket.status = 'healthy';
      this.servers.websocket.health = response.data;
      this.servers.websocket.lastCheck = Date.now();
      this.updateAggregatedMetrics('websocket', response.data);
    } catch (error) {
      this.servers.websocket.status = 'unhealthy';
      this.servers.websocket.error = error.message;
      this.servers.websocket.lastCheck = Date.now();
    }
  }

  async checkGraphQLServer() {
    try {
      const response = await axios.post(`http://localhost:${this.config.graphqlPort}/graphql`, {
        query: '{ __typename }'
      }, {
        timeout: 2000
      });

      this.servers.graphql.status = 'healthy';
      this.servers.graphql.health = { status: 'ok', timestamp: Date.now() };
      this.servers.graphql.lastCheck = Date.now();
    } catch (error) {
      this.servers.graphql.status = 'unhealthy';
      this.servers.graphql.error = error.message;
      this.servers.graphql.lastCheck = Date.now();
    }
  }

  async checkGRPCServer() {
    try {
      const response = await axios.get(`http://localhost:${this.config.grpcPort}/health`, {
        timeout: 2000
      });

      this.servers.grpc.status = 'healthy';
      this.servers.grpc.health = response.data;
      this.servers.grpc.lastCheck = Date.now();
    } catch (error) {
      this.servers.grpc.status = 'unhealthy';
      this.servers.grpc.error = error.message;
      this.servers.grpc.lastCheck = Date.now();
    }
  }

  updateAggregatedMetrics(protocol, health) {
    if (health.operations) {
      for (const [op, stats] of Object.entries(health.operations)) {
        const key = `${protocol}-${op}`;
        if (!this.aggregatedMetrics.operations[key]) {
          this.aggregatedMetrics.operations[key] = {
            protocol,
            operation: op,
            count: 0,
            errors: 0,
            totalLatency: 0
          };
        }

        this.aggregatedMetrics.operations[key].count += parseInt(stats.count) || 0;
        this.aggregatedMetrics.operations[key].errors += parseInt(stats.errors) || 0;
        this.aggregatedMetrics.totalRequests += parseInt(stats.count) || 0;
        this.aggregatedMetrics.totalErrors += parseInt(stats.errors) || 0;
      }
    }
  }

  aggregateHealth() {
    const statuses = Object.values(this.servers).map(s => s.status);
    const healthyCount = statuses.filter(s => s === 'healthy').length;
    const totalCount = statuses.length;

    const overallStatus = healthyCount === totalCount ? 'excellent'
      : healthyCount >= totalCount * 0.75 ? 'good'
      : healthyCount >= totalCount * 0.5 ? 'degraded'
      : 'critical';

    const healthSnapshot = {
      timestamp: Date.now(),
      overallStatus,
      serverStatus: {
        http: this.servers.http.status,
        websocket: this.servers.websocket.status,
        graphql: this.servers.graphql.status,
        grpc: this.servers.grpc.status
      },
      healthyCount,
      totalCount
    };

    this.protocolHealthHistory.push(healthSnapshot);
    if (this.protocolHealthHistory.length > 1000) {
      this.protocolHealthHistory.shift();
    }

    return healthSnapshot;
  }

  getCoordinatedStatus() {
    return {
      timestamp: Date.now(),
      uptime: Date.now() - this.aggregatedMetrics.startTime,
      coordinator: {
        status: 'running',
        protocolHealthChecks: this.config.healthCheckInterval
      },
      servers: this.servers,
      aggregated: {
        totalRequests: this.aggregatedMetrics.totalRequests,
        totalErrors: this.aggregatedMetrics.totalErrors,
        successRate: this.aggregatedMetrics.totalRequests > 0
          ? ((this.aggregatedMetrics.totalRequests - this.aggregatedMetrics.totalErrors) / this.aggregatedMetrics.totalRequests * 100).toFixed(1)
          : 0,
        operationCount: Object.keys(this.aggregatedMetrics.operations).length
      },
      recentHealth: this.protocolHealthHistory.slice(-5)
    };
  }

  getOperationMetrics(protocol = null, operation = null) {
    const filtered = {};

    for (const [key, stats] of Object.entries(this.aggregatedMetrics.operations)) {
      const [proto, op] = key.split('-');

      if ((protocol && proto !== protocol) || (operation && op !== operation)) {
        continue;
      }

      const successRate = stats.count > 0
        ? ((stats.count - stats.errors) / stats.count * 100).toFixed(1)
        : 0;

      filtered[key] = {
        ...stats,
        successRate,
        avgLatency: stats.count > 0 ? (stats.totalLatency / stats.count).toFixed(2) : 0
      };
    }

    return filtered;
  }

  getHealthRecommendations() {
    const recommendations = [];

    for (const [protocol, server] of Object.entries(this.servers)) {
      if (server.status === 'unhealthy') {
        recommendations.push({
          level: 'critical',
          protocol,
          message: `${protocol} server is unreachable: ${server.error}`
        });
      }
    }

    const httpErrorRate = this.servers.http.health?.operations?.query?.errorRate || 0;
    if (parseFloat(httpErrorRate) > 5) {
      recommendations.push({
        level: 'warning',
        protocol: 'http',
        message: `HTTP query operation error rate is ${httpErrorRate}%`
      });
    }

    const protocolCount = Object.values(this.servers).filter(s => s.status === 'healthy').length;
    if (protocolCount < 2) {
      recommendations.push({
        level: 'critical',
        protocol: 'coordinator',
        message: 'Only 1 or fewer protocols healthy - system degradation detected'
      });
    }

    return recommendations;
  }

  startManagementServer(port) {
    const express = require('express');
    const app = express();
    app.use(express.json());

    app.get('/status', (req, res) => {
      res.json(this.getCoordinatedStatus());
    });

    app.get('/health', (req, res) => {
      const health = this.aggregateHealth();
      res.json(health);
    });

    app.get('/metrics', (req, res) => {
      const protocol = req.query.protocol || null;
      const operation = req.query.operation || null;
      res.json(this.getOperationMetrics(protocol, operation));
    });

    app.get('/recommendations', (req, res) => {
      res.json(this.getHealthRecommendations());
    });

    app.post('/failover/:protocol', async (req, res) => {
      const { protocol } = req.params;

      const recommendations = this.getHealthRecommendations();
      const hasIssues = recommendations.some(r => r.protocol === protocol);

      if (!hasIssues) {
        return res.status(400).json({
          error: `${protocol} is currently healthy - failover not needed`
        });
      }

      res.json({
        protocol,
        action: 'failover-initiated',
        timestamp: Date.now(),
        alternatives: this.getAlternativeRouting(protocol)
      });
    });

    app.get('/protocol/:protocol/details', (req, res) => {
      const { protocol } = req.params;
      const server = this.servers[protocol];

      if (!server) {
        return res.status(404).json({ error: 'Protocol not found' });
      }

      res.json({
        protocol,
        status: server.status,
        lastCheck: server.lastCheck,
        health: server.health,
        error: server.error,
        operationMetrics: this.getOperationMetrics(protocol)
      });
    });

    const server = app.listen(port, () => {
      console.log(`\n╔════════════════════════════════════════════════════════╗`);
      console.log(`║        QLever Server Coordinator (Self-Orchestrating)  ║`);
      console.log(`╚════════════════════════════════════════════════════════╝\n`);
      console.log(`   Coordinator Port: ${port}`);
      console.log(`   Management Endpoints:`);
      console.log(`     Status: http://localhost:${port}/status`);
      console.log(`     Health: http://localhost:${port}/health`);
      console.log(`     Metrics: http://localhost:${port}/metrics`);
      console.log(`     Recommendations: http://localhost:${port}/recommendations`);
      console.log(`     Protocol Details: http://localhost:${port}/protocol/{http|websocket|graphql|grpc}/details\n`);
      console.log(`   Monitored Servers:`);
      console.log(`     HTTP: localhost:${this.config.httpPort}`);
      console.log(`     WebSocket: localhost:${this.config.wsPort}`);
      console.log(`     GraphQL: localhost:${this.config.graphqlPort}`);
      console.log(`     gRPC: localhost:${this.config.grpcPort}\n`);
    });

    process.on('SIGTERM', () => {
      console.log('\nCoordinator shutting down gracefully...');
      server.close();
    });

    return server;
  }

  getAlternativeRouting(failedProtocol) {
    const alternatives = [];
    const mapping = {
      'http': 'websocket',
      'websocket': 'graphql',
      'graphql': 'grpc',
      'grpc': 'http'
    };

    const primary = mapping[failedProtocol];
    if (primary && this.servers[primary].status === 'healthy') {
      alternatives.push({
        protocol: primary,
        port: this.config[`${primary}Port`],
        priority: 1
      });
    }

    for (const [proto, server] of Object.entries(this.servers)) {
      if (proto !== failedProtocol && server.status === 'healthy' && proto !== primary) {
        alternatives.push({
          protocol: proto,
          port: this.config[`${proto}Port`],
          priority: alternatives.length + 1
        });
      }
    }

    return alternatives;
  }
}

module.exports = ServerCoordinator;

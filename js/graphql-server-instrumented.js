const express = require('express');
const { ApolloServer, gql } = require('@apollo/server');
const { expressMiddleware } = require('@apollo/server/express4');
const axios = require('axios');
const ProtocolInstrumentation = require('./protocol-instrumentation');

const app = express();
app.use(express.json());

const instrumentation = new ProtocolInstrumentation('graphql', {
  maxRetries: 3,
  retryDelay: 50,
  healthUpdateInterval: 5000,
  metricsWindow: 100
});

const typeDefs = gql`
  type Variable {
    value: String!
  }

  type Binding {
    s: Variable
    p: Variable
    o: Variable
    match: Variable
    score: Variable
  }

  type Timings {
    query_ms: Int
    planning_ms: Int
    execution_ms: Int
  }

  type QueryResult {
    head: [String!]!
    results: [Binding!]!
    timings: Timings
  }

  type Plan {
    planId: Int!
  }

  type CacheResult {
    success: Boolean!
    name: String
    cached: Boolean
  }

  type SearchResult {
    head: [String!]!
    results: [Binding!]!
  }

  type OperationHealth {
    operation: String!
    count: Int!
    errors: Int!
    successRate: String!
    latency: LatencyStats!
  }

  type LatencyStats {
    min: Float!
    max: Float!
    avg: String!
    p50: Float!
    p95: Float!
    p99: Float!
  }

  type CircuitBreakerStatus {
    state: String!
    failures: Int
    lastFailure: String
  }

  type OperationStatus {
    operation: String!
    metrics: OperationHealth!
    circuitBreaker: CircuitBreakerStatus
  }

  type ProtocolHealth {
    protocol: String!
    uptime: Int!
    totalOperations: Int!
    totalErrors: Int!
    overallSuccessRate: String!
    status: String!
  }

  type Query {
    query(handle: Int!, sparql: String!, timings: Boolean): QueryResult
    parseAndPlan(handle: Int!, sparql: String!): Plan
    executePlan(handle: Int!, planId: Int!, timings: Boolean): QueryResult
    textSearch(handle: Int!, query: String!, limit: Int): SearchResult
    openIndex(indexPath: String!): Handle
    closeIndex(handle: Int!): CloseResult
    pinResult(handle: Int!, name: String!, sparql: String!): CacheResult
    eraseResult(handle: Int!, name: String!): CacheResult

    health: ProtocolHealth
    operationStatus(operation: String!): OperationStatus
    allMetrics: [OperationHealth!]
  }

  type Handle {
    handle: Int!
  }

  type CloseResult {
    success: Boolean!
    handle: Int!
  }

  type Mutation {
    open(indexPath: String, config: String): Handle
    close(handle: Int!): CloseResult
  }
`;

const resolvers = {
  Query: {
    query: async (_, { handle, sparql, timings }) => {
      return await instrumentation.executeWithInstrumentation(
        'query',
        () => axios.post('http://localhost:3000/api/query', {
          handle,
          sparql,
          timings: timings ? 1 : 0
        }).then(r => ({
          head: r.data.head.vars,
          results: r.data.results.bindings,
          timings: r.data.timings
        })),
        ['HTTP.Response.Structure']
      );
    },

    parseAndPlan: async (_, { handle, sparql }) => {
      return await instrumentation.executeWithInstrumentation(
        'parse-and-plan',
        () => axios.post('http://localhost:3000/api/parse-and-plan', {
          handle,
          sparql
        }).then(r => r.data),
        []
      );
    },

    executePlan: async (_, { handle, planId, timings }) => {
      return await instrumentation.executeWithInstrumentation(
        'execute-plan',
        () => axios.post('http://localhost:3000/api/execute-plan', {
          handle,
          planId,
          timings: timings ? 1 : 0
        }).then(r => ({
          head: r.data.head.vars,
          results: r.data.results.bindings,
          timings: r.data.timings
        })),
        []
      );
    },

    textSearch: async (_, { handle, query, limit }) => {
      return await instrumentation.executeWithInstrumentation(
        'text-search',
        () => axios.post('http://localhost:3000/api/text-search', {
          handle,
          query,
          limit: limit || 50
        }).then(r => ({
          head: r.data.head.vars,
          results: r.data.results.bindings
        })),
        []
      );
    },

    openIndex: async (_, { indexPath }) => {
      return await instrumentation.executeWithInstrumentation(
        'open',
        () => axios.post('http://localhost:3000/api/open', {
          indexPath
        }).then(r => ({ handle: r.data.handle })),
        []
      );
    },

    closeIndex: async (_, { handle }) => {
      return await instrumentation.executeWithInstrumentation(
        'close',
        () => axios.post('http://localhost:3000/api/close', {
          handle
        }).then(r => ({ success: r.data.success, handle: r.data.handle })),
        []
      );
    },

    pinResult: async (_, { handle, name, sparql }) => {
      return await instrumentation.executeWithInstrumentation(
        'pin-result',
        () => axios.post('http://localhost:3000/api/pin-result', {
          handle,
          name,
          sparql
        }).then(r => ({ success: r.data.success, name: r.data.name, cached: r.data.cached })),
        []
      );
    },

    eraseResult: async (_, { handle, name }) => {
      return await instrumentation.executeWithInstrumentation(
        'erase-result',
        () => axios.post('http://localhost:3000/api/erase-result', {
          handle,
          name
        }).then(r => ({ success: r.data.success, name: r.data.name })),
        []
      );
    },

    health: async () => {
      return instrumentation.getHealth();
    },

    operationStatus: async (_, { operation }) => {
      const metrics = instrumentation.getOperationHealth(operation);
      if (!metrics) throw new Error(`Operation ${operation} not found`);

      const cbStatus = instrumentation.getCircuitBreakerStatus(operation);
      return { operation, metrics, circuitBreaker: cbStatus };
    },

    allMetrics: async () => {
      const metrics = instrumentation.getMetrics();
      return Object.entries(metrics).map(([op, data]) => ({
        operation: op,
        ...data
      }));
    }
  },

  Mutation: {
    open: async (_, { indexPath, config }) => {
      const result = await instrumentation.executeWithInstrumentation(
        'open',
        () => axios.post('http://localhost:3000/api/open', {
          indexPath: indexPath || './index',
          config
        }).then(r => ({ handle: r.data.handle })),
        []
      );
      return result;
    },

    close: async (_, { handle }) => {
      return await instrumentation.executeWithInstrumentation(
        'close',
        () => axios.post('http://localhost:3000/api/close', {
          handle
        }).then(r => ({ success: r.data.success, handle: r.data.handle })),
        []
      );
    }
  }
};

const server = new ApolloServer({
  typeDefs,
  resolvers,
  introspection: true
});

app.get('/health', instrumentation.createHealthEndpoint());
app.get('/metrics', instrumentation.createMetricsEndpoint());
app.get('/operation/:operation', instrumentation.createOperationStatusEndpoint());
app.get('/recommendations', (req, res) => {
  res.json(instrumentation.getRecommendations());
});

async function startServer() {
  await server.start();
  app.use('/graphql', expressMiddleware(server));

  const PORT = process.env.GRAPHQL_PORT || 3003;

  app.listen(PORT, () => {
    console.log(`\n╔════════════════════════════════════════════════════════╗`);
    console.log(`║   QLever GraphQL Server (Self-Observing via Framework) ║`);
    console.log(`╚════════════════════════════════════════════════════════╝\n`);
    console.log(`   Port: ${PORT}`);
    console.log(`   GraphQL Endpoint: http://localhost:${PORT}/graphql`);
    console.log(`   Health: http://localhost:${PORT}/health`);
    console.log(`   Metrics: http://localhost:${PORT}/metrics`);
    console.log(`   Recommendations: http://localhost:${PORT}/recommendations\n`);

    instrumentation.startHealthMonitoring();
  });
}

startServer().catch(console.error);

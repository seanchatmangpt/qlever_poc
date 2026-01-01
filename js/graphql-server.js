const express = require('express');
const { ApolloServer, gql } = require('@apollo/server');
const { expressMiddleware } = require('@apollo/server/express4');
const axios = require('axios');

const typeDefs = gql`
  type Query {
    queryIndex(handle: Int!, sparql: String!, timings: Boolean): QueryResult
    searchText(handle: Int!, query: String!, limit: Int): SearchResult
    health: HealthStatus
  }

  type Mutation {
    openIndex(indexPath: String!, config: String): OpenResult
    closeIndex(handle: Int!): CloseResult
    cacheResult(handle: Int!, name: String!, sparql: String!): CacheResult
    eraseResult(handle: Int!, name: String!): EraseResult
    clearCache(handle: Int!): ClearCacheResult
    writeMaterializedView(handle: Int!, name: String!, sparql: String!): ViewResult
    loadMaterializedView(handle: Int!, name: String!): ViewResult
    parseAndPlan(handle: Int!, sparql: String!): PlanResult
    executePlan(handle: Int!, planId: String!, timings: Boolean): QueryResult
  }

  type QueryResult {
    variables: [String!]!
    bindings: [Binding!]!
    timings: Timings
    success: Boolean!
  }

  type Binding {
    values: [String!]!
  }

  type Timings {
    queryMs: Int
    planningMs: Int
    executionMs: Int
  }

  type SearchResult {
    variables: [String!]!
    results: [String!]!
    count: Int!
  }

  type OpenResult {
    handle: Int!
    indexPath: String!
    success: Boolean!
  }

  type CloseResult {
    handle: Int!
    closed: Boolean!
  }

  type CacheResult {
    handle: Int!
    name: String!
    cached: Boolean!
  }

  type EraseResult {
    handle: Int!
    name: String!
    erased: Boolean!
  }

  type ClearCacheResult {
    handle: Int!
    cleared: Boolean!
  }

  type ViewResult {
    handle: Int!
    name: String!
    success: Boolean!
  }

  type PlanResult {
    handle: Int!
    planId: String!
    success: Boolean!
  }

  type HealthStatus {
    status: String!
    timestamp: String!
  }
`;

const resolvers = {
  Query: {
    queryIndex: async (_, { handle, sparql, timings }) => {
      try {
        const response = await axios.post('http://localhost:3000/api/query', {
          handle,
          sparql,
          timings: timings ? 1 : 0
        });

        const bindings = response.data.results.bindings.map(binding => ({
          values: Object.keys(binding).map(key => binding[key].value)
        }));

        return {
          variables: response.data.head.vars,
          bindings,
          timings: response.data.timings ? {
            queryMs: response.data.timings.query_ms,
            planningMs: response.data.timings.planning_ms,
            executionMs: response.data.timings.execution_ms
          } : null,
          success: true
        };
      } catch (error) {
        throw new Error(`Query failed: ${error.message}`);
      }
    },

    searchText: async (_, { handle, query, limit }) => {
      try {
        const response = await axios.post('http://localhost:3000/api/text-search', {
          handle,
          query,
          limit: limit || 50
        });

        const results = response.data.results.bindings.map(binding =>
          Object.values(binding).map(v => v.value).join(' ')
        );

        return {
          variables: response.data.head.vars,
          results,
          count: results.length
        };
      } catch (error) {
        throw new Error(`Search failed: ${error.message}`);
      }
    },

    health: async () => {
      return {
        status: 'ok',
        timestamp: new Date().toISOString()
      };
    }
  },

  Mutation: {
    openIndex: async (_, { indexPath, config }) => {
      try {
        const response = await axios.post('http://localhost:3000/api/open', {
          indexPath,
          config: config ? JSON.parse(config) : null
        });

        return {
          handle: response.data.handle,
          indexPath: response.data.indexPath,
          success: response.data.success
        };
      } catch (error) {
        throw new Error(`Failed to open index: ${error.message}`);
      }
    },

    closeIndex: async (_, { handle }) => {
      try {
        await axios.post('http://localhost:3000/api/close', { handle });
        return { handle, closed: true };
      } catch (error) {
        throw new Error(`Failed to close index: ${error.message}`);
      }
    },

    cacheResult: async (_, { handle, name, sparql }) => {
      try {
        await axios.post('http://localhost:3000/api/pin-result', {
          handle,
          name,
          sparql
        });

        return { handle, name, cached: true };
      } catch (error) {
        throw new Error(`Failed to cache result: ${error.message}`);
      }
    },

    eraseResult: async (_, { handle, name }) => {
      try {
        await axios.post('http://localhost:3000/api/erase-result', {
          handle,
          name
        });

        return { handle, name, erased: true };
      } catch (error) {
        throw new Error(`Failed to erase result: ${error.message}`);
      }
    },

    clearCache: async (_, { handle }) => {
      try {
        await axios.post('http://localhost:3000/api/clear-cache', { handle });
        return { handle, cleared: true };
      } catch (error) {
        throw new Error(`Failed to clear cache: ${error.message}`);
      }
    },

    writeMaterializedView: async (_, { handle, name, sparql }) => {
      try {
        await axios.post('http://localhost:3000/api/write-materialized-view', {
          handle,
          name,
          sparql
        });

        return { handle, name, success: true };
      } catch (error) {
        throw new Error(`Failed to write materialized view: ${error.message}`);
      }
    },

    loadMaterializedView: async (_, { handle, name }) => {
      try {
        await axios.post('http://localhost:3000/api/load-materialized-view', {
          handle,
          name
        });

        return { handle, name, success: true };
      } catch (error) {
        throw new Error(`Failed to load materialized view: ${error.message}`);
      }
    },

    parseAndPlan: async (_, { handle, sparql }) => {
      try {
        const response = await axios.post('http://localhost:3000/api/parse-and-plan', {
          handle,
          sparql
        });

        return {
          handle,
          planId: response.data.planId,
          success: response.data.success
        };
      } catch (error) {
        throw new Error(`Failed to parse and plan: ${error.message}`);
      }
    },

    executePlan: async (_, { handle, planId, timings }) => {
      try {
        const response = await axios.post('http://localhost:3000/api/execute-plan', {
          handle,
          planId,
          timings: timings ? 1 : 0
        });

        const bindings = response.data.results.bindings.map(binding => ({
          values: Object.keys(binding).map(key => binding[key].value)
        }));

        return {
          variables: response.data.head.vars,
          bindings,
          timings: response.data.timings ? {
            queryMs: response.data.timings.query_ms,
            planningMs: response.data.timings.planning_ms,
            executionMs: response.data.timings.execution_ms
          } : null,
          success: true
        };
      } catch (error) {
        throw new Error(`Failed to execute plan: ${error.message}`);
      }
    }
  }
};

async function startServer() {
  const app = express();
  app.use(express.json());

  const server = new ApolloServer({
    typeDefs,
    resolvers,
    introspection: true
  });

  await server.start();
  app.use('/graphql', expressMiddleware(server));

  app.get('/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
  });

  const PORT = process.env.GRAPHQL_PORT || 3003;
  app.listen(PORT, () => {
    console.log(`GraphQL server listening on http://localhost:${PORT}/graphql`);
  });
}

startServer().catch(err => console.error('Failed to start GraphQL server:', err));

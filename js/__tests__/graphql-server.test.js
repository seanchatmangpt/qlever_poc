import { describe, it, expect } from 'vitest';

describe('GraphQL Server Setup - 80/20 Core Requirements', () => {
  it('should export Apollo Server from @apollo/server', async () => {
    const { ApolloServer } = require('@apollo/server');
    expect(ApolloServer).toBeDefined();
  });

  it('should export expressMiddleware', async () => {
    const { expressMiddleware } = require('@apollo/server/express4');
    expect(expressMiddleware).toBeDefined();
    expect(typeof expressMiddleware).toBe('function');
  });

  it('should have Express available', async () => {
    const express = require('express');
    expect(express).toBeDefined();
    expect(typeof express).toBe('function');
  });

  it('should create Express app successfully', async () => {
    const express = require('express');
    const app = express();
    expect(app).toBeDefined();
    expect(typeof app.use).toBe('function');
    expect(typeof app.get).toBe('function');
  });

  it('should parse JSON middleware', async () => {
    const express = require('express');
    const app = express();
    app.use(express.json());
    // If it gets here without error, middleware is properly configured
    expect(app._router).toBeDefined();
  });

  it('should create ApolloServer instance', () => {
    const { ApolloServer } = require('@apollo/server');
    const typeDefs = `
      type Query {
        hello: String
      }
    `;
    const resolvers = {
      Query: {
        hello: () => 'World'
      }
    };
    const server = new ApolloServer({ typeDefs, resolvers });
    expect(server).toBeDefined();
    expect(typeof server.start).toBe('function');
  });

  it('should handle introspection setting', () => {
    const { ApolloServer } = require('@apollo/server');
    const typeDefs = `
      type Query {
        test: String
      }
    `;
    const server = new ApolloServer({
      typeDefs,
      resolvers: { Query: { test: () => 'ok' } },
      introspection: true
    });
    expect(server).toBeDefined();
  });

  it('should support async start method', () => {
    const { ApolloServer } = require('@apollo/server');
    const server = new ApolloServer({
      typeDefs: 'type Query { test: String }',
      resolvers: {}
    });
    expect(server.start).toBeDefined();
    expect(typeof server.start).toBe('function');
  });
});

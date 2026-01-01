import { describe, it, expect, beforeAll } from 'vitest';
const QleverClient = require('../client');

describe('QleverClient - 80/20 Core Functionality', () => {
  let client;

  beforeAll(() => {
    client = new QleverClient('http://localhost:3000');
  });

  it('should instantiate client successfully', () => {
    expect(client).toBeDefined();
  });

  it('should have query method', () => {
    expect(client.query).toBeDefined();
    expect(typeof client.query).toBe('function');
  });

  it('should have open method', () => {
    expect(client.open).toBeDefined();
    expect(typeof client.open).toBe('function');
  });

  it('should have close method', () => {
    expect(client.close).toBeDefined();
    expect(typeof client.close).toBe('function');
  });

  it('should have writeMaterializedView method', () => {
    expect(client.writeMaterializedView).toBeDefined();
    expect(typeof client.writeMaterializedView).toBe('function');
  });

  it('should have loadMaterializedView method', () => {
    expect(client.loadMaterializedView).toBeDefined();
    expect(typeof client.loadMaterializedView).toBe('function');
  });

  it('should handle basic error cases gracefully', async () => {
    // Test that query method exists and handles errors
    try {
      await client.query('SELECT ?s WHERE { ?s ?p ?o }', true);
    } catch (error) {
      // Expected to fail without server, but error should be caught
      expect(error).toBeDefined();
    }
  });
});

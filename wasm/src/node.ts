/**
 * Node.js-specific exports and utilities for QLever WASM
 */

import * as wasm from "./pkg/qlever_wasm";

export { QleverClient, QueryResponse, QueryBuilder, ResultFormat } from "./pkg/qlever_wasm";
export type { SparqlResult, SparqlBinding } from "./index.d";

/**
 * Initialize the WASM module for Node.js usage
 */
export async function init(): Promise<void> {
  // In Node.js, we might need to do additional setup
  if (typeof wasm !== "undefined") {
    try {
      wasm.set_panic_hook();
    } catch (e) {
      console.warn("Failed to set panic hook:", e);
    }
  }
}

/**
 * Create a QLever client instance
 * @param endpoint - QLever server endpoint
 * @returns QleverClient instance
 */
export function createClient(endpoint: string) {
  return new wasm.QleverClient(endpoint);
}

/**
 * Create a query builder instance
 * @returns QueryBuilder instance
 */
export function createQueryBuilder() {
  return new wasm.QueryBuilder();
}

/**
 * Helper function to execute a simple SPARQL query
 * @param endpoint - QLever server endpoint
 * @param sparqlQuery - SPARQL query string
 * @param format - Result format (default: "json")
 * @returns Promise<any> - Query results
 */
export async function executeQuery(
  endpoint: string,
  sparqlQuery: string,
  format: string = "json"
): Promise<any> {
  const client = new wasm.QleverClient(endpoint);
  const response = await client.query(sparqlQuery, format);
  return response.data();
}

/**
 * Helper function to check server connectivity
 * @param endpoint - QLever server endpoint
 * @returns Promise<boolean> - true if server is reachable
 */
export async function checkServerHealth(endpoint: string): Promise<boolean> {
  const client = new wasm.QleverClient(endpoint);
  return client.ping();
}

/**
 * Export wasm module for direct access if needed
 */
export const wasmModule = wasm;

/**
 * Node.js specific: Batch query execution with file support
 * @param endpoint - QLever server endpoint
 * @param queries - Array of SPARQL queries
 * @param format - Result format (default: "json")
 * @returns Promise<any[]> - Array of results
 */
export async function batchQueries(
  endpoint: string,
  queries: string[],
  format: string = "json"
): Promise<any[]> {
  const client = new wasm.QleverClient(endpoint);
  const results = [];

  for (const query of queries) {
    try {
      const response = await client.query(query, format);
      results.push({
        success: true,
        data: response.data(),
        executionTime: response.execution_time_ms(),
      });
    } catch (error) {
      results.push({
        success: false,
        error: String(error),
      });
    }
  }

  return results;
}

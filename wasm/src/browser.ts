/**
 * Browser-specific exports and utilities for QLever WASM
 */

import * as wasm from "./pkg/qlever_wasm";

export { QleverClient, QueryResponse, QueryBuilder, ResultFormat } from "./pkg/qlever_wasm";
export type { SparqlResult, SparqlBinding } from "./index.d";

/**
 * Initialize the WASM module for browser usage
 */
export async function init(): Promise<void> {
  // In browser, the wasm module is already loaded
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

/**
 * Node.js-specific exports for QLever WASM embedded library
 *
 * IMPORTANT: This module exports ONLY the embedded libqlever library API.
 * It does NOT expose HTTP server/client functionality.
 *
 * Usage:
 *   import { initializeWasm, createStore } from 'qlever-wasm/node';
 *
 *   await initializeWasm();
 *   const store = createStore();
 *   await store.init('/path/to/index');
 *   const results = await store.query('SELECT ?s WHERE { ?s ?p ?o }');
 */

import * as wasm from "./pkg/qlever_wasm";

// Export embedded libqlever Store API
export { QleverStore, QueryBuilder } from "./pkg/qlever_wasm";
export type { SparqlResult, SparqlBinding } from "./index.d";

/**
 * Initialize the WASM module for Node.js usage
 *
 * Sets up panic hooks for better error reporting.
 * Must be called before creating stores.
 *
 * @returns Promise that resolves when initialization is complete
 */
export async function initializeWasm(): Promise<void> {
  if (typeof wasm !== "undefined") {
    try {
      wasm.set_panic_hook();
    } catch (e) {
      console.warn("Failed to set panic hook:", e);
    }
  }
}

/**
 * Alias for initializeWasm() for backward compatibility
 *
 * @deprecated Use initializeWasm() instead
 */
export async function init(): Promise<void> {
  return initializeWasm();
}

/**
 * Create a new embedded QLever store instance
 *
 * The store must be initialized with init() before use.
 *
 * @returns QleverStore instance
 *
 * @example
 *   const store = createStore();
 *   await store.init('/path/to/index');
 *   const results = await store.query('SELECT * WHERE { ?s ?p ?o }');
 */
export function createStore(): wasm.QleverStore {
  return new wasm.QleverStore();
}

/**
 * Create a query builder instance for constructing SPARQL queries
 *
 * @returns QueryBuilder instance
 *
 * @example
 *   const builder = new QueryBuilder()
 *     .select('?s ?p ?o')
 *     .where_clause('?s ?p ?o')
 *     .limit(100);
 *   const query = builder.build();
 */
export function createQueryBuilder(): wasm.QueryBuilder {
  return new wasm.QueryBuilder();
}

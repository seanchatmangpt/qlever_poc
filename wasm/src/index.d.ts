/**
 * QLever WebAssembly Bindings for JavaScript/Node.js
 * High-performance SPARQL query client with TypeScript support
 */

export enum ResultFormat {
  Json = "json",
  Xml = "xml",
  Csv = "csv",
}

export interface SparqlBinding {
  [key: string]: {
    type: string;
    value: string;
    "xml:lang"?: string;
    datatype?: string;
  };
}

export interface SparqlResult {
  head: {
    vars: string[];
  };
  results: {
    bindings: SparqlBinding[];
  };
  execution_time_ms?: number;
}

/**
 * Query response wrapper
 */
export class QueryResponse {
  /**
   * Get the response data
   */
  data(): any;

  /**
   * Get the response format
   */
  format(): string;

  /**
   * Get execution time in milliseconds
   */
  execution_time_ms(): number;

  /**
   * Convert response to JSON object
   */
  to_json(): any;

  /**
   * Convert response to string
   */
  to_string(): string;
}

/**
 * QLever Client for querying SPARQL endpoints
 * Works in both Browser and Node.js environments
 */
export class QleverClient {
  /**
   * Create a new QLever client
   * @param endpoint - QLever server endpoint (e.g., "http://localhost:7023")
   */
  constructor(endpoint: string);

  /**
   * Execute a SPARQL query
   * @param query - SPARQL query string
   * @param format - Result format ("json", "xml", or "csv")
   * @returns Promise resolving to QueryResponse
   */
  query(query: string, format: string): Promise<QueryResponse>;

  /**
   * Execute a SPARQL query with custom headers
   * @param query - SPARQL query string
   * @param format - Result format ("json", "xml", or "csv")
   * @param headers - Custom headers as object
   * @returns Promise resolving to QueryResponse
   */
  query_with_headers(query: string, format: string, headers: Record<string, string>): Promise<QueryResponse>;

  /**
   * Check if the QLever server is reachable
   * @returns Promise resolving to boolean (true if reachable)
   */
  ping(): Promise<boolean>;

  /**
   * Get the endpoint URL
   */
  endpoint(): string;
}

/**
 * Query builder for constructing SPARQL queries
 */
export class QueryBuilder {
  /**
   * Create a new query builder
   */
  constructor();

  /**
   * Add SELECT clause
   * @param vars - Variables to select (e.g., "?subject ?predicate ?object")
   */
  select(vars: string): QueryBuilder;

  /**
   * Add FROM clause
   * @param graph - Graph URI
   */
  from(graph: string): QueryBuilder;

  /**
   * Add WHERE clause
   * @param pattern - Graph pattern
   */
  where_clause(pattern: string): QueryBuilder;

  /**
   * Build the final query string
   */
  build(): string;
}

/**
 * Initialize the WASM module (required for some features)
 */
export function init(): Promise<void>;

/**
 * Set panic hook for better error messages in development
 */
export function set_panic_hook(): void;

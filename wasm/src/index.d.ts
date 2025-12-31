/**
 * QLever WebAssembly Bindings for JavaScript/Node.js
 * High-performance SPARQL query client with full SPARQL 1.1 support
 */

export enum QueryType {
  Select = 0,
  Construct = 1,
  Describe = 2,
  Ask = 3,
}

export enum ResultFormat {
  Json = "json",
  Xml = "xml",
  Csv = "csv",
  Turtle = "turtle",
  NTriples = "ntriples",
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
 * Query builder for constructing SPARQL queries with full SPARQL 1.1 support
 */
export class QueryBuilder {
  /**
   * Create a new query builder (defaults to SELECT)
   */
  constructor();

  /**
   * Set query type to SELECT
   */
  select_query(): QueryBuilder;

  /**
   * Set query type to CONSTRUCT
   */
  construct_query(): QueryBuilder;

  /**
   * Set query type to DESCRIBE
   */
  describe_query(): QueryBuilder;

  /**
   * Set query type to ASK
   */
  ask_query(): QueryBuilder;

  /**
   * Add SELECT variables
   * @param vars - Variables to select (e.g., "?subject ?predicate ?object")
   */
  select(vars: string): QueryBuilder;

  /**
   * Add CONSTRUCT template
   * @param template - Triple patterns for result (e.g., "?s ?p ?o")
   */
  construct(template: string): QueryBuilder;

  /**
   * Add DESCRIBE variables
   * @param vars - Variables to describe
   */
  describe(vars: string): QueryBuilder;

  /**
   * Add FROM clause
   * @param graph - Graph URI
   */
  from(graph: string): QueryBuilder;

  /**
   * Add WHERE clause pattern
   * @param pattern - Graph pattern for matching
   */
  where_clause(pattern: string): QueryBuilder;

  /**
   * Add FILTER condition
   * @param condition - SPARQL filter expression (without FILTER keyword)
   */
  filter(condition: string): QueryBuilder;

  /**
   * Add OPTIONAL pattern
   * @param pattern - Optional graph pattern
   */
  optional(pattern: string): QueryBuilder;

  /**
   * Add BIND statement for variable binding
   * @param expression - Expression to bind
   * @param var - Variable name (e.g., "?label")
   */
  bind(expression: string, var: string): QueryBuilder;

  /**
   * Add GROUP BY clause
   * @param vars - Variables to group by (space-separated)
   */
  group_by(vars: string): QueryBuilder;

  /**
   * Add ORDER BY clause in ascending order
   * @param vars - Variables to order by (space-separated)
   */
  order_by(vars: string): QueryBuilder;

  /**
   * Add ORDER BY clause in descending order
   * @param vars - Variables to order by (space-separated)
   */
  order_by_desc(vars: string): QueryBuilder;

  /**
   * Add LIMIT clause
   * @param count - Maximum number of results
   */
  limit(count: number): QueryBuilder;

  /**
   * Add OFFSET clause
   * @param count - Number of results to skip
   */
  offset(count: number): QueryBuilder;

  /**
   * Add DISTINCT modifier to SELECT
   */
  distinct(): QueryBuilder;

  /**
   * Add VALUES clause for inline data
   * @param clause - VALUES clause (e.g., "VALUES (?x) { (1) (2) (3) }")
   */
  values(clause: string): QueryBuilder;

  /**
   * Build the final SPARQL query string
   * @returns Complete SPARQL query as string
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

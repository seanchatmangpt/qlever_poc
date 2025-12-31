/**
 * QLever WebAssembly Bindings for JavaScript/Node.js
 * High-performance SPARQL 1.1 query client with in-memory RDF store
 * Compatible with RDF/JS specification and Oxigraph API
 */

export enum QueryType {
  Select = 0,
  Construct = 1,
  Describe = 2,
  Ask = 3,
}

/**
 * RDF Term types
 */
export enum RdfTermType {
  NamedNode = 0,
  BlankNode = 1,
  Literal = 2,
  DefaultGraph = 3,
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
 * RDF Term - Represents an RDF resource, literal, or blank node
 * Implements RDF/JS specification
 */
export class RdfTerm {
  /**
   * Check if this is a named node (IRI)
   */
  is_named_node(): boolean;

  /**
   * Check if this is a blank node
   */
  is_blank_node(): boolean;

  /**
   * Check if this is a literal
   */
  is_literal(): boolean;

  /**
   * Get the IRI value if this is a named node
   */
  as_iri(): string | null;

  /**
   * Get the literal value if this is a literal
   */
  as_literal_value(): string | null;

  /**
   * Convert to N-Quads string representation
   */
  to_n_quads(): string;
}

/**
 * RDF Triple (subject, predicate, object)
 */
export class Triple {
  subject: RdfTerm;
  predicate: RdfTerm;
  object: RdfTerm;

  constructor(subject: string, predicate: string, object: string);

  /**
   * Convert to N-Triples format
   */
  to_n_triples(): string;
}

/**
 * RDF Quad (subject, predicate, object, graph)
 * Default graph when graph is omitted
 */
export class Quad {
  subject: RdfTerm;
  predicate: RdfTerm;
  object: RdfTerm;
  graph: RdfTerm;

  constructor(subject: string, predicate: string, object: string, graph?: string);

  /**
   * Convert to N-Quads format
   */
  to_n_quads(): string;
}

/**
 * Pattern for matching quads in the store
 * Use null/undefined for wildcard matching
 */
export class QuadPattern {
  constructor(
    subject?: string | null,
    predicate?: string | null,
    object?: string | null,
    graph?: string | null
  );

  /**
   * Check if a quad matches this pattern
   */
  matches(quad: Quad): boolean;

  /**
   * Count the number of variables (wildcards) in this pattern
   */
  variable_count(): number;
}

/**
 * RDF/JS DataFactory for creating RDF terms, triples, and quads
 * Implements the RDF/JS specification
 */
export class DataFactory {
  /**
   * Create a named node from an IRI
   * @param iri - The IRI string
   * @returns A named node term
   */
  static named_node(iri: string): RdfTerm;

  /**
   * Create a blank node with optional label
   * @param label - Optional blank node label (auto-generated if omitted)
   * @returns A blank node term
   */
  static blank_node(label?: string): RdfTerm;

  /**
   * Create a literal with optional language tag or datatype
   * @param value - The literal value
   * @param langOrDatatype - Language tag (starts with @) or datatype IRI
   * @returns A literal term
   */
  static literal(value: string, langOrDatatype?: string): RdfTerm;

  /**
   * Create a triple
   * @param subject - Subject IRI
   * @param predicate - Predicate IRI
   * @param object - Object IRI
   * @returns A triple
   */
  static triple(subject: string, predicate: string, object: string): Triple;

  /**
   * Create a quad
   * @param subject - Subject IRI
   * @param predicate - Predicate IRI
   * @param object - Object IRI
   * @param graph - Optional graph IRI (defaults to default graph)
   * @returns A quad
   */
  static quad(subject: string, predicate: string, object: string, graph?: string): Quad;

  /**
   * Create a pattern for matching quads
   * @param subject - Subject IRI or null for wildcard
   * @param predicate - Predicate IRI or null for wildcard
   * @param object - Object IRI or null for wildcard
   * @param graph - Graph IRI or null for wildcard
   * @returns A quad pattern
   */
  static quad_pattern(
    subject?: string | null,
    predicate?: string | null,
    object?: string | null,
    graph?: string | null
  ): QuadPattern;

  /**
   * Get the default graph term
   * @returns The default graph term
   */
  static default_graph(): RdfTerm;

  /**
   * Convert a term to its N-Quads string representation
   * @param term - The term to convert
   * @returns N-Quads string representation
   */
  static term_to_string(term: RdfTerm): string;

  /**
   * Get the IRI of a named node
   * @param term - The term to extract from
   * @returns The IRI string or null if not a named node
   */
  static get_iri(term: RdfTerm): string | null;

  /**
   * Get the value of a literal
   * @param term - The term to extract from
   * @returns The literal value or null if not a literal
   */
  static get_literal_value(term: RdfTerm): string | null;
}

/**
 * In-Memory RDF/Quad Store
 * Provides complete CRUD operations and pattern matching
 * Compatible with Oxigraph Store API
 */
export class Store {
  /**
   * Create a new empty store
   */
  constructor();

  /**
   * Create a store with initial quads
   * @param quads - Initial quads to add
   * @returns A new store with the quads
   */
  static with_quads(quads: Quad[]): Store;

  /**
   * Add a quad to the store
   * @param quad - The quad to add
   * @returns true if the quad was newly added, false if it already existed
   */
  add(quad: Quad): boolean;

  /**
   * Delete a quad from the store
   * @param quad - The quad to delete
   * @returns true if the quad was deleted, false if it didn't exist
   */
  delete(quad: Quad): boolean;

  /**
   * Check if a quad exists in the store
   * @param quad - The quad to check
   * @returns true if the quad is in the store
   */
  has(quad: Quad): boolean;

  /**
   * Find all quads matching a pattern
   * @param pattern - The pattern to match (null/undefined for wildcards)
   * @returns Array of matching quads
   */
  match_quads(pattern: QuadPattern): Quad[];

  /**
   * Get the number of quads in the store
   * @returns The quad count
   */
  size(): number;

  /**
   * Clear all quads from the store
   */
  clear(): void;

  /**
   * Get all quads in the store
   * @returns Array of all quads
   */
  quads(): Quad[];

  /**
   * Get statistics about the store
   * @returns String with store statistics
   */
  get_stats(): string;

  /**
   * Execute a SPARQL SELECT query on the store
   * @param query - SPARQL query string
   * @param options - Query options (baseIRI, etc.)
   * @returns Promise resolving to query results
   */
  query(query: string, options?: QueryOptions): Promise<QueryResult>;

  /**
   * Execute a SPARQL UPDATE query on the store
   * @param query - SPARQL UPDATE string
   * @returns Promise resolving to update result
   */
  update(query: string): Promise<UpdateResult>;

  /**
   * Load RDF data from a string
   * @param data - The RDF data string
   * @param format - The RDF format (turtle, jsonld, trig, etc.)
   * @param options - Load options (baseIRI, defaultGraph, etc.)
   * @returns Promise resolving to load result
   */
  load(data: string, format: string, options?: LoadOptions): Promise<LoadResult>;

  /**
   * Dump store contents to a string
   * @param format - The output format
   * @param options - Dump options
   * @returns Promise resolving to the serialized RDF
   */
  dump(format: string, options?: DumpOptions): Promise<string>;
}

/**
 * Options for SPARQL queries
 */
export interface QueryOptions {
  baseIRI?: string;
  defaultGraph?: string;
  namedGraphs?: string[];
  resultsFormat?: "json" | "xml" | "csv";
  timeout?: number;
}

/**
 * Options for loading RDF data
 */
export interface LoadOptions {
  baseIRI?: string;
  defaultGraph?: string;
  unchecked?: boolean;
}

/**
 * Options for dumping RDF data
 */
export interface DumpOptions {
  format?: string;
  fromNamedGraph?: string;
}

/**
 * Result of a SPARQL query
 */
export interface QueryResult {
  head: { vars: string[] };
  results: { bindings: SparqlBinding[] };
  execution_time_ms: number;
}

/**
 * Result of a SPARQL UPDATE
 */
export interface UpdateResult {
  success: boolean;
  inserts: number;
  deletes: number;
}

/**
 * Result of loading RDF data
 */
export interface LoadResult {
  quads_loaded: number;
  errors: string[];
}

/**
 * Initialize the WASM module (required for some features)
 */
export function init(): Promise<void>;

/**
 * Set panic hook for better error messages in development
 */
export function set_panic_hook(): void;

/**
 * Get the DataFactory singleton
 */
export function get_data_factory(): DataFactory;

/**
 * RDF Format Parser
 * Supports parsing of N-Triples, N-Quads, Turtle, and other RDF formats
 */
export class RdfParser {
  /**
   * Parse N-Quads format
   * @param data - N-Quads data string
   * @returns Array of quads
   */
  static parse_nquads(data: string): Quad[];

  /**
   * Parse N-Triples format
   * @param data - N-Triples data string
   * @returns Array of quads with default graph
   */
  static parse_ntriples(data: string): Quad[];

  /**
   * Parse Turtle format (simplified)
   * @param data - Turtle data string
   * @param baseIRI - Optional base IRI for relative IRIs
   * @returns Array of quads with default graph
   */
  static parse_turtle(data: string, baseIRI?: string): Quad[];

  /**
   * Auto-detect RDF format from content
   * @param data - The RDF data string
   * @returns Format string: "nquads", "turtle", "jsonld", etc.
   */
  static detect_format(data: string): string;

  /**
   * Parse RDF data with optional format specification
   * @param data - The RDF data string
   * @param format - Optional format string (auto-detected if omitted)
   * @returns Array of quads
   */
  static parse(data: string, format?: string): Quad[];
}

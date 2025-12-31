/**
 * QLever WASM Wrapper - JavaScript API Layer
 *
 * This TypeScript module provides the public JavaScript API for QLever WASM,
 * wrapping the low-level libqlever_bindings and implementing the Oxigraph-compatible
 * RDF/JS DataFactory interface.
 */

// Type definitions from index.d.ts
export interface RdfTerm {
  type: 'NamedNode' | 'BlankNode' | 'Literal' | 'DefaultGraph';
  value: string;
  language?: string;
  datatype?: string;
}

export interface Triple {
  subject: RdfTerm;
  predicate: RdfTerm;
  object: RdfTerm;
}

export interface Quad extends Triple {
  graph: RdfTerm;
}

export interface QuadPattern {
  subject?: RdfTerm | null;
  predicate?: RdfTerm | null;
  object?: RdfTerm | null;
  graph?: RdfTerm | null;
}

export interface SparqlBinding {
  [key: string]: {
    type: string;
    value: string;
    'xml:lang'?: string;
    datatype?: string;
  };
}

export interface QueryResult {
  head: { vars: string[] };
  results: { bindings: SparqlBinding[] };
  execution_time_ms?: number;
}

// ============================================================================
// DataFactory Implementation (RDF/JS compliant)
// ============================================================================

/**
 * RDF/JS DataFactory for creating RDF terms
 * Implements the standard RDF/JS specification for term creation
 */
export class DataFactory {
  /**
   * Create a named node (IRI)
   */
  static namedNode(iri: string): RdfTerm {
    return {
      type: 'NamedNode',
      value: iri,
    };
  }

  /**
   * Create a blank node
   */
  static blankNode(label?: string): RdfTerm {
    return {
      type: 'BlankNode',
      value: label || `b_${Math.random().toString(36).substr(2, 9)}`,
    };
  }

  /**
   * Create a literal
   */
  static literal(value: string, languageOrDatatype?: string): RdfTerm {
    const term: RdfTerm = {
      type: 'Literal',
      value,
    };

    if (languageOrDatatype) {
      if (languageOrDatatype.startsWith('@')) {
        term.language = languageOrDatatype.substring(1);
      } else {
        term.datatype = languageOrDatatype;
      }
    }

    return term;
  }

  /**
   * Get the default graph term
   */
  static defaultGraph(): RdfTerm {
    return {
      type: 'DefaultGraph',
      value: '',
    };
  }

  /**
   * Create a triple
   */
  static triple(subject: RdfTerm, predicate: RdfTerm, object: RdfTerm): Triple {
    return { subject, predicate, object };
  }

  /**
   * Create a quad
   */
  static quad(
    subject: RdfTerm,
    predicate: RdfTerm,
    object: RdfTerm,
    graph?: RdfTerm
  ): Quad {
    return {
      subject,
      predicate,
      object,
      graph: graph || DataFactory.defaultGraph(),
    };
  }

  /**
   * Convert term to N-Quads string representation
   */
  static termToString(term: RdfTerm): string {
    switch (term.type) {
      case 'NamedNode':
        return `<${term.value}>`;
      case 'BlankNode':
        return `_:${term.value}`;
      case 'Literal':
        let literal = `"${this.escapeLiteral(term.value)}"`;
        if (term.language) {
          literal += `@${term.language}`;
        } else if (term.datatype) {
          literal += `^^<${term.datatype}>`;
        }
        return literal;
      case 'DefaultGraph':
        return '';
      default:
        return '';
    }
  }

  private static escapeLiteral(value: string): string {
    return value
      .replace(/\\/g, '\\\\')
      .replace(/"/g, '\\"')
      .replace(/\n/g, '\\n')
      .replace(/\r/g, '\\r');
  }
}

// ============================================================================
// QueryBuilder Implementation
// ============================================================================

export class QueryBuilder {
  private queryType: 'SELECT' | 'CONSTRUCT' | 'DESCRIBE' | 'ASK' = 'SELECT';
  private selectVars: string[] = [];
  private constructTemplate: string = '';
  private describeVars: string[] = [];
  private whereClauses: string[] = [];
  private filters: string[] = [];
  private optionals: string[] = [];
  private binds: Array<[string, string]> = [];
  private groupByVars: string[] = [];
  private orderByClauses: Array<[string, boolean]> = [];
  private limit?: number;
  private offset?: number;
  private distinct = false;
  private fromClauses: string[] = [];

  selectQuery(): QueryBuilder {
    this.queryType = 'SELECT';
    return this;
  }

  constructQuery(): QueryBuilder {
    this.queryType = 'CONSTRUCT';
    return this;
  }

  describeQuery(): QueryBuilder {
    this.queryType = 'DESCRIBE';
    return this;
  }

  askQuery(): QueryBuilder {
    this.queryType = 'ASK';
    return this;
  }

  select(...vars: string[]): QueryBuilder {
    this.selectVars = vars;
    return this;
  }

  construct(template: string): QueryBuilder {
    this.constructTemplate = template;
    return this;
  }

  describe(...vars: string[]): QueryBuilder {
    this.describeVars = vars;
    return this;
  }

  where(pattern: string): QueryBuilder {
    this.whereClauses.push(pattern);
    return this;
  }

  filter(condition: string): QueryBuilder {
    this.filters.push(`FILTER (${condition})`);
    return this;
  }

  optional(pattern: string): QueryBuilder {
    this.optionals.push(`OPTIONAL { ${pattern} }`);
    return this;
  }

  bind(expression: string, variable: string): QueryBuilder {
    this.binds.push([expression, variable]);
    return this;
  }

  groupBy(...vars: string[]): QueryBuilder {
    this.groupByVars = vars;
    return this;
  }

  orderBy(...vars: string[]): QueryBuilder {
    this.orderByClauses = vars.map(v => [v, false]);
    return this;
  }

  orderByDesc(...vars: string[]): QueryBuilder {
    this.orderByClauses = vars.map(v => [v, true]);
    return this;
  }

  limit(count: number): QueryBuilder {
    this.limit = count;
    return this;
  }

  offset(count: number): QueryBuilder {
    this.offset = count;
    return this;
  }

  distinct(): QueryBuilder {
    this.distinct = true;
    return this;
  }

  from(...graphs: string[]): QueryBuilder {
    this.fromClauses = graphs.map(g => `FROM <${g}>`);
    return this;
  }

  build(): string {
    let query = '';

    // Build query prefix
    switch (this.queryType) {
      case 'SELECT':
        query += 'SELECT ';
        if (this.distinct) query += 'DISTINCT ';
        query += this.selectVars.join(' ') || '*';
        query += '\n';
        break;
      case 'CONSTRUCT':
        query += 'CONSTRUCT {\n';
        query += this.constructTemplate;
        query += '\n}\n';
        break;
      case 'DESCRIBE':
        query += 'DESCRIBE ';
        query += this.describeVars.join(' ') || '*';
        query += '\n';
        break;
      case 'ASK':
        query += 'ASK\n';
        break;
    }

    // Add FROM clauses
    if (this.fromClauses.length > 0) {
      query += this.fromClauses.join('\n') + '\n';
    }

    // Build WHERE clause
    query += 'WHERE {\n';
    query += '  ' + this.whereClauses.join('\n  ') + '\n';

    // Add optionals
    if (this.optionals.length > 0) {
      query += '  ' + this.optionals.join('\n  ') + '\n';
    }

    // Add filters
    if (this.filters.length > 0) {
      query += '  ' + this.filters.join('\n  ') + '\n';
    }

    // Add binds
    for (const [expr, var_] of this.binds) {
      query += `  BIND (${expr} AS ${var_})\n`;
    }

    query += '}\n';

    // Add GROUP BY
    if (this.groupByVars.length > 0) {
      query += 'GROUP BY ' + this.groupByVars.join(' ') + '\n';
    }

    // Add ORDER BY
    if (this.orderByClauses.length > 0) {
      const orderClauses = this.orderByClauses.map(([v, desc]) =>
        desc ? `DESC(${v})` : v
      );
      query += 'ORDER BY ' + orderClauses.join(' ') + '\n';
    }

    // Add LIMIT
    if (this.limit !== undefined) {
      query += `LIMIT ${this.limit}\n`;
    }

    // Add OFFSET
    if (this.offset !== undefined) {
      query += `OFFSET ${this.offset}\n`;
    }

    return query;
  }
}

// ============================================================================
// Store Implementation (using libqlever WASM backend)
// ============================================================================

/**
 * In-memory RDF Store backed by libqlever
 * Provides SPARQL query execution and RDF data management
 */
export class Store {
  private backend: any;  // QleverStore from libqlever_bindings
  private indexPath?: string;

  constructor() {
    // Backend will be initialized when libqlever WASM module is loaded
    this.backend = null;
  }

  /**
   * Initialize the store with a QLever index
   * @param indexBasename - Path to QLever index files (e.g., "/data/wikidata")
   */
  async init(indexBasename: string): Promise<void> {
    // This will be called after libqlever WASM module is loaded
    // The actual initialization happens in the JavaScript layer
    this.indexPath = indexBasename;

    if (!this.backend) {
      throw new Error('libqlever WASM module not initialized');
    }

    const result = this.backend.init(indexBasename);
    if (!result) {
      throw new Error(this.backend.get_last_error());
    }
  }

  /**
   * Execute a SPARQL query
   * @param sparql - SPARQL query string
   * @returns Query results in SPARQL JSON format
   */
  async query(sparql: string): Promise<QueryResult> {
    if (!this.backend || !this.backend.is_initialized()) {
      throw new Error('Store not initialized');
    }

    const result = this.backend.query(sparql);
    if (typeof result === 'string' && result.startsWith('{"error"')) {
      throw new Error(JSON.parse(result).error);
    }

    return JSON.parse(result);
  }

  /**
   * Add a quad to the store
   * Note: libqlever is read-only for WASM; use SPARQL INSERT for updates
   */
  add(quad: Quad): boolean {
    // For read-only libqlever WASM, this returns false
    // Full SPARQL UPDATE support would require architecture changes
    console.warn('Store.add() not supported with libqlever WASM backend');
    return false;
  }

  /**
   * Delete a quad from the store
   * Note: libqlever is read-only for WASM
   */
  delete(quad: Quad): boolean {
    console.warn('Store.delete() not supported with libqlever WASM backend');
    return false;
  }

  /**
   * Check if a quad exists in the store
   * Uses pattern matching via SPARQL query
   */
  async has(quad: Quad): Promise<boolean> {
    const pattern = `${DataFactory.termToString(quad.subject)} ${DataFactory.termToString(
      quad.predicate
    )} ${DataFactory.termToString(quad.object)}`;
    const sparql = `ASK WHERE { ${pattern} }`;

    try {
      const result = await this.query(sparql);
      return result.boolean || false;
    } catch {
      return false;
    }
  }

  /**
   * Get all quads or match a pattern
   */
  async matchQuads(pattern?: QuadPattern): Promise<Quad[]> {
    // Convert pattern to SPARQL query
    const s = pattern?.subject ? DataFactory.termToString(pattern.subject) : '?s';
    const p = pattern?.predicate ? DataFactory.termToString(pattern.predicate) : '?p';
    const o = pattern?.object ? DataFactory.termToString(pattern.object) : '?o';

    const sparql = `SELECT ?s ?p ?o WHERE { ${s} ${p} ${o} }`;

    const result = await this.query(sparql);
    const quads: Quad[] = [];

    for (const binding of result.results.bindings) {
      quads.push(
        DataFactory.quad(
          this.bindingToTerm(binding['s']),
          this.bindingToTerm(binding['p']),
          this.bindingToTerm(binding['o'])
        )
      );
    }

    return quads;
  }

  /**
   * Get the number of quads in the store
   */
  async size(): Promise<number> {
    const result = await this.query('SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }');
    const bindings = result.results.bindings;
    if (bindings.length > 0 && bindings[0]['count']) {
      return parseInt(bindings[0]['count'].value);
    }
    return 0;
  }

  /**
   * Get store statistics
   */
  async getStats(): Promise<string> {
    if (!this.backend) {
      throw new Error('Store not initialized');
    }
    return this.backend.get_stats();
  }

  private bindingToTerm(binding: any): RdfTerm {
    if (!binding) {
      return DataFactory.defaultGraph();
    }

    switch (binding.type) {
      case 'uri':
      case 'iri':
        return DataFactory.namedNode(binding.value);
      case 'bnode':
        return DataFactory.blankNode(binding.value);
      case 'literal':
        return DataFactory.literal(binding.value, binding['xml:lang'] || binding.datatype);
      default:
        return DataFactory.defaultGraph();
    }
  }
}

// ============================================================================
// WASM Module Initialization
// ============================================================================

let wasmModule: any = null;
let wasmInitialized = false;

/**
 * Initialize the libqlever WASM module
 * Must be called before using Store
 */
export async function initializeWasm(): Promise<void> {
  if (wasmInitialized) {
    return;
  }

  try {
    // Import and initialize the WASM module
    // This will be the output from wasm-bindgen
    wasmModule = await import('./wasm_pkg');
    wasmInitialized = true;
  } catch (error) {
    throw new Error(`Failed to initialize QLever WASM module: ${error}`);
  }
}

/**
 * Create a new Store instance
 * Must call initializeWasm() first
 */
export function createStore(): Store {
  if (!wasmInitialized) {
    throw new Error('Call initializeWasm() before creating a Store');
  }

  const store = new Store();
  store['backend'] = new wasmModule.QleverStore();
  return store;
}

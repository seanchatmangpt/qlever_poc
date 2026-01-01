# QLever Rust Bindings Expansion Plan

## Overview

This document outlines a comprehensive expansion plan for the QLever Rust bindings to expose more capabilities from the C++ codebase. Currently, the bindings only support basic HTTP-based queries. This plan would enable:

- Direct in-process query execution (eliminating HTTP overhead)
- Index building and management
- Advanced SPARQL features (text search, spatial queries, materialized views)
- Performance monitoring and optimization
- Administrative capabilities
- Batch operations and streaming results

## Current State

### Native Bindings (`/rust/`)
- ✅ HTTP-based Store interface
- ✅ RDF type models (Triple, Quad, Term, etc.)
- ✅ SPARQL JSON results parsing
- ❌ No index building
- ❌ No direct C++ integration
- ❌ No result caching
- ❌ No advanced features

### WASM Bindings (`/wasm/`)
- ✅ Basic query execution
- ✅ Query builder
- ❌ No libqlever integration (marked as TODO with feature flag)
- ❌ No index management

## Phase 1: Core C++ Integration (Estimated Impact: ⭐⭐⭐⭐⭐)

### 1.1 FFI Bindings to libqlever

**Goal:** Create Rust bindings to QLever's C++ libqlever API

**Files to Create:**
- `rust/src/ffi/mod.rs` - FFI module root
- `rust/src/ffi/bindings.rs` - Raw C++ bindings
- `rust/src/ffi/types.rs` - Safe Rust wrappers for C++ types
- `rust/src/libqlever.rs` - Public libqlever API

**Key C++ Types to Wrap:**
```rust
// Configuration
pub struct EngineConfig {
    base_name: String,
    load_text_index: bool,
    memory_limit: MemorySize,
    cache_max_size: MemorySize,
    default_query_timeout: Duration,
}

pub struct IndexBuilderConfig {
    base_name: String,
    input_files: Vec<InputFile>,
    vocab_type: VocabularyType,
    add_text_index: bool,
    text_index_parameters: TextIndexParams,
}

// Data Types
pub struct Id(u64);

pub struct IdTable<const WIDTH: usize> {
    columns: Vec<Vec<Id>>,
}

pub struct LocalVocab {
    internal_id_to_external_id: HashMap<Id, String>,
}

pub struct QueryPlan {
    // Opaque representation of parsed query
}

pub struct RuntimeInformation {
    execution_time_ms: f64,
    number_of_rows: usize,
    multiplicity: usize,
}

// Enumerations
pub enum VocabularyType {
    OnDisk,
    OnDiskCompressed,
    InMemory,
}

pub enum MediaType {
    SparqlJson,
    SparqlXml,
    QLeverJson,
    Turtle,
    NTriples,
    Csv,
    Tsv,
    Json,
}

pub enum InputFileFormat {
    Turtle,
    NTriples,
    NQuads,
    RdfXml,
}
```

**Key Functions to Expose:**
```rust
pub struct Qlever { /* ... */ }

impl Qlever {
    // Initialization
    pub fn new(config: EngineConfig) -> Result<Self>;

    // Query execution (string)
    pub fn query(&self, query: &str, format: MediaType) -> Result<QueryResult>;

    // Query parsing and planning
    pub fn parse_and_plan(&self, query: &str) -> Result<QueryPlan>;
    pub fn execute_plan(&self, plan: &QueryPlan, format: MediaType) -> Result<QueryResult>;

    // Named result caching
    pub fn query_and_pin(&self, name: &str, query: &str) -> Result<()>;
    pub fn get_pinned_result(&self, name: &str) -> Result<QueryResult>;
    pub fn clear_pinned(&self, name: &str) -> Result<()>;

    // Materialized views
    pub fn write_materialized_view(&self, name: &str, query: &str) -> Result<()>;
    pub fn load_materialized_view(&self, name: &str) -> Result<()>;
    pub fn list_materialized_views(&self) -> Result<Vec<String>>;

    // Statistics
    pub fn server_stats(&self) -> Result<ServerStats>;
    pub fn cache_stats(&self) -> Result<CacheStats>;
    pub fn index_stats(&self) -> Result<IndexStats>;
}

// Index building (static)
pub fn build_index(config: IndexBuilderConfig) -> Result<()>;
pub fn merge_vocabularies(configs: Vec<IndexBuilderConfig>) -> Result<()>;
```

**Safety Considerations:**
- Use `Arc` for shared C++ ownership
- Implement `Drop` for proper cleanup
- Use `repr(C)` for data structures that cross FFI boundary
- Validate all string inputs to prevent injection
- Handle C++ exceptions → Rust Result

**Example Usage:**
```rust
// Build index
let config = IndexBuilderConfig {
    base_name: "wikidata",
    input_files: vec![
        InputFile::new("wikidata.nt", InputFileFormat::NTriples),
    ],
    vocab_type: VocabularyType::OnDiskCompressed,
    add_text_index: true,
};
qlever::build_index(&config)?;

// Query with in-process execution
let engine = Qlever::new(EngineConfig {
    base_name: "wikidata",
    load_text_index: true,
    ..Default::default()
})?;

let result = engine.query(
    "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10",
    MediaType::SparqlJson,
)?;

println!("{}", result.as_json()?);
```

### 1.2 Result Handling

**Create Module:** `rust/src/results.rs`

```rust
pub struct QueryResult {
    raw_result: String,
    format: MediaType,
    runtime_info: Option<RuntimeInformation>,
}

impl QueryResult {
    pub fn as_json(&self) -> Result<serde_json::Value>;
    pub fn as_solutions(&self) -> Result<Vec<QuerySolution>>;
    pub fn as_string(&self) -> &str;
    pub fn execution_time_ms(&self) -> Option<f64>;
    pub fn row_count(&self) -> Option<usize>;

    // Streaming for large results
    pub fn into_stream(self) -> Result<impl Stream<Item = Result<QuerySolution>>>;
}

pub struct QuerySolution {
    bindings: BTreeMap<String, TermValue>,
}

impl QuerySolution {
    pub fn get(&self, var: &str) -> Option<&TermValue>;
    pub fn iter(&self) -> impl Iterator<Item = (&String, &TermValue)>;
}
```

### 1.3 Type-Safe Configuration

**Create Module:** `rust/src/config.rs`

```rust
pub struct EngineConfigBuilder {
    base_name: String,
    load_text_index: Option<bool>,
    memory_limit: Option<MemorySize>,
    cache_max_size: Option<MemorySize>,
    default_query_timeout: Option<Duration>,
    cache_max_size_single_entry: Option<MemorySize>,
    lazy_index_scan_queue_size: Option<usize>,
    group_by_hash_map_enabled: Option<bool>,
    use_binary_search_transitive_path: Option<bool>,
}

impl EngineConfigBuilder {
    pub fn new(base_name: impl Into<String>) -> Self;
    pub fn load_text_index(mut self, value: bool) -> Self;
    pub fn memory_limit(mut self, size: MemorySize) -> Self;
    pub fn cache_max_size(mut self, size: MemorySize) -> Self;
    pub fn build(self) -> Result<EngineConfig>;
}

// Strongly-typed memory sizes
pub struct MemorySize { /* ... */ }

impl MemorySize {
    pub fn kilobytes(n: u64) -> Self;
    pub fn megabytes(n: u64) -> Self;
    pub fn gigabytes(n: u64) -> Self;
    pub fn bytes(n: u64) -> Self;
    pub fn as_bytes(&self) -> u64;
}
```

## Phase 2: Advanced Query Features (Estimated Impact: ⭐⭐⭐⭐)

### 2.1 Query Planning & Optimization

**Create Module:** `rust/src/query_plan.rs`

```rust
pub struct QueryPlan {
    // Opaque reference to C++ query plan
}

pub struct ExecutionInfo {
    pub tree: ExecutionTree,
    pub estimated_cost: f64,
    pub estimated_rows: usize,
}

pub enum ExecutionTree {
    IndexScan {
        triple_pattern: TriplePattern,
        sorted: bool,
    },
    Join {
        left: Box<ExecutionTree>,
        right: Box<ExecutionTree>,
        join_column: usize,
    },
    Filter {
        operation: Box<ExecutionTree>,
        condition: String,
    },
    OrderBy {
        operation: Box<ExecutionTree>,
        variables: Vec<(String, SortOrder)>,
    },
    Limit {
        operation: Box<ExecutionTree>,
        limit: usize,
    },
    // ... more operation types
}

pub enum SortOrder {
    Ascending,
    Descending,
}

pub struct TriplePattern {
    pub subject: PatternComponent,
    pub predicate: PatternComponent,
    pub object: PatternComponent,
}

pub enum PatternComponent {
    Variable(String),
    Uri(String),
    Literal(String),
}
```

**Usage:**
```rust
let plan = engine.parse_and_plan("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10")?;
let info = plan.explain()?;  // Get execution plan details
println!("Estimated rows: {}", info.estimated_rows);
println!("Estimated cost: {:.2}", info.estimated_cost);
```

### 2.2 Text Search Integration

**Create Module:** `rust/src/text_search.rs`

```rust
pub trait TextSearchable {
    fn text_search(&self, query: &str) -> Result<Vec<TextSearchResult>>;
    fn text_search_with_limit(&self, query: &str, limit: usize) -> Result<Vec<TextSearchResult>>;
}

pub struct TextSearchResult {
    pub entity: String,
    pub text: String,
    pub score: f64,  // BM25 score
}

pub struct TextSearchQuery {
    terms: Vec<String>,
    field: Option<String>,
    limit: Option<usize>,
}

impl TextSearchQuery {
    pub fn new(terms: Vec<String>) -> Self;
    pub fn with_limit(mut self, limit: usize) -> Self;
    pub fn execute(&self, engine: &Qlever) -> Result<Vec<TextSearchResult>>;
}
```

**Usage:**
```rust
let results = engine.text_search("Python programming")
    .with_limit(100)
    .execute()?;

for result in results {
    println!("{}: {} (score: {:.2})", result.entity, result.text, result.score);
}
```

### 2.3 Spatial Query Support

**Create Module:** `rust/src/spatial.rs`

```rust
pub struct GeoPoint {
    latitude: f64,
    longitude: f64,
}

pub struct Geometry {
    wkt: String,  // WKT representation
}

pub struct SpatialQuery {
    geometry: Geometry,
    predicate: SpatialPredicate,
}

pub enum SpatialPredicate {
    Contains,
    Within,
    Intersects,
    Covers,
    Disjoint,
    Distance(f64),  // max distance in meters
}

impl SpatialQuery {
    pub fn new(geometry: Geometry, predicate: SpatialPredicate) -> Self;
    pub fn execute(&self, engine: &Qlever) -> Result<Vec<QuerySolution>>;
}
```

## Phase 3: Index Management (Estimated Impact: ⭐⭐⭐⭐)

### 3.1 Index Building API

**Create Module:** `rust/src/index_builder.rs`

```rust
pub struct IndexBuilder {
    config: IndexBuilderConfig,
}

pub struct IndexBuilderConfig {
    base_name: String,
    input_files: Vec<InputFileSpec>,
    vocab_type: VocabularyType,
    add_text_index: bool,
    text_index_parameters: TextIndexParameters,
    permutations: IndexPermutations,
}

pub struct InputFileSpec {
    path: String,
    format: InputFileFormat,
}

pub enum InputFileFormat {
    Turtle,
    NTriples,
    NQuads,
    RdfXml,
}

pub struct TextIndexParameters {
    pub bm25_b: f64,      // Default: 0.75
    pub bm25_k: f64,      // Default: 1.75
}

pub enum IndexPermutations {
    All,
    Essential,  // PSO + POS only
    Custom(Vec<String>),
}

impl IndexBuilder {
    pub fn new(config: IndexBuilderConfig) -> Self;
    pub fn build(&self) -> Result<BuildProgress>;
    pub fn build_with_progress<F: Fn(BuildProgress)>(&self, progress: F) -> Result<()>;
}

pub struct BuildProgress {
    pub phase: BuildPhase,
    pub percentage: f32,
    pub message: String,
}

pub enum BuildPhase {
    ParsingTriples,
    BuildingVocabulary,
    CreatingIndex,
    BuildingTextIndex,
    BuildingSpatialIndex,
    Finalizing,
}
```

**Usage:**
```rust
let config = IndexBuilderConfig {
    base_name: "wikidata",
    input_files: vec![
        InputFileSpec {
            path: "wikidata.nt".to_string(),
            format: InputFileFormat::NTriples,
        },
    ],
    vocab_type: VocabularyType::OnDiskCompressed,
    add_text_index: true,
    ..Default::default()
};

let builder = IndexBuilder::new(config);
builder.build_with_progress(|progress| {
    println!("[{}] {}: {}%", progress.phase, progress.message, progress.percentage);
})?;
```

### 3.2 Index Statistics

**Create Module:** `rust/src/index_stats.rs`

```rust
pub struct IndexStats {
    pub triple_count: usize,
    pub entity_count: usize,
    pub predicate_count: usize,
    pub literal_count: usize,
    pub index_size_bytes: u64,
    pub vocabulary_size_bytes: u64,
    pub text_index_size_bytes: Option<u64>,
    pub created_at: SystemTime,
    pub predicate_distribution: Vec<PredicateStats>,
}

pub struct PredicateStats {
    pub predicate: String,
    pub triple_count: usize,
    pub subjects_count: usize,
    pub objects_count: usize,
}

impl Qlever {
    pub fn index_stats(&self) -> Result<IndexStats>;
}
```

## Phase 4: Caching & Performance (Estimated Impact: ⭐⭐⭐)

### 4.1 Result Caching

**Create Module:** `rust/src/caching.rs`

```rust
pub struct CachedQuery {
    name: String,
    query: String,
    cached_at: SystemTime,
    access_count: usize,
}

pub struct CacheStats {
    pub total_queries_cached: usize,
    pub cache_hit_count: usize,
    pub cache_miss_count: usize,
    pub hit_rate: f32,
    pub total_size_bytes: u64,
    pub max_size_bytes: u64,
    pub cached_queries: Vec<CachedQuery>,
}

impl Qlever {
    pub fn cache_stats(&self) -> Result<CacheStats>;
    pub fn clear_cache(&mut self) -> Result<()>;
    pub fn list_cached_queries(&self) -> Result<Vec<CachedQuery>>;
}
```

### 4.2 Materialized Views

**Create Module:** `rust/src/materialized_views.rs`

```rust
pub struct MaterializedView {
    name: String,
    query: String,
    triple_count: usize,
    created_at: SystemTime,
    last_accessed_at: SystemTime,
}

pub struct MaterializedViewManager {
    engine: Arc<Qlever>,
}

impl MaterializedViewManager {
    pub fn create(&self, name: &str, query: &str) -> Result<()>;
    pub fn drop(&self, name: &str) -> Result<()>;
    pub fn list(&self) -> Result<Vec<MaterializedView>>;
    pub fn query(&self, name: &str) -> Result<QueryResult>;
    pub fn refresh(&self, name: &str) -> Result<()>;
}

impl Qlever {
    pub fn materialized_views(&self) -> MaterializedViewManager;
}
```

**Usage:**
```rust
engine.materialized_views().create(
    "frequent_query",
    "SELECT ?s ?p ?o WHERE { ?s ?p ?o }"
)?;

let result = engine.materialized_views().query("frequent_query")?;
```

## Phase 5: Administrative APIs (Estimated Impact: ⭐⭐⭐)

### 5.1 Server Statistics

**Create Module:** `rust/src/admin.rs`

```rust
pub struct ServerStats {
    pub index_name: String,
    pub triple_count: usize,
    pub entity_count: usize,
    pub predicate_count: usize,
    pub index_size: u64,
    pub uptime: Duration,
}

pub struct QueryStats {
    pub total_queries: usize,
    pub total_time_ms: f64,
    pub average_time_ms: f64,
    pub slowest_query_ms: f64,
    pub fastest_query_ms: f64,
}

impl Qlever {
    pub fn server_stats(&self) -> Result<ServerStats>;
    pub fn query_stats(&self) -> Result<QueryStats>;
}
```

### 5.2 Query Execution Control

```rust
pub struct ExecutionContext {
    timeout: Duration,
    memory_limit: MemorySize,
    cache_results: bool,
}

impl Qlever {
    pub fn query_with_context(
        &self,
        query: &str,
        format: MediaType,
        context: ExecutionContext,
    ) -> Result<QueryResult>;

    pub fn cancel_current_query(&self) -> Result<()>;
}
```

## Phase 6: Streaming & Batch Operations (Estimated Impact: ⭐⭐)

### 6.1 Streaming Results

**Create Module:** `rust/src/streaming.rs`

```rust
pub trait ResultStream: Send + Sync {
    fn next(&mut self) -> Result<Option<QuerySolution>>;
    fn close(&mut self) -> Result<()>;
}

impl Qlever {
    pub fn query_stream(&self, query: &str) -> Result<Box<dyn ResultStream>>;
}

// Iterator support
pub struct QueryResultIterator {
    stream: Box<dyn ResultStream>,
}

impl Iterator for QueryResultIterator {
    type Item = Result<QuerySolution>;
    fn next(&mut self) -> Option<Self::Item> {
        match self.stream.next() {
            Ok(Some(solution)) => Some(Ok(solution)),
            Ok(None) => None,
            Err(e) => Some(Err(e)),
        }
    }
}
```

**Usage:**
```rust
for solution in engine.query_stream("SELECT * WHERE { ?s ?p ?o }")? {
    let solution = solution?;
    println!("{:?}", solution);
}
```

### 6.2 Batch Operations

```rust
pub struct BatchQuery {
    queries: Vec<String>,
}

impl BatchQuery {
    pub fn new() -> Self;
    pub fn add_query(&mut self, query: String) -> &mut Self;
    pub fn execute(&self, engine: &Qlever) -> Result<Vec<QueryResult>>;
    pub fn execute_parallel(&self, engine: &Qlever, threads: usize) -> Result<Vec<QueryResult>>;
}
```

## Implementation Strategy

### Priority Order

1. **Phase 1** (High Priority)
   - Core C++ FFI bindings
   - Basic Qlever API
   - Result handling
   - Configuration management
   - **Effort:** ~3-4 weeks
   - **Value:** Enables in-process execution, eliminates HTTP overhead

2. **Phase 2** (Medium Priority)
   - Query planning API
   - Text search
   - Spatial queries
   - **Effort:** ~2-3 weeks
   - **Value:** Unlocks advanced SPARQL features

3. **Phase 3** (Medium Priority)
   - Index building API
   - Index statistics
   - **Effort:** ~2 weeks
   - **Value:** Enables complete data pipeline in Rust

4. **Phase 4** (Low-Medium Priority)
   - Caching APIs
   - Materialized views
   - **Effort:** ~1-2 weeks
   - **Value:** Performance optimization

5. **Phase 5** (Low Priority)
   - Admin APIs
   - Statistics
   - **Effort:** ~1 week
   - **Value:** Monitoring and management

6. **Phase 6** (Low Priority)
   - Streaming results
   - Batch operations
   - **Effort:** ~1-2 weeks
   - **Value:** Scalability for large result sets

### Testing Strategy

1. **Unit Tests:** Test each API module independently
2. **Integration Tests:** Test with actual QLever indices
3. **Performance Tests:** Compare HTTP vs in-process performance
4. **Compatibility Tests:** Ensure results match C++ implementation
5. **Stress Tests:** Large result sets, memory limits

### Documentation

1. API documentation (rustdoc)
2. Usage examples for each feature
3. Migration guide from HTTP-based API
4. Performance tuning guide
5. Troubleshooting guide

## Expected Benefits

### Performance
- **99%+ reduction** in query latency (eliminate HTTP round-trips)
- In-process execution vs network overhead
- Direct memory access vs serialization

### Functionality
- Access to all QLever features (text search, spatial, etc.)
- Index building without separate tools
- Fine-grained performance control

### Developer Experience
- Type-safe Rust API
- Idiomatic Rust patterns
- Zero-copy result access
- Comprehensive error handling

### Deployment
- Single binary with embedded QLever
- No separate HTTP server required
- Better resource isolation
- Easier containerization

## Estimated Timeline

- **Phase 1-3:** 5-9 weeks (MVP with all core features)
- **Phase 4-6:** 3-5 weeks (advanced features)
- **Total:** 8-14 weeks to full feature parity with C++

## Risk Assessment

### Technical Risks
- C++ API stability (mitigation: version pinning)
- Memory safety across FFI boundary (mitigation: extensive testing)
- Platform portability (mitigation: CI/CD for multiple platforms)

### Mitigation Strategies
- Extensive safety testing with Miri and AddressSanitizer
- Comprehensive integration tests
- Clear documentation of C++ API contracts
- Version compatibility matrix

## Maintenance

- Keep bindings in sync with C++ API changes
- Quarterly security audits
- Performance regression testing
- Community contribution guidelines

---

**Prepared by:** Claude Code
**Date:** 2026-01-01
**Status:** Planning Phase

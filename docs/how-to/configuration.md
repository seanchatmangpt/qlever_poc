# How to: Configure QLever

Configure QLever using the `Qleverfile` with the **Python CLI tool** (`pip install qlever`).

> **Note:** For developers building from source, see [JSON Settings Reference](../reference/configuration.md) instead.

## What is a Qleverfile?

A YAML configuration file (`Qleverfile`) controls the Python `qlever` CLI tool:
- What data to index
- How to optimize the index
- Server settings (port, memory, etc.)
- Advanced features (text search, spatial, etc.)

## Minimal Configuration

Simplest possible setup:

```yaml
data:
  input_files: data.ttl

index:
  name: my-index

server:
  port: 7023
```

This works, but let's make it production-ready.

## Complete Configuration Template

```yaml
# ============================================
# DATA
# ============================================
data:
  input_files:
    - data1.ttl
    - data2.ttl.gz
  format: ttl  # ttl, nt, nq, rdf, jsonld

# ============================================
# INDEX
# ============================================
index:
  # Basic settings
  name: my-knowledge-graph
  description: "Production knowledge graph for XYZ"

  # Memory and performance
  memory_limit: 16GB
  use_compression: true

  # Index permutations (optimize for query patterns)
  permutations: [SPO, PSO]

  # Text search (optional)
  text_search:
    enabled: true
    predicates:
      - rdfs:label
      - rdfs:comment

  # Spatial queries (optional)
  spatial_search:
    enabled: false
    location_predicates:
      - geo:asWKT

# ============================================
# SERVER
# ============================================
server:
  port: 7023
  host: 0.0.0.0  # Listen on all interfaces

  # Memory for query execution
  memory_limit: 32GB

  # Maximum queries to run in parallel
  max_concurrent_queries: 4

  # Query timeout (in seconds)
  query_timeout: 300

  # Keep server running? (useful for development)
  background: true

# ============================================
# ADVANCED
# ============================================
advanced:
  # Logging level: DEBUG, INFO, WARN, ERROR
  log_level: INFO

  # Number of indexing threads
  num_threads: 8

  # Cache settings
  cache:
    enabled: true
    size: 4GB
```

## Common Scenarios

### Small Dataset (< 1M triples)

```yaml
data:
  input_files: small_data.ttl

index:
  name: small-index
  memory_limit: 2GB

server:
  port: 7023
  memory_limit: 4GB
```

**Build time:** < 1 minute
**Disk space:** ~200 MB
**Query time:** < 50ms

### Medium Dataset (1M - 100M triples)

```yaml
data:
  input_files: medium_data.ttl.gz

index:
  name: medium-index
  memory_limit: 8GB
  use_compression: true
  permutations: [SPO, PSO]

  text_search:
    enabled: true
    predicates:
      - rdfs:label

server:
  port: 7023
  memory_limit: 16GB
```

**Build time:** 5-15 minutes
**Disk space:** 2-5 GB
**Query time:** < 100ms

### Large Dataset (> 100M triples)

```yaml
data:
  input_files:
    - data_part1.ttl.gz
    - data_part2.ttl.gz

index:
  name: large-index
  memory_limit: 32GB
  use_compression: true
  permutations: [SPO, PSO, OSP]

  text_search:
    enabled: true
    predicates:
      - rdfs:label
      - rdfs:comment
      - skos:altLabel

  spatial_search:
    enabled: true
    location_predicates:
      - geo:asWKT

server:
  port: 7023
  memory_limit: 64GB
  max_concurrent_queries: 8

advanced:
  num_threads: 16
```

**Build time:** 30 minutes - 2 hours
**Disk space:** 10GB - 100GB
**Query time:** < 200ms

### Development Setup (Optimization = Speed, Quality)

```yaml
data:
  input_files: dev_data.ttl

index:
  name: dev-index
  memory_limit: 2GB
  # No compression for faster builds
  use_compression: false

server:
  port: 7023
  memory_limit: 4GB
  query_timeout: 60  # Shorter timeout during development

advanced:
  log_level: DEBUG
  num_threads: 4
```

## Configuration Options Reference

### Data Section

| Option | Values | Default | Notes |
|--------|--------|---------|-------|
| `input_files` | Path(s) to RDF files | — | Required. Supports .ttl, .nt, .nq, .rdf, .jsonld, .gz, .bz2 |
| `format` | ttl, nt, nq, rdf, jsonld | Auto-detect | Explicit format if auto-detect fails |

### Index Section

| Option | Values | Default | Notes |
|--------|--------|---------|-------|
| `name` | Any string | "index" | Used in file naming |
| `memory_limit` | Size string (2GB, 500MB) | 8GB | Max memory during indexing |
| `use_compression` | true/false | false | Save space, slightly slower queries |
| `permutations` | [SPO, PSO, OSP] | [SPO] | Trade memory for query speed |

### Server Section

| Option | Values | Default | Notes |
|--------|--------|---------|-------|
| `port` | 1-65535 | 7023 | HTTP server port |
| `host` | IP address | localhost | 0.0.0.0 for all interfaces |
| `memory_limit` | Size string | 16GB | Max memory for query execution |
| `max_concurrent_queries` | Number | 4 | Queries to run in parallel |
| `query_timeout` | Seconds | 300 | Max query execution time |

## Changing Configuration

### After Creating a Qleverfile

```bash
# Use the configuration
qlever index
qlever start
```

### Modifying an Existing Configuration

```bash
# Edit Qleverfile
vi Qleverfile

# Most changes require rebuilding the index
qlever index

# Then restart
qlever stop
qlever start
```

### Command-Line Overrides

Override Qleverfile settings from command line:

```bash
# Use different port
qlever start --port 8000

# Use different memory
qlever start --memory 32GB

# Explicit log level
qlever start --log-level DEBUG
```

## Memory Sizing Guide

**How to allocate memory properly:**

1. **System memory:** Check what you have
   ```bash
   free -h  # Linux
   vm_stat  # macOS
   ```

2. **Index memory:** For indexing process
   - Rough estimate: 2x the RDF file size
   - Example: 500MB RDF file → use 1GB index memory

3. **Server memory:** For running queries
   - Start with 2x index memory
   - Larger = can handle more concurrent queries
   - Smaller = fewer queries run in parallel

4. **System buffer:** Keep ~25% free
   - Example: 32GB system → allocate max 24GB total

**Example calculation:**
```
System memory: 64GB
Index memory: 32GB (for building)
Server memory: 24GB (for queries)
Keep free: ~8GB (system buffer)
```

## Troubleshooting Configuration

**"Out of memory" during indexing**
→ Increase `index.memory_limit` or reduce data

**"Out of memory" during queries**
→ Increase `server.memory_limit` or reduce `max_concurrent_queries`

**Index build is very slow**
→ Increase `advanced.num_threads` or reduce data

**Queries are slow**
→ Add more permutations (`[SPO, PSO, OSP]`)

**Disk space is too large**
→ Enable `use_compression: true`

## Next Steps

- **Optimize Performance?** [How-to: Performance](./performance.md)
- **Enable Text Search?** [How-to: Text Search](./text-search.md)
- **Add Spatial Queries?** [How-to: Spatial Queries](./spatial-queries.md)

---

**Pro Tip:** Start with the template for your dataset size, then tweak based on actual performance.

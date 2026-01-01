# CLI Reference

Complete reference for the `qlever` command-line interface.

Use `qlever --help` for command overview or `qlever <command> --help` for command details.

## Global Options

Available with all commands:

```bash
qlever --help              # Show help
qlever --version           # Show version
qlever --config FILE       # Use specific Qleverfile
```

## Commands

### setup-config

Download a pre-configured `Qleverfile` for a known dataset.

```bash
qlever setup-config <config-name>
```

**Available configs:**
- `wikidata-small` — Wikidata sample (~1M triples)
- `wikidata-full` — Full Wikidata (recommended for advanced users)
- `dbpedia` — DBpedia dataset
- `dblp` — DBLP computer science publications
- `openstreetmap` — Geographic data
- `uniprot` — Protein database

**Example:**
```bash
qlever setup-config wikidata-small
# Creates Qleverfile configured for Wikidata sample
```

### index

Build an index from RDF data.

```bash
qlever index [OPTIONS]
```

**Options:**
- `--config FILE` — Use specific Qleverfile
- `--show` — Show command that will be executed, don't run it
- `--force` — Force rebuild, overwriting existing index

**Example:**
```bash
# Build index using Qleverfile
qlever index

# Show what command will run
qlever index --show

# Rebuild from scratch
qlever index --force
```

### start

Start the QLever server.

```bash
qlever start [OPTIONS]
```

**Options:**
- `--port PORT` — Use specific port (default: 7023)
- `--memory SIZE` — Max memory (default: 16GB)
  - Examples: `8GB`, `4096MB`, `1024MB`
- `--docker` — Run in Docker container
- `--background` — Run in background
- `--show` — Show command, don't run it
- `--host ADDRESS` — Bind to specific address (default: localhost)

**Example:**
```bash
# Start server on default port 7023
qlever start

# Start on custom port with more memory
qlever start --port 8000 --memory 32GB

# Run in background
qlever start --background

# In Docker
qlever start --docker
```

### query

Execute a SPARQL query.

```bash
qlever query [OPTIONS] "SPARQL QUERY"
```

**Options:**
- `--show` — Show command, don't run it
- `--format FORMAT` — Output format: json, csv, tsv, xml
- `--output FILE` — Save results to file
- `--show-timing` — Show query execution time

**Example:**
```bash
# Simple query
qlever query "SELECT ?x WHERE { ?x a ?type } LIMIT 10"

# Show timing information
qlever query --show-timing "SELECT ?x WHERE { ?x a ?type }"

# Save results to file
qlever query --output results.json "SELECT ?x WHERE { ?x a ?type }"

# Different output format
qlever query --format csv "SELECT ?x WHERE { ?x a ?type }"
```

### stop

Stop the running QLever server.

```bash
qlever stop
```

Gracefully shuts down the server. Any running queries are completed first.

### status

Check server status and statistics.

```bash
qlever status
```

**Output includes:**
- Server running? (yes/no)
- Memory usage (current / limit)
- Queries running (count)
- Index information

**Example output:**
```
Status: Running
Memory: 8.2 GB / 32 GB (25.6%)
Queries: 1 running
Index: my-index (45.3M triples)
```

### index-info

Show information about the current index.

```bash
qlever index-info
```

**Output includes:**
- Number of triples
- Number of unique subjects/predicates/objects
- Index size
- Permutations available
- Text search enabled?
- Spatial search enabled?

### logs

Show recent server logs.

```bash
qlever logs [OPTIONS]
```

**Options:**
- `--follow` — Follow logs in real-time
- `--lines N` — Show last N lines (default: 50)

**Example:**
```bash
# Show last 50 lines
qlever logs

# Follow logs live
qlever logs --follow

# Show last 100 lines
qlever logs --lines 100
```

### restart

Stop and start the server.

```bash
qlever restart [OPTIONS]
```

Same options as `start`.

### export

Export indexed data to RDF format.

```bash
qlever export [OPTIONS] --output FILE
```

**Options:**
- `--format FORMAT` — Output format: ttl, nt, nq
- `--output FILE` — Output file (required)

**Example:**
```bash
# Export to N-Triples
qlever export --format nt --output dump.nt

# Export to Turtle
qlever export --format ttl --output dump.ttl
```

### validate-data

Check RDF data for syntax errors.

```bash
qlever validate-data [OPTIONS] FILE
```

**Options:**
- `--format FORMAT` — Explicit format (auto-detected by default)
- `--strict` — Strict validation (fail on warnings)

**Example:**
```bash
# Validate RDF file
qlever validate-data data.ttl

# Validate with strict rules
qlever validate-data --strict data.ttl
```

### compact

Optimize index for better performance.

```bash
qlever compact
```

Rebuilds internal data structures for faster queries. Run periodically on large indexes.

## Common Workflows

### First-Time Setup

```bash
# 1. Create config
qlever setup-config wikidata-small

# 2. Build index
qlever index

# 3. Start server
qlever start

# 4. Query
qlever query "SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

### Production Deployment

```bash
# 1. Setup with custom data
qlever setup-config custom
# Edit Qleverfile as needed

# 2. Build and validate
qlever validate-data data.ttl
qlever index

# 3. Start with sufficient memory
qlever start --memory 64GB --port 7023

# 4. Check status
qlever status

# 5. Follow logs
qlever logs --follow
```

### Development Iteration

```bash
# Make changes to Qleverfile
vi Qleverfile

# Rebuild index
qlever index --force

# Restart server
qlever restart --memory 8GB

# Test query
qlever query --show-timing "SELECT ..."
```

## Output Formats

### JSON (default)

```bash
qlever query --format json "SELECT ?x ?y WHERE { ... }"
# Returns: {"results": [{"x": "...", "y": "..."}]}
```

### CSV

```bash
qlever query --format csv "SELECT ?x ?y WHERE { ... }"
# Returns: x,y
#          value1,value2
```

### TSV (Tab-separated)

```bash
qlever query --format tsv "SELECT ?x ?y WHERE { ... }"
# Same as CSV but tab-separated
```

### XML

```bash
qlever query --format xml "SELECT ?x ?y WHERE { ... }"
# Returns SPARQL-compliant XML
```

## Tips & Tricks

**Measure query performance:**
```bash
qlever query --show-timing "SELECT ..."
```

**Dry-run (see command without executing):**
```bash
qlever index --show
qlever start --show
qlever query --show "SELECT ..."
```

**Use environment variables:**
```bash
export QLEVER_PORT=8000
export QLEVER_MEMORY=32GB
qlever start
```

**Check exact command being run:**
```bash
qlever query --show "SELECT ?x WHERE { ?x a ?type }"
# Shows the exact curl command
```

---

For more details, run `qlever <command> --help` or see [How-to Guides](../how-to/).

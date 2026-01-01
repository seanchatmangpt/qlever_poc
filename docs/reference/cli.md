# C++ Binary Reference

Reference for `IndexBuilderMain` and `ServerMain` — the compiled C++ binaries used to build indexes and run queries.

> **Note:** If using the Python `qlever` CLI tool from `qlever-control`, refer to that repository's documentation instead. These docs are for developers building from source.

## IndexBuilderMain

Build RDF indexes from raw data.

### Usage

```bash
IndexBuilderMain [OPTIONS]
```

### Required Options

| Option | Short | Argument | Purpose |
|--------|-------|----------|---------|
| `--index-basename` | `-i` | STRING | Output index file basename (e.g., "wikidata") |
| `--kg-input-file` | `-f` | FILE | Input RDF file (use `-` for stdin) |
| `--file-format` | `-F` | FORMAT | Input format: `ttl`, `nt`, `nq`, `rdf`, `jsonld` |

### Optional Options

| Option | Short | Argument | Purpose |
|--------|-------|----------|---------|
| `--settings-file` | `-s` | FILE | JSON settings file for parsing options |
| `--text-docs-input-file` | `-d` | FILE | Text search: documents input |
| `--text-words-input-file` | `-w` | FILE | Text search: words/scores input |
| `--text-words-from-literals` | `-W` | (flag) | Include RDF literals in text index |
| `--default-graph` | `-g` | IRI | Default graph IRI for triples |
| `--parse-parallel` | `-p` | (flag) | Enable parallel RDF parsing |
| `--vocabulary-type` | — | TYPE | Vocabulary implementation (auto-selected) |
| `--bm25-b` | — | FLOAT | BM25 parameter b (0-1, default 0.75) |
| `--bm25-k` | — | FLOAT | BM25 parameter k1 (>=0, default 1.2) |

### Examples

**Simple Turtle indexing:**
```bash
IndexBuilderMain -F ttl -f data.ttl -i my-index
```

**From stdin with settings:**
```bash
cat data.nt | IndexBuilderMain -F nt -f - -i my-index -s settings.json
```

**With parallel parsing:**
```bash
IndexBuilderMain -F ttl -f large.ttl -i big-index -p -s settings.json
```

**With text indexing:**
```bash
IndexBuilderMain -F ttl -f data.ttl -i my-index \
  -d text-docs.txt -w text-words.txt -W
```

### Settings File (JSON)

Create a `settings.json` file for parsing options:

```json
{
  "ascii-prefixes-only": false,
  "num-triples-per-batch": 50000000,
  "parser-batch-size": 1000,
  "parallel-parsing": true,
  "languages-internal": ["en"],
  "prefixes-external": [
    "<http://www.wikidata.org/entity/statement>"
  ],
  "locale": {
    "language": "en",
    "country": "US",
    "ignore-punctuation": true
  }
}
```

**Key Options:**
- `num-triples-per-batch` — Batching during parsing (larger = faster, more memory)
- `parallel-parsing` — Use multiple threads for parsing
- `languages-internal` — Languages to optimize for (storage efficiency)
- `ascii-prefixes-only` — Only ASCII IRIs (memory optimization)

## ServerMain

Run the QLever query server.

### Usage

```bash
ServerMain [OPTIONS]
```

### Required Options

| Option | Short | Argument | Purpose |
|--------|-------|----------|---------|
| `--index-basename` | `-i` | STRING | Index file basename (must exist) |
| `--port` | `-p` | INTEGER | HTTP port (e.g., 7023) |

### Optional Options

| Option | Short | Argument | Purpose |
|--------|-------|----------|---------|
| `--memory-max-size` | `-m` | SIZE | Max memory for queries (e.g., "16GB") |
| `--cache-max-size` | `-c` | SIZE | Cache size limit |
| `--num-simultaneous-queries` | `-j` | INTEGER | Parallel queries (default: 1) |
| `--access-token` | `-a` | STRING | Authentication token (empty = no auth) |
| `--text` | `-t` | (flag) | Load text index if available |
| `--no-patterns` | `-P` | (flag) | Disable ql:has-predicate pattern queries |
| `--only-pso-and-pos-permutations` | `-o` | (flag) | Use only PSO/POS indexes |

### Examples

**Start server on port 7023 with 16GB memory:**
```bash
ServerMain -i my-index -p 7023 -m 16GB
```

**With multiple concurrent queries:**
```bash
ServerMain -i my-index -p 7023 -m 32GB -j 4
```

**With text search enabled:**
```bash
ServerMain -i my-index -p 7023 -t
```

**With authentication:**
```bash
ServerMain -i my-index -p 7023 -a "my-secret-token"
```

## Querying the Server

Once `ServerMain` is running, query via HTTP:

### REST API

```bash
# SPARQL query
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10"

# Response formats
curl -Gs http://localhost:7023 \
  --data-urlencode "query=..." \
  -H "Accept: application/json"       # JSON (default)
  # or "application/sparql-results+json"
  # or "text/csv"
  # or "text/tab-separated-values"
```

### Query Parameters

| Parameter | Required | Values | Purpose |
|-----------|----------|--------|---------|
| `query` | Yes | SPARQL string | The query to execute |
| `format` | No | json, csv, tsv, xml | Result format |
| `send` | No | string or file | How to send (usually default) |

## Real-World Example: Complete Workflow

```bash
#!/bin/bash
set -e

# 1. Prepare data
wget https://example.org/data.nt.gz
gunzip data.nt.gz

# 2. Create settings
cat > settings.json << 'EOF'
{
  "num-triples-per-batch": 50000000,
  "parallel-parsing": true,
  "languages-internal": ["en"]
}
EOF

# 3. Build index (takes time)
IndexBuilderMain -F nt -f data.nt -i my-knowledge-graph -s settings.json

# 4. Start server
ServerMain -i my-knowledge-graph -p 7023 -m 16GB -j 4 &
sleep 2

# 5. Query
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10"

# 6. Stop server
kill %1
```

## Troubleshooting

**"Index not found" error**
```bash
# Verify index files exist
ls my-index.* | head -10
# Should show: my-index.vocabulary, my-index.pso, etc.
```

**"Port already in use"**
```bash
# Use a different port
ServerMain -i my-index -p 8000
```

**Out of memory during indexing**
```bash
# Reduce batch size in settings.json
"num-triples-per-batch": 10000000
# or disable parallel parsing
"parallel-parsing": false
```

**Out of memory during queries**
```bash
# Increase server memory
ServerMain -i my-index -p 7023 -m 32GB

# or reduce concurrent queries
ServerMain -i my-index -p 7023 -j 2
```

## For End Users

If you're **not building from source**, use the Python CLI tool instead:

```bash
pip install qlever
qlever setup-config wikidata-small
qlever index
qlever start
qlever query "SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

See the [`qlever-control` repository](https://github.com/ad-freiburg/qlever-control) for full documentation.

---

**Related:**
- [JSON Configuration Reference](./configuration.md)
- [SPARQL Support](./sparql.md)
- [How-to: Performance](../how-to/performance.md)
- [Architecture Overview](../explanation/architecture.md)

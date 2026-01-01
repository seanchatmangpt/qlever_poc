# Quick Start: Copy-Paste Commands

The fastest way to get QLever running. Copy, paste, run.

## For End Users (Using `pip install qlever`)

```bash
# 1. Install
pip install qlever

# 2. Create a demo project
mkdir my-qlever && cd my-qlever
qlever setup-config wikidata-small

# 3. Build index
qlever index

# 4. Start server
qlever start

# 5. Query (in another terminal)
qlever query "SELECT ?x WHERE { ?x a ?type } LIMIT 10"

# 6. Stop
qlever stop
```

**See:** [`qlever-control` documentation](https://github.com/ad-freiburg/qlever-control)

---

## For Developers (Building from Source)

### Setup

```bash
# Clone and build
git clone https://github.com/ad-freiburg/qlever
cd qlever
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
cmake --build .
```

### Build an Index

```bash
# Prepare data
wget https://example.org/data.ttl.gz
gunzip data.ttl.gz

# Create settings
cat > settings.json << 'EOF'
{
  "num-triples-per-batch": 50000000,
  "parallel-parsing": true,
  "languages-internal": ["en"]
}
EOF

# Build index
./build/IndexBuilderMain -F ttl -f data.ttl -i my-index -s settings.json
```

### Start Server

```bash
# Run server with 16GB memory, 4 parallel queries
./build/ServerMain -i my-index -p 7023 -m 16GB -j 4
```

### Query the Server

```bash
# In another terminal
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

---

## Docker Quick Start

```bash
# Start Wikidata demo
docker run -p 7023:7001 \
  -e INDEX_PREFIX=wikidata \
  adfreiburg/qlever:latest

# Query
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

---

## Common Tasks

### Change Server Port

**Python CLI:**
```bash
qlever start --port 8000
```

**C++ Binary:**
```bash
./build/ServerMain -i my-index -p 8000
```

### Increase Memory

**Python CLI:**
```bash
qlever start --memory 32GB
```

**C++ Binary:**
```bash
./build/ServerMain -i my-index -p 7023 -m 32GB
```

### Enable Multiple Concurrent Queries

**C++ Binary:**
```bash
./build/ServerMain -i my-index -p 7023 -j 4
```

### Handle Out of Memory During Indexing

**Reduce batch size** in `settings.json`:
```json
{
  "num-triples-per-batch": 10000000
}
```

Then rebuild:
```bash
./build/IndexBuilderMain -F ttl -f data.ttl -i my-index -s settings.json
```

### Enable Text Search During Indexing

```bash
./build/IndexBuilderMain -F ttl -f data.ttl -i my-index -W
```

Then start server with text flag:
```bash
./build/ServerMain -i my-index -p 7023 -t
```

---

## Query Examples

### Find All Items of a Type

```sparql
SELECT ?item WHERE {
  ?item a wd:Q571 .  # wd:Q571 = book
}
LIMIT 100
```

### Count by Category

```sparql
SELECT ?type (COUNT(*) as ?count) WHERE {
  ?item a ?type .
}
GROUP BY ?type
ORDER BY DESC(?count)
LIMIT 20
```

### Get Details

```sparql
SELECT ?name ?author ?year WHERE {
  ?book a wd:Q571 .
  ?book rdfs:label ?name .
  FILTER(LANG(?name) = "en")

  OPTIONAL { ?book wdt:P50 ?a . ?a rdfs:label ?author . FILTER(LANG(?author) = "en") }
  OPTIONAL { ?book wdt:P577 ?date . BIND(YEAR(?date) as ?year) }
}
LIMIT 50
```

### Text Search

```sparql
SELECT ?item ?score WHERE {
  ?item qlever:text "Einstein" ?score .
}
ORDER BY DESC(?score)
LIMIT 10
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Port already in use" | Use different port: `-p 8000` |
| "Out of memory" | Reduce `num-triples-per-batch` or increase RAM |
| "Query timeout" | Add more FILTER conditions or LIMIT results |
| "Index not found" | Check index files exist: `ls my-index.*` |
| "Connection refused" | Make sure ServerMain is running |

---

## Next Steps

- **[Full Tutorials](./tutorials/)** — Learn by doing
- **[How-to Guides](./how-to/)** — Solve specific problems
- **[Reference](./reference/)** — Look things up
- **[Concepts](./explanation/)** — Understand the system

---

**Questions?** See [QLever documentation index](./INDEX.md)

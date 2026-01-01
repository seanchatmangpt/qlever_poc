# Quick Start: Run QLever in 5 Minutes

Get QLever running with sample data in just a few commands. No prior RDF/SPARQL knowledge needed.

## Prerequisites

- Python 3.8+ with pip
- Docker (optional but recommended) or 5GB free disk space
- 5 minutes of your time

## Step 1: Install QLever CLI

```bash
pip install --upgrade qlever
qlever --version
```

## Step 2: Create a Project Directory

```bash
mkdir my-qlever-demo
cd my-qlever-demo
```

## Step 3: Download a Sample Configuration

QLever includes example configurations for popular datasets. Start with a small one:

```bash
# For Wikidata sample (recommended for first time)
qlever setup-config wikidata-small

# Or try DBLP (computer science publications)
# qlever setup-config dblp
```

This creates a `Qleverfile` with all settings pre-configured.

## Step 4: Build the Index

This processes the RDF data and creates an optimized index (~2-5 minutes for small samples):

```bash
qlever index
```

You'll see progress output. Once complete, you have an indexed dataset ready to query.

## Step 5: Start the Server

```bash
qlever start
```

Your QLever server is now running at **http://localhost:7023**

## Step 6: Run Your First Query

In another terminal:

```bash
# Query: Find all presidents of the United States
curl -X POST http://localhost:7023 \
  -H "Content-Type: application/sparql-query" \
  --data "
SELECT ?president ?name WHERE {
  ?president wdt:P39 wd:Q11696 .
  ?president rdfs:label ?name .
  FILTER(LANG(?name) = \"en\")
}
LIMIT 10
"
```

Or open **http://localhost:7023/gui** in your browser for an interactive query interface with autocompletion.

## Step 7: Explore

Try more queries:

```bash
# Find all works by Wikidata users
curl -X POST http://localhost:7023/query \
  -H "Content-Type: application/sparql-query" \
  --data "
SELECT DISTINCT ?type (COUNT(?item) as ?count) WHERE {
  ?item a ?type
}
GROUP BY ?type
ORDER BY DESC(?count)
LIMIT 20
"
```

## Next Steps

- **Learn SPARQL Basics** → [Your First Query](./02-first-query.md)
- **Use Your Own Data** → [Load Your Data](./03-load-data.md)
- **Advanced Features** → [How-to Guides](../how-to/)

## Stop the Server

```bash
# When done, stop the server
qlever stop

# Or in the server terminal: Ctrl+C
```

## Common Issues & Solutions

### "qlever: command not found"

**Problem:** Command is not in your PATH

**Fix:**
```bash
# Make sure Python 3.8+ is installed
python3 --version

# Reinstall qlever
pip install --upgrade qlever

# Verify installation
qlever --version
```

See [Troubleshooting: Installation](../TROUBLESHOOTING.md#installation--setup)

### "Connection refused" when querying

**Problem:** Server isn't running or wrong port

**Fix:**
```bash
# Check if server is running
qlever status

# If not running, start it
qlever start

# Verify port is correct (should be 7023)
curl http://localhost:7023
```

See [Troubleshooting: Connection refused](../TROUBLESHOOTING.md#connection-refused-when-querying)

### "Out of memory" during indexing

**Problem:** Not enough memory allocated

**Fix:**
```bash
# Increase memory allocation
qlever stop
qlever index --memory 32GB  # Or larger

# Or reduce batch size in Qleverfile
# Then retry
qlever index --force
```

See [Troubleshooting: Out of memory](../TROUBLESHOOTING.md#out-of-memory-during-qlever-index)

### More issues?

See the [complete Troubleshooting Guide](../TROUBLESHOOTING.md) for solutions to 20+ common problems.

## Troubleshooting

**Port 7023 already in use?**
```bash
qlever start --port 7024
```

**Want to use Docker instead?**
```bash
qlever start --docker
```

**Need more details?**
See [How-to: Configure QLever](../how-to/configuration.md)

---

**Congratulations!** You've successfully run QLever. You're ready to learn more about SPARQL and optimize for your use case.

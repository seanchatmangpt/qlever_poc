---
diataxis_type: tutorial
title: "Quick Start: Run QLever in 5 Minutes"
description: "Step-by-step guide to install, configure, and run your first QLever query in Docker"
audience: humans
status: complete
last_updated: 2026-01-02
difficulty: beginner
estimated_time: "5 minutes"
prerequisites:
  - "Docker installed (or 5GB disk space)"
  - "Python 3.8+"
  - "Basic command line familiarity"
related_docs:
  - "tutorials/02-first-query.md"
  - "how-to/configuration.md"
  - "how-to/troubleshooting.md"
  - "reference/cli.md"
keywords:
  - quickstart
  - installation
  - first query
  - Docker
  - getting started
  - beginner
  - setup
semantic_tags:
  - "tutorial/quickstart"
  - "installation/docker"
  - "getting-started/first-steps"
agent_priority: high
search_boost: 3.0
---

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

## Next Steps

- **Learn SPARQL Basics** → [Your First Query](./02-first-query.md)
- **Use Your Own Data** → [Load Your Data](./03-load-data.md)
- **Advanced Features** → [How-to Guides](../how-to/)

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

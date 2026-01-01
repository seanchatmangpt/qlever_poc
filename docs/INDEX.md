# QLever Documentation

⚠️ **IMPORTANT:** This repository contains the **C++ backend** (query engine). For the user-friendly **Python CLI tool** (`qlever setup-config`, `qlever index`, `qlever start`), see [`qlever-control`](https://github.com/ad-freiburg/qlever-control).

**Confused?**
- **End users**: Install `pip install qlever` → [qlever-control docs](https://github.com/ad-freiburg/qlever-control)
- **Developers/Contributors**: Continue with docs below
- **Docker users**: See [Quickstart](./tutorials/01-quickstart.md)

---

## 📚 [Tutorials](./tutorials/) — Learn by Doing
Step-by-step guides for Docker users and developers.

- **[Quick Start](./tutorials/01-quickstart.md)** — Run QLever in Docker (5 minutes)
- **[Your First Query](./tutorials/02-first-query.md)** — Write basic SPARQL queries
- **[Build from Source](./tutorials/03-load-data.md)** — Compile and run natively

## 🔧 [How-to Guides](./how-to/) — Solve Specific Problems
Practical solutions for common tasks.

- **[Optimize Query Performance](./how-to/performance.md)** — Speed up slow queries
- **[Text Search & Autocompletion](./how-to/text-search.md)** — Enable full-text search
- **[Spatial Queries](./how-to/spatial-queries.md)** — Query geographic data
- **[Configure QLever](./how-to/configuration.md)** — Tune memory, indexing, and server settings

## 📖 [Reference](./reference/) — Look Things Up
Complete, structured information for developers.

- **[C++ Binaries](./reference/cli.md)** — IndexBuilderMain & ServerMain options (for building from source)
- **[JSON Configuration](./reference/configuration.md)** — Settings files for IndexBuilderMain
- **[SPARQL Support](./reference/sparql.md)** — SPARQL features and limitations
- **[HTTP API](./reference/api.md)** — ServerMain REST API endpoints

## 💡 [Explanation](./explanation/) — Understand the System
Big-picture concepts and architecture.

- **[What is RDF & SPARQL?](./explanation/rdf-sparql.md)** — Core concepts explained
- **[How QLever Works](./explanation/architecture.md)** — System design and components
- **[Query Optimization](./explanation/optimization.md)** — Cost-based planning and join orders
- **[Performance Characteristics](./explanation/performance.md)** — When QLever excels and trade-offs

---

## ⚡ Quick Navigation

**Copy-paste ready to go?** → [Quick Start Commands](./QUICK_START.md) ⭐ START HERE

**Having problems?** → [Troubleshooting Guide](./TROUBLESHOOTING.md)

**Just want to get started?** → [Quick Start Tutorial](./tutorials/01-quickstart.md)

**Looking for a specific command?** → [CLI Reference](./reference/cli.md)

**Understanding concepts?** → See [Explanation](./explanation/)

---

## 📊 Find Demos

QLever hosts live demo instances at http://qlever.cs.uni-freiburg.de with Wikidata, OpenStreetMap, UniProt, DBLP, and more. Each demo includes dataset statistics and query examples.

---

**Last Updated:** January 2026 | **Status:** Complete Diataxis Suite | **Contributing?** Check CLAUDE.md

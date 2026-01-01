# QLever Documentation

⚠️ **IMPORTANT:** This repository contains the **C++ backend** (query engine). For the user-friendly **Python CLI tool** (`qlever setup-config`, `qlever index`, `qlever start`), see [`qlever-control`](https://github.com/ad-freiburg/qlever-control).

**Confused?**
- **End users**: Install `pip install qlever` → [qlever-control docs](https://github.com/ad-freiburg/qlever-control)
- **Developers/Contributors**: Continue with docs below
- **Docker users**: See [Quickstart](./tutorials/01-quickstart.md)

---

This documentation follows the **[Diataxis](https://diataxis.fr/)** framework, organizing content by user intent:

- **Tutorials** = Learning-oriented (step-by-step, hands-on)
- **How-to Guides** = Problem-oriented (goal-focused, practical)
- **Reference** = Information-oriented (complete, technical)
- **Explanation** = Understanding-oriented (background, conceptual)

---

## 📚 [Tutorials](./tutorials/) — Learn by Doing

Step-by-step guides for Docker users and developers. Start here if you're new to QLever.

- **[Quick Start](./tutorials/01-quickstart.md)** — Run QLever in Docker (5 minutes)
- **[Your First Query](./tutorials/02-first-query.md)** — Write basic SPARQL queries
- **[Build from Source](./tutorials/03-load-data.md)** — Compile and run natively

**Best for:** Learning the basics, getting productive quickly

---

## 🔧 [How-to Guides](./how-to/) — Solve Specific Problems

Practical solutions for common tasks. Use when you know what you want to accomplish.

### Getting Started
- **[Quick Start Commands](./how-to/quick-start.md)** ⭐ — Copy-paste examples for all use cases
- **[Build from Source](./how-to/native-setup.md)** — Compile and run natively

### Configuration & Performance
- **[Optimize Query Performance](./how-to/performance.md)** — Speed up slow queries
- **[Configure QLever](./how-to/configuration.md)** — Tune memory, indexing, and server settings

### Features
- **[Text Search & Autocompletion](./how-to/text-search.md)** — Enable full-text search
- **[Spatial Queries](./how-to/spatial-queries.md)** — Query geographic data
- **[SHACL Integration](./how-to/shacl-integration.md)** — Use SHACL validation
- **[N3 Format Guide](./how-to/n3-format.md)** — Work with Notation3 files
- **[N3 Troubleshooting](./how-to/n3-troubleshooting.md)** — Fix N3-related issues

### Troubleshooting
- **[Troubleshooting Guide](./how-to/troubleshooting.md)** — Solutions for common problems

**Best for:** Solving specific problems, practical examples

---

## 📖 [Reference](./reference/) — Look Things Up

Complete, structured information for developers. Use to look up types, methods, and specifications.

### Core APIs
- **[C++ Binaries](./reference/cli.md)** — IndexBuilderMain & ServerMain options
- **[HTTP API](./reference/api.md)** — ServerMain REST API endpoints
- **[JSON Configuration](./reference/configuration.md)** — Settings files for IndexBuilderMain
- **[SPARQL Support](./reference/sparql.md)** — SPARQL features and limitations

### Advanced Features
- **[Advanced Features](./reference/advanced-features.md)** — SPARQL+Text, autocompletion, statistics
- **[SPARQL+Text Reference](./reference/sparql-plus-text.md)** — Text search query syntax
- **[Path Search](./reference/path-search.md)** — Graph path finding API
- **[SHACL Compliance](./reference/shacl-compliance.md)** — W3C SHACL specification compliance

### Tools & Data
- **[Master Makefile](./reference/master-makefile.md)** — Makefile targets and configuration
- **[Knowledge Bases](./reference/knowledge-bases.md)** — Available datasets and collections

**Best for:** Finding exact signatures, looking up details, technical specifications

---

## 💡 [Explanation](./explanation/) — Understand the System

Big-picture concepts and architecture. Understand *why* QLever works the way it does.

- **[What is RDF & SPARQL?](./explanation/rdf-sparql.md)** — Core concepts explained
- **[How QLever Works](./explanation/architecture.md)** — System design and components
- **[Query Optimization](./explanation/optimization.md)** — Cost-based planning and join orders
- **[Performance Characteristics](./explanation/performance.md)** — When QLever excels and trade-offs
- **[CONSTRUCT Causation](./explanation/construct-causation.md)** — CONSTRUCT query patterns
- **[CONSTRUCT Causation Modes](./explanation/construct-causation-modes.md)** — Execution models

**Best for:** Understanding design decisions, learning concepts, architectural insights

---

## ⚡ Quick Navigation

**Copy-paste ready to go?** → [Quick Start Commands](./how-to/quick-start.md) ⭐ START HERE

**Having problems?** → [Troubleshooting Guide](./how-to/troubleshooting.md)

**Just want to get started?** → [Quick Start Tutorial](./tutorials/01-quickstart.md)

**Looking for a specific command?** → [CLI Reference](./reference/cli.md)

**Understanding concepts?** → See [Explanation](./explanation/)

---

## 📊 Find Demos

QLever hosts live demo instances at http://qlever.cs.uni-freiburg.de with Wikidata, OpenStreetMap, UniProt, DBLP, and more. Each demo includes dataset statistics and query examples.

---

## 📁 Additional Resources

- **[Datalog Documentation](./datalog/)** — Recursive queries and inference rules
- **[Roadmap](./roadmap/)** — Future features and plans
- **[Archive](./archive/)** — Deprecated and legacy documentation

---

**Last Updated:** January 2026 | **Status:** Complete Diataxis Suite | **Contributing?** Check CLAUDE.md

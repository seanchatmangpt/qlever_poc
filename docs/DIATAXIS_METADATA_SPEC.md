# Diataxis Document Metadata Specification

**Version**: 1.0.0
**Status**: Specification Closure - CLOSED
**Last Updated**: 2026-01-02

---

## Overview

This specification defines the canonical metadata header format for all Diataxis-structured documentation in the QLever project. The format enables efficient discovery by both human readers and autonomous agents.

## Design Principles

1. **Machine-Parseable**: YAML frontmatter format for programmatic access
2. **Human-Readable**: Clear field names and structured values
3. **Diataxis-Aligned**: Enforces the four document types (tutorial, how-to, explanation, reference)
4. **Agent-Optimized**: Includes semantic search fields, prerequisites, and invariants
5. **Version-Aware**: Tracks document freshness and status
6. **Minimal-Required**: Only essential fields are mandatory

## Metadata Schema

### YAML Frontmatter Structure

All Diataxis documents MUST begin with YAML frontmatter delimited by `---`:

```yaml
---
# REQUIRED FIELDS
diataxis_type: <tutorial|how-to|explanation|reference>
title: <string>
description: <string>
audience: <agents|humans|all>
status: <draft|complete|deprecated|archived>
last_updated: <YYYY-MM-DD>

# RECOMMENDED FIELDS
difficulty: <beginner|intermediate|advanced>
estimated_time: <string>  # e.g., "5 minutes", "30 minutes", "2 hours"
prerequisites: <list>
related_docs: <list>
keywords: <list>

# OPTIONAL AGENT-SPECIFIC FIELDS
agent_priority: <low|medium|high|critical>
semantic_tags: <list>
invariants: <list>
specifications: <list>
search_boost: <float>  # 0.0-10.0, default 1.0
---
```

## Field Definitions

### Required Fields

#### `diataxis_type`
- **Type**: `enum`
- **Values**: `tutorial`, `how-to`, `explanation`, `reference`
- **Purpose**: Primary Diataxis classification
- **Validation**: MUST be one of the four canonical types

#### `title`
- **Type**: `string`
- **Purpose**: Human-readable document title
- **Constraint**: 1-100 characters
- **Example**: `"Quick Start: Run QLever in 5 Minutes"`

#### `description`
- **Type**: `string`
- **Purpose**: One-sentence summary for search results and listings
- **Constraint**: 1-200 characters
- **Example**: `"Step-by-step guide to install, configure, and run your first QLever query"`

#### `audience`
- **Type**: `enum`
- **Values**: `agents`, `humans`, `all`
- **Purpose**: Target reader type for filtering
- **Semantics**:
  - `agents`: Primarily for autonomous agent consumption (specifications, invariants, schemas)
  - `humans`: Primarily for human readers (tutorials, explanations)
  - `all`: Useful for both (references, how-to guides)

#### `status`
- **Type**: `enum`
- **Values**: `draft`, `complete`, `deprecated`, `archived`
- **Purpose**: Document lifecycle state
- **Semantics**:
  - `draft`: Work in progress, may contain gaps
  - `complete`: Reviewed, accurate, ready for use
  - `deprecated`: Superseded by newer document (include `superseded_by` field)
  - `archived`: Historical record, no longer maintained

#### `last_updated`
- **Type**: `date`
- **Format**: `YYYY-MM-DD`
- **Purpose**: Freshness indicator for stale document detection
- **Example**: `2026-01-02`

### Recommended Fields

#### `difficulty`
- **Type**: `enum`
- **Values**: `beginner`, `intermediate`, `advanced`
- **Purpose**: Skill level required (primarily for tutorials and how-tos)
- **Omit for**: Reference and explanation documents

#### `estimated_time`
- **Type**: `string`
- **Purpose**: Expected completion time for tutorials/how-tos
- **Format**: Free-form human-readable (e.g., "5 minutes", "1 hour")
- **Example**: `"15 minutes"`

#### `prerequisites`
- **Type**: `list[string]`
- **Purpose**: Required knowledge or completed documents before starting
- **Format**: List of document paths or knowledge areas
- **Example**:
  ```yaml
  prerequisites:
    - "Docker installed"
    - "docs/tutorials/01-quickstart.md"
    - "Basic command line knowledge"
  ```

#### `related_docs`
- **Type**: `list[string]`
- **Purpose**: Cross-references to related documentation
- **Format**: Relative paths from `/docs/` or absolute URLs
- **Example**:
  ```yaml
  related_docs:
    - "how-to/performance.md"
    - "reference/api.md"
    - "https://www.w3.org/TR/sparql11-query/"
  ```

#### `keywords`
- **Type**: `list[string]`
- **Purpose**: Semantic search terms for discovery
- **Constraint**: 3-20 keywords
- **Example**:
  ```yaml
  keywords:
    - SPARQL
    - RDF
    - indexing
    - query optimization
    - performance
  ```

### Optional Agent-Specific Fields

#### `agent_priority`
- **Type**: `enum`
- **Values**: `low`, `medium`, `high`, `critical`
- **Purpose**: Agent search ranking hint
- **Default**: `medium`
- **Semantics**:
  - `critical`: Core specifications, invariants, safety-critical docs
  - `high`: Architecture, APIs, primary references
  - `medium`: How-tos, explanations, standard references
  - `low`: Archives, deprecated, historical

#### `semantic_tags`
- **Type**: `list[string]`
- **Purpose**: Ontological tags for semantic search
- **Format**: Structured tags using `/` hierarchy
- **Example**:
  ```yaml
  semantic_tags:
    - "architecture/query-execution"
    - "api/http-rest"
    - "data-format/rdf"
    - "optimization/cost-based"
  ```

#### `invariants`
- **Type**: `list[string]`
- **Purpose**: System invariants documented in this file (for specification documents)
- **Use Case**: Agent verification and validation workflows
- **Example**:
  ```yaml
  invariants:
    - "Epoch binding: every artifact keyed with (epochId, manifestSha256)"
    - "Determinism filter: only deterministic queries captured"
    - "Atomic visibility: all writes atomic or lock-guarded"
  ```

#### `specifications`
- **Type**: `list[string]`
- **Purpose**: External specifications referenced or implemented
- **Format**: Specification names or URLs
- **Example**:
  ```yaml
  specifications:
    - "W3C SPARQL 1.1 Query Language"
    - "RDF 1.1 Turtle"
    - "C++20 Standard"
  ```

#### `search_boost`
- **Type**: `float`
- **Range**: `0.0-10.0`
- **Default**: `1.0`
- **Purpose**: Search result ranking multiplier
- **Use Case**: Boost frequently accessed documents (e.g., index page = 5.0, archived doc = 0.5)

## Document Type Guidelines

### Tutorial

**Purpose**: Learning-oriented, step-by-step guides

**Required Fields**:
- `diataxis_type: tutorial`
- `difficulty: beginner|intermediate|advanced`
- `estimated_time: <string>`
- `prerequisites: <list>`

**Recommended Fields**:
- `keywords: <list>`
- `related_docs: <list>` (include "Next Steps" tutorials)

**Example Metadata**:
```yaml
---
diataxis_type: tutorial
title: "Quick Start: Run QLever in 5 Minutes"
description: "Step-by-step guide to install, configure, and run your first QLever query"
audience: humans
status: complete
last_updated: 2026-01-02
difficulty: beginner
estimated_time: "5 minutes"
prerequisites:
  - "Docker installed (or 5GB disk space)"
  - "Python 3.8+"
related_docs:
  - "tutorials/02-first-query.md"
  - "how-to/configuration.md"
keywords:
  - quickstart
  - installation
  - first query
  - Docker
  - getting started
semantic_tags:
  - "tutorial/quickstart"
  - "installation/docker"
agent_priority: high
search_boost: 3.0
---
```

### How-To Guide

**Purpose**: Problem-oriented, goal-focused guides

**Required Fields**:
- `diataxis_type: how-to`
- `description: <specific problem statement>`

**Recommended Fields**:
- `difficulty: <level>`
- `estimated_time: <string>`
- `prerequisites: <list>`
- `related_docs: <list>` (link to reference docs)

**Example Metadata**:
```yaml
---
diataxis_type: how-to
title: "Claude Code Setup: SessionStart Hooks"
description: "Configure automatic dependency installation for Claude Code on the web"
audience: humans
status: complete
last_updated: 2026-01-02
difficulty: intermediate
estimated_time: "15 minutes"
prerequisites:
  - "GitHub repository access"
  - "Basic shell scripting knowledge"
related_docs:
  - "reference/cli.md"
  - "explanation/architecture.md"
keywords:
  - Claude Code
  - SessionStart hooks
  - environment setup
  - automation
  - dependencies
semantic_tags:
  - "configuration/claude-code"
  - "automation/hooks"
  - "development/setup"
agent_priority: medium
---
```

### Explanation

**Purpose**: Understanding-oriented, conceptual background

**Required Fields**:
- `diataxis_type: explanation`
- `audience: all` (typically)

**Recommended Fields**:
- `keywords: <list>` (concept names)
- `related_docs: <list>` (link to how-tos and references)

**Example Metadata**:
```yaml
---
diataxis_type: explanation
title: "How QLever Works: Architecture Overview"
description: "High-level understanding of QLever's system architecture and performance characteristics"
audience: all
status: complete
last_updated: 2026-01-02
related_docs:
  - "explanation/optimization.md"
  - "explanation/performance.md"
  - "how-to/performance.md"
  - "reference/api.md"
keywords:
  - architecture
  - indexing
  - query execution
  - permutations
  - memory management
  - cost-based optimization
semantic_tags:
  - "architecture/system-design"
  - "architecture/data-structures"
  - "performance/optimization"
agent_priority: high
search_boost: 2.5
---
```

### Reference

**Purpose**: Information-oriented, comprehensive technical details

**Required Fields**:
- `diataxis_type: reference`
- `audience: all` (typically)

**Recommended Fields**:
- `specifications: <list>` (if implementing external specs)
- `keywords: <list>` (API names, method names)
- `semantic_tags: <list>` (for API categorization)

**Example Metadata**:
```yaml
---
diataxis_type: reference
title: "HTTP REST API Reference"
description: "Complete ServerMain REST API specification for SPARQL query execution"
audience: all
status: complete
last_updated: 2026-01-02
related_docs:
  - "reference/cli.md"
  - "reference/sparql.md"
  - "how-to/performance.md"
  - "tutorials/02-first-query.md"
keywords:
  - HTTP API
  - REST
  - ServerMain
  - SPARQL endpoint
  - query parameters
  - response formats
  - JSON
  - CSV
  - XML
specifications:
  - "W3C SPARQL 1.1 Protocol"
  - "HTTP/1.1 (RFC 7231)"
semantic_tags:
  - "api/http-rest"
  - "api/sparql-endpoint"
  - "reference/server"
agent_priority: high
search_boost: 2.0
---
```

## Agent Discovery Patterns

### Search by Document Type

```python
# Agent code example
docs = [d for d in all_docs if d.metadata['diataxis_type'] == 'reference']
```

### Search by Audience

```python
# Filter agent-relevant docs
agent_docs = [d for d in all_docs if d.metadata['audience'] in ['agents', 'all']]
```

### Search by Keywords

```python
# Semantic search
results = [d for d in all_docs if 'SPARQL' in d.metadata.get('keywords', [])]
```

### Search by Prerequisites

```python
# Dependency resolution
prereq_chain = resolve_prerequisites(doc.metadata['prerequisites'])
```

### Search by Invariants

```python
# Find specification documents
spec_docs = [d for d in all_docs if 'invariants' in d.metadata]
```

### Weighted Search Ranking

```python
# Combine priority and boost
score = base_relevance * doc.metadata.get('search_boost', 1.0)
if doc.metadata.get('agent_priority') == 'critical':
    score *= 2.0
```

## Validation Rules

1. **YAML Syntax**: Metadata MUST be valid YAML
2. **Required Fields**: All required fields MUST be present
3. **Enum Validation**: Enum fields MUST use only specified values
4. **Date Format**: `last_updated` MUST match `YYYY-MM-DD`
5. **Path Validation**: `related_docs` paths SHOULD resolve to existing files
6. **Status Consistency**: `deprecated` documents SHOULD include `superseded_by` field
7. **Diataxis Alignment**: Content MUST match declared `diataxis_type`

## Migration Guide

### Adding Metadata to Existing Documents

1. Insert YAML frontmatter at the very beginning of the file
2. Fill required fields based on document content
3. Add recommended fields for better discoverability
4. Use `git log <file>` to determine `last_updated` date
5. Validate with YAML parser

### Automated Migration Script

```bash
# Generate metadata for all docs (to be implemented)
python scripts/add-metadata.py docs/
```

## Changelog

- **1.0.0** (2026-01-02): Initial specification closure

---

**Status**: CLOSED (zero degrees of freedom for implementation)
**Validation**: Deterministic schema, machine-parseable, agent-optimized

# Diataxis Metadata Quick Reference

**One-page reference for adding metadata headers to documentation**

---

## Minimal Template (Required Fields Only)

```yaml
---
diataxis_type: <tutorial|how-to|explanation|reference>
title: "<Document Title>"
description: "<One-sentence summary>"
audience: <agents|humans|all>
status: <draft|complete|deprecated|archived>
last_updated: YYYY-MM-DD
---
```

## Full Template (All Fields)

```yaml
---
# REQUIRED (6 fields)
diataxis_type: <tutorial|how-to|explanation|reference>
title: "<Document Title>"
description: "<One-sentence summary>"
audience: <agents|humans|all>
status: <draft|complete|deprecated|archived>
last_updated: YYYY-MM-DD

# RECOMMENDED (5 fields)
difficulty: <beginner|intermediate|advanced>
estimated_time: "<5 minutes|30 minutes|2 hours>"
prerequisites:
  - "<Requirement 1>"
  - "<docs/path/to/prereq.md>"
related_docs:
  - "<docs/path/to/related.md>"
  - "<https://external-url.com>"
keywords:
  - <keyword1>
  - <keyword2>
  - <keyword3>

# OPTIONAL - AGENT-SPECIFIC (5 fields)
agent_priority: <low|medium|high|critical>
semantic_tags:
  - "<category/subcategory>"
  - "<domain/topic>"
invariants:
  - "<System invariant 1>"
specifications:
  - "<Specification name or URL>"
search_boost: <0.0-10.0>
---
```

## Field Cheat Sheet

| Field | Type | Values | Required | Purpose |
|-------|------|--------|----------|---------|
| `diataxis_type` | enum | tutorial, how-to, explanation, reference | ✅ | Document category |
| `title` | string | 1-100 chars | ✅ | Document title |
| `description` | string | 1-200 chars | ✅ | One-sentence summary |
| `audience` | enum | agents, humans, all | ✅ | Target readers |
| `status` | enum | draft, complete, deprecated, archived | ✅ | Lifecycle state |
| `last_updated` | date | YYYY-MM-DD | ✅ | Freshness indicator |
| `difficulty` | enum | beginner, intermediate, advanced | 📝 | Skill level |
| `estimated_time` | string | Free-form | 📝 | Completion time |
| `prerequisites` | list | Strings or paths | 📝 | Required knowledge |
| `related_docs` | list | Paths or URLs | 📝 | Cross-references |
| `keywords` | list | 3-20 strings | 📝 | Search terms |
| `agent_priority` | enum | low, medium, high, critical | ⚙️ | Agent ranking |
| `semantic_tags` | list | Hierarchical tags | ⚙️ | Ontological search |
| `invariants` | list | Strings | ⚙️ | System constraints |
| `specifications` | list | Names or URLs | ⚙️ | External specs |
| `search_boost` | float | 0.0-10.0 | ⚙️ | Ranking multiplier |

**Legend**: ✅ Required | 📝 Recommended | ⚙️ Optional (agent-specific)

## Quick Examples

### Tutorial
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
keywords: [quickstart, installation, Docker]
agent_priority: high
search_boost: 3.0
---
```

### How-To
```yaml
---
diataxis_type: how-to
title: "Configure Performance Settings"
description: "Optimize QLever query performance through configuration tuning"
audience: humans
status: complete
last_updated: 2026-01-02
difficulty: intermediate
related_docs: ["reference/configuration.md"]
keywords: [performance, optimization, tuning]
---
```

### Explanation
```yaml
---
diataxis_type: explanation
title: "How QLever Works: Architecture Overview"
description: "High-level understanding of QLever's system architecture"
audience: all
status: complete
last_updated: 2026-01-02
keywords: [architecture, indexing, query execution]
agent_priority: high
search_boost: 2.5
---
```

### Reference
```yaml
---
diataxis_type: reference
title: "HTTP REST API Reference"
description: "Complete ServerMain REST API specification"
audience: all
status: complete
last_updated: 2026-01-02
specifications: ["W3C SPARQL 1.1 Protocol"]
keywords: [HTTP, API, REST, SPARQL]
agent_priority: high
---
```

## Semantic Tags Examples

Use hierarchical structure with `/` delimiter:

```yaml
semantic_tags:
  - "architecture/query-execution"
  - "architecture/data-structures"
  - "api/http-rest"
  - "api/sparql-endpoint"
  - "configuration/claude-code"
  - "tutorial/quickstart"
  - "performance/optimization"
  - "development/setup"
```

## Validation Checklist

- [ ] YAML syntax valid (check with `yq .`)
- [ ] All 6 required fields present
- [ ] `diataxis_type` is one of: tutorial, how-to, explanation, reference
- [ ] `audience` is one of: agents, humans, all
- [ ] `status` is one of: draft, complete, deprecated, archived
- [ ] `last_updated` format is YYYY-MM-DD
- [ ] `difficulty` (if present) is: beginner, intermediate, or advanced
- [ ] `agent_priority` (if present) is: low, medium, high, or critical
- [ ] `search_boost` (if present) is between 0.0 and 10.0
- [ ] `related_docs` paths exist or are valid URLs
- [ ] Keywords list has 3-20 items

## Priority Guidelines

| Document Type | Suggested Priority | Suggested Boost |
|---------------|-------------------|-----------------|
| Index pages | critical | 5.0 |
| Quick starts | high | 3.0 |
| API references | high | 2.0 |
| Architecture docs | high | 2.5 |
| How-to guides | medium | 1.0 |
| Explanations | medium | 1.5 |
| Advanced tutorials | medium | 1.0 |
| Archived docs | low | 0.5 |

## Common Patterns

### Tutorial Series
```yaml
prerequisites:
  - "docs/tutorials/01-quickstart.md"
related_docs:
  - "tutorials/03-advanced-queries.md"
```

### Deprecated Document
```yaml
status: deprecated
superseded_by: "docs/how-to/new-version.md"
search_boost: 0.5
```

### Agent Specification
```yaml
audience: agents
agent_priority: critical
invariants:
  - "Epoch binding: every artifact keyed with (epochId, manifestSha256)"
  - "Determinism filter: only deterministic queries captured"
```

### Multi-Language Doc
```yaml
keywords:
  - SPARQL
  - Python
  - JavaScript
  - REST API
semantic_tags:
  - "api/http-rest"
  - "examples/python"
  - "examples/javascript"
```

## Validation Commands

### Check YAML Syntax
```bash
# Extract and validate frontmatter
awk '/^---$/,/^---$/{print}' docs/example.md | yq .
```

### List All Documents by Type
```bash
# Find all tutorials
grep -l "diataxis_type: tutorial" docs/**/*.md
```

### Check Required Fields
```python
import yaml

with open('docs/example.md') as f:
    content = f.read()
    frontmatter = content.split('---')[1]
    metadata = yaml.safe_load(frontmatter)

required = ['diataxis_type', 'title', 'description', 'audience', 'status', 'last_updated']
missing = [f for f in required if f not in metadata]
print(f"Missing: {missing}" if missing else "✓ All required fields present")
```

---

**Full Specification**: See `/home/user/qlever/docs/DIATAXIS_METADATA_SPEC.md`

**Examples**: See `/home/user/qlever/docs/examples/`

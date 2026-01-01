# CLI Command Style Guide

Standardized syntax and conventions for all CLI examples in QLever documentation.

## Python CLI (`qlever`)

### Canonical Command Forms

#### setup-config
```bash
qlever setup-config <config-name>
```
Examples: `wikidata-small`, `dblp`, `openstreetmap`

#### index
```bash
qlever index [--force] [--monitor]
```

#### start
```bash
qlever start [--port PORT] [--memory SIZE] [--background]
```

#### query
```bash
qlever query [--show-timing] [--format FORMAT] "SPARQL_STRING"
```

#### stop
```bash
qlever stop
```

#### status
```bash
qlever status
```

### Example Conventions

**Always show expected output:**
```bash
$ qlever --version
QLever version 2.8.3
```

**Show error cases with explanations:**
```bash
$ qlever index
# Expected when done:
# ✓ Index built successfully: 123,456,789 triples

# Error: "Qleverfile not found"
# Fix: Run `qlever setup-config` first
```

**Use comments to explain options:**
```bash
# Start server with 32GB memory limit
qlever start --memory 32GB

# Show query execution time
qlever query --show-timing "SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

---

## C++ Binaries (IndexBuilderMain, ServerMain)

### Canonical Forms

#### IndexBuilderMain
```bash
IndexBuilderMain -F FORMAT -f INPUT_FILE -i INDEX_NAME [-s settings.json]
```

#### ServerMain
```bash
ServerMain -i INDEX_NAME -p PORT [-m MEMORY] [-j CONCURRENT_QUERIES]
```

### Example Conventions

**Always specify all required flags:**
```bash
# Complete example
IndexBuilderMain -F ttl -f data.ttl -i my-index -s settings.json

# Not just:
IndexBuilderMain -i my-index  # Missing required -F and -f
```

**Show piping for stdin:**
```bash
cat data.ttl | IndexBuilderMain -F ttl -f - -i my-index
```

**Document flag meanings in comments:**
```bash
IndexBuilderMain \
  -F ttl \              # File format: ttl, nt, nq, rdf, jsonld
  -f data.ttl \         # Input file (use - for stdin)
  -i my-index \         # Output index name
  -s settings.json      # Optional: parsing settings file
```

---

## Documentation Conventions

### Last Validation

Every example should include a comment showing when it was last tested:

```markdown
```bash
qlever query "SELECT ?x WHERE { ?x a ?type } LIMIT 10"
# Last validated: 2025-01-01 against QLever 2.8.3
```
```

### Expected Output Blocks

After command examples, show what success looks like:

```bash
$ qlever start
# Expected output:
# ✓ Server started on http://localhost:7023
# ✓ Loaded index: my-index (123M triples)
# Ready for queries
```

### Error Handling

Link to troubleshooting for common errors:

```markdown
⚠️ **If you see:** `Connection refused`
This means ServerMain isn't running.
See [Troubleshooting: Connection refused](../TROUBLESHOOTING.md#connection-refused)
```

### Configuration Examples

Show both minimal and complete versions:

**Minimal:**
```bash
qlever index    # Uses defaults from Qleverfile
```

**Complete (with explanation):**
```bash
qlever index --force --memory 32GB    # Force rebuild with more memory
```

---

## Consistency Checks for Authors

When writing documentation examples:

- [ ] Command syntax matches canonical form above
- [ ] All required flags are specified
- [ ] Examples include expected output
- [ ] Errors link to TROUBLESHOOTING.md
- [ ] Last validation date is included
- [ ] Comments explain non-obvious flags
- [ ] Minimal example shown before full example

---

## Automated Validation

Examples are validated in CI/CD:

```bash
# Extract and run code blocks
scripts/validate-examples.sh docs/

# Check syntax consistency
scripts/validate-cli-syntax.sh docs/
```

---

## Quick Reference

| Context | Syntax | Example |
|---------|--------|---------|
| Python user | `qlever CMD [OPTS]` | `qlever start --memory 32GB` |
| Developer (binary) | `./build/BinaryName -OPTS` | `./build/ServerMain -i idx -p 7023` |
| Docker user | `docker run -p 7023:7001 ...` | `docker run -p 7023:7001 adfreiburg/qlever` |

---

**Last Updated:** 2026-01-01
**Status:** Active style guide for documentation

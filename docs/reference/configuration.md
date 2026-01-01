# JSON Settings Reference

Settings files control parsing behavior for `IndexBuilderMain`. Pass them with the `-s` flag.

## Quick Example

```bash
IndexBuilderMain -F ttl -f data.ttl -i my-index -s settings.json
```

`settings.json`:
```json
{
  "num-triples-per-batch": 50000000,
  "parallel-parsing": true,
  "languages-internal": ["en"]
}
```

## All Options

### Parsing Performance

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `num-triples-per-batch` | integer | 10000000 | Triples per batch during parsing. Larger = faster, more memory |
| `parser-batch-size` | integer | 1000 | Internal parser buffer size |
| `parallel-parsing` | boolean | false | Use multiple threads for RDF parsing |

**Tuning:**
- `num-triples-per-batch`: Start at 50M for large datasets
- `parser-batch-size`: Rarely needs adjustment (1000-5000 typical)
- `parallel-parsing`: Always enable on multi-core systems

### Language Optimization

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `languages-internal` | array | ["en"] | Languages to optimize for (storage/speed) |

**Effect:** Specified languages stored more efficiently (used 1-3x more space for other languages).

**Example:**
```json
{
  "languages-internal": ["en", "de", "fr"]
}
```

### IRI Constraints

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `ascii-prefixes-only` | boolean | false | Restrict IRIs to ASCII-only (memory savings) |
| `prefixes-external` | array | [] | Prefixes stored externally (saved space) |

**Use `ascii-prefixes-only` if:**
- Your dataset uses only ASCII IRIs (English ontologies)
- You want to minimize memory usage

**Use `prefixes-external` for:**
- Wikidata: Thousands of statement IRIs
- Large external namespace prefixes

**Example:**
```json
{
  "ascii-prefixes-only": false,
  "prefixes-external": [
    "<http://www.wikidata.org/entity/statement/>",
    "<http://www.wikidata.org/value/>"
  ]
}
```

### Locale & Collation

| Key | Type | Description |
|-----|------|-------------|
| `locale.language` | string | Language code (en, de, fr, etc.) |
| `locale.country` | string | Country code (US, DE, etc.) |
| `locale.ignore-punctuation` | boolean | Ignore punctuation in sorting |

**Example:**
```json
{
  "locale": {
    "language": "en",
    "country": "US",
    "ignore-punctuation": true
  }
}
```

## Real-World Examples

### Small Dataset (< 10M triples)

```json
{
  "num-triples-per-batch": 10000000,
  "parallel-parsing": false,
  "languages-internal": ["en"]
}
```

**Build time:** < 1 minute

### Medium Dataset (10M - 100M triples)

```json
{
  "num-triples-per-batch": 50000000,
  "parallel-parsing": true,
  "languages-internal": ["en"],
  "ascii-prefixes-only": false
}
```

**Build time:** 5-15 minutes

### Large Dataset (Wikidata-scale, 1B+ triples)

```json
{
  "num-triples-per-batch": 50000000,
  "parser-batch-size": 2000,
  "parallel-parsing": true,
  "languages-internal": ["en", "de", "fr"],
  "ascii-prefixes-only": false,
  "prefixes-external": [
    "<http://www.wikidata.org/entity/statement/>",
    "<http://www.wikidata.org/value/>",
    "<http://www.wikidata.org/reference/>"
  ],
  "locale": {
    "language": "en",
    "country": "US",
    "ignore-punctuation": true
  }
}
```

**Build time:** 1-3 hours

## How to Use Settings Files

### Create a settings file:

```bash
cat > settings.json << 'EOF'
{
  "num-triples-per-batch": 50000000,
  "parallel-parsing": true,
  "languages-internal": ["en"]
}
EOF
```

### Pass to IndexBuilderMain:

```bash
IndexBuilderMain -F ttl -f data.ttl -i my-index -s settings.json
```

## Memory & Performance Tuning

### If indexing is slow:

1. Increase `num-triples-per-batch` (if you have memory):
   ```json
   {
     "num-triples-per-batch": 100000000
   }
   ```

2. Enable parallel parsing:
   ```json
   {
     "parallel-parsing": true
   }
   ```

### If running out of memory:

1. Decrease `num-triples-per-batch`:
   ```json
   {
     "num-triples-per-batch": 10000000
   }
   ```

2. Disable parallel parsing:
   ```json
   {
     "parallel-parsing": false
   }
   ```

3. Enable `ascii-prefixes-only` (if applicable):
   ```json
   {
     "ascii-prefixes-only": true
   }
   ```

## Comparing Settings

| Setting | Impact | Memory | Speed |
|---------|--------|--------|-------|
| `num-triples-per-batch` (larger) | High | ↑ High | ↑ Fast |
| `parallel-parsing: true` | High | ↑ Moderate | ↑ Fast |
| `ascii-prefixes-only: true` | Medium | ↓ Lower | ↔ Same |
| `languages-internal` (more) | Low | ↑ Slight | ↔ Same |

---

**Related:**
- [C++ Binary Reference](./cli.md) - IndexBuilderMain/ServerMain options
- [How-to: Configuration](../how-to/configuration.md) - For Python `qlever` CLI users
- [Architecture Overview](../explanation/architecture.md) - Understand indexing pipeline

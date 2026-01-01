# HTTP REST API Reference

Query QLever via HTTP. All queries are POST requests to the running ServerMain.

## Basic Query

### Endpoint

```
POST http://localhost:7023/
```

### Parameters

| Parameter | Required | Type | Default | Purpose |
|-----------|----------|------|---------|---------|
| `query` | Yes | string | — | SPARQL query to execute |
| `format` | No | string | json | Result format (json, csv, tsv, xml) |

### Curl Example

```bash
# Simple query with default JSON format
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10"
```

### Response Format: JSON

```json
{
  "results": {
    "bindings": [
      {
        "x": {
          "type": "uri",
          "value": "http://wikidata.org/entity/Q5"
        },
        "type": {
          "type": "uri",
          "value": "http://www.w3.org/2002/07/owl#Class"
        }
      }
    ]
  },
  "warnings": []
}
```

## Output Formats

### JSON (Default)

Most flexible for programmatic access.

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?name WHERE { ?x rdfs:label ?name }" \
  -H "Accept: application/json"
```

Response:
```json
{
  "results": {
    "bindings": [
      { "name": { "type": "literal", "value": "Alice" } },
      { "name": { "type": "literal", "value": "Bob" } }
    ]
  }
}
```

### CSV

Comma-separated values. Good for spreadsheets.

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?name ?age WHERE { ... }" \
  -H "Accept: text/csv"
```

Response:
```
name,age
Alice,30
Bob,25
Carol,28
```

### TSV

Tab-separated values. Similar to CSV but tab-delimited.

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?name ?age WHERE { ... }" \
  -H "Accept: text/tab-separated-values"
```

Response:
```
name	age
Alice	30
Bob	25
Carol	28
```

### XML

SPARQL-standard XML format.

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?name WHERE { ... }" \
  -H "Accept: application/sparql-results+xml"
```

Response:
```xml
<?xml version="1.0"?>
<sparql xmlns="http://www.w3.org/2005/sparql-results#">
  <results>
    <result>
      <binding name="name">
        <literal>Alice</literal>
      </binding>
    </result>
  </results>
</sparql>
```

## Query Examples

### Simple SELECT

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a wd:Q571 } LIMIT 100"
```

### With FILTER

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=
    SELECT ?person ?name WHERE {
      ?person rdfs:label ?name .
      FILTER(CONTAINS(?name, 'Einstein'))
    }
    LIMIT 50
  "
```

### With JOIN

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=
    SELECT ?book ?title ?author ?authorName WHERE {
      ?book a wd:Q571 .
      ?book rdfs:label ?title .
      ?book wdt:P50 ?author .
      ?author rdfs:label ?authorName .
      FILTER(LANG(?title) = 'en')
      FILTER(LANG(?authorName) = 'en')
    }
    LIMIT 20
  "
```

### With Aggregation

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=
    SELECT ?author (COUNT(?book) as ?bookCount) WHERE {
      ?book a wd:Q571 .
      ?book wdt:P50 ?author .
    }
    GROUP BY ?author
    ORDER BY DESC(?bookCount)
    LIMIT 50
  "
```

### With GROUP BY and HAVING

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=
    SELECT ?country (COUNT(?person) as ?count) WHERE {
      ?person a wd:Q5 .
      ?person wdt:P27 ?country .
    }
    GROUP BY ?country
    HAVING COUNT(?person) > 100000
    ORDER BY DESC(?count)
  "
```

## Error Handling

### Invalid Query

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { INVALID }"
```

Response (HTTP 400):
```json
{
  "error": "Invalid SPARQL syntax",
  "details": "..."
}
```

### Query Timeout

```bash
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x ?p ?o }"
```

Response (HTTP 408):
```json
{
  "error": "Query timeout",
  "message": "Query exceeded 300 seconds"
}
```

### Connection Refused

If ServerMain is not running:

```bash
curl: (7) Failed to connect to localhost port 7023: Connection refused
```

Solution: Start ServerMain
```bash
ServerMain -i my-index -p 7023 -m 16GB
```

## Advanced Usage

### With Authentication

If ServerMain was started with `-a "token123"`:

```bash
curl -Gs http://localhost:7023 \
  -H "Authorization: Bearer token123" \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type }"
```

### With Custom Headers

Request specific format:

```bash
curl -Gs http://localhost:7023 \
  -H "Accept: application/json" \
  --data-urlencode "query=SELECT ?x WHERE { ... }"
```

### POST Method (Alternative)

Instead of GET with URL parameters:

```bash
curl -X POST http://localhost:7023 \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "query=SELECT%20%3Fx%20WHERE%20%7B%20%3Fx%20a%20%3Ftype%20%7D%20LIMIT%2010"
```

### From Python

```python
import requests

response = requests.get('http://localhost:7023', params={
    'query': 'SELECT ?x WHERE { ?x a ?type } LIMIT 10'
})
results = response.json()
print(results['results']['bindings'])
```

### From JavaScript/Node.js

```javascript
const queryString = 'SELECT ?x WHERE { ?x a ?type } LIMIT 10';
const response = await fetch(`http://localhost:7023?query=${encodeURIComponent(queryString)}`);
const results = await response.json();
console.log(results.results.bindings);
```

## Performance Tips

### 1. Use LIMIT for Large Result Sets

```bash
# Instead of:
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a wd:Q5 }"
  # Would return 8+ billion results!

# Do this:
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a wd:Q5 } LIMIT 1000"
```

### 2. Use FILTER Early

```sparql
# Slow
SELECT ?author WHERE {
  ?author ?p ?o .
  FILTER(?author = wd:Q1234)
}

# Fast
SELECT ?author WHERE {
  ?author ?p ?o .
  FILTER(?author = wd:Q1234)
}
```

### 3. Request Only Needed Format

JSON is most flexible but requires more parsing. Use CSV/TSV for large result sets:

```bash
# For large dataset, use CSV (smaller, faster to parse)
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x ?y WHERE { ... LIMIT 100000 }" \
  -H "Accept: text/csv"
```

### 4. Use Client-Side Caching

For repeated queries, cache the results:

```bash
# Cache query results for 5 minutes
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?type (COUNT(*) as ?count) WHERE { ?x a ?type } GROUP BY ?type" \
  -H "Cache-Control: max-age=300"
```

## Status Endpoint

Check server health:

```bash
curl http://localhost:7023/health
```

Response:
```json
{
  "status": "running",
  "uptime_seconds": 3600,
  "queries_processed": 1245
}
```

(Note: Availability depends on ServerMain implementation)

## Related Documentation

- **[C++ Binary Reference](./cli.md)** — ServerMain startup options
- **[SPARQL Reference](./sparql.md)** — SPARQL query syntax
- **[How-to: Performance](../how-to/performance.md)** — Optimize queries
- **[Tutorial: Your First Query](../tutorials/02-first-query.md)** — Learn SPARQL

---

**Common Patterns:**

```bash
# Health check
curl -s http://localhost:7023/health | jq .

# Test query
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT (COUNT(*) as ?count) WHERE { ?x ?p ?o } LIMIT 1"

# Save results to file
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type } LIMIT 10000" \
  -H "Accept: text/csv" > results.csv

# Stream results
curl -Gs http://localhost:7023 \
  --data-urlencode "query=SELECT ?x WHERE { ?x a ?type }" \
  -H "Accept: text/csv" | head -100
```

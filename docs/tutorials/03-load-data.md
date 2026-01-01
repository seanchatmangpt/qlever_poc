# Load Your Data

Use your own RDF dataset with QLever. Supports multiple formats and data sources.

## Supported Formats

- **N-Triples** (`.nt`) — Simplest, one triple per line
- **Turtle** (`.ttl`) — More compact, human-readable
- **N-Quads** (`.nq`) — N-Triples with graph names
- **RDF/XML** — Native RDF XML format
- **JSON-LD** — JSON format with RDF semantics
- **Gzip/Bzip2** — Automatic decompression of `.gz` and `.bz2` files

## Step 1: Prepare Your Data

If you have RDF data, you can use it directly. If not, you can:

**Option A: Use a public dataset**
Many datasets are freely available in RDF format:
- [Wikidata](https://www.wikidata.org/wiki/Wikidata:Download) (encyclopedic knowledge)
- [DBpedia](https://www.dbpedia.org/) (Wikipedia in RDF)
- [GeoNames](https://www.geonames.org/ontology/documentation.html) (geographical data)
- [UniProt](https://www.uniprot.org/) (protein database)

**Option B: Create sample RDF data**

Create a file `books.ttl`:
```turtle
@prefix ex: <http://example.com/> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .

ex:book1 rdfs:label "The Great Gatsby"@en ;
         ex:author ex:fitzgerald ;
         ex:year "1925"^^xsd:integer .

ex:fitzgerald rdfs:label "F. Scott Fitzgerald"@en ;
              ex:birthYear "1896"^^xsd:integer .

ex:book2 rdfs:label "To Kill a Mockingbird"@en ;
         ex:author ex:lee ;
         ex:year "1960"^^xsd:integer .

ex:lee rdfs:label "Harper Lee"@en ;
       ex:birthYear "1926"^^xsd:integer .
```

**Option C: Convert from another format**

Use tools like `riot` (from Apache Jena) to convert:
```bash
# Convert N-Triples to Turtle
riot --output=TURTLE data.nt > data.ttl
```

## Step 2: Create a Qleverfile

Create `Qleverfile` in your project directory:

```yaml
# Basic configuration for custom data
data:
  input_files: books.ttl        # Your RDF file(s)
  format: ttl                   # Format: ttl, nt, nq, rdf, jsonld

index:
  name: my-books                # Index name
  description: "My book collection"

server:
  port: 7023
```

**For multiple files:**
```yaml
data:
  input_files:
    - file1.ttl
    - file2.ttl
    - file3.ttl
```

**For compressed files:**
```yaml
data:
  input_files: data.ttl.gz      # Automatically decompressed
```

## Step 3: Build the Index

```bash
qlever index
```

This processes all input files and creates an optimized index. Output shows:
- Number of triples parsed
- Index size and structure
- Estimated query performance

**Progress indicator:**
```
Parsing input... [████████████████] 100%
Building index... [████████████████] 100%
Optimizing... [████████████████] 100%
Index complete: 45,321 triples in 2.3 GB
```

## Step 4: Start Querying

```bash
qlever start
```

Now query your data:
```bash
curl -X POST http://localhost:7023 \
  -H "Content-Type: application/sparql-query" \
  --data "
SELECT ?title ?author WHERE {
  ?book rdfs:label ?title ;
        ex:author ?a .
  ?a rdfs:label ?author .
}
"
```

## Advanced Configuration

### Memory Settings

```yaml
index:
  memory_limit: 8GB              # Max memory for indexing

server:
  memory_limit: 16GB             # Max memory for queries
```

### Performance Optimization

```yaml
index:
  # Create specific permutations for faster queries
  permutations: [SPO, OSP, PSO]
  # Enable compression for large datasets
  use_compression: true
```

### Text Search

Enable full-text search on specific properties:

```yaml
index:
  text_search:
    enabled: true
    predicates:
      - rdfs:label
      - rdfs:comment
```

See [How-to: Text Search](../how-to/text-search.md) for more.

### Spatial Queries

If your data includes geographic points:

```yaml
index:
  spatial_search:
    enabled: true
    location_predicate: ex:location
```

See [How-to: Spatial Queries](../how-to/spatial-queries.md) for details.

## Testing Your Data Load

After indexing, verify the load succeeded:

```bash
# Get statistics about your index
qlever index-info

# Should show something like:
# Index name: my-books
# Triples: 45,321
# Subjects: 12,456
# Predicates: 89
```

## Troubleshooting

**"Out of memory" error during indexing?**
- Reduce `memory_limit` if you set it
- Break into smaller files
- Use compression with `use_compression: true`

**"Unsupported format" error?**
- Check file extension is correct (`.ttl`, `.nt`, `.nq`)
- Or explicitly set `format: ttl` in Qleverfile
- Convert file to N-Triples as fallback format

**Queries are slow?**
- See [How-to: Optimize Performance](../how-to/performance.md)
- Check memory allocation with `qlever index-info`

**Data validation errors?**
- Use `qlever validate-data` to check RDF syntax before indexing
- Check for invalid URIs or literals in your data

## Next Steps

- **Write SPARQL Queries** → [Your First Query](./02-first-query.md)
- **Speed up Queries** → [How-to: Performance](../how-to/performance.md)
- **Add Text Search** → [How-to: Text Search](../how-to/text-search.md)
- **Understand Your Data** → [How-to: Configuration](../how-to/configuration.md)

---

**Tips:**
- Start with smaller datasets while learning
- Use Turtle format (`ttl`) for readability
- Compress large files with gzip to save disk space
- QLever automatically finds the fastest permutations for your data

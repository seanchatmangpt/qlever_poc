# How to: Enable Text Search & Autocompletion

Add full-text search capabilities and query autocompletion to your QLever instance.

## What is Text Search in QLever?

Text search lets you find entities by their content, not just structure:

```sparql
# Without text search (no results if spelling is wrong):
SELECT ?book WHERE {
  ?book rdfs:label "The Great Gatsby" .
}

# With text search (fuzzy matching, scoring):
SELECT ?book ?score WHERE {
  ?book qlever:text \"Great Gatsby\" ?score .
}
# Returns: The Great Gatsby, The Great Gatsby (2013), Great Gatsby's Cafe, etc.
# Sorted by relevance score
```

## Step 1: Enable Text Search During Indexing

Edit your `Qleverfile`:

```yaml
index:
  text_search:
    enabled: true
    # Specify which properties to index for text search
    predicates:
      - rdfs:label
      - rdfs:comment
      - skos:altLabel
```

Then rebuild your index:

```bash
qlever index
```

This adds full-text search capability (takes extra time/space, usually ~20% more).

## Step 2: Query with Text Search

Use the special `qlever:text` predicate:

```sparql
SELECT ?person ?score WHERE {
  ?person qlever:text "Albert Einstein" ?score .
}
ORDER BY DESC(?score)
LIMIT 10
```

Results include:
- Albert Einstein (exact match, highest score)
- Albertus Einsteinii (similar name)
- Einstein, Albert (different word order)

Sorted by relevance.

## Advanced: Combine with Structure

Text search + structural queries:

```sparql
SELECT ?person ?birthPlace ?score WHERE {
  # Text search for the name
  ?person qlever:text "Einstein" ?score .

  # Structural query for properties
  ?person a wd:Q5 .              # Is a person
  ?person wdt:P569 ?birth .      # Has birth date
  FILTER(YEAR(?birth) > 1800)

  ?person wdt:P19 ?birthPlace .  # Born in
}
ORDER BY DESC(?score)
LIMIT 20
```

## Advanced: Text Search Options

### Language-Specific Search

```sparql
SELECT ?item ?score WHERE {
  # Search in German labels only
  ?item qlever:text "Eisenbahn" ?score .
  FILTER(?item IN (?german_items))
}
```

### Prefix Search

Find all items starting with a prefix:

```sparql
SELECT ?item WHERE {
  ?item qlever:text "United*" ?score .
}
# Matches: United States, United Kingdom, United Nations, etc.
```

### Phrase Search

Exact phrase matching:

```sparql
SELECT ?item ?score WHERE {
  ?item qlever:text "\"New York\"" ?score .
}
# Matches phrases, not individual words
```

## Step 3: Enable Autocompletion (Optional)

The web GUI includes smart autocompletion:

```bash
qlever start
# Open http://localhost:7023/gui
# Type in the query box—suggestions appear automatically!
```

This requires text search enabled. As you type, QLever suggests:
- Matching properties (`wdt:P`, `rdfs:`)
- Matching entities (`wd:Q`)
- Valid query completions

## Configuration Options

Fine-tune text search in `Qleverfile`:

```yaml
index:
  text_search:
    enabled: true
    predicates:
      - rdfs:label
      - rdfs:comment

    # How many results to keep from text search
    max_results: 10000

    # Language settings
    language: "en"  # or auto-detect

    # Scoring algorithm: BM25 (default), TF-IDF
    scoring: BM25
```

## Troubleshooting

**"qlever:text is not a valid predicate"**
- Make sure text search is enabled in Qleverfile
- Rebuild the index: `qlever index`

**Autocompletion is slow**
- Reduce the number of text-indexed predicates
- Make sure server has enough memory

**Results are too broad or too narrow**
- Adjust the query to be more specific
- Add `FILTER` conditions for structural constraints
- Use phrase search `"exact phrase"` for precision

## Performance Notes

Text search adds to index size and indexing time:
- Extra indexing time: Usually +20-50%
- Index size increase: Usually +15-30%
- Query performance: Negligible (text search is very fast)

It's worth it if you need full-text search capabilities.

## Next Steps

- **Spatial Queries?** [How-to: Spatial Queries](./spatial-queries.md)
- **Performance?** [How-to: Performance](./performance.md)
- **Configuration?** [How-to: Configuration](./configuration.md)

---

**Tip:** Enable text search from the start when setting up your index. It's hard to add later without rebuilding.

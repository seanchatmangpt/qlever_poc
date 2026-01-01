# How to: Spatial & Geographic Queries

Query geographic data efficiently using coordinates and geometric operations.

## What is Spatial Search?

Find entities based on location:

```sparql
# Find all cities within 50km of Berlin
SELECT ?city ?distance WHERE {
  ?berlin geo:asWKT ?berlinLocation .
  FILTER(?berlin IN (wd:Q64))  # Berlin

  ?city a wd:Q515 .            # Is a city
  ?city geo:asWKT ?cityLocation .

  # Spatial distance calculation
  BIND(ST_Distance(?berlinLocation, ?cityLocation) as ?distance)
  FILTER(?distance < 50)       # Less than 50km
}
ORDER BY ?distance
```

## Prerequisites

Your data needs geographic information in one of these formats:

### Format 1: WKT (Well-Known Text) — Recommended

```turtle
@prefix geo: <http://www.opengis.net/ont/sf#> .
@prefix ex: <http://example.com/> .

ex:berlin geo:asWKT "POINT(13.404954 52.520008)"^^geo:wktLiteral .
ex:paris geo:asWKT "POINT(2.3522 48.8566)"^^geo:wktLiteral .

# Polygon example (for areas)
ex:berlin_area geo:asWKT "POLYGON((13.0 52.0, 14.0 52.0, 14.0 53.0, 13.0 53.0, 13.0 52.0))"^^geo:wktLiteral .
```

### Format 2: Latitude/Longitude Properties

```turtle
@prefix ex: <http://example.com/> .
@prefix schema: <https://schema.org/> .

ex:berlin schema:latitude "52.520008"^^xsd:float ;
          schema:longitude "13.404954"^^xsd:float .
```

## Step 1: Enable Spatial Indexing

Update your `Qleverfile`:

```yaml
index:
  spatial_search:
    enabled: true
    # Map properties to location data
    location_predicates:
      - geo:asWKT
      - schema:geo
```

Rebuild the index:

```bash
qlever index
```

## Step 2: Basic Spatial Queries

### Find Distances

```sparql
PREFIX geo: <http://www.opengis.net/ont/sf#>

SELECT ?place1 ?place2 ?distance WHERE {
  ?place1 geo:asWKT ?loc1 .
  ?place2 geo:asWKT ?loc2 .

  BIND(ST_Distance(?loc1, ?loc2) as ?distance)
  FILTER(?place1 < ?place2)  # Avoid duplicates
  FILTER(?distance < 100)    # Within 100 km
}
ORDER BY ?distance
LIMIT 10
```

### Find Points Within Radius

```sparql
PREFIX geo: <http://www.opengis.net/ont/sf#>

SELECT ?city WHERE {
  # Reference point (Berlin)
  ?berlin geo:asWKT "POINT(13.404954 52.520008)"^^geo:wktLiteral .

  # Find all cities
  ?city a wd:Q515 .
  ?city geo:asWKT ?cityLocation .

  # Within 50km
  BIND(ST_Distance(
    ?berlin,
    ?cityLocation
  ) as ?dist)
  FILTER(?dist < 50)
}
```

### Check if Points are Inside Area

```sparql
PREFIX geo: <http://www.opengis.net/ont/sf#>

SELECT ?city WHERE {
  # Define search area (Europe's bounding box, simplified)
  BIND("POLYGON((-10 35, 40 35, 40 70, -10 70, -10 35))"^^geo:wktLiteral as ?europe)

  ?city a wd:Q515 .
  ?city geo:asWKT ?location .

  # Check if inside
  FILTER(ST_Within(?location, ?europe))
}
```

## Spatial Functions

### Distance Functions

```sparql
# Distance between two points (in meters)
BIND(ST_Distance(?point1, ?point2) as ?dist)

# Distance in kilometers
BIND(ST_Distance(?point1, ?point2) / 1000 as ?distKm)

# Great-circle distance (more accurate for Earth)
BIND(ST_GreatCircleDistance(?lat1, ?lon1, ?lat2, ?lon2) as ?dist)
```

### Area Functions

```sparql
# Check if point is in polygon
FILTER(ST_Within(?point, ?polygon))

# Check if polygons overlap
FILTER(ST_Intersects(?poly1, ?poly2))

# Get area of a polygon (square meters)
BIND(ST_Area(?polygon) as ?area)
```

### Geometry Functions

```sparql
# Center point of a geometry
BIND(ST_Centroid(?geometry) as ?center)

# Bounding box
BIND(ST_Envelope(?geometry) as ?bbox)

# Buffer around a point
BIND(ST_Buffer(?point, 5000) as ?circle)  # 5km buffer
```

## Real-World Example: Find Nearby Restaurants

```sparql
PREFIX geo: <http://www.opengis.net/ont/sf#>
PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>

SELECT ?restaurant ?name ?distance WHERE {
  # My location (San Francisco)
  BIND("POINT(-122.4194 37.7749)"^^geo:wktLiteral as ?myLocation)

  # Find restaurants
  ?restaurant a wd:Q1201623 .           # Is a restaurant
  ?restaurant rdfs:label ?name .
  ?restaurant geo:asWKT ?location .

  # Calculate distance
  BIND(ST_Distance(?myLocation, ?location) as ?distance)
  FILTER(?distance < 10000)  # Within 10km

  FILTER(LANG(?name) = "en")
}
ORDER BY ?distance
LIMIT 20
```

## Configuration for Best Performance

```yaml
index:
  spatial_search:
    enabled: true
    location_predicates:
      - geo:asWKT
      - schema:geo

    # Spatial index type
    index_type: quadtree  # or "rtree"

    # How many spatial levels to index
    max_levels: 16

    # For large datasets
    use_compression: true
```

## Troubleshooting

**"Invalid WKT format" error**
- Check geometry syntax: `POINT(lng lat)` (longitude first!)
- Make sure coordinates are valid: longitude [-180, 180], latitude [-90, 90]

**Spatial queries are slow**
- Make sure spatial indexing is enabled
- Verify index was built with spatial support: `qlever index-info`
- Check that location data is indexed

**Distance results seem wrong**
- Verify you're using longitude, latitude (not lat, lng)
- Check coordinate system matches (usually WGS84)
- For Wikidata, coordinates are longitude first, then latitude

**Memory usage is high**
- Spatial indexes are memory-intensive
- Try disabling if not needed
- Or increase server memory allocation

## Performance Notes

Spatial queries are highly optimized in QLever:
- Index uses S2 geometry for efficient spatial operations
- Distance queries are nearly as fast as non-spatial queries
- Complex polygon operations may be slower

Typical performance:
- Distance within radius: <100ms for millions of points
- Polygon membership: <50ms for thousands of polygons
- Nearest neighbor: <200ms

## Next Steps

- **Performance Issues?** [How-to: Performance](./performance.md)
- **Text Search?** [How-to: Text Search](./text-search.md)
- **Configuration?** [How-to: Configuration](./configuration.md)

---

**Pro Tip:** For best results, pre-process your geographic data to ensure correct formats before loading into QLever.

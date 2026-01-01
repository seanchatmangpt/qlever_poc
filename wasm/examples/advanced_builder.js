// Advanced QueryBuilder example demonstrating SPARQL 1.1 features
// This example shows how to use the enhanced QueryBuilder with aggregations,
// UNION queries, and advanced filters.

const qlever = require('../pkg/index.js');

// Example 1: Aggregation Functions
console.log("=== Example 1: Aggregation Functions ===");
const aggregationQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?type (COUNT(?s) as ?count) (AVG(?score) as ?avg_score)")
    .where_clause("?s a ?type . ?s :score ?score")
    .group_by("?type")
    .order_by_desc("?count")
    .limit(10)
    .build();

console.log("Aggregation Query:");
console.log(aggregationQuery);
console.log("");

// Example 2: UNION Queries
console.log("=== Example 2: UNION Queries ===");
const peopleQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?name")
    .where_clause("?x a :Person . ?x :name ?name");

const organizationQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?name")
    .where_clause("?x a :Organization . ?x :name ?name");

const unionQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?name")
    .where_clause("?x a :Person . ?x :name ?name")
    .union(organizationQuery)
    .limit(50)
    .build();

console.log("UNION Query:");
console.log(unionQuery);
console.log("");

// Example 3: EXISTS and NOT EXISTS filters
console.log("=== Example 3: EXISTS and NOT EXISTS Filters ===");
const filterQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?person ?name")
    .where_clause("?person a :Person . ?person :name ?name")
    .filter_exists("?person :worksFor ?company")
    .filter_not_exists("?person :retired true")
    .build();

console.log("Filter Query with EXISTS/NOT EXISTS:");
console.log(filterQuery);
console.log("");

// Example 4: Property Paths
console.log("=== Example 4: Property Paths ===");
const propertyPathQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?person ?friend ?distance")
    .where_clause("?person :knows ?friend")
    .property_path("?person", "foaf:knows+", "?distant_friend")
    .limit(20)
    .build();

console.log("Property Path Query:");
console.log(propertyPathQuery);
console.log("");

// Example 5: GROUP_CONCAT and Multiple Aggregations
console.log("=== Example 5: GROUP_CONCAT Aggregation ===");
const groupConcatQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?author")
    .where_clause("?book :author ?author . ?book :title ?title")
    .group_by("?author")
    .group_concat("?title", ", ")
    .count("?title")
    .build();

console.log("GROUP_CONCAT Query:");
console.log(groupConcatQuery);
console.log("");

// Example 6: HAVING clause (via FILTER)
console.log("=== Example 6: HAVING Clause ===");
const havingQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?type (COUNT(?s) as ?count)")
    .where_clause("?s a ?type")
    .group_by("?type")
    .having("COUNT(?s) > 100")
    .order_by_desc("?count")
    .build();

console.log("Query with HAVING:");
console.log(havingQuery);
console.log("");

// Example 7: Complex Multi-Feature Query
console.log("=== Example 7: Complex Multi-Feature Query ===");
const complexQuery = new qlever.QueryBuilder()
    .select_query()
    .select("?category (COUNT(?product) as ?product_count) (AVG(?price) as ?avg_price)")
    .where_clause("?product :category ?category . ?product :price ?price")
    .filter("?price > 10")
    .filter_exists("?product :inStock true")
    .group_by("?category")
    .having("COUNT(?product) > 5")
    .order_by_desc("?product_count")
    .limit(50)
    .distinct()
    .build();

console.log("Complex Query with Multiple Features:");
console.log(complexQuery);
console.log("");

console.log("✓ All examples generated successfully!");

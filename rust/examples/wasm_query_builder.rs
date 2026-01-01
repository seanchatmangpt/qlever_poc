//! WASM QueryBuilder with SPARQL 1.1 features
//!
//! This example demonstrates the enhanced QueryBuilder with:
//! - UNION queries
//! - MINUS queries
//! - GROUP BY with aggregations
//! - HAVING clauses
//! - Property paths
//! - FILTER EXISTS/NOT EXISTS
//!
//! Run with: cargo run --example wasm_query_builder

#[cfg(target_arch = "wasm32")]
fn main() {
    use qlever::wasm::QueryBuilder;

    // Basic SELECT query
    println!("=== Basic SELECT ===");
    let query = QueryBuilder::new()
        .select("?name ?age")
        .where_clause("?person <http://example.org/name> ?name .")
        .where_clause("?person <http://example.org/age> ?age .")
        .filter("?age > 18")
        .limit(10)
        .to_sparql();
    println!("{}\n", query);

    // UNION query
    println!("=== UNION Query ===");
    let employees = QueryBuilder::new()
        .select("?person ?label")
        .where_clause("?person a <http://example.org/Employee> .")
        .where_clause("?person <http://www.w3.org/2000/01/rdf-schema#label> ?label .");

    let contractors = QueryBuilder::new()
        .select("?person ?label")
        .where_clause("?person a <http://example.org/Contractor> .")
        .where_clause("?person <http://www.w3.org/2000/01/rdf-schema#label> ?label .");

    let query = QueryBuilder::new()
        .select("?person ?label")
        .union(&employees)
        .union(&contractors)
        .to_sparql();
    println!("{}\n", query);

    // GROUP BY with aggregation
    println!("=== GROUP BY with Aggregation ===");
    let query = QueryBuilder::new()
        .select("?department")
        .count("?employee", "?count")
        .where_clause("?employee <http://example.org/department> ?department .")
        .group_by("?department")
        .order_by("?count", false)  // DESC
        .to_sparql();
    println!("{}\n", query);

    // Property paths
    println!("=== Property Paths ===");
    let query = QueryBuilder::new()
        .select("?ancestor ?descendant")
        .property_path("?descendant", "^<http://example.org/parent>+", "?ancestor")
        .limit(100)
        .to_sparql();
    println!("{}\n", query);

    // FILTER EXISTS
    println!("=== FILTER EXISTS ===");
    let query = QueryBuilder::new()
        .select("?person ?name")
        .where_clause("?person a <http://example.org/Person> .")
        .where_clause("?person <http://example.org/name> ?name .")
        .filter_exists("?person <http://example.org/phoneNumber> ?phone")
        .to_sparql();
    println!("{}\n", query);

    // Complex query with multiple features
    println!("=== Complex Query ===");
    let query = QueryBuilder::new()
        .distinct()
        .select("?department ?avgSalary ?maxSalary")
        .where_clause("?employee <http://example.org/department> ?department .")
        .where_clause("?employee <http://example.org/salary> ?salary .")
        .avg("?salary", "?avgSalary")
        .max("?salary", "?maxSalary")
        .filter_exists("?employee <http://example.org/active> true")
        .group_by("?department")
        .having("?avgSalary > 50000")
        .order_by("?avgSalary", false)
        .limit(50)
        .to_sparql();
    println!("{}\n", query);
}

#[cfg(not(target_arch = "wasm32"))]
fn main() {
    use qlever::wasm::QueryBuilder;

    println!("WASM QueryBuilder Examples (non-WASM runtime)");
    println!("============================================\n");

    // Basic SELECT query
    println!("=== Basic SELECT ===");
    let query = QueryBuilder::new()
        .select("?name ?age")
        .where_clause("?person <http://example.org/name> ?name .")
        .where_clause("?person <http://example.org/age> ?age .")
        .filter("?age > 18")
        .limit(10)
        .to_sparql();
    println!("{}\n", query);

    // UNION query
    println!("=== UNION Query ===");
    let employees = QueryBuilder::new()
        .select("?person ?label")
        .where_clause("?person a <http://example.org/Employee> .")
        .where_clause("?person <http://www.w3.org/2000/01/rdf-schema#label> ?label .");

    let contractors = QueryBuilder::new()
        .select("?person ?label")
        .where_clause("?person a <http://example.org/Contractor> .")
        .where_clause("?person <http://www.w3.org/2000/01/rdf-schema#label> ?label .");

    let query = QueryBuilder::new()
        .select("?person ?label")
        .union(&employees)
        .union(&contractors)
        .to_sparql();
    println!("{}\n", query);

    // GROUP BY with aggregation
    println!("=== GROUP BY with COUNT ===");
    let query = QueryBuilder::new()
        .select("?department")
        .count("?employee", "?count")
        .where_clause("?employee <http://example.org/department> ?department .")
        .group_by("?department")
        .order_by("?count", false)
        .to_sparql();
    println!("{}\n", query);

    // Property paths
    println!("=== Property Paths ===");
    let query = QueryBuilder::new()
        .select("?ancestor ?descendant")
        .property_path("?descendant", "^<http://example.org/parent>+", "?ancestor")
        .limit(100)
        .to_sparql();
    println!("{}\n", query);

    // FILTER EXISTS
    println!("=== FILTER EXISTS ===");
    let query = QueryBuilder::new()
        .select("?person ?name")
        .where_clause("?person a <http://example.org/Person> .")
        .where_clause("?person <http://example.org/name> ?name .")
        .filter_exists("?person <http://example.org/phoneNumber> ?phone")
        .to_sparql();
    println!("{}\n", query);

    // Complex aggregations
    println!("=== Multiple Aggregations ===");
    let query = QueryBuilder::new()
        .select("?category")
        .sum("?amount", "?total")
        .avg("?amount", "?average")
        .count("?transaction", "?txcount")
        .where_clause("?transaction <http://example.org/category> ?category .")
        .where_clause("?transaction <http://example.org/amount> ?amount .")
        .group_by("?category")
        .having("?total > 10000")
        .order_by("?total", false)
        .to_sparql();
    println!("{}\n", query);

    println!("✓ All examples generated successfully!");
}

//! Example showing how to use the QLever in-memory SPARQL database

use qlever::{NamedNode, Store, Term, Triple};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create an in-memory store
    let store = Store::new();

    // Create some test data
    println!("Adding triples to the store...");

    // Triple: Alice knows Bob
    let alice = NamedNode::new("http://example.org/alice".to_string())?;
    let bob = NamedNode::new("http://example.org/bob".to_string())?;
    let knows = NamedNode::new("http://example.org/knows".to_string())?;
    store.insert(Triple::new(
        alice.clone(),
        knows.clone(),
        Term::NamedNode(bob.clone()),
    ))?;

    // Triple: Bob knows Carol
    let carol = NamedNode::new("http://example.org/carol".to_string())?;
    store.insert(Triple::new(
        bob.clone(),
        knows.clone(),
        Term::NamedNode(carol.clone()),
    ))?;

    // Triple: Alice's name is "Alice"
    let name_predicate = NamedNode::new("http://example.org/name".to_string())?;
    store.insert(Triple::new(
        alice,
        name_predicate,
        Term::Literal(qlever::Literal::new_simple("Alice")),
    ))?;

    println!("Added 3 triples to the store\n");

    // Query all triples
    println!("All triples in the store:");
    let all_triples = store.query_triples(None, None, None)?;
    for triple in &all_triples {
        println!("  {}", triple);
    }

    // Query triples with specific predicate
    println!("\nTriples with 'knows' predicate:");
    let knows_triples = store.query_triples(None, Some(knows.as_str()), None)?;
    for triple in &knows_triples {
        println!("  {}", triple);
    }

    // Get vocabulary
    println!("\nSubjects in the store:");
    for subject in store.subjects()? {
        println!("  {}", subject);
    }

    println!("\nPredicates in the store:");
    for predicate in store.predicates()? {
        println!("  {}", predicate);
    }

    println!("\nObjects in the store:");
    for object in store.objects()? {
        println!("  {}", object);
    }

    println!("\nTotal triples: {}", store.triple_count());

    Ok(())
}

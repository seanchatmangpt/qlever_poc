# QLever Datalog Guide

## Table of Contents

1. [Introduction](#introduction)
2. [What is Datalog?](#what-is-datalog)
3. [Why Datalog in QLever?](#why-datalog-in-qlever)
4. [Datalog vs SPARQL](#datalog-vs-sparql)
5. [Quick Start](#quick-start)
6. [Supported Features](#supported-features)
7. [Limitations](#limitations)
8. [Architecture Overview](#architecture-overview)
9. [Further Reading](#further-reading)

---

## Introduction

QLever extends its powerful SPARQL query engine with **Datalog** support, enabling users to define recursive rules and complex inference patterns that seamlessly integrate with RDF data. This guide introduces Datalog in QLever and helps you get started quickly.

**What You'll Learn:**
- What Datalog is and why it's useful
- How Datalog compares to SPARQL
- How to write and execute Datalog rules in QLever
- Key features and current limitations

---

## What is Datalog?

**Datalog** is a declarative logic programming language based on a subset of Prolog. It excels at expressing recursive queries and inference rules in a simple, readable syntax.

### Core Concepts

**Rules**: Datalog programs consist of rules that derive new facts from existing data:

```datalog
# Rule syntax: head(Args) :- body1, body2, ...
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```

**Facts**: Ground truths stored in your RDF database:

```turtle
:Alice :parentOf :Bob .
:Bob :parentOf :Carol .
```

**Queries**: Questions you ask about the data:

```sparql
SELECT ?anc ?desc WHERE {
  ?anc <http://example.org/ancestor> ?desc .
}
```

**Inference**: The system automatically derives new facts by applying rules:

```
Input facts:  parent(Alice, Bob), parent(Bob, Carol)
After rules:  ancestor(Alice, Bob), ancestor(Bob, Carol), ancestor(Alice, Carol)
```

---

## Why Datalog in QLever?

### Advantages Over Pure SPARQL

1. **Recursive Queries Made Simple**
   - SPARQL 1.1 has limited recursion support (property paths only)
   - Datalog makes recursion natural and expressive

2. **Declarative Inference**
   - Define rules once, apply everywhere
   - No need to manually write complex recursive SPARQL queries

3. **Ontology Reasoning**
   - Express RDFS/OWL inference rules in pure Datalog
   - Custom inference rules for domain-specific logic

4. **Performance**
   - QLever's Datalog engine uses optimized fixpoint computation
   - Leverages existing SPARQL query optimization infrastructure
   - Semi-naive evaluation prevents redundant computation

5. **Seamless Integration**
   - Datalog rules integrate directly with SPARQL queries
   - Use rule predicates alongside RDF triples
   - Same index structures, same performance characteristics

### Use Cases

- **Graph Reachability**: Shortest paths, transitive closure, connected components
- **Family Trees**: Ancestor/descendant relationships, cousins, degrees of separation
- **Organizational Hierarchies**: Reporting chains, management levels, team structures
- **Social Networks**: Friends-of-friends, influence propagation, community detection
- **Ontology Inference**: Subclass hierarchies, property inheritance, domain reasoning
- **Access Control**: Permission inheritance, role hierarchies
- **Supply Chains**: Part dependencies, supplier networks, logistics planning

---

## Datalog vs SPARQL

### Side-by-Side Comparison

**Task**: Find all ancestors (transitive closure of parent relationship)

#### SPARQL 1.1 Property Path
```sparql
SELECT ?ancestor ?descendant WHERE {
  ?ancestor <http://example.org/parentOf>+ ?descendant .
}
```

**Limitations:**
- Only works for simple transitive properties
- Cannot express complex join patterns
- Limited to built-in path operators (*, +, ?)
- Cannot bind intermediate variables

#### Datalog Rules
```datalog
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```

**Advantages:**
- Explicit rule definition
- Can reference intermediate variables (?y)
- Supports arbitrary complex patterns
- Easy to extend with additional constraints

### When to Use Each

| Use Case | Best Choice | Reason |
|----------|-------------|--------|
| Simple triple patterns | SPARQL | Direct and efficient |
| Aggregations, grouping | SPARQL | Built-in aggregate functions |
| Transitive closure | Datalog | Clean recursive syntax |
| Complex inference | Datalog | Rule composition and reuse |
| Graph algorithms | Datalog | Natural recursive expression |
| Standard queries | SPARQL | Industry standard, widely supported |
| Custom reasoning | Datalog | Flexible rule definitions |

### Hybrid Approach (Recommended)

Combine both for maximum power:

```datalog
# Define inference rules in Datalog
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```

```sparql
# Use rules in SPARQL queries
SELECT ?name ?ancestorCount WHERE {
  ?person <http://example.org/name> ?name .
  ?person <http://example.org/ancestor> ?anc .
}
GROUP BY ?name
HAVING (COUNT(?anc) > 5)
ORDER BY DESC(?ancestorCount)
```

---

## Quick Start

### Step 1: Prepare Your RDF Data

Load RDF data into QLever as usual:

```turtle
@prefix : <http://example.org/> .

:Alice :parentOf :Bob .
:Bob :parentOf :Carol .
:Carol :parentOf :David .
```

### Step 2: Define Datalog Rules

Create a Datalog program file (`rules.datalog`):

```datalog
# Base case: direct parent is an ancestor
ancestor(?x, ?y) :- parent(?x, ?y).

# Recursive case: ancestor of ancestor is an ancestor
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Define parent in terms of RDF predicate
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
```

### Step 3: Load Rules into QLever

**C++ API:**
```cpp
#include "parser/DatalogParser.h"
#include "parser/RuleDatabase.h"

// Parse rules from file
auto program = DatalogParser::parseDatalogProgram(rulesText);
auto ruleDatabase = std::make_shared<RuleDatabase>();
for (const auto& rule : program.ruleDatabase.getAllRules()) {
  ruleDatabase->addRule(rule);
}

// Attach to query execution context
qec->setRuleDatabase(ruleDatabase);
```

**Future CLI Support (planned):**
```bash
# Load rules when starting server
qlever start --datalog-rules rules.datalog
```

### Step 4: Query with Rule Predicates

Use rule predicates in SPARQL queries:

```sparql
# Query using the ancestor rule
SELECT ?ancestor ?descendant WHERE {
  ?ancestor <http://example.org/ancestor> ?descendant .
}
```

**Results:**
```
?ancestor  | ?descendant
-----------|------------
:Alice     | :Bob
:Alice     | :Carol
:Alice     | :David
:Bob       | :Carol
:Bob       | :David
:Carol     | :David
```

### Step 5: Combine with Regular SPARQL

```sparql
# Find people with many ancestors
SELECT ?person (COUNT(?anc) AS ?ancestorCount) WHERE {
  ?person a :Person .
  ?anc <http://example.org/ancestor> ?person .
}
GROUP BY ?person
ORDER BY DESC(?ancestorCount)
```

---

## Supported Features

### ✅ Currently Supported

1. **Rule Definitions**
   - Head-body rule syntax: `head(Args) :- body1, body2, ...`
   - Multiple rules per predicate
   - Recursive rules (transitive closure, etc.)

2. **Variables**
   - SPARQL-style variables: `?x`, `?VarName`
   - Variable sharing across atoms
   - Existential variables (appear only in body)

3. **Predicates**
   - Custom Datalog predicates
   - RDF predicates (IRIs): `?x <http://example.org/prop> ?y`
   - Mixing both in rule bodies

4. **Terms**
   - Variables: `?x`
   - IRIs: `<http://example.org/resource>`
   - String literals: `"text"`, `"text"@en`, `"text"^^<datatype>`

5. **Recursion**
   - Direct recursion: rule references itself
   - Indirect recursion: mutual recursion through other rules
   - Fixpoint computation with semi-naive evaluation

6. **Optimization**
   - Cost-based query planning
   - Integration with SPARQL optimizer
   - Efficient join ordering
   - Index-backed evaluation

7. **Comments**
   - Line comments: `# This is a comment`
   - C-style comments: `/* Multi-line comment */`

### 🚧 Planned Features (Future Releases)

1. **Stratified Negation**
   ```datalog
   notAncestor(?x, ?y) :- person(?x), person(?y), NOT ancestor(?x, ?y).
   ```

2. **Aggregation in Rules**
   ```datalog
   avgChildAge(?parent, ?avg) :- parent(?parent, ?child),
                                  ?child :age ?age,
                                  AVG(?age, ?avg).
   ```

3. **Built-in Predicates**
   ```datalog
   adult(?person) :- ?person :age ?age, ?age >= 18.
   ```

4. **Materialized Views**
   - Pre-compute and cache rule results
   - Incremental maintenance on data updates

---

## Limitations

### Current Limitations

1. **No Stratified Negation** (yet)
   - Cannot use `NOT` in rule bodies
   - Workaround: Use SPARQL `FILTER NOT EXISTS` in base patterns

2. **No Aggregation in Rules** (yet)
   - Cannot use `COUNT`, `SUM`, etc. in rule heads
   - Workaround: Use SPARQL aggregation in final query

3. **Limited Built-ins** (yet)
   - No arithmetic comparisons in rules
   - No string functions in rules
   - Workaround: Use SPARQL filters in base patterns

4. **Performance Considerations**
   - Very large transitive closures may be slow
   - Iteration limit (default 1000) prevents infinite loops
   - Memory usage grows with result size

5. **No Dynamic Rule Loading** (yet)
   - Rules must be loaded at server startup (in current API)
   - Cannot add/remove rules at runtime
   - Planned for future release

### Semantic Differences from Traditional Datalog

1. **Open World Assumption**
   - QLever uses RDF's open world semantics
   - Traditional Datalog uses closed world assumption

2. **RDF Integration**
   - Variables can bind to any RDF term (IRI, literal, blank node)
   - Must be careful with literal/IRI mixing

3. **Blank Nodes**
   - Blank nodes are treated as distinct entities
   - No skolemization or blank node unification across rules

---

## Architecture Overview

### How It Works

```
┌─────────────────────────────────────────────────────────────┐
│                    SPARQL Query                              │
│  SELECT ?x ?y WHERE { ?x ancestor(?y) }                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              DatalogQueryPlanner                             │
│  - Detects rule predicates in query                         │
│  - Chooses evaluation strategy                              │
│    • Non-recursive: RuleExpansion                           │
│    • Recursive: FixpointComputation                         │
└────────────────────┬────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
┌────────────────┐      ┌──────────────────────┐
│ RuleExpansion  │      │ FixpointComputation  │
│ (Non-recursive)│      │   (Recursive)        │
│                │      │                      │
│ - Map head     │      │ - Iterate until      │
│   variables    │      │   fixpoint           │
│ - Expand body  │      │ - Semi-naive eval    │
│ - Build exec   │      │ - Deduplication      │
│   tree         │      │ - Iteration limit    │
└────────┬───────┘      └──────────┬───────────┘
         │                         │
         └────────────┬────────────┘
                      ▼
         ┌────────────────────────┐
         │  QueryExecutionTree    │
         │  (Standard SPARQL ops) │
         │  - IndexScan           │
         │  - Join                │
         │  - Filter              │
         └────────┬───────────────┘
                  ▼
         ┌────────────────────────┐
         │   IdTable Results      │
         └────────────────────────┘
```

### Key Components

1. **RuleDatabase**: Thread-safe storage for all Datalog rules
2. **DatalogParser**: Parses Datalog syntax into DatalogRule objects
3. **DatalogQueryPlanner**: Detects and plans Datalog predicates in SPARQL queries
4. **RuleExpansion**: Executes non-recursive rules (single expansion)
5. **FixpointComputation**: Executes recursive rules (iterative fixpoint)

### Evaluation Strategies

**Non-Recursive Rules** → RuleExpansion:
```datalog
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
```
- Single expansion
- Direct translation to SPARQL patterns
- Efficient as regular index scans

**Recursive Rules** → FixpointComputation:
```datalog
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```
- Iterative evaluation
- Iteration i: apply rules to results from iteration i-1
- Stop when no new facts (fixpoint reached)
- Semi-naive optimization (only use new facts)

---

## Further Reading

### Documentation

- **[Datalog Syntax Reference](DATALOG_SYNTAX.md)** — Complete syntax guide
- **[Datalog Examples](DATALOG_EXAMPLES.md)** — Real-world use cases with complete code
- **[Datalog Performance Guide](DATALOG_PERFORMANCE.md)** — Optimization tips and benchmarks
- **[Datalog C++ API](DATALOG_API.md)** — Programming guide for developers

### Academic References

1. **Datalog Foundations**
   - Abiteboul, S., Hull, R., & Vianu, V. (1995). *Foundations of Databases*. Addison-Wesley.

2. **Semi-Naive Evaluation**
   - Bancilhon, F., & Ramakrishnan, R. (1986). "An amateur's introduction to recursive query processing strategies". *ACM SIGMOD*.

3. **Datalog in Practice**
   - Huang, S. S., Green, T. J., & Loo, B. T. (2011). "Datalog and emerging applications: an interactive tutorial". *ACM SIGMOD*.

### External Resources

- **[Datalog Educational System](http://des.sourceforge.net/)** — Online Datalog interpreter
- **[Soufflé Datalog](https://souffle-lang.github.io/)** — High-performance Datalog compiler
- **[Datomic](https://www.datomic.com/)** — Commercial database with Datalog queries

### QLever Resources

- **[QLever Documentation](../INDEX.md)** — Main documentation index
- **[SPARQL Tutorial](../tutorials/)** — SPARQL basics
- **[QLever Performance](https://qlever.dev/evaluation)** — Performance benchmarks

---

## Next Steps

1. **Learn the syntax**: Read [DATALOG_SYNTAX.md](DATALOG_SYNTAX.md)
2. **Try examples**: Work through [DATALOG_EXAMPLES.md](DATALOG_EXAMPLES.md)
3. **Optimize queries**: Study [DATALOG_PERFORMANCE.md](DATALOG_PERFORMANCE.md)
4. **Integrate in code**: See [DATALOG_API.md](DATALOG_API.md)

---

**Questions or feedback?** Open an issue on [GitHub](https://github.com/ad-freiburg/qlever/issues) or start a [discussion](https://github.com/ad-freiburg/qlever/discussions).

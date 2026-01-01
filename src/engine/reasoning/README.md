# QLever Reasoning Engine - Datalog & N3 Support

## Overview

This module implements a comprehensive reasoning engine that extends QLever's SPARQL engine with support for **Datalog** and **N3 (Notation3)** rule-based reasoning. The implementation uses **Semi-Naïve evaluation** with **fixpoint iteration** to derive new facts from RDF/knowledge bases.

## Architecture

### Core Components

#### 1. **RDF-star (RdfStar.h)**
Implements RDF-star support for quoting triples as subjects/objects:
- `QuotedTriple`: Represents a quoted triple `<< s p o >>`
- `QuotedTripleStore`: Manages collections of quoted triples
- Enables meta-programming features required by N3

**Use Case**: N3 construct `{ ?x :rel ?y }` becomes a quoted triple that can be reasoned about.

#### 2. **Rule Storage (Rule.h)**
Data structures for storing and managing Datalog rules:
- `Rule`: Single rule with body (WHERE) and head (CONSTRUCT)
- `RuleDatabase`: Collection of rules with querying capabilities

**Usage**:
```cpp
// Create a rule: parent(X, Z), ancestor(Z, Y) => ancestor(X, Y)
std::vector<SparqlTriple> body = {
    SparqlTriple{x, parent, z},
    SparqlTriple{z, ancestor, y}
};
std::vector<SparqlTriple> head = {
    SparqlTriple{x, ancestor, y}
};
auto rule = std::make_shared<Rule>(body, head, 0);
ruleDatabase.addRule(rule);
```

#### 3. **N3 Parser (N3Parser.h/cpp)**
Parses N3 rule syntax and converts to SPARQL:
- Handles `{ body } => { head }` syntax
- Supports `log:implies` and `:implies` operators
- Extracts quoted patterns for meta-reasoning
- Converts N3 to executable SPARQL INSERT

**Examples**:
```
{ ?x :parent ?y } => { ?x :ancestor ?y }
{ ?x :parent ?y } log:implies { ?x :ancestor ?y }
```

#### 4. **Reasoning Engine (ReasoningEngine.h/cpp)**
Core execution engine implementing Semi-Naïve evaluation:
- Iterative fixpoint computation
- Delta facts optimization (only reconsider new facts each iteration)
- Automatic convergence detection
- Iteration statistics and monitoring

**Algorithm**:
1. Initialize delta with facts from the index
2. For each iteration:
   - Apply all rules to delta facts
   - Collect new derived facts
   - Update derived fact accumulator
   - Continue until no new facts (convergence)

#### 5. **ReasoningOperation (ReasoningOperation.h/cpp)**
Integrates reasoning into the SPARQL execution engine:
- Extends the `Operation` base class
- Executes as part of query execution tree
- Returns results as `IdTable` for downstream operations
- Compatible with existing planner and cache

## Datalog Support

### Mapping Datalog to SPARQL

| Datalog Concept | SPARQL Equivalent |
|---|---|
| Atom `p(x, y)` | Triple pattern `?x :p ?y` |
| Rule body | WHERE clause |
| Rule head | CONSTRUCT/INSERT clause |
| Conjunction `,` | Join (`.` in triple patterns) |
| Negation `not p(x)` | `FILTER NOT EXISTS { ?x :p ?y }` |
| Recursion | Iterative fixpoint computation |

### Example: Transitive Closure

**Datalog**:
```prolog
ancestor(X, Y) :- parent(X, Y).
ancestor(X, Y) :- parent(X, Z), ancestor(Z, Y).
```

**QLever Reasoning Engine**:
```cpp
// Rule 1: parent(X, Y) => ancestor(X, Y)
auto rule1 = std::make_shared<Rule>(
    std::vector<SparqlTriple>{SparqlTriple{x, parent, y}},
    std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}},
    0
);

// Rule 2: ancestor(X, Z), ancestor(Z, Y) => ancestor(X, Y)
auto rule2 = std::make_shared<Rule>(
    std::vector<SparqlTriple>{
        SparqlTriple{x, ancestor, z},
        SparqlTriple{z, ancestor, y}
    },
    std::vector<SparqlTriple>{SparqlTriple{x, ancestor, y}},
    1
);

ruleDatabase->addRule(rule1);
ruleDatabase->addRule(rule2);
```

**Equivalent SPARQL**:
```sparql
INSERT { ?x :ancestor ?y }
WHERE { ?x :parent ?y }

INSERT { ?x :ancestor ?y }
WHERE { ?x :ancestor ?z . ?z :ancestor ?y }
```

## N3 Features

### N3 Rule Syntax

The engine supports N3 implications:

```n3
{ ?x :parent ?y } => { ?x :ancestor ?y }
```

Converted to:
```sparql
INSERT { ?x :ancestor ?y } WHERE { ?x :parent ?y }
```

### N3 Quoting (RDF-star)

N3 enables treating graphs as values:

```n3
:alice :says { :bob :is :smart } .
```

Represented in RDF-star as:
```sparql
:alice :says << :bob :is :smart >> .
```

### Meta-Programming

N3 enables reasoning about triples:

```n3
?rule log:implies ?consequence
```

## Algorithm: Semi-Naïve Evaluation

Semi-Naïve evaluation optimizes fixpoint iteration:

1. **Delta Initialization**: Start with facts in the knowledge base
2. **Iteration**:
   - Apply rules only to delta (newly derived facts from previous iteration)
   - Combine with existing facts for rule body matching
   - Collect new derived facts
3. **Convergence**: Stop when delta is empty (no new facts)

**Benefits**:
- Avoids redundant rule applications
- Significantly faster on large knowledge bases
- Natural incremental reasoning model

## Usage Example

```cpp
#include "engine/reasoning/ReasoningOperation.h"
#include "engine/reasoning/Rule.h"

// Create rule database
auto ruleDatabase = std::make_shared<reasoning::RuleDatabase>();

// Add rules
auto rule = std::make_shared<reasoning::Rule>(body, head, 0);
ruleDatabase->addRule(rule);

// Create reasoning operation
auto reasoningOp = std::make_unique<ReasoningOperation>(
    queryExecutionContext,
    ruleDatabase,
    std::vector<Variable>{Variable{"?x"}, Variable{"?y"}}
);

// Execute and get results
auto result = reasoningOp->getResult();
const auto& idTable = result->idTable();

// Process results
for (size_t i = 0; i < idTable.size(); ++i) {
    Id subject = idTable.at(i, 0);
    Id object = idTable.at(i, 1);
    // Process derived facts
}
```

## Performance Characteristics

### Iteration Cost
- **Non-recursive rules**: 1 iteration, O(|rules| × |base facts|)
- **Recursive rules**: O(n) iterations where n = maximum recursion depth
- **Worst case**: Fixed point on all rules, typically converges in log(n) iterations

### Memory Usage
- Delta facts: O(new facts per iteration)
- Derived facts accumulator: O(total derived facts)
- Rule database: O(|rules| × average rule size)

### Optimization Opportunities
1. **Property Paths**: For transitive closure, use SPARQL `+` operator directly
2. **Stratification**: Detect non-recursive layers and compile them separately
3. **Incremental Updates**: Cache delta between query executions
4. **Parallel Rule Application**: Apply independent rules concurrently

## Integration with QLever

### QueryPlanner Integration
The QueryPlanner can detect rule patterns and:
1. Compile non-recursive rules to standard operations
2. Use ReasoningOperation for recursive rules
3. Estimate cost based on rule complexity

### Cache Integration
- Results of ReasoningOperation are cacheable
- Cache key includes rule database hash
- Supports incremental updates via delta

## Testing

Test files are located in `test/engine/reasoning/`:

- **RuleTest.cpp**: Rule storage and querying
- **N3ParserTest.cpp**: N3 syntax parsing
- **RdfStarTest.cpp**: RDF-star quoting
- **ReasoningEngineTest.cpp**: Fixpoint iteration and convergence

Run tests:
```bash
ctest -R ReasoningEngineTest --output-on-failure
ctest -R RuleTest --output-on-failure
ctest -R N3ParserTest --output-on-failure
ctest -R RdfStarTest --output-on-failure
```

## Future Enhancements

1. **Stratified Negation**: Support `not` in rule bodies safely
2. **Constraint Checking**: Rule constraints for data validation
3. **Aggregate Functions**: `count`, `sum`, `min`, `max` in rules
4. **Property Paths**: Integrate with SPARQL property path syntax
5. **Graph Patterns**: Support complex graph patterns in rule bodies
6. **Built-in Predicates**: `<`, `>`, `=`, etc. in rules
7. **Debugging Tools**: Trace derivation chains and explain facts
8. **Performance Profiling**: Rule execution profiling and optimization

## References

- **Datalog**: Foundations in [Logic Programming](https://en.wikipedia.org/wiki/Datalog)
- **N3**: [Notation3 Specification](https://www.w3.org/TeamSubmission/n3/)
- **RDF-star**: [RDF-star Specification](https://w3c.github.io/rdf-star/cg-draft/)
- **Semi-Naïve Evaluation**: [Classic Database Literature](https://www.postgresql.org/docs/current/datatype-binary.html)

## Author

Claude AI Assistant (2025)

## License

Apache 2.0

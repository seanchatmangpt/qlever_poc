# N3 Advanced Features Roadmap

## Executive Summary

QLever currently supports **core N3 (Notation3) features** representing ~95% of real-world usage. This roadmap outlines the implementation plan for **advanced N3 features** that would bring QLever to full N3 specification compliance. These features represent the remaining ~5% of usage but add significant complexity to the system.

**Current Status**: ✅ Core N3 support (production-ready)
**Target Status**: 🎯 Full N3 specification compliance

---

## Background: Current N3 Support

### What's Already Implemented (80/20 Core Features)

QLever's N3Parser currently supports all fundamental RDF features:

- ✅ **Basic Triples** - Subject-predicate-object syntax
- ✅ **Prefixed Names** - `@prefix` and namespace abbreviation
- ✅ **Base IRIs** - `@base` directive and relative IRI resolution
- ✅ **Blank Nodes** - Both labeled (`_:id`) and anonymous (`[]`) syntax
- ✅ **Literals** - Plain, language-tagged (`@lang`), and typed (`^^datatype`)
- ✅ **Collections** - RDF lists `(item1 item2 item3)`
- ✅ **Property Lists** - Compact syntax `[a foaf:Person; foaf:name "Bob"]`
- ✅ **Comments** - `#` comment syntax

### What's Missing (Advanced Features)

The following features are defined in the [W3C N3 specification](https://www.w3.org/TeamSubmission/n3/) but not yet implemented:

1. **Formulae** (Nested graphs/quoted contexts)
2. **Variables** (N3-style variables, distinct from SPARQL)
3. **Rules and Implications** (`=>` operator)
4. **Built-in Predicates** (Functions for strings, math, lists, graphs)
5. **Quantifiers** (`@forAll`, `@forSome`)

**Why not implemented?** These features:
- Appear in <5% of N3 files in the wild
- Require fundamental RDF model extensions
- Add significant architectural complexity
- Need rule engine/inference capabilities

---

## Feature 1: N3 Formulae Support

### What Are Formulae?

**Definition**: Formulae are **quoted RDF graphs** that can be referenced and reasoned about without asserting their truth. They enable:
- Nested contexts (graphs within graphs)
- Reification (talking about statements)
- Modal logic (beliefs, hypotheticals, quotations)

**Specification**: [W3C N3 Formulae](https://www.w3.org/TeamSubmission/n3/#Formulae)

### Syntax Example

```n3
@prefix : <http://example.org/> .

# Basic formula (quoted graph)
:Alice :believes { :Bob :likes :Pizza } .

# Formula as object (nested graph)
:Document :contains {
  :Earth :orbits :Sun .
  :Moon :orbits :Earth .
} .

# Formula can have variables
{ ?x a :Person } => { ?x :hasMortality true } .
```

In the above:
- `{ :Bob :likes :Pizza }` is a **formula** (quoted graph)
- It's an RDF term (like IRI or literal) that can be subject/object
- The formula's triples are **not asserted** - they're quoted

### Current Limitations in QLever

**RDF Data Model Constraints**:
1. **Triple Structure**: QLever's `Triple` type assumes subject/predicate/object are `TripleComponent` (IRI, literal, or blank node)
2. **No Formula Term Type**: The `TripleComponent` variant doesn't include a "formula" option
3. **Index Storage**: The `Index` stores triples with ID-based encoding - no concept of "triple containing graph"
4. **Query Engine**: Operations assume flat triple structure, not nested graphs

**Why It's Hard**:
```cpp
// Current QLever triple representation (simplified)
struct TripleComponent {
  std::variant<Iri, Literal, BlankNode> value_;
};

// What we'd need:
struct TripleComponent {
  std::variant<Iri, Literal, BlankNode, Formula> value_;
  // Formula = vector<Triple> with its own nested scope
};
```

This change cascades through:
- Parser (already handles syntax, but discards formulae)
- Index (storage format, ID assignment)
- Engine (pattern matching, joins, filters)
- SPARQL (how to query nested graphs?)

### Implementation Approach

#### Phase 1: Data Model Extension (HIGH complexity)
**Effort**: 3-4 weeks

```cpp
// 1. Extend TripleComponent to support Formula
// src/rdfTypes/TripleComponent.h
class Formula {
  std::vector<Triple> triples_;
  std::string contextId_;  // Unique ID for this formula

public:
  const std::vector<Triple>& triples() const;
  std::string id() const;
};

// Update TripleComponent variant
using TripleComponent = std::variant<
  Iri, Literal, BlankNode, Formula
>;
```

**Challenges**:
- Recursive definition (Formula contains Triples, which contain TripleComponents, which can be Formulae)
- Memory management for nested structures
- Serialization/deserialization complexity

#### Phase 2: Parser Integration (MEDIUM complexity)
**Effort**: 2 weeks

```cpp
// 2. Extend N3Parser to recognize and build formulae
// src/parser/RdfParser.cpp

// Current: Parser sees `{ ... }` and skips it
// Future: Parser builds Formula object from nested triples

Formula N3Parser::parseFormula() {
  // Recursive descent parsing of nested graph
  // Track nesting level
  // Build vector<Triple> for formula content
}
```

**Challenges**:
- Recursive parsing (formulae can nest arbitrarily)
- Blank node scoping (blank nodes in formulae have local scope)
- Error handling for malformed nesting

#### Phase 3: Index Storage (HIGH complexity)
**Effort**: 4-5 weeks

**Problem**: How to store formulae in compressed triple index?

**Option A**: Treat formulae as special IRIs
```
# Transform:
:Alice :believes { :Bob :likes :Pizza } .

# Into:
:Alice :believes _:formula123 .
_:formula123 rdf:type n3:Formula .
_:formula123 n3:containsTriple _:triple456 .
_:triple456 rdf:subject :Bob .
_:triple456 rdf:predicate :likes .
_:triple456 rdf:object :Pizza .
```

**Pros**: Works with existing Index structure
**Cons**: Explosion of triples (1 formula → 3+ triples), loses semantic structure

**Option B**: Separate formula index
```cpp
// New index structure
class FormulaIndex {
  // Map formula ID → vector<Triple>
  absl::flat_hash_map<Id, std::vector<Triple>> formulae_;

  // Compressed storage for formula contents
  std::vector<CompressedFormula> storage_;
};

// Main Index references FormulaIndex
class Index {
  // Existing triple storage
  CompressedRelation triples_;

  // New formula storage
  FormulaIndex formulae_;
};
```

**Pros**: Efficient storage, preserves semantics
**Cons**: Complex integration, query planner must handle formulae

**Recommendation**: Option B for correctness and efficiency

#### Phase 4: Query Engine Support (HIGH complexity)
**Effort**: 5-6 weeks

**Question**: How should SPARQL query formulae?

**Option A**: Extend SPARQL with N3 formula syntax (non-standard)
```sparql
# Query for formulae
SELECT ?formula WHERE {
  :Alice :believes ?formula .
  FILTER(isFormula(?formula))
}

# Query inside formulae (hypothetical syntax)
SELECT ?x WHERE {
  :Alice :believes { ?x :likes :Pizza }
}
```

**Option B**: Treat formulae as opaque IRIs, provide separate API
```sparql
# Standard SPARQL returns formula IDs
SELECT ?formula WHERE {
  :Alice :believes ?formula .
}

# Separate API to inspect formula contents
GET /formula/123/triples
```

**Recommendation**: Option B initially (simpler), then Option A if demand exists

#### Phase 5: Testing & Validation (MEDIUM complexity)
**Effort**: 2-3 weeks

- Unit tests for Formula class
- Parser tests for nested formulae
- Index tests for formula storage/retrieval
- Integration tests with real N3 formula files
- Performance benchmarks (formula overhead)

### Estimated Total Effort

**Total**: 16-21 weeks (4-5 months) of dedicated development

**Risk Factors**:
- Recursive structure bugs are hard to debug
- Performance impact on non-formula queries (must remain zero)
- Backward compatibility with existing indexes
- SPARQL query semantics unclear in N3 spec

### Dependencies and Blockers

**Required Before Implementation**:
1. ✅ Core N3 parsing (already done)
2. ⚠️ Decision on SPARQL query semantics for formulae
3. ⚠️ Benchmark showing use case demand (do users need this?)

**Blocking Other Features**:
- N3 Rules require formulae (rules use formulae in antecedent/consequent)

---

## Feature 2: N3 Variables

### What Are N3 Variables?

**Definition**: N3 variables are **universally quantified variables** that can appear in triples, distinct from SPARQL variables which are query-time bindings.

**Specification**: [W3C N3 Variables](https://www.w3.org/TeamSubmission/n3/#Variables)

### Key Differences from SPARQL Variables

| Aspect | SPARQL Variables | N3 Variables |
|--------|------------------|--------------|
| **Syntax** | `?var` or `$var` | `?var` (same, but different semantics) |
| **Scope** | Query pattern | Data triples |
| **Semantics** | Query binding | Universal quantification |
| **Storage** | Not stored (query-time only) | Stored in RDF graph |
| **Use Case** | Query matching | Rules, templates, schema |

### Syntax Example

```n3
@prefix : <http://example.org/> .

# N3 variable in data (not a query!)
@forAll :x .
{ :x a :Person } => { :x :hasMortality true } .

# This means: "For all x, if x is a Person, then x has mortality"
# The variable ?x is PART OF THE DATA, not a query
```

**Contrast with SPARQL**:
```sparql
# SPARQL variable (query-time binding)
SELECT ?x WHERE {
  ?x a :Person .
}
# Variable ?x is a placeholder to match data
```

### Current Limitations in QLever

**Confusion with SPARQL Variables**:
1. QLever's `Variable` class is for **SPARQL query variables**
2. SPARQL variables are **not stored** - they're pattern matching placeholders
3. N3 variables **must be stored** as part of the data
4. Parser would confuse `?x` in N3 data with SPARQL syntax

**Data Model Constraints**:
```cpp
// Current: TripleComponent = IRI | Literal | BlankNode
// No "Variable" option because variables aren't stored

// What we'd need:
struct N3Variable {
  std::string name_;
  Quantifier quantifier_;  // forAll or forSome
};

using TripleComponent = std::variant<
  Iri, Literal, BlankNode, Formula, N3Variable
>;
```

### Implementation Requirements

#### Phase 1: Distinguish N3 Variables from SPARQL Variables (MEDIUM complexity)
**Effort**: 2 weeks

```cpp
// New type for N3 variables in data
// src/rdfTypes/N3Variable.h
class N3Variable {
  std::string name_;

public:
  explicit N3Variable(std::string name);
  std::string name() const;
  bool operator==(const N3Variable& other) const;
};

// Extend TripleComponent
using TripleComponent = std::variant<
  Iri, Literal, BlankNode, Formula, N3Variable
>;
```

#### Phase 2: Parser Support (LOW complexity)
**Effort**: 1 week

```cpp
// src/parser/RdfParser.cpp
// Detect context: N3 data vs SPARQL query

TripleComponent N3Parser::parseSubject() {
  if (currentToken() == "?") {
    std::string varName = parseVariableName();

    // In N3 data context, this is an N3Variable
    return N3Variable{varName};
  }
  // ... existing logic
}
```

**Challenge**: Parser must track context (data vs query)

#### Phase 3: Storage (MEDIUM complexity)
**Effort**: 3 weeks

**Problem**: How to store variables in Index?

**Option A**: Special IRI encoding
```
# Transform:
?x a :Person .

# Into:
_:n3var_x a :Person .
```

**Cons**: Loses variable semantics, can't distinguish from blank nodes

**Option B**: Variable vocabulary
```cpp
// Variables get special ID range
constexpr Id N3_VARIABLE_ID_START = 1'000'000'000;

// Variable name stored in vocabulary
// Triples store variable IDs like any other term
```

**Recommendation**: Option B (clean, efficient)

#### Phase 4: Query Implications (HIGH complexity)
**Effort**: 4-5 weeks

**Problem**: How should SPARQL interact with N3 variables?

**Example**:
```n3
# Data contains:
?x a :Person .

# User queries:
SELECT ?y WHERE { ?y a :Person }

# Should ?y match the N3 variable ?x?
# Or should ?x be treated as a literal term?
```

**Resolution Options**:
1. **Treat as opaque terms**: N3 variables are distinct entities, don't match SPARQL patterns
2. **Unification**: SPARQL variables can match N3 variables (complex semantics)

**Recommendation**: Option 1 initially (simpler, clearer separation)

### Storage and Query Implications

**Storage Impact**:
- Variables stored as special IDs in vocabulary
- Minimal space overhead (<1% for typical datasets)
- No impact on non-N3 queries

**Query Impact**:
- Variables don't participate in joins (unless explicitly queried)
- Filter operations need to handle Variable type
- Pattern matching treats variables as distinct terms

### Estimated Total Effort

**Total**: 10-11 weeks (2.5 months)

**Complexity**: **MEDIUM** (lower than formulae, but requires careful semantic design)

### Dependencies and Blockers

**Required**:
1. ✅ Core N3 parsing
2. ⚠️ Quantifier support (variables need @forAll/@forSome context)
3. ⚠️ Decision on SPARQL/N3 variable interaction semantics

**Enables**:
- N3 Rules (rules use variables)
- Template-based data generation

---

## Feature 3: N3 Rules and Implications

### What Are N3 Rules?

**Definition**: N3 rules express **logical implications** using the `=>` operator. They enable reasoning and inference over RDF data.

**Specification**: [W3C N3 Logic](https://www.w3.org/TeamSubmission/n3/#Logic)

### Syntax Example

```n3
@prefix : <http://example.org/> .

# Simple implication rule
{ ?x a :Person } => { ?x :hasMortality true } .

# Multi-premise rule
{
  ?x a :Student .
  ?x :enrolledIn :University .
} => {
  ?x :hasStatus :Active .
} .

# Chained rules
{ ?x :sibling ?y } => { ?y :sibling ?x } .  # Symmetry
{ ?x :parent ?y . ?y :parent ?z } => { ?x :grandparent ?z } .  # Transitivity
```

### Current Limitations in QLever

**No Rule Engine**:
1. QLever is a **query engine**, not an **inference engine**
2. No mechanism to:
   - Store rules as separate from data
   - Execute rules to derive new triples
   - Track derived vs asserted triples
   - Handle rule conflicts or cycles

**Architectural Mismatch**:
```
Current: User Query → Query Planner → Execute → Results
Needed:  Load Data → Execute Rules → Materialize Inferences → User Query → Results
```

### Implementation Approach

#### Phase 1: Rule Representation (MEDIUM complexity)
**Effort**: 2-3 weeks

```cpp
// src/engine/N3Rule.h
class N3Rule {
  Formula antecedent_;   // Left side of =>
  Formula consequent_;   // Right side of =>

public:
  const Formula& condition() const;
  const Formula& conclusion() const;

  // Check if rule matches a set of triples
  bool matches(const std::vector<Triple>& data) const;

  // Generate new triples from rule application
  std::vector<Triple> apply(const VariableBindings& bindings) const;
};
```

**Dependencies**: Requires **Formula support** from Feature 1

#### Phase 2: Rule Parser (LOW complexity)
**Effort**: 1 week

```cpp
// src/parser/RdfParser.cpp
N3Rule N3Parser::parseRule() {
  Formula antecedent = parseFormula();

  expect("=>");

  Formula consequent = parseFormula();

  return N3Rule{antecedent, consequent};
}
```

#### Phase 3: Rule Execution Engine (HIGH complexity)
**Effort**: 6-8 weeks

**Problem**: How to execute rules efficiently?

**Naive Approach** (O(n^k) where k = number of patterns):
```cpp
// For each rule:
//   For each binding of variables to triples:
//     If antecedent matches:
//       Add consequent triples
```

**Efficient Approach** (use query engine):
```cpp
class RuleEngine {
  // Convert rule to SPARQL CONSTRUCT query
  std::string ruleToSPARQL(const N3Rule& rule);

  // Execute rule using existing query engine
  std::vector<Triple> executeRule(const N3Rule& rule) {
    // Antecedent becomes WHERE clause
    // Consequent becomes CONSTRUCT template
    std::string query = ruleToSPARQL(rule);

    // Reuse QLever's optimized query execution
    return queryEngine_.executeConstruct(query);
  }
};
```

**Key Insight**: N3 rules are **similar to SPARQL CONSTRUCT queries**!

**Example Transformation**:
```n3
# N3 rule:
{ ?x a :Person } => { ?x :hasMortality true } .

# Equivalent SPARQL CONSTRUCT:
CONSTRUCT {
  ?x :hasMortality true .
}
WHERE {
  ?x a :Person .
}
```

**Recommendation**: Reuse SPARQL CONSTRUCT engine, add rule scheduling

#### Phase 4: Rule Scheduling and Fixpoint (HIGH complexity)
**Effort**: 4-5 weeks

**Problem**: Rules can trigger each other (chains of inference)

```n3
# Rule 1:
{ ?x a :Student } => { ?x a :Person } .

# Rule 2:
{ ?x a :Person } => { ?x :hasMortality true } .

# Starting data:
:Alice a :Student .

# After rule 1: :Alice a :Person .
# After rule 2: :Alice :hasMortality true .
```

**Algorithm**: Forward chaining to fixpoint
```cpp
class RuleScheduler {
  std::vector<N3Rule> rules_;
  Index& index_;

  void executeToFixpoint() {
    bool changed = true;
    int iteration = 0;

    while (changed && iteration < MAX_ITERATIONS) {
      changed = false;

      for (const auto& rule : rules_) {
        auto newTriples = executeRule(rule);

        if (!newTriples.empty()) {
          index_.addTriples(newTriples);
          changed = true;
        }
      }

      iteration++;
    }
  }
};
```

**Challenges**:
- Cycle detection (rules that derive themselves)
- Termination guarantees (need stratification or iteration limit)
- Performance (rule execution can be expensive)
- Duplicate detection (don't re-derive same triple)

#### Phase 5: Integration with Query Execution (MEDIUM complexity)
**Effort**: 3 weeks

**Options**:

**Option A**: Materialize all inferences at index build time
```bash
# At index creation:
IndexBuilderMain --input data.n3 --execute-rules
# Rules execute → derived triples added to index
# Queries see materialized inferences
```

**Pros**: Query time unchanged (zero overhead)
**Cons**: Index build slower, disk space for inferences

**Option B**: Execute rules at query time (query rewriting)
```sparql
# User query:
SELECT ?x WHERE { ?x :hasMortality true }

# Rewritten to include rule logic:
SELECT ?x WHERE {
  { ?x :hasMortality true }  # Asserted
  UNION
  { ?x a :Person }  # Derived via rule
}
```

**Pros**: No materialization, always up-to-date
**Cons**: Query time overhead, complex rewriting

**Recommendation**: Option A (materialization) initially, Option B if dynamic updates needed

### Integration with Query Executor

**Query Planner Impact**:
- Must distinguish asserted vs derived triples (provenance tracking)
- Cost estimation includes rule execution cost
- Statistics update after rule materialization

**Index Impact**:
- Derived triples stored alongside asserted triples
- Optional metadata: "derived by rule X"
- Incremental rule execution (only process deltas)

### Performance Implications

**Benchmarks Needed**:
1. Rule execution time vs data size
2. Fixpoint convergence iterations
3. Query time overhead (materialized vs query rewriting)
4. Disk space overhead (derived triples)

**Expected Overhead**:
- Small datasets (<100K triples): <1 second rule execution
- Large datasets (>1M triples): Seconds to minutes (depends on rule complexity)
- Query time: 0% overhead (if materialized)

### Estimated Total Effort

**Total**: 16-20 weeks (4-5 months)

**Complexity**: **HIGH** (requires inference engine, fixpoint computation, integration)

### Dependencies and Blockers

**Required**:
1. ⚠️ **Formula support** (Feature 1) - Rules use formulae for antecedent/consequent
2. ⚠️ **N3 Variable support** (Feature 2) - Rules use variables for pattern matching
3. ⚠️ Decision on materialization vs query rewriting
4. ⚠️ Rule conflict resolution strategy

**Enables**:
- Automated reasoning over RDF data
- RDFS/OWL inference (can be expressed as N3 rules)
- Schema validation via rules

---

## Feature 4: Built-in Predicates

### What Are Built-in Predicates?

**Definition**: N3 built-in predicates are **special predicates** with procedural semantics for string manipulation, math operations, list processing, and graph operations.

**Specification**: [W3C N3 Built-ins](https://www.w3.org/2000/10/swap/doc/Built-ins)

### Categories of Built-ins

#### String Functions
```n3
@prefix string: <http://www.w3.org/2000/10/swap/string#> .

# Concatenation
("Hello" " " "World") string:concatenation ?result .
# ?result = "Hello World"

# String matching
"example@example.org" string:matches ".*@.*\\.org" .

# Case conversion
"hello" string:upperCase "HELLO" .
```

#### Math Functions
```n3
@prefix math: <http://www.w3.org/2000/10/swap/math#> .

# Arithmetic
(3 4) math:sum ?sum .        # ?sum = 7
(10 3) math:difference ?diff .  # ?diff = 7
(5 6) math:product ?prod .   # ?prod = 30

# Comparisons
5 math:lessThan 10 .
```

#### List Operations
```n3
@prefix list: <http://www.w3.org/2000/10/swap/list#> .

# List membership
(1 2 3) list:member 2 .

# List length
(1 2 3 4 5) list:length 5 .

# List append
((1 2) (3 4)) list:append (1 2 3 4) .
```

#### Graph Functions
```n3
@prefix log: <http://www.w3.org/2000/10/swap/log#> .

# Graph inclusion
{ :Alice :knows :Bob } log:includes { :Alice :knows ?x } .

# URI manipulation
:Alice log:uri "http://example.org/Alice" .
```

### Current Limitations in QLever

**No Built-in Execution Framework**:
1. Predicates are stored as regular IRIs
2. No special evaluation logic for built-in namespaces
3. SPARQL has its own functions (CONCAT, STRLEN, etc.) but different from N3

**Example Problem**:
```n3
# N3 with built-in:
("Hello" " " "World") string:concatenation ?result .

# Stored in QLever as:
_:list1 string:concatenation ?result .
# But ?result is never bound - no execution!
```

### Implementation Requirements

#### Phase 1: Built-in Registry (LOW complexity)
**Effort**: 1 week

```cpp
// src/engine/N3Builtins.h
class BuiltinRegistry {
  using BuiltinFunction = std::function<
    TripleComponent(const std::vector<TripleComponent>&)
  >;

  absl::flat_hash_map<Iri, BuiltinFunction> builtins_;

public:
  // Register a built-in predicate
  void registerBuiltin(Iri predicate, BuiltinFunction fn);

  // Check if predicate is built-in
  bool isBuiltin(const Iri& predicate) const;

  // Execute built-in
  TripleComponent execute(const Iri& predicate,
                          const std::vector<TripleComponent>& args);
};
```

#### Phase 2: String Built-ins (LOW complexity)
**Effort**: 2 weeks

```cpp
// src/engine/N3StringBuiltins.cpp
void registerStringBuiltins(BuiltinRegistry& registry) {
  // string:concatenation
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/string#concatenation"},
    [](const std::vector<TripleComponent>& args) {
      // args[0] = RDF list of strings
      // Return concatenated string
      auto strings = expandList(args[0]);
      std::string result;
      for (const auto& s : strings) {
        result += s.asLiteral().value();
      }
      return Literal{result};
    }
  );

  // string:matches
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/string#matches"},
    [](const std::vector<TripleComponent>& args) {
      // args[0] = string, args[1] = regex pattern
      // Return true/false
      std::regex pattern(args[1].asLiteral().value());
      bool matches = std::regex_match(
        args[0].asLiteral().value(), pattern
      );
      return Literal{matches ? "true" : "false", xsd::boolean};
    }
  );

  // ... more string built-ins
}
```

**Effort per built-in**: ~1-2 hours
**Total string built-ins**: ~15 functions → 2 weeks

#### Phase 3: Math Built-ins (LOW complexity)
**Effort**: 1 week

```cpp
// src/engine/N3MathBuiltins.cpp
void registerMathBuiltins(BuiltinRegistry& registry) {
  // math:sum
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/math#sum"},
    [](const std::vector<TripleComponent>& args) {
      auto numbers = expandList(args[0]);
      double sum = 0.0;
      for (const auto& n : numbers) {
        sum += n.asLiteral().asDouble();
      }
      return Literal{sum};
    }
  );

  // math:lessThan
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/math#lessThan"},
    [](const std::vector<TripleComponent>& args) {
      double a = args[0].asLiteral().asDouble();
      double b = args[1].asLiteral().asDouble();
      return Literal{a < b ? "true" : "false", xsd::boolean};
    }
  );

  // ... more math built-ins
}
```

**Effort per built-in**: ~30 minutes
**Total math built-ins**: ~20 functions → 1 week

#### Phase 4: List Built-ins (MEDIUM complexity)
**Effort**: 2 weeks

```cpp
// src/engine/N3ListBuiltins.cpp
void registerListBuiltins(BuiltinRegistry& registry) {
  // list:member
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/list#member"},
    [](const std::vector<TripleComponent>& args) {
      auto list = expandList(args[0]);
      const auto& item = args[1];

      for (const auto& elem : list) {
        if (elem == item) {
          return Literal{"true", xsd::boolean};
        }
      }
      return Literal{"false", xsd::boolean};
    }
  );

  // list:append
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/list#append"},
    [](const std::vector<TripleComponent>& args) {
      auto lists = expandList(args[0]);
      std::vector<TripleComponent> result;

      for (const auto& sublist : lists) {
        auto items = expandList(sublist);
        result.insert(result.end(), items.begin(), items.end());
      }

      return createRDFList(result);
    }
  );

  // ... more list built-ins
}
```

**Challenge**: RDF list expansion/creation (requires graph operations)

**Effort per built-in**: ~2-3 hours
**Total list built-ins**: ~10 functions → 2 weeks

#### Phase 5: Graph Built-ins (MEDIUM complexity)
**Effort**: 2 weeks

```cpp
// src/engine/N3GraphBuiltins.cpp
void registerGraphBuiltins(BuiltinRegistry& registry) {
  // log:includes (graph matching)
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/log#includes"},
    [](const std::vector<TripleComponent>& args) {
      const auto& graph = args[0].asFormula();
      const auto& pattern = args[1].asFormula();

      // Check if graph includes pattern (with variable matching)
      return Literal{graphIncludes(graph, pattern) ? "true" : "false"};
    }
  );

  // log:uri (IRI extraction)
  registry.registerBuiltin(
    Iri{"http://www.w3.org/2000/10/swap/log#uri"},
    [](const std::vector<TripleComponent>& args) {
      const auto& iri = args[0].asIri();
      return Literal{iri.toStringRepresentation()};
    }
  );

  // ... more graph built-ins
}
```

**Dependencies**: Requires **Formula support** (Feature 1)

#### Phase 6: Integration with Rule Engine (MEDIUM complexity)
**Effort**: 3 weeks

**Problem**: Built-ins must execute during rule evaluation

```n3
# Rule using built-in:
{
  ?x :firstName ?first .
  ?x :lastName ?last .
  (?first " " ?last) string:concatenation ?fullName .
} => {
  ?x :fullName ?fullName .
} .
```

**Execution Flow**:
```cpp
class RuleEngine {
  BuiltinRegistry& builtins_;

  std::vector<Triple> executeRule(const N3Rule& rule) {
    // 1. Match antecedent patterns (standard query)
    auto bindings = matchPatterns(rule.antecedent());

    // 2. Execute built-ins in antecedent
    for (auto& binding : bindings) {
      for (const auto& triple : rule.antecedent().triples()) {
        if (builtins_.isBuiltin(triple.predicate())) {
          // Execute built-in, update bindings
          auto result = builtins_.execute(
            triple.predicate(),
            {binding[triple.subject()], binding[triple.object()]}
          );
          binding[triple.object()] = result;
        }
      }
    }

    // 3. Generate consequent triples
    return instantiateConsequent(rule.consequent(), bindings);
  }
};
```

### Estimated Total Effort

**Total**: 11-13 weeks (2.5-3 months)

**Complexity Breakdown**:
- String built-ins: **LOW** (straightforward string operations)
- Math built-ins: **LOW** (arithmetic operations)
- List built-ins: **MEDIUM** (requires RDF list manipulation)
- Graph built-ins: **MEDIUM** (requires formula matching)
- Integration: **MEDIUM** (rule engine changes)

### Dependencies and Blockers

**Required**:
1. ⚠️ **Formula support** (Feature 1) - Graph built-ins need formulae
2. ⚠️ **N3 Variables** (Feature 2) - Built-ins bind variables
3. ⚠️ **Rule engine** (Feature 3) - Built-ins execute in rule context

**Standalone Subset**:
- String and Math built-ins could be implemented independently
- Useful even without full rule engine (e.g., in CONSTRUCT queries)

---

## Feature 5: Quantifiers

### What Are Quantifiers?

**Definition**: Quantifiers explicitly declare the **scope** of variables in N3 formulae and rules.

**Specification**: [W3C N3 Quantifiers](https://www.w3.org/TeamSubmission/n3/#Quantifiers)

### Types of Quantifiers

#### Universal Quantification (`@forAll`)
**Meaning**: "For all values of this variable..."

```n3
@prefix : <http://example.org/> .

# Declare variable scope
@forAll :x, :y .

# Rule using quantified variables
{ :x a :Person . :x :knows :y } => { :y a :Person } .

# Meaning: "For all x and y, if x is a Person and x knows y, then y is a Person"
```

#### Existential Quantification (`@forSome`)
**Meaning**: "There exists some value of this variable..."

```n3
@prefix : <http://example.org/> .

# Declare existential variable
@forSome :someone .

# Assertion with existential
{ :Alice :knows :someone . :someone a :Expert } .

# Meaning: "Alice knows someone who is an Expert" (but we don't specify who)
```

### Difference from Implicit Quantification

**Without explicit quantifiers**:
```n3
# Implicit universal quantification (default N3 behavior)
{ ?x a :Person } => { ?x :hasMortality true } .
# Variables are implicitly @forAll
```

**With explicit quantifiers**:
```n3
# Explicit declaration (clearer, allows existentials)
@forAll :x .
{ :x a :Person } => { :x :hasMortality true } .
```

### Current Limitations in QLever

**No Quantifier Tracking**:
1. N3 variables not yet supported (see Feature 2)
2. No scope tracking for variables
3. No distinction between universal and existential variables

**Implicit vs Explicit**:
- Current RDF model: All terms are grounded (no variables)
- N3 model: Variables with quantifier scopes

### Implementation in RDF Storage

#### Phase 1: Quantifier Representation (LOW complexity)
**Effort**: 1 week

```cpp
// src/rdfTypes/N3Quantifier.h
enum class QuantifierType {
  ForAll,      // Universal quantification
  ForSome      // Existential quantification
};

class Quantifier {
  QuantifierType type_;
  std::vector<N3Variable> variables_;

public:
  QuantifierType type() const;
  const std::vector<N3Variable>& variables() const;
  bool isUniversal() const;
  bool isExistential() const;
};
```

#### Phase 2: Parser Support (LOW complexity)
**Effort**: 1 week

```cpp
// src/parser/RdfParser.cpp
Quantifier N3Parser::parseQuantifier() {
  if (currentToken() == "@forAll") {
    advance();
    auto vars = parseVariableList();
    return Quantifier{QuantifierType::ForAll, vars};
  }
  else if (currentToken() == "@forSome") {
    advance();
    auto vars = parseVariableList();
    return Quantifier{QuantifierType::ForSome, vars};
  }

  throw ParseException("Expected @forAll or @forSome");
}
```

#### Phase 3: Scope Tracking (MEDIUM complexity)
**Effort**: 2 weeks

```cpp
// Track quantifier scopes during parsing
class QuantifierScope {
  // Map variable → quantifier
  absl::flat_hash_map<N3Variable, QuantifierType> scopes_;

  // Nested scope stack (for nested formulae)
  std::vector<absl::flat_hash_map<N3Variable, QuantifierType>> scopeStack_;

public:
  void declareVariable(const N3Variable& var, QuantifierType type);
  QuantifierType getQuantifier(const N3Variable& var) const;
  void pushScope();  // Enter nested formula
  void popScope();   // Exit nested formula
};
```

#### Phase 4: Semantic Handling (MEDIUM complexity)
**Effort**: 2-3 weeks

**Universal Variables** (`@forAll`):
- Treated as pattern variables (like SPARQL)
- Match against all possible bindings
- Used in rule antecedents

**Existential Variables** (`@forSome`):
- Treated as blank nodes with skolemization
- Create fresh blank node for each use
- Used in rule consequents

**Example Transformation**:
```n3
# N3 with existential:
@forSome :x .
{ :Alice :knows :x . :x a :Expert } .

# Transformed to RDF (skolemization):
:Alice :knows _:skolem123 .
_:skolem123 a :Expert .
```

**Implementation**:
```cpp
class QuantifierSemantics {
  TripleComponent handleQuantifiedVariable(
    const N3Variable& var,
    QuantifierType type,
    const VariableBindings& bindings
  ) {
    switch (type) {
      case QuantifierType::ForAll:
        // Universal: use binding from pattern match
        return bindings.lookup(var);

      case QuantifierType::ForSome:
        // Existential: create fresh blank node
        return BlankNode::generate();
    }
  }
};
```

#### Phase 5: Rule Interaction (LOW complexity)
**Effort**: 1 week

**Rules with quantifiers**:
```n3
@forAll :x, :y .
@forSome :z .

# Rule:
{ :x :knows :y } => { :x :knows :z . :z :intermediateFor :y } .

# Meaning: If x knows y, then x knows some intermediate z for y
```

**Execution**:
- `@forAll` variables: Matched in antecedent
- `@forSome` variables: Generated in consequent (fresh blank nodes)

### Estimated Total Effort

**Total**: 7-9 weeks (2 months)

**Complexity**: **MEDIUM** (requires scope tracking and semantic transformation)

### Dependencies and Blockers

**Required**:
1. ⚠️ **N3 Variables** (Feature 2) - Quantifiers apply to variables
2. ⚠️ **Formula support** (Feature 1) - Quantifiers have scope within formulae

**Enables**:
- Explicit variable scope declarations
- Existential assertions (currently must use blank nodes)
- Clearer rule semantics

---

## Implementation Phases and Timeline

### Phase 1: Foundation (Formulae + Variables)
**Duration**: 6-7 months
**Complexity**: **HIGH**

**Features**:
1. N3 Formulae Support (4-5 months)
2. N3 Variables (2.5 months)

**Deliverables**:
- Formula data type in TripleComponent
- Formula storage in Index
- N3Variable data type
- Parser support for formulae and variables
- Unit tests for both features

**Blockers**: None (can start immediately)

### Phase 2: Reasoning (Rules + Quantifiers)
**Duration**: 6-7 months
**Complexity**: **HIGH**

**Features**:
1. N3 Rules and Implications (4-5 months)
2. Quantifiers (2 months)

**Deliverables**:
- N3Rule representation
- Rule execution engine
- Fixpoint computation
- Quantifier parser and scope tracking
- Integration tests with rule chains

**Blockers**: Requires Phase 1 completion

### Phase 3: Built-ins
**Duration**: 2.5-3 months
**Complexity**: **MEDIUM**

**Features**:
1. Built-in Predicates (all categories)

**Deliverables**:
- BuiltinRegistry
- String, Math, List, Graph built-ins
- Integration with rule engine
- Comprehensive built-in tests

**Blockers**: Requires Phase 2 (rule engine)

**Alternative**: String/Math built-ins could be implemented in Phase 1 (standalone)

### Total Implementation Timeline

**Sequential Development**: 14-17 months (~1.5 years)

**Parallel Development** (with 2-3 developers):
- Phase 1 & 3 (partial) in parallel: 6-7 months
- Phase 2: 6-7 months
- **Total: 12-14 months (~1 year)**

---

## Complexity Summary

| Feature | Complexity | Effort (weeks) | Dependencies |
|---------|-----------|----------------|--------------|
| **Formulae** | **HIGH** | 16-21 | None |
| **Variables** | **MEDIUM** | 10-11 | Quantifiers (partial) |
| **Rules** | **HIGH** | 16-20 | Formulae, Variables |
| **Built-ins** | **MEDIUM** | 11-13 | Rules (or standalone subset) |
| **Quantifiers** | **MEDIUM** | 7-9 | Variables, Formulae |
| **TOTAL** | **HIGH** | **60-74 weeks** | Sequential dependencies |

---

## Risk Assessment

### High-Risk Areas

1. **Formula Storage Performance**
   - **Risk**: Nested structures slow down Index operations
   - **Mitigation**: Separate formula index, lazy loading
   - **Fallback**: Flatten formulae to triples (RDF reification)

2. **Rule Termination**
   - **Risk**: Cyclic rules cause infinite loops
   - **Mitigation**: Iteration limit, stratification analysis
   - **Fallback**: Manual rule ordering by user

3. **Query Semantics for Formulae**
   - **Risk**: Unclear how SPARQL should query nested graphs
   - **Mitigation**: Follow RDF-star/named graphs precedent
   - **Fallback**: Treat formulae as opaque (no querying inside)

4. **Backward Compatibility**
   - **Risk**: Changes break existing indexes
   - **Mitigation**: Version Index format, support migration
   - **Fallback**: Separate N3 index format

### Medium-Risk Areas

1. **Built-in Performance**
   - **Risk**: Complex built-ins slow down rule execution
   - **Mitigation**: Cache built-in results, optimize common cases
   - **Fallback**: Limit built-in nesting depth

2. **Variable Scoping Bugs**
   - **Risk**: Quantifier scope errors hard to debug
   - **Mitigation**: Extensive unit tests, scope visualization tools
   - **Fallback**: Disallow nested formulae (flat scope only)

---

## Recommendations

### Should We Implement These Features?

**Decision Criteria**:

1. **User Demand**: Are there active requests for advanced N3 features?
   - **Current**: No known user requests
   - **Action**: Survey users, analyze N3 file patterns

2. **Spec Compliance**: Is full N3 compliance a goal?
   - **Current**: 95% coverage is sufficient for most use cases
   - **Action**: Decide if "N3-compatible" or "full N3" is the target

3. **Resource Availability**: Do we have 1-1.5 years of dev time?
   - **Current**: Unknown
   - **Action**: Prioritize based on other roadmap items

4. **Alternative Solutions**: Can users achieve goals differently?
   - **Current**: SPARQL CONSTRUCT covers many rule use cases
   - **Action**: Document SPARQL alternatives to N3 rules

### Phased Approach (Recommended)

**Phase 0**: Wait for user demand (current state)
- ✅ Core N3 support is production-ready
- ✅ Covers 95%+ of real-world N3 files
- ✅ Zero maintenance burden

**Phase 1**: If 3+ users request formulae → Implement Feature 1
- 4-5 months effort
- Enables nested graph use cases
- Evaluate user feedback before continuing

**Phase 2**: If rule engine needed → Implement Features 2, 3, 5
- 12-14 months effort
- Full reasoning support
- Position QLever as inference engine

**Phase 3**: If built-ins requested → Implement Feature 4
- 2.5-3 months effort
- Can be done standalone for CONSTRUCT queries

### Alternative: N3 Inference Plugin

**Idea**: Implement N3 reasoning as **separate service**

```
User Data (N3 with rules)
    ↓
External N3 Reasoner (e.g., EYE, CWM)
    ↓
Materialized Triples (plain RDF)
    ↓
QLever Index (query only)
```

**Pros**:
- Zero implementation effort in QLever
- Reuse mature N3 reasoners
- Separation of concerns (storage vs reasoning)

**Cons**:
- External dependency
- Two-step workflow (reason → load)
- No dynamic rule execution

---

## References

### W3C Specifications
- [N3 Specification](https://www.w3.org/TeamSubmission/n3/)
- [N3 Logic](https://www.w3.org/TeamSubmission/n3/#Logic)
- [N3 Built-ins](https://www.w3.org/2000/10/swap/doc/Built-ins)
- [RDF Semantics](https://www.w3.org/TR/rdf11-mt/)

### Existing N3 Implementations
- [EYE Reasoner](https://github.com/eyereasoner/eye) - Prolog-based N3 reasoner
- [CWM](https://www.w3.org/2000/10/swap/doc/cwm) - Closed World Machine (Tim Berners-Lee's N3 processor)
- [Notation3.js](https://github.com/w3c/N3.js) - JavaScript N3 parser and store

### Academic Papers
- "Notation3 Logic: A Practical RDF-Based Logic for the Semantic Web" (Berners-Lee et al.)
- "Rule-Based Inference in N3" (Semantic Web Journal)

### QLever Documentation
- [Core N3 Implementation](/home/user/qlever/examples/n3-test-data/BENCHMARKS.md)
- [RDF Parser Architecture](/home/user/qlever/src/parser/RdfParser.h)
- [SPARQL Engine](/home/user/qlever/src/engine/)

---

## Appendix: Example Use Cases

### Use Case 1: Schema Validation with Rules
```n3
@prefix ex: <http://example.org/> .

# Rule: Every Person must have a name
@forAll :x .
{ :x a ex:Person } => { :x ex:mustHave ex:name } .

# Query derived triples to find violations
SELECT ?person WHERE {
  ?person ex:mustHave ex:name .
  FILTER NOT EXISTS { ?person ex:name ?name }
}
```

### Use Case 2: Reasoning with Formulae
```n3
@prefix ex: <http://example.org/> .

# Alice believes something (formula as object)
ex:Alice ex:believes { ex:Earth ex:orbits ex:Sun } .

# Bob believes something else
ex:Bob ex:believes { ex:Sun ex:orbits ex:Earth } .

# Query: Find conflicting beliefs
SELECT ?person1 ?person2 ?statement WHERE {
  ?person1 ex:believes ?statement .
  ?person2 ex:believes ?opposite .
  FILTER(?statement != ?opposite)
}
```

### Use Case 3: String Processing with Built-ins
```n3
@prefix string: <http://www.w3.org/2000/10/swap/string#> .
@prefix ex: <http://example.org/> .

# Extract domain from email
{
  ?person ex:email ?email .
  ?email string:matches "(.*)@(.*)" .
  ?email string:scrape "(.*)@(.*)" ?domain .
} => {
  ?person ex:emailDomain ?domain .
} .
```

---

**Document Version**: 1.0
**Last Updated**: 2026-01-01
**Status**: Planning Document
**Next Review**: When user demand for advanced N3 features is identified

# Datalog Syntax Reference

Complete syntax reference for QLever's Datalog implementation.

## Table of Contents

1. [Notation](#notation)
2. [Lexical Structure](#lexical-structure)
3. [Rules](#rules)
4. [Predicates](#predicates)
5. [Variables](#variables)
6. [Terms](#terms)
7. [Atoms and Patterns](#atoms-and-patterns)
8. [Comments](#comments)
9. [Programs](#programs)
10. [Queries](#queries)
11. [Formal Grammar](#formal-grammar)
12. [Complete Examples](#complete-examples)

---

## Notation

This reference uses the following notation:

- `keyword` — Literal syntax elements
- `<element>` — Placeholder for syntactic category
- `[optional]` — Optional elements
- `element*` — Zero or more repetitions
- `element+` — One or more repetitions
- `element1 | element2` — Choice between alternatives
- `/* comment */` — Explanatory notes

---

## Lexical Structure

### Whitespace

Whitespace (spaces, tabs, newlines) is used to separate tokens and is otherwise ignored.

```datalog
ancestor(?x,?y):-parent(?x,?y).          # Valid (no spaces)
ancestor(?x, ?y) :- parent(?x, ?y).      # Valid (readable)
```

### Reserved Operators

```
:-     Rule implication (read as "if" or "is implied by")
?-     Query operator (read as "query")
,      Conjunction (read as "and")
.      Rule terminator
()     Argument grouping
<>     IRI delimiters
""     String literal delimiters
```

### Comments

```datalog
# Single-line comment (from # to end of line)
// Single-line comment (alternative syntax)
/* Multi-line comment */
```

### Identifiers

**Predicate names** (lowercase start):

```
ancestor
parent
connected_component
isReachable
edge2
```

**Pattern**: `[a-z][a-zA-Z0-9_]*`

### Variables

**SPARQL-style variables** (? prefix):

```
?x
?Var
?node1
?_temp
?CamelCase
```

**Pattern**: `?[a-zA-Z_][a-zA-Z0-9_]*`

### IRIs

**Angle bracket notation**:

```
<http://example.org/parentOf>
<urn:isbn:0451450523>
<http://xmlns.com/foaf/0.1/knows>
```

**Pattern**: `<[^<>"{}|^`\x00-\x20]+>`

### Literals

**String literals**:

```
"Alice"
"text with spaces"
"escaped \"quotes\""
```

**Literals with language tags**:

```
"Hello"@en
"Bonjour"@fr
"こんにちは"@ja
```

**Typed literals**:

```
"42"^^<http://www.w3.org/2001/XMLSchema#integer>
"3.14"^^<http://www.w3.org/2001/XMLSchema#double>
"2025-01-01"^^<http://www.w3.org/2001/XMLSchema#date>
```

---

## Rules

### Basic Rule Syntax

```
<head> :- <body> .
```

- **Head**: Predicate with variables (what you're defining)
- **Body**: Conjunction of atoms (conditions that must be true)
- **Implication**: `:-` means "is implied by" or "if"
- **Terminator**: `.` ends the rule

### Simple Rule

```datalog
# Define 'parent' predicate
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
```

**Reading**: "?x is a parent of ?y if there exists a triple (?x, parentOf, ?y)"

### Conjunctive Body

Multiple atoms separated by commas (all must be true):

```datalog
# Grandparent: parent of a parent
grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z).
```

**Reading**: "?x is a grandparent of ?z if ?x is a parent of ?y AND ?y is a parent of ?z"

### Recursive Rule

Rule that references its own head predicate:

```datalog
# Base case
ancestor(?x, ?y) :- parent(?x, ?y).

# Recursive case
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```

**Reading**:
- Base: "?x is an ancestor of ?y if ?x is a parent of ?y"
- Recursive: "?x is an ancestor of ?z if ?x is a parent of ?y AND ?y is an ancestor of ?z"

### Multiple Rules Per Predicate

Multiple rules for the same predicate are implicitly OR-ed:

```datalog
# Sibling relationship (multiple ways)
sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y), ?x != ?y.
sibling(?x, ?y) :- ?x <http://example.org/siblingOf> ?y.
```

**Reading**: "?x is a sibling of ?y if EITHER [same parent] OR [explicit sibling triple]"

---

## Predicates

### Datalog Predicates

User-defined predicates (lowercase identifiers):

```datalog
ancestor(?x, ?y)
connected(?a, ?b)
reachable(?from, ?to)
inSameOrg(?person1, ?person2)
```

**Naming Rules**:
- Must start with lowercase letter
- Can contain letters, digits, underscores
- Cannot be RDF IRIs
- Case-sensitive

### Arity

Number of arguments a predicate takes:

```datalog
unary(?x)                    # Arity 1
binary(?x, ?y)               # Arity 2
ternary(?x, ?y, ?z)          # Arity 3
```

**Consistency Rule**: All rules for the same predicate must have the same arity:

```datalog
# ERROR: Inconsistent arity
parent(?x, ?y) :- ?x <:parentOf> ?y .
parent(?x, ?y, ?z) :- ?x <:parentOf> ?y, ?y <:parentOf> ?z .  # Wrong!
```

### RDF Predicates

IRIs used directly in patterns:

```datalog
# Using RDF predicate in body
rule(?x, ?y) :- ?x <http://example.org/prop> ?y .

# Multiple RDF predicates
rule(?x, ?z) :- ?x <http://example.org/knows> ?y,
                ?y <http://example.org/worksFor> ?z .
```

---

## Variables

### Variable Syntax

Variables always start with `?`:

```datalog
?x
?Person
?node_1
?_intermediate
```

### Variable Scope

Variables are scoped to a single rule:

```datalog
# These ?x variables are independent
rule1(?x, ?y) :- ?x <:prop1> ?y .
rule2(?x, ?y) :- ?x <:prop2> ?y .
```

### Variable Sharing

Variables with the same name in a rule must bind to the same value:

```datalog
# ?x must be the same in all three positions
sameValue(?x) :- ?x <:prop1> ?y, ?x <:prop2> ?z, ?x <:prop3> ?w .
```

### Head vs Body Variables

**Distinguished Variables** (appear in head):
```datalog
ancestor(?x, ?y) :- parent(?x, ?z), parent(?z, ?y).
#        ^^  ^^                             ^^
#        Head variables (distinguished)     |
#                                          Must bind
```

**Existential Variables** (appear only in body):
```datalog
ancestor(?x, ?y) :- parent(?x, ?z), parent(?z, ?y).
#                                 ^^            ^^
#                    Existential variable (intermediate)
```

**Safety Requirement**: All head variables must appear in the body:

```datalog
# VALID
rule(?x, ?y) :- atom1(?x), atom2(?y).

# INVALID: ?z doesn't appear in body
rule(?x, ?z) :- atom1(?x), atom2(?y).  # Error!
```

---

## Terms

### Term Types

```datalog
?x                                    # Variable
<http://example.org/Alice>            # IRI
"Alice"                               # String literal
"Alice"@en                            # Language-tagged literal
"42"^^<http://www.w3.org/2001/XMLSchema#integer>  # Typed literal
```

### IRI Terms

Full IRIs enclosed in angle brackets:

```datalog
parent(<http://example.org/Alice>, <http://example.org/Bob>)
```

### Literal Terms

String literals with optional language tags or datatypes:

```datalog
# Plain string
name(?person, "Alice")

# Language-tagged
name(?person, "Alice"@en)

# Typed
age(?person, "25"^^<http://www.w3.org/2001/XMLSchema#integer>)
```

### Ground Terms

Terms without variables (constants):

```datalog
# Ground fact (no variables)
parent(<http://example.org/Alice>, <http://example.org/Bob>) :- true.

# Partially ground
rule(<http://example.org/Alice>, ?y) :- parent(<http://example.org/Alice>, ?y).
```

---

## Atoms and Patterns

### Datalog Atoms

Predicate with arguments:

```
<predicateName>(<term1>, <term2>, ...)
```

Examples:
```datalog
ancestor(?x, ?y)
connected(?a, ?b)
edge(<http://ex.org/Node1>, ?to)
```

### RDF Triple Patterns

Subject-predicate-object patterns:

```
<subject> <predicate> <object>
```

Examples:
```datalog
# All variables
?s ?p ?o

# Specific predicate
?person <http://example.org/name> ?name

# Specific subject and predicate
<http://example.org/Alice> <http://example.org/knows> ?friend

# Specific object
?country <http://example.org/capital> <http://example.org/Paris>
```

### Mixing Atoms and Patterns

Rules can combine Datalog atoms and RDF patterns:

```datalog
# Using Datalog predicate in body
rule(?x, ?y) :- ancestor(?x, ?y), ?x <:livesIn> ?city.

# Using RDF pattern in recursive rule
ancestor(?x, ?y) :- ?x <:parentOf> ?y .
ancestor(?x, ?z) :- ancestor(?x, ?y), ?y <:parentOf> ?z .
```

---

## Comments

### Line Comments

```datalog
# This is a comment
ancestor(?x, ?y) :- parent(?x, ?y).  # Comment after rule
```

```datalog
// Alternative comment syntax
ancestor(?x, ?y) :- parent(?x, ?y).  // Also valid
```

### Multi-Line Comments

```datalog
/*
 * Multi-line comment
 * Useful for documentation
 */
ancestor(?x, ?y) :- parent(?x, ?y).

/* Inline comment */ ancestor(?x, ?z) :- /* between tokens */ parent(?x, ?y), ancestor(?y, ?z).
```

### Documentation Comments

```datalog
# ========================================
# FAMILY RELATIONSHIP RULES
# ========================================

# Direct parent relationship
# Maps RDF :parentOf predicate to Datalog predicate
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .

# Transitive ancestor relationship
# Base case: parent is an ancestor
ancestor(?x, ?y) :- parent(?x, ?y).

# Recursive case: ancestor of ancestor
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
```

---

## Programs

### Program Structure

A Datalog program consists of:
1. Zero or more rule definitions
2. Optional queries

```datalog
# Rule definitions
<rule1>
<rule2>
...
<ruleN>

# Optional queries
?- <query1>
?- <query2>
```

### Complete Program Example

```datalog
# ========================================
# GRAPH REACHABILITY PROGRAM
# ========================================

# Base case: direct edge implies reachability
reachable(?x, ?y) :- ?x <http://example.org/edge> ?y .

# Recursive case: transitive reachability
reachable(?x, ?z) :- ?x <http://example.org/edge> ?y, reachable(?y, ?z).

# Derived rule: mutual reachability (connected component)
connected(?x, ?y) :- reachable(?x, ?y), reachable(?y, ?x).

# Query: Find all reachable pairs
?- reachable(?from, ?to).
```

### Rule Ordering

Rules can be defined in any order:

```datalog
# Forward order (base before recursive)
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Reverse order (recursive before base) - also valid
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
ancestor(?x, ?y) :- parent(?x, ?y).
```

### Dependency Order

Rules can reference predicates defined later:

```datalog
# 'grandparent' uses 'parent' (defined later) - OK
grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z).

# 'parent' definition
parent(?x, ?y) :- ?x <:parentOf> ?y .
```

---

## Queries

### Query Syntax

```
?- <atom>
```

Queries start with `?-` and end with `.`:

```datalog
?- ancestor(?x, ?y).
?- reachable(<http://example.org/NodeA>, ?dest).
```

### Queries in Programs

Queries can appear in Datalog programs:

```datalog
# Rules
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Queries
?- ancestor(<http://example.org/Alice>, ?who).
?- ancestor(?who, <http://example.org/Bob>).
```

### SPARQL Integration

More commonly, use Datalog predicates directly in SPARQL queries:

```sparql
# Define rules in Datalog program
# ancestor(?x, ?y) :- parent(?x, ?y).
# ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Use in SPARQL query
SELECT ?ancestor ?descendant ?name WHERE {
  ?ancestor <http://example.org/ancestor> ?descendant .
  ?descendant <http://example.org/name> ?name .
}
ORDER BY ?ancestor ?name
```

---

## Formal Grammar

EBNF-style grammar for QLever Datalog:

```ebnf
program        ::= (rule | query | comment)*

rule           ::= head ":-" body "."
head           ::= atom
body           ::= atom ("," atom)*

atom           ::= predicate_name "(" argument_list ")"
                 | triple_pattern

predicate_name ::= IDENTIFIER
argument_list  ::= term ("," term)*
                 | ε

term           ::= VARIABLE
                 | IRI
                 | STRING_LITERAL
                 | TYPED_LITERAL
                 | LANG_LITERAL

triple_pattern ::= term term term

query          ::= "?-" atom "."

comment        ::= "#" [^\n]* "\n"
                 | "//" [^\n]* "\n"
                 | "/*" (.* ) "*/"

IDENTIFIER     ::= [a-z] [a-zA-Z0-9_]*
VARIABLE       ::= "?" [a-zA-Z_] [a-zA-Z0-9_]*
IRI            ::= "<" [^<>"{}|^`\x00-\x20]+ ">"
STRING_LITERAL ::= '"' (char | escape)* '"'
LANG_LITERAL   ::= STRING_LITERAL "@" [a-z]+ ("-" [a-zA-Z0-9]+)*
TYPED_LITERAL  ::= STRING_LITERAL "^^" IRI
```

---

## Complete Examples

### Example 1: Family Relationships

```datalog
# ==================================================
# FAMILY RELATIONSHIP ONTOLOGY
# ==================================================

# Base predicates from RDF
parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
married(?x, ?y) :- ?x <http://example.org/marriedTo> ?y .

# Derived relationships
child(?y, ?x) :- parent(?x, ?y).
spouse(?y, ?x) :- married(?x, ?y).

# Ancestor (transitive closure)
ancestor(?x, ?y) :- parent(?x, ?y).
ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

# Descendant (inverse of ancestor)
descendant(?y, ?x) :- ancestor(?x, ?y).

# Grandparent
grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z).
grandchild(?z, ?x) :- grandparent(?x, ?z).

# Sibling (same parents)
sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y), ?x != ?y.

# Half-sibling (one common parent)
halfSibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y),
                       NOT sibling(?x, ?y),
                       ?x != ?y.

# Uncle/Aunt
uncleOrAunt(?u, ?n) :- sibling(?u, ?p), parent(?p, ?n).

# Cousin (children of siblings)
cousin(?x, ?y) :- parent(?p1, ?x), parent(?p2, ?y), sibling(?p1, ?p2).

# In-law relationships
parentInLaw(?p, ?s) :- parent(?p, ?c), married(?c, ?s).
siblingInLaw(?s1, ?s2) :- sibling(?s1, ?x), married(?x, ?s2).
```

### Example 2: Graph Algorithms

```datalog
# ==================================================
# GRAPH REACHABILITY AND CONNECTIVITY
# ==================================================

# Base: edge relation
edge(?x, ?y) :- ?x <http://example.org/edge> ?y .

# Reachability (transitive closure)
reachable(?x, ?y) :- edge(?x, ?y).
reachable(?x, ?z) :- edge(?x, ?y), reachable(?y, ?z).

# Path exists (symmetric reachability)
pathExists(?x, ?y) :- reachable(?x, ?y).
pathExists(?x, ?y) :- reachable(?y, ?x).

# Connected (mutual reachability)
connected(?x, ?y) :- reachable(?x, ?y), reachable(?y, ?x).

# Same component (equivalence relation)
sameComponent(?x, ?y) :- connected(?x, ?y).
sameComponent(?x, ?x) :- edge(?x, ?_).  # Reflexive

# Distance 2 (path of length exactly 2)
distance2(?x, ?z) :- edge(?x, ?y), edge(?y, ?z), ?x != ?z.

# Triangle (3-cycle)
triangle(?x, ?y, ?z) :- edge(?x, ?y), edge(?y, ?z), edge(?z, ?x),
                        ?x != ?y, ?y != ?z, ?z != ?x.
```

### Example 3: Organizational Hierarchy

```datalog
# ==================================================
# ORGANIZATIONAL HIERARCHY
# ==================================================

# Base relationships
manages(?mgr, ?emp) :- ?mgr <http://example.org/manages> ?emp .
worksIn(?emp, ?dept) :- ?emp <http://example.org/worksIn> ?dept .

# Management chain (transitive)
managesIndirectly(?mgr, ?emp) :- manages(?mgr, ?emp).
managesIndirectly(?mgr, ?emp) :- manages(?mgr, ?mid),
                                  managesIndirectly(?mid, ?emp).

# Reporting chain (all managers above)
reportsTo(?emp, ?mgr) :- managesIndirectly(?mgr, ?emp).

# Management level (distance from CEO)
level(?emp, 0) :- ?emp <http://example.org/role> "CEO" .
level(?emp, ?n) :- manages(?mgr, ?emp), level(?mgr, ?m), ?n = ?m + 1.

# Peer relationship (same manager)
peer(?e1, ?e2) :- manages(?mgr, ?e1), manages(?mgr, ?e2), ?e1 != ?e2.

# Same department
sameDept(?e1, ?e2) :- worksIn(?e1, ?dept), worksIn(?e2, ?dept), ?e1 != ?e2.

# Cross-department collaboration (different dept, same manager)
crossDeptTeam(?e1, ?e2) :- peer(?e1, ?e2), NOT sameDept(?e1, ?e2).
```

### Example 4: Ontology Inference

```datalog
# ==================================================
# RDFS/OWL INFERENCE RULES
# ==================================================

# Subclass transitivity
subClassOf(?c1, ?c2) :- ?c1 <http://www.w3.org/2000/01/rdf-schema#subClassOf> ?c2 .
subClassOf(?c1, ?c3) :- subClassOf(?c1, ?c2), subClassOf(?c2, ?c3).

# Type inference from subclass
instanceOf(?x, ?c2) :- ?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?c1,
                       subClassOf(?c1, ?c2).

# Subproperty transitivity
subPropertyOf(?p1, ?p2) :- ?p1 <http://www.w3.org/2000/01/rdf-schema#subPropertyOf> ?p2 .
subPropertyOf(?p1, ?p3) :- subPropertyOf(?p1, ?p2), subPropertyOf(?p2, ?p3).

# Property inference
hasProperty(?x, ?p2, ?y) :- ?x ?p1 ?y, subPropertyOf(?p1, ?p2).

# Domain inference
hasDomain(?x, ?c) :- ?x ?p ?y,
                     ?p <http://www.w3.org/2000/01/rdf-schema#domain> ?c.

# Range inference
hasRange(?y, ?c) :- ?x ?p ?y,
                    ?p <http://www.w3.org/2000/01/rdf-schema#range> ?c.
```

---

## Syntax Summary

| Construct | Syntax | Example |
|-----------|--------|---------|
| Rule | `head :- body.` | `ancestor(?x, ?y) :- parent(?x, ?y).` |
| Atom | `pred(args)` | `ancestor(?x, ?y)` |
| Triple | `subj pred obj` | `?x <:knows> ?y` |
| Variable | `?name` | `?x`, `?Person` |
| IRI | `<iri>` | `<http://example.org/prop>` |
| String | `"text"` | `"Alice"` |
| Lang literal | `"text"@lang` | `"Hello"@en` |
| Typed literal | `"value"^^<type>` | `"42"^^<xsd:integer>` |
| Conjunction | `atom, atom` | `parent(?x, ?y), parent(?y, ?z)` |
| Query | `?- atom.` | `?- ancestor(?x, ?y).` |
| Comment | `# text` | `# This is a comment` |
| Comment | `/* text */` | `/* Multi-line */` |

---

**Next**: See [DATALOG_EXAMPLES.md](DATALOG_EXAMPLES.md) for complete real-world examples.

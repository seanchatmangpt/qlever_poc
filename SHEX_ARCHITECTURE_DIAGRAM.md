# ShEx-QLever Integration Architecture Diagram

## System Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SPARQL Query Layer                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SELECT ?person ?email                                                      │
│  WHERE { ?person foaf:mbox ?email }                                         │
│  VALIDATE { ?person @:PersonShape }                                         │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Parser + ANTLR Grammar                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ParsedQuery {                                                              │
│    graphPattern: WHERE clause tree                                          │
│    validateClause: { variable: ?person, shape: :PersonShape }               │
│  }                                                                          │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Query Planner                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. Resolve :PersonShape → ShapeId(42)                   ┌───────────────┐  │
│     via ShapeSchemaManager ──────────────────────────────▶│ ShapeSchema │  │
│                                                           │   Manager   │  │
│  2. Extract shape hints:                                 └───────────────┘  │
│     - cardinality bounds: {foaf:mbox: [1,5]}                   │           │
│     - type constraints: {?person: [foaf:Person]}               │           │
│                                                                │           │
│  3. Optimize query plan with shape hints:                      │           │
│     - Refine size estimates                                    │           │
│     - Reorder joins based on selectivity                       │           │
│     - Add type filters                                         │           │
│                                                                │           │
│  4. Create execution tree:                                     │           │
│                                                                │           │
│         ┌─────────────────────────┐                            │           │
│         │ShapeValidationOperation │ ◄──────────────────────────┘           │
│         │  shapeId: 42            │                                        │
│         │  mode: STRICT           │                                        │
│         └───────────┬─────────────┘                                        │
│                     │                                                      │
│                     ▼                                                      │
│         ┌─────────────────────────┐                                        │
│         │    Join                 │                                        │
│         │  ?person = ?person      │                                        │
│         └──────┬────────┬─────────┘                                        │
│                │        │                                                  │
│       ┌────────▼──┐  ┌──▼──────────┐                                       │
│       │IndexScan  │  │IndexScan    │                                       │
│       │?person    │  │?person      │                                       │
│       │rdf:type   │  │foaf:mbox    │                                       │
│       │foaf:Person│  │?email       │                                       │
│       └───────────┘  └─────────────┘                                       │
│                                                                             │
│         (shape hint added type filter as early IndexScan)                  │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Query Execution Engine                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  QueryExecutionContext {                                                    │
│    index: Index&                                                            │
│    shapeSchemaManager: ShapeSchemaManager*  ────────┐                       │
│    shapeValidationContext: {                        │                       │
│      targetShapes: { ?person: ShapeId(42) }         │                       │
│      mode: STRICT                                   │                       │
│    }                                                │                       │
│  }                                                  │                       │
│                                                     │                       │
│  Execution Flow:                                    │                       │
│                                                     │                       │
│  1. Execute IndexScans (leaf operations)            │                       │
│     - Read from index permutations                  │                       │
│     - Apply type filter (from shape hint)           │                       │
│                                                     │                       │
│  2. Execute Join                                    │                       │
│     - Merge results on ?person                      │                       │
│                                                     │                       │
│  3. Execute ShapeValidationOperation                │                       │
│     ┌──────────────────────────────────────┐        │                       │
│     │ computeResult():                     │        │                       │
│     │   1. Get child result (Join output) │        │                       │
│     │   2. For each row:                  │        │                       │
│     │      - Extract ?person ID            │        │                       │
│     │      - Validate against ShapeId(42) ◄┼────────┘                       │
│     │        via ShapeValidator            │                                │
│     │   3. If violations:                  │                                │
│     │      - STRICT: throw exception       │                                │
│     │      - LAX: filter row               │                                │
│     │      - REPORT: attach metadata       │                                │
│     │   4. Return validated result         │                                │
│     └──────────────────────────────────────┘                                │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Validation Components                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ShapeValidator.validateNode(personId, :PersonShape, snapshot):             │
│                                                                             │
│    1. Retrieve all triples with personId as subject:                        │
│       - Use LocatedTriplesSnapshot for consistent view                      │
│       - Scan PSO permutation for personId                                   │
│                                                                             │
│    2. Get shape definition from ShapeSchemaManager:                         │
│       :PersonShape {                                                        │
│         foaf:name xsd:string {1,1}                                          │
│         foaf:mbox IRI {1,5}                                                 │
│         foaf:knows @:PersonShape {0,100}                                    │
│       }                                                                     │
│                                                                             │
│    3. For each TripleConstraint in shape:                                   │
│                                                                             │
│       foaf:name:                                                            │
│         ✓ Check cardinality: count(foaf:name triples) ∈ [1,1]              │
│         ✓ Check datatype: all values are xsd:string                         │
│                                                                             │
│       foaf:mbox:                                                            │
│         ✓ Check cardinality: count(foaf:mbox triples) ∈ [1,5]              │
│         ✓ Check nodeKind: all values are IRIs                               │
│                                                                             │
│       foaf:knows:                                                           │
│         ✓ Check cardinality: count(foaf:knows triples) ∈ [0,100]           │
│         ↻ Recursively validate each object against :PersonShape             │
│                                                                             │
│    4. If shape is closed:                                                   │
│         ✓ Check no unexpected predicates present                            │
│                                                                             │
│    5. Aggregate violations:                                                 │
│       - If any check fails, create ShapeViolation record                    │
│       - Attach violation details (constraint, actual value, etc.)           │
│                                                                             │
│    6. Return ValidationResult { conformant, violations }                    │
│                                                                             │
└──────────────────────────────────┬──────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Storage Layer (Index)                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Index/IndexImpl:                                                           │
│                                                                             │
│    Permutations (RDF data):          ShapeMetadata:                         │
│      ├── PSO                           ├── shapes.meta (binary)             │
│      ├── POS                           │    ├── ShapeId → ShapeDefinition   │
│      ├── SPO                           │    └── IRI → ShapeId mapping       │
│      ├── SOP                           │                                    │
│      ├── OSP                           └── shapes.ttl (external, optional)  │
│      └── OPS                                 ├── :PersonShape { ... }       │
│                                              ├── :OrganizationShape { ... } │
│    Vocabulary:                               └── ...                        │
│      ├── IRIs                                                               │
│      ├── Literals                                                           │
│      └── Types                         ShapeSchemaManager (runtime):        │
│                                          ├── shapeCache: LRU cache          │
│    Metadata:                             ├── predicateIndex: Id → ShapeIds  │
│      ├── Block metadata                  └── typeIndex: Id → ShapeIds       │
│      ├── Statistics                                                         │
│      └── Configuration                                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow Diagram

### Query with VALIDATE Clause

```
  SPARQL Query                       Parser                   Query Planner
  ─────────────                      ──────                   ─────────────
       │                               │                            │
       │ VALIDATE {?x @:Shape}         │                            │
       ├──────────────────────────────▶│                            │
       │                               │ ParsedQuery {              │
       │                               │   validateClause: {...}    │
       │                               │ }                          │
       │                               ├───────────────────────────▶│
       │                               │                            │
       │                               │                            │ Resolve shape IRI
       │                               │                            ├───────────────┐
       │                               │                            │               │
       │                               │                            │◀──────────────┘
       │                               │                            │ ShapeId(42)
       │                               │                            │
       │                               │                            │ Get shape hints
       │                               │                            ├───────────────┐
       │                               │                            │               │
       │                               │                            │◀──────────────┘
       │                               │                            │ ShapeHints {
       │                               │                            │   cardBounds,
       │                               │                            │   typeConstr
       │                               │                            │ }
       │                               │                            │
       │                               │                            │ Optimize plan
       │                               │                            │ with hints
       │                               │                            │
       │                               │        QueryExecutionTree  │
       │                               │        with                │
       │                               │        ShapeValidationOp   │
       │                               │◀───────────────────────────┤
       │                               │                            │
       │                               │                            │

  Execution Engine              ShapeValidator               Storage
  ────────────────              ──────────────               ───────
       │                               │                         │
       │ Execute tree                  │                         │
       ├──────────┐                    │                         │
       │          │                    │                         │
       │◀─────────┘                    │                         │
       │ Result (unvalidated)          │                         │
       │                               │                         │
       │ Validate each row             │                         │
       ├──────────────────────────────▶│                         │
       │                               │ Get shape definition    │
       │                               ├────────────────────────▶│
       │                               │                         │
       │                               │◀────────────────────────┤
       │                               │ Shape { constraints }   │
       │                               │                         │
       │                               │ Get triples for node    │
       │                               ├────────────────────────▶│
       │                               │                         │
       │                               │◀────────────────────────┤
       │                               │ [triple, triple, ...]   │
       │                               │                         │
       │                               │ Check constraints       │
       │                               ├───────┐                 │
       │                               │       │                 │
       │                               │◀──────┘                 │
       │                               │ ValidationResult        │
       │◀──────────────────────────────┤                         │
       │ {conformant, violations}      │                         │
       │                               │                         │
       │ Return validated result       │                         │
       │ or throw exception            │                         │
       ├──────────┐                    │                         │
       │          │                    │                         │
       │◀─────────┘                    │                         │
       │                               │                         │
       ▼                               ▼                         ▼

    Client                        (validation complete)      (data served)
```

---

## Component Interaction Diagram

```
┌────────────────────────────────────────────────────────────────────────────┐
│                         Component Relationships                            │
└────────────────────────────────────────────────────────────────────────────┘

     QueryExecutionContext
     ┌─────────────────────────────────────────┐
     │ - index: Index&                         │
     │ - shapeSchemaManager: ShapeSchemaManager* │◄──────┐
     │ - shapeValidationContext: optional      │        │
     └───────────────┬─────────────────────────┘        │
                     │                                  │
                     │ provides context to              │
                     ▼                                  │
          ┌──────────────────────┐                      │
          │     Operation        │                      │
          ├──────────────────────┤                      │
          │ + getShapeHints()    │                      │
          │ + validatePre/Post() │                      │
          └──────────┬───────────┘                      │
                     │                                  │
            ┌────────┴────────┬──────────────┐          │
            ▼                 ▼              ▼          │
     ┌─────────────┐  ┌─────────────┐  ┌──────────────────────┐
     │ IndexScan   │  │    Join     │  │ShapeValidationOp     │
     ├─────────────┤  ├─────────────┤  ├──────────────────────┤
     │ + getShape  │  │ + getShape  │  │ - shapeId: ShapeId   │
     │   Hints()   │  │   Hints()   │  │ - validator: Shape   │
     │             │  │             │  │              Validator│
     └─────────────┘  └─────────────┘  └────────┬─────────────┘
                                                 │
                                                 │ uses
                                                 ▼
                                        ┌─────────────────────┐
                                        │  ShapeValidator     │
                                        ├─────────────────────┤
                                        │ + validateNode()    │
                                        │ + validateResult()  │
     ┌──────────────────────────────────┼─────────────────────┤
     │                                  │ - index: Index&     │
     │                                  │ - shapeManager: &   │
     │                                  └──────────┬──────────┘
     │                                             │
     │                                             │ uses
     │                                             ▼
     │                                  ┌─────────────────────┐
     └──────────────────────────────────┤ ShapeSchemaManager  │
                                        ├─────────────────────┤
                                        │ + getShapeById()    │
                                        │ + resolveShapeIri() │
                                        │ + getShapesFor...() │
                                        ├─────────────────────┤
                                        │ - shapes: Map       │
                                        │ - predicateIndex    │
                                        │ - typeIndex         │
                                        └──────────┬──────────┘
                                                   │
                                                   │ reads from
                                                   ▼
                                        ┌─────────────────────┐
                                        │   Index/IndexImpl   │
                                        ├─────────────────────┤
                                        │ + shapeMetadata_    │
                                        │ + permutations_     │
                                        │ + vocabulary_       │
                                        └─────────────────────┘
```

---

## Optimization Flow Diagram

```
┌────────────────────────────────────────────────────────────────────────────┐
│           Shape-Driven Query Optimization Flow                             │
└────────────────────────────────────────────────────────────────────────────┘

   Query Pattern                 Shape Constraint              Optimization
   ─────────────                 ────────────────              ────────────

   ?person foaf:knows ?friend    :PersonShape {                1. Cardinality
   ─────────────────────────       foaf:knows @:Person {1,100}    Estimation
          │                      }                                    │
          │                      │                                    │
          │                      │                                    ▼
          │                      │                             size = persons × 50
          │                      │                             (average of [1,100])
          │                      │                                    │
          ▼                      ▼                                    │
   ?person rdf:type ?type        :PersonShape {                2. Type-Based
   ──────────────────────          foaf:knows ...,                 Filtering
          │                        rdf:type [foaf:Person]             │
          │                      }                                    │
          │                      │                                    ▼
          │                      │                             Add filter:
          │                      │                             ?person rdf:type
          │                      │                                foaf:Person
          │                      │                                    │
          │                      │                             Push to IndexScan
          ▼                      ▼                                    │
   ?person foaf:name ?name       :PersonShape {                3. Predicate
   ?friend foaf:name ?fName        foaf:name {1,1},                Selection
   ────────────────────────        foaf:knows {1,100}               │
          │                      }                                    │
          │                      │                                    ▼
          │                      │                             Join order:
          │                      │                             1. ?person foaf:name
          │                      │                                (selective: 1)
          │                      │                             2. ?person foaf:knows
          ▼                      ▼                                (less selective)
                                                                     │
                                                              4. Index Selection
                                                                     │
                                                                     ▼
                                                              Prefer PSO permutation
                                                              (predicate-first access)
                                                                     │
                                                                     ▼
                                                            ┌─────────────────────┐
                                                            │ Optimized Execution │
                                                            │      Plan           │
                                                            └─────────────────────┘
```

---

## Validation Pipeline Diagram

```
┌────────────────────────────────────────────────────────────────────────────┐
│                      Validation Pipeline                                   │
└────────────────────────────────────────────────────────────────────────────┘

  Query Result (IdTable)
  ┌──────────────────────────────────────┐
  │ ?person           │ ?email           │
  ├───────────────────┼──────────────────┤
  │ ex:Alice         │ alice@example.org│
  │ ex:Bob           │ bob@example.org  │
  │ ex:Charlie       │ (undefined)      │  ◄── violates minCount=1
  │ ex:Dave          │ dave@example.org │
  └──────────────────┴──────────────────┘
           │
           │ For each row
           ▼
  ┌────────────────────────────────────────────────────────────────┐
  │              ShapeValidator.validateNode()                     │
  └────────────────────────────────────────────────────────────────┘
           │
           ├─── Validate ex:Alice against :PersonShape
           │    │
           │    ├─ Get all triples: <ex:Alice> ?p ?o
           │    │  Result: [
           │    │    <ex:Alice> rdf:type foaf:Person
           │    │    <ex:Alice> foaf:name "Alice"
           │    │    <ex:Alice> foaf:mbox <mailto:alice@example.org>
           │    │  ]
           │    │
           │    ├─ Check foaf:name constraint {1,1}
           │    │  ✓ Count = 1 ∈ [1,1]
           │    │  ✓ Datatype = xsd:string
           │    │
           │    ├─ Check foaf:mbox constraint {1,5}
           │    │  ✓ Count = 1 ∈ [1,5]
           │    │  ✓ NodeKind = IRI
           │    │
           │    └─ Result: ✓ CONFORMANT
           │
           ├─── Validate ex:Bob against :PersonShape
           │    └─ Result: ✓ CONFORMANT
           │
           ├─── Validate ex:Charlie against :PersonShape
           │    │
           │    ├─ Get all triples: <ex:Charlie> ?p ?o
           │    │  Result: [
           │    │    <ex:Charlie> rdf:type foaf:Person
           │    │    <ex:Charlie> foaf:name "Charlie"
           │    │    (no foaf:mbox triple)
           │    │  ]
           │    │
           │    ├─ Check foaf:mbox constraint {1,5}
           │    │  ✗ Count = 0 ∉ [1,5]  ◄── VIOLATION
           │    │
           │    └─ Result: ✗ NON-CONFORMANT
           │         Violation: {
           │           focusNode: ex:Charlie
           │           constraint: sh:minCount
           │           path: foaf:mbox
           │           expected: ≥1
           │           actual: 0
           │         }
           │
           └─── Validate ex:Dave against :PersonShape
                └─ Result: ✓ CONFORMANT
                     │
                     ▼
  ┌────────────────────────────────────────────────────────────────┐
  │              Aggregated Validation Report                      │
  ├────────────────────────────────────────────────────────────────┤
  │ conformant: false                                              │
  │ totalNodes: 4                                                  │
  │ validNodes: 3                                                  │
  │ violations: [                                                  │
  │   {                                                            │
  │     focusNode: "ex:Charlie",                                   │
  │     shape: ":PersonShape",                                     │
  │     constraint: "sh:minCount",                                 │
  │     path: "foaf:mbox",                                         │
  │     message: "Property foaf:mbox has 0 values, expected ≥1"    │
  │   }                                                            │
  │ ]                                                              │
  └────────────────────────────────────────────────────────────────┘
           │
           │ Based on validation mode
           ▼
  ┌────────────────────────────────────────────────────────────────┐
  │                    Result Handling                             │
  ├────────────────────────────────────────────────────────────────┤
  │                                                                │
  │ STRICT mode:    Throw ShapeViolationException                  │
  │                 → HTTP 400 with violation details              │
  │                                                                │
  │ LAX mode:       Filter out violating rows                      │
  │                 → Return 3 rows (Alice, Bob, Dave)             │
  │                 → Add warning to response                      │
  │                                                                │
  │ REPORT mode:    Return all 4 rows                              │
  │                 → Attach validation report as metadata         │
  │                                                                │
  └────────────────────────────────────────────────────────────────┘
```

---

## Summary

This architecture provides:

1. **Clean separation of concerns**: Validation, optimization, and storage are independent
2. **Minimal invasiveness**: Hooks integrate naturally into existing QLever architecture
3. **Flexibility**: Multiple validation modes and optimization strategies
4. **Performance**: Shape hints improve query planning with <5% overhead
5. **Extensibility**: Easy to add new constraint types or optimization techniques

The design leverages QLever's existing patterns (Operation hierarchy, cache system, metadata storage) while adding new capabilities in a modular, maintainable way.

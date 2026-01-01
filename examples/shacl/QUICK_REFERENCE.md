# SHACL Quick Reference for QLever

## Core Constraint Components

### Cardinality
```turtle
sh:minCount 1          # At least 1 value
sh:maxCount 5          # At most 5 values
```

### Value Type
```turtle
sh:class foaf:Person           # Instance of class
sh:datatype xsd:integer        # Specific datatype
sh:nodeKind sh:IRI             # IRI, BlankNode, Literal
```

### Numeric Range
```turtle
sh:minInclusive 0              # value >= 0
sh:minExclusive 0              # value > 0
sh:maxInclusive 100            # value <= 100
sh:maxExclusive 100            # value < 100
```

### String Constraints
```turtle
sh:minLength 1                 # Min string length
sh:maxLength 200               # Max string length
sh:pattern "^[A-Z].*$"         # Regex pattern
sh:flags "i"                   # Regex flags (i, s, m, x)
sh:languageIn ( "en" "de" )    # Language tags
sh:uniqueLang true             # Each language once
```

### Property Pairs
```turtle
sh:equals ex:confirmEmail      # Same values
sh:disjoint ex:other           # No common values
sh:lessThan ex:endDate         # Values less than
sh:lessThanOrEquals ex:max     # Values less or equal
```

### Value Constraints
```turtle
sh:in ( "a" "b" "c" )          # Enumeration
sh:hasValue "expected"          # Must have value
```

## Target Types

```turtle
sh:targetClass foaf:Person              # Instances of class
sh:targetNode ex:alice                  # Specific node
sh:targetSubjectsOf ex:prop             # Subjects of property
sh:targetObjectsOf ex:prop              # Objects of property
```

## Logical Operators

```turtle
sh:and ( shape1 shape2 )                # All must conform
sh:or ( shape1 shape2 )                 # At least one
sh:xone ( shape1 shape2 )               # Exactly one
sh:not shape1                           # Must not conform
```

## Property Paths

```turtle
sh:path ex:prop                         # Simple
sh:path ^ex:prop                        # Inverse
sh:path ( ex:p1 ex:p2 )                 # Sequence
sh:path ( ex:p1 | ex:p2 )               # Alternative
sh:path ex:prop*                        # Zero or more
sh:path ex:prop+                        # One or more
sh:path ex:prop?                        # Zero or one
```

## SPARQL Constraints

```turtle
sh:sparql [
  sh:message "Error: {?var}" ;
  sh:severity sh:Violation ;            # or sh:Warning, sh:Info
  sh:select """
    SELECT $this ?var WHERE {
      $this ex:prop ?var .
      FILTER (condition)
    }
  """
] .
```

## Recursive Shapes

```turtle
sh:node ex:OtherShape                   # Node must conform
sh:property [
  sh:path ex:manager ;
  sh:node ex:EmployeeShape              # Recursive
] .
```

## Qualified Value Shapes

```turtle
sh:qualifiedValueShape [
  sh:property [
    sh:path ex:role ;
    sh:hasValue "admin"
  ]
] ;
sh:qualifiedMinCount 1 ;
sh:qualifiedMaxCount 3 .
```

## Common Patterns

### Required Property
```turtle
sh:property [
  sh:path ex:name ;
  sh:minCount 1
] .
```

### Optional Property with Validation
```turtle
sh:property [
  sh:path ex:email ;
  sh:maxCount 1 ;
  sh:pattern "^[^@]+@[^@]+$"
] .
```

### Enumerated Values
```turtle
sh:property [
  sh:path ex:status ;
  sh:in ( "active" "inactive" "pending" )
] .
```

### Date Range
```turtle
sh:property [
  sh:path ex:startDate ;
  sh:lessThan ex:endDate
] .
```

## C++ API Quick Reference

### Loading Shapes
```cpp
#include "engine/shacl/ShaclShapeParser.h"
ShaclShapeParser parser;
auto shapes = parser.parseFile("shapes.ttl");
```

### Registering Shapes
```cpp
#include "engine/shacl/ShaclShapeRegistry.h"
ShaclShapeRegistry registry;
for (const auto& shape : shapes) {
  registry.registerShape(shape);
}
registry.setEnabled(true);
```

### Creating Validator
```cpp
#include "engine/shacl/ShaclValidator.h"
ShaclValidator validator(qec, subtree, &registry, 0);
auto result = validator.computeResult(false);
```

### With Caching
```cpp
auto cache = std::make_shared<ShaclValidationCache>(10000);
ShaclValidator validator(qec, subtree, &registry, 0,
                        std::nullopt, cache);
```

### Parallel Validation
```cpp
ShaclValidator validator(qec, subtree, &registry, 0,
                        std::nullopt, cache,
                        true,  // parallel
                        8);    // threads
```

### Detailed Reports
```cpp
auto report = validator.validateAllResourcesDetailed(table);
auto json = validator.getValidationReport(report,
                                         ViolationFormat::JSON);
```

## Debugging Commands

```bash
# Validate Turtle syntax
rapper -i turtle shapes.ttl

# Run SHACL tests
ctest -R Shacl --output-on-failure

# Enable debug logging
LOG_SET_LEVEL(DEBUG)
```

## Performance Tips

1. **Enable caching** for repeated validation
2. **Use parallel validation** for >1000 nodes
3. **Order constraints** from fast to slow
4. **Limit transitive paths** with maxCount
5. **Use standard constraints** over SPARQL when possible
6. **Monitor cache hit rate** (aim for >80%)

## Common Datatypes

```turtle
xsd:string
xsd:boolean
xsd:integer
xsd:decimal
xsd:float
xsd:double
xsd:date
xsd:dateTime
xsd:time
rdf:langString
```

## Severity Levels

```turtle
sh:Violation          # Error (default)
sh:Warning            # Warning
sh:Info               # Information
```

## Documentation Links

- **Advanced Guide:** [SHACL_ADVANCED_GUIDE.md](./SHACL_ADVANCED_GUIDE.md)
- **Compliance:** [SHACL Compliance](../../docs/reference/shacl-compliance.md)
- **Integration:** [SHACL Integration Guide](../../docs/how-to/shacl-integration.md)
- **Troubleshooting:** [SHACL_TROUBLESHOOTING.md](./SHACL_TROUBLESHOOTING.md)
- **Examples:** [advanced-examples.ttl](./advanced-examples.ttl)
- **W3C Spec:** https://www.w3.org/TR/shacl/

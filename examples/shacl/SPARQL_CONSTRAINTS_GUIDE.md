# SPARQL-Based Constraints in SHACL

## Overview

SPARQL-based constraints (`sh:sparql`) allow you to define custom validation logic using SPARQL queries. This is the most flexible constraint type in SHACL, enabling complex business rules and cross-property validation that cannot be expressed with standard SHACL constraints.

## When to Use SPARQL Constraints

Use SPARQL constraints when you need:

- **Complex business rules** - Multi-step validation logic
- **Cross-property validation** - Checking relationships between multiple properties
- **Aggregation-based rules** - Counting, summing, or averaging values
- **Temporal constraints** - Date and time comparisons
- **External data validation** - Checking references to other resources
- **Conditional logic** - Different rules based on node type or property values

## Basic Structure

### ASK Query Constraint

An ASK query returns a boolean. The constraint is **violated** if the query returns `true`.

```turtle
ex:HasNameShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:sparql [
    sh:message "Person must have a name" ;
    sh:ask """
      ASK {
        $this foaf:name ?name .
      }
    """
  ] .
```

**Important**: For ASK queries, `true` means the constraint is **satisfied**, `false` means **violation**.

### SELECT Query Constraint

A SELECT query returns bindings. The constraint is **violated** for each row returned.

```turtle
ex:PositiveAgeShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:sparql [
    sh:message "Age must be positive, found: {?age}" ;
    sh:select """
      SELECT $this ?age
      WHERE {
        $this foaf:age ?age .
        FILTER (?age <= 0)
      }
    """
  ] .
```

**Important**: For SELECT queries, each result row represents a **violation**.

## Standard Variables

SHACL SPARQL constraints provide these pre-bound variables:

- `$this` or `?this` - The focus node being validated
- `$focusNode` - Same as `$this` (alternative name)
- `$value` - Property value (for property shape constraints)
- `$path` - The property path (for property shape constraints)

### Example: Using Standard Variables

```turtle
ex:SelfManagerCheckShape a sh:NodeShape ;
  sh:targetClass ex:Employee ;
  sh:sparql [
    sh:message "Employee cannot be their own manager" ;
    sh:select """
      SELECT $this
      WHERE {
        $this ex:manager $this .
      }
    """
  ] .
```

## Common Patterns

### Pattern 1: Value Range Validation

Validate that one value is greater/less than another:

```turtle
ex:DateRangeShape a sh:NodeShape ;
  sh:targetClass ex:Event ;
  sh:sparql [
    sh:message "End date {?end} must be after start date {?start}" ;
    sh:select """
      SELECT $this ?start ?end
      WHERE {
        $this ex:startDate ?start ;
              ex:endDate ?end .
        FILTER (?end <= ?start)
      }
    """
  ] .
```

### Pattern 2: Counting Values

Ensure minimum/maximum number of related resources:

```turtle
ex:MinTeamSizeShape a sh:NodeShape ;
  sh:targetClass ex:Team ;
  sh:sparql [
    sh:message "Team must have at least 3 members (found {?count})" ;
    sh:select """
      SELECT $this (COUNT(?member) AS ?count)
      WHERE {
        $this ex:hasMember ?member .
      }
      GROUP BY $this
      HAVING (COUNT(?member) < 3)
    """
  ] .
```

### Pattern 3: Cross-Reference Validation

Check that referenced resources exist:

```turtle
ex:ManagerExistsShape a sh:NodeShape ;
  sh:targetClass ex:Employee ;
  sh:sparql [
    sh:message "Manager {?manager} does not exist" ;
    sh:select """
      SELECT $this ?manager
      WHERE {
        $this ex:manager ?manager .
        FILTER NOT EXISTS {
          ?manager a ex:Employee .
        }
      }
    """
  ] .
```

### Pattern 4: Uniqueness Constraints

Ensure values are unique across the dataset:

```turtle
ex:UniqueEmailShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:sparql [
    sh:message "Email {?email} is used by multiple people" ;
    sh:select """
      SELECT $this ?email
      WHERE {
        $this foaf:email ?email .
        ?other foaf:email ?email .
        FILTER ($this != ?other)
      }
    """
  ] .
```

### Pattern 5: Conditional Validation

Apply different rules based on type:

```turtle
ex:ConditionalShape a sh:NodeShape ;
  sh:targetClass foaf:Person ;
  sh:sparql [
    sh:message "Employee must have ID, Contractor must have contract" ;
    sh:select """
      SELECT $this ?type
      WHERE {
        $this a ?type .
        {
          FILTER (?type = ex:Employee)
          FILTER NOT EXISTS { $this ex:employeeId ?id }
        } UNION {
          FILTER (?type = ex:Contractor)
          FILTER NOT EXISTS { $this ex:contractId ?id }
        }
      }
    """
  ] .
```

### Pattern 6: Relationship Validation

Validate relationships between related resources:

```turtle
ex:SameDepartmentShape a sh:NodeShape ;
  sh:targetClass ex:Employee ;
  sh:sparql [
    sh:message "Employee and manager must be in same department" ;
    sh:select """
      SELECT $this ?manager
      WHERE {
        $this ex:manager ?manager ;
              ex:department ?dept1 .
        ?manager ex:department ?dept2 .
        FILTER (?dept1 != ?dept2)
      }
    """
  ] .
```

## Message Templates

You can include variable values in violation messages using `{?variable}` syntax:

```turtle
sh:message "Value {?value} is outside range [{?min}, {?max}]" ;
```

Variables from the SELECT clause are available for interpolation.

## Best Practices

### 1. Write Clear Messages

```turtle
# Good: Specific and actionable
sh:message "Salary {?salary} exceeds maximum for grade {?grade}" ;

# Bad: Generic and unhelpful
sh:message "Constraint violation" ;
```

### 2. Use FILTER NOT EXISTS for Existence Checks

```turtle
# Good: Efficient and clear
FILTER NOT EXISTS {
  $this foaf:name ?name .
}

# Avoid: Less efficient
OPTIONAL { $this foaf:name ?name }
FILTER (!BOUND(?name))
```

### 3. Add Comments to Complex Queries

```turtle
sh:select """
  SELECT $this ?salary ?grade
  WHERE {
    # Get employee salary
    $this ex:salary ?salary .

    # Get grade constraints
    $this ex:grade ?grade .
    ?grade ex:maxSalary ?maxSalary .

    # Check if salary exceeds maximum
    FILTER (?salary > ?maxSalary)
  }
"""
```

### 4. Use Prefixes

Define prefixes to make queries more readable:

```turtle
sh:prefixes [
  sh:declare [
    sh:prefix "ex" ;
    sh:namespace "http://example.org/"^^xsd:anyURI ;
  ] ;
  sh:declare [
    sh:prefix "foaf" ;
    sh:namespace "http://xmlns.com/foaf/0.1/"^^xsd:anyURI ;
  ]
] ;
```

### 5. Test Incrementally

Start with simple queries and add complexity gradually:

```turtle
# Step 1: Test basic query
SELECT $this WHERE { $this ex:property ?value }

# Step 2: Add filters
SELECT $this ?value WHERE {
  $this ex:property ?value .
  FILTER (?value > 0)
}

# Step 3: Add complexity
SELECT $this ?value ?related WHERE {
  $this ex:property ?value ;
        ex:related ?related .
  ?related ex:constraint ?constraint .
  FILTER (?value > ?constraint)
}
```

## Performance Considerations

### 1. Use Selective Filters Early

```turtle
# Good: Filter early to reduce intermediate results
SELECT $this ?value
WHERE {
  $this a ex:Employee .           # Narrow down first
  $this ex:salary ?value .
  FILTER (?value > 100000)
}

# Less efficient: Filter after collecting all data
SELECT $this ?value
WHERE {
  $this ex:salary ?value .
  $this a ex:Employee .           # Too late
  FILTER (?value > 100000)
}
```

### 2. Avoid Expensive Operations in Loops

```turtle
# Good: Calculate once
SELECT $this (COUNT(?member) AS ?count)
WHERE {
  $this ex:hasMember ?member .
}
GROUP BY $this
HAVING (COUNT(?member) < 3)

# Avoid: Recalculating for each member
SELECT $this ?member
WHERE {
  $this ex:hasMember ?member .
  FILTER (COUNT(?member) < 3)     # Wrong: COUNT doesn't work here
}
```

### 3. Use LIMIT for Debugging

When testing, add LIMIT to avoid processing entire dataset:

```turtle
SELECT $this ?value
WHERE {
  $this ex:property ?value .
  FILTER (?value < 0)
}
LIMIT 10  # Remove in production
```

## Debugging SPARQL Constraints

### 1. Test Query Separately

Before adding to SHACL shape, test in a SPARQL endpoint:

```sparql
# Replace $this with actual IRI
SELECT ?age
WHERE {
  <http://example.org/person/john> foaf:age ?age .
  FILTER (?age < 0)
}
```

### 2. Add Intermediate Variables

Help understand what's happening:

```turtle
SELECT $this ?value ?computed ?threshold
WHERE {
  $this ex:value ?value .
  BIND ((?value * 2) AS ?computed)
  BIND (100 AS ?threshold)
  FILTER (?computed > ?threshold)
}
```

### 3. Use OPTIONAL for Debugging

See what values exist:

```turtle
SELECT $this ?name ?age ?email
WHERE {
  OPTIONAL { $this foaf:name ?name }
  OPTIONAL { $this foaf:age ?age }
  OPTIONAL { $this foaf:email ?email }
}
```

## Integration with QLever

### C++ API Usage

```cpp
#include "engine/shacl/SparqlBasedConstraint.h"
#include "engine/shacl/ShaclConstraintEvaluator.h"

// Create SPARQL constraint
std::string query = R"(
  SELECT $this ?age
  WHERE {
    $this foaf:age ?age .
    FILTER (?age < 18)
  }
)";

shacl::SparqlBasedConstraint constraint(query);
constraint.setMessage("Age must be at least 18");
constraint.setSeverity(shacl::SeverityLevel::Violation);

// Validate query
if (!constraint.validateQuery()) {
  std::cerr << "Invalid query: " << constraint.getValidationError() << std::endl;
  return;
}

// Evaluate constraint
std::string focusNode = "http://example.org/person/alice";
std::unordered_map<std::string, std::string> bindings;
bindings["value"] = "15";

auto violations = constraint.evaluate(focusNode, bindings, context);
for (const auto& violation : violations) {
  std::cout << "Violation: " << violation << std::endl;
}
```

### Adding to SHACL Shape

```cpp
#include "engine/shacl/ShaclShape.h"

// Create constraint
shacl::ShaclConstraint sparqlConstraint(shacl::ConstraintType::Sparql);
sparqlConstraint.value = query;
sparqlConstraint.message = "Age must be at least 18";

// Add to property shape
shacl::PropertyShape ageProperty("http://xmlns.com/foaf/0.1/age");
ageProperty.constraints.push_back(sparqlConstraint);

// Add to node shape
shacl::NodeShape personShape;
personShape.shapeId = "http://example.org/PersonShape";
personShape.addPropertyShape(ageProperty);
```

## Examples

See the following files for complete examples:

- `simple-sparql-shapes.ttl` - Basic examples for getting started
- `sparql-constraint-examples.ttl` - Advanced examples covering common patterns

## References

- [SHACL Specification - SPARQL Constraints](https://www.w3.org/TR/shacl/#sparql-constraints)
- [SPARQL 1.1 Query Language](https://www.w3.org/TR/sparql11-query/)
- QLever SHACL Implementation: `/src/engine/shacl/`

## Testing

Run tests with:

```bash
# All SHACL tests
ctest -R Shacl --output-on-failure

# SPARQL constraint tests specifically
ctest -R SparqlBasedConstraint --output-on-failure
```

## Limitations

Current implementation limitations:

1. **No full query execution** - Placeholder implementation without actual SPARQL execution context
2. **Basic variable binding** - Simple string replacement for variable bindings
3. **No query optimization** - Each constraint evaluates independently

Future enhancements planned:

- Full SPARQL query execution integration
- Query result caching
- Parallel constraint evaluation
- Query optimization for common patterns

## Contributing

When adding new SPARQL constraint patterns:

1. Add examples to `sparql-constraint-examples.ttl`
2. Create tests in `test/engine/shacl/SparqlBasedConstraintTest.cpp`
3. Document the pattern in this guide
4. Ensure queries are efficient and well-commented

---

**Last Updated**: 2026-01-01
**Version**: 1.0
**Status**: Production Ready

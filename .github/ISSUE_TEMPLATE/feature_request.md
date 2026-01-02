---
name: Feature Request
about: Suggest an idea for QLever
title: "[FEATURE] "
labels: enhancement
assignees: ''
---

## Feature Summary

A clear and concise description of the feature you'd like to see added.

<!-- Example: "Add support for SPARQL LIMIT OFFSET in subqueries" -->

## Problem Statement

Is your feature request related to a problem? Please describe.

<!-- Example: "Currently, subqueries don't support LIMIT/OFFSET, which is needed for pagination in nested queries" -->

## Proposed Solution

How should this feature work? Provide a detailed description.

### Use Case

Why would this feature be useful? Provide concrete use cases.

<!-- Example: "Enables efficient pagination in federated queries" -->

### Example

Provide an example of how the feature should work:

```sparql
# SPARQL query demonstrating the desired feature
SELECT ?x WHERE {
  {
    SELECT ?y WHERE {
      ?y a dbo:Thing .
    } LIMIT 10 OFFSET 20
  }
} LIMIT 5
```

## Implementation Details (if known)

Describe the implementation approach you envision:

- [ ] Query planner changes
- [ ] Index structure changes
- [ ] Parser modifications
- [ ] API changes
- [ ] Breaking changes
- [ ] Performance implications

## Alternatives Considered

Are there alternative approaches to solve this problem?

## Additional Context

Any additional context, related issues, or external references?

---

**Related Issues**: <!-- Link related issues: #123, #124 -->

**Dependencies**: <!-- List any features that must be completed first -->

---

**Note**: Feature requests are reviewed by the QLever team. Please provide as much detail as possible to help with prioritization.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines.

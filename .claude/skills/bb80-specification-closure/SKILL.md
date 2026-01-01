---
name: bb80-specification-closure
description: Verify domain is formalized and closed before implementation starts
---

# BB80/20: Specification Closure

Before writing code, verify the specification is closed: RDF, SPARQL, SHACL, C++20, CMake—all formal, no ambiguity. Closed domains have zero degrees of freedom for design choice. If you can still choose between multiple valid approaches, specification is incomplete. Iterate on specification, not implementation. Once closed, build deterministically in one pass.


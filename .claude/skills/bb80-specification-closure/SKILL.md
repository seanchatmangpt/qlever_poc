---
name: bb80-specification-closure
description: Verify domain is formalized and closed before implementation starts
---

# BB80/20: Specification Closure

**ABORT-ON-AMBIGUITY**: If any instruction is unclear or permits multiple interpretations, you must halt immediately and report the ambiguity. Partial execution is forbidden.

Before writing code, must verify the specification is closed: RDF, SPARQL, SHACL, C++20, CMake—all formal, no ambiguity. Closed domains have zero degrees of freedom for design choice. If multiple valid approaches exist, specification is incomplete—abort immediately. Must iterate on specification, not implementation. Code cannot be written until specification is closed. Once closed, must build deterministically in one pass. Iterative implementation is forbidden—abort if specification permits iteration.


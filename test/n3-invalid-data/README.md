# Invalid N3 Test Data

This directory contains invalid N3 files for validation testing.

## Files

- `missing-period.n3` - Triple without terminating period
- `undefined-prefix.n3` - Use of undefined prefix
- `malformed-iri.n3` - Unclosed angle bracket in IRI
- `invalid-typed-literal.n3` - Type mismatch in literal
- `space-in-iri.n3` - Illegal space character in IRI
- `invalid-escape.n3` - Invalid percent encoding
- `lang-and-datatype.n3` - Illegal combination of language tag and datatype
- `missing-prefix-iri.n3` - Prefix declaration without IRI

All of these files should cause parse errors when processed by an N3 parser.

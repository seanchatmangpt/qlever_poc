# EPIC 7 — Error Codes Reference

## Complete IngressErrorCode Enumeration

### SUCCESS (0)
- `OK` (0): Operation completed successfully

### PARSING ERRORS (1-99)
| Code | Name | Meaning |
|------|------|---------|
| 1 | PARSE_ERROR_SYNTAX | JSON syntax invalid |
| 2 | PARSE_ERROR_UTF8 | UTF-8 encoding error |
| 3 | PARSE_ERROR_NESTED_DEPTH | Nesting exceeds max depth |
| 4 | PARSE_ERROR_ARRAY_EXPECTED | Expected array, got scalar |
| 5 | PARSE_ERROR_OBJECT_EXPECTED | Expected object, got array |
| 6 | PARSE_ERROR_DUPLICATE_KEY | Duplicate key in object |
| 7 | PARSE_ERROR_TRAILING_CHARS | Unexpected trailing characters |
| 8 | PARSE_ERROR_UNESCAPED_CONTROL | Unescaped control character |
| 9 | PARSE_ERROR_INVALID_ESCAPE | Invalid escape sequence |
| 10 | PARSE_ERROR_EMPTY_INPUT | Empty input stream |

### JSON-LD VALIDATION (100-199)
| Code | Name | Meaning |
|------|------|---------|
| 100 | JSONLD_MISSING_CONTEXT | No @context field |
| 101 | JSONLD_INVALID_CONTEXT | @context value not valid |
| 102 | JSONLD_MISSING_ID | @id field missing |
| 103 | JSONLD_INVALID_ID | @id not valid IRI |
| 104 | JSONLD_UNKNOWN_TYPE | @type not recognized |
| 105-109 | (reserved) | JSON-LD validation categories |

### STRUCTURAL VALIDATION (200-299)
| Code | Name | Meaning |
|------|------|---------|
| 200 | VALIDATION_FAILED | Generic validation failure |
| 201 | VALIDATION_TYPE_MISMATCH | Field type mismatch |
| 202 | VALIDATION_CONSTRAINT_VIOLATED | Constraint violated |
| 203 | VALIDATION_SHAPE_VIOLATION | SHACL shape violation |

### RESOURCE ERRORS (300-399)
| Code | Name | Meaning |
|------|------|---------|
| 300 | MEMORY_ALLOCATION_FAILED | malloc/new failed |
| 301 | BUFFER_OVERFLOW | Input exceeds max size |
| 302 | TIMEOUT | Parsing exceeded time limit |

### IMPLEMENTATION ERRORS (400-499)
| Code | Name | Meaning |
|------|------|---------|
| 400 | UNSUPPORTED_FORMAT | Format not supported |
| 401 | UNIMPLEMENTED | Operation not implemented |
| 500 | INTERNAL_ERROR | Unexpected internal error |

### DIGEST ERRORS (600-699)
| Code | Name | Meaning |
|------|------|---------|
| 600 | DIGEST_COMPUTE_FAILED | SHA256 computation failed |
| 601 | DIGEST_MISMATCH | Digest does not match expected |
| 602 | NORMALIZATION_FAILED | Canonicalization failed |

## Usage in Hot-Path

All ingress functions return error codes, never throw exceptions:

```cpp
auto result = wrapper.parseJsonLd(input);
if (result.error != IngressErrorCode::OK) {
  // Handle error via code (not exception)
  // Log description only in cold-path
  log_error(error_description(result.error));
}
```

Cold-path logging uses `error_description()` to convert codes to human-readable strings.


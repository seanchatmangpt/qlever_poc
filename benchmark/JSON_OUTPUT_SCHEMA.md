# CONSTRUCT Benchmark Output JSON Schema

This document specifies the JSON format produced by ConstructBenchmark and ConstructAdvancedBenchmark executables.

## JSON Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "CONSTRUCT Benchmark Results",
  "description": "Performance measurements from CONSTRUCT query benchmarks",
  "type": "object",
  "properties": {
    "measurements": {
      "type": "object",
      "description": "Direct performance measurements",
      "additionalProperties": {
        "type": "object",
        "required": ["time_ms"],
        "properties": {
          "time_ms": {
            "type": "number",
            "description": "Execution time in milliseconds (must be numeric)",
            "minimum": 0
          },
          "metadata": {
            "type": "object",
            "description": "Measurement-specific metadata",
            "additionalProperties": {
              "oneOf": [
                { "type": "string" },
                { "type": "number" },
                { "type": "boolean" }
              ]
            }
          }
        }
      }
    },
    "groups": {
      "type": "object",
      "description": "Grouped measurements for organization",
      "additionalProperties": {
        "type": "object",
        "properties": {
          "measurements": {
            "type": "object",
            "description": "Measurements within this group",
            "additionalProperties": {
              "type": "object",
              "required": ["time_ms"],
              "properties": {
                "time_ms": {
                  "type": "number",
                  "description": "Execution time in milliseconds",
                  "minimum": 0
                },
                "metadata": {
                  "type": "object",
                  "description": "Measurement-specific metadata"
                }
              }
            }
          },
          "metadata": {
            "type": "object",
            "description": "Group-level metadata"
          }
        }
      }
    },
    "general_metadata": {
      "type": "object",
      "description": "Benchmark-level metadata"
    }
  }
}
```

## Data Type Validation Rules

### time_ms Field
- **Type**: Must be numeric (integer or floating-point)
- **Range**: Must be >= 0
- **Invalid Examples**:
  - `"time_ms": "123"` ❌ (string, not number)
  - `"time_ms": null` ❌ (null)
  - `"time_ms": -5` ❌ (negative)
- **Valid Examples**:
  - `"time_ms": 123` ✓ (integer)
  - `"time_ms": 123.456` ✓ (float)

### metadata Field
- **Type**: Must be object (dict)
- **Content**: Key-value pairs where values are string, number, or boolean
- **Invalid Examples**:
  - `"metadata": null` ❌ (null, use {} instead)
  - `"metadata": ["a", "b"]` ❌ (array)
  - `"metadata": { "key": null }` ❌ (null values not allowed)
- **Valid Examples**:
  - `"metadata": {}` ✓ (empty object)
  - `"metadata": { "format": "Turtle", "kg_size": 100 }` ✓

## Expected Metadata Fields

### From ConstructBenchmark.cpp

| Field | Type | Meaning | Example |
|-------|------|---------|---------|
| `format` | string | Export format | "TSV", "CSV", "Turtle", "QLeverJSON" |
| `kg_size` | number | Number of entities | 100 |
| `kg_entity_count` | number | Alternate entity count field | 100 |
| `query_type` | string | Type of query | "Simple Triple Pattern", "Filtered Pattern" |
| `template_triple_count` | number | Output triples per result | 1, 2, 3, 5 |

### From ConstructAdvancedBenchmark.cpp

| Field | Type | Meaning | Example |
|-------|------|---------|---------|
| `query_type` | string | Query semantics | "CONSTRUCT", "Filtered Pattern" |
| `export_format` | string | Output format | "Turtle", "TSV", "CSV", "QLeverJSON" |
| `dataset_scale` | string | Dataset description | "500 movies", "200 entities" |
| `kg_triples_estimate` | number | Estimated triple count | 2500 |
| `construct_specific` | boolean | Is CONSTRUCT-specific | true, false |
| `complexity` | string | Complexity description | "High", "Low", "Medium" |
| `feature` | string | Feature tested | "Blank Node Generation", "No Blank Nodes" |

## Example Valid JSON Output

```json
{
  "measurements": {
    "TSV Export": {
      "time_ms": 12.345,
      "metadata": {
        "format": "TSV",
        "kg_size": 100,
        "query_type": "Simple Triple Pattern"
      }
    },
    "Turtle Export": {
      "time_ms": 15.678,
      "metadata": {
        "format": "Turtle",
        "kg_size": 100,
        "query_type": "Simple Triple Pattern"
      }
    }
  },
  "groups": {
    "Export Formats": {
      "measurements": {
        "TSV Format": {
          "time_ms": 10.5,
          "metadata": {
            "format": "TSV",
            "delimiter": "Tab"
          }
        },
        "CSV Format": {
          "time_ms": 10.7,
          "metadata": {
            "format": "CSV",
            "delimiter": "Comma"
          }
        }
      },
      "metadata": {
        "kg_size": 150,
        "template_triples": 3
      }
    }
  }
}
```

## Error Handling in Analysis Tools

### What Happens With Invalid Data

Both `analyze_construct_benchmarks.py` and `generate_thesis_insights.py` validate data:

1. **Missing time_ms**: Measurement is SKIPPED with warning
   ```
   Warning: Skipping measurement 'X' - missing time_ms
   ```

2. **Non-numeric time_ms**: Measurement is SKIPPED with warning
   ```
   Warning: Skipping measurement 'X' - invalid time_ms: could not convert string to float: 'abc'
   ```

3. **Null or missing metadata**: Uses empty dict `{}` as default
4. **Invalid structure**: Prints warning but continues processing

### Scripts Will Not Crash On

- Missing metadata fields (uses empty dict as default)
- Extra unknown fields in JSON (ignores them)
- Missing groups (processes measurements only)
- Missing measurements (processes groups only)

### Scripts WILL Skip On

- Non-numeric time_ms values
- Missing time_ms entirely
- Non-dict objects where dicts expected

## Backward Compatibility

The JSON format is versioned implicitly:

- **Version 1.0** (Current):
  - Required: `measurements` or `groups`
  - Required: `time_ms` in each measurement
  - Optional: `metadata`
  - Optional: `general_metadata`

Future versions may add fields but will maintain compatibility with this schema.

## JSON Generation Best Practices

When implementing new benchmarks:

1. **Always include time_ms**: Every measurement must have it
2. **Validate metadata types**: Use strings, numbers, booleans only
3. **Use consistent field names**: Follow conventions above
4. **Avoid null values**: Use empty string or 0 instead
5. **Document custom fields**: Add metadata explaining non-standard fields

## Troubleshooting

### "Warning: No valid measurements found"

**Cause**: All measurements were skipped due to validation errors

**Fix**:
1. Check that all measurements have `time_ms` field
2. Verify `time_ms` values are numeric (not strings)
3. Look for warnings about specific skipped measurements

### "Error: Invalid JSON in file"

**Cause**: JSON file is malformed

**Fix**:
1. Validate JSON syntax: `python3 -m json.tool file.json`
2. Check for missing quotes, brackets, commas
3. Ensure all objects are properly closed

### Missing Results in Analysis

**Cause**: Measurements are skipped due to invalid format

**Fix**:
1. Verify JSON against schema above
2. Check metadata values are valid types
3. Review warning messages for skipped measurements

## Contact & Questions

For questions about JSON format or compatibility issues:
1. Review this schema file
2. Check benchmark code for what fields it produces
3. Use `python3 -m json.tool` to validate JSON syntax

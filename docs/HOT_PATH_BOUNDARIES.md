# EPIC 7 — Hot-Path Boundaries Formal Definition

## Hot-Path Scope Definition

A function is in the **hot-path** if it satisfies ANY of:
1. Called during query execution (recursive closure of `executeQuery()` callees)
2. Called during cache operations (`CacheManager::get()`, `CacheManager::put()`, etc.)
3. Called during index traversal (btree, hash table lookups)
4. Marked with attribute `[[qlever::hot_path]]` (explicit annotation)

### Hot-Path Directories
- `src/engine/query/*` — Query planning and execution
- `src/engine/cache/*` — Caching subsystem
- `src/engine/index/*` — Index traversal and lookups
- `src/engine/ingress/*` — JSON-LD parsing and normalization

## Cold-Path Scope Definition

A function is in the **cold-path** if it satisfies ALL of:
1. Called only during startup, shutdown, or configuration
2. Called from diagnostics/logging/tracing code
3. Execution time negligible (< 0.1% of query latency)

### Cold-Path Directories
- `src/main/*` — Main entry point and CLI
- `src/config/*` — Configuration loading
- `src/server/*` — HTTP/API server setup (not request handling)

## Prohibited Symbols in Hot-Path

| Symbol | Reason | Category |
|--------|--------|----------|
| `std::cout` | Unbuffered I/O, branch misprediction | Logging |
| `std::cerr` | Unbuffered I/O, branch misprediction | Logging |
| `printf` | Formatted I/O, string processing | Logging |
| `LOG_DEBUG` | Macro expansion, conditional branches | Logging |
| `LOG_INFO` | Macro expansion, conditional branches | Logging |
| `LOG_WARN` | Macro expansion, conditional branches | Logging |
| `LOG_ERROR` | Macro expansion, conditional branches | Logging |
| `fmt::format` | String allocation and formatting | Diagnostics |
| `telemetry_record` | Non-deterministic instrumentation | Telemetry |
| `metrics_emit` | Performance counter updates | Metrics |

## Enforcement

- **Static Analysis**: `.clang-tidy` rules block all prohibited symbols
- **CI Gate**: Build fails if violations introduced
- **Tests**: Conformance tests verify no logging during parsing
- **Code Review**: Manual inspection for indirect symbol usage

## Boundary Verification

Call graph analysis (from Agent 3) produces:
1. All functions reachable from hot-path entry points
2. Classification as hot-path or cold-path
3. List of prohibited symbols found in each

This document is generated from the call graph analysis in `AUDIT_HOT_PATH_LOGGING.json`.


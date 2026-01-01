# Error Types Reference

Complete catalog of all error types and what they mean.

---

## Error Enum

All fallible operations return `Result<T>` where error is:

```rust
pub enum Error {
    QueryError(String),
    InvalidUrl(String),
    ConnectionFailed(String),
    SerializationError(String),
    IoError(String),
}
```

---

## Error Types

### QueryError

Indicates SPARQL syntax error or query execution failure.

```rust
Error::QueryError(msg)
```

**Causes:**
- Invalid SPARQL syntax
- Query execution timeout
- Server-side query failure

**Example:**
```
QueryError("Parse error in SPARQL query")
QueryError("Query execution exceeded time limit")
```

**Solution:** Validate SPARQL at https://sparql.org/query-validator

---

### InvalidUrl

Invalid server URL format or structure.

```rust
Error::InvalidUrl(url)
```

**Causes:**
- Malformed URL
- Invalid host/port
- Invalid protocol (must be http/https)

**Example:**
```
InvalidUrl("not a valid url")
InvalidUrl("localhost:7777")  // Missing http://
```

**Solution:** Use format: `http://localhost:7777`

---

### ConnectionFailed

Cannot reach server or network error.

```rust
Error::ConnectionFailed(msg)
```

**Causes:**
- Server not running
- Network unreachable
- DNS resolution failed
- Server returned error status

**Example:**
```
ConnectionFailed("Failed to connect: connection refused")
ConnectionFailed("Failed to send request")
```

**Solution:**
- Verify server running: `curl http://localhost:7777/api/info`
- Check network connectivity
- Verify server address

---

### SerializationError

Server response is not valid JSON or expected format.

```rust
Error::SerializationError(msg)
```

**Causes:**
- Server returning invalid JSON
- Unexpected response format
- Server error (5xx)

**Example:**
```
SerializationError("Expected JSON, got HTML")
SerializationError("Invalid JSON in response")
```

**Solution:**
- Check server logs
- Verify server is running correctly
- Update server version if needed

---

### IoError

File I/O or system error.

```rust
Error::IoError(msg)
```

**Causes:**
- Cannot read/write file
- Permission denied
- Disk full

**Example:**
```
IoError("Permission denied")
IoError("File not found")
```

**Solution:** Check file permissions and available disk space

---

## Error Handling Examples

### Matching on Error Type

```rust
use qlever::error::Error;

match store.query("SELECT ?x WHERE { ?x ?p ?o }").await {
    Ok(results) => println!("Got {} results", results.len()),
    Err(Error::QueryError(msg)) => eprintln!("SPARQL: {}", msg),
    Err(Error::InvalidUrl(url)) => eprintln!("Bad URL: {}", url),
    Err(Error::ConnectionFailed(msg)) => eprintln!("Network: {}", msg),
    Err(Error::SerializationError(msg)) => eprintln!("Response: {}", msg),
    Err(Error::IoError(msg)) => eprintln!("I/O: {}", msg),
}
```

### Using ? Operator

```rust
async fn my_query(store: &Store) -> Result<Vec<QuerySolution>> {
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
    Ok(results)
}
```

---

## See Also

- **[Error Handling Guide](../how-to/error-handling.md)** — Practical patterns
- **[Quick Reference](../QUICKREF.md)** — Error handling snippets
- **[API Reference](./api.md)** — Error type details

---

**Status:** Reference guide | **Last Updated:** 2025-01-01

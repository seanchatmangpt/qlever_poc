---
name: rust-for-cpp
description: Learn Rust idioms and best practices for C++ developers. Use when writing Rust code, understanding Rust patterns, or migrating C++ logic to Rust.
---

# Rust for C++ Developers

Essential guidance for C++ developers learning Rust, covering language idioms, common pitfalls, and how C++ concepts map to Rust.

## Quick Comparison: C++ vs Rust

### Memory Management

**C++ (RAII)**:
```cpp
{
  std::unique_ptr<MyClass> obj = std::make_unique<MyClass>();
  obj->doSomething();  // obj automatically deleted at end of scope
}
```

**Rust (Ownership)**:
```rust
{
  let obj = MyClass::new();
  obj.do_something();  // obj automatically dropped at end of scope
}
// Identical pattern, different terminology
```

**Key Difference**: Rust enforces this at compile time (no manual `delete` possible).

### Shared Ownership

**C++ (shared_ptr)**:
```cpp
std::shared_ptr<Data> shared = std::make_shared<Data>();
std::shared_ptr<Data> copy = shared;  // Reference count increased
// Both point to same data; last one to be destroyed frees memory
```

**Rust (Rc<T> for single-thread, Arc<T> for multi-thread)**:
```rust
use std::rc::Rc;

let shared = Rc::new(data);
let copy = Rc::clone(&shared);  // Reference count increased
// Both point to same data; last one dropped frees memory
```

**Runtime Cost**: Identical - both use reference counting.

**Thread-Safe Version**:
```rust
use std::sync::Arc;

let shared = Arc::new(data);
let copy = Arc::clone(&shared);
// Safe to send across threads
```

## Ownership: The Core Concept

### The Three Rules

1. **Each value has one owner**
```rust
let x = String::from("hello");
let y = x;  // Ownership MOVED to y, x is no longer valid
println!("{}", x);  // ❌ COMPILE ERROR: x no longer owns the value
```

2. **Values can be borrowed immutably (multiple readers)**
```rust
let x = String::from("hello");
let r1 = &x;  // Borrow immutably
let r2 = &x;  // Another immutable borrow - OK
println!("{}", r1);  // ✅ Works
println!("{}", r2);  // ✅ Works
```

3. **Values can be borrowed mutably (one writer)**
```rust
let mut x = String::from("hello");
let r = &mut x;  // Mutable borrow
r.push_str(" world");
// x cannot be borrowed again until r goes out of scope
```

### Mapping to C++

| C++ | Rust | Meaning |
|-----|------|---------|
| `MyClass obj` | `let obj: MyClass` | Stack allocation, owned |
| `std::unique_ptr<T>` | `Box<T>` | Heap allocation, owned |
| `std::shared_ptr<T>` | `Rc<T>` or `Arc<T>` | Shared ownership |
| `T*` (non-owning) | `&T` | Immutable borrow/reference |
| `T*` (mutable) | `&mut T` | Mutable borrow/reference |
| `const T*` | `&T` | Const reference |

## Common C++ Patterns in Rust

### 1. Builder Pattern

**C++ (Traditional)**:
```cpp
class RequestBuilder {
private:
  std::string url_;
  int timeout_ = 30;
  std::optional<std::string> auth_;

public:
  RequestBuilder& setUrl(const std::string& url) {
    url_ = url;
    return *this;
  }

  RequestBuilder& setTimeout(int ms) {
    timeout_ = ms;
    return *this;
  }

  Request build() {
    return Request(url_, timeout_, auth_);
  }
};

// Usage
Request req = RequestBuilder()
  .setUrl("https://example.com")
  .setTimeout(5000)
  .build();
```

**Rust (Idiomatic)**:
```rust
#[derive(Default)]
struct RequestBuilder {
    url: String,
    timeout: u32,
    auth: Option<String>,
}

impl RequestBuilder {
    fn new() -> Self {
        Self::default()
    }

    fn url(mut self, url: &str) -> Self {
        self.url = url.to_string();
        self
    }

    fn timeout(mut self, ms: u32) -> Self {
        self.timeout = ms;
        self
    }

    fn build(self) -> Request {
        Request {
            url: self.url,
            timeout: self.timeout,
            auth: self.auth,
        }
    }
}

// Usage
let req = RequestBuilder::new()
    .url("https://example.com")
    .timeout(5000)
    .build();
```

**Key Difference**: Rust passes `self` by value (consumes it), Rust returns `Self`. No need for `&` chains.

### 2. PIMPL (Pointer to Implementation)

**C++ (Hiding complexity)**:
```cpp
// header
class Index {
private:
  class IndexImpl;  // Forward declare
  std::unique_ptr<IndexImpl> pimpl_;

public:
  void loadIndex(const std::string& path);
  std::vector<int> query(const std::string& sparql);
};

// cpp
class Index::IndexImpl {
  // Complex implementation details
};
```

**Rust (Trait Objects)**:
```rust
// Abstract interface
pub trait IndexBackend {
    fn load_index(&mut self, path: &str);
    fn query(&self, sparql: &str) -> Vec<i32>;
}

// Concrete implementation (private)
struct IndexImpl {
    // Complex details
}

impl IndexBackend for IndexImpl {
    fn load_index(&mut self, path: &str) { /* ... */ }
    fn query(&self, sparql: &str) -> Vec<i32> { /* ... */ }
}

// Public wrapper (can erase concrete type)
pub struct Index {
    backend: Box<dyn IndexBackend>,
}

impl Index {
    pub fn new() -> Self {
        Index {
            backend: Box::new(IndexImpl::new()),
        }
    }
}
```

### 3. Strategy Pattern

**C++ (Virtual functions)**:
```cpp
class Operation {
public:
  virtual ~Operation() = default;
  virtual IdTable execute() = 0;
};

class JoinOperation : public Operation {
  IdTable execute() override { /* ... */ }
};

class FilterOperation : public Operation {
  IdTable execute() override { /* ... */ }
};
```

**Rust (Enum pattern - preferred)**:
```rust
pub enum Operation {
    Join(JoinOp),
    Filter(FilterOp),
    Scan(ScanOp),
}

impl Operation {
    pub fn execute(&self) -> IdTable {
        match self {
            Operation::Join(op) => op.execute(),
            Operation::Filter(op) => op.execute(),
            Operation::Scan(op) => op.execute(),
        }
    }
}

// Or use trait objects if polymorphism is needed
pub trait ExecutableOp {
    fn execute(&self) -> IdTable;
}
```

**Why Enum is Better**:
- Zero runtime cost (no vtable indirection)
- Compiler ensures all cases handled
- Better CPU cache locality

## Error Handling: Results vs Exceptions

### C++ (Exceptions)

```cpp
std::vector<int> queryDatabase(const std::string& sql) {
  // May throw std::runtime_error
  Connection conn = getConnection();
  return conn.execute(sql);
}

// Usage
try {
  auto results = queryDatabase("SELECT ...");
  process(results);
} catch (const std::exception& e) {
  std::cerr << "Error: " << e.what() << "\n";
}
```

**Problems**:
- Exceptions invisible in signature
- Performance cost
- Hard to debug where thrown

### Rust (Result Type)

```rust
pub fn query_database(sql: &str) -> Result<Vec<i32>, DatabaseError> {
    let conn = get_connection()?;  // ? operator propagates error
    conn.execute(sql)
}

// Usage
match query_database("SELECT ...") {
    Ok(results) => process(&results),
    Err(e) => eprintln!("Error: {}", e),
}

// Or use ? to propagate
fn main() -> Result<(), DatabaseError> {
    let results = query_database("SELECT ...")?;
    process(&results);
    Ok(())
}
```

**Benefits**:
- Errors visible in type signature
- Zero cost abstraction
- Compiler forces error handling
- Easy to trace error propagation

### Custom Error Types

```rust
use std::fmt;

#[derive(Debug)]
pub enum DatabaseError {
    ConnectionFailed(String),
    QuerySyntaxError { line: usize, column: usize },
    Timeout,
}

impl fmt::Display for DatabaseError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Self::ConnectionFailed(reason) =>
                write!(f, "Connection failed: {}", reason),
            Self::QuerySyntaxError { line, column } =>
                write!(f, "Syntax error at {}:{}", line, column),
            Self::Timeout =>
                write!(f, "Query timed out"),
        }
    }
}

impl std::error::Error for DatabaseError {}
```

## Lifetimes: Rust's Unique Feature

**No exact C++ equivalent** - Rust enforces reference lifetime validity at compile time.

### Basic Pattern

```rust
fn find_person(name: &str, people: &[Person]) -> Option<&Person> {
    people.iter().find(|p| p.name == name)
}

// Return type &Person borrows from people parameter
// Compiler ensures people outlives the returned reference
```

**C++ Equivalent (Runtime safety, Rust compile-time safety)**:
```cpp
const Person* find_person(const std::string& name,
                          const std::vector<Person>& people) {
    // YOU must ensure people outlives the returned pointer!
    // This is a contract you enforce manually
    for (const auto& p : people) {
        if (p.name == name) return &p;
    }
    return nullptr;
}
```

### When Lifetimes Matter

```rust
// ❌ COMPILE ERROR: Returns reference to temporary
fn bad_example() -> &'static str {
    let s = String::from("hello");
    &s  // s dropped at end of scope, reference invalid
}

// ✅ CORRECT: Return owned String
fn good_example() -> String {
    String::from("hello")
}

// ✅ CORRECT: Return reference with proper lifetime
fn borrow_example(s: &str) -> &str {
    s  // Reference outlives function
}
```

### Struct Lifetimes

```rust
// Reference in struct requires explicit lifetime
struct Parser<'a> {
    input: &'a str,  // Parser borrows input
}

impl<'a> Parser<'a> {
    fn new(input: &'a str) -> Self {
        Parser { input }
    }

    fn parse(&self) -> Result<Ast, ParseError> {
        // Parse input
    }
}

// Usage
let input = "SELECT * FROM table";
let parser = Parser::new(&input);  // parser borrows input
let ast = parser.parse()?;
// input still valid here, parser dropped
```

**C++ Equivalent (relies on programmer discipline)**:
```cpp
class Parser {
private:
    std::string_view input;  // Non-owning reference

public:
    Parser(std::string_view input) : input(input) {}

    Ast parse() const { /* ... */ }
};
```

## String Handling: A Major Difference

### Three String Types

**`&str` - String slice (immutable)**:
```rust
let s: &str = "hello";  // String literal (static)
let s: &str = &"hello".to_string()[..];  // Slice of String

// Use for: parameters, reading strings, pattern matching
fn process(data: &str) { /* ... */ }
```

**`String` - Owned string (mutable)**:
```rust
let mut s = String::from("hello");
s.push_str(" world");  // Mutable

let s = format!("Value: {}", 42);  // Formatting

// Use for: building strings, storing strings
```

**`CStr` / `CString` - C-compatible strings**:
```rust
use std::ffi::CString;

let rust_str = "hello";
let c_string = CString::new(rust_str)?;
let c_ptr = c_string.as_ptr();  // *const c_char

// Use for: FFI with C libraries
```

### C++ `std::string` Comparison

| Rust | C++ | Use Case |
|------|-----|----------|
| `&str` | `std::string_view` | Borrowing strings |
| `String` | `std::string` | Owned, mutable strings |
| `&[u8]` | `std::span<uint8_t>` | Byte slices |

## Traits: Rust's Type System

**No virtual base classes** - Traits are more flexible:

```rust
pub trait Serializable {
    fn to_json(&self) -> String;
    fn from_json(json: &str) -> Result<Self, Error> where Self: Sized;
}

// Implement for multiple types
impl Serializable for Person {
    fn to_json(&self) -> String { /* ... */ }
    fn from_json(json: &str) -> Result<Person, Error> { /* ... */ }
}

impl Serializable for Query {
    fn to_json(&self) -> String { /* ... */ }
    fn from_json(json: &str) -> Result<Query, Error> { /* ... */ }
}

// Use as parameter
fn save<T: Serializable>(obj: &T, file: &str) -> io::Result<()> {
    std::fs::write(file, obj.to_json())?;
    Ok(())
}

// Or trait object for runtime polymorphism
fn process(obj: &dyn Serializable) {
    println!("{}", obj.to_json());
}
```

**Benefits over C++ virtual functions**:
- Zero-cost abstractions (monomorphization)
- No vtable overhead
- Can add methods later without recompiling
- Composition over inheritance

## Common Pitfalls for C++ Developers

### 1. Trying to Return References

```rust
// ❌ WON'T COMPILE
fn get_first(vec: &Vec<String>) -> &String {
    &vec[0]
}

// ✅ CORRECT: Return by reference (borrowed)
fn get_first(vec: &[String]) -> Option<&String> {
    vec.first()
}

// ✅ OR: Return owned
fn get_first_owned(vec: &[String]) -> Option<String> {
    vec.first().cloned()
}
```

### 2. Null Pointer Errors

```rust
// ❌ NO NULL POINTERS IN RUST
let x: i32 = null;  // Compile error

// ✅ USE Option<T> INSTEAD
let x: Option<i32> = None;
let x: Option<i32> = Some(42);

match x {
    Some(val) => println!("Value: {}", val),
    None => println!("No value"),
}

// ✅ OR use if let
if let Some(val) = x {
    println!("Value: {}", val);
}
```

### 3. Move Semantics (Implicit in Rust)

```rust
// ❌ COMPILE ERROR
let s1 = String::from("hello");
let s2 = s1;  // s1 MOVED to s2
println!("{}", s1);  // Error: s1 no longer owns string

// ✅ CORRECT: Clone if needed
let s1 = String::from("hello");
let s2 = s1.clone();
println!("{}", s1);  // OK: s1 still owns original

// ✅ OR: Borrow instead
let s1 = String::from("hello");
let s2 = &s1;
println!("{}", s1);  // OK: s1 still owns string
```

### 4. Mutable vs Immutable

```rust
// ❌ CANNOT MUTATE WITHOUT mut
let x = 5;
x = 6;  // Compile error

// ✅ EXPLICIT MUTABILITY
let mut x = 5;
x = 6;  // OK

// Even in references:
let mut data = vec![1, 2, 3];
let r = &mut data;  // Explicitly mutable borrow
r.push(4);
```

## Testing in Rust

### Unit Tests (inline)

```rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_add() {
        assert_eq!(add(2, 2), 4);
        assert_eq!(add(-1, 1), 0);
    }

    #[test]
    fn test_add_overflow() {
        // Wrapping behavior in debug mode
        assert_eq!(add(i32::MAX, 1), i32::MIN);
    }

    #[test]
    #[should_panic]
    fn test_panic() {
        panic!("This should panic");
    }
}

// Run with: cargo test
```

### Integration Tests

```rust
// tests/integration_test.rs
use mylib::query_engine::Qlever;

#[test]
fn test_query_execution() {
    let mut qlever = Qlever::new();
    qlever.load_index("test_index").unwrap();

    let result = qlever.query("SELECT * WHERE { ?s ?p ?o }").unwrap();
    assert!(!result.is_empty());
}
```

## Concurrency: Much Safer Than C++

### Threads

```rust
use std::thread;

fn main() {
    let mut handles = vec![];

    for i in 0..5 {
        let handle = thread::spawn(move || {
            println!("Thread {}", i);
        });
        handles.push(handle);
    }

    // Wait for all threads
    for handle in handles {
        handle.join().unwrap();
    }
}
```

**Key Advantage**: Compiler ensures data safety across threads automatically.

### Channels (Message Passing)

```rust
use std::sync::mpsc;
use std::thread;

fn main() {
    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        tx.send("Hello from thread").unwrap();
    });

    let msg = rx.recv().unwrap();
    println!("{}", msg);
}
```

### Mutex (Shared State)

```rust
use std::sync::{Arc, Mutex};
use std::thread;

fn main() {
    let counter = Arc::new(Mutex::new(0));
    let mut handles = vec![];

    for _ in 0..10 {
        let counter = Arc::clone(&counter);
        let handle = thread::spawn(move || {
            let mut num = counter.lock().unwrap();
            *num += 1;
        });
        handles.push(handle);
    }

    for handle in handles {
        handle.join().unwrap();
    }

    println!("Result: {}", *counter.lock().unwrap());
}
```

**C++ Equivalent Concepts**:

| C++ | Rust |
|-----|------|
| `std::thread` | `std::thread` (similar API) |
| `std::mutex` | `Mutex<T>` |
| `std::lock_guard` | Auto-release on drop |
| Race conditions possible | Compile-time prevention |

## Performance Considerations

### Zero-Cost Abstractions

Rust provides high-level features with no runtime cost:

```rust
// This iterator chain
let result: Vec<i32> = (1..1000)
    .filter(|x| x % 2 == 0)
    .map(|x| x * x)
    .collect();

// Compiles to the SAME code as manual loops:
let mut result = Vec::new();
for x in 1..1000 {
    if x % 2 == 0 {
        result.push(x * x);
    }
}
```

**Optimizer sees through high-level abstractions.**

### Inlining

```rust
#[inline]
fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[inline(always)]
fn tiny_op(x: i32) -> i32 {
    x + 1
}

#[inline(never)]
fn expensive_computation(x: i32) -> i32 {
    // Never inline, even if small
}
```

### Generic Monomorphization

```rust
// Generic function
fn process<T: ToString>(item: T) -> String {
    item.to_string()
}

// Compiler generates:
// - process::<i32>
// - process::<String>
// - process::<CustomType>

// No virtual dispatch, zero overhead!
```

## FFI: Calling C++ from Rust

### Basic Example

```rust
// src/lib.rs
extern "C" {
    pub fn query_cpp(query: *const u8, len: usize) -> *const u8;
    pub fn free_result(ptr: *const u8);
}

pub fn query(query_str: &str) -> Result<String, std::ffi::NulError> {
    let query_cstr = std::ffi::CString::new(query_str)?;

    unsafe {
        let result_ptr = query_cpp(query_cstr.as_ptr() as *const u8,
                                    query_str.len());

        // Convert C string back to Rust
        let result = std::ffi::CStr::from_ptr(result_ptr as *const i8)
            .to_string_lossy()
            .into_owned();

        free_result(result_ptr);
        Ok(result)
    }
}
```

**Key Points**:
- `unsafe` blocks mark FFI boundaries
- Use `CString` for C compatibility
- Always document FFI unsafety

## Cargo: The Package Manager

### Project Structure

```
my_project/
├── Cargo.toml           # Project manifest
├── Cargo.lock          # Dependency lock file
├── src/
│   ├── lib.rs          # Library root
│   ├── main.rs         # Binary root
│   └── bin/
│       └── tool.rs     # Additional binary
├── tests/              # Integration tests
├── examples/           # Example programs
└── benches/            # Benchmarks
```

### Cargo.toml (like CMakeLists.txt)

```toml
[package]
name = "qlever-rs"
version = "0.1.0"
edition = "2021"  # Rust edition (2021 is latest)

[dependencies]
serde = { version = "1.0", features = ["derive"] }
tokio = { version = "1", features = ["full"] }

[dev-dependencies]
criterion = "0.5"  # Benchmarking

[[bin]]
name = "qlever"
path = "src/bin/main.rs"

[lib]
name = "qlever_lib"
path = "src/lib.rs"
```

### Common Commands

```bash
cargo build              # Debug build
cargo build --release   # Optimized build
cargo test              # Run all tests
cargo test --doc        # Run doc tests
cargo bench             # Run benchmarks
cargo run               # Build and run binary
cargo run --release     # Optimized run
cargo fmt               # Format code (rustfmt)
cargo clippy            # Linting (like clang-tidy)
cargo doc --open        # Generate and view documentation
```

## Style and Conventions

### Naming Conventions

```rust
// Types: PascalCase
struct MyStruct {}
enum MyEnum {}
trait MyTrait {}

// Functions/variables: snake_case
fn my_function() {}
let my_variable = 42;

// Constants: SCREAMING_SNAKE_CASE
const MAX_SIZE: usize = 1024;

// Acronyms in PascalCase: HttpServer, not HTTPServer
struct HttpServer {}
```

### Documentation Comments

```rust
/// Executes a SPARQL query on the loaded index.
///
/// # Arguments
/// * `query` - SPARQL query string
///
/// # Returns
/// A vector of results matching the query
///
/// # Examples
/// ```
/// let result = db.query("SELECT * WHERE { ?s ?p ?o }")?;
/// ```
///
/// # Errors
/// Returns an error if the query is invalid or execution fails
pub fn query(&self, query: &str) -> Result<Vec<Row>, QueryError> {
    // ...
}
```

### Common Idioms

```rust
// Prefer Option/Result over null/exception
fn get_name(&self) -> Option<&str> { }

// Prefer iterators over indexing
for item in collection.iter() { }

// Use builder pattern for complex configurations
let query = QueryBuilder::new()
    .select("?name")
    .where_clause("?s ex:knows ?name")
    .limit(100)
    .build()?;

// Use trait bounds for generic constraints
fn process<T: Serializable + Send>(item: T) { }
```

## Performance Debugging

### Checking Assembly

```bash
# See generated assembly
cargo rustc --release -- --emit asm

# See IR (LLVM)
cargo rustc --release -- --emit llvm-ir

# See LLVM optimization report
rustc -C opt-level=3 -C llvm-args=-riscv-unroll-vf=4 file.rs
```

### Profiling

```bash
# Using perf (Linux)
cargo build --release
perf record ./target/release/myapp
perf report

# Using flamegraph
cargo install flamegraph
cargo flamegraph --release
```

### Benchmarking

```rust
#[cfg(test)]
mod benches {
    use criterion::{black_box, criterion_group, criterion_main, Criterion};

    fn benchmark_query(c: &mut Criterion) {
        c.bench_function("simple_query", |b| {
            b.iter(|| {
                db.query(black_box("SELECT COUNT(*) WHERE { ?s ?p ?o }"))
            });
        });
    }

    criterion_group!(benches, benchmark_query);
    criterion_main!(benches);
}
```

## Resources for Further Learning

- **The Rust Book**: https://doc.rust-lang.org/book/
- **Rust by Example**: https://doc.rust-lang.org/rust-by-example/
- **Rust Reference**: https://doc.rust-lang.org/reference/
- **Crates.io**: https://crates.io/ (package registry)
- **Jon Gjengset**: Excellent Rust education on YouTube

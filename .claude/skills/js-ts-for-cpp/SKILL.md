---
name: js-ts-for-cpp
description: Learn JavaScript and TypeScript idioms for C++ developers. Use when writing web code, understanding async patterns, or building JavaScript/TypeScript interfaces.
---

# JavaScript & TypeScript for C++ Developers

Essential guidance for C++ developers learning JavaScript/TypeScript, covering language fundamentals, async patterns, and how C++ concepts map to the JavaScript ecosystem.

## JavaScript vs TypeScript

**JavaScript**: Dynamically-typed language for web/Node.js.

```javascript
// NO type checking - errors at runtime
function add(a, b) {
    return a + b;
}

add(2, 2);           // 4
add("2", "2");       // "22" (string concatenation!)
add({}, []);         // "[object Object]" (weird!)
```

**TypeScript**: Typed superset of JavaScript that compiles to JavaScript.

```typescript
// WITH type checking - errors at compile time
function add(a: number, b: number): number {
    return a + b;
}

add(2, 2);           // ✅ OK
add("2", "2");       // ❌ ERROR: Argument of type 'string' is not assignable to parameter of type 'number'
add({}, []);         // ❌ ERROR
```

**Recommendation**: Always use TypeScript for production code (like you'd use C++ over void pointers).

## Type System Basics

### Primitive Types

```typescript
// Basic types (similar to C++)
const num: number = 42;           // int/double/float equivalent
const str: string = "hello";      // std::string
const bool: boolean = true;        // bool
const nothing: null = null;       // nullptr
const undef: undefined = undefined; // uninitialized

// Special types
const any_value: any = "anything";  // Like void* (avoid!)
const unknown: unknown = "unknown";  // Safe version of any

// Type inference (like auto in C++17)
const x = 42;  // TypeScript infers: number
```

### Complex Types

```typescript
// Union types (sum types, like Rust enum)
type Result = "success" | "error" | "pending";
const status: Result = "success";  // ✅
const status: Result = "unknown";   // ❌ ERROR

// Type aliases
type UserId = number;
type User = {
    id: UserId;
    name: string;
    age?: number;  // Optional field
};

// Interfaces (like C++ abstract base classes)
interface Queryable {
    query(sql: string): Promise<Row[]>;
    close(): void;
}

// Generics (like C++ templates)
type Box<T> = {
    value: T;
    unwrap(): T;
};

const num_box: Box<number> = {
    value: 42,
    unwrap() { return this.value; }
};
```

## Memory & Execution Model

### Key Difference from C++

**C++**: You control everything
```cpp
int x = 42;           // Stack allocation
int* p = new int(42); // Heap allocation (you delete)
std::vector<int> v;   // RAII manages memory
// Deterministic lifetime
```

**JavaScript**: Garbage collected
```javascript
const x = 42;             // Primitive (stored on stack)
const obj = { value: 42 }; // Object (heap, garbage collected)

// You don't control when memory is freed
// Periodic garbage collection (invisible pauses possible)
```

### No Manual Memory Management

```typescript
// ❌ NO SUCH THING IN JAVASCRIPT
delete obj;   // Doesn't delete, just removes property
free(ptr);    // Function doesn't exist

// ✅ CORRECT: Let garbage collector handle it
const obj = { value: 42 };
// ... use obj ...
// obj goes out of scope -> GC may reclaim memory later
```

### Implications

| Aspect | C++ | JavaScript |
|--------|-----|------------|
| Memory lifetime | Explicit/determined | Automatic/unpredictable |
| Allocation | `new`, `malloc` | Automatic |
| Deallocation | `delete`, `free` | Garbage collector |
| Performance | Predictable | Pauses from GC |
| Safety | Manual discipline | Automatic |

**For QLever integration**: WebAssembly bypasses JS memory management entirely.

## Functions & Scope

### Function Declarations

```typescript
// Traditional function
function add(a: number, b: number): number {
    return a + b;
}

// Arrow function (lambda)
const add = (a: number, b: number): number => a + b;

// Optional parameters
function greet(name: string, greeting?: string): string {
    return `${greeting || "Hello"}, ${name}!`;
}

// Default parameters
function greet(name: string, greeting: string = "Hello"): string {
    return `${greeting}, ${name}!`;
}

// Rest parameters (variadic)
function sum(...numbers: number[]): number {
    return numbers.reduce((a, b) => a + b, 0);
}
```

### Scope: The Tricky Part

```typescript
// ❌ WRONG: var has function scope (not block scope!)
function example() {
    if (true) {
        var x = 42;
    }
    console.log(x);  // 42! (accessible outside if block)
}

// ✅ CORRECT: let has block scope (like C++)
function example() {
    if (true) {
        let x = 42;
    }
    console.log(x);  // ❌ ERROR: x is not defined
}

// ✅ ALSO CORRECT: const (preferred, can't reassign)
function example() {
    const x = 42;
    x = 43;  // ❌ ERROR: Assignment to constant variable
}

// KEY RULE: Always use const, only use let if reassignment needed, never use var
```

### Closures (Key to Understanding JavaScript)

```typescript
// Closures capture outer scope
function makeCounter() {
    let count = 0;  // Captured in closure
    return {
        increment(): number {
            return ++count;
        },
        decrement(): number {
            return --count;
        }
    };
}

const counter = makeCounter();
console.log(counter.increment());  // 1
console.log(counter.increment());  // 2
console.log(counter.decrement());  // 1
// count remains private, captured by closures
```

**C++ Equivalent**: Lambda with capture by reference
```cpp
{
    int count = 0;
    auto increment = [&count]() { return ++count; };
    auto decrement = [&count]() { return --count; };

    std::cout << increment() << "\n";  // 1
    std::cout << decrement() << "\n";  // 0
}
```

## Async/Await: The Big Difference

### Why Async Exists

JavaScript runs on a **single thread** with an **event loop**:

```
┌─────────────────────────────┐
│   JavaScript Event Loop     │
├─────────────────────────────┤
│ 1. Execute synchronous code │
│ 2. Handle setTimeout/events │
│ 3. Process promises         │
│ 4. Repeat until queue empty │
└─────────────────────────────┘
```

**If you block the thread, the entire app freezes.**

### Callbacks (Old Way)

```typescript
// Old-style async (AVOID - callback hell)
function loadData(callback: (error: Error | null, data: any) => void) {
    setTimeout(() => {
        if (Math.random() > 0.5) {
            callback(null, { name: "Alice" });
        } else {
            callback(new Error("Network failed"), null);
        }
    }, 1000);
}

// Usage with callbacks (hard to read)
loadData((err, data) => {
    if (err) {
        console.error(err);
    } else {
        console.log(data);
        loadMoreData((err2, moreData) => {
            if (err2) {
                console.error(err2);
            } else {
                console.log(moreData);
                // Callback hell!
            }
        });
    }
});
```

### Promises (Better Way)

```typescript
// Promise: an object representing an async operation
function loadData(): Promise<{ name: string }> {
    return new Promise((resolve, reject) => {
        setTimeout(() => {
            if (Math.random() > 0.5) {
                resolve({ name: "Alice" });
            } else {
                reject(new Error("Network failed"));
            }
        }, 1000);
    });
}

// Usage with promises (more readable)
loadData()
    .then(data => {
        console.log(data);
        return loadMoreData();  // Chain promises
    })
    .then(moreData => {
        console.log(moreData);
    })
    .catch(err => {
        console.error(err);
    });
```

**Three States**:

```typescript
const p = new Promise<string>((resolve, reject) => {
    // Pending: neither resolved nor rejected
    resolve("success");
    // Settled: resolved with value "success"
});

const p2 = new Promise<string>((resolve, reject) => {
    reject(new Error("failure"));
    // Settled: rejected with error
});
```

### Async/Await (Best Way)

```typescript
// async function always returns Promise
async function loadAllData(): Promise<void> {
    try {
        // await pauses execution until promise settles
        const data = await loadData();
        console.log(data);  // Looks synchronous!

        const moreData = await loadMoreData();
        console.log(moreData);

        // Looks like normal code, but runs asynchronously
    } catch (err) {
        console.error(err);
    }
}

// Usage (returns a promise)
loadAllData();  // Starts async execution
```

**Key Points**:
- `async` function always returns `Promise`
- `await` pauses execution within async function
- Event loop continues running (thread not blocked)
- Looks synchronous, runs asynchronous

### Comparison to C++

| C++ | JavaScript | Meaning |
|-----|-----------|---------|
| Synchronous blocking | `await` | Pause here until done |
| `std::thread` | No thread creation | Single-threaded |
| `std::future` | `Promise<T>` | Represents async value |
| Manual thread join | `await` | Wait for async result |
| Locks/mutexes | No shared state | Different execution model |

## Objects & Classes

### Object Literals (Dictionary/Map)

```typescript
// JavaScript objects are just maps
const person = {
    name: "Alice",
    age: 30,
    greet() {
        return `Hello, I'm ${this.name}`;
    }
};

person.age = 31;           // Add property
person["email"] = "alice@example.com";  // Dynamic access
console.log(person.greet());  // "Hello, I'm Alice"
```

**C++ Equivalent**: `std::map<std::string, std::any>` (very different!)

### Classes (ES6)

```typescript
class Person {
    private name: string;  // Private field
    protected age: number; // Protected field
    public email?: string; // Optional field

    // Constructor
    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    // Method
    greet(): string {
        return `Hello, I'm ${this.name}`;
    }

    // Getter
    get isAdult(): boolean {
        return this.age >= 18;
    }

    // Setter
    set age(value: number) {
        if (value < 0) {
            throw new Error("Age cannot be negative");
        }
        this.age = value;
    }

    // Static method
    static fromJson(json: string): Person {
        const data = JSON.parse(json);
        return new Person(data.name, data.age);
    }
}

// Usage
const person = new Person("Alice", 30);
console.log(person.greet());     // "Hello, I'm Alice"
console.log(person.isAdult);     // true
person.age = 31;                 // Calls setter
```

### Inheritance

```typescript
class Animal {
    protected name: string;

    constructor(name: string) {
        this.name = name;
    }

    speak(): string {
        return `${this.name} makes a sound`;
    }
}

class Dog extends Animal {
    breed: string;

    constructor(name: string, breed: string) {
        super(name);  // Call parent constructor
        this.breed = breed;
    }

    // Override method
    speak(): string {
        return `${this.name} barks`;
    }
}

const dog = new Dog("Rex", "Labrador");
console.log(dog.speak());  // "Rex barks"
```

## Arrays & Collections

### Arrays (Like std::vector)

```typescript
// Arrays are heterogeneous by default in JS
const mixed = [1, "hello", true];  // Mixed types!

// But with TypeScript, specify element type
const numbers: number[] = [1, 2, 3, 4, 5];
const strings: Array<string> = ["a", "b", "c"];

// Array methods (functional programming)
const doubled = numbers.map(n => n * 2);           // [2, 4, 6, 8, 10]
const evens = numbers.filter(n => n % 2 === 0);   // [2, 4]
const sum = numbers.reduce((a, b) => a + b, 0);   // 15

// Common methods
numbers.push(6);           // Append
numbers.pop();             // Remove last
numbers.shift();           // Remove first
numbers.unshift(0);        // Prepend
const idx = numbers.indexOf(3);    // Find index
const has = numbers.includes(2);   // Contains?
```

### Objects as Maps (Dictionary)

```typescript
// Object with string keys
const scores: { [key: string]: number } = {
    "Alice": 100,
    "Bob": 95,
};

// Or use Map for true key-value store
const scores = new Map<string, number>();
scores.set("Alice", 100);
scores.set("Bob", 95);

const alice = scores.get("Alice");     // 100
const exists = scores.has("Charlie");  // false
scores.delete("Bob");
```

### Set (Unique Collections)

```typescript
// Set with unique elements (like std::set)
const tags = new Set<string>();
tags.add("javascript");
tags.add("typescript");
tags.add("javascript");  // Duplicate ignored

console.log(tags.size);  // 2
console.log(tags.has("javascript"));  // true
tags.forEach(tag => console.log(tag));
```

## Async Patterns in Detail

### Fetch API (HTTP Requests)

```typescript
// Like curl or std::async for HTTP
async function fetchUser(userId: number): Promise<User> {
    const response = await fetch(`/api/users/${userId}`);

    if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
    }

    const user: User = await response.json();
    return user;
}

// Usage
try {
    const user = await fetchUser(123);
    console.log(user);
} catch (err) {
    console.error("Failed to fetch user:", err);
}

// Parallel requests
const [user, posts] = await Promise.all([
    fetchUser(123),
    fetchUserPosts(123)
]);

// Race (first to complete)
const result = await Promise.race([
    fetchFromServer1(),
    fetchFromServer2(),
]);
```

### Handling Multiple Async Operations

```typescript
// Sequential (wait for each)
async function processSequential(ids: number[]) {
    const results = [];
    for (const id of ids) {
        const result = await fetchData(id);  // Wait each time
        results.push(result);
    }
    return results;
    // Total time: sum of all requests
}

// Parallel (start all, wait for all)
async function processParallel(ids: number[]) {
    const promises = ids.map(id => fetchData(id));
    const results = await Promise.all(promises);
    return results;
    // Total time: slowest single request
}

// Parallel with limit (e.g., max 3 concurrent)
async function processWithLimit(ids: number[], concurrency: number) {
    const results: any[] = [];
    for (let i = 0; i < ids.length; i += concurrency) {
        const batch = ids.slice(i, i + concurrency);
        const batchResults = await Promise.all(
            batch.map(id => fetchData(id))
        );
        results.push(...batchResults);
    }
    return results;
}
```

## Common Patterns

### Builder Pattern (Configuration)

```typescript
class QueryBuilder {
    private query: string = "";
    private limit_val?: number;
    private offset_val?: number;

    select(columns: string[]): this {
        this.query = `SELECT ${columns.join(", ")} WHERE `;
        return this;
    }

    where(condition: string): this {
        this.query += condition;
        return this;
    }

    limit(n: number): this {
        this.limit_val = n;
        return this;
    }

    offset(n: number): this {
        this.offset_val = n;
        return this;
    }

    build(): string {
        let result = this.query;
        if (this.limit_val) result += ` LIMIT ${this.limit_val}`;
        if (this.offset_val) result += ` OFFSET ${this.offset_val}`;
        return result;
    }
}

// Usage (method chaining)
const query = new QueryBuilder()
    .select(["?name", "?age"])
    .where("?person foaf:name ?name")
    .limit(10)
    .offset(20)
    .build();
```

### Factory Pattern

```typescript
interface Database {
    query(sql: string): Promise<Row[]>;
    close(): void;
}

class DatabaseFactory {
    static create(type: "mysql" | "postgres"): Database {
        switch (type) {
            case "mysql":
                return new MySQLDatabase();
            case "postgres":
                return new PostgresDatabase();
        }
    }
}

const db = DatabaseFactory.create("postgres");
await db.query("SELECT * FROM users");
```

### Strategy Pattern

```typescript
interface SortStrategy<T> {
    sort(items: T[]): T[];
}

class QuickSort implements SortStrategy<number> {
    sort(items: number[]): number[] {
        // ... quicksort implementation
    }
}

class BubbleSort implements SortStrategy<number> {
    sort(items: number[]): number[] {
        // ... bubblesort implementation
    }
}

class Sorter<T> {
    constructor(private strategy: SortStrategy<T>) {}

    execute(items: T[]): T[] {
        return this.strategy.sort(items);
    }
}

const sorter = new Sorter(new QuickSort());
const sorted = sorter.execute([5, 2, 8, 1]);
```

## Testing

### Jest (Like Google Test)

```typescript
// test.spec.ts or test.test.ts
describe("Calculator", () => {
    describe("add", () => {
        it("should add two positive numbers", () => {
            expect(add(2, 3)).toBe(5);
        });

        it("should handle negative numbers", () => {
            expect(add(-1, 1)).toBe(0);
        });

        it("should throw on invalid input", () => {
            expect(() => add("a", 2)).toThrow();
        });
    });
});

// Async tests
describe("Database", () => {
    it("should load data", async () => {
        const data = await db.query("SELECT *");
        expect(data).toHaveLength(10);
    });
});

// Setup/teardown
describe("Server", () => {
    beforeAll(async () => {
        server = new Server();
        await server.start();
    });

    afterAll(async () => {
        await server.stop();
    });

    it("should handle requests", async () => {
        const res = await fetch("http://localhost:3000");
        expect(res.status).toBe(200);
    });
});
```

**Run tests**:
```bash
npm test
npm test -- --watch
npm test -- --coverage
```

## Module System

### ES6 Modules

```typescript
// math.ts
export function add(a: number, b: number): number {
    return a + b;
}

export const PI = 3.14159;

export default class Calculator {
    // ...
}

// main.ts
import Calculator, { add, PI } from "./math.js";

const result = add(2, 3);
console.log(PI);
const calc = new Calculator();
```

### CommonJS (Node.js Old Style)

```javascript
// math.js
module.exports = {
    add: (a, b) => a + b,
    PI: 3.14159
};

// main.js
const { add, PI } = require("./math.js");
const result = add(2, 3);
```

**Use ES6 modules for new code.**

## Common Pitfalls for C++ Developers

### 1. Type Coercion (Implicit Conversions)

```typescript
// ❌ BAD: Unexpected coercion
"5" + 3;           // "53" (string concatenation!)
"5" - 3;           // 2 (numeric operation!)
true + 1;          // 2
null == undefined; // true (loose equality)

// ✅ GOOD: Use strict equality and explicit types
"5" === 3;         // false
null === undefined; // false
Number("5") + 3;   // 8

// TypeScript prevents most of this
const x: string = "5";
const y: number = 3;
x + y;  // ❌ ERROR: Operator '+' cannot be applied to types 'string' and 'number'
```

### 2. `this` Binding

```typescript
class Logger {
    name = "Logger";

    // ❌ BAD: this may be wrong
    log(msg: string) {
        console.log(`${this.name}: ${msg}`);
    }

    // ✅ GOOD: Arrow function binds this
    logSafe = (msg: string) => {
        console.log(`${this.name}: ${msg}`);
    };
}

const logger = new Logger();
const boundLog = logger.log;
const boundLogSafe = logger.logSafe;

logger.log("hello");           // "Logger: hello" ✅
boundLog("hello");             // ❌ "undefined: hello" (this is wrong!)
boundLogSafe("hello");         // "Logger: hello" ✅
```

### 3. Hoisting (Variables Declared Anywhere)

```typescript
// ❌ WRONG: var hoists to top (confusing)
function example() {
    console.log(x);  // undefined (not an error!)
    var x = 5;
    console.log(x);  // 5
}

// ✅ CORRECT: let/const don't hoist
function example() {
    console.log(x);  // ❌ ERROR: x is not defined
    let x = 5;
    console.log(x);  // 5
}
```

### 4. Mutation vs Immutability

```typescript
// ❌ BAD: Mutating objects can cause bugs
const user = { name: "Alice", age: 30 };
const modified = user;
modified.age = 31;  // Also changes user.age!

// ✅ GOOD: Create new objects
const user = { name: "Alice", age: 30 };
const modified = { ...user, age: 31 };  // Spread operator
console.log(user.age);      // 30 (unchanged)
console.log(modified.age);  // 31 (new object)
```

### 5. Promise Anti-patterns

```typescript
// ❌ BAD: Unnecessary nesting
async function badAsync() {
    const data = await new Promise(resolve => {
        setTimeout(() => resolve(42), 1000);
    });
    return data;
}

// ✅ GOOD: Just return the promise
function goodAsync(): Promise<number> {
    return new Promise(resolve => {
        setTimeout(() => resolve(42), 1000);
    });
}

// ❌ BAD: Forgetting await
async function loadData() {
    const promise = fetchData();  // Promise, not data!
    console.log(promise);          // [object Promise]
}

// ✅ GOOD: Use await
async function loadData() {
    const data = await fetchData();
    console.log(data);  // Actual data
}
```

## WebAssembly Integration

For calling C++ code from JavaScript (like QLever):

```typescript
// Load compiled WebAssembly module
const wasm = await WebAssembly.instantiate(
    wasmModule,
    { env: { js_log: console.log } }
);

// Call exported functions
const result = wasm.instance.exports.query("SELECT * WHERE...");

// Access memory
const buffer = new Uint8Array(wasm.instance.exports.memory.buffer);
```

**Typical workflow**:
1. C++ code compiled to WebAssembly (.wasm)
2. JavaScript loads and instantiates the module
3. Call exported functions and pass data via memory buffers
4. Results returned as primitives or via shared memory

## Performance Considerations

### Common Performance Issues

```typescript
// ❌ SLOW: Creating new arrays repeatedly
function processLarge(items: number[]) {
    for (let i = 0; i < items.length; i++) {
        const subset = items.slice(0, i);  // New array each time!
        process(subset);
    }
}

// ✅ FAST: Reuse buffers or use iterators
function processLarge(items: number[]) {
    for (let i = 0; i < items.length; i++) {
        process(items.slice(0, i));  // Still allocates, but fewer
    }
}

// ❌ SLOW: JSON parsing in loop
for (const json of jsonArray) {
    const obj = JSON.parse(json);  // Parse each time
}

// ✅ FAST: Parse once, reuse
const objects = jsonArray.map(j => JSON.parse(j));

// ❌ SLOW: String concatenation in loop
let result = "";
for (const item of items) {
    result += item.toString();  // New string each time!
}

// ✅ FAST: Array join
const result = items.map(i => i.toString()).join("");
```

### Memory Leaks in JavaScript

```typescript
// ❌ LEAK: Global reference prevents GC
let cache: any[] = [];
function fetchAndCache(id: number) {
    const data = expensiveOperation(id);
    cache.push(data);  // Never removed, grows forever
}

// ✅ CORRECT: Limit cache size
const cache = new Map<number, any>();
function fetchAndCache(id: number) {
    const data = expensiveOperation(id);
    if (cache.size > 1000) {
        const firstKey = cache.keys().next().value;
        cache.delete(firstKey);
    }
    cache.set(id, data);
}

// ❌ LEAK: Event listeners not removed
element.addEventListener("click", handler);
// If element is removed, listener may prevent GC

// ✅ CORRECT: Remove listeners
element.removeEventListener("click", handler);
// Or use weak references
```

## Build Tools & NPM

### Package.json (Like Cargo.toml)

```json
{
  "name": "qlever-js",
  "version": "1.0.0",
  "description": "JavaScript bindings for QLever",
  "main": "dist/index.js",
  "scripts": {
    "build": "tsc",
    "test": "jest",
    "lint": "eslint src/**/*.ts",
    "dev": "tsc --watch"
  },
  "dependencies": {
    "axios": "^1.0.0"
  },
  "devDependencies": {
    "typescript": "^5.0.0",
    "jest": "^29.0.0",
    "@types/jest": "^29.0.0"
  }
}
```

### Common Commands

```bash
npm install              # Install dependencies (like conan install)
npm install lodash       # Add dependency
npm install --save-dev   # Add dev dependency
npm run build            # Run build script
npm test                 # Run tests
npm start                # Run start script
npm publish              # Publish to npm registry
npm run lint             # Run linter
```

## Resources

- **TypeScript Handbook**: https://www.typescriptlang.org/docs/
- **MDN Web Docs**: https://developer.mozilla.org/en-US/
- **JavaScript.info**: https://javascript.info/
- **Async/Await Guide**: https://javascript.info/async-await
- **Node.js Docs**: https://nodejs.org/en/docs/
- **WebAssembly Docs**: https://webassembly.org/
- **Effective TypeScript**: Book by Dan Vanderkam

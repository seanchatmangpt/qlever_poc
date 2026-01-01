# Learning Path — Choose Your Journey

Use this guide to navigate the documentation based on your goals and experience level.

---

## Quick Navigation

**I want to...** → **I should read...**

| Goal | Time | Resource |
|------|------|----------|
| 🚀 Get running in 15 minutes | 15 min | [Getting Started](./tutorials/getting-started.md) |
| 📋 Copy-paste a code snippet | 2 min | [Quick Reference](./QUICKREF.md) |
| 🎯 Execute different query types | 20 min | [Query Types How-To](./how-to/query-types.md) |
| ⚡ Make my queries faster | 25 min | [Performance How-To](./how-to/performance.md) |
| 📚 Look up a specific API | 5 min | [API Reference](./reference/api.md) |
| 💡 Understand the architecture | 30 min | [Architecture Guide](./explanations/architecture.md) |
| 🐛 Debug an error | 10 min | [Error Handling](./how-to/error-handling.md) *(coming soon)* |
| 🔌 Work with RDF data | 20 min | [RDF Data How-To](./how-to/rdf-data.md) *(coming soon)* |

---

## Learning Paths by Experience Level

### 👶 Totally New to Rust/SPARQL/RDF?

**Duration: 1-2 hours**

1. **Learn Rust basics** (if needed)
   - [Rust Book - Chapters 1-5](https://doc.rust-lang.org/book/) (~30 min)
   - Learn: variables, functions, ownership

2. **Learn SPARQL basics** (if needed)
   - [SPARQL Tutorial](https://www.w3.org/2009/sparql/wiki/Main_Page) (~20 min)
   - Learn: SELECT, WHERE, LIMIT

3. **Start with QLever** ✨
   - [Getting Started Tutorial](./tutorials/getting-started.md) (15 min)
   - Execute your first query!

4. **Learn query types**
   - [Query Types How-To](./how-to/query-types.md) (20 min)
   - Practice: Write queries in SPARQL

5. **Learn optimization**
   - [Performance How-To](./how-to/performance.md) (25 min)
   - Practice: Cache queries, batch operations

**Total time:** ~2 hours to productive ✅

---

### 🎯 I Know Rust, New to QLever/SPARQL

**Duration: 30-45 minutes**

1. **Skip to Getting Started** (but you already know async/await)
   - [Getting Started Tutorial](./tutorials/getting-started.md) (10 min)
   - Focus on the API sections, skip Rust explanations

2. **Learn SPARQL**
   - [Query Types How-To](./how-to/query-types.md) (15 min)
   - Understand: SELECT, ASK, CONSTRUCT, DESCRIBE

3. **Optimize immediately**
   - [Performance How-To](./how-to/performance.md) (20 min)
   - You care about: caching, concurrency, streaming

**Total time:** ~45 minutes to productive ✅

---

### 🚀 I Know SPARQL, New to Rust QLever Library

**Duration: 20-30 minutes**

1. **Just run Getting Started**
   - [Getting Started Tutorial](./tutorials/getting-started.md) (15 min)
   - You understand SPARQL; focus on Rust API

2. **Check Quick Reference**
   - [Quick Reference](./QUICKREF.md) (5 min)
   - Copy patterns for common operations

3. **Understand async/await if needed**
   - [Async/Await Model](./explanations/async-model.md) *(coming soon)* (10 min)

**Total time:** ~30 minutes to productive ✅

---

### 🏆 I Know Everything; Just Show Me the API

**Duration: 5 minutes**

1. ⚡ [Quick Reference](./QUICKREF.md) — Copy-paste patterns (2 min)
2. 📚 [API Reference](./reference/api.md) — Look up types (3 min)
3. ✅ Done!

---

## Feature-Based Learning Paths

### "I want to execute queries"

**Estimated time: 20 minutes**

```
Start: Getting Started [15 min]
         ↓
      Learn SPARQL variants [Query Types - 15 min]
         ↓
      Choose: ASK for speed OR SELECT for detail
         ↓
      Done! You can query
```

✅ Resources:
- [Getting Started](./tutorials/getting-started.md)
- [Query Types How-To](./how-to/query-types.md)
- [Quick Reference](./QUICKREF.md) — Copy patterns

---

### "I want to make my app fast"

**Estimated time: 25 minutes**

```
Start: Getting Started [15 min]
         ↓
      Learn Performance [Performance - 25 min]
         ↓
      Choose: Caching? Streaming? Concurrency?
         ↓
      Benchmark and measure
         ↓
      Done! You're optimized
```

✅ Resources:
- [Performance How-To](./how-to/performance.md) ⭐ (highest impact)
- [Quick Reference](./QUICKREF.md) — Copy cache patterns
- [Architecture Guide](./explanations/architecture.md) — Understand tradeoffs

---

### "I want to understand the design"

**Estimated time: 45 minutes**

```
Start: Architecture Guide [30 min]
         ↓
      Understand layers and design decisions
         ↓
      Deep-dive: FFI & Performance [20 min]
         ↓
      Deep-dive: Async Model [15 min]
         ↓
      Done! You understand the "why"
```

✅ Resources:
- [Architecture & Design](./explanations/architecture.md) ⭐
- [FFI & Performance](./explanations/ffi-performance.md) *(coming soon)*
- [Async/Await Model](./explanations/async-model.md) *(coming soon)*

---

### "I want to work with RDF data"

**Estimated time: 25 minutes**

```
Start: Getting Started [15 min]
         ↓
      Learn RDF concepts [RDF Data - 20 min]
         ↓
      Create NamedNodes, Literals, Triples
         ↓
      Work with query results
         ↓
      Done! You're comfortable with RDF
```

✅ Resources:
- [Getting Started](./tutorials/getting-started.md) — Learn concepts
- [RDF Data How-To](./how-to/rdf-data.md) *(coming soon)*
- [Quick Reference](./QUICKREF.md) — Copy patterns
- [API Reference](./reference/api.md) — Look up types

---

## Recommended Reading Order

### For Learning (First Time)
1. 📍 [Getting Started](./tutorials/getting-started.md) ← **Start here**
2. 📍 [Query Types](./how-to/query-types.md)
3. 📍 [Performance](./how-to/performance.md)
4. 📍 [API Reference](./reference/api.md) — as needed
5. 💡 [Architecture](./explanations/architecture.md) — optional deep-dive

### For Reference (After Learning)
1. ⚡ [Quick Reference](./QUICKREF.md) ← Start here
2. 📚 [API Reference](./reference/api.md) ← Look up types
3. 📋 [How-To Guides](./how-to/) ← Solve specific problems

### For Understanding Design
1. 💡 [Architecture](./explanations/architecture.md) ← Start here
2. 💡 [FFI & Performance](./explanations/ffi-performance.md)
3. 💡 [Async/Await](./explanations/async-model.md)

---

## Decision Tree: "Which Document Should I Read?"

```
┌─ Do I know SPARQL? ──→ No ──┐
│                              │
Yes                            └─→ Read: SPARQL tutorial
│                                  Then: Getting Started
└─ Do I know Rust async? ──→ No ──┐
                                   │
                         Yes       └─→ Read: Tokio tutorial
                         │              Then: Getting Started
                         │
                         └─ Do I just need code? ──→ Yes ──→ Quick Reference
                                                │
                                            No  └─ Want step-by-step? ──→ Yes ──→ Getting Started
                                                                          │
                                                                      No  └─ Need specific solution? ──→ Yes ──→ How-To Guides
                                                                                                       │
                                                                                                   No  └─ Want to understand design? ──→ Architecture
```

---

## Quickest Path to Each Goal

### "Run code in 5 minutes"
→ [Quick Reference](./QUICKREF.md)

### "Understand SPARQL in 15 minutes"
→ [Query Types How-To](./how-to/query-types.md)

### "Get productive in 30 minutes"
→ [Getting Started](./tutorials/getting-started.md) + [Query Types](./how-to/query-types.md)

### "Optimize queries in 30 minutes"
→ [Performance How-To](./how-to/performance.md)

### "Understand architecture in 45 minutes"
→ [Architecture Guide](./explanations/architecture.md) + [FFI Explanation](./explanations/ffi-performance.md)

---

## Resources by Type

### 🎓 Learning Paths
- [Getting Started Tutorial](./tutorials/getting-started.md) — Best for beginners
- This page — Choose your path

### 📋 Problem-Focused Guides
- [Query Types How-To](./how-to/query-types.md) — "How do I query?"
- [Performance How-To](./how-to/performance.md) — "How do I optimize?"
- [RDF Data How-To](./how-to/rdf-data.md) — "How do I work with RDF?"
- [Error Handling How-To](./how-to/error-handling.md) — "How do I handle errors?"

### 📚 Reference
- [Quick Reference](./QUICKREF.md) — One-page cheatsheet ⭐
- [API Reference](./reference/api.md) — Complete API
- [Error Types](./reference/errors.md) — All error types

### 💡 Understanding
- [Architecture & Design](./explanations/architecture.md) — How it works
- [FFI & Performance](./explanations/ffi-performance.md) — Performance details
- [Async/Await Model](./explanations/async-model.md) — Concurrency design

---

## Time Investment vs Value

```
Quick Reference    ████████████████  2 min   → Immediate value
Getting Started    ████████████████████████  15 min  → Foundation
Query Types        ██████████████████████   20 min  → Practical skills
Performance        ██████████████████████   25 min  → Production ready
Architecture       ██████████████████████████  30 min  → Deep understanding
All How-To Guides  ████████████████████████████  60+ min  → Expert level
```

**80/20 Recommendation:** Invest 45 minutes total:
1. Getting Started (15 min)
2. Query Types (15 min)
3. Performance (15 min)

= Productive for 95% of use cases ✅

---

## Still Lost?

1. **For quick answers:** [Quick Reference](./QUICKREF.md)
2. **For specific problem:** [How-To Guides](./how-to/)
3. **For complete details:** [API Reference](./reference/)
4. **For deep understanding:** [Architecture Guide](./explanations/architecture.md)
5. **Still stuck?** Open an [issue](https://github.com/seanchatmangpt/qlever/issues)

---

**Last Updated:** 2025-01-01 | **Status:** Learning Path Guide

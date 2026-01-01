# Datalog C++ API Documentation

Complete guide to using QLever's Datalog features programmatically from C++.

## Table of Contents

1. [Overview](#overview)
2. [Core Classes](#core-classes)
3. [Basic Usage](#basic-usage)
4. [Parsing Datalog Rules](#parsing-datalog-rules)
5. [Managing Rules](#managing-rules)
6. [Querying with Rules](#querying-with-rules)
7. [Advanced Features](#advanced-features)
8. [Error Handling](#error-handling)
9. [Thread Safety](#thread-safety)
10. [Performance Considerations](#performance-considerations)
11. [Complete Examples](#complete-examples)

---

## Overview

QLever's Datalog API provides:
- **RuleDatabase**: Thread-safe storage for Datalog rules
- **DatalogParser**: Parsing Datalog syntax into rule objects
- **DatalogRule**: Representation of individual rules
- **RuleExpansion**: Execution of non-recursive rules
- **FixpointComputation**: Execution of recursive rules
- **DatalogQueryPlanner**: Integration with SPARQL query planning

### Header Files

```cpp
#include "parser/DatalogParser.h"      // Parsing Datalog programs
#include "parser/DatalogRule.h"        // Rule representation
#include "parser/RuleDatabase.h"       // Rule storage
#include "engine/RuleExpansion.h"      // Non-recursive evaluation
#include "engine/FixpointComputation.h" // Recursive evaluation
#include "engine/DatalogQueryPlanner.h" // Query planning integration
```

---

## Core Classes

### DatalogRule

Represents a single Datalog rule.

**Declaration:**
```cpp
class DatalogRule {
 public:
  DatalogRule(std::string headPredicate,
              std::vector<Variable> headVariables,
              std::vector<SparqlTriple> bodyPatterns,
              std::vector<SparqlFilter> filters = {},
              bool isRecursive = false);

  [[nodiscard]] const std::string& getHeadPredicate() const;
  [[nodiscard]] const std::vector<Variable>& getHeadVariables() const;
  [[nodiscard]] const std::vector<SparqlTriple>& getBodyPatterns() const;
  [[nodiscard]] const std::vector<SparqlFilter>& getFilters() const;
  [[nodiscard]] size_t getArity() const;
  [[nodiscard]] bool isRecursive() const;
  [[nodiscard]] std::string toString() const;
};
```

**Example:**
```cpp
// Create a simple parent rule
DatalogRule parentRule(
    "parent",                          // Head predicate
    {Variable{"?x"}, Variable{"?y"}},  // Head variables
    {SparqlTriple(                     // Body pattern
        Variable{"?x"},
        TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
        Variable{"?y"}
    )}
);

std::cout << "Rule: " << parentRule.toString() << std::endl;
// Output: parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
```

---

### RuleDatabase

Thread-safe container for storing and retrieving rules.

**Declaration:**
```cpp
class RuleDatabase {
 public:
  void addRule(DatalogRule rule);

  [[nodiscard]] std::vector<DatalogRule> getRulesByPredicate(
      const std::string& predicateName) const;

  [[nodiscard]] std::vector<DatalogRule> getAllRules() const;

  [[nodiscard]] bool hasRuleFor(const std::string& predicateName) const;

  [[nodiscard]] size_t getRuleCount() const;

  [[nodiscard]] size_t getPredicateCount() const;

  void clear();

  [[nodiscard]] std::vector<std::string> getPredicateNames() const;
};
```

**Example:**
```cpp
auto ruleDb = std::make_shared<RuleDatabase>();

// Add multiple rules
ruleDb->addRule(parentRule);
ruleDb->addRule(ancestorBaseRule);
ruleDb->addRule(ancestorRecRule);

// Query rules
auto ancestorRules = ruleDb->getRulesByPredicate("ancestor");
std::cout << "Found " << ancestorRules.size() << " ancestor rules" << std::endl;

// Check if rule exists
if (ruleDb->hasRuleFor("parent")) {
  std::cout << "Parent rules defined" << std::endl;
}
```

---

### DatalogParser

Static parser for Datalog syntax.

**Declaration:**
```cpp
class DatalogParser {
 public:
  static DatalogRule parseDatalogRule(std::string_view ruleText);

  struct ParsedProgram {
    RuleDatabase ruleDatabase;
    std::vector<QueryInfo> queries;
    std::vector<std::string> warnings;
  };

  static ParsedProgram parseDatalogProgram(std::string_view programText);
};
```

**Example:**
```cpp
// Parse single rule
auto rule = DatalogParser::parseDatalogRule(
    "ancestor(?x, ?y) :- parent(?x, ?y)."
);

// Parse complete program
std::string program = R"(
  parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
  ancestor(?x, ?y) :- parent(?x, ?y).
  ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).
)";

auto parsed = DatalogParser::parseDatalogProgram(program);
std::cout << "Parsed " << parsed.ruleDatabase.getRuleCount() << " rules" << std::endl;
```

---

## Basic Usage

### Complete Workflow

```cpp
#include "parser/DatalogParser.h"
#include "parser/RuleDatabase.h"
#include "engine/QueryExecutionContext.h"

int main() {
  // 1. Create or load index
  Index index;
  index.createFromFile("data.ttl");

  // 2. Create query execution context
  QueryExecutionContext qec(index, ...);

  // 3. Parse and load Datalog rules
  std::string rules = R"(
    ancestor(?x, ?y) :- ?x <http://example.org/parentOf> ?y .
    ancestor(?x, ?z) :- ?x <http://example.org/parentOf> ?y, ancestor(?y, ?z).
  )";

  auto parsed = DatalogParser::parseDatalogProgram(rules);
  auto ruleDb = std::make_shared<RuleDatabase>();
  for (const auto& rule : parsed.ruleDatabase.getAllRules()) {
    ruleDb->addRule(rule);
  }

  // 4. Attach rules to query context
  qec.setRuleDatabase(ruleDb);

  // 5. Execute SPARQL query using Datalog rules
  std::string query = R"(
    SELECT ?anc ?desc WHERE {
      ?anc <http://example.org/ancestor> ?desc .
    }
  )";

  auto result = qec.execute(query);

  // 6. Process results
  for (const auto& row : result.idTable()) {
    std::cout << row[0] << " → " << row[1] << std::endl;
  }

  return 0;
}
```

---

## Parsing Datalog Rules

### Parsing Single Rule

```cpp
#include "parser/DatalogParser.h"

try {
  auto rule = DatalogParser::parseDatalogRule(
      "grandparent(?x, ?z) :- parent(?x, ?y), parent(?y, ?z)."
  );

  std::cout << "Head: " << rule.getHeadPredicate() << std::endl;
  std::cout << "Arity: " << rule.getArity() << std::endl;
  std::cout << "Body patterns: " << rule.getBodyPatterns().size() << std::endl;
  std::cout << "Recursive: " << (rule.isRecursive() ? "yes" : "no") << std::endl;

} catch (const ParseException& e) {
  std::cerr << "Parse error: " << e.what() << std::endl;
}
```

**Output:**
```
Head: grandparent
Arity: 2
Body patterns: 2
Recursive: no
```

---

### Parsing Program from File

```cpp
#include <fstream>
#include <sstream>
#include "parser/DatalogParser.h"

std::string loadFile(const std::string& filename) {
  std::ifstream file(filename);
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main() {
  // Load Datalog program from file
  std::string programText = loadFile("rules.datalog");

  // Parse program
  auto parsed = DatalogParser::parseDatalogProgram(programText);

  std::cout << "Parsed program:" << std::endl;
  std::cout << "  Rules: " << parsed.ruleDatabase.getRuleCount() << std::endl;
  std::cout << "  Predicates: " << parsed.ruleDatabase.getPredicateCount() << std::endl;
  std::cout << "  Queries: " << parsed.queries.size() << std::endl;

  // Check for warnings
  if (!parsed.warnings.empty()) {
    std::cout << "Warnings:" << std::endl;
    for (const auto& warning : parsed.warnings) {
      std::cout << "  - " << warning << std::endl;
    }
  }

  return 0;
}
```

---

### Handling Parse Errors

```cpp
#include "util/ParseException.h"

try {
  auto rule = DatalogParser::parseDatalogRule("invalid syntax here");
} catch (const ParseException& e) {
  std::cerr << "Parse error at line " << e.line()
            << ", column " << e.column() << ": "
            << e.what() << std::endl;
}
```

**Output:**
```
Parse error at line 1, column 8: Expected '(' after predicate name
```

---

## Managing Rules

### Adding Rules to Database

```cpp
auto ruleDb = std::make_shared<RuleDatabase>();

// Method 1: Add pre-constructed rule
DatalogRule rule1(...);
ruleDb->addRule(rule1);

// Method 2: Parse and add
auto rule2 = DatalogParser::parseDatalogRule("parent(?x, ?y) :- ...");
ruleDb->addRule(rule2);

// Method 3: Parse program and add all
auto program = DatalogParser::parseDatalogProgram(rulesText);
for (const auto& rule : program.ruleDatabase.getAllRules()) {
  ruleDb->addRule(rule);
}
```

---

### Querying RuleDatabase

```cpp
auto ruleDb = std::make_shared<RuleDatabase>();
// ... add rules ...

// Get all rules for a specific predicate
auto ancestorRules = ruleDb->getRulesByPredicate("ancestor");
for (const auto& rule : ancestorRules) {
  std::cout << rule.toString() << std::endl;
}

// Get all predicate names
auto predicates = ruleDb->getPredicateNames();
for (const auto& pred : predicates) {
  std::cout << "Predicate: " << pred << std::endl;
}

// Get all rules
auto allRules = ruleDb->getAllRules();
std::cout << "Total rules: " << allRules.size() << std::endl;

// Check existence
if (ruleDb->hasRuleFor("cousin")) {
  std::cout << "Cousin rules defined" << std::endl;
}
```

---

### Clearing Rules

```cpp
ruleDb->clear();
std::cout << "Rules cleared. Count: " << ruleDb->getRuleCount() << std::endl;
// Output: Rules cleared. Count: 0
```

---

## Querying with Rules

### Using Rules in SPARQL Queries

```cpp
#include "parser/SparqlParser.h"
#include "engine/QueryPlanner.h"

// 1. Setup rule database
auto ruleDb = std::make_shared<RuleDatabase>();
auto ancestorRule = DatalogParser::parseDatalogRule(
    "ancestor(?x, ?y) :- ?x <http://example.org/parentOf> ?y."
);
ruleDb->addRule(ancestorRule);

// 2. Attach to query execution context
qec.setRuleDatabase(ruleDb);

// 3. Write SPARQL query using rule predicate
std::string sparqlQuery = R"(
  PREFIX : <http://example.org/>
  SELECT ?ancestor ?descendant WHERE {
    ?ancestor :ancestor ?descendant .
  }
)";

// 4. Execute query
auto result = qec.execute(sparqlQuery);

// 5. Process results
for (size_t i = 0; i < result.idTable().size(); ++i) {
  const auto& row = result.idTable()[i];
  std::cout << "Ancestor: " << row[0] << ", Descendant: " << row[1] << std::endl;
}
```

---

### Direct Operation Usage (Advanced)

#### Non-Recursive Rule: RuleExpansion

```cpp
#include "engine/RuleExpansion.h"

// Setup
auto ruleDb = std::make_shared<RuleDatabase>();
ruleDb->addRule(parentRule);

// Create RuleExpansion operation
std::vector<TripleComponent> arguments = {
    Variable{"?x"},
    Variable{"?y"}
};

auto ruleExpansion = std::make_shared<RuleExpansion>(
    &qec,
    ruleDb,
    "parent",      // Rule predicate name
    arguments      // Arguments to bind
);

// Execute
auto result = ruleExpansion->getResult();

// Access results
const auto& idTable = result->idTable();
std::cout << "Result size: " << idTable.size() << " rows" << std::endl;
```

---

#### Recursive Rule: FixpointComputation

```cpp
#include "engine/FixpointComputation.h"

// Setup
auto ruleDb = std::make_shared<RuleDatabase>();
ruleDb->addRule(ancestorBaseRule);
ruleDb->addRule(ancestorRecRule);

// Create FixpointComputation operation
std::vector<TripleComponent> arguments = {
    Variable{"?x"},
    Variable{"?y"}
};

auto fixpoint = std::make_shared<FixpointComputation>(
    &qec,
    ruleDb,
    "ancestor",     // Recursive predicate name
    arguments,
    1000            // Max iterations (default: 1000)
);

// Execute
auto result = fixpoint->getResult();

// Access results
const auto& idTable = result->idTable();
std::cout << "Fixpoint result size: " << idTable.size() << " rows" << std::endl;
```

---

## Advanced Features

### Constructing Rules Programmatically

```cpp
#include "parser/DatalogRule.h"
#include "parser/SparqlTriple.h"

// Create head variables
std::vector<Variable> headVars = {Variable{"?x"}, Variable{"?y"}};

// Create body patterns
std::vector<SparqlTriple> bodyPatterns;

// Pattern 1: ?x :parentOf ?y
bodyPatterns.push_back(SparqlTriple(
    Variable{"?x"},
    TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
    Variable{"?y"}
));

// Create rule
DatalogRule customRule(
    "myCustomPredicate",   // Head predicate
    headVars,              // Head variables
    bodyPatterns,          // Body patterns
    {},                    // No filters
    false                  // Not recursive
);

// Add to database
ruleDb->addRule(customRule);
```

---

### Adding Filters to Rules

```cpp
#include "parser/data/SparqlFilter.h"

// Create filter: ?age > 18
SparqlFilter ageFilter(...);  // Construct filter expression

// Create rule with filter
std::vector<SparqlFilter> filters = {ageFilter};

DatalogRule ruleWithFilter(
    "adult",
    {Variable{"?person"}},
    bodyPatterns,
    filters,         // Pass filters
    false
);
```

**Note**: Filter construction requires understanding SPARQL expression API. See QLever's SPARQL parser for details.

---

### Recursive Rules with Multiple Body Patterns

```cpp
// Ancestor rule: ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z)
std::vector<SparqlTriple> recursiveBody = {
    SparqlTriple(
        Variable{"?x"},
        TripleComponent::Iri::fromIriref("<http://example.org/parentOf>"),
        Variable{"?y"}
    ),
    SparqlTriple(
        Variable{"?y"},
        TripleComponent::Iri::fromIriref("<http://example.org/ancestor>"),
        Variable{"?z"}
    )
};

DatalogRule ancestorRecursive(
    "ancestor",
    {Variable{"?x"}, Variable{"?z"}},
    recursiveBody,
    {},
    true  // Mark as recursive
);

ruleDb->addRule(ancestorRecursive);
```

---

## Error Handling

### Parse Errors

```cpp
#include "util/ParseException.h"

try {
  auto rule = DatalogParser::parseDatalogRule("malformed rule");
} catch (const ParseException& e) {
  std::cerr << "Error: " << e.what() << std::endl;
  std::cerr << "Line: " << e.line() << ", Column: " << e.column() << std::endl;
}
```

---

### Runtime Errors

```cpp
#include <stdexcept>

try {
  // Execute query with Datalog rules
  auto result = qec.execute(query);

} catch (const std::runtime_error& e) {
  std::cerr << "Execution error: " << e.what() << std::endl;

} catch (const std::exception& e) {
  std::cerr << "Unexpected error: " << e.what() << std::endl;
}
```

---

### Fixpoint Iteration Limit

```cpp
// Set custom iteration limit
auto fixpoint = std::make_shared<FixpointComputation>(
    &qec, ruleDb, "ancestor", arguments,
    500  // Max 500 iterations (default is 1000)
);

try {
  auto result = fixpoint->getResult();
} catch (const std::runtime_error& e) {
  std::cerr << "Error: " << e.what() << std::endl;
  // Possible message: "Fixpoint computation exceeded iteration limit (500)"
}
```

---

## Thread Safety

### RuleDatabase Thread Safety

**RuleDatabase is fully thread-safe**:

```cpp
auto ruleDb = std::make_shared<RuleDatabase>();

// Thread 1: Adding rules
std::thread t1([&ruleDb]() {
  ruleDb->addRule(rule1);
  ruleDb->addRule(rule2);
});

// Thread 2: Querying rules
std::thread t2([&ruleDb]() {
  auto rules = ruleDb->getRulesByPredicate("ancestor");
  // Process rules...
});

t1.join();
t2.join();
```

**Internal locking**: Uses `ad_utility::Synchronized<T>` for mutex protection.

---

### QueryExecutionContext Thread Safety

**QueryExecutionContext is NOT thread-safe for writes**:

```cpp
// DON'T DO THIS:
// std::thread t1([&qec]() { qec.setRuleDatabase(ruleDb1); });
// std::thread t2([&qec]() { qec.setRuleDatabase(ruleDb2); });

// INSTEAD: Create separate contexts
QueryExecutionContext qec1(index, ...);
QueryExecutionContext qec2(index, ...);

std::thread t1([&qec1]() {
  qec1.setRuleDatabase(ruleDb);
  auto result = qec1.execute(query);
});

std::thread t2([&qec2]() {
  qec2.setRuleDatabase(ruleDb);
  auto result = qec2.execute(query);
});
```

---

## Performance Considerations

### Memory Management

```cpp
// Use shared_ptr for RuleDatabase to avoid copies
auto ruleDb = std::make_shared<RuleDatabase>();

// Pass by const reference to avoid copies
void processRules(const RuleDatabase& db) {
  auto rules = db.getAllRules();
  // ...
}
```

---

### Rule Ordering

Rules are evaluated in the order added:

```cpp
// Base case first (recommended)
ruleDb->addRule(ancestorBase);
ruleDb->addRule(ancestorRecursive);

// vs. Recursive first (also valid, may affect optimization)
ruleDb->addRule(ancestorRecursive);
ruleDb->addRule(ancestorBase);
```

**Recommendation**: Add base cases before recursive cases for clarity.

---

### Iteration Limit Tuning

```cpp
// Conservative limit for potentially slow queries
auto fixpoint = std::make_shared<FixpointComputation>(
    &qec, ruleDb, "ancestor", args, 100
);

// Higher limit for known deep recursions
auto fixpoint2 = std::make_shared<FixpointComputation>(
    &qec, ruleDb, "transitiveClosure", args, 5000
);
```

---

### Pre-compiling Rules

```cpp
// Parse rules once at startup
static const auto GLOBAL_RULE_DB = []() {
  auto db = std::make_shared<RuleDatabase>();
  auto program = DatalogParser::parseDatalogProgram(RULE_SOURCE);
  for (const auto& rule : program.ruleDatabase.getAllRules()) {
    db->addRule(rule);
  }
  return db;
}();

// Reuse in multiple contexts
qec1.setRuleDatabase(GLOBAL_RULE_DB);
qec2.setRuleDatabase(GLOBAL_RULE_DB);
```

---

## Complete Examples

### Example 1: Family Tree Application

```cpp
#include <iostream>
#include <memory>
#include "parser/DatalogParser.h"
#include "parser/RuleDatabase.h"
#include "engine/QueryExecutionContext.h"
#include "index/Index.h"

int main() {
  // Load RDF data
  Index index;
  index.createFromFile("family.ttl");

  // Create query context
  QueryExecutionContext qec(index, /* ... */);

  // Define Datalog rules
  std::string familyRules = R"(
    # Base predicates
    parent(?x, ?y) :- ?x <http://example.org/parentOf> ?y .

    # Derived relationships
    child(?y, ?x) :- parent(?x, ?y).
    sibling(?x, ?y) :- parent(?p, ?x), parent(?p, ?y), ?x != ?y.

    # Ancestor (recursive)
    ancestor(?x, ?y) :- parent(?x, ?y).
    ancestor(?x, ?z) :- parent(?x, ?y), ancestor(?y, ?z).

    # Cousin (children of siblings)
    cousin(?x, ?y) :- parent(?p1, ?x), parent(?p2, ?y), sibling(?p1, ?p2).
  )";

  // Parse and load rules
  auto program = DatalogParser::parseDatalogProgram(familyRules);
  auto ruleDb = std::make_shared<RuleDatabase>();
  for (const auto& rule : program.ruleDatabase.getAllRules()) {
    ruleDb->addRule(rule);
  }

  qec.setRuleDatabase(ruleDb);

  // Query 1: Find all ancestors of Alice
  std::string query1 = R"(
    SELECT ?ancestor ?name WHERE {
      ?ancestor <http://example.org/ancestor> <http://example.org/Alice> .
      ?ancestor <http://example.org/name> ?name .
    }
  )";

  auto result1 = qec.execute(query1);
  std::cout << "Ancestors of Alice:" << std::endl;
  for (const auto& row : result1.idTable()) {
    std::cout << "  " << row[0] << ": " << row[1] << std::endl;
  }

  // Query 2: Find all cousin pairs
  std::string query2 = R"(
    SELECT ?name1 ?name2 WHERE {
      ?c1 <http://example.org/cousin> ?c2 .
      ?c1 <http://example.org/name> ?name1 .
      ?c2 <http://example.org/name> ?name2 .
      FILTER(?c1 < ?c2)
    }
  )";

  auto result2 = qec.execute(query2);
  std::cout << "\nCousins:" << std::endl;
  for (const auto& row : result2.idTable()) {
    std::cout << "  " << row[0] << " ↔ " << row[1] << std::endl;
  }

  return 0;
}
```

---

### Example 2: Graph Analysis Library

```cpp
#include "DatalogGraphAnalyzer.h"

class DatalogGraphAnalyzer {
 public:
  DatalogGraphAnalyzer(QueryExecutionContext* qec) : qec_(qec) {
    initializeRules();
  }

  // Find all nodes reachable from start
  IdTable findReachableNodes(const TripleComponent& startNode) {
    std::string query = R"(
      SELECT ?target WHERE {
        <)" + startNode.toRdfLiteral() + R"(> <http://example.org/reachable> ?target .
      }
    )";
    return qec_->execute(query).idTable();
  }

  // Find connected components
  std::vector<std::vector<TripleComponent>> findConnectedComponents() {
    std::string query = R"(
      SELECT ?node1 ?node2 WHERE {
        ?node1 <http://example.org/connected> ?node2 .
      }
    )";
    auto result = qec_->execute(query).idTable();
    // Process results into component groups...
    return components;
  }

 private:
  void initializeRules() {
    std::string graphRules = R"(
      edge(?x, ?y) :- ?x <http://example.org/edge> ?y .
      reachable(?x, ?y) :- edge(?x, ?y).
      reachable(?x, ?z) :- edge(?x, ?y), reachable(?y, ?z).
      connected(?x, ?y) :- reachable(?x, ?y), reachable(?y, ?x).
    )";

    auto program = DatalogParser::parseDatalogProgram(graphRules);
    auto ruleDb = std::make_shared<RuleDatabase>();
    for (const auto& rule : program.ruleDatabase.getAllRules()) {
      ruleDb->addRule(rule);
    }
    qec_->setRuleDatabase(ruleDb);
  }

  QueryExecutionContext* qec_;
};

// Usage
int main() {
  // Setup...
  DatalogGraphAnalyzer analyzer(&qec);

  auto reachable = analyzer.findReachableNodes(startNode);
  std::cout << "Reachable nodes: " << reachable.size() << std::endl;

  auto components = analyzer.findConnectedComponents();
  std::cout << "Connected components: " << components.size() << std::endl;

  return 0;
}
```

---

### Example 3: Dynamic Rule Loading

```cpp
#include <fstream>
#include <filesystem>

class DynamicRuleLoader {
 public:
  DynamicRuleLoader(QueryExecutionContext* qec) : qec_(qec) {
    ruleDb_ = std::make_shared<RuleDatabase>();
  }

  // Load rules from file
  void loadRulesFromFile(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    std::stringstream buffer;
    buffer << file.rdbuf();

    auto program = DatalogParser::parseDatalogProgram(buffer.str());

    for (const auto& rule : program.ruleDatabase.getAllRules()) {
      ruleDb_->addRule(rule);
    }

    qec_->setRuleDatabase(ruleDb_);

    std::cout << "Loaded " << program.ruleDatabase.getRuleCount()
              << " rules from " << filepath << std::endl;
  }

  // Load all .datalog files from directory
  void loadRulesFromDirectory(const std::filesystem::path& dirpath) {
    for (const auto& entry : std::filesystem::directory_iterator(dirpath)) {
      if (entry.path().extension() == ".datalog") {
        loadRulesFromFile(entry.path());
      }
    }
  }

  // Get current rule statistics
  void printStatistics() const {
    std::cout << "Rule Database Statistics:" << std::endl;
    std::cout << "  Total rules: " << ruleDb_->getRuleCount() << std::endl;
    std::cout << "  Predicates: " << ruleDb_->getPredicateCount() << std::endl;

    for (const auto& pred : ruleDb_->getPredicateNames()) {
      auto rules = ruleDb_->getRulesByPredicate(pred);
      std::cout << "    " << pred << ": " << rules.size() << " rule(s)" << std::endl;
    }
  }

 private:
  QueryExecutionContext* qec_;
  std::shared_ptr<RuleDatabase> ruleDb_;
};

// Usage
int main() {
  QueryExecutionContext qec(index, /* ... */);
  DynamicRuleLoader loader(&qec);

  // Load all rules from directory
  loader.loadRulesFromDirectory("/path/to/datalog/rules");

  // Print statistics
  loader.printStatistics();

  // Execute queries...
  return 0;
}
```

---

## API Reference Summary

### Core Classes

| Class | Purpose | Thread-Safe |
|-------|---------|-------------|
| `DatalogRule` | Rule representation | Immutable |
| `RuleDatabase` | Rule storage | Yes |
| `DatalogParser` | Parse Datalog syntax | Stateless |
| `RuleExpansion` | Execute non-recursive rules | No |
| `FixpointComputation` | Execute recursive rules | No |
| `DatalogQueryPlanner` | Integrate with SPARQL | No |

### Key Methods

#### RuleDatabase
- `void addRule(DatalogRule)`
- `std::vector<DatalogRule> getRulesByPredicate(string)`
- `std::vector<DatalogRule> getAllRules()`
- `bool hasRuleFor(string)`
- `size_t getRuleCount()`
- `void clear()`

#### DatalogParser
- `static DatalogRule parseDatalogRule(string_view)`
- `static ParsedProgram parseDatalogProgram(string_view)`

#### DatalogRule
- `const string& getHeadPredicate()`
- `const vector<Variable>& getHeadVariables()`
- `const vector<SparqlTriple>& getBodyPatterns()`
- `size_t getArity()`
- `bool isRecursive()`
- `string toString()`

---

## Best Practices

1. **Use shared_ptr for RuleDatabase**: Avoids copying large rule sets
2. **Parse rules once**: Cache parsed rules, reuse across queries
3. **Handle parse errors**: Always wrap parsing in try-catch
4. **Set iteration limits**: Prevent runaway recursion
5. **Enable logging**: Use QLever's logging for debugging
6. **Profile performance**: Monitor iteration counts and memory usage

---

**Next**: See [DATALOG_GUIDE.md](DATALOG_GUIDE.md) for conceptual overview or [DATALOG_EXAMPLES.md](DATALOG_EXAMPLES.md) for more examples.

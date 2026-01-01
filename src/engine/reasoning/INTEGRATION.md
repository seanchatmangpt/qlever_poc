# QueryPlanner Integration Guide for Reasoning Engine

## Overview

This document describes how to integrate the Reasoning Engine into QLever's existing QueryPlanner to enable Datalog and N3 reasoning during query execution.

## Integration Points

### 1. QueryPlanner::createExecutionTree()

Add reasoning detection at the start of query planning:

```cpp
QueryExecutionTree QueryPlanner::createExecutionTree(ParsedQuery& pq,
                                                      bool isSubquery) {
  // NEW: Check for N3 rule syntax
  reasoning::QueryReasoningHelper helper(executionContext_);

  if (helper.isReasoningQuery(pq)) {
    LOG(INFO) << "Detected N3 reasoning query";

    auto ruleDatabase = helper.extractRulesFromQuery(pq);

    // Extract output variables from SELECT clause
    std::vector<Variable> outputVars =
        pq.getSelectClause().getSelectedVariables();

    auto reasoningTree = helper.createReasoningExecutionTree(
        ruleDatabase, outputVars);

    if (reasoningTree) {
      return *reasoningTree;
    }
  }

  // Existing query planning logic...
  return existingQueryPlanner(...);
}
```

### 2. QueryPlanner Cost Estimation

Update cost estimation to account for reasoning:

```cpp
size_t QueryPlanner::estimateOperationCost(const Operation& op) {
  // Check if operation contains rules
  if (auto reasoningOp = dynamic_cast<const ReasoningOperation*>(&op)) {
    return reasoning::QueryReasoningHelper::estimateReasoningCost(
        reasoningOp->getRuleDatabase());
  }

  // Existing cost estimation...
  return existingCostEstimate(op);
}
```

### 3. Optimization: Property Path Conversion

Add optimization pass to detect simple transitive rules:

```cpp
std::shared_ptr<QueryExecutionTree> QueryPlanner::optimizeReasoning(
    const std::shared_ptr<RuleDatabase>& ruleDatabase) {

  reasoning::QueryReasoningHelper helper(executionContext_);

  // Check each rule for optimization opportunities
  for (const auto& rule : ruleDatabase->getRules()) {
    if (auto propertyPath = helper.convertToPropertyPath(*rule)) {
      LOG(INFO) << "Optimizing transitive rule to property path: "
                << propertyPath.value();

      // Create property path operation instead of general reasoning
      // This is more efficient for simple transitive closure
      return createPropertyPathOperation(propertyPath.value());
    }
  }

  // If no optimizations found, use general reasoning
  return createReasoningOperation(ruleDatabase);
}
```

## Implementation Steps

### Step 1: Add Header Includes

In `QueryPlanner.h`:

```cpp
#include "engine/reasoning/QueryReasoningHelper.h"
#include "engine/reasoning/ReasoningOperation.h"
```

### Step 2: Modify createExecutionTree()

In `QueryPlanner.cpp`, update the main planning function:

```cpp
QueryExecutionTree QueryPlanner::createExecutionTree(ParsedQuery& pq,
                                                      bool isSubquery) {
  // Check for N3 rules
  reasoning::QueryReasoningHelper reasoningHelper(executionContext_);

  if (reasoningHelper.containsN3Rules(pq._originalString)) {
    auto ruleDatabase = reasoningHelper.extractRulesFromSparql(
        pq._originalString);

    // Apply optimizations where applicable
    // ... existing logic to extract output variables ...

    auto reasoningTree = reasoningHelper.createReasoningExecutionTree(
        ruleDatabase, selectedVars);

    if (reasoningTree) {
      LOG(INFO) << "Planning reasoning query with "
                << ruleDatabase->size() << " rules";
      return *reasoningTree;
    }
  }

  // Fall back to existing query planner
  return existingCreateExecutionTree(pq, isSubquery);
}
```

### Step 3: Update Cost Estimation

In `QueryPlanner.cpp`, modify cost estimation:

```cpp
size_t QueryPlanner::getCostEstimate(const QueryExecutionTree& tree) {
  const auto& op = tree.getRootOperation();

  // Handle reasoning operations
  if (dynamic_cast<const ReasoningOperation*>(op) != nullptr) {
    return reasoning::QueryReasoningHelper::estimateReasoningCost(
        ruleDatabase);
  }

  // Existing cost estimation...
  return existingCostEstimate(op);
}
```

## Example: Using the Reasoning Engine

### Example 1: Transitive Closure

**Input Query (N3 Rule Format)**:
```sparql
PREFIX : <http://example.org/>

# Define ancestor relationship
{ ?x :parent ?y } => { ?x :ancestor ?y } .
{ ?x :ancestor ?z . ?z :ancestor ?y } => { ?x :ancestor ?y } .

# Query all ancestors
SELECT ?person ?ancestor WHERE {
  ?person :ancestor ?ancestor .
}
```

**Engine Processing**:
1. QueryPlanner detects N3 rule syntax
2. Creates RuleDatabase with 2 rules
3. Checks Rule 2: recursive pattern
4. Creates ReasoningOperation for fixpoint iteration
5. Executes reasoning to derive all ancestor facts
6. Returns results

### Example 2: Optimized Property Path

**Input Query**:
```sparql
PREFIX : <http://example.org/>

# Simple transitive closure (optimizable)
{ ?x :reaches ?y . ?y :reaches ?z } => { ?x :reaches ?z } .

SELECT ?start ?end WHERE {
  ?start :reaches ?end .
}
```

**Engine Processing**:
1. QueryPlanner detects N3 rule
2. QueryReasoningHelper detects optimizable pattern
3. Converts to SPARQL property path: `?x :reaches+ ?end`
4. Creates standard Operation (PathSearch or similar)
5. More efficient than general reasoning

## Performance Considerations

### When Reasoning is Used
- Complex rule chains (genealogy, knowledge inference)
- Recursive predicates (ancestor, reachability)
- Non-optimizable rule patterns

### When Property Paths Are Used
- Simple transitive closure: `{ p(X,Z), p(Z,Y) } => p(X,Y)`
- Binary relations only
- No complex rule patterns

### Memory Usage
- Fixpoint iteration stores all derived facts
- For large graphs, may exceed available memory
- Consider applying LIMIT/OFFSET to reason over subsets

## Testing the Integration

### Unit Tests

```cpp
TEST(QueryPlannerReasoningTest, DetectsN3Rules) {
  QueryExecutionContext qec(...);
  QueryPlanner planner(&qec);

  std::string n3Query = "{ ?x :parent ?y } => { ?x :ancestor ?y } SELECT ...";
  ParsedQuery pq = parseQuery(n3Query);

  auto tree = planner.createExecutionTree(pq);

  // Should create ReasoningOperation, not standard SELECT
  EXPECT_TRUE(dynamic_cast<ReasoningOperation*>(tree.getRootOperation()));
}

TEST(QueryPlannerReasoningTest, OptimizesTransitiveClosure) {
  // Test that simple transitive rules use property paths
}
```

### Integration Tests

Execute full queries with reasoning:

```bash
# Test file with N3 rules
./qlever_main --dataset test_data.nt --query test_reasoning.sparql

# Expected: reasoning executes and returns correct results
```

## Debugging

### Enable Logging

```cpp
// In QueryPlanner or ReasoningEngine code
LOG(DEBUG) << "Rule database contains " << ruleDatabase->size()
           << " rules";

for (const auto& stat : engine->getIterationStats()) {
  LOG(DEBUG) << "Iteration: " << stat.newFactsGenerated
             << " new facts, total: " << stat.totalFactsInDatabase;
}
```

### Trace Rule Applications

```cpp
// In ReasoningEngine::applyRule()
LOG(DEBUG) << "Applying rule " << rule.getRuleId();
LOG(DEBUG) << "Generated " << newFacts.size() << " facts";
```

## Migration Path

### Phase 1: Core Integration
- Add QueryReasoningHelper to QueryPlanner
- Basic N3 rule detection and execution
- Test with simple rules

### Phase 2: Optimization
- Implement property path conversion
- Cost-based selection between approaches
- Performance profiling

### Phase 3: Advanced Features
- Stratified negation support
- Built-in predicates in rule bodies
- Incremental reasoning updates

### Phase 4: Production Ready
- Comprehensive test suite
- Documentation and examples
- Performance benchmarks
- Production deployment

## Related Files

- `src/engine/reasoning/ReasoningOperation.h` - Main operation class
- `src/engine/reasoning/QueryReasoningHelper.h` - Integration helper
- `src/engine/reasoning/Rule.h` - Rule storage
- `src/engine/reasoning/N3Parser.h` - N3 parsing
- `test/engine/reasoning/` - Test suite

## References

- Main README: `src/engine/reasoning/README.md`
- CLAUDE.md: `CLAUDE.md` in repository root
- QLever Documentation: Repository documentation

## Questions?

For implementation questions, refer to:
1. `ReasoningOperation.h` for Operation interface
2. `QueryPlanner.h` for planning integration
3. Test files in `test/engine/reasoning/` for examples

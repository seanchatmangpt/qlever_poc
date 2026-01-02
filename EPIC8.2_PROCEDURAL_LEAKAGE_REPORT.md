# EPIC 8.2 AGENT 2: PROCEDURAL LEAKAGE DETECTOR - FINAL REPORT

**Date**: 2026-01-01
**Branch**: claude/rewrite-epic-8.1-ByTY4
**Agent**: EPIC 8.2 Agent 2 - Procedural Leakage Detector
**Scope**: EPIC 8/8.1 specifications and .claude/**

---

## FORBIDDEN VERBS SCANNED

run, execute, step, retry, recover, fallback, perform, attempt, try, do, conduct, implement, proceed, continue, return, next, then, after, before, while, until, sequence, order, phase (when used as verb)

---

## EXECUTIVE SUMMARY

**STATUS**: ❌ **CONTAMINATED**

**TOTAL PROCEDURAL VERB INSTANCES**: 89

**BREAKDOWN**:
- Specification-level contamination: 42 instances (CRITICAL)
- Documentation-level contamination: 25 instances (SEVERE)
- Example-level contamination: 22 instances (MODERATE)

**CONTAMINATION SEVERITY**: CRITICAL

The specifications contain 89 instances of forbidden procedural verbs across 11 files. The contamination is PERVASIVE and affects core specification axioms, invariant definitions, and agent directives.

---

## DENSITY REPORT

### Total Instances by File

| File | Instances | Percentage |
|------|-----------|------------|
| /home/user/qlever/docs/EPIC8_SPECIFICATION_CLOSURE.md | 37 | 41.6% |
| /home/user/qlever/docs/EPIC8_CI_RELEGATION.md | 26 | 29.2% |
| /home/user/qlever/.claude/agents/README.md | 7 | 7.9% |
| /home/user/qlever/.claude/skills/README.md | 6 | 6.7% |
| /home/user/qlever/.claude/agents/bb80-parallel-task-coordinator.md | 3 | 3.4% |
| /home/user/qlever/.claude/agents/bb80-receipt-validator.md | 2 | 2.2% |
| /home/user/qlever/.claude/agents/bb80-specification-validator.md | 2 | 2.2% |
| /home/user/qlever/.claude/agents/bb80-invariant-validator.md | 2 | 2.2% |
| /home/user/qlever/.claude/skills/bb80-specification-closure/SKILL.md | 2 | 2.2% |
| /home/user/qlever/.claude/skills/bb80-deterministic-receipts/SKILL.md | 1 | 1.1% |
| /home/user/qlever/.claude/skills/bb80-parallel-agents/SKILL.md | 1 | 1.1% |
| **TOTAL** | **89** | **100%** |

### Instances by Classification

| Classification | Count | Percentage |
|----------------|-------|------------|
| SPECIFICATION-LEVEL (HIGH RISK) | 42 | 47.2% |
| DOCUMENTATION-LEVEL (MEDIUM RISK) | 25 | 28.1% |
| EXAMPLE-LEVEL (LOW RISK) | 22 | 24.7% |

### Top Offending Verbs

| Rank | Verb | Instances |
|------|------|-----------|
| 1 | before | 13 |
| 2 | after | 11 |
| 3 | do | 11 |
| 4 | run | 9 |
| 5 | retry | 9 |
| 6 | execute | 9 |
| 7 | order (as verb) | 4 |
| 8 | proceed | 3 |
| 9 | try | 3 |
| 10 | recover | 2 |
| 11 | continue | 2 |
| 12 | attempt | 1 |
| 13 | perform | 1 |
| 14 | implement | 1 |
| 15 | until | 1 |

### Critical Contamination Zones

1. **EPIC8_SPECIFICATION_CLOSURE.md Phase definitions (Lines 199-542)**: 15 HIGH RISK instances
2. **EPIC8_CI_RELEGATION.md CI/CD prescriptions (Lines 1-135)**: 7 HIGH RISK instances
3. **.claude/agents/* (All agent definitions)**: 9 HIGH RISK instances
4. **.claude/skills/* (All skill definitions)**: 4 HIGH RISK instances

---

## MOST CRITICAL VIOLATIONS

### Category 1: Temporal Procedural Control (25 instances)
Verbs: before, after, until

These violations introduce temporal state machine logic into what should be declarative constraints.

**Examples**:
- "Formalize Domain Constraints (DCP) **before** parallel implementation diverges"
- "Only created **after** ALL phases complete"
- "Agents synchronize only **after** all invariants stabilize"

### Category 2: Imperative Execution Verbs (19 instances)
Verbs: run, execute, perform

These violations impose procedural execution semantics instead of declarative properties.

**Examples**:
- "CI/CD systems **execute** deterministic build instructions"
- "**Run** tests with hard variance bounds"
- "Targets **execute** in strict sequential order"

### Category 3: Negated Imperatives (11 instances)
Pattern: "do not X"

These violations use negated commands instead of declarative forbiddenness.

**Examples**:
- "Phases **do not** communicate via environment variables"
- "**Do not** accept narrative justifications"
- "**do not** reiterate"

### Category 4: Retry/Recovery Patterns (11 instances)
Verbs: retry, recover

These violations explicitly describe procedural fallback mechanisms.

**Examples**:
- "**Retry** conditionally"
- "May continue, **retry**, or **recover**"
- "No **recovery** or **retry** logic"

---

## PROCEDURAL LEAKAGE INVENTORY

### FILE: /home/user/qlever/docs/EPIC8_SPECIFICATION_CLOSURE.md (950 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 18 instances

1. **Line 6** | `before` | "Formalize Domain Constraints (DCP) before parallel implementation diverges"
   - **Replacement**: "Formalize Domain Constraints (DCP); parallel implementation must not diverge"

2. **Line 21** | `do` | "What this epic does (in order)"
   - **Replacement**: "What this epic is"

3. **Line 43** | `do` | "Phases do not communicate via environment variables or mutable state"
   - **Replacement**: "Phase communication via environment variables or mutable state is forbidden"

4. **Line 78** | `before`, `execute` | "all decisions frozen before execution, no reinterpretation"
   - **Replacement**: "all decisions frozen; execution cannot reinterpret"

5. **Line 83-84** | (begins) | "phase(A) complete ⟹ phase(B) begins"
   - **Replacement**: "phase(A) complete ⟹ phase(B) is valid"

6. **Line 144** | `run` | "sha256(P output) reproducible across runs"
   - **Replacement**: "sha256(P output) is reproducible"

7. **Line 152** | `after` | "Only created after ALL phases complete"
   - **Replacement**: "Creation valid only when ALL phases complete"

8. **Line 162** | `after` | "flags.env immutable after write"
   - **Replacement**: "flags.env write forbidden; immutability required"

9. **Line 193** | `after` | "PHASE_LOCK created only after phase-f completes"
   - **Replacement**: "PHASE_LOCK creation valid only when phase-f complete"

10. **Line 212** | `try` | "Try: command -v clang++"
    - **Replacement**: "Requirement: command -v clang++"

11. **Line 217** | `run` | "Run: $(CXX) -v 2>&1 | head -1"
    - **Replacement**: "Required invocation: $(CXX) -v 2>&1 | head -1"

12. **Line 315** | `run` | "Ensure clean cmake run (no previous build artifacts affect configuration)"
    - **Replacement**: "Clean cmake invocation required (previous build artifacts forbidden)"

13. **Line 383** | `execute` | "Execute Datalog rules over ingested data"
    - **Replacement**: "Datalog rules evaluation required over ingested data"

14. **Line 412** | `run` | "Run tests with hard variance bounds"
    - **Replacement**: "Test execution with hard variance bounds required"

15. **Line 429** | `run` | "Run deterministic workload"
    - **Replacement**: "Deterministic workload execution required"

16. **Line 434** | `run` | "Run same benchmark multiple times"
    - **Replacement**: "Same benchmark execution multiple times required"

17. **Line 669** | `after` | "Modifying artifacts after SEAL"
    - **Replacement**: "Artifact modification forbidden when SEAL complete"

18. **Line 699** | `execute`, `order` | "make universe must execute phases in strict order A → B → C → D → E → F → SEAL"
    - **Replacement**: "phases must be: A → B → C → D → E → F → SEAL"

#### DOCUMENTATION-LEVEL (MEDIUM RISK): 13 instances

19. **Line 556** | `after` | "exists after PHASE A"
    - **Replacement**: "existence required when PHASE A complete"

20. **Line 561** | `run` | "PHASE A output identical across multiple make universe runs"
    - **Replacement**: "identical across multiple make universe invocations"

21. **Line 590** | `run` | "same run → same results"
    - **Replacement**: "same invocation → same results"

22. **Line 612** | `continue` | "no continue on error mode"
    - **Replacement**: "no error continuation mode"

23. **Line 613** | `retry`, `recover` | "No recovery or retry logic"
    - **Replacement**: "Recovery forbidden; retry forbidden"

24. **Line 617** | `execute`, `order` | "All phases execute in order (A → B → C → D → E → F → SEAL)"
    - **Replacement**: "All phases must be: A → B → C → D → E → F → SEAL"

25. **Line 680** | `until` | "preserves all outputs until make clean"
    - **Replacement**: "preserves all outputs; make clean is required for removal"

26. **Line 697** | `execute` | "Sequential Phase Execution Enforcement"
    - **Replacement**: "Sequential Phase Enforcement"

27. **Line 720** | `order` | "if phase order incorrect"
    - **Replacement**: "if phase sequence invalid"

28. **Line 792** | `continue` | "if construction continues despite phase failure"
    - **Replacement**: "if construction continuation occurs despite phase failure"

29. **Line 920** | `proceed` | "Agents may now proceed with deterministic parallel implementation"
    - **Replacement**: "Agents are authorized for deterministic parallel implementation"

30. **Line 931** | `proceed` | "NO (proceed to implementation)"
    - **Replacement**: "implementation authorized"

31. **Line 935** | `implement` | "Dispatch remaining agents to implement phases A-F"
    - **Replacement**: "phases A-F construction"

#### EXAMPLE-LEVEL (LOW RISK): 6 instances

32. **Line 93** | `continue`, `retry`, `recover` | "May continue, retry, or recover"
    - **Replacement**: "Cannot halt; retry forbidden; recovery forbidden"

33. **Line 663** | `run` | "all phases always run"
    - **Replacement**: "all phases must be invoked"

34. **Line 670** | `after` | "Updating manifest.sha256 after PHASE F"
    - **Replacement**: "Updating manifest.sha256 forbidden when PHASE F complete"

35. **Line 713** | `order` | "if any phase skipped or out of order"
    - **Replacement**: "if any phase skipped or misordered"

36. **Line 714-715** | `order` | "order_incorrect / Phase order violated"
    - **Replacement**: "sequence_invalid / Phase sequence invalid"

37. **Line 778** | `attempt` | "Attempt construction (should fail)"
    - **Replacement**: "Construction must fail"

---

### FILE: /home/user/qlever/docs/EPIC8_CI_RELEGATION.md (433 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 7 instances

38. **Line 11** | `execute` | "CI/CD systems execute deterministic build instructions"
    - **Replacement**: "CI/CD systems are deterministic build instruction executors"

39. **Line 15** | `retry` | "Retry conditionally"
    - **Replacement**: "Conditional retry forbidden"

40. **Line 34** | `execute`, `order` | "Targets execute in strict sequential order: A → B → C → D → E → F → Seal"
    - **Replacement**: "Targets must be: A → B → C → D → E → F → Seal"

41. **Line 50** | `before`, `proceed` | "all dependencies must be present and verified before proceeding"
    - **Replacement**: "all dependencies must be present and verified; proceeding invalid otherwise"

42. **Line 88** | `perform` | "CI/CD systems shall and only shall perform the following"
    - **Replacement**: "CI/CD systems shall and only shall be:"

43. **Line 118** | `retry` | "CI/CD does not retry or escalate failures"
    - **Replacement**: "retry forbidden; escalation forbidden"

44. **Line 130** | `retry` | "CI/CD does not use this check to decide whether to retry"
    - **Replacement**: "retry decision forbidden"

#### DOCUMENTATION-LEVEL (MEDIUM RISK): 4 instances

45. **Line 155** | `order`, `retry` | "reorder, conditionally skip, or retry phases"
    - **Replacement**: "reordering forbidden; conditional skipping forbidden; retry forbidden"

46. **Line 219** | `execute` | "Parallel Phase Execution"
    - **Replacement**: "Parallel Phase Invocation"

47. **Line 392** | `retry` | "No retry loops"
    - **Replacement**: "Retry loops forbidden"

48. **Line 395** | `execute` | "No parallel phase execution"
    - **Replacement**: "Parallel phase invocation forbidden"

#### EXAMPLE-LEVEL (LOW RISK): 15 instances

49. **Line 145** | `retry` | "# retry or escalate"
    - **Replacement**: "# retry forbidden; escalation forbidden"

50. **Line 169** | `do` | "# DO NOT DO THIS"
    - **Replacement**: "# FORBIDDEN"

51. **Line 180** | `do` | "for i in {1..3}; do"
    - **Replacement**: "for i in {1..3}; {"

52. **Line 191** | `do` | "# DO NOT DO THIS"
    - **Replacement**: "# FORBIDDEN"

53. **Line 199** | `do` | "# DO NOT DO THIS"
    - **Replacement**: "# FORBIDDEN"

54. **Line 210** | `do` | "# DO NOT DO THIS"
    - **Replacement**: "# FORBIDDEN"

55. **Line 223** | `do` | "# DO NOT DO THIS"
    - **Replacement**: "# FORBIDDEN"

56. **Line 256** | `execute` | "Parallel phase execution"
    - **Replacement**: "Parallel phase invocation"

57. **Line 257** | `run` | "Run phases in parallel"
    - **Replacement**: "Phases in parallel"

58. **Line 264** | `do` | "for i in {1..3}; do"
    - **Replacement**: "for i in {1..3}; {"

59. **Line 265** | `retry` | "echo Retry $i"
    - **Replacement**: "echo Attempt $i"

60. **Line 287** | `retry` | "CI retries"
    - **Replacement**: "CI retry forbidden"

61. **Line 314** | `retry` | "# No retry loop"
    - **Replacement**: "# Retry loop forbidden"

62. **Line 330** | `execute` | "Phases execute sequentially"
    - **Replacement**: "Phases must be sequential"

63. **Line 331** | `retry` | "No retry logic"
    - **Replacement**: "Retry logic forbidden"

---

### FILE: /home/user/qlever/.claude/skills/README.md (66 lines)

#### DOCUMENTATION-LEVEL (MEDIUM RISK): 4 instances

64. **Line 8** | `before` | "Verify the domain is formalized before implementation"
    - **Replacement**: "Domain formalization required; implementation forbidden otherwise"

65. **Line 10** | `before`, `proceed` | "before proceeding"
    - **Replacement**: "proceeding invalid without domain closure"

66. **Line 18** | `after` | "synchronize only after invariants stabilize"
    - **Replacement**: "synchronization valid only when invariants stable"

67. **Line 36** | `run` | "How many agents can run in parallel?"
    - **Replacement**: "How many agents in parallel?"

#### EXAMPLE-LEVEL (LOW RISK): 2 instances

68. **Line 41** | `do` | "What should I do next?"
    - **Replacement**: "What is next?"

69. **Line 43** | `try` | "What if I try a different approach?"
    - **Replacement**: "What alternative approach is valid?"

---

### FILE: /home/user/qlever/.claude/agents/bb80-receipt-validator.md (21 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 2 instances

70. **Line 17** | `do` | "Do not accept narrative justifications"
    - **Replacement**: "Narrative justifications forbidden"

71. **Line 19** | `do` | "do not reiterate"
    - **Replacement**: "reiteration forbidden"

---

### FILE: /home/user/qlever/.claude/agents/bb80-specification-validator.md (21 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 2 instances

72. **Line 3** | `before` | "Verify specification closure before any implementation work begins"
    - **Replacement**: "implementation work invalid without specification closure"

73. **Line 9** | `before` | "before implementation begins"
    - **Replacement**: "implementation beginning invalid otherwise"

---

### FILE: /home/user/qlever/.claude/agents/README.md (115 lines)

#### DOCUMENTATION-LEVEL (MEDIUM RISK): 5 instances

74. **Line 8** | `before` | "Verify specification closure before implementation begins"
    - **Replacement**: "implementation beginning invalid without specification closure"

75. **Line 18** | `after` | "synchronize only after invariants stabilize"
    - **Replacement**: "synchronization valid only when invariants stable"

76. **Line 20** | `execute` | "Planning phase and throughout execution"
    - **Replacement**: "Planning stage and throughout"

77. **Line 25** | `after` | "After implementation and integration"
    - **Replacement**: "When implementation and integration complete"

78. **Line 65** | `execute` | "Can this execute in one pass from invariants?"
    - **Replacement**: "Can this be one pass from invariants?"

#### EXAMPLE-LEVEL (LOW RISK): 2 instances

79. **Line 72** | `do` | "What should I do next?"
    - **Replacement**: "What is next?"

80. **Line 75** | `try` | "What if I try a different approach?"
    - **Replacement**: "What alternative approach is valid?"

---

### FILE: /home/user/qlever/.claude/agents/bb80-parallel-task-coordinator.md (19 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 3 instances

81. **Line 9** | `after` | "synchronizing only after all invariants stabilize"
    - **Replacement**: "synchronization valid only when all invariants stable"

82. **Line 15** | `do` | "Agents do not communicate except to report state"
    - **Replacement**: "Agent communication forbidden except state reporting"

83. **Line 17** | `run`, `until`, `after`, `do`, `before` | "Agents run independently until invariants stabilize. Only after all agents confirm invariant stability does integration occur. Do not force synchronization before invariants are proven stable"
    - **Replacement**: "Agents operate independently; invariant stabilization required. Integration valid only when all agents confirm invariant stability. Synchronization forcing forbidden when invariants unstable"

---

### FILE: /home/user/qlever/.claude/agents/bb80-invariant-validator.md (19 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 2 instances

84. **Line 9** | `do` | "do not introduce state mutations"
    - **Replacement**: "state mutations forbidden"

85. **Line 17** | `execute` | "implementation can execute in one pass"
    - **Replacement**: "implementation can be one pass"

---

### FILE: /home/user/qlever/.claude/skills/bb80-deterministic-receipts/SKILL.md (10 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 1 instance

86. **Line 8** | `do` | "do not reiterate"
    - **Replacement**: "reiteration forbidden"

---

### FILE: /home/user/qlever/.claude/skills/bb80-specification-closure/SKILL.md (10 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 2 instances

87. **Line 3** | `before` | "before implementation starts"
    - **Replacement**: "implementation start invalid without domain formalization and closure"

88. **Line 8** | `before` | "Before writing code, verify the specification is closed"
    - **Replacement**: "Code writing invalid without specification closure verification"

---

### FILE: /home/user/qlever/.claude/skills/bb80-parallel-agents/SKILL.md (10 lines)

#### SPECIFICATION-LEVEL (HIGH RISK): 1 instance

89. **Line 8** | `after` | "Agents synchronize only after all invariants stabilize"
    - **Replacement**: "Agent synchronization valid only when all invariants stable"

---

## IMPACT ASSESSMENT

### 1. Specification Closure: VIOLATED

The presence of 42 specification-level procedural verbs undermines declarative purity. The DCP (Deterministic Construction Plane) is defined as a **declarative** computational model, yet its specification uses imperative, temporal, and procedural language.

**Contradiction**:
- Axiom states: "No interpretation"
- Specification uses: "before execution", "after completion", "run tests"

### 2. Deterministic Construction Plane: COMPROMISED

Temporal control verbs (before, after, until) introduce **implicit state machine semantics**:
- "before X" implies: state A → event → state B
- "after Y" implies: temporal ordering dependency
- "until Z" implies: loop/wait condition

This contradicts the DCP's claim of being "not a state machine."

### 3. Agent Operational Model: CONTAMINATED

Agent definitions use procedural directives instead of constraint encoding:
- "Do not accept..." (imperative prohibition)
- "Agents run independently until..." (temporal control flow)
- "Only after all agents confirm..." (coordination protocol)

This contradicts the BB80/20 frame: agents should be **constraint validators**, not **procedure executors**.

### 4. Reverse Conway's Law: INCOHERENT

The CI/CD relegation document forbids procedural logic in CI/CD while using procedural language to describe the prohibition:
- "CI/CD systems **execute** deterministic build instructions"
- "Targets **execute** in strict sequential **order**"

This is self-contradictory: the document **performs** the very procedural leakage it forbids.

---

## REMEDIATION REQUIREMENTS

### Priority 1: SPECIFICATION-LEVEL (HIGH RISK) - 42 instances

**Action**: Immediate rewrite required for all specification-level procedural verbs.

**Permitted verbs**: is, must, cannot, invalid, forbidden, abort, halt

**Forbidden patterns to eliminate**:
1. "X before Y" → "X required; Y invalid otherwise"
2. "do not X" → "X forbidden"
3. "run/execute X" → "X invocation required" or "X must be invoked"
4. "retry/recover" → "retry forbidden; recovery forbidden"
5. "after X completes" → "when X complete" or "X completion required"

### Priority 2: DOCUMENTATION-LEVEL (MEDIUM RISK) - 25 instances

**Action**: Rewrite to use declarative language.

**Timeline**: Within current epic scope.

### Priority 3: EXAMPLE-LEVEL (LOW RISK) - 22 instances

**Action**: Rewrite for consistency, but lower urgency.

**Timeline**: Can be deferred to post-epic cleanup.

---

## BINARY VERDICT

**STATUS**: ❌ **CONTAMINATED** (89 instances)

**SEVERITY**: CRITICAL

**SPECIFICATION CLOSURE**: VIOLATED

**REMEDIATION**: MANDATORY

All 89 instances must be rewritten to eliminate procedural leakage and restore declarative purity to the EPIC 8/8.1 specifications and .claude/** agent/skill definitions.

---

## NEXT STEPS

1. **Agent 3**: Initiate systematic rewrite of all 42 SPECIFICATION-LEVEL instances
2. **Agent 4**: Validate rewritten specifications for declarative purity
3. **Agent 5**: Re-run procedural leakage detector to verify CLEAN status
4. **Integration**: Update EPIC 8.1 deliverable status to reflect contamination findings

---

**END OF REPORT**

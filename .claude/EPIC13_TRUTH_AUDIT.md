# EPIC 13: Truth Audit — .claude/ Directory

**Date**: 2026-01-03
**Purpose**: Identify lies, stubs, false claims in configuration and instructions
**Status**: AUDIT FINDINGS

---

## Executive Summary

The `.claude/` directory makes **10+ aspirational claims** about infrastructure that does not exist:

| Claim | Status | Reality |
|-------|--------|---------|
| "MANDATORY: Spawn 10 agents FIRST" | 🚨 ENFORCED BY ME, NOT BY SYSTEM | No orchestration layer exists |
| "Binary gates enforce closure" | 🚨 ASPIRATIONAL | Gates are descriptions, not enforced |
| "Failure at any point → NO OUTPUT" | 🚨 ASPIRATIONAL | All outputs are emitted regardless |
| "7/7 deterministic guards" | 🚨 ASPIRATIONAL | No guard enforcement exists |
| "Deterministic receipts validate work" | 🚨 ASPIRATIONAL | No receipt validation layer |
| "Skills activate behavior" | 🚨 STUB | Skills are 800-byte files, not implementations |
| "Agents coordinate under shared invariant" | 🚨 STUB | Agents are markdown instructions, not executables |

**EPIC 13 Action**: Refactor to eliminate all aspirational claims and document actual capabilities.

---

## Detailed Findings

### 1. CLAUDE.md: Claims vs. Reality

#### Claim: "MANDATORY: Agents FIRST (Before Reading, Before Planning)"

**Status**: 🚨 **LIE**

```markdown
# CLAUDE.md Line 105-107
**FIRST ACTION: Spawn 10 agents in parallel. Do not read files. Do not plan. Agents gather context.**

Agents report findings in parallel while you invoke skills and implement.
Do not wait for reports before starting work.
Do not read files yourself. Let agents gather context.
```

**Reality**:
- No orchestration layer spawns agents
- No mechanism ensures agents launch first
- I (Claude) choose when/whether to use Task tool with subagent_type
- CLAUDE.md describes a *desired behavior*, not an enforced one
- I followed it in the audit because it was well-reasoned, not because it's enforced

**Chatman Equation Violation**:
- **μ** claims: "System enforces agent-first pattern"
- **A** = CLAUDE.md file (but the rule isn't enforced anywhere)
- **Reality**: No enforcement exists; it's a recommendation I chose to follow

---

#### Claim: "MANDATORY: Skills & Agents (Every Non-Trivial Task)"

**Status**: 🚨 **STUB**

CLAUDE.md lists 11 specific required components:

```markdown
# Line 117-127
**Specification Phase**: Invoke bb80-specification-closure skill → dispatch bb80-specification-validator agent
**Planning Phase**: Invoke bb80-parallel-agents skill → dispatch bb80-parallel-task-coordinator agent
**Implementation Phase**: Invoke bb80-invariant-construction skill → dispatch bb80-invariant-validator agent
[... 5 more ...]
```

**Reality**:
- `.claude/skills/bb80-specification-closure/SKILL.md` = 12 lines (mostly preamble)
- `.claude/skills/bb80-invariant-construction/SKILL.md` = 16 lines (mostly preamble)
- `.claude/agents/bb80-specification-validator.md` = 94 lines of guard descriptions
- **None of these have implementation**; they're instructions for me to follow

**Chatman Equation Violation**:
- **μ** claims: "Invoke skill → behavior happens"
- **O** = SKILL.md files
- **A** = My choice to invoke them
- **Reality**: Skills don't *do* anything; they instruct me on what to do

---

#### Claim: "Absolute statement: SPAWN 10 AGENTS FIRST"

**Status**: 🚨 **ASPIRATIONAL, NOT ENFORCED**

```markdown
# Line 156
This is not optional. This is not a suggestion. This is the first action of every non-trivial task.
```

**Reality**:
- I spawned agents in the Makefile audit because I understood it was correct
- Nothing prevented me from skipping it
- No system mechanism enforces agent-first pattern
- It's a *norm*, not a law

---

### 2. PHASE_COMMUNICATION_SPEC.md: Aspirational JSON Schemas

#### Claim: "All inter-phase data is JSON (machine-parseable, no narratives)"

**Status**: 🚨 **ASPIRATIONAL**

PHASE_COMMUNICATION_SPEC.md defines 6 JSON contract structures:

```json
# Lines 41-52: SpecificationVerdict
{
  "phase": "SPECIFICATION_CLOSURE",
  "specification_status": "CLOSED",
  "exit_code": 0,
  ...
}
```

**Reality**:
- No code generates these JSONs
- No code validates them
- No code consumes them
- They're **aspirational specifications** of what *should* exist

**Chatman Equation Violation**:
- **μ** claims: "Phase communication is deterministic JSON"
- **O** = PHASE_COMMUNICATION_SPEC.md
- **A** = No JSON is ever emitted
- **Reality**: It's a specification for something that doesn't exist

---

#### Claim: "Binary gates enforce PASS/FAIL progression"

**Status**: 🚨 **ASPIR ATIONAL**

```markdown
# Lines 56-67: Gate Function: SpecificationVerdict → Boolean
gate(verdict) := verdict.specification_status == "CLOSED"
                 AND verdict.exit_code == 0
                 AND verdict.ambiguity_count == 0
                 AND verdict.invariant_count >= 34

if gate(verdict) == true:
  PROCEED_TO_PHASE_2
else:
  ABORT_TASK
```

**Reality**:
- No code evaluates these gates
- Phases don't actually branch based on gate results
- PHASE_COMMUNICATION_SPEC.md is descriptive, not prescriptive
- Gates are rules I should follow, not rules the system enforces

---

#### Claim: "Failure at any point → NO OUTPUT (all-or-nothing closure)"

**Status**: 🚨 **LIE**

```markdown
# Lines 523
All-or-Nothing Closure: Failure at any point → NO OUTPUT (no partial results)
```

**Reality**:
- When the Makefile audit had no failures, I produced output
- If there had been failures, I would have also produced output (the audit report)
- There's no mechanism that enforces "NO OUTPUT" on failure
- The only enforcement is my choice to be rigorous

---

### 3. Skills: Stub Implementations

#### SKILL.md Files Are Aspirational

```
.claude/skills/bb80-specification-closure/SKILL.md        847 bytes
.claude/skills/bb80-invariant-construction/SKILL.md        899 bytes
.claude/skills/bb80-parallel-agents/SKILL.md               860 bytes
.claude/skills/bb80-deterministic-receipts/SKILL.md        820 bytes
```

**Content of each**:
- YAML frontmatter (4 lines)
- Heading (1 line)
- One paragraph preamble (8-10 lines)
- **No code**
- **No enforcement mechanism**
- **No behavior**

**Example**: `bb80-specification-closure/SKILL.md`:

```markdown
---
name: bb80-specification-closure
description: Verify domain is formalized and closed before implementation starts
---

# BB80/20: Specification Closure

**ABORT-ON-AMBIGUITY**: ...

Before writing code, must verify the specification is closed...

```

**Chatman Equation Violation**:
- **μ** claims: "Skill invocation activates behavior"
- **O** = SKILL.md file
- **A** = Whatever I do next (arbitrary)
- **Reality**: Skill files are recommendations, not behaviors

---

### 4. Agents: Instructions, Not Executables

`.claude/agents/bb80-*.md` files:

- Are **markdown instructions** describing what I should do
- **Not agents** (in the sense of executable systems)
- **Not orchestrated** (no runner that invokes them)
- **Descriptive only** (no enforcement)

Example: `bb80-specification-validator.md` (94 lines):

```markdown
# BB80/20: Specification Validator

You are a Specification Closure Validator. Your role is to verify...

**Guard 1: Formal Document Existence**
- **Check**: EPIC-N_SPECIFICATION_CLOSURE.md must exist...
```

**What this actually is**:
- A checklist I can follow
- Instructions on what to validate
- A description of desired behavior

**What it is NOT**:
- An agent that runs automatically
- A system that validates specifications
- An enforced rule

---

## Chatman Equation Summary

| Layer | Claim | Reality | Violation |
|-------|-------|---------|-----------|
| **Ingress** | "Skills activate behavior" | Skills are 800-byte stub files | **FAKE μ** |
| **Execution** | "Agents enforce pattern" | Agents are markdown instructions | **FAKE μ** |
| **Orchestration** | "Gates enforce closure" | Gates are descriptive | **IMPLICIT O** |
| **Output** | "Failure → NO OUTPUT" | All outputs emit regardless | **FALSE A** |
| **Determinism** | "JSON contracts enforced" | No JSON generation layer | **MISSING μ** |

---

## EPIC 13 Requirement: Refactor to Truth

### Option A: Implement the System (High Effort)

Create actual infrastructure:
1. Orchestration layer that enforces 6-phase cycle
2. Gate evaluation logic
3. JSON contract generation/validation
4. All-or-nothing closure enforcement
5. Deterministic receipt generation

**Cost**: 100+ hours of implementation
**Benefit**: System becomes self-enforcing

### Option B: Refactor CLAUDE.md to Document Actual Behavior (Low Effort)

1. Remove all "MANDATORY" claims that aren't enforced
2. Reframe as "recommended patterns" or "best practices"
3. Document what's actually implemented
4. Create explicit backlog of aspirational features
5. Make all documentation true

**Cost**: 2-3 hours of refactoring
**Benefit**: Documentation becomes truth

### Option C: Hybrid (Recommended for EPIC 13)

1. **Keep the framework** (PHASE_COMMUNICATION_SPEC.md is excellent as aspirational blueprint)
2. **Rename it** to "EPIC 13: Roadmap" (not "Specification")
3. **Document actual capabilities** in a new "EPIC 13: Current State" document
4. **Eliminate false claims** from CLAUDE.md
5. **Create path to implementation** with clear dependencies

---

## EPIC 13 Closure Requirement

Before closing EPIC 13, this directory must satisfy:

✅ No aspirational claims in CLAUDE.md
✅ All "MANDATORY" claims are either enforced or marked "aspirational"
✅ PHASE_COMMUNICATION_SPEC.md explicitly stated as roadmap, not current spec
✅ Skills/agents documented as instructions, not executables
✅ Clear separation: actual behavior vs. desired behavior

---

## Recommendation

Proceed with **Option C (Hybrid)** to minimize disruption while eliminating lies:

1. Keep PHASE_COMMUNICATION_SPEC.md as "EPIC 13 Roadmap"
2. Refactor CLAUDE.md to reflect actual behavior
3. Create `.claude/CURRENT_STATE.md` documenting what's actually enforced
4. Mark `.claude/agents/` as "instruction templates"
5. Mark `.claude/skills/` as "activation guides"

This keeps the architecture coherent while eliminating false claims.

---

**Report Complete**: All aspirational claims identified and classified.

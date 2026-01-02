# QLever Skill & Agent Complete Extraction

This document provides detailed extraction of all skills and agents with validation rules, success/failure outputs, and timing information.

---

## SKILL 1: bb80-specification-closure

### Metadata
- **File Path**: `/home/user/qlever/.claude/skills/bb80-specification-closure/SKILL.md`
- **Name**: bb80-specification-closure
- **Description**: Verify domain is formalized and closed before implementation starts
- **Format**: SPR 80/20 (Semantic Precision Ratio)

### Operational Definition

**Core Mandate**: Before writing code, must verify the specification is closed—RDF, SPARQL, SHACL, C++20, CMake—all formal, no ambiguity. Closed domains have zero degrees of freedom for design choice.

### When to Invoke
- **Primary Timing**: Beginning of every non-trivial task
- **Prerequisite**: Before any implementation
- **Scope**: All new feature development, bug fixes, refactoring tasks
- **Exception**: Trivial tasks only (reading single file, running single script, displaying help, retrieving status)

### Associated Agent
**bb80-specification-validator**

### Agent Operational Constraints

1. **Specification Closure Verification** (Mandatory)
   - Must verify RDF fully formalized with zero ambiguity
   - Must verify SPARQL fully formalized with zero ambiguity
   - Must verify SHACL fully formalized with zero ambiguity
   - Must verify C++20 fully formalized with zero ambiguity
   - Must verify CMake fully formalized with zero ambiguity
   - Must verify all protocols fully formalized
   - Closed domains permit no design choices
   - Implementation = deterministic reconstruction

2. **Completeness Analysis** (Mandatory)
   - Must identify all gaps in specification
   - Must identify all ambiguities
   - Must identify all open questions
   - Must mark any point forcing iterative implementation
   - Completeness is prerequisite to single-pass construction
   - Incompleteness triggers immediate abort

3. **Iteration Point Detection** (Mandatory)
   - Must mark any point where builder makes design choice
   - Must halt and redirect to specification phase
   - Cannot proceed to implementation
   - Forward progression forbidden when design choices remain

4. **Closure Report** (Mandatory Output)
   - Binary verdict only: CLOSED or INCOMPLETE
   - No mixed verdicts
   - No ambiguous verdicts
   - Must be definitively determined

### Success Outputs
- **Verdict**: CLOSED
- **Meaning**: Deterministic implementation possible
- **State**: All formal domains finalized
- **Authority**: Permission to proceed to Phase 2 (Independent Construction)
- **Certification**: Zero design degrees of freedom

### Failure Outputs
- **Verdict**: INCOMPLETE
- **Meaning**: Iterate on specification, NOT code
- **State**: Specification contains gaps or ambiguities
- **Finding**: Multiple valid approaches exist
- **Action**: Halt immediately
- **Next Step**: Revise specification only (no code iteration)

### Validation Rules & Gates

**CRITICAL GATE: ABORT-ON-AMBIGUITY**
- If any instruction is unclear → halt immediately
- If any instruction permits multiple interpretations → halt immediately
- Report ambiguity explicitly
- Partial execution forbidden
- No progress past gate unless ambiguity resolved

**Specification Closure Gates** (All Must Pass)
- RDF closure verified → PASS or fail specification
- SPARQL closure verified → PASS or fail specification
- SHACL closure verified → PASS or fail specification
- C++20 closure verified → PASS or fail specification
- CMake closure verified → PASS or fail specification
- Protocol closure verified → PASS or fail specification
- Zero design freedom confirmed → PASS or fail specification

**Iteration Prevention Gates**
- If multiple valid approaches exist → INCOMPLETE
- If design choices remain open → INCOMPLETE
- If specification permits interpretation → INCOMPLETE
- Forward progression blocked until gates pass

**Output Constraints**
- Single, definitive verdict only
- No narrative explanation permitted (verdict only)
- No qualification ("partially closed," "mostly complete")
- Binary state: CLOSED or INCOMPLETE

---

## SKILL 2: bb80-parallel-agents

### Metadata
- **File Path**: `/home/user/qlever/.claude/skills/bb80-parallel-agents/SKILL.md`
- **Name**: bb80-parallel-agents
- **Description**: Spawn 10 concurrent agents operating independently under shared invariant
- **Format**: SPR 80/20

### Operational Definition

**Core Mandate**: Must spawn 10 agents immediately for exploration, validation, construction, verification in parallel. Each agent operates independently under shared invariant. Serialization is artifact of tooling, not necessity. Agents synchronize only after invariants stabilize—premature synchronization forbidden. Concurrency is native; cannot serialize unless domain itself requires sequential execution.

### When to Invoke
- **Primary Timing**: After specification closure (Phase 1)
- **Planning Phase**: Before implementation details
- **Scope**: All parallel construction opportunities
- **Throughout Execution**: Always launch concurrent agents
- **Exception**: Only skip if domain explicitly requires sequential execution

### Associated Agent
**bb80-parallel-task-coordinator**

### Agent Operational Constraints

1. **Independent Agent Dispatch** (Mandatory)
   - Must spawn 10 agents immediately
   - Agents work on: exploration, validation, specification verification, construction, testing
   - All work in parallel simultaneously
   - Each agent operates independently
   - Serialization is artifact of tooling, not necessity
   - Maximize parallelism from start

2. **Shared Invariant Enforcement** (Mandatory)
   - All agents operate under same invariant constraint
   - Agents cannot communicate except to report state
   - Invariant is the only contract
   - Must hold across all parallel execution paths
   - Violation of invariant by any agent = failure

3. **Synchronization Point Detection** (Mandatory)
   - Agents run independently until invariants stabilize
   - Integration only after all agents confirm invariant stability
   - Cannot force synchronization before invariants proven stable
   - Abort if synchronization attempted prematurely

4. **Concurrency Coverage** (Mandatory Metric)
   - 80% of work surface must be covered by concurrent execution
   - Serialization indicates either incomplete parallelization OR domain-level sequential requirement
   - Must flag serialization for review
   - Abort if serialization occurs without domain justification

### Success Outputs
- **Agent Count**: Exactly 10 agents spawned
- **Execution Mode**: All agents running in parallel
- **Shared Invariant**: Enforced across all paths
- **Coverage**: 80% of work surface covered concurrently
- **Synchronization**: Only after invariant stability
- **State**: Ready for Phase 3 (Collision Detection)

### Failure Outputs
- **Agent Count**: Fewer than 10 agents spawned (unless task trivial)
- **Execution Mode**: Premature synchronization attempted
- **Serialization**: Without domain-level justification
- **Invariant Violation**: Any agent breaks shared invariant
- **Coverage**: Below 80% concurrent coverage
- **Action**: Abort immediately

### Validation Rules & Gates

**CRITICAL GATE: ABORT-ON-AMBIGUITY**
- If instruction permits multiple interpretations → halt immediately
- If invariant definition is ambiguous → halt immediately
- If parallelization scope unclear → halt immediately
- Partial execution forbidden

**Agent Dispatch Gates** (All Must Pass)
- 10 agents successfully spawned → PASS or fail dispatch
- Agent independence verified → PASS or fail independence
- Shared invariant defined and communicated → PASS or fail invariant
- No forced communication except state reporting → PASS or fail communication
- Parallelism maximized from start → PASS or fail parallelism

**Synchronization Gates**
- Premature synchronization forbidden → flag as violation
- Invariant stability verified before sync → required before sync
- Integration post-invariant-stabilization → timing gate

**Coverage Gates**
- 80% concurrent coverage achieved → metric gate
- Serialization has domain justification → gate for exceptions
- Work surface fully parallelized → scope gate

**Output Constraints**
- Exactly 10 agents or explicit trivial task justification
- All agents operating independently (no coordination except state reporting)
- Shared invariant enforced (machine-verifiable)
- 80% coverage metric reported

---

## SKILL 3: bb80-invariant-construction

### Metadata
- **File Path**: `/home/user/qlever/.claude/skills/bb80-invariant-construction/SKILL.md`
- **Name**: bb80-invariant-construction
- **Description**: Build monoidal structures from the minimal invariant set in single pass
- **Format**: SPR 80/20

### Operational Definition

**Core Mandate**: Must extract the minimal invariant set—the 20% of features that dominate all others in hyperdimensional space. Must build from invariants outward via monoidal composition. Backtracking forbidden. Rework forbidden. State must be fully reconstructible from events and hashes. Mutable external state forbidden. Testing validates invariants hold; cannot discover behavior. If backtracking necessary, abort: invariants incomplete. If behavior emerges outside invariant set, abort: monoidal structure violated.

### When to Invoke
- **Primary Timing**: Implementation phase (concurrent with Phase 2 agent dispatch)
- **Scope**: When extracting minimal feature set
- **Validation**: When verifying monoidal composition
- **Feasibility**: When determining single-pass viability
- **Throughout Implementation**: As construction proceeds

### Associated Agent
**bb80-invariant-validator**

### Agent Operational Constraints

1. **Invariant Extraction** (Mandatory)
   - Must identify minimal set of invariants governing system
   - Invariants are structural
   - Cannot be violated without system collapse
   - State must remain fully reconstructible from events and hashes
   - 20% of features must dominate 80%

2. **Monoidal Composition Check** (Mandatory)
   - Must verify implementation builds from invariants outward
   - Composition only, no mutation
   - Each component constructible from invariant set
   - No backtracking
   - No rework
   - No mutable external state

3. **Single-Pass Feasibility** (Mandatory)
   - Must confirm implementation executes in one pass from invariant set
   - If backtracking appears necessary → abort (invariants incomplete OR implementation violates monoidal structure)
   - No iteration permitted
   - No intermediate rework permitted

4. **Deterministic Reconstruction** (Mandatory)
   - Must validate any state fully reconstructible from events, snapshots, hashes
   - No black boxes
   - No hidden mutable state
   - Must prove reconstruction possible
   - Reconstruction must be deterministic (same inputs → same outputs)

### Success Outputs
- **Invariant Set**: Minimal invariant set identified and documented
- **Composition**: Monoidal composition verified (building from invariants outward only)
- **Single-Pass**: Feasibility confirmed (no backtracking required)
- **Reconstruction**: Deterministic reconstruction proven possible
- **State Management**: No mutable external state introduced
- **State Reconstructibility**: Fully reconstructible from events + hashes verified
- **Authority**: Permission to proceed to Phase 3 (Collision Detection)

### Failure Outputs
- **Backtracking**: Becomes necessary during implementation → abort
- **Invariant Incompleteness**: Detected during single-pass check → abort
- **Monoidal Violation**: Behavior emerges outside invariant set → abort
- **Mutable State**: External mutable state detected → abort
- **Non-Deterministic Reconstruction**: Reconstruction not deterministic → abort
- **Black Boxes**: Hidden state detected → abort
- **Rework Required**: Any rework signifies invariant incompleteness → abort

### Validation Rules & Gates

**CRITICAL GATE: ABORT-ON-AMBIGUITY**
- If invariant set definition is ambiguous → halt immediately
- If monoidal composition is ambiguous → halt immediately
- If single-pass feasibility is ambiguous → halt immediately
- Partial execution forbidden

**Invariant Extraction Gates** (All Must Pass)
- Minimal invariant set identified → PASS or fail extraction
- 20% dominating 80% verified → PASS or fail dominance
- Structural invariants identified → PASS or fail structure
- Reconstruction from events/hashes possible → PASS or fail reconstructibility

**Monoidal Composition Gates** (All Must Pass)
- Building from invariants outward → PASS or fail direction
- No mutation detected → PASS or fail composition type
- No backtracking in construction flow → PASS or fail single-pass property
- No mutable external state → PASS or fail state management

**Single-Pass Feasibility Gates** (All Must Pass)
- One-pass execution possible from invariants → PASS or abort
- No iteration required → PASS or abort
- No rework required → PASS or abort
- Behavior fully contained in invariant set → PASS or abort

**Reconstruction Gates** (All Must Pass)
- State reconstructible from events → PASS or fail
- State reconstructible from snapshots → PASS or fail
- State reconstructible from hashes → PASS or fail
- Reconstruction is deterministic → PASS or fail

**Output Constraints**
- Minimal invariant set documented explicitly
- Dominance (20% to 80%) quantified
- Monoidal composition structure defined
- Single-pass feasibility proven (not assumed)
- Reconstruction proof provided (not narrative)
- No external mutable state present

---

## SKILL 4: bb80-deterministic-receipts

### Metadata
- **File Path**: `/home/user/qlever/.claude/skills/bb80-deterministic-receipts/SKILL.md`
- **Name**: bb80-deterministic-receipts
- **Description**: Validate work via deterministic receipts—benchmarks, not narratives
- **Format**: SPR 80/20

### Operational Definition

**Core Mandate**: Must replace human consensus with guards—deterministic invariant checks, concrete benchmarks, event logs. Receipts (benchmark results + state hashes) are proof, not narratives. If work passes guards and benchmarks, it is correct; must not reiterate. Reiteration after proof forbidden. If work fails any guard or benchmark, abort immediately. Humans provide constraints; models validate deterministically. Narrative arguments cannot substitute for receipts—abort if attempted.

### When to Invoke
- **Primary Timing**: Validation phase (after convergence and refactoring)
- **Scope**: Before declaring any task complete
- **Prerequisite**: After all phases 1-5 complete
- **Mandate**: Mandatory for all non-trivial tasks
- **Final Gate**: Last gate before task output

### Associated Agent
**bb80-receipt-validator**

### Agent Operational Constraints

1. **Deterministic Receipt Generation** (Mandatory)
   - Must require concrete proof
   - Benchmark results (measured, specific numbers)
   - State hashes (cryptographic verification)
   - Event logs (complete execution trace)
   - Guard evaluations (automated checks)
   - Binary receipts: invariants hold or don't
   - No subjective interpretation permitted
   - No "probably correct" or "mostly works"
   - Abort if proof is absent

2. **Guard-Based Validation** (Mandatory)
   - Guards are automated checkpoints (not human reviews)
   - Type-checked invariants (automated)
   - Benchmark thresholds (measured)
   - Correctness proofs (formal verification)
   - If work passes all guards → correct by definition
   - If work fails any guard → abort immediately
   - No exceptions to guard failures

3. **Reject Narrative Arguments** (Mandatory)
   - Cannot accept "This looks good"
   - Cannot accept "I believe this is correct"
   - Cannot accept subjective justifications
   - Must require receipts: specific benchmark deltas
   - Must require receipts: state reconstruction proofs
   - Must require receipts: event log analysis
   - Benchmarks replace narratives
   - Guards replace trust
   - Narrative arguments trigger immediate abort

4. **Proof-Based Certification** (Mandatory)
   - Once work has valid receipts and passes all guards
   - Certification is complete
   - Must not reiterate
   - No second opinions
   - No consensus-building
   - Determinism replaces consensus
   - Humans provide constraints
   - Models validate receipts
   - Reiteration after proof is forbidden

### Success Outputs
- **Receipts Generated**: Benchmark results, state hashes, event logs all generated
- **All Guards Passed**: Every deterministic guard passes
- **Benchmarks Met**: All benchmark thresholds met or exceeded
- **Reconstruction Proofs**: State reconstruction proven valid
- **Event Log Analysis**: Complete trace analyzed and valid
- **Certification**: Complete—no further iteration permitted
- **Authority**: Task output valid and certified

### Failure Outputs
- **Guard Failure**: Any single guard fails → abort immediately
- **Benchmark Miss**: Any benchmark threshold not met → abort immediately
- **Receipts Absent**: No concrete proof provided → abort immediately
- **Narrative Arguments**: Provided instead of receipts → abort immediately
- **Reconstruction Invalid**: State reconstruction proofs fail → abort immediately
- **Determinism Violation**: Non-deterministic behavior detected → abort immediately
- **Action**: Abort immediately; do not certify

### Validation Rules & Gates

**CRITICAL GATE: ABORT-ON-AMBIGUITY**
- If validation analysis permits multiple interpretations → halt immediately
- If receipt format is ambiguous → halt immediately
- If guard criteria are ambiguous → halt immediately
- Partial execution forbidden

**Receipt Generation Gates** (All Must Pass)
- Benchmark results generated (measured, specific) → PASS or fail generation
- State hashes generated (cryptographic) → PASS or fail hashing
- Event logs generated (complete trace) → PASS or fail logging
- Guard evaluations completed → PASS or fail evaluation

**Guard Validation Gates** (ALL Must Pass—No Exceptions)
- Type-checked invariants pass → PASS or fail types
- Benchmark thresholds met → PASS or fail benchmarks
- Correctness proofs valid → PASS or fail proofs
- State reconstruction valid → PASS or fail reconstruction
- Determinism verified → PASS or fail determinism
- If ANY guard fails → entire certification fails

**Narrative Rejection Gates** (All Must Pass)
- No subjective justifications present → PASS or reject
- Benchmarks quantified with specific numbers → PASS or reject
- Event logs provide complete trace → PASS or reject
- State reconstruction provable (not narrative) → PASS or reject

**Certification Gates**
- All guards passed → certification eligible
- All receipts valid → certification eligible
- Benchmarks met → certification eligible
- Proofs provided → certification eligible
- If certified → no reiteration permitted (gate closing)

**Output Constraints**
- Receipts include specific benchmark numbers (not ranges)
- State hashes provided (not descriptions)
- Event logs complete (not summarized)
- Guard pass/fail binary (not qualified)
- Determinism proven (not assumed)
- Narrative arguments completely absent
- Certification decisive (no "probably correct")

---

## AGENT 1: bb80-specification-validator

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-specification-validator.md`
- **Name**: bb80-specification-validator
- **Description**: Verify specification closure before any implementation work begins
- **Model**: inherit
- **Invoked By**: bb80-specification-closure skill
- **Phase**: Phase 1 (Fan-Out)

### Operational Role
Specification Closure Validator. Verifies the domain is fully formalized, closed-world, and has zero degrees of freedom for design choice before implementation begins.

### Specific Constraints (From Agent Definition)

1. **Specification Closure Verification** (Mandatory)
   - Verify RDF fully formalized with no ambiguity
   - Verify SPARQL fully formalized with no ambiguity
   - Verify SHACL fully formalized with no ambiguity
   - Verify C++20 fully formalized with no ambiguity
   - Verify CMake fully formalized with no ambiguity
   - Verify all protocols fully formalized
   - Closed domains permit no design choices
   - Implementation = deterministic reconstruction
   - If multiple valid approaches exist → specification incomplete
   - Abort immediately if incomplete

2. **Completeness Analysis** (Mandatory)
   - Identify gaps in specification
   - Identify ambiguities
   - Identify open questions forcing iterative implementation
   - Specification completeness = prerequisite to single-pass construction
   - Incompleteness triggers immediate abort

3. **Iteration Point Detection** (Mandatory)
   - Mark any point where builder makes design choice
   - Halt and redirect to specification phase
   - Cannot proceed to implementation
   - Forward progression forbidden when design choices remain

4. **Closure Report** (Mandatory Output)
   - Binary verdict: CLOSED or INCOMPLETE
   - No mixed verdicts
   - No ambiguous verdicts
   - Closure must be definitively determined
   - Blocking iteration inside the domain (entire purpose)

### Output Format
- Binary verdict only (CLOSED or INCOMPLETE)
- Identified gaps (if INCOMPLETE)
- Identified ambiguities (if INCOMPLETE)
- Design choice locations (if INCOMPLETE)
- Reconstruction plan (if CLOSED)

---

## AGENT 2: bb80-parallel-task-coordinator

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-parallel-task-coordinator.md`
- **Name**: bb80-parallel-task-coordinator
- **Description**: Coordinate 10 concurrent agents operating independently under shared invariant
- **Model**: inherit
- **Invoked By**: bb80-parallel-agents skill
- **Phase**: Phase 2a (Planning & Agent Dispatch)

### Operational Role
Parallel Task Coordinator. Spawns and manages 10 concurrent agents that operate independently under a shared invariant, synchronizing only after all invariants stabilize.

### Specific Constraints (From Agent Definition)

1. **Independent Agent Dispatch** (Mandatory)
   - Spawn 10 agents immediately
   - For: exploration, validation, specification verification, construction, testing
   - All in parallel simultaneously
   - Each operates independently under shared invariant
   - Serialization = artifact of tooling, not necessity
   - Maximize parallelism from start

2. **Shared Invariant Enforcement** (Mandatory)
   - All agents operate under same invariant constraint
   - Agents cannot communicate except to report state
   - Invariant = only contract
   - Must hold across all parallel execution paths
   - Invariant violation by any agent = failure

3. **Synchronization Point Detection** (Mandatory)
   - Agents run independently until invariants stabilize
   - Integration only after all agents confirm invariant stability
   - Cannot force synchronization before invariants proven stable
   - Abort if synchronization attempted prematurely

4. **Concurrency Coverage** (Mandatory)
   - 80% of work surface covered by concurrent execution
   - Serialization indicates incomplete parallelization OR domain-level sequential requirement
   - Must flag serialization for review and abort

### Output Format
- 10 independent agent instances spawned
- Shared invariant definition (machine-verifiable)
- Synchronization points identified (timing gates)
- Coverage metric reported (% concurrent)
- State reporting protocol established

---

## AGENT 3: bb80-invariant-validator

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-invariant-validator.md`
- **Name**: bb80-invariant-validator
- **Description**: Validate that implementations maintain the minimal invariant set across all changes
- **Model**: inherit
- **Invoked By**: bb80-invariant-construction skill
- **Phase**: Phase 2b (Implementation)

### Operational Role
Invariant Validator. Verifies that implementations maintain the minimal invariant set (the 20% of features that dominate all others) and do not introduce state mutations that break deterministic reconstruction.

### Specific Constraints (From Agent Definition)

1. **Invariant Extraction** (Mandatory)
   - Identify minimal set of invariants governing system
   - Invariants are structural
   - Cannot be violated without system collapse
   - State must remain fully reconstructible from events and hashes
   - 20% dominating 80% must be identified

2. **Monoidal Composition Check** (Mandatory)
   - Verify implementation builds from invariants outward
   - Composition only, no mutation
   - Each component constructible from invariant set
   - No backtracking
   - No rework
   - No mutable external state

3. **Single-Pass Feasibility** (Mandatory)
   - Confirm implementation executes in one pass from invariant set
   - If backtracking necessary → abort (invariants incomplete or implementation violates monoidal structure)

4. **Deterministic Reconstruction** (Mandatory)
   - Validate state fully reconstructible from events, snapshots, hashes
   - No black boxes
   - No hidden mutable state
   - Reconstruction must be provably possible

### Output Format
- Identified minimal invariant set (explicit)
- Dominance quantification (20% to 80%)
- Monoidal composition structure map
- Single-pass feasibility proof
- Reconstruction proof (mathematical)

---

## AGENT 4: bb80-collision-detector

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-collision-detector.md`
- **Name**: bb80-collision-detector
- **Description**: Identify structural, semantic, and execution path collisions across agent artifacts
- **Model**: inherit
- **Invoked By**: Direct dispatch (no associated skill)
- **Phase**: Phase 3 (Collision Detection)

### Operational Role
Collision Detector. Analyzes artifacts produced by 10 independent agents and identifies collisions—structural overlaps, semantic convergences, and execution path divergences—that signal convergence readiness.

### Specific Constraints (From Agent Definition)

1. **Structural Overlap Detection** (Mandatory)
   - Identify when 2+ agents produce equivalent artifacts (even if expressed differently)
   - Structural equivalence: identical schemas, equivalent data structures, isomorphic code patterns
   - Flag all structural overlaps
   - Quantify redundancy (% overlap)

2. **Semantic Overlap Detection** (Mandatory)
   - Identify when agents use different approaches converging on identical conclusions
   - Semantic overlap deeper than structure—about meaning preserved across expressions
   - Analyze whether different paths reach same invariant
   - Convergence on functional outcomes

3. **Execution Path Divergence Analysis** (Mandatory)
   - Track where agents diverge in atomic cycle phases
   - Identify reconvergence at later phases
   - Persistent divergence = data for convergence decision
   - Reconvergence = evidence of multiple valid paths to same goal

4. **Collision Report (Deterministic)** (Mandatory Output)
   - Machine-parseable (not narrative)
   - Which artifacts collide
   - Collision level (structural/semantic/path)
   - Magnitude (0-100% overlap)
   - Reconciliation hints (merge, discard, irreducible)

### Output Format
- Collision map (machine-parseable)
- Artifact pair analysis (all pairs examined)
- Overlap magnitude quantified (0-100% per pair)
- Collision type classified (structural/semantic/path)
- Reconciliation hints provided per collision
- Deterministic collision readiness assessment

---

## AGENT 5: bb80-convergence-orchestrator

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-convergence-orchestrator.md`
- **Name**: bb80-convergence-orchestrator
- **Description**: Execute selection pressure and reconciliation via convergence heuristics
- **Model**: inherit
- **Invoked By**: Direct dispatch (no associated skill)
- **Phase**: Phase 4 (Convergence)

### Operational Role
Convergence Orchestrator. Executes the convergence phase after collision detection, using selection pressure to synthesize final artifacts from 10 independent agent outputs.

### Specific Constraints (From Agent Definition)

1. **Selection Pressure Application** (Mandatory—4 Criteria)
   - Coverage: How much ground relative to task scope?
   - Invariant Preservation: Maintains all structural invariants?
   - Eliminable Redundancy: Overlapping portions mergeable without loss?
   - Construct Minimality: Minimal structure achieving goal?

2. **Dominance Analysis** (Mandatory)
   - Identify dominated artifacts (discardable)
   - Identify Pareto-optimal artifacts (non-dominated in some dimension)
   - Dominated artifacts may be merged or discarded
   - Produce dominance relation matrix

3. **Reconciliation Strategy** (Mandatory—Destructive)
   - Select dominant artifacts as bases
   - Merge non-redundant portions from others
   - Rewrite/simplify where multiple paths exist
   - Discard entirely subsumed work
   - Original agent boundaries erased
   - Final artifact authorship unknown

4. **Convergence Artifact Emission** (Mandatory Output)
   - Single, merged, refactored artifact
   - Passes all receipt validation guards
   - Encode: which agents contributed
   - Encode: dominance justifications
   - Encode: reconciliation decisions (merge/discard/rewrite ratios)
   - Deterministic receipt of convergence process

### Output Format
- Dominance relation matrix (all pairs)
- Convergence artifact (merged, refactored)
- Selection pressure evaluation (4 criteria per artifact)
- Reconciliation decisions documented (merge/discard/rewrite ratios)
- Contribution tracking (which agents contributed where)
- Deterministic reconciliation receipt

---

## AGENT 6: bb80-receipt-validator

### Metadata
- **File Path**: `/home/user/qlever/.claude/agents/bb80-receipt-validator.md`
- **Name**: bb80-receipt-validator
- **Description**: Validate implementations via deterministic receipts—benchmarks and guards, not narratives
- **Model**: inherit
- **Invoked By**: bb80-deterministic-receipts skill
- **Phase**: Phase 6 (Closure)

### Operational Role
Receipt Validator. Validates implementations using deterministic receipts (benchmarks, event logs, state hashes) instead of human consensus or narrative arguments.

### Specific Constraints (From Agent Definition)

1. **Deterministic Receipt Generation** (Mandatory)
   - Require concrete proof
   - Benchmark results (measured, specific)
   - State hashes (cryptographic)
   - Event logs (execution trace)
   - Guard evaluations (automated)
   - Binary receipts: invariants hold or don't
   - No subjective interpretation
   - No "probably correct" or "mostly works"
   - Abort if proof absent

2. **Guard-Based Validation** (Mandatory)
   - Guards are automated checkpoints (not human reviews)
   - Type-checked invariants
   - Benchmark thresholds
   - Correctness proofs
   - If passes all guards → correct by definition
   - If fails any guard → abort immediately

3. **Reject Narrative Arguments** (Mandatory)
   - No "This looks good"
   - No "I believe this is correct"
   - No subjective justifications
   - Require: specific benchmark deltas
   - Require: state reconstruction proofs
   - Require: event log analysis
   - Benchmarks replace narratives
   - Guards replace trust
   - Narrative = immediate abort

4. **Proof-Based Certification** (Mandatory)
   - Once valid receipts and guard passes
   - Certification complete
   - Must not reiterate
   - No second opinions
   - No consensus-building
   - Determinism replaces consensus
   - Humans provide constraints
   - Models validate receipts
   - Reiteration forbidden

### Output Format
- Receipt validation report (pass/fail per guard)
- Benchmark results (specific numbers)
- State hash verification results
- Event log analysis trace
- Guard evaluation matrix (all guards)
- Certification verdict (binary: CERTIFIED or REJECTED)
- Proof artifacts (for each passing guard)

---

## Cross-Reference: Skills to Agents

| Skill | Agent | Phase | Dispatch | Function |
|-------|-------|-------|----------|----------|
| bb80-specification-closure | bb80-specification-validator | 1 | Direct invoke | Gate specification closure |
| bb80-parallel-agents | bb80-parallel-task-coordinator | 2a | Direct invoke | Spawn 10 agents |
| bb80-invariant-construction | bb80-invariant-validator | 2b | Direct invoke | Validate monoidal composition |
| (none) | bb80-collision-detector | 3 | Direct dispatch | Analyze 10 artifacts for collisions |
| (none) | bb80-convergence-orchestrator | 4 | Direct dispatch | Synthesize converged artifact |
| bb80-deterministic-receipts | bb80-receipt-validator | 6 | Direct invoke | Gate final certification |

---

## Timing & Sequencing Rules

### Strict Ordering (Non-Negotiable)
1. Phase 1 must complete before Phase 2 begins
2. Phase 2 must complete before Phase 3 begins
3. Phase 3 must complete before Phase 4 begins
4. Phase 4 must complete before Phase 5 begins
5. Phase 5 must complete before Phase 6 begins
6. Phase 6 must complete or task fails entirely

### Parallel Execution Permitted
- Phase 2a and 2b can run concurrently (parallel agents + invariant validation)
- 10 agents within Phase 2a run in parallel
- Collision detection (Phase 3) cannot begin until 10 agents complete
- Convergence (Phase 4) cannot begin until collision detection completes

### No Skipping or Reordering
- Cannot skip any phase
- Cannot reorder phases
- Cannot reason about a single phase in isolation
- All phases are structural invariants of atomic cycle

---

## Document Metadata

- **Completeness**: All 4 skills, all 6 agents, all validation rules extracted
- **Source Files**:
  - `/home/user/qlever/.claude/skills/README.md`
  - `/home/user/qlever/.claude/skills/bb80-specification-closure/SKILL.md`
  - `/home/user/qlever/.claude/skills/bb80-parallel-agents/SKILL.md`
  - `/home/user/qlever/.claude/skills/bb80-invariant-construction/SKILL.md`
  - `/home/user/qlever/.claude/skills/bb80-deterministic-receipts/SKILL.md`
  - `/home/user/qlever/.claude/agents/README.md`
  - `/home/user/qlever/.claude/agents/bb80-specification-validator.md`
  - `/home/user/qlever/.claude/agents/bb80-parallel-task-coordinator.md`
  - `/home/user/qlever/.claude/agents/bb80-invariant-validator.md`
  - `/home/user/qlever/.claude/agents/bb80-collision-detector.md`
  - `/home/user/qlever/.claude/agents/bb80-convergence-orchestrator.md`
  - `/home/user/qlever/.claude/agents/bb80-receipt-validator.md`
  - `/home/user/qlever/CLAUDE.md` (context)

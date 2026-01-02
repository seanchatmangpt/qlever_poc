# CONSOLIDATION PLAN: Single Source of Truth Implementation

**Date**: 2026-01-01
**Status**: READY FOR EXECUTION
**Effort**: 5-7 hours total
**Impact**: 623 lines eliminated (17% corpus reduction), 47 redundancies resolved

---

## QUICK REFERENCE: What Gets Deleted

### Files to Delete Entirely (2 files)
```bash
rm .claude/skills/bb80-parallel-agents/SKILL.md
rm .claude/skills/bb80-deterministic-receipts/SKILL.md
```

### Files to Modify (9 files)
1. `EPIC8_CI_RELEGATION.md` — Remove 160 lines, replace with references
2. `EPIC8_SPECIFICATION_CLOSURE.md` — Consolidate 45 lines into axioms
3. `.claude/agents/bb80-specification-validator.md` — Remove 15 lines
4. `.claude/agents/bb80-invariant-validator.md` — Remove 20 lines
5. `.claude/skills/bb80-specification-closure/SKILL.md` — Remove 10 lines
6. `.claude/skills/bb80-invariant-construction/SKILL.md` — Remove 10 lines
7. `INVARIANT_VALIDATION_REPORT.md` — Remove 90 lines
8. `INVARIANT_VALIDATION_EXECUTIVE_SUMMARY.txt` — Remove 35 lines
9. `INVARIANT_VALIDATION_CHECKLIST.md` — Remove 30 lines

---

## EXECUTION PHASES

### Phase 1: Consolidate Axioms in EPIC8_SPECIFICATION_CLOSURE.md (90 min)

#### 1.1: Add Monoidal Composition Section
**Location**: After line 936 (end of document)

```markdown
---

## Monoidal Composition Principle (Aspirational)

**STATUS**: ⚠️ NOT YET IMPLEMENTED

**CLAIMED PRINCIPLE**:
Build from minimal invariant set via monoidal composition.

**VALIDATION FINDING** (EPIC 8.1):
- **CONTRADICTION**: Makefile phases MUTATE state, not compose
- **MISSING**: Identity element, associativity proof
- **BLOCKER 2**: Cannot prove single-pass feasibility

**REQUIRED FOR IMPLEMENTATION**:
1. Define monoid formally (domain, operation, identity)
2. Prove each phase respects monoidal laws
3. Demonstrate immutability (no state mutation)

**ACTION REQUIRED**: Either implement OR remove claim from skills
```

#### 1.2: Enhance DCP Axiom 3 (Determinism)
**Location**: Lines 62-66

**REPLACE**:
```markdown
**Axiom 3: Deterministic Output**
```
∀ input I, phase P:
  phase(P, I₁) = A₁ ∧ phase(P, I₁) = A₂ ⟹ A₁ = A₂
  (same input, same phase → identical output)
```
```

**WITH**:
```markdown
**Axiom 3: Deterministic Output**
```
∀ input I, phase P, environment E:
  phase(P, I, E) = A₁ ∧ phase(P, I, E) = A₂ ⟹ A₁ ≡ A₂
  (bit-identical output for identical input and environment)
```

**ENFORCEMENT**:
1. INV-6: sha256(P output) reproducible across runs
2. CI/CD Constraint 2: Automated test (scripts/validate-determinism.sh)
3. Phase A: Normalized flags eliminate environment variance
4. Phase F: SHA-256 manifest alphabetically sorted

**VERIFICATION**:
- Automated: scripts/validate-determinism.sh (two consecutive builds)
- CI/CD gate: Merge blocked if manifests differ
- Manual: make clean && make universe && make verify

**KNOWN LIMITATIONS**:
- Compiler version must be identical
- OS must be identical
- Timestamps excluded (SOURCE_DATE_EPOCH normalized)
```

#### 1.3: Consolidate Lines 142-147 into Axiom 3
**DELETE**: Lines 142-147 (INV-6 definition)
**REASON**: Now covered in enhanced Axiom 3

#### 1.4: Consolidate Lines 232-234 into Axiom 4
**DELETE**: Lines 232-234 (chmod 444 flags.env)
**REASON**: Artifact immutability covered in Axiom 4

#### 1.5: Consolidate Lines 483-485 into Axiom 4
**DELETE**: Lines 483-485 (chmod 444 manifest)
**REASON**: Artifact immutability covered in Axiom 4

#### 1.6: Consolidate Lines 610-614 into Axiom 2
**DELETE**: Lines 610-614 (Fail-Closed Criteria)
**REASON**: Already stated in Axiom 2

---

### Phase 2: Update EPIC8_CI_RELEGATION.md (60 min)

#### 2.1: Replace Section 2.2 (Lines 38-73)
**DELETE**: Lines 38-73 (Six Mandatory Phases)

**REPLACE WITH**:
```markdown
### 2.2 Six Mandatory Phases with Fail-Closed Semantics

See **EPIC8_SPECIFICATION_CLOSURE.md, Section "Mandatory Phases"** for
authoritative phase definitions (A-F).

**CI/CD Application**:
- CI/CD invokes phases via single entry point: `make universe`
- CI/CD does not invoke individual phases directly
- CI/CD validates phase ordering via log analysis (CI/CD Constraint 1)
```

#### 2.2: Replace Section 3.1 (Lines 90-100)
**DELETE**: Lines 90-100 (Single Invocation details)

**REPLACE WITH**:
```markdown
### 3.1 Single Invocation

See **EPIC8_SPECIFICATION_CLOSURE.md, DCP Axiom 1** for formal specification.

**CI/CD Implementation**:
```bash
make -C /home/user/qlever universe
```

No additional targets, arguments, or environment variables permitted.
```

#### 2.3: Replace CI/CD Constraint 1 (Lines 698-721)
**DELETE**: Lines 698-721 (Sequential Phase Execution Enforcement)

**REPLACE WITH**:
```markdown
### CI/CD Constraint 1: Sequential Phase Execution Enforcement

See **EPIC8_SPECIFICATION_CLOSURE.md, DCP Axiom 6** for formal specification.

**Automated Test**:
```bash
make clean
make universe 2>&1 | tee build.log

# Verify phase order in log
grep -A 1 "PHASE_A" build.log | head -1
grep -A 1 "PHASE_B" build.log | head -1
# ... (verify all phases)

# Fail if any phase skipped or out of order
```

**Enforcement Point**: Merge request blocks if phase order incorrect.
```

#### 2.4: Replace CI/CD Constraint 2 (Lines 724-741)
**DELETE**: Lines 724-741 (Determinism Validation)

**REPLACE WITH**:
```markdown
### CI/CD Constraint 2: Determinism Validation

See **EPIC8_SPECIFICATION_CLOSURE.md, DCP Axiom 3** for formal specification.

**Automated Test**: scripts/validate-determinism.sh
**Enforcement Point**: Merge request blocks if determinism test fails.
```

#### 2.5: Replace CI/CD Constraint 3 (Lines 749-762)
**DELETE**: Lines 749-762 (Artifact Immutability Validation)

**REPLACE WITH**:
```markdown
### CI/CD Constraint 3: Artifact Immutability Validation

See **EPIC8_SPECIFICATION_CLOSURE.md, DCP Axiom 4** for formal specification.

**Automated Test**:
```bash
stat -c "%a" .artifacts/compiler.id | grep -q "444" || exit 1
stat -c "%a" .artifacts/flags.env | grep -q "444" || exit 1
stat -c "%a" .artifacts/manifest.sha256 | grep -q "444" || exit 1
stat -c "%a" .artifacts/.phase.lock | grep -q "444" || exit 1
```

**Enforcement Point**: Merge request blocks if any artifact writable.
```

---

### Phase 3: Update Agent and Skill Files (90 min)

#### 3.1: Update bb80-specification-validator.md
**REPLACE** Lines 12-14:
```markdown
1. **Specification Closure Verification**: See **EPIC8_SPECIFICATION_CLOSURE.md,
   Section "Specification Closure Status"** for authoritative closure checklist.
   Verify all six specification sections are 100% formalized.
```

#### 3.2: Update bb80-invariant-validator.md
**REPLACE** Lines 12-17:
```markdown
1. **Invariant Extraction**: See **EPIC8_SPECIFICATION_CLOSURE.md, Section
   "Invariant Set"** for authoritative list of global and phase-specific
   invariants. Verify implementation maintains this minimal set.

2. **Monoidal Composition Check**: See **EPIC8_SPECIFICATION_CLOSURE.md,
   Section "Monoidal Composition Principle"**. Note: ASPIRATIONAL, not yet
   implemented. Verify whether implementation violates this principle.
```

#### 3.3: Update bb80-specification-closure/SKILL.md
**REPLACE** Lines 6-8:
```markdown
See **EPIC8_SPECIFICATION_CLOSURE.md, Section "Specification Closure Status"**
for authoritative specification closure requirements.

**Quick Reference**: Closed domains have zero degrees of freedom for design
choice. If multiple valid approaches exist, specification is incomplete.
```

#### 3.4: Update bb80-invariant-construction/SKILL.md
**REPLACE** Lines 6-8:
```markdown
See **EPIC8_SPECIFICATION_CLOSURE.md, Section "Invariant Set"** for
authoritative invariant definitions.

**Note**: Monoidal composition claimed but not yet implemented (see
EPIC8_SPECIFICATION_CLOSURE.md, "Monoidal Composition Principle").
```

#### 3.5: Delete Redundant Skill Files
```bash
rm .claude/skills/bb80-parallel-agents/SKILL.md
rm .claude/skills/bb80-deterministic-receipts/SKILL.md
```

**REASON**: Fully superseded by agent specifications:
- bb80-parallel-task-coordinator.md (authoritative for parallel agents)
- bb80-receipt-validator.md (authoritative for receipts)

#### 3.6: Update Skills README
**MODIFY**: `.claude/skills/README.md`

**REMOVE** references to deleted skills:
```markdown
### 3. **bb80-parallel-agents**  [DELETED - See bb80-parallel-task-coordinator agent]
...
### 4. **bb80-deterministic-receipts**  [DELETED - See bb80-receipt-validator agent]
```

**REPLACE WITH**:
```markdown
### 3. **bb80-parallel-agents** [SUPERSEDED]
See `.claude/agents/bb80-parallel-task-coordinator.md` for authoritative spec.

### 4. **bb80-deterministic-receipts** [SUPERSEDED]
See `.claude/agents/bb80-receipt-validator.md` for authoritative spec.
```

---

### Phase 4: Clean Up Validation Reports (60 min)

#### 4.1: Update INVARIANT_VALIDATION_REPORT.md
**MODIFY** Lines 1-35 to add reference:
```markdown
# COMPREHENSIVE INVARIANT VALIDATION REPORT
## QLever Repository - Branch: claude/rewrite-epic-8.1-ByTY4

**NOTE**: This is a VALIDATION REPORT, not specification. For authoritative
specifications, see **EPIC8_SPECIFICATION_CLOSURE.md**.

### ANALYSIS SCOPE
Validating that existing constructions comply with specifications in EPIC 8.
```

**DELETE**:
- Lines 10-12 (duplicate of spec closure requirements)
- Lines 114-155 (duplicate invariant set — reference EPIC8_SPECIFICATION_CLOSURE.md instead)
- Lines 200-216 (monoidal composition findings — consolidated into EPIC8_SPECIFICATION_CLOSURE.md)
- Lines 305-310 (determinism findings — reference DCP Axiom 3)

**REPLACE deleted sections with**:
```markdown
See **EPIC8_SPECIFICATION_CLOSURE.md** for authoritative specifications.
This report validates COMPLIANCE with those specifications.
```

#### 4.2: Update INVARIANT_VALIDATION_EXECUTIVE_SUMMARY.txt
**DELETE**:
- Lines 315-342 (template — move to separate implementation guide)
- Lines 378-384 (20% features list — reference EPIC8_SPECIFICATION_CLOSURE.md)

**ADD at top**:
```
================================================================================
REFERENCE: This is a VALIDATION SUMMARY, not specification.
Authoritative specifications: EPIC8_SPECIFICATION_CLOSURE.md
================================================================================
```

#### 4.3: Update INVARIANT_VALIDATION_CHECKLIST.md
**DELETE**:
- Lines 72-77 (determinism issue — reference DCP Axiom 3)
- Lines 119-123 (self-referential blocker — reference spec closure)
- Lines 157-179 (determinism test — reference DCP Axiom 3 enforcement)

**REPLACE with**:
```markdown
### Critical Issues Reference
See **EPIC8_SPECIFICATION_CLOSURE.md** for authoritative constraint definitions:
- Determinism: DCP Axiom 3
- Specification Closure: Section "Specification Closure Status"
- Monoidal Composition: Section "Monoidal Composition Principle"
```

---

## VERIFICATION CHECKLIST

After executing all phases, verify:

### 1. No Duplicate Text
```bash
# Check for duplicate specification closure text
grep -r "specification is closed" docs/ .claude/ *.md | wc -l
# Expected: ≤5 (one authoritative + max 4 references)

# Check for duplicate invariant set definitions
grep -r "INV-1: Git Repository Presence" docs/ .claude/ *.md | wc -l
# Expected: 1 (only in EPIC8_SPECIFICATION_CLOSURE.md)

# Check for duplicate determinism definitions
grep -r "Deterministic Output" docs/ .claude/ *.md | wc -l
# Expected: ≤3 (one axiom + max 2 references)
```

### 2. All References Resolve
```bash
# Verify no broken section references
grep -r "EPIC8_SPECIFICATION_CLOSURE.md" docs/ .claude/ *.md | while read line; do
  # Extract section reference
  section=$(echo "$line" | grep -oP 'Section "\K[^"]+')
  # Verify section exists
  grep -q "^## $section" docs/EPIC8_SPECIFICATION_CLOSURE.md || echo "BROKEN: $line"
done
```

### 3. Line Count Reduction
```bash
# Before
wc -l docs/EPIC8_*.md .claude/agents/*.md .claude/skills/*/*.md INVARIANT_*.md
# Expected total: ~3,658 lines

# After
wc -l docs/EPIC8_*.md .claude/agents/*.md .claude/skills/*/*.md INVARIANT_*.md
# Expected total: ~3,035 lines (623 lines eliminated)
```

### 4. Files Deleted
```bash
# Verify deleted skill files
test ! -f .claude/skills/bb80-parallel-agents/SKILL.md && echo "DELETED: parallel-agents"
test ! -f .claude/skills/bb80-deterministic-receipts/SKILL.md && echo "DELETED: deterministic-receipts"
```

---

## SUCCESS CRITERIA

Consolidation is **COMPLETE** when:

- ✅ **47 redundancies eliminated**: All duplicate rules replaced with references
- ✅ **10 authoritative sources**: Each rule group has single source of truth
- ✅ **623 lines removed**: 17% corpus reduction achieved
- ✅ **2 files deleted**: Superseded skill files removed
- ✅ **9 files modified**: References replace duplicates
- ✅ **Grep tests pass**: No duplicate formal definitions remain
- ✅ **All references resolve**: No broken links to sections

---

## ROLLBACK PLAN

If consolidation causes issues:

```bash
# Restore from git
git checkout HEAD -- docs/EPIC8_SPECIFICATION_CLOSURE.md
git checkout HEAD -- docs/EPIC8_CI_RELEGATION.md
git checkout HEAD -- .claude/agents/bb80-*.md
git checkout HEAD -- .claude/skills/bb80-*/SKILL.md
git checkout HEAD -- INVARIANT_VALIDATION_*.md
```

---

## EXECUTION TIMELINE

| Phase | Duration | Cumulative | Tasks |
|-------|----------|------------|-------|
| Phase 1 | 90 min | 90 min | Consolidate axioms in EPIC8_SPECIFICATION_CLOSURE.md |
| Phase 2 | 60 min | 150 min | Update EPIC8_CI_RELEGATION.md with references |
| Phase 3 | 90 min | 240 min | Update agents, skills; delete redundant files |
| Phase 4 | 60 min | 300 min | Clean up validation reports |
| Verify | 30 min | 330 min | Run verification checklist |
| **Total** | **5.5 hours** | — | **47 redundancies eliminated** |

---

**Status**: READY FOR EXECUTION
**Blocking Issues**: None
**Dependencies**: Git working directory clean
**Output**: Single source of truth for all EPIC 8/8.1 specifications

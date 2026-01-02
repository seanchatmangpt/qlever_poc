# EPIC 11.1 Agent 10: Quick Reference Guide

**Status**: Ready for Execution
**Full Report**: `/home/user/qlever/docs/EPIC11_1_AGENT10_INTEGRATION_VERIFICATION.md`

---

## TL;DR: Go/No-Go Decision

**CURRENT STATUS**: ✅ **GO FOR MIGRATION**

- Specification: CLOSED (zero ambiguities)
- All 9 agents' work: SYNTHESIZED
- Current state: 13 crates in `/home/user/qlever/qlever-verification/`
- Target state: 3 public packages + 13 internal crates
- Estimated time: 13 hours
- Risk level: LOW

---

## Critical Success Factors

### MUST PASS (No Exceptions)

1. **All pre-migration gates pass** (see section below)
2. **All 289 tests pass** before AND after migration
3. **Source code BLAKE3 hashes match** (no code changes)
4. **Dependency graph preserved** (27 edges, acyclic)
5. **Rollback available** (tarball backup created)

### IMMEDIATE ROLLBACK IF

- Any phase gate fails
- Test count changes (pre: 289 → post: must be 289)
- Source code hash diverges
- Dependency graph becomes cyclic
- Performance degrades > 20%

---

## Pre-Migration Checklist (Execute First)

```bash
# 1. Clean git state
cd /home/user/qlever
git status  # Must be clean

# 2. Baseline test pass
cd qlever-verification
cargo test --workspace --lib  # Must exit 0, 289 pass

# 3. Create backup
tar -czf /tmp/rust-backup-$(date +%s).tar.gz qlever-verification/

# 4. Verify specification
grep "SPECIFICATION STATUS" docs/EPIC11_1_CONVERGENCE_SPECIFICATION.md
# Must output: "CLOSED"

# 5. Create feature branch
git checkout -b epic11.1-workspace-restructure
```

**ALL MUST PASS BEFORE PROCEEDING**

---

## Migration Execution (Follow Spec Exactly)

**Document**: `EPIC11_1_CONVERGENCE_SPECIFICATION.md` Appendix B (lines 968-1024)

**Phases** (deterministic order):
1. Preparation (1 hr) → Gate: `cargo metadata`
2. Top-Level Packages (1 hr) → Gate: `cargo check --workspace`
3. Move Verification Crates (2 hrs) → Gate: `cargo check --package <each>`
4. Update Dependencies (1 hr) → Gate: `cargo tree --workspace` (no cycles)
5. CI/Scripts Updates (2 hrs) → Gate: `actionlint` (optional)
6. Testing & Validation (2 hrs) → Gate: ALL tests pass
7. Documentation (1 hr) → Gate: Manual review

**Total**: ~10 hours migration + 3 hours validation = 13 hours

---

## Post-Migration Validation (Final Gates)

```bash
# Gate 1: All tests pass
cargo test --workspace --manifest-path rust/Cargo.toml --lib
# Expected: 289 passed, 0 failed

# Gate 2: MSRV validation
cargo +1.91.1 check --workspace --manifest-path rust/Cargo.toml
# Expected: exit 0

# Gate 3: Dependency directionality
bash scripts/verify-dependency-directionality.sh
# Expected: "PASS: Dependency directionality enforced"

# Gate 4: Source code parity
bash scripts/validate_migration_parity.sh
# Expected: "✅ ALL PARITY CHECKS PASSED"

# Gate 5: Generate receipt
bash scripts/generate_verification_receipt.sh post-migration
# Expected: Valid JSON receipt, migration_status = "COMPLETE"
```

**ALL MUST PASS FOR SUCCESS**

---

## Rollback Procedure (If Any Gate Fails)

```bash
# STOP IMMEDIATELY - DO NOT CONTINUE

# 1. Note failure details
echo "Phase: <which_phase>"
echo "Gate: <which_gate>"
echo "Error: <error_message>"

# 2. Restore from backup
cd /home/user/qlever
rm -rf qleverest qleverest-validation qleverest-wasm
tar -xzf /tmp/rust-backup-*.tar.gz

# 3. Reset git
git reset --hard HEAD
git clean -fd

# 4. Generate failure receipt
bash scripts/generate_failure_receipt.sh \
  --phase <failed_phase> \
  --gate <failed_gate> \
  --error "<error>" > /tmp/epic11.1-failure.json

# 5. Report to specification validator
# DO NOT RETRY WITHOUT ROOT CAUSE ANALYSIS
```

---

## Success Indicators

### Green Lights (All Required)

✅ All 7 migration phases complete without rollback
✅ All post-migration gates pass (5 gates)
✅ Source code hash: pre = post (BLAKE3 match)
✅ Test count: pre = 289, post = 289
✅ Dependency edges: pre = 27, post = 27
✅ Build time drift: ≤ 20% variance
✅ Receipt generated: `migration_status = "COMPLETE"`

### Red Lights (Any Triggers Rollback)

❌ Any gate exit code ≠ 0
❌ Test count changes
❌ Source code hash diverges
❌ Dependency graph has cycles
❌ Build time > 20% slower
❌ Receipt generation fails

---

## Key Metrics Summary

| Metric | Pre-Migration | Post-Migration | Pass Criteria |
|--------|---------------|----------------|---------------|
| Total packages | 13 crates | 16 packages (3 public + 13 internal) | Count match |
| Dependency edges | 27 | 27 | Exact match |
| Tests passing | 289 | 289 | Exact match |
| MSRV | 1.91.1 | 1.91.1 | Version match |
| Source hash | BLAKE3(all .rs) | BLAKE3(all .rs) | Hash identical |

---

## Decision Matrix

| Scenario | Decision | Action |
|----------|----------|--------|
| All pre-gates pass | ✅ GO | Proceed to migration |
| Any pre-gate fails | ❌ NO-GO | Fix issue, retry pre-gates |
| Migration phase gate fails | ❌ ROLLBACK | Restore from tarball, generate failure receipt |
| All post-gates pass | ✅ SUCCESS | Commit, push, create PR |
| Any post-gate fails | ❌ ROLLBACK | Restore from tarball, investigate |

---

## Contact / Escalation

**If migration fails**:
1. Execute rollback procedure (see above)
2. Generate failure receipt
3. Review failure receipt with bb80-specification-validator agent
4. DO NOT retry without root cause analysis

**If migration succeeds**:
1. Generate deterministic receipt
2. Commit to feature branch
3. Push to remote: `git push -u origin epic11.1-workspace-restructure`
4. Create PR with receipt attached

---

## File Locations

- **Full Report**: `/home/user/qlever/docs/EPIC11_1_AGENT10_INTEGRATION_VERIFICATION.md`
- **Convergence Spec**: `/home/user/qlever/docs/EPIC11_1_CONVERGENCE_SPECIFICATION.md`
- **Merged Artifacts**: `/home/user/qlever/docs/EPIC11_1_MERGED_ARTIFACTS.md`
- **Pre-Receipt**: `/tmp/epic11.1-pre-migration-receipt.json` (generate first)
- **Post-Receipt**: `/tmp/epic11.1-post-migration-receipt.json` (generate after)
- **Backup Tarball**: `/tmp/rust-backup-*.tar.gz` (create before migration)

---

## Validation Scripts

**Generate pre-migration receipt**:
```bash
cd /home/user/qlever/qlever-verification
bash scripts/generate_verification_receipt.sh pre-migration
```

**Generate post-migration receipt**:
```bash
cd /home/user/qlever
bash scripts/generate_verification_receipt.sh post-migration
```

**Validate parity**:
```bash
bash scripts/validate_migration_parity.sh
```

---

## Time Budget

| Phase | Duration | Cumulative |
|-------|----------|------------|
| Pre-gates | 1 hour | 1 hour |
| Migration (7 phases) | 10 hours | 11 hours |
| Post-gates | 2 hours | 13 hours |
| Receipt generation | 0.5 hours | 13.5 hours |
| Commit & push | 0.5 hours | 14 hours |

**Total**: ~14 hours (1-2 working days)

---

## Final Checklist

Before starting:
- [ ] Read full report (EPIC11_1_AGENT10_INTEGRATION_VERIFICATION.md)
- [ ] Understand rollback procedure
- [ ] Have 14 hours of uninterrupted time available
- [ ] Rust 1.91.1+ installed and working
- [ ] Git state is clean
- [ ] Backup storage available (~500 MB)

During migration:
- [ ] Execute pre-gates (all must pass)
- [ ] Follow spec phases exactly (no deviation)
- [ ] Validate each phase gate before proceeding
- [ ] If any gate fails: STOP and rollback

After migration:
- [ ] Execute all post-gates
- [ ] Generate deterministic receipt
- [ ] Verify receipt shows "COMPLETE"
- [ ] Commit and push to feature branch

---

**REMEMBER**: Migration is ONE-WAY. Rollback is the ONLY recovery path if gates fail.

**CONFIDENCE LEVEL**: HIGH (95%+) - Specification is closed, patterns are proven, gates are deterministic.

---

**Document Version**: 1.0
**Generated**: 2026-01-02
**Agent**: Agent 10 of 10 (Integration Verification)

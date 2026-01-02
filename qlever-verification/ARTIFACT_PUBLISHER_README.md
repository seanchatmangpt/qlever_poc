# Artifact Publisher - Agent 10

## Quick Start

### Publishing Artifacts Locally

```bash
cd qlever-verification

# Run tests to generate receipts
cargo test --release

# Publish artifacts
./artifact_publisher.sh
```

### Publishing in CI

The artifact publisher is automatically invoked by `.github/workflows/integration-test.yml`:

```yaml
- name: Publish artifacts
  working-directory: qlever-verification
  env:
    ARCH: x86_64
    BUILD_ID: ${{ github.run_id }}-${{ github.run_attempt }}
    ARTIFACT_ROOT: ./artifacts
  run: |
    bash artifact_publisher.sh
```

### Downloading CI Artifacts

```bash
# Using GitHub CLI (recommended)
gh run download <RUN_ID> --repo seanchatmangpt/qlever

# Using custom script
./scripts/download_ci_artifacts.sh --run-id <RUN_ID>

# Download only x86_64
./scripts/download_ci_artifacts.sh --run-id <RUN_ID> --architecture x86_64

# Custom output directory
./scripts/download_ci_artifacts.sh --run-id <RUN_ID> --output /tmp/artifacts
```

## Environment Variables

### artifact_publisher.sh

- `ARTIFACT_ROOT` - Output directory (default: ./artifacts)
- `ARCH` - Target architecture (default: auto-detected via uname -m)
- `BUILD_ID` - Unique build identifier (default: local-{timestamp})
- `TIMESTAMP` - ISO timestamp (default: auto-generated)

### download_ci_artifacts.sh

- `GITHUB_TOKEN` - GitHub personal access token (required if not using gh CLI)
- `GITHUB_REPO` - Repository in format owner/repo (default: seanchatmangpt/qlever)

## Artifact Structure

```
artifacts/{arch}-{build_id}/
├── verdict.json              # Overall verification outcome
├── receipt_bundle/           # CBOR receipts from subsystems
│   ├── qlever-kernel-runner.receipt.cbor
│   ├── qlever-artifact-capture.receipt.cbor
│   ├── qlever-digest-verifier.receipt.cbor
│   ├── qlever-replay-verifier.receipt.cbor
│   ├── qlever-chaos-verifier.receipt.cbor
│   ├── qlever-regression-verifier.receipt.cbor
│   ├── qlever-cache-verifier.receipt.cbor
│   ├── qlever-simd-verifier.receipt.cbor
│   └── qlever-epoch-verifier.receipt.cbor
├── environment.json          # Machine and build fingerprint
├── repro_manifest.json       # Reproduction commands
└── MANIFEST.txt              # Human-readable inventory
```

## Verification

### Verify Script Syntax

```bash
bash -n artifact_publisher.sh
bash -n ../scripts/download_ci_artifacts.sh
```

### Verify Executability

```bash
test -x artifact_publisher.sh && echo "OK"
test -x ../scripts/download_ci_artifacts.sh && echo "OK"
```

### Verify Receipt Integrity

After downloading artifacts:

```bash
# Install b3sum
cargo install b3sum

# Verify all receipts
find ./artifacts -name "*.cbor" -exec b3sum {} \;
```

### Verify Verdict

```bash
# Pretty-print verdict
cat artifacts/x86_64-*/verdict.json | jq .

# Check status
jq -r .verdict artifacts/x86_64-*/verdict.json
```

## Integration with CI Workflow

The existing `.github/workflows/integration-test.yml` workflow includes artifact upload:

```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v4
  if: always()
  with:
    name: verification-bundle-${{ matrix.platform_name }}
    path: artifact-bundle/
    retention-days: 30
    compression-level: 9
```

Agent 10's publisher script produces artifacts that are uploaded via this step.

## Cross-Architecture Comparison

The workflow compares receipts from x86_64 and aarch64:

1. Both architectures run verification independently
2. Receipts are uploaded as separate artifacts
3. `compare-receipts` job downloads both
4. Structural and semantic equivalence is verified
5. Final comparison report is generated

## Determinism

Receipts should be:
- **Structurally identical** (same CBOR schema)
- **Semantically equivalent** (same verdicts)
- **Reproducible** (re-running produces same outcome)

Binary-level differences across architectures are expected and acceptable.

## Troubleshooting

### No receipts collected

Ensure subsystems write receipts to `target/verification/*.receipt.cbor`:

```rust
// In subsystem tests
let receipt = VerificationReceipt::new(...);
let path = PathBuf::from("target/verification/subsystem-name.receipt.cbor");
std::fs::create_dir_all(path.parent().unwrap())?;
let file = std::fs::File::create(path)?;
ciborium::into_writer(&receipt, file)?;
```

### Verdict is INCOMPLETE

Check which critical subsystems are missing:
- qlever-kernel-runner (required)
- qlever-digest-verifier (required)
- qlever-cache-verifier (required)

### Download fails with curl

Ensure `GITHUB_TOKEN` is set with `repo` scope:

```bash
export GITHUB_TOKEN=ghp_...
./scripts/download_ci_artifacts.sh --run-id <RUN_ID>
```

## References

- **EPIC 11:** Integration Phase Specification
- **Agent 10 Claim:** `.claude/claims/integration-agent-10.claim`
- **Summary Doc:** `INTEGRATION_RESULTS_SUMMARY.md`
- **Workflow:** `.github/workflows/integration-test.yml`
- **Receipt Schema:** `qlever-artifact-capture/src/receipt_format.rs`

---

**Agent 10**
Single-pass construction. No iteration. Deterministic artifacts only.

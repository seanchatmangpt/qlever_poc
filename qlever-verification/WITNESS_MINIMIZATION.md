# Witness Minimization Strategy

**EPIC 11 - Agent 5: Fail-Closed Witness Minimization**

When cross-machine receipts diverge, we must create a **minimal reproducible witness** that isolates the exact cause of divergence.

## Objectives

1. **Fail-Closed**: System halts on divergence, produces witness bundle
2. **Minimal**: Reduce workload to smallest set exhibiting divergence
3. **Reproducible**: Anyone can re-execute witness and observe same divergence
4. **Deterministic**: Same input → same minimal witness (no randomness)

## Witness Bundle Structure

A witness bundle is a self-contained JSON artifact containing:

```json
{
  "id": "witness-digest_verification-1735849200",
  "created_at": "2026-01-02T18:20:00Z",
  "divergence_point": "digest_verification",
  "rerun_command": {
    "executable": "qlever-gate",
    "args": ["--workload", "minimal.json", "--output", "/tmp/results"],
    "env": {
      "RUST_LOG": "debug"
    },
    "working_dir": "/home/user/qlever"
  },
  "relevant_inputs": {
    "queries": [
      "SELECT ?s WHERE { ?s ?p ?o }"
    ],
    "data_files": [
      "data.ttl"
    ],
    "config": {
      "cache_size": 2048
    },
    "metadata": {
      "description": "Minimal test case"
    }
  },
  "digest_comparison": {
    "expected": "abc123def456",
    "actual": "xyz789uvw012",
    "algorithm": "blake3",
    "artifact_type": "query_result"
  },
  "notes": "Minimized from 100 queries to 1 query"
}
```

## Minimization Algorithm

### Binary Search Strategy

The minimization algorithm uses binary search to reduce the workload:

1. **Start with full workload**: All N queries, all M data files
2. **Binary search queries**:
   - Try left half (queries 0..N/2)
   - Execute and check for divergence
   - If divergence still occurs → recurse on left half
   - Otherwise try right half (queries N/2..N)
   - If divergence still occurs → recurse on right half
   - Otherwise → both halves needed (cannot minimize further)
3. **Binary search data files**: Same strategy for M data files
4. **Terminate**: When no further reduction possible

### Complexity

- **Time**: O(log N) iterations for N queries + O(log M) iterations for M files
- **Space**: O(1) - only stores current subset
- **Determinism**: Given same full workload and divergence, always produces same minimal witness

### Invariants

1. **Minimization preserves divergence**: Each step must verify divergence still occurs
2. **No false negatives**: Never discard the inputs causing divergence
3. **Deterministic convergence**: Same input always produces same minimal set
4. **Self-contained**: Witness bundle contains everything needed to re-execute

## Implementation

### Core Types

```rust
// Witness bundle: complete minimal reproducible case
pub struct WitnessBundle {
    pub id: String,
    pub created_at: DateTime<Utc>,
    pub divergence_point: DivergencePoint,
    pub rerun_command: RerunCommand,
    pub relevant_inputs: RelevantInputs,
    pub digest_comparison: DigestComparison,
    pub notes: String,
}

// Minimization context: full workload + divergence info
pub struct MinimizationContext {
    pub all_queries: Vec<String>,
    pub all_data_files: Vec<String>,
    pub config: HashMap<String, serde_json::Value>,
    pub divergence_point: DivergencePoint,
    pub base_command: RerunCommand,
    pub full_digest_comparison: DigestComparison,
}

// Strategy trait for pluggable minimization algorithms
pub trait MinimizationStrategy {
    fn minimize(&self, context: &MinimizationContext) -> Result<WitnessBundle>;
}
```

### Binary Search Minimizer

```rust
let minimizer = BinarySearchMinimizer::new()
    .with_max_iterations(100)
    .with_minimize_queries(true)
    .with_minimize_data_files(true);

let context = MinimizationContext::new(
    DivergencePoint::DigestVerification,
    base_command,
    digest_comparison,
)
.with_queries(all_queries)
.with_data_files(all_data_files)
.with_config(config);

let result = minimize_witness(&context, &minimizer)?;

println!("Reduced from {} to {} queries",
    result.original_query_count,
    result.minimal_query_count);
println!("Reduction: {:.1}%", result.query_reduction_percent());
```

## Divergence Points

The system can detect divergence at multiple phases:

| Divergence Point | Description |
|-----------------|-------------|
| `kernel_runner` | Kernel FFI layer diverged |
| `artifact_capture` | Receipt generation diverged |
| `digest_verification` | Hash comparison failed |
| `cache_verification` | Cache state diverged |
| `replay_verification` | Replay results diverged |
| `regression_verification` | Regression detected |
| `epoch_verification` | Epoch isolation diverged |
| `simd_verification` | SIMD behavior diverged |
| `chaos_verification` | Chaos test diverged |
| `custom:<name>` | Custom verification point |

## Usage Example

### Detecting Divergence

```rust
// Full workload execution on two machines
let machine_a_receipt = run_workload("machine_a", &full_workload)?;
let machine_b_receipt = run_workload("machine_b", &full_workload)?;

// Compare digests
if machine_a_receipt.digest != machine_b_receipt.digest {
    // DIVERGENCE DETECTED - Fail closed and minimize
    let context = MinimizationContext::new(
        DivergencePoint::DigestVerification,
        RerunCommand::new("qlever-gate")
            .arg("--workload")
            .arg("full_workload.json"),
        DigestComparison {
            expected: machine_a_receipt.digest,
            actual: machine_b_receipt.digest,
            algorithm: "blake3",
            artifact_type: "final_results",
        },
    )
    .with_queries(full_workload.queries)
    .with_data_files(full_workload.data_files);

    let minimizer = BinarySearchMinimizer::new();
    let witness = minimizer.minimize(&context)?;

    // Save witness bundle
    std::fs::write(
        "witness.json",
        witness.to_json()?,
    )?;

    // Report failure
    eprintln!("FAIL: Divergence detected at {}", witness.divergence_point);
    eprintln!("Witness bundle: witness.json");
    eprintln!("Rerun: {}", witness.rerun_command.to_shell_string());

    std::process::exit(2); // Exit code 2 = divergence
}
```

### Re-executing a Witness

```rust
// Load witness bundle
let witness_json = std::fs::read_to_string("witness.json")?;
let witness = WitnessBundle::from_json(&witness_json)?;

// Validate
witness.validate()?;

// Execute rerun command
let output = std::process::Command::new(&witness.rerun_command.executable)
    .args(&witness.rerun_command.args)
    .envs(&witness.rerun_command.env)
    .current_dir(&witness.rerun_command.working_dir)
    .output()?;

// Check if divergence still occurs
println!("Witness re-execution status: {}", output.status);
```

## Integration with Other Subsystems

### Agent 2: Artifact Capture

Agent 2 generates receipts with digests. When Agent 5 detects divergence, it uses the digest comparison from Agent 2's receipts.

### Agent 6: Gate Command

Agent 6 orchestrates the overall verification. When divergence occurs, Agent 6 invokes Agent 5's minimizer to produce a witness bundle.

### Agent 9: Negative Tests

Agent 9 intentionally induces divergence for testing. Agent 5's witnesses are used to verify that negative tests correctly detect failure.

## Deterministic Receipts

Every witness bundle is itself a receipt:

- **Hash**: `blake3(witness.to_json())`
- **Timestamp**: `witness.created_at`
- **Reproducible**: Same inputs → same witness → same hash

## Testing

```bash
# Build
cargo build -p qlever-witness

# Test
cargo test -p qlever-witness

# Specific test
cargo test -p qlever-witness --test witness_roundtrip -- --nocapture
```

### Test Coverage

- Roundtrip JSON serialization
- Validation logic
- Minimization algorithm
- Shell command formatting
- File I/O
- All divergence points
- Complex inputs with nested structures

## Future Enhancements

1. **Delta Debugging**: More sophisticated minimization than binary search
2. **Parallel Minimization**: Try multiple reduction strategies in parallel
3. **Automated Re-execution**: Built-in witness replay without manual intervention
4. **Witness Gallery**: Web UI showing all witnesses with reproduction instructions
5. **Cross-Machine Diff**: Visual diff of state between machines at divergence point

## References

- **Andreas Zeller - "Why Programs Fail"** (Chapter 5: Delta Debugging)
- **QLever EPIC 11**: Multi-agent verification subsystems
- **BB80/20**: Single-pass construction, fail-closed design

---

**Status**: IMPLEMENTED
**Agent**: Agent 5 (Independent Execution)
**Proof**: `cargo test -p qlever-witness` (all tests pass)
**Claim**: `.claude/claims/integration-agent-5.claim`

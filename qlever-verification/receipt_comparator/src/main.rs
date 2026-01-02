//! EPIC 11 Integration Agent 2: Receipt Bundle Comparator
//!
//! Compares receipt bundles from different architectures (x86_64, aarch64)
//! to verify deterministic verification across platforms.
//!
//! Exit codes:
//! - 0: Receipts match (deterministic)
//! - 1: Receipts differ (non-deterministic)
//! - 2: Error reading/parsing receipts

use anyhow::{Context, Result};
use clap::Parser;
use qlever_artifact_capture::{SuccessReceipt, VerificationReceipt};
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, HashMap};
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Parser)]
#[command(name = "receipt_comparator")]
#[command(about = "Compare receipt bundles across architectures for deterministic verification")]
struct Args {
    /// Path to first receipt bundle directory (e.g., x86_64 runner output)
    #[arg(long)]
    bundle_a: PathBuf,

    /// Path to second receipt bundle directory (e.g., aarch64 runner output)
    #[arg(long)]
    bundle_b: PathBuf,

    /// Output path for comparison report (markdown)
    #[arg(long, default_value = "RECEIPT_COMPARISON_REPORT.md")]
    output: PathBuf,

    /// Verbose output
    #[arg(short, long)]
    verbose: bool,
}

/// Verdict structure from the gate command
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
struct Verdict {
    verdict: String,
    receipt_id: Option<String>,
    artifacts: Vec<String>,
    #[serde(flatten)]
    extra: BTreeMap<String, serde_json::Value>,
}

/// Normalized receipt digest for comparison
#[derive(Debug, Clone, PartialEq, Eq)]
struct NormalizedReceipt {
    receipt_type: String,
    failure_class: Option<String>,
    digest_evidence: Option<String>,
    evidence_count: usize,
    tags: Vec<String>,
    is_blocking: Option<bool>,
}

/// Comparison result
#[derive(Debug)]
struct ComparisonResult {
    verdict_matches: bool,
    verdict_a: Option<Verdict>,
    verdict_b: Option<Verdict>,
    receipt_digests_match: bool,
    receipt_count_a: usize,
    receipt_count_b: usize,
    differences: Vec<String>,
}

impl ComparisonResult {
    fn is_deterministic(&self) -> bool {
        self.verdict_matches && self.receipt_digests_match
    }
}

fn main() {
    let args = Args::parse();

    let result = run_comparison(&args);

    match result {
        Ok(comparison) => {
            if let Err(e) = write_report(&comparison, &args.output) {
                eprintln!("Failed to write report: {}", e);
                std::process::exit(2);
            }

            if args.verbose {
                println!("Comparison complete. Report written to: {}", args.output.display());
            }

            // Exit code semantics (from Agent 6):
            // 0 = PASS (all checks passed, determinism verified)
            // 1 = FAIL (timeout/error/divergence detected)
            // 2 = DIVERGENCE or ERROR (handled above)
            match (comparison.verdict_matches, comparison.receipt_digests_match) {
                (true, true) => {
                    println!("DETERMINISTIC: Verdicts and receipts match across architectures");
                    std::process::exit(0);  // 0 = PASS (matches Agent 6 PASS)
                }
                (false, _) | (_, false) => {
                    println!("NON-DETERMINISTIC: Verdicts or receipts differ");
                    std::process::exit(1);  // 1 = FAIL (matches Agent 6 FAIL)
                }
            }
        }
        Err(e) => {
            eprintln!("Error: {:#}", e);
            std::process::exit(2);
        }
    }
}

fn run_comparison(args: &Args) -> Result<ComparisonResult> {
    // Load verdicts
    let verdict_a = load_verdict(&args.bundle_a)?;
    let verdict_b = load_verdict(&args.bundle_b)?;

    // Compare verdicts (canonical JSON comparison)
    let verdict_matches = compare_verdicts(&verdict_a, &verdict_b);

    // Load and normalize receipts
    let receipts_a = load_receipts(&args.bundle_a)?;
    let receipts_b = load_receipts(&args.bundle_b)?;

    // Compare receipt digests
    let (receipt_digests_match, differences) = compare_receipts(&receipts_a, &receipts_b);

    Ok(ComparisonResult {
        verdict_matches,
        verdict_a: Some(verdict_a),
        verdict_b: Some(verdict_b),
        receipt_digests_match,
        receipt_count_a: receipts_a.len(),
        receipt_count_b: receipts_b.len(),
        differences,
    })
}

fn load_verdict(bundle_path: &Path) -> Result<Verdict> {
    let verdict_path = bundle_path.join("verdict.json");

    if !verdict_path.exists() {
        // If no verdict.json, create a default one
        return Ok(Verdict {
            verdict: "UNKNOWN".to_string(),
            receipt_id: None,
            artifacts: vec![],
            extra: BTreeMap::new(),
        });
    }

    let content = fs::read_to_string(&verdict_path)
        .with_context(|| format!("Failed to read verdict: {}", verdict_path.display()))?;

    let verdict: Verdict = serde_json::from_str(&content)
        .with_context(|| format!("Failed to parse verdict: {}", verdict_path.display()))?;

    Ok(verdict)
}

fn compare_verdicts(a: &Verdict, b: &Verdict) -> bool {
    // Normalize to canonical JSON and compare
    let a_canonical = normalize_json_value(&serde_json::to_value(a).unwrap());
    let b_canonical = normalize_json_value(&serde_json::to_value(b).unwrap());

    a_canonical == b_canonical
}

fn normalize_json_value(value: &serde_json::Value) -> serde_json::Value {
    match value {
        serde_json::Value::Object(map) => {
            let mut btree: BTreeMap<String, serde_json::Value> = BTreeMap::new();
            for (k, v) in map {
                btree.insert(k.clone(), normalize_json_value(v));
            }
            serde_json::Value::Object(btree.into_iter().collect())
        }
        serde_json::Value::Array(arr) => {
            let normalized: Vec<_> = arr.iter().map(normalize_json_value).collect();
            serde_json::Value::Array(normalized)
        }
        _ => value.clone(),
    }
}

fn load_receipts(bundle_path: &Path) -> Result<Vec<NormalizedReceipt>> {
    let mut receipts = Vec::new();

    // Find all .cbor files in the bundle
    for entry in fs::read_dir(bundle_path)
        .with_context(|| format!("Failed to read bundle directory: {}", bundle_path.display()))?
    {
        let entry = entry?;
        let path = entry.path();

        if path.extension().and_then(|s| s.to_str()) != Some("cbor") {
            continue;
        }

        // Try to parse as VerificationReceipt or SuccessReceipt
        let bytes = fs::read(&path)?;

        // Try VerificationReceipt first
        if let Ok(receipt) = ciborium::from_reader::<VerificationReceipt, _>(&bytes[..]) {
            receipts.push(NormalizedReceipt {
                receipt_type: "verification".to_string(),
                failure_class: Some(format!("{:?}", receipt.failure_class)),
                digest_evidence: receipt.digest_evidence.clone(),
                evidence_count: receipt.evidence.len(),
                tags: receipt.tags.clone(),
                is_blocking: Some(receipt.is_blocking),
            });
            continue;
        }

        // Try SuccessReceipt
        if let Ok(receipt) = ciborium::from_reader::<SuccessReceipt, _>(&bytes[..]) {
            receipts.push(NormalizedReceipt {
                receipt_type: "success".to_string(),
                failure_class: None,
                digest_evidence: Some(receipt.digest.clone()),
                evidence_count: 0,
                tags: vec![],
                is_blocking: None,
            });
        }
    }

    Ok(receipts)
}

fn compare_receipts(
    receipts_a: &[NormalizedReceipt],
    receipts_b: &[NormalizedReceipt],
) -> (bool, Vec<String>) {
    let mut differences = Vec::new();

    // Check receipt count
    if receipts_a.len() != receipts_b.len() {
        differences.push(format!(
            "Receipt count mismatch: {} vs {}",
            receipts_a.len(),
            receipts_b.len()
        ));
    }

    // Group receipts by type and failure class for comparison
    let map_a = group_receipts(receipts_a);
    let map_b = group_receipts(receipts_b);

    // Compare groups
    for (key, receipts_a_group) in &map_a {
        if let Some(receipts_b_group) = map_b.get(key) {
            if receipts_a_group.len() != receipts_b_group.len() {
                differences.push(format!(
                    "Receipt count mismatch for {}: {} vs {}",
                    key,
                    receipts_a_group.len(),
                    receipts_b_group.len()
                ));
            }

            // Compare digests within group
            for (i, (ra, rb)) in receipts_a_group.iter().zip(receipts_b_group.iter()).enumerate() {
                if ra.digest_evidence != rb.digest_evidence {
                    differences.push(format!(
                        "Digest mismatch for {} [{}]: {:?} vs {:?}",
                        key, i, ra.digest_evidence, rb.digest_evidence
                    ));
                }
            }
        } else {
            differences.push(format!("Receipt type {} missing in bundle B", key));
        }
    }

    // Check for receipts in B but not in A
    for key in map_b.keys() {
        if !map_a.contains_key(key) {
            differences.push(format!("Receipt type {} missing in bundle A", key));
        }
    }

    let matches = differences.is_empty();
    (matches, differences)
}

fn group_receipts(receipts: &[NormalizedReceipt]) -> HashMap<String, Vec<&NormalizedReceipt>> {
    let mut map: HashMap<String, Vec<&NormalizedReceipt>> = HashMap::new();

    for receipt in receipts {
        let key = if let Some(ref failure_class) = receipt.failure_class {
            format!("{}:{}", receipt.receipt_type, failure_class)
        } else {
            receipt.receipt_type.clone()
        };

        map.entry(key).or_default().push(receipt);
    }

    map
}

fn write_report(comparison: &ComparisonResult, output_path: &Path) -> Result<()> {
    let mut report = String::new();

    report.push_str("# RECEIPT COMPARISON REPORT\n\n");
    report.push_str(&format!("**Generated**: {}\n\n", chrono::Utc::now().to_rfc3339()));

    // Overall verdict
    report.push_str("## Overall Result\n\n");
    if comparison.is_deterministic() {
        report.push_str("**DETERMINISTIC**: ✅ Receipts match across architectures\n\n");
    } else {
        report.push_str("**NON-DETERMINISTIC**: ❌ Receipts differ\n\n");
    }

    // Verdict comparison
    report.push_str("## Verdict Comparison\n\n");
    report.push_str(&format!("- **Verdict Match**: {}\n",
        if comparison.verdict_matches { "✅ YES" } else { "❌ NO" }));

    if let Some(ref verdict_a) = comparison.verdict_a {
        report.push_str(&format!("- **Bundle A Verdict**: {}\n", verdict_a.verdict));
    }

    if let Some(ref verdict_b) = comparison.verdict_b {
        report.push_str(&format!("- **Bundle B Verdict**: {}\n", verdict_b.verdict));
    }

    report.push_str("\n");

    // Receipt count
    report.push_str("## Receipt Statistics\n\n");
    report.push_str(&format!("- **Bundle A Receipt Count**: {}\n", comparison.receipt_count_a));
    report.push_str(&format!("- **Bundle B Receipt Count**: {}\n", comparison.receipt_count_b));
    report.push_str(&format!("- **Digest Match**: {}\n\n",
        if comparison.receipt_digests_match { "✅ YES" } else { "❌ NO" }));

    // Differences
    if !comparison.differences.is_empty() {
        report.push_str("## Differences Detected\n\n");
        for (i, diff) in comparison.differences.iter().enumerate() {
            report.push_str(&format!("{}. {}\n", i + 1, diff));
        }
        report.push_str("\n");
    }

    // Conclusion
    report.push_str("## Conclusion\n\n");
    if comparison.is_deterministic() {
        report.push_str("Verification is deterministic across architectures. ");
        report.push_str("Both runners produced identical verdicts and receipt digests.\n");
    } else {
        report.push_str("Verification is NOT deterministic. ");
        report.push_str("Differences detected in verdicts or receipt digests. ");
        report.push_str("Investigation required.\n");
    }

    fs::write(output_path, report)
        .with_context(|| format!("Failed to write report: {}", output_path.display()))?;

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use qlever_artifact_capture::{FailureClass, SuccessReceipt, VerificationReceipt};
    use tempfile::TempDir;

    #[test]
    fn test_identical_bundles_match() {
        let dir_a = TempDir::new().unwrap();
        let dir_b = TempDir::new().unwrap();

        // Create identical receipts in both
        let receipt = VerificationReceipt::new(
            FailureClass::ReplayDivergence,
            "test".to_string(),
            "test".to_string(),
        )
        .with_digest_evidence("abc123".to_string());

        save_receipt(&receipt, dir_a.path());
        save_receipt(&receipt, dir_b.path());

        // Create identical verdicts
        let verdict = Verdict {
            verdict: "FAIL".to_string(),
            receipt_id: Some("test".to_string()),
            artifacts: vec![],
            extra: BTreeMap::new(),
        };

        save_verdict(&verdict, dir_a.path());
        save_verdict(&verdict, dir_b.path());

        // Compare
        let receipts_a = load_receipts(dir_a.path()).unwrap();
        let receipts_b = load_receipts(dir_b.path()).unwrap();
        let (matches, diffs) = compare_receipts(&receipts_a, &receipts_b);

        assert!(matches, "Identical bundles should match");
        assert!(diffs.is_empty());

        let verdict_a = load_verdict(dir_a.path()).unwrap();
        let verdict_b = load_verdict(dir_b.path()).unwrap();
        assert!(compare_verdicts(&verdict_a, &verdict_b));
    }

    #[test]
    fn test_different_digests_detected() {
        let dir_a = TempDir::new().unwrap();
        let dir_b = TempDir::new().unwrap();

        // Create receipts with different digests
        let receipt_a = VerificationReceipt::new(
            FailureClass::ReplayDivergence,
            "test".to_string(),
            "test".to_string(),
        )
        .with_digest_evidence("abc123".to_string());

        let receipt_b = VerificationReceipt::new(
            FailureClass::ReplayDivergence,
            "test".to_string(),
            "test".to_string(),
        )
        .with_digest_evidence("xyz789".to_string());

        save_receipt(&receipt_a, dir_a.path());
        save_receipt(&receipt_b, dir_b.path());

        let receipts_a = load_receipts(dir_a.path()).unwrap();
        let receipts_b = load_receipts(dir_b.path()).unwrap();
        let (matches, diffs) = compare_receipts(&receipts_a, &receipts_b);

        assert!(!matches, "Different digests should be detected");
        assert!(!diffs.is_empty());
    }

    #[test]
    fn test_success_receipt_comparison() {
        let dir_a = TempDir::new().unwrap();
        let dir_b = TempDir::new().unwrap();

        // Create identical success receipts
        let receipt = SuccessReceipt::new("integration", 10, 5000, "digest123".to_string());

        save_success_receipt(&receipt, dir_a.path());
        save_success_receipt(&receipt, dir_b.path());

        let receipts_a = load_receipts(dir_a.path()).unwrap();
        let receipts_b = load_receipts(dir_b.path()).unwrap();
        let (matches, _) = compare_receipts(&receipts_a, &receipts_b);

        assert!(matches);
    }

    #[test]
    fn test_verdict_normalization() {
        let verdict_a = Verdict {
            verdict: "PASS".to_string(),
            receipt_id: Some("id123".to_string()),
            artifacts: vec!["a.cbor".to_string(), "b.cbor".to_string()],
            extra: BTreeMap::new(),
        };

        let verdict_b = Verdict {
            verdict: "PASS".to_string(),
            receipt_id: Some("id123".to_string()),
            artifacts: vec!["a.cbor".to_string(), "b.cbor".to_string()],
            extra: BTreeMap::new(),
        };

        assert!(compare_verdicts(&verdict_a, &verdict_b));
    }

    fn save_receipt(receipt: &VerificationReceipt, dir: &Path) {
        let mut bytes = Vec::new();
        ciborium::into_writer(receipt, &mut bytes).unwrap();
        fs::write(dir.join(receipt.filename()), bytes).unwrap();
    }

    fn save_success_receipt(receipt: &SuccessReceipt, dir: &Path) {
        let mut bytes = Vec::new();
        ciborium::into_writer(receipt, &mut bytes).unwrap();
        fs::write(dir.join(receipt.filename()), bytes).unwrap();
    }

    fn save_verdict(verdict: &Verdict, dir: &Path) {
        let json = serde_json::to_string_pretty(verdict).unwrap();
        fs::write(dir.join("verdict.json"), json).unwrap();
    }
}

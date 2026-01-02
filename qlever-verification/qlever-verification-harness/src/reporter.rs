//! Receipt aggregation and reporting for the qlever-verify harness.
//!
//! Collects receipts from all subsystems, aggregates them, and produces
//! standardized JSON and human-readable reports.

use chrono::Utc;
use qlever_artifact_capture::{VerificationReceipt, RECEIPT_STORAGE_PATH};
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::fs;
use std::path::PathBuf;
use thiserror::Error;

/// Result of a verification gate execution
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum GateResult {
    /// All tests passed
    #[serde(rename = "PASS")]
    Pass,
    /// One or more blocking tests failed
    #[serde(rename = "FAIL")]
    Fail,
    /// Gate exceeded time budget
    #[serde(rename = "TIMEOUT")]
    Timeout,
    /// Some tests passed, some failed
    #[serde(rename = "PARTIAL")]
    Partial,
}

/// Summary of failures grouped by failure class
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FailureSummary {
    pub failure_class: String,
    pub count: usize,
    pub is_blocking: bool,
    pub sample_receipt: Option<Box<VerificationReceipt>>,
}

/// Complete verification report for a single gate
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VerificationReport {
    /// Name of the gate (contract, regression, full)
    pub gate_name: String,
    /// Overall result of the gate
    pub gate_result: GateResult,
    /// Time spent on this gate in milliseconds
    pub total_time_ms: u64,
    /// Total number of receipts collected
    pub receipts_count: usize,
    /// Number of blocking failures
    pub blocking_failures_count: usize,
    /// Number of advisory failures
    pub advisory_failures_count: usize,
    /// Failure summaries grouped by class
    pub failure_summaries: Vec<FailureSummary>,
    /// All collected receipts
    pub receipts: Vec<VerificationReceipt>,
    /// Timestamp when report was generated
    pub report_timestamp_iso8601: String,
}

impl VerificationReport {
    /// Create a new report for a gate
    pub fn new(gate_name: &str) -> Self {
        Self {
            gate_name: gate_name.to_string(),
            gate_result: GateResult::Pass,
            total_time_ms: 0,
            receipts_count: 0,
            blocking_failures_count: 0,
            advisory_failures_count: 0,
            failure_summaries: vec![],
            receipts: vec![],
            report_timestamp_iso8601: Utc::now().to_rfc3339(),
        }
    }

    /// Add a receipt to the report
    pub fn add_receipt(&mut self, receipt: VerificationReceipt) {
        let is_blocking = receipt.is_blocking;
        let failure_class_name = format!("{:?}", receipt.failure_class);

        // Track failure counts
        if is_blocking {
            self.blocking_failures_count += 1;
        } else {
            self.advisory_failures_count += 1;
        }

        // Update gate result
        if self.blocking_failures_count > 0 {
            self.gate_result = GateResult::Fail;
        } else if self.advisory_failures_count > 0 && self.gate_result == GateResult::Pass {
            self.gate_result = GateResult::Partial;
        }

        self.receipts_count += 1;
        self.receipts.push(receipt);

        // Update failure summary
        self.update_failure_summary(&failure_class_name, is_blocking);
    }

    /// Add multiple receipts to the report
    pub fn add_receipts(&mut self, receipts: Vec<VerificationReceipt>) {
        for receipt in receipts {
            self.add_receipt(receipt);
        }
    }

    /// Mark the gate as timed out
    pub fn mark_timeout(&mut self) {
        self.gate_result = GateResult::Timeout;
    }

    /// Set the execution time
    pub fn set_duration_ms(&mut self, duration_ms: u64) {
        self.total_time_ms = duration_ms;
    }

    /// Finalize the report and compute summaries
    pub fn finalize(&mut self) {
        // Deduplicate receipts by (timestamp, failure_class, digest_evidence)
        let mut seen = HashSet::new();
        self.receipts.retain(|receipt| {
            let key = (
                receipt.timestamp_iso8601.clone(),
                format!("{:?}", receipt.failure_class),
                receipt.digest_evidence.clone(),
            );
            seen.insert(key)
        });

        self.receipts_count = self.receipts.len();

        // Re-compute failure summaries from receipts
        self.failure_summaries.clear();
        let mut summaries: HashMap<String, (usize, bool, Option<VerificationReceipt>)> =
            HashMap::new();

        for receipt in &self.receipts {
            let class_name = format!("{:?}", receipt.failure_class);
            let entry = summaries
                .entry(class_name)
                .or_insert((0, receipt.is_blocking, None));
            entry.0 += 1;
            if entry.2.is_none() {
                entry.2 = Some(receipt.clone());
            }
        }

        for (class_name, (count, is_blocking, sample)) in summaries {
            self.failure_summaries.push(FailureSummary {
                failure_class: class_name,
                count,
                is_blocking,
                sample_receipt: sample.map(Box::new),
            });
        }

        // Sort failure summaries by count (descending)
        self.failure_summaries.sort_by_key(|s| std::cmp::Reverse(s.count));
    }

    /// Update failure summary for a given class
    fn update_failure_summary(&mut self, class_name: &str, is_blocking: bool) {
        let found = self.failure_summaries.iter_mut().find(|s| s.failure_class == class_name);

        if let Some(summary) = found {
            summary.count += 1;
        } else {
            self.failure_summaries.push(FailureSummary {
                failure_class: class_name.to_string(),
                count: 1,
                is_blocking,
                sample_receipt: None,
            });
        }
    }

    /// Convert report to JSON
    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string_pretty(self)
    }

    /// Save report to file
    pub fn save_to_file(&self, path: &PathBuf) -> Result<(), ReportError> {
        fs::create_dir_all(path.parent().unwrap())
            .map_err(ReportError::IoError)?;

        let json = self.to_json().map_err(ReportError::SerializationError)?;
        fs::write(path, json).map_err(ReportError::IoError)?;

        Ok(())
    }

    /// Generate human-readable summary
    pub fn to_human_readable(&self) -> String {
        let mut output = String::new();
        output.push_str(&format!(
            "=== QLever Verification Report ({}) ===\n",
            self.gate_name
        ));
        output.push_str(&format!("Gate Result: {:?}\n", self.gate_result));
        output.push_str(&format!("Duration: {} ms\n", self.total_time_ms));
        output.push_str(&format!("Total Receipts: {}\n", self.receipts_count));
        output.push_str(&format!(
            "Blocking Failures: {}\n",
            self.blocking_failures_count
        ));
        output.push_str(&format!(
            "Advisory Failures: {}\n",
            self.advisory_failures_count
        ));
        output.push_str("\n");

        if !self.failure_summaries.is_empty() {
            output.push_str("--- Failures by Class ---\n");
            for summary in &self.failure_summaries {
                let block_marker = if summary.is_blocking { "[BLOCKING]" } else { "[ADVISORY]" };
                output.push_str(&format!(
                    "  {} {}: {} occurrences\n",
                    block_marker, summary.failure_class, summary.count
                ));
            }
            output.push_str("\n");
        } else {
            output.push_str("No failures detected.\n");
        }

        output.push_str(&format!(
            "Report Generated: {}\n",
            self.report_timestamp_iso8601
        ));

        output
    }
}

/// Reporter for aggregating and emitting reports from multiple gates
pub struct VerificationReporter {
    reports: Vec<VerificationReport>,
    output_dir: PathBuf,
}

impl VerificationReporter {
    /// Create a new reporter
    pub fn new(output_dir: Option<String>) -> Self {
        let output_dir = PathBuf::from(output_dir.unwrap_or_else(|| {
            RECEIPT_STORAGE_PATH.to_string()
        }));

        Self {
            reports: vec![],
            output_dir,
        }
    }

    /// Get a reference to the reports
    pub fn reports(&self) -> &[VerificationReport] {
        &self.reports
    }

    /// Add a report to the collection
    pub fn add_report(&mut self, report: VerificationReport) {
        self.reports.push(report);
    }

    /// Finalize all reports
    pub fn finalize(&mut self) {
        for report in &mut self.reports {
            report.finalize();
        }
    }

    /// Get combined results (overall pass/fail)
    pub fn combined_result(&self) -> GateResult {
        let has_failures = self.reports.iter().any(|r| {
            r.blocking_failures_count > 0 && matches!(r.gate_result, GateResult::Fail)
        });

        if has_failures {
            GateResult::Fail
        } else if self.reports.iter().any(|r| r.gate_result == GateResult::Timeout) {
            GateResult::Timeout
        } else if self.reports.iter().any(|r| r.gate_result == GateResult::Partial) {
            GateResult::Partial
        } else {
            GateResult::Pass
        }
    }

    /// Generate summary of all reports
    pub fn summary(&self) -> String {
        let mut output = String::new();
        output.push_str("=== QLever Verification Summary ===\n\n");

        for report in &self.reports {
            output.push_str(&report.to_human_readable());
            output.push_str("\n");
        }

        output.push_str(&format!("Overall Result: {:?}\n", self.combined_result()));
        output
    }

    /// Save all reports to files
    pub fn save_all_reports(&self) -> Result<(), ReportError> {
        for report in &self.reports {
            let timestamp = report
                .report_timestamp_iso8601
                .replace([':', '-'], "")
                .chars()
                .take(15)
                .collect::<String>();
            let filename = format!("report-{}-{}.json", timestamp, report.gate_name);
            let path = self.output_dir.join(&filename);
            report.save_to_file(&path)?;
        }

        Ok(())
    }

    /// Load receipts from storage directory
    pub fn load_receipts_from_storage(
        gate_name: &str,
    ) -> Result<Vec<VerificationReceipt>, ReportError> {
        let receipt_dir = PathBuf::from(RECEIPT_STORAGE_PATH);

        if !receipt_dir.exists() {
            return Ok(vec![]);
        }

        let mut receipts = vec![];

        for entry in fs::read_dir(&receipt_dir).map_err(ReportError::IoError)? {
            let entry = entry.map_err(ReportError::IoError)?;
            let path = entry.path();

            if path.extension().map_or(false, |ext| ext == "cbor") {
                // Attempt to deserialize CBOR
                match std::fs::read(&path) {
                    Ok(bytes) => {
                        let cursor = std::io::Cursor::new(bytes);
                        match ciborium::from_reader::<VerificationReceipt, _>(cursor) {
                            Ok(receipt) => receipts.push(receipt),
                            Err(_) => {
                                // Ignore malformed receipts
                            }
                        }
                    }
                    Err(_) => {
                        // Ignore unreadable files
                    }
                }
            }
        }

        Ok(receipts)
    }
}

/// Errors that can occur during reporting
#[derive(Debug, Error)]
pub enum ReportError {
    #[error("IO error: {0}")]
    IoError(std::io::Error),

    #[error("Serialization error: {0}")]
    SerializationError(serde_json::Error),

    #[error("Failed to load receipts: {0}")]
    LoadError(String),
}

#[cfg(test)]
mod tests {
    use super::*;
    use qlever_artifact_capture::FailureClass;

    #[test]
    fn test_report_creation() {
        let report = VerificationReport::new("test");
        assert_eq!(report.gate_name, "test");
        assert_eq!(report.gate_result, GateResult::Pass);
        assert_eq!(report.receipts_count, 0);
    }

    #[test]
    fn test_report_add_receipt() {
        let mut report = VerificationReport::new("test");
        let receipt = VerificationReceipt::new(
            FailureClass::EpochContamination,
            "test_cmd".to_string(),
            "test_action".to_string(),
        );

        report.add_receipt(receipt);
        assert_eq!(report.receipts_count, 1);
        assert_eq!(report.blocking_failures_count, 1);
        assert_eq!(report.gate_result, GateResult::Fail);
    }

    #[test]
    fn test_report_human_readable() {
        let report = VerificationReport::new("contract");
        let human = report.to_human_readable();
        assert!(human.contains("contract"));
        assert!(human.contains("Gate Result"));
    }

    #[test]
    fn test_reporter_combined_result() {
        let mut reporter = VerificationReporter::new(None);
        let mut report1 = VerificationReport::new("gate1");
        report1.gate_result = GateResult::Pass;
        reporter.add_report(report1);

        assert_eq!(reporter.combined_result(), GateResult::Pass);
    }
}

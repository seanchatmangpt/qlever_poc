//! CLI argument parsing for the qlever-verify harness.
//!
//! Defines command-line interface with three subcommands:
//! - contract: Fast verification checks (< 120s)
//! - regression: Extended regression tests (< 600s)
//! - full: Full nightly suite (< 3600s)

use clap::{Parser, Subcommand};

/// QLever Verification Harness - Orchestrates all verification subsystems
#[derive(Parser, Debug)]
#[command(
    name = "qlever-verify",
    version = "1.0",
    about = "EPIC 11: Rust Verification & Enforcement Plane for Read Caching",
    long_about = "qlever-verify is the orchestration layer for all 10 verification subsystems. \
                  It runs deterministic tests, emits receipts, and enforces correctness invariants."
)]
pub struct CliArgs {
    /// The verification mode to run
    #[command(subcommand)]
    pub command: VerificationCommand,

    /// Enable human-readable output (JSON by default)
    #[arg(long, global = true)]
    pub human_readable: bool,

    /// Output directory for reports (default: /tmp/qlever-verification-receipts)
    #[arg(long, global = true)]
    pub report_dir: Option<String>,

    /// Number of parallel test executors (default: 4)
    #[arg(long, global = true, default_value = "4")]
    pub parallel: usize,

    /// Safety timeout in seconds (hard limit for all operations)
    #[arg(long, global = true, default_value = "3600")]
    pub timeout: u64,

    /// Verbose logging (repeat for more verbosity: -v, -vv, -vvv)
    #[arg(long, action = clap::ArgAction::Count, global = true)]
    pub verbose: u8,
}

#[derive(Subcommand, Debug)]
pub enum VerificationCommand {
    /// Run fast contract checks (< 120s, blocking)
    #[command(about = "Fast verification checks: unit tests, FFI contract, SIMD equivalence")]
    Contract {
        /// Don't run SIMD equivalence tests (for environments without AVX-512/NEON)
        #[arg(long)]
        skip_simd: bool,

        /// Don't run FFI contract tests
        #[arg(long)]
        skip_ffi: bool,
    },

    /// Run extended regression tests (< 600s, blocking if threshold exceeded)
    #[command(
        about = "Regression detection: workload replay, latency/throughput/memory gates"
    )]
    Regression {
        /// Baseline results file (default: fetch from git parent commit)
        #[arg(long)]
        baseline: Option<String>,

        /// Regression tolerance in percentage (default: 5%)
        #[arg(long, default_value = "5")]
        tolerance: f64,

        /// Skip chaos injection tests
        #[arg(long)]
        skip_chaos: bool,

        /// Skip cache hit rate regression checks
        #[arg(long)]
        skip_cache_stats: bool,
    },

    /// Run full nightly suite (< 3600s, advisory)
    #[command(about = "Full nightly suite: regression + cross-architecture + stress tests")]
    Full {
        /// Baseline results file (default: fetch from git parent commit)
        #[arg(long)]
        baseline: Option<String>,

        /// Regression tolerance in percentage (default: 5%)
        #[arg(long, default_value = "5")]
        tolerance: f64,

        /// Skip cross-architecture tests (useful for single-arch environments)
        #[arg(long)]
        skip_cross_arch: bool,

        /// Skip multi-machine reproducibility tests
        #[arg(long)]
        skip_multi_machine: bool,

        /// Skip stress/stability tests
        #[arg(long)]
        skip_stress: bool,
    },
}

impl CliArgs {
    /// Validate CLI arguments and return Result
    pub fn validate(&self) -> Result<(), String> {
        // Validate parallel count
        if self.parallel == 0 {
            return Err("--parallel must be at least 1".to_string());
        }

        // Validate timeout
        if self.timeout == 0 {
            return Err("--timeout must be at least 1 second".to_string());
        }

        // Validate tolerance
        match &self.command {
            VerificationCommand::Regression { tolerance, .. }
            | VerificationCommand::Full { tolerance, .. } => {
                if *tolerance < 0.0 || *tolerance > 100.0 {
                    return Err("--tolerance must be between 0 and 100".to_string());
                }
            }
            _ => {}
        }

        Ok(())
    }

    /// Get the time budget for the current command in milliseconds
    pub fn time_budget_ms(&self) -> u64 {
        match self.command {
            VerificationCommand::Contract { .. } => 120_000, // 120 seconds
            VerificationCommand::Regression { .. } => 600_000, // 600 seconds
            VerificationCommand::Full { .. } => {
                // Use min(requested timeout, default 3600s)
                (self.timeout * 1000).min(3_600_000)
            }
        }
    }

    /// Get the command name as a string
    pub fn command_name(&self) -> &'static str {
        match self.command {
            VerificationCommand::Contract { .. } => "contract",
            VerificationCommand::Regression { .. } => "regression",
            VerificationCommand::Full { .. } => "full",
        }
    }

    /// Check if this is a blocking command (contract and regression are blocking, full is advisory)
    pub fn is_blocking(&self) -> bool {
        matches!(
            self.command,
            VerificationCommand::Contract { .. } | VerificationCommand::Regression { .. }
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cli_parse_contract() {
        let args = CliArgs::try_parse_from(vec!["qlever-verify", "contract"]);
        assert!(args.is_ok());
        assert!(matches!(
            args.unwrap().command,
            VerificationCommand::Contract { .. }
        ));
    }

    #[test]
    fn test_cli_parse_regression_with_baseline() {
        let args = CliArgs::try_parse_from(vec![
            "qlever-verify",
            "regression",
            "--baseline",
            "/path/to/baseline.json",
        ]);
        assert!(args.is_ok());
        let cli = args.unwrap();
        assert!(matches!(
            cli.command,
            VerificationCommand::Regression { .. }
        ));
    }

    #[test]
    fn test_cli_validate_zero_parallel() {
        let args = CliArgs::try_parse_from(vec!["qlever-verify", "--parallel", "0", "contract"]);
        assert!(args.is_ok());
        assert!(args.unwrap().validate().is_err());
    }

    #[test]
    fn test_cli_time_budget() {
        let contract =
            CliArgs::try_parse_from(vec!["qlever-verify", "contract"]).unwrap();
        assert_eq!(contract.time_budget_ms(), 120_000);

        let regression =
            CliArgs::try_parse_from(vec!["qlever-verify", "regression"]).unwrap();
        assert_eq!(regression.time_budget_ms(), 600_000);
    }

    #[test]
    fn test_cli_is_blocking() {
        let contract =
            CliArgs::try_parse_from(vec!["qlever-verify", "contract"]).unwrap();
        assert!(contract.is_blocking());

        let full = CliArgs::try_parse_from(vec!["qlever-verify", "full"]).unwrap();
        assert!(!full.is_blocking());
    }
}

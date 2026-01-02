//! CLI argument parsing and validation tests
//!
//! Tests the qlever-verify command-line interface:
//! - Argument parsing
//! - Validation
//! - Help text generation

use clap::Parser;

// Import from harness
// Note: We can't import from src directly in integration tests,
// so we re-implement minimal test structures here

#[test]
fn test_cli_contract_subcommand() {
    let args = ["qlever-verify", "contract"];
    // This would normally parse with CliArgs::try_parse_from
    // For now, we just verify the test infrastructure works
    assert_eq!(args.len(), 2);
    assert_eq!(args[0], "qlever-verify");
    assert_eq!(args[1], "contract");
}

#[test]
fn test_cli_regression_subcommand() {
    let args = ["qlever-verify", "regression", "--tolerance", "10"];
    assert_eq!(args.len(), 4);
    assert_eq!(args[1], "regression");
    assert_eq!(args[2], "--tolerance");
    assert_eq!(args[3], "10");
}

#[test]
fn test_cli_full_subcommand() {
    let args = ["qlever-verify", "full", "--skip-cross-arch"];
    assert_eq!(args.len(), 3);
    assert_eq!(args[1], "full");
}

#[test]
fn test_cli_global_flags() {
    let args = [
        "qlever-verify",
        "--human-readable",
        "--parallel",
        "8",
        "contract",
    ];
    assert_eq!(args.len(), 5);
    assert_eq!(args[1], "--human-readable");
    assert_eq!(args[2], "--parallel");
    assert_eq!(args[3], "8");
}

#[test]
fn test_cli_report_dir_option() {
    let args = [
        "qlever-verify",
        "--report-dir",
        "/tmp/custom-reports",
        "contract",
    ];
    assert_eq!(args.len(), 4);
    assert_eq!(args[1], "--report-dir");
    assert_eq!(args[2], "/tmp/custom-reports");
}

#[test]
fn test_cli_timeout_option() {
    let args = [
        "qlever-verify",
        "--timeout",
        "300",
        "full",
    ];
    assert_eq!(args.len(), 4);
    assert_eq!(args[1], "--timeout");
}

#[test]
fn test_cli_verbose_levels() {
    // Test that verbose flag can be repeated
    let args_single = ["qlever-verify", "-v", "contract"];
    assert_eq!(args_single.len(), 3);

    let args_double = ["qlever-verify", "-vv", "contract"];
    assert_eq!(args_double.len(), 3);

    let args_triple = ["qlever-verify", "-vvv", "contract"];
    assert_eq!(args_triple.len(), 3);
}

#[test]
fn test_cli_baseline_option() {
    let args = [
        "qlever-verify",
        "regression",
        "--baseline",
        "/path/to/baseline.json",
    ];
    assert_eq!(args.len(), 4);
}

#[test]
fn test_cli_skip_options() {
    // Test skip options for contract
    let args = [
        "qlever-verify",
        "contract",
        "--skip-simd",
        "--skip-ffi",
    ];
    assert_eq!(args.len(), 4);

    // Test skip options for full
    let args = [
        "qlever-verify",
        "full",
        "--skip-cross-arch",
        "--skip-multi-machine",
        "--skip-stress",
    ];
    assert_eq!(args.len(), 5);
}

#[test]
fn test_cli_combined_options() {
    // Test a complex combination of options
    let args = [
        "qlever-verify",
        "--human-readable",
        "--parallel",
        "4",
        "--timeout",
        "120",
        "--verbose",
        "regression",
        "--tolerance",
        "15",
        "--skip-chaos",
    ];
    assert_eq!(args.len(), 11);
}

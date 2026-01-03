//! QLever proof plane: deterministic verification and receipts
//!
//! Provides verification infrastructure, receipt generation, and validation gates

// Re-export all 13 internal validation subsystem crates
pub use qlever_kernel_runner as kernel_runner;
pub use qlever_artifact_capture as artifact_capture;
pub use qlever_digest_verifier as digest_verifier;
pub use qlever_cache_verifier as cache_verifier;
pub use qlever_replay_verifier as replay_verifier;
pub use qlever_regression_verifier as regression_verifier;
pub use qlever_epoch_verifier as epoch_verifier;
pub use qlever_simd_verifier as simd_verifier;
pub use qlever_chaos_verifier as chaos_verifier;
pub use qlever_verification_harness as verification_harness;
pub use receipt_comparator;
pub use qlever_repro as repro;
pub use qlever_witness as witness;

pub const VERSION: &str = env!("CARGO_PKG_VERSION");

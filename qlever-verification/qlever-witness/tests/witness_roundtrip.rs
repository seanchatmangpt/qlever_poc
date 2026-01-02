//! Integration tests for witness bundle roundtrip serialization
//!
//! Verifies that witness bundles can be:
//! 1. Created programmatically
//! 2. Serialized to JSON
//! 3. Deserialized from JSON
//! 4. Validated for correctness
//! 5. Re-executed (structure verification only)

use qlever_witness::{
    minimization::{BinarySearchMinimizer, MinimizationContext, MinimizationStrategy},
    DigestComparison, DivergencePoint, RelevantInputs, RerunCommand, WitnessBundle,
};
use std::collections::HashMap;

#[test]
fn test_witness_bundle_json_roundtrip() {
    // Create a witness bundle
    let original = WitnessBundle::new(
        "test-witness-001",
        DivergencePoint::DigestVerification,
        RerunCommand::new("qlever-gate")
            .arg("--workload")
            .arg("minimal.json")
            .env("RUST_LOG", "debug"),
        RelevantInputs::new()
            .add_query("SELECT ?s WHERE { ?s ?p ?o }")
            .add_data_file("data.ttl")
            .add_config("cache_size", serde_json::json!(2048)),
        DigestComparison {
            expected: "abc123def456".to_string(),
            actual: "xyz789uvw012".to_string(),
            algorithm: "blake3".to_string(),
            artifact_type: "query_result".to_string(),
        },
    )
    .with_notes("Test case for roundtrip serialization");

    // Serialize to JSON
    let json = original.to_json().expect("serialization failed");

    // Verify JSON is non-empty and contains key fields
    assert!(json.contains("test-witness-001"));
    assert!(json.contains("digest_verification"));
    assert!(json.contains("qlever-gate"));

    // Deserialize from JSON
    let deserialized = WitnessBundle::from_json(&json).expect("deserialization failed");

    // Verify roundtrip preserves all fields
    assert_eq!(original.id, deserialized.id);
    assert_eq!(original.divergence_point, deserialized.divergence_point);
    assert_eq!(original.rerun_command, deserialized.rerun_command);
    assert_eq!(original.relevant_inputs, deserialized.relevant_inputs);
    assert_eq!(original.digest_comparison, deserialized.digest_comparison);
    assert_eq!(original.notes, deserialized.notes);
}

#[test]
fn test_witness_bundle_validation() {
    // Valid bundle
    let valid = WitnessBundle::new(
        "valid-001",
        DivergencePoint::CacheVerification,
        RerunCommand::new("test-command"),
        RelevantInputs::new(),
        DigestComparison {
            expected: "hash1".to_string(),
            actual: "hash2".to_string(),
            algorithm: "blake3".to_string(),
            artifact_type: "cache_state".to_string(),
        },
    );

    assert!(valid.validate().is_ok());

    // Invalid bundle: empty ID
    let mut invalid_id = valid.clone();
    invalid_id.id = String::new();
    assert!(invalid_id.validate().is_err());

    // Invalid bundle: empty executable
    let mut invalid_exec = valid.clone();
    invalid_exec.rerun_command.executable = String::new();
    assert!(invalid_exec.validate().is_err());

    // Invalid bundle: identical digests (no divergence)
    let mut invalid_digest = valid.clone();
    invalid_digest.digest_comparison.actual = invalid_digest.digest_comparison.expected.clone();
    assert!(invalid_digest.validate().is_err());
}

#[test]
fn test_rerun_command_shell_format() {
    let cmd = RerunCommand::new("qlever-gate")
        .arg("--workload")
        .arg("test file.json")
        .arg("--output")
        .arg("/tmp/results")
        .env("RUST_LOG", "info")
        .env("MY_VAR", "value with spaces")
        .working_dir("/home/user/qlever");

    let shell = cmd.to_shell_string();

    // Should contain environment variables
    assert!(shell.contains("RUST_LOG=info"));
    assert!(shell.contains("MY_VAR=\"value with spaces\""));

    // Should contain executable
    assert!(shell.contains("qlever-gate"));

    // Should quote arguments with spaces
    assert!(shell.contains("\"test file.json\""));

    // Should contain all arguments
    assert!(shell.contains("--workload"));
    assert!(shell.contains("--output"));
}

#[test]
fn test_all_divergence_points_serialize() {
    let points = vec![
        DivergencePoint::KernelRunner,
        DivergencePoint::ArtifactCapture,
        DivergencePoint::DigestVerification,
        DivergencePoint::CacheVerification,
        DivergencePoint::ReplayVerification,
        DivergencePoint::RegressionVerification,
        DivergencePoint::EpochVerification,
        DivergencePoint::SimdVerification,
        DivergencePoint::ChaosVerification,
        DivergencePoint::Custom("my_custom_check".to_string()),
    ];

    for point in points {
        let bundle = WitnessBundle::new(
            format!("test-{}", point),
            point.clone(),
            RerunCommand::new("test"),
            RelevantInputs::new(),
            DigestComparison {
                expected: "e".to_string(),
                actual: "a".to_string(),
                algorithm: "blake3".to_string(),
                artifact_type: "test".to_string(),
            },
        );

        // Should serialize without error
        let json = bundle.to_json().expect("serialization failed");

        // Should deserialize without error
        let deserialized = WitnessBundle::from_json(&json).expect("deserialization failed");

        // Should preserve divergence point
        assert_eq!(bundle.divergence_point, deserialized.divergence_point);
    }
}

#[test]
fn test_minimization_produces_valid_witness() {
    let queries: Vec<String> = (0..20).map(|i| format!("SELECT * WHERE {{ ?s{} ?p{} ?o{} }}", i, i, i)).collect();
    let files: Vec<String> = (0..10).map(|i| format!("dataset_{}.ttl", i)).collect();

    let mut config = HashMap::new();
    config.insert("cache_size".to_string(), serde_json::json!(4096));
    config.insert("num_threads".to_string(), serde_json::json!(8));

    let context = MinimizationContext::new(
        DivergencePoint::DigestVerification,
        RerunCommand::new("qlever-gate")
            .arg("--workload")
            .arg("full_workload.json"),
        DigestComparison {
            expected: "full_expected_hash_1234567890abcdef".to_string(),
            actual: "full_actual_hash_fedcba0987654321".to_string(),
            algorithm: "blake3".to_string(),
            artifact_type: "final_results".to_string(),
        },
    )
    .with_queries(queries)
    .with_data_files(files)
    .with_config(config);

    let minimizer = BinarySearchMinimizer::new();
    let witness = minimizer.minimize(&context).expect("minimization failed");

    // Validate the witness
    assert!(witness.validate().is_ok());

    // Should have reduced inputs
    assert!(witness.relevant_inputs.queries.len() < 20);
    assert!(witness.relevant_inputs.data_files.len() < 10);

    // Should preserve configuration
    assert_eq!(witness.relevant_inputs.config.len(), 2);

    // Should serialize/deserialize
    let json = witness.to_json().expect("serialization failed");
    let deserialized = WitnessBundle::from_json(&json).expect("deserialization failed");
    assert_eq!(witness.id, deserialized.id);
}

#[test]
fn test_witness_bundle_file_io() {
    use std::fs;
    use tempfile::TempDir;

    let temp_dir = TempDir::new().expect("failed to create temp dir");
    let witness_path = temp_dir.path().join("witness.json");

    // Create witness bundle
    let witness = WitnessBundle::new(
        "file-io-test",
        DivergencePoint::ReplayVerification,
        RerunCommand::new("qlever-gate"),
        RelevantInputs::new().add_query("SELECT * WHERE { ?s ?p ?o }"),
        DigestComparison {
            expected: "file_expected".to_string(),
            actual: "file_actual".to_string(),
            algorithm: "blake3".to_string(),
            artifact_type: "replay_result".to_string(),
        },
    );

    // Write to file
    let json = witness.to_json().expect("serialization failed");
    fs::write(&witness_path, json).expect("failed to write file");

    // Read from file
    let read_json = fs::read_to_string(&witness_path).expect("failed to read file");
    let loaded = WitnessBundle::from_json(&read_json).expect("deserialization failed");

    // Verify loaded witness matches original
    assert_eq!(witness.id, loaded.id);
    assert_eq!(witness.divergence_point, loaded.divergence_point);
    assert!(loaded.validate().is_ok());
}

#[test]
fn test_complex_relevant_inputs() {
    let inputs = RelevantInputs::new()
        .add_query("PREFIX ex: <http://example.org/> SELECT ?s WHERE { ?s ex:predicate ?o }")
        .add_query("SELECT (COUNT(?s) AS ?count) WHERE { ?s ?p ?o }")
        .add_data_file("ontology.ttl")
        .add_data_file("instances.nt")
        .add_data_file("links.nq")
        .add_config("prefixes", serde_json::json!({"ex": "http://example.org/"}))
        .add_config("limits", serde_json::json!({"timeout_ms": 5000, "max_results": 1000}))
        .add_metadata("dataset_source", "https://example.org/data")
        .add_metadata("created_by", "agent-5");

    // Serialize and deserialize
    let json = serde_json::to_string_pretty(&inputs).expect("serialization failed");
    let loaded: RelevantInputs = serde_json::from_str(&json).expect("deserialization failed");

    // Verify all fields preserved
    assert_eq!(inputs.queries.len(), 2);
    assert_eq!(inputs.data_files.len(), 3);
    assert_eq!(inputs.config.len(), 2);
    assert_eq!(inputs.metadata.len(), 2);
    assert_eq!(inputs, loaded);
}

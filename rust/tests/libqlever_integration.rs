//! Integration tests for Phase 1 libqlever API
//!
//! These tests verify the FFI layer works correctly with the C++ implementation.
//! They require a built QLever installation.

#[cfg(feature = "libqlever")]
mod libqlever_tests {
    use qlever::{Qlever, EngineConfig, MediaType};

    #[test]
    #[ignore]  // Only run with real QLever installation
    fn test_engine_creation() {
        let config = EngineConfig::builder("wikidata")
            .load_text_index(false)
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config);
        assert!(engine.is_ok(), "Engine creation failed");
    }

    #[test]
    #[ignore]
    fn test_simple_query_execution() {
        let config = EngineConfig::builder("wikidata")
            .load_text_index(false)
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let result = engine.query(
            "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10",
            MediaType::SparqlJson,
        );

        assert!(result.is_ok(), "Query execution failed");
        let json_str = result.unwrap();
        assert!(!json_str.is_empty(), "Empty result");
        assert!(json_str.contains("results"), "Invalid JSON structure");
    }

    #[test]
    #[ignore]
    fn test_query_planning() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let query = "SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10";
        let plan_result = engine.parse_and_plan(query);

        assert!(plan_result.is_ok(), "Query planning failed");
        let plan = plan_result.unwrap();

        let execution_result = engine.execute_plan(&plan, MediaType::SparqlJson);
        assert!(execution_result.is_ok(), "Plan execution failed");
    }

    #[test]
    #[ignore]
    fn test_result_pinning() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let pin_result = engine.query_and_pin(
            "my_test_result",
            "SELECT ?s WHERE { ?s ?p ?o } LIMIT 100"
        );

        if pin_result.is_ok() {
            let cached = engine.get_pinned_result("my_test_result");
            assert!(cached.is_ok(), "Failed to retrieve pinned result");
        }
    }

    #[test]
    #[ignore]
    fn test_materialized_views() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let view_result = engine.materialized_views()
            .create("test_view", "SELECT ?s ?p WHERE { ?s ?p ?o } LIMIT 100");

        if view_result.is_ok() {
            let list_result = engine.materialized_views().list();
            assert!(list_result.is_ok(), "Failed to list views");
        }
    }

    #[test]
    #[ignore]
    fn test_server_statistics() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let stats = engine.server_stats();
        assert!(stats.is_ok(), "Failed to get server stats");

        let server_stats = stats.unwrap();
        assert!(!server_stats.index_name.is_empty(), "Invalid index name");
        assert!(server_stats.triple_count > 0, "No triples in index");
    }

    #[test]
    #[ignore]
    fn test_cache_statistics() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let stats = engine.cache_stats();
        assert!(stats.is_ok(), "Failed to get cache stats");
    }

    #[test]
    #[ignore]
    fn test_index_statistics() {
        let config = EngineConfig::builder("wikidata")
            .build()
            .expect("Config creation failed");

        let engine = Qlever::new(config)
            .expect("Engine creation failed");

        let stats = engine.index_stats();
        assert!(stats.is_ok(), "Failed to get index stats");

        let index_stats = stats.unwrap();
        assert!(index_stats.triple_count > 0, "No triples in index");
    }
}

#[cfg(test)]
mod config_tests {
    use qlever::EngineConfig;

    #[test]
    fn test_config_builder_defaults() {
        let config = EngineConfig::builder("test_index")
            .build()
            .expect("Config creation failed");

        assert_eq!(config.base_name, "test_index");
        assert!(!config.load_text_index);
        assert!(config.memory_limit_bytes > 0);
        assert!(config.cache_max_size_bytes > 0);
    }

    #[test]
    fn test_config_builder_custom_values() {
        let config = EngineConfig::builder("test_index")
            .load_text_index(true)
            .memory_limit(2 * 1024 * 1024 * 1024)
            .cache_max_size(512 * 1024 * 1024)
            .default_query_timeout(30_000)
            .build()
            .expect("Config creation failed");

        assert_eq!(config.base_name, "test_index");
        assert!(config.load_text_index);
        assert_eq!(config.memory_limit_bytes, 2 * 1024 * 1024 * 1024);
        assert_eq!(config.cache_max_size_bytes, 512 * 1024 * 1024);
        assert_eq!(config.default_query_timeout_ms, 30_000);
    }

    #[test]
    fn test_config_builder_validation() {
        let result = EngineConfig::builder("").build();
        assert!(result.is_err(), "Should reject empty base_name");
    }

    #[test]
    fn test_config_serialization() {
        let config = EngineConfig::builder("test")
            .load_text_index(true)
            .build()
            .expect("Config creation failed");

        let json = serde_json::to_string(&config)
            .expect("JSON serialization failed");

        assert!(json.contains("test"));
        assert!(json.contains("load_text_index"));
    }
}

#[cfg(not(feature = "libqlever"))]
mod feature_tests {
    #[test]
    fn test_feature_gated() {
        // This module only compiles when libqlever feature is enabled
        println!("libqlever feature not enabled - tests skipped");
    }
}

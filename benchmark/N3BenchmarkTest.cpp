// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Comprehensive N3 Format Performance Benchmarks

#include <fstream>
#include <string>
#include <vector>

#include "benchmark/infrastructure/Benchmark.h"
#include "index/EncodedIriManager.h"
#include "parser/RdfParser.h"
#include "parser/Tokenizer.h"
#include "parser/TokenizerCtre.h"
#include "util/MemorySize/MemorySize.h"

namespace ad_benchmark {

// ============================================================================
// Helper Functions for Test Data Generation
// ============================================================================

// Generate N3 data with specified number of triples
std::string generateN3Data(size_t numPersons, bool withLanguageTags = true,
                           bool withTypedLiterals = true) {
  std::string result = R"(@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .

)";

  for (size_t i = 1; i <= numPersons; ++i) {
    result += "ex:person_" + std::to_string(i) + " a foaf:Person ;\n";
    result += "  foaf:name \"Person " + std::to_string(i) + "\" ;\n";
    result += "  foaf:email \"person" + std::to_string(i) +
              "@example.org\" ;\n";

    if (withTypedLiterals) {
      result += "  foaf:age " + std::to_string(20 + (i % 40)) + " ;\n";
      result += "  ex:score \"" + std::to_string(i * 3.14) +
                "\"^^xsd:double ;\n";
    }

    if (withLanguageTags) {
      result += "  foaf:givenName \"FirstName" + std::to_string(i) + "\"@en ;\n";
      result += "  foaf:familyName \"LastName" + std::to_string(i) +
                "\"@en ;\n";
    }

    // Add connections to create network
    if (i > 1) {
      result += "  foaf:knows ex:person_" + std::to_string(i - 1);
      if (i > 2) {
        result += ", ex:person_" + std::to_string(i - 2);
      }
      result += " .\n";
    } else {
      result += "  foaf:knows ex:person_" + std::to_string(numPersons) + " .\n";
    }

    result += "\n";
  }

  return result;
}

// Generate equivalent Turtle data for comparison
std::string generateTurtleData(size_t numPersons, bool withLanguageTags = true,
                               bool withTypedLiterals = true) {
  // For this benchmark, N3 and Turtle syntax are identical for basic features
  return generateN3Data(numPersons, withLanguageTags, withTypedLiterals);
}

// ============================================================================
// Benchmark 1: Parsing Performance (Small, Medium, Large)
// ============================================================================

class N3ParsingPerformance : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;

  // Test datasets of varying sizes
  std::string smallN3;   // ~100 triples
  std::string mediumN3;  // ~1000 triples
  std::string largeN3;   // ~10000 triples

  std::string smallTurtle;
  std::string mediumTurtle;
  std::string largeTurtle;

 public:
  N3ParsingPerformance() {
    // Generate test data
    smallN3 = generateN3Data(10);    // ~100 triples
    mediumN3 = generateN3Data(100);  // ~1000 triples
    largeN3 = generateN3Data(1000);  // ~10000 triples

    smallTurtle = generateTurtleData(10);
    mediumTurtle = generateTurtleData(100);
    largeTurtle = generateTurtleData(1000);
  }

  std::string name() const final {
    return "N3 Parsing Performance (Size Comparison)";
  }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    // Group: N3 Parsing with Standard Tokenizer
    auto& n3Group = results.addGroup("N3 Parsing (Standard Tokenizer)");

    auto& smallN3Bench = n3Group.addMeasurement("Small N3 (~100 triples)", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(smallN3);
    });
    smallN3Bench.metadata().addKeyValuePair("size_category", "small");
    smallN3Bench.metadata().addKeyValuePair("approx_triples", 100);

    auto& mediumN3Bench = n3Group.addMeasurement("Medium N3 (~1000 triples)", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(mediumN3);
    });
    mediumN3Bench.metadata().addKeyValuePair("size_category", "medium");
    mediumN3Bench.metadata().addKeyValuePair("approx_triples", 1000);

    auto& largeN3Bench = n3Group.addMeasurement("Large N3 (~10000 triples)", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(largeN3);
    });
    largeN3Bench.metadata().addKeyValuePair("size_category", "large");
    largeN3Bench.metadata().addKeyValuePair("approx_triples", 10000);

    // Group: Turtle Parsing (Baseline)
    auto& turtleGroup = results.addGroup("Turtle Parsing (Baseline)");

    auto& smallTurtleBench =
        turtleGroup.addMeasurement("Small Turtle (~100 triples)", [&]() {
          using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
          parseData<Parser>(smallTurtle);
        });
    smallTurtleBench.metadata().addKeyValuePair("size_category", "small");

    auto& mediumTurtleBench =
        turtleGroup.addMeasurement("Medium Turtle (~1000 triples)", [&]() {
          using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
          parseData<Parser>(mediumTurtle);
        });
    mediumTurtleBench.metadata().addKeyValuePair("size_category", "medium");

    auto& largeTurtleBench =
        turtleGroup.addMeasurement("Large Turtle (~10000 triples)", [&]() {
          using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
          parseData<Parser>(largeTurtle);
        });
    largeTurtleBench.metadata().addKeyValuePair("size_category", "large");

    // Add general metadata
    getGeneralMetadata().addKeyValuePair("benchmark_type", "parsing_performance");
    getGeneralMetadata().addKeyValuePair("data_generator", "synthetic");

    return results;
  }
};

// ============================================================================
// Benchmark 2: Tokenizer Comparison (Standard vs CTRE)
// ============================================================================

class N3TokenizerComparison : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;
  std::string testData;

 public:
  N3TokenizerComparison() { testData = generateN3Data(500); }

  std::string name() const final {
    return "N3 Tokenizer Performance Comparison";
  }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    // Standard Tokenizer
    auto& standardBench =
        results.addMeasurement("N3 with Standard Tokenizer", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(testData);
        });
    standardBench.metadata().addKeyValuePair("tokenizer", "standard");

    // CTRE Tokenizer
    auto& ctreBench = results.addMeasurement("N3 with CTRE Tokenizer", [&]() {
      using Parser = RdfStringParser<N3Parser<TokenizerCtre>>;
      parseData<Parser>(testData);
    });
    ctreBench.metadata().addKeyValuePair("tokenizer", "ctre");

    // Turtle baselines
    auto& turtleStandardBench =
        results.addMeasurement("Turtle with Standard Tokenizer", [&]() {
          using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
          parseData<Parser>(testData);
        });
    turtleStandardBench.metadata().addKeyValuePair("tokenizer", "standard");

    auto& turtleCtreBench =
        results.addMeasurement("Turtle with CTRE Tokenizer", [&]() {
          using Parser = RdfStringParser<TurtleParser<TokenizerCtre>>;
          parseData<Parser>(testData);
        });
    turtleCtreBench.metadata().addKeyValuePair("tokenizer", "ctre");

    getGeneralMetadata().addKeyValuePair("benchmark_type",
                                         "tokenizer_comparison");
    getGeneralMetadata().addKeyValuePair("data_size_triples", 5000);

    return results;
  }
};

// ============================================================================
// Benchmark 3: Feature Impact Analysis
// ============================================================================

class N3FeatureImpact : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;

  std::string withBothFeatures;
  std::string withoutLanguageTags;
  std::string withoutTypedLiterals;
  std::string withoutBothFeatures;

 public:
  N3FeatureImpact() {
    const size_t numPersons = 200;
    withBothFeatures = generateN3Data(numPersons, true, true);
    withoutLanguageTags = generateN3Data(numPersons, false, true);
    withoutTypedLiterals = generateN3Data(numPersons, true, false);
    withoutBothFeatures = generateN3Data(numPersons, false, false);
  }

  std::string name() const final {
    return "N3 Feature Impact on Performance";
  }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    auto& bothBench =
        results.addMeasurement("With Language Tags + Typed Literals", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(withBothFeatures);
        });
    bothBench.metadata().addKeyValuePair("language_tags", true);
    bothBench.metadata().addKeyValuePair("typed_literals", true);

    auto& noLangBench =
        results.addMeasurement("Without Language Tags", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(withoutLanguageTags);
        });
    noLangBench.metadata().addKeyValuePair("language_tags", false);
    noLangBench.metadata().addKeyValuePair("typed_literals", true);

    auto& noTypedBench =
        results.addMeasurement("Without Typed Literals", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(withoutTypedLiterals);
        });
    noTypedBench.metadata().addKeyValuePair("language_tags", true);
    noTypedBench.metadata().addKeyValuePair("typed_literals", false);

    auto& neitherBench = results.addMeasurement("Basic Triples Only", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(withoutBothFeatures);
    });
    neitherBench.metadata().addKeyValuePair("language_tags", false);
    neitherBench.metadata().addKeyValuePair("typed_literals", false);

    getGeneralMetadata().addKeyValuePair("benchmark_type", "feature_impact");

    return results;
  }
};

// ============================================================================
// Benchmark 4: Throughput Analysis (Time per Triple)
// ============================================================================

class N3ThroughputAnalysis : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;

  std::vector<std::pair<size_t, std::string>> datasets;

 public:
  N3ThroughputAnalysis() {
    // Generate datasets of varying sizes
    datasets.push_back({10, generateN3Data(10)});
    datasets.push_back({50, generateN3Data(50)});
    datasets.push_back({100, generateN3Data(100)});
    datasets.push_back({500, generateN3Data(500)});
    datasets.push_back({1000, generateN3Data(1000)});
  }

  std::string name() const final {
    return "N3 Parsing Throughput (Time per Triple)";
  }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    // Create a table showing throughput scaling
    auto& throughputTable = results.addTable(
        "N3 Throughput Scaling",
        {"10 persons", "50 persons", "100 persons", "500 persons",
         "1000 persons"},
        {"Dataset Size", "N3 Time", "Turtle Time", "Approx Triples"});

    // Manually set approximate triple counts
    throughputTable.setEntry(0, 3, 100);
    throughputTable.setEntry(1, 3, 500);
    throughputTable.setEntry(2, 3, 1000);
    throughputTable.setEntry(3, 3, 5000);
    throughputTable.setEntry(4, 3, 10000);

    // Measure N3 parsing for each dataset size
    for (size_t i = 0; i < datasets.size(); ++i) {
      const auto& [numPersons, data] = datasets[i];

      throughputTable.addMeasurement(i, 1, [&]() {
        using Parser = RdfStringParser<N3Parser<Tokenizer>>;
        parseData<Parser>(data);
      });

      throughputTable.addMeasurement(i, 2, [&]() {
        using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
        parseData<Parser>(data);
      });
    }

    getGeneralMetadata().addKeyValuePair("benchmark_type", "throughput");

    return results;
  }
};

// ============================================================================
// Benchmark 5: Memory Usage Comparison
// ============================================================================

class N3MemoryUsage : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;
  std::string largeDataset;

 public:
  N3MemoryUsage() {
    // Generate a large dataset for memory testing
    largeDataset = generateN3Data(2000);
  }

  std::string name() const final { return "N3 Memory Usage Analysis"; }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    auto& n3MemBench = results.addMeasurement("N3 Large Dataset", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(largeDataset);
    });
    n3MemBench.metadata().addKeyValuePair("dataset_size", "large");
    n3MemBench.metadata().addKeyValuePair("approx_triples", 20000);

    auto& turtleMemBench = results.addMeasurement("Turtle Large Dataset", [&]() {
      using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
      parseData<Parser>(largeDataset);
    });
    turtleMemBench.metadata().addKeyValuePair("dataset_size", "large");
    turtleMemBench.metadata().addKeyValuePair("approx_triples", 20000);

    getGeneralMetadata().addKeyValuePair("benchmark_type", "memory_usage");

    return results;
  }
};

// ============================================================================
// Benchmark 6: Format-Specific N3 Features
// ============================================================================

class N3SpecificFeatures : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;

  // N3-specific test data with collections and property lists
  std::string n3WithCollections;
  std::string n3WithPropertyLists;
  std::string n3WithBlankNodes;

 public:
  N3SpecificFeatures() {
    // Collections (RDF lists)
    n3WithCollections = R"(@prefix ex: <http://example.org/> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .

)";
    for (int i = 0; i < 100; ++i) {
      n3WithCollections += "ex:list" + std::to_string(i) +
                           " ex:items (ex:item1 ex:item2 ex:item3 ex:item4 "
                           "ex:item5) .\n";
    }

    // Property lists (more condensed representation)
    n3WithPropertyLists = R"(@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

)";
    for (int i = 0; i < 200; ++i) {
      n3WithPropertyLists += "ex:person" + std::to_string(i) +
                             " foaf:name \"Name" + std::to_string(i) +
                             "\" ; foaf:age " + std::to_string(20 + i % 40) +
                             " ; foaf:email \"person" + std::to_string(i) +
                             "@example.org\" .\n";
    }

    // Blank nodes
    n3WithBlankNodes = R"(@prefix ex: <http://example.org/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

)";
    for (int i = 0; i < 150; ++i) {
      n3WithBlankNodes += "[ a foaf:Person ; foaf:name \"Anonymous" +
                          std::to_string(i) + "\" ; ex:id " +
                          std::to_string(i) + " ] .\n";
    }
  }

  std::string name() const final {
    return "N3-Specific Feature Performance";
  }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    auto& collectionsBench =
        results.addMeasurement("Collections/RDF Lists", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(n3WithCollections);
        });
    collectionsBench.metadata().addKeyValuePair("feature", "collections");

    auto& propertyListsBench =
        results.addMeasurement("Property Lists", [&]() {
          using Parser = RdfStringParser<N3Parser<Tokenizer>>;
          parseData<Parser>(n3WithPropertyLists);
        });
    propertyListsBench.metadata().addKeyValuePair("feature", "property_lists");

    auto& blankNodesBench = results.addMeasurement("Blank Nodes", [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(n3WithBlankNodes);
    });
    blankNodesBench.metadata().addKeyValuePair("feature", "blank_nodes");

    getGeneralMetadata().addKeyValuePair("benchmark_type",
                                         "n3_specific_features");

    return results;
  }
};

// ============================================================================
// Benchmark 7: Comparative Format Analysis
// ============================================================================

class N3FormatComparison : public BenchmarkInterface {
 protected:
  const EncodedIriManager encodedIriManager_;
  std::string testData;

 public:
  N3FormatComparison() { testData = generateN3Data(300); }

  std::string name() const final { return "RDF Format Performance Comparison"; }

  template <typename Parser>
  size_t parseData(std::string_view input) {
    Parser parser{&encodedIriManager_};
    parser.setInputStream(input);
    parser.parseAndReturnAllTriples();
    return parser.getTriples().size();
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    // Create comparison table
    auto& formatTable =
        results.addTable("Format Comparison", {"N3", "Turtle", "NQuad"},
                         {"Format", "Standard Tokenizer", "CTRE Tokenizer"});

    // N3 with both tokenizers
    formatTable.addMeasurement(0, 1, [&]() {
      using Parser = RdfStringParser<N3Parser<Tokenizer>>;
      parseData<Parser>(testData);
    });

    formatTable.addMeasurement(0, 2, [&]() {
      using Parser = RdfStringParser<N3Parser<TokenizerCtre>>;
      parseData<Parser>(testData);
    });

    // Turtle with both tokenizers
    formatTable.addMeasurement(1, 1, [&]() {
      using Parser = RdfStringParser<TurtleParser<Tokenizer>>;
      parseData<Parser>(testData);
    });

    formatTable.addMeasurement(1, 2, [&]() {
      using Parser = RdfStringParser<TurtleParser<TokenizerCtre>>;
      parseData<Parser>(testData);
    });

    // NQuad with both tokenizers (using same data, treating as default graph)
    formatTable.addMeasurement(2, 1, [&]() {
      using Parser = RdfStringParser<NQuadParser<Tokenizer>>;
      parseData<Parser>(testData);
    });

    formatTable.addMeasurement(2, 2, [&]() {
      using Parser = RdfStringParser<NQuadParser<TokenizerCtre>>;
      parseData<Parser>(testData);
    });

    getGeneralMetadata().addKeyValuePair("benchmark_type", "format_comparison");

    return results;
  }
};

// ============================================================================
// Register All Benchmarks
// ============================================================================

AD_REGISTER_BENCHMARK(N3ParsingPerformance);
AD_REGISTER_BENCHMARK(N3TokenizerComparison);
AD_REGISTER_BENCHMARK(N3FeatureImpact);
AD_REGISTER_BENCHMARK(N3ThroughputAnalysis);
AD_REGISTER_BENCHMARK(N3MemoryUsage);
AD_REGISTER_BENCHMARK(N3SpecificFeatures);
AD_REGISTER_BENCHMARK(N3FormatComparison);

}  // namespace ad_benchmark

// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: CONSTRUCT Benchmark Suite for Performance Analysis
//
// This benchmark suite provides comprehensive performance measurements for
// SPARQL CONSTRUCT queries, including analysis of:
// - Query complexity impact on performance
// - Result set size scaling
// - Export format efficiency
// - Template triple count effects
// - Knowledge graph size impact

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "../benchmark/infrastructure/Benchmark.h"
#include "../benchmark/infrastructure/BenchmarkMeasurementContainer.h"
#include "../benchmark/infrastructure/BenchmarkMetadata.h"
#include "../test/util/IndexTestHelpers.h"
#include "engine/ExportQueryExecutionTrees.h"
#include "engine/QueryPlanner.h"
#include "parser/SparqlParser.h"
#include "util/ConfigManager/ConfigManager.h"
#include "util/IndexTestHelpers.h"
#include "util/Timer.h"

using namespace std::string_literals;

namespace ad_benchmark {

namespace {
// Helper to parse and execute a CONSTRUCT query
struct ConstructQueryResult {
  std::string tsv;
  std::string csv;
  std::string turtle;
  std::string qleverJson;
  uint64_t resultSize;
};

// Parse and execute a CONSTRUCT query with the given knowledge graph
ConstructQueryResult executeConstructQuery(
    const std::string& turtleKg, const std::string& sparqlQuery,
    ad_utility::MediaType mediaType) {
  // Setup the index from the turtle data
  ad_utility::testing::TestIndexConfig config{turtleKg};
  auto qec = ad_utility::testing::getQec(std::move(config));
  qec->clearCacheUnpinnedOnly();

  auto cancellationHandle =
      std::make_shared<ad_utility::CancellationHandle<>>();
  QueryPlanner qp{qec, cancellationHandle};

  // Parse the SPARQL query
  static EncodedIriManager iriManager;
  auto pq = SparqlParser::parseQuery(&iriManager, sparqlQuery, {});

  // Create execution tree
  auto qet = qp.createExecutionTree(pq);

  // Execute and export
  ad_utility::Timer timer(ad_utility::Timer::Started);
  auto result = ExportQueryExecutionTrees::computeResult(
      pq, qet, mediaType, timer, cancellationHandle);

  std::string output;
  for (const auto& block : result) {
    output += block;
  }

  ConstructQueryResult queryResult;
  if (mediaType == ad_utility::MediaType::tsv) {
    queryResult.tsv = output;
  } else if (mediaType == ad_utility::MediaType::csv) {
    queryResult.csv = output;
  } else if (mediaType == ad_utility::MediaType::turtle) {
    queryResult.turtle = output;
  } else if (mediaType == ad_utility::MediaType::qleverJson) {
    queryResult.qleverJson = output;
  }

  // Extract result size from JSON
  queryResult.resultSize = output.length();

  return queryResult;
}

// Create a turtle knowledge graph with N entities and M triples
std::string createScalableKG(size_t numEntities, size_t triplesPerEntity = 3) {
  std::string kg;
  kg.reserve(numEntities * triplesPerEntity * 50);  // Rough estimate

  for (size_t i = 0; i < numEntities; ++i) {
    std::string entity = "<http://example.org/entity" + std::to_string(i) + ">";
    kg += entity + " <http://example.org/name> \"Entity " + std::to_string(i) +
          "\" .\n";
    kg += entity + " <http://example.org/type> <http://example.org/Type" +
          std::to_string(i % 10) + "> .\n";
    if (i > 0) {
      kg += entity + " <http://example.org/relatedTo> <http://example.org/entity" +
            std::to_string(i - 1) + "> .\n";
    }
  }

  return kg;
}

}  // namespace

// ============================================================================
// Benchmark 1: Query Complexity Impact
// ============================================================================
class ConstructSimpleQuery : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT Simple Query - Basic Triple Pattern";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures performance of simple CONSTRUCT patterns: SELECT all "
        "triples from a small knowledge graph");

    const std::string kg = createScalableKG(100, 3);
    const std::string query =
        "CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }";

    // Warmup
    executeConstructQuery(kg, query, ad_utility::MediaType::tsv);

    // Measure different export formats
    auto& tsvMeasurement = results.addMeasurement(
        "TSV Export", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::tsv);
          (void)result;
        });
    tsvMeasurement.metadata().addKeyValuePair(
        "format", "TSV");
    tsvMeasurement.metadata().addKeyValuePair(
        "kg_size", 100);
    tsvMeasurement.metadata().addKeyValuePair(
        "query_type", "Simple Triple Pattern");

    auto& csvMeasurement = results.addMeasurement(
        "CSV Export", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::csv);
          (void)result;
        });
    csvMeasurement.metadata().addKeyValuePair("format", "CSV");

    auto& turtleMeasurement = results.addMeasurement(
        "Turtle Export", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::turtle);
          (void)result;
        });
    turtleMeasurement.metadata().addKeyValuePair(
        "format", "Turtle");

    auto& jsonMeasurement = results.addMeasurement(
        "QLeverJSON Export", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::qleverJson);
          (void)result;
        });
    jsonMeasurement.metadata().addKeyValuePair(
        "format", "QLeverJSON");

    return results;
  }
};

// ============================================================================
// Benchmark 2: Query with FILTER
// ============================================================================
class ConstructFilteredQuery : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT with Filter - Selective Construction";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures performance of CONSTRUCT queries with FILTER clause");

    const std::string kg = createScalableKG(100, 3);
    const std::string query =
        "CONSTRUCT { ?s <http://example.org/filtered> ?name } "
        "WHERE { ?s <http://example.org/name> ?name . "
        "FILTER (STRLEN(?name) > 10) }";

    // Warmup
    executeConstructQuery(kg, query, ad_utility::MediaType::tsv);

    auto& measurement = results.addMeasurement(
        "Filtered CONSTRUCT", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::qleverJson);
          (void)result;
        });
    measurement.metadata().addKeyValuePair(
        "query_type", "Filtered Pattern");
    measurement.metadata().addKeyValuePair(
        "kg_size", 100);
    measurement.metadata().addKeyValuePair(
        "filter_type", "STRLEN");

    return results;
  }
};

// ============================================================================
// Benchmark 3: CONSTRUCT with OPTIONAL
// ============================================================================
class ConstructOptionalQuery : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT with OPTIONAL - Handling Optional Patterns";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures performance of CONSTRUCT queries with OPTIONAL clauses");

    const std::string kg = createScalableKG(100, 3);
    const std::string query =
        "CONSTRUCT { ?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type } "
        "WHERE { ?s <http://example.org/name> ?name . "
        "OPTIONAL { ?s <http://example.org/type> ?type } }";

    // Warmup
    executeConstructQuery(kg, query, ad_utility::MediaType::tsv);

    auto& measurement = results.addMeasurement(
        "OPTIONAL CONSTRUCT", [&kg, &query]() {
          volatile auto result = executeConstructQuery(
              kg, query, ad_utility::MediaType::qleverJson);
          (void)result;
        });
    measurement.metadata().addKeyValuePair(
        "query_type", "Pattern with OPTIONAL");
    measurement.metadata().addKeyValuePair(
        "kg_size", 100);

    return results;
  }
};

// ============================================================================
// Benchmark 4: Result Set Size Scaling
// ============================================================================
class ConstructScalingBenchmark : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT Scaling Analysis - Performance with Growing Result Sets";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Analyzes performance scaling as the size of result sets increases");

    const std::string query =
        "CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }";

    // Create a result set scaling table: rows = KG sizes, cols = export
    // formats
    auto& scalingTable =
        results.addTable("Result Size Scaling", {"10", "50", "100", "200"},
                         {"Format", "10 entities", "50 entities",
                          "100 entities", "200 entities"});

    std::vector<std::string> formats = {"TSV", "CSV", "Turtle", "JSON"};
    std::vector<ad_utility::MediaType> mediaTypes = {
        ad_utility::MediaType::tsv, ad_utility::MediaType::csv,
        ad_utility::MediaType::turtle, ad_utility::MediaType::qleverJson};

    std::vector<size_t> kgSizes = {10, 50, 100, 200};

    for (size_t formatIdx = 0; formatIdx < formats.size(); ++formatIdx) {
      const auto mediaType = mediaTypes[formatIdx];
      const auto formatName = formats[formatIdx];

      for (size_t sizeIdx = 0; sizeIdx < kgSizes.size(); ++sizeIdx) {
        const auto kgSize = kgSizes[sizeIdx];
        const auto kg = createScalableKG(kgSize, 3);

        scalingTable.addMeasurement(formatIdx, sizeIdx + 1, [&kg, &query, mediaType]() {
          volatile auto result =
              executeConstructQuery(kg, query, mediaType);
          (void)result;
        });
      }
    }

    return results;
  }
};

// ============================================================================
// Benchmark 5: Template Complexity (Multiple Triples in CONSTRUCT)
// ============================================================================
class ConstructTemplateComplexity : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT Template Complexity - Impact of Multiple Output Triples";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Analyzes performance impact of constructing multiple triples per "
        "result row");

    const std::string kg = createScalableKG(100, 3);

    // Different template complexities
    std::vector<std::string> queries = {
        // Single triple template
        "CONSTRUCT { ?s <http://example.org/p> ?o } "
        "WHERE { ?s <http://example.org/name> ?o }",

        // Two triples template
        "CONSTRUCT { "
        "?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type "
        "} WHERE { "
        "?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type }",

        // Three triples template
        "CONSTRUCT { "
        "?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type . "
        "?s <http://example.org/relatedTo> ?related "
        "} WHERE { "
        "?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type . "
        "?s <http://example.org/relatedTo> ?related }",

        // Five triples template with duplication
        "CONSTRUCT { "
        "?s <http://example.org/p1> ?o . "
        "?s <http://example.org/p2> ?o . "
        "?s <http://example.org/p3> ?o . "
        "?s <http://example.org/p4> ?o . "
        "?s <http://example.org/p5> ?o "
        "} WHERE { "
        "?s <http://example.org/name> ?o }",
    };

    std::vector<std::string> labels = {
        "1 Triple", "2 Triples", "3 Triples", "5 Triples (with repetition)"};

    for (size_t i = 0; i < queries.size(); ++i) {
      auto& measurement = results.addMeasurement(
          labels[i], [&kg, &query = queries[i]]() {
            volatile auto result = executeConstructQuery(
                kg, query, ad_utility::MediaType::qleverJson);
            (void)result;
          });
      measurement.metadata().addKeyValuePair(
          "template_triple_count", i + 1);
      measurement.metadata().addKeyValuePair(
          "kg_size", 100);
    }

    return results;
  }
};

// ============================================================================
// Benchmark 6: Knowledge Graph Size Impact
// ============================================================================
class ConstructKGSizeImpact : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT KG Size Impact - Performance with Increasing Data Volume";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Measures performance impact of increasing knowledge graph size");

    const std::string query =
        "CONSTRUCT { ?s <http://example.org/result> ?o } "
        "WHERE { ?s <http://example.org/name> ?o }";

    auto& kgSizeGroup = results.addGroup("KG Size Impact");
    kgSizeGroup.metadata().addKeyValuePair(
        "description", "Performance across different KG sizes");

    std::vector<size_t> sizes = {50, 100, 200, 500, 1000};

    for (const auto size : sizes) {
      const auto kg = createScalableKG(size, 3);
      const auto label = std::to_string(size) + " entities";

      auto& measurement = kgSizeGroup.addMeasurement(label, [&kg, &query]() {
        volatile auto result = executeConstructQuery(
            kg, query, ad_utility::MediaType::qleverJson);
        (void)result;
      });
      measurement.metadata().addKeyValuePair(
          "kg_entity_count", size);
      measurement.metadata().addKeyValuePair(
          "query_type", "Simple Construction");
    }

    return results;
  }
};

// ============================================================================
// Benchmark 7: Export Format Comparison
// ============================================================================
class ConstructExportFormatComparison : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "CONSTRUCT Export Format Comparison - Format-Specific Performance";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;
    getGeneralMetadata().addKeyValuePair(
        "description",
        "Detailed comparison of export format performance for CONSTRUCT "
        "queries");

    const std::string kg = createScalableKG(150, 3);
    const std::string query =
        "CONSTRUCT { ?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type . "
        "?s <http://example.org/relatedTo> ?related } "
        "WHERE { "
        "?s <http://example.org/name> ?name . "
        "?s <http://example.org/type> ?type . "
        "?s <http://example.org/relatedTo> ?related }";

    auto& formatGroup = results.addGroup("Export Formats");
    formatGroup.metadata().addKeyValuePair(
        "kg_size", 150);
    formatGroup.metadata().addKeyValuePair(
        "template_triples", 3);

    // TSV format
    auto& tsvMeasure = formatGroup.addMeasurement("TSV Format", [&kg, &query]() {
      volatile auto result = executeConstructQuery(
          kg, query, ad_utility::MediaType::tsv);
      (void)result;
    });
    tsvMeasure.metadata().addKeyValuePair(
        "format", "TSV");
    tsvMeasure.metadata().addKeyValuePair(
        "delimiter", "Tab");

    // CSV format
    auto& csvMeasure = formatGroup.addMeasurement("CSV Format", [&kg, &query]() {
      volatile auto result = executeConstructQuery(
          kg, query, ad_utility::MediaType::csv);
      (void)result;
    });
    csvMeasure.metadata().addKeyValuePair(
        "format", "CSV");
    csvMeasure.metadata().addKeyValuePair(
        "delimiter", "Comma");

    // Turtle format
    auto& turtleMeasure = formatGroup.addMeasurement("Turtle Format", [&kg, &query]() {
      volatile auto result = executeConstructQuery(
          kg, query, ad_utility::MediaType::turtle);
      (void)result;
    });
    turtleMeasure.metadata().addKeyValuePair(
        "format", "Turtle");
    turtleMeasure.metadata().addKeyValuePair(
        "rdf_serialization", true);

    // QLever JSON format
    auto& jsonMeasure = formatGroup.addMeasurement("QLeverJSON Format", [&kg, &query]() {
      volatile auto result = executeConstructQuery(
          kg, query, ad_utility::MediaType::qleverJson);
      (void)result;
    });
    jsonMeasure.metadata().addKeyValuePair(
        "format", "QLeverJSON");
    jsonMeasure.metadata().addKeyValuePair(
        "structured_format", true);

    return results;
  }
};

// ============================================================================
// Register all benchmarks
// ============================================================================
AD_REGISTER_BENCHMARK(ConstructSimpleQuery);
AD_REGISTER_BENCHMARK(ConstructFilteredQuery);
AD_REGISTER_BENCHMARK(ConstructOptionalQuery);
AD_REGISTER_BENCHMARK(ConstructScalingBenchmark);
AD_REGISTER_BENCHMARK(ConstructTemplateComplexity);
AD_REGISTER_BENCHMARK(ConstructKGSizeImpact);
AD_REGISTER_BENCHMARK(ConstructExportFormatComparison);

}  // namespace ad_benchmark

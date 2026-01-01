// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant

#include <memory>
#include <string>
#include <vector>

#include "../benchmark/infrastructure/Benchmark.h"
#include "../benchmark/infrastructure/BenchmarkMeasurementContainer.h"
#include "../test/util/IdTestHelpers.h"
#include "../test/util/IndexTestHelpers.h"
#include "engine/ExportQueryExecutionTrees.h"
#include "engine/QueryPlanner.h"
#include "parser/SparqlParser.h"
#include "util/CancellationHandle.h"
#include "util/Timer.h"

using namespace std::string_literals;

namespace ad_benchmark {

/*
 * Benchmarks for SPARQL CONSTRUCT queries.
 * Tests performance of CONSTRUCT query execution with various data sizes
 * and query complexities.
 */
class ConstructBenchmark : public BenchmarkInterface {
 private:
  // Helper to parse and execute a CONSTRUCT query
  nlohmann::json executeCONSTRUCTQuery(const std::string& turtleData,
                                       const std::string& query,
                                       ad_utility::MediaType mediaType) {
    ad_utility::testing::TestIndexConfig config{turtleData};
    auto qec = ad_utility::testing::getQec(std::move(config));
    qec->clearCacheUnpinnedOnly();

    auto cancellationHandle =
        std::make_shared<ad_utility::CancellationHandle<>>();
    QueryPlanner qp{qec, cancellationHandle};

    static EncodedIriManager iriManager;
    auto pq = SparqlParser::parseQuery(&iriManager, query, {});
    auto qet = qp.createExecutionTree(pq);

    ad_utility::Timer timer(ad_utility::Timer::Started);
    std::string result;
    for (const auto& block : ExportQueryExecutionTrees::computeResult(
             pq, qet, mediaType, timer, std::move(cancellationHandle))) {
      result += block;
    }
    return nlohmann::json::parse(result);
  }

  using enum ad_utility::MediaType;

  // Generate a Turtle knowledge graph with n triples
  std::string generateTurtleKG(size_t numTriples) {
    std::string kg;
    for (size_t i = 0; i < numTriples; ++i) {
      kg += "<http://example.org/subject" + std::to_string(i) + "> "
            "<http://example.org/property" + std::to_string(i % 10) + "> "
            "\"value" + std::to_string(i) + "\" .\n";
    }
    return kg;
  }

 public:
  std::string name() const final { return "CONSTRUCT Query Benchmarks"; }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results;

    // Benchmark 1: Simple CONSTRUCT with small dataset
    results.addMeasurement(
        "Simple CONSTRUCT with 100 triples",
        [this]() {
          std::string kg = generateTurtleKG(100);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 2: Simple CONSTRUCT with medium dataset
    results.addMeasurement(
        "Simple CONSTRUCT with 1000 triples",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 3: Simple CONSTRUCT with large dataset
    results.addMeasurement(
        "Simple CONSTRUCT with 10000 triples",
        [this]() {
          std::string kg = generateTurtleKG(10000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 4: CONSTRUCT with pattern rewriting (fewer triples)
    results.addMeasurement(
        "Pattern rewriting CONSTRUCT (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s <http://example.org/rewritten> ?o }
            WHERE { ?s ?p ?o }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 5: CONSTRUCT with filter
    results.addMeasurement(
        "CONSTRUCT with FILTER (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE {
              ?s ?p ?o
              FILTER(STRSTARTS(STR(?o), "value1"))
            }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 6: CONSTRUCT with ORDER BY
    results.addMeasurement(
        "CONSTRUCT with ORDER BY (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
            ORDER BY ?s
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 7: CONSTRUCT with LIMIT
    results.addMeasurement(
        "CONSTRUCT with LIMIT (1000 triples, limit 100)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
            LIMIT 100
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 8: CONSTRUCT to TSV format
    results.addMeasurement(
        "CONSTRUCT export to TSV format (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT { ?s ?p ?o }
            WHERE { ?s ?p ?o }
          )";
          ad_utility::testing::TestIndexConfig config{kg};
          auto qec = ad_utility::testing::getQec(std::move(config));
          qec->clearCacheUnpinnedOnly();

          auto cancellationHandle =
              std::make_shared<ad_utility::CancellationHandle<>>();
          QueryPlanner qp{qec, cancellationHandle};

          static EncodedIriManager iriManager;
          auto pq = SparqlParser::parseQuery(&iriManager, query, {});
          auto qet = qp.createExecutionTree(pq);

          ad_utility::Timer timer(ad_utility::Timer::Started);
          std::string result;
          for (const auto& block : ExportQueryExecutionTrees::computeResult(
                   pq, qet, tsv, timer,
                   std::move(cancellationHandle))) {
            result += block;
          }
        });

    // Benchmark 9: Multiple pattern CONSTRUCT
    results.addMeasurement(
        "Multiple pattern CONSTRUCT (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT {
              ?s <http://example.org/type> ?type ;
                 <http://example.org/value> ?o ;
                 <http://example.org/relation> ?s2 .
              ?s2 <http://example.org/inverse> ?s .
            }
            WHERE {
              ?s ?p ?o
              BIND(?p AS ?type)
              BIND(?s AS ?s2)
            }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    // Benchmark 10: CONSTRUCT with OPTIONAL
    results.addMeasurement(
        "CONSTRUCT with OPTIONAL (1000 triples)",
        [this]() {
          std::string kg = generateTurtleKG(1000);
          std::string query = R"(
            CONSTRUCT {
              ?s ?p ?o ;
                 ?p2 ?o2 .
            }
            WHERE {
              ?s ?p ?o
              OPTIONAL { ?s ?p2 ?o2 }
            }
          )";
          executeCONSTRUCTQuery(kg, query, qleverJson);
        });

    return results;
  }
};

// Register the benchmark
AD_REGISTER_BENCHMARK(ConstructBenchmark);

}  // namespace ad_benchmark

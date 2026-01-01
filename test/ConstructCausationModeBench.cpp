// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: AI Assistant (Claude)
//
// Benchmarks for CONSTRUCT causation modes.
// These are the 20% that matter: measurable performance gates.
//
// Run: ./ConstructCausationModeBench --benchmark_out=baseline.json
// Compare: compare.py baseline.json current.json

#include <benchmark/benchmark.h>

#include "./util/GTestHelpers.h"
#include "./util/IndexTestHelpers.h"
#include "./util/OperationTestHelpers.h"
#include "engine/QueryExecutionTree.h"
#include "parser/SparqlParser.h"

namespace {

QueryExecutionContext* qec = nullptr;

// Initialize once for all benchmarks
void SetUpBenchmarks() {
  if (!qec) {
    qec = ad_utility::testing::getQec();
  }
}

}  // namespace

// ============================================================================
// BENCHMARK 1: POST-DECISION STATE TRANSFORMATION
// ============================================================================

static void BM_PostDecisionTransform_1K(benchmark::State& state) {
  SetUpBenchmarks();

  // Simple deterministic state transformation
  // Pattern: age -> status (no choice, deterministic function)
  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/status> ?status .
    }
    WHERE {
      ?entity <http://example.org/age> ?age .
      BIND(IF(?age < 18, "minor",
              IF(?age < 65, "adult", "senior")) AS ?status)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_PostDecisionTransform_1K)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_PostDecision_1K");

// Larger dataset
static void BM_PostDecisionTransform_100K(benchmark::State& state) {
  SetUpBenchmarks();

  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/status> ?status .
    }
    WHERE {
      ?entity <http://example.org/age> ?age .
      BIND(IF(?age < 18, "minor",
              IF(?age < 65, "adult", "senior")) AS ?status)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_PostDecisionTransform_100K)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_PostDecision_100K");

// ============================================================================
// BENCHMARK 2: CONTRADICTION DETECTION
// ============================================================================

static void BM_ContradictionDetection_1K(benchmark::State& state) {
  SetUpBenchmarks();

  // Detect entities with conflicting property assignments
  // Pattern: find ?e where ?e ?p ?v1 and ?e ?p ?v2, ?v1 != ?v2
  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/hasConflict> ?conflict .
      ?conflict <http://example.org/property> ?property .
      ?conflict <http://example.org/value1> ?v1 .
      ?conflict <http://example.org/value2> ?v2 .
    }
    WHERE {
      ?entity ?property ?v1 .
      ?entity ?property ?v2 .
      FILTER (?v1 != ?v2)
      BIND(IRI(CONCAT(
        "http://example.org/conflict_", STR(?entity), "_", STR(?property)
      )) AS ?conflict)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_ContradictionDetection_1K)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Contradiction_1K");

static void BM_ContradictionDetection_100K(benchmark::State& state) {
  SetUpBenchmarks();

  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/hasConflict> ?conflict .
    }
    WHERE {
      ?entity ?property ?v1 .
      ?entity ?property ?v2 .
      FILTER (?v1 != ?v2)
      BIND(IRI(CONCAT(
        "http://example.org/conflict_", STR(?entity), "_", STR(?property)
      )) AS ?conflict)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_ContradictionDetection_100K)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Contradiction_100K");

// ============================================================================
// BENCHMARK 3: MECHANICAL COMPATIBILITY CHECK
// ============================================================================

static void BM_CompatibilityCheck_10Systems(benchmark::State& state) {
  SetUpBenchmarks();

  // Which systems can coordinate with which others?
  // Pattern: ?a requires ?i, ?b produces ?i -> compatible
  const std::string query = R"(
    CONSTRUCT {
      ?systemA <http://example.org/compatibleWith> ?systemB .
    }
    WHERE {
      ?systemA <http://example.org/requiresInvariant> ?invariant .
      ?systemB <http://example.org/producesInvariant> ?invariant .
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_CompatibilityCheck_10Systems)
    ->Unit(benchmark::kMicrosecond)
    ->Name("BM_Compatibility_10Systems");

static void BM_CompatibilityCheck_100Systems(benchmark::State& state) {
  SetUpBenchmarks();

  const std::string query = R"(
    CONSTRUCT {
      ?systemA <http://example.org/compatibleWith> ?systemB .
    }
    WHERE {
      ?systemA <http://example.org/requiresInvariant> ?invariant .
      ?systemB <http://example.org/producesInvariant> ?invariant .
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_CompatibilityCheck_100Systems)
    ->Unit(benchmark::kMicrosecond)
    ->Name("BM_Compatibility_100Systems");

// ============================================================================
// BENCHMARK 4: AUDIT TRAIL CREATION
// ============================================================================

static void BM_AuditTrailCreation_SingleChange(benchmark::State& state) {
  SetUpBenchmarks();

  // Create immutable audit trail for state changes
  // Pattern: old value + new value + rule + timestamp
  const std::string query = R"(
    CONSTRUCT {
      ?audit <http://example.org/changedEntity> ?entity .
      ?audit <http://example.org/previousValue> ?oldValue .
      ?audit <http://example.org/newValue> ?newValue .
      ?audit <http://example.org/appliedRule> ?rule .
      ?audit <http://example.org/timestamp> ?time .
    }
    WHERE {
      ?entity <http://example.org/value> ?oldValue .
      ?entity <http://example.org/proposedValue> ?newValue .
      FILTER (?oldValue != ?newValue)
      ?rule <http://example.org/triggers> ?entity .
      BIND(NOW() AS ?time)
      BIND(IRI(CONCAT("http://example.org/audit_",
                      URICODE(STR(?entity)), "_", STR(?time))) AS ?audit)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_AuditTrailCreation_SingleChange)
    ->Unit(benchmark::kMicrosecond)
    ->Name("BM_Audit_SingleChange");

static void BM_AuditTrailCreation_1KChanges(benchmark::State& state) {
  SetUpBenchmarks();

  const std::string query = R"(
    CONSTRUCT {
      ?audit <http://example.org/changedEntity> ?entity .
      ?audit <http://example.org/previousValue> ?oldValue .
      ?audit <http://example.org/newValue> ?newValue .
      ?audit <http://example.org/appliedRule> ?rule .
      ?audit <http://example.org/timestamp> ?time .
    }
    WHERE {
      ?entity <http://example.org/value> ?oldValue .
      ?entity <http://example.org/proposedValue> ?newValue .
      FILTER (?oldValue != ?newValue)
      ?rule <http://example.org/triggers> ?entity .
      BIND(NOW() AS ?time)
      BIND(IRI(CONCAT("http://example.org/audit_",
                      URICODE(STR(?entity)), "_", STR(?time))) AS ?audit)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_AuditTrailCreation_1KChanges)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Audit_1KChanges");

// ============================================================================
// BENCHMARK 5: DRIFT ANALYSIS
// ============================================================================

static void BM_DriftAnalysis_Frequency(benchmark::State& state) {
  SetUpBenchmarks();

  // Compute how frequently entities change (drift rate)
  // Pattern: count changes, compute rate
  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/changeFrequency> ?frequency .
      ?entity <http://example.org/driftRate> ?rate .
    }
    WHERE {
      {
        SELECT ?entity (COUNT(?audit) AS ?frequency)
               (MAX(?time) - MIN(?time) AS ?duration)
        WHERE {
          ?audit <http://example.org/changedEntity> ?entity .
          ?audit <http://example.org/timestamp> ?time .
        }
        GROUP BY ?entity
      }
      BIND(?frequency / (?duration + 1) AS ?rate)
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_DriftAnalysis_Frequency)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Drift_Frequency");

static void BM_DriftAnalysis_Anomalies(benchmark::State& state) {
  SetUpBenchmarks();

  // Detect untracked changes (anomalies)
  // Pattern: changes without matching audit entry
  const std::string query = R"(
    CONSTRUCT {
      ?entity <http://example.org/hasAnomalousChange> "true"^^xsd:boolean .
    }
    WHERE {
      ?entity <http://example.org/value> ?current .
      OPTIONAL {
        ?audit <http://example.org/changedEntity> ?entity .
        ?audit <http://example.org/newValue> ?current .
      }
      FILTER (!BOUND(?audit))
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_DriftAnalysis_Anomalies)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Drift_Anomalies");

// ============================================================================
// INTEGRATION: ALL FIVE MODES TOGETHER
// ============================================================================

static void BM_Integration_MedicalRecords(benchmark::State& state) {
  SetUpBenchmarks();

  // Real-world example: medical records system
  // Combines all five modes:
  // 1. Deterministic patient status (post-decision)
  // 2. Medication conflicts (error as data)
  // 3. Provider compatibility (mechanical)
  // 4. Treatment history (audit trail)
  // 5. Forbidden actions (no psychology)
  const std::string query = R"(
    CONSTRUCT {
      # Mode 1: Deterministic status
      ?patient <http://example.org/medicalStatus> ?status .

      # Mode 2: Explicit conflicts
      ?patient <http://example.org/medicationConflict> ?conflict .

      # Mode 3: Mechanical compatibility
      ?provider <http://example.org/canTreat> ?patient .

      # Mode 4: Audit trail
      ?audit <http://example.org/changedPatient> ?patient .
      ?audit <http://example.org/treatment> ?treatment .
      ?audit <http://example.org/timestamp> ?time .

      # Mode 5: Boundary exposition
      ?action <http://example.org/violatesHIPAA> ?rule .
    }
    WHERE {
      # Load patient
      ?patient <http://example.org/age> ?age .
      OPTIONAL { ?patient <http://example.org/medications> ?meds . }

      # Mode 1: Compute status (deterministic from age)
      BIND(IF(?age < 18, "pediatric",
              IF(?age < 65, "adult", "geriatric")) AS ?status) .

      # Mode 2: Find medication conflicts
      OPTIONAL {
        ?med1 <http://example.org/contraindicated> ?med2 .
        FILTER(?med1 IN (?meds) && ?med2 IN (?meds))
        BIND(CONCAT(?med1, " + ", ?med2) AS ?conflict)
      }

      # Mode 3: Check provider compatibility
      ?provider <http://example.org/certified> ?status .

      # Mode 4: Create audit on treatment
      OPTIONAL {
        ?patient <http://example.org/receivedTreatment> ?treatment .
        FILTER(BOUND(?treatment))
        BIND(NOW() AS ?time)
        BIND(IRI(CONCAT("audit_", URICODE(STR(?patient)))) AS ?audit)
      }

      # Mode 5: Flag forbidden actions
      ?action <http://example.org/violatesPolicy> ?rule .
      ?rule <http://example.org/type> "HIPAA" .
    }
  )";

  auto parsedQuery = SparqlParser::parseQuery(query);

  for (auto _ : state) {
    auto tree =
        QueryExecutionTree::createFromParsedQuery(qec, *parsedQuery);
    auto result = tree->execute();
    benchmark::DoNotOptimize(result);
  }
}

BENCHMARK(BM_Integration_MedicalRecords)
    ->Unit(benchmark::kMillisecond)
    ->Name("BM_Integration_Medical");

// ============================================================================
// MAIN: Run all benchmarks
// ============================================================================

BENCHMARK_MAIN();

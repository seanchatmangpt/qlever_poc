//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude AI Assistant

#ifndef QLEVER_CONFORMANCE_SHACL_CONFORMANCE_RUNNER_H
#define QLEVER_CONFORMANCE_SHACL_CONFORMANCE_RUNNER_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "engine/conformance/ConformanceRunner.h"
#include "engine/conformance/ConformanceTestCase.h"
#include "engine/shacl/ShaclShapeParser.h"
#include "engine/shacl/ShaclShapeRegistry.h"
#include "engine/shacl/ShaclValidator.h"
#include "util/json.h"

namespace ad_engine::conformance {

// SHACL-specific conformance test runner
// Loads test cases from conformance/shacl/ directory
// Executes SHACL validation and compares against expected results
class ShaclConformanceRunner : public ConformanceRunner {
 public:
  // Constructor
  ShaclConformanceRunner();

  // Load all test cases from conformance/shacl/ directory
  std::vector<ConformanceTestCase> loadTestCases(
      const std::string& conformanceDir);

  // Execute a single SHACL test case
  nlohmann::json executeShaclTest(const ConformanceTestCase& testCase);

 protected:
  // Override base class method for SHACL-specific execution
  nlohmann::json executeTest(const ConformanceTestCase& testCase) override;

 private:
  // Helper: Load RDF data from TTL file
  // Returns JSON representation of the loaded triples
  nlohmann::json loadRdfData(const std::string& ttlPath);

  // Helper: Load SHACL shapes from TTL file
  std::unique_ptr<shacl::ShaclShapeRegistry> loadShaclShapes(
      const std::string& shapesPath);

  // Helper: Run validation and convert result to JSON
  nlohmann::json runValidation(const nlohmann::json& data,
                                shacl::ShaclShapeRegistry* shapeRegistry,
                                const nlohmann::json& metadata);

  // Helper: Convert SHACL violations to JSON format
  nlohmann::json violationsToJson(
      const std::vector<shacl::ShaclViolation>& violations);

  // Helper: Check if guard was triggered based on metadata
  bool checkGuardTriggered(const nlohmann::json& metadata,
                           size_t violationsCount, size_t focusNodesCount);
};

}  // namespace ad_engine::conformance

#endif  // QLEVER_CONFORMANCE_SHACL_CONFORMANCE_RUNNER_H

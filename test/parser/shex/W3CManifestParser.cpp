#include "W3CManifestParser.h"
#include <fstream>
#include <iostream>

namespace shex::testing {

// ============================================================================
// W3CManifestParser Implementation
// ============================================================================

std::vector<W3CTestCase> W3CManifestParser::parseManifest(
    const std::filesystem::path& manifestPath) {
  std::vector<W3CTestCase> testCases;

  try {
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
      lastError_ = "Failed to open manifest file: " + manifestPath.string();
      return testCases;
    }

    nlohmann::json manifest;
    file >> manifest;

    if (!validateManifestStructure(manifest)) {
      lastError_ = "Invalid manifest structure in: " + manifestPath.string();
      return testCases;
    }

    testCases = parseJSONLDManifest(manifest, manifestPath);

  } catch (const std::exception& e) {
    lastError_ = "Error parsing manifest " + manifestPath.string() + ": " + e.what();
  }

  return testCases;
}

std::vector<W3CTestCase> W3CManifestParser::parseManifestsInDirectory(
    const std::filesystem::path& directory) {
  std::vector<W3CTestCase> allTestCases;

  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
      if (entry.is_regular_file() &&
          (entry.path().filename() == "manifest.json" ||
           entry.path().filename() == "manifest.jsonld")) {
        auto testCases = parseManifest(entry.path());
        allTestCases.insert(allTestCases.end(), testCases.begin(), testCases.end());
      }
    }
  } catch (const std::exception& e) {
    lastError_ = "Error scanning directory " + directory.string() + ": " + e.what();
  }

  return allTestCases;
}

std::vector<W3CTestCase> W3CManifestParser::parseJSONLDManifest(
    const nlohmann::json& manifest,
    const std::filesystem::path& manifestPath) {
  std::vector<W3CTestCase> testCases;

  // Handle both array and object manifest formats
  if (manifest.is_array()) {
    for (const auto& entry : manifest) {
      auto testCase = parseTestEntry(entry, manifestPath);
      if (testCase) {
        updateStatistics(*testCase);
        testCases.push_back(*testCase);
      }
    }
  } else if (manifest.is_object()) {
    // Handle JSON-LD context-based format
    if (manifest.contains("@graph")) {
      for (const auto& entry : manifest["@graph"]) {
        auto testCase = parseTestEntry(entry, manifestPath);
        if (testCase) {
          updateStatistics(*testCase);
          testCases.push_back(*testCase);
        }
      }
    } else if (manifest.contains("entries")) {
      for (const auto& entry : manifest["entries"]) {
        auto testCase = parseTestEntry(entry, manifestPath);
        if (testCase) {
          updateStatistics(*testCase);
          testCases.push_back(*testCase);
        }
      }
    }
  }

  return testCases;
}

std::optional<W3CTestCase> W3CManifestParser::parseTestEntry(
    const nlohmann::json& entry,
    const std::filesystem::path& manifestPath) {
  try {
    W3CTestCase testCase;

    // Extract ID
    if (entry.contains("@id")) {
      testCase.id = entry["@id"].get<std::string>();
    } else if (entry.contains("id")) {
      testCase.id = entry["id"].get<std::string>();
    } else {
      return std::nullopt;  // ID is required
    }

    // Extract name
    if (entry.contains("name")) {
      testCase.name = entry["name"].get<std::string>();
    } else {
      testCase.name = testCase.id;
    }

    // Determine test type
    testCase.type = determineTestType(entry);

    // Extract schema file
    if (entry.contains("schema")) {
      testCase.schemaFile = resolvePath(entry["schema"].get<std::string>(), manifestPath);
    } else if (entry.contains("action") && entry["action"].is_object()) {
      if (entry["action"].contains("schema")) {
        testCase.schemaFile = resolvePath(entry["action"]["schema"].get<std::string>(), manifestPath);
      }
    }

    // Extract data file (for validation tests)
    if (entry.contains("data")) {
      testCase.dataFile = resolvePath(entry["data"].get<std::string>(), manifestPath);
    } else if (entry.contains("action") && entry["action"].is_object()) {
      if (entry["action"].contains("data")) {
        testCase.dataFile = resolvePath(entry["action"]["data"].get<std::string>(), manifestPath);
      }
    }

    // Extract shape map
    if (entry.contains("shapeMap")) {
      testCase.shapeMap = entry["shapeMap"].get<std::string>();
    } else if (entry.contains("action") && entry["action"].is_object()) {
      if (entry["action"].contains("shapeMap")) {
        testCase.shapeMap = entry["action"]["shapeMap"].get<std::string>();
      }
    }

    // Determine expected result
    if (entry.contains("@type")) {
      std::string type = entry["@type"].get<std::string>();
      testCase.shouldPass = (type.find("Negative") == std::string::npos);
    } else if (entry.contains("type")) {
      std::string type = entry["type"].get<std::string>();
      testCase.shouldPass = (type.find("Negative") == std::string::npos);
    }

    // Extract features
    testCase.features = extractFeatures(entry);

    // Extract category
    testCase.category = extractCategory(entry);

    // Extract comment
    if (entry.contains("comment")) {
      testCase.comment = entry["comment"].get<std::string>();
    } else if (entry.contains("rdfs:comment")) {
      testCase.comment = entry["rdfs:comment"].get<std::string>();
    }

    // Set metadata
    testCase.manifestPath = manifestPath.string();

    return testCase;

  } catch (const std::exception& e) {
    std::cerr << "Error parsing test entry: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::string W3CManifestParser::resolvePath(const std::string& relativePath,
                                           const std::filesystem::path& manifestPath) {
  std::filesystem::path basePath = manifestPath.parent_path();
  std::filesystem::path fullPath = basePath / relativePath;
  return std::filesystem::absolute(fullPath).string();
}

TestType W3CManifestParser::determineTestType(const nlohmann::json& entry) {
  std::string typeStr;

  if (entry.contains("@type")) {
    typeStr = entry["@type"].get<std::string>();
  } else if (entry.contains("type")) {
    typeStr = entry["type"].get<std::string>();
  }

  if (typeStr.find("ValidationTest") != std::string::npos ||
      typeStr.find("Validation") != std::string::npos) {
    return TestType::VALIDATION;
  } else if (typeStr.find("NegativeSyntax") != std::string::npos) {
    return TestType::NEGATIVE_SYNTAX;
  } else if (typeStr.find("NegativeStructure") != std::string::npos) {
    return TestType::NEGATIVE_STRUCTURE;
  } else if (typeStr.find("PositiveSyntax") != std::string::npos) {
    return TestType::POSITIVE_SYNTAX;
  } else if (typeStr.find("RepresentativeSyntax") != std::string::npos) {
    return TestType::REPRESENTATIVE_SYNTAX;
  }

  // Default to validation test
  return TestType::VALIDATION;
}

std::vector<std::string> W3CManifestParser::extractFeatures(const nlohmann::json& entry) {
  std::vector<std::string> features;

  if (entry.contains("trait")) {
    if (entry["trait"].is_array()) {
      for (const auto& trait : entry["trait"]) {
        features.push_back(trait.get<std::string>());
      }
    } else {
      features.push_back(entry["trait"].get<std::string>());
    }
  }

  if (entry.contains("feature")) {
    if (entry["feature"].is_array()) {
      for (const auto& feature : entry["feature"]) {
        features.push_back(feature.get<std::string>());
      }
    } else {
      features.push_back(entry["feature"].get<std::string>());
    }
  }

  // Infer features from test name/ID if not explicitly specified
  if (features.empty()) {
    std::string id = entry.value("@id", entry.value("id", ""));
    if (id.find("cardinality") != std::string::npos) features.push_back("cardinality");
    if (id.find("nodeKind") != std::string::npos) features.push_back("nodeKind");
    if (id.find("datatype") != std::string::npos) features.push_back("datatype");
    if (id.find("closed") != std::string::npos) features.push_back("closedShapes");
    if (id.find("extend") != std::string::npos) features.push_back("extends");
  }

  return features;
}

std::string W3CManifestParser::extractCategory(const nlohmann::json& entry) {
  std::string category = "general";

  if (entry.contains("category")) {
    category = entry["category"].get<std::string>();
  } else {
    // Infer category from features
    auto features = extractFeatures(entry);
    if (!features.empty()) {
      category = features[0];
    }
  }

  return category;
}

void W3CManifestParser::updateStatistics(const W3CTestCase& testCase) {
  stats_.totalTests++;

  switch (testCase.type) {
    case TestType::VALIDATION:
      stats_.validationTests++;
      break;
    case TestType::NEGATIVE_SYNTAX:
      stats_.negativeSyntaxTests++;
      break;
    case TestType::NEGATIVE_STRUCTURE:
      stats_.negativeStructureTests++;
      break;
    case TestType::POSITIVE_SYNTAX:
      stats_.positiveSyntaxTests++;
      break;
    case TestType::REPRESENTATIVE_SYNTAX:
      stats_.representativeSyntaxTests++;
      break;
  }

  stats_.testsByCategory[testCase.category]++;

  for (const auto& feature : testCase.features) {
    stats_.testsByFeature[feature]++;
  }
}

bool W3CManifestParser::validateManifestStructure(const nlohmann::json& manifest) {
  // Accept both array and object formats
  if (!manifest.is_array() && !manifest.is_object()) {
    return false;
  }

  // For objects, require @graph or entries
  if (manifest.is_object()) {
    return manifest.contains("@graph") || manifest.contains("entries");
  }

  return true;
}

// ============================================================================
// TestCaseFilter Implementation
// ============================================================================

std::vector<W3CTestCase> TestCaseFilter::filterByType(
    const std::vector<W3CTestCase>& tests, TestType type) {
  std::vector<W3CTestCase> filtered;
  for (const auto& test : tests) {
    if (test.type == type) {
      filtered.push_back(test);
    }
  }
  return filtered;
}

std::vector<W3CTestCase> TestCaseFilter::filterByFeature(
    const std::vector<W3CTestCase>& tests, const std::string& feature) {
  std::vector<W3CTestCase> filtered;
  for (const auto& test : tests) {
    for (const auto& f : test.features) {
      if (f == feature) {
        filtered.push_back(test);
        break;
      }
    }
  }
  return filtered;
}

std::vector<W3CTestCase> TestCaseFilter::filterByCategory(
    const std::vector<W3CTestCase>& tests, const std::string& category) {
  std::vector<W3CTestCase> filtered;
  for (const auto& test : tests) {
    if (test.category == category) {
      filtered.push_back(test);
    }
  }
  return filtered;
}

std::vector<W3CTestCase> TestCaseFilter::filterByExpectedResult(
    const std::vector<W3CTestCase>& tests, bool shouldPass) {
  std::vector<W3CTestCase> filtered;
  for (const auto& test : tests) {
    if (test.shouldPass == shouldPass) {
      filtered.push_back(test);
    }
  }
  return filtered;
}

absl::flat_hash_set<std::string> TestCaseFilter::getUniqueFeatures(
    const std::vector<W3CTestCase>& tests) {
  absl::flat_hash_set<std::string> features;
  for (const auto& test : tests) {
    for (const auto& feature : test.features) {
      features.insert(feature);
    }
  }
  return features;
}

absl::flat_hash_set<std::string> TestCaseFilter::getUniqueCategories(
    const std::vector<W3CTestCase>& tests) {
  absl::flat_hash_set<std::string> categories;
  for (const auto& test : tests) {
    categories.insert(test.category);
  }
  return categories;
}

}  // namespace shex::testing

// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// N3 Format Compliance Verification Utility
// Analyzes N3 files for feature usage and compatibility with QLever's parser

#ifndef QLEVER_SRC_UTIL_N3COMPLIANCEVERIFIER_H
#define QLEVER_SRC_UTIL_N3COMPLIANCEVERIFIER_H

#include <map>
#include <string>
#include <vector>

namespace ad_utility {

// Represents a single compliance issue found in an N3 file
struct N3ComplianceIssue {
  std::string feature;
  std::string description;
  size_t lineNumber;
  std::string lineContent;
  bool isSupported;
};

// Main class for verifying N3 file compliance with QLever's parser
class N3ComplianceVerifier {
 public:
  // Analyze a given N3 file for compliance
  void analyzeFile(const std::string& filename);

  // Check if the analyzed file is compatible with QLever
  bool isCompatible() const;

  // Generate a detailed compliance report in Markdown format
  std::string generateReport() const;

  // Get a feature matrix showing which features are used and supported
  std::map<std::string, bool> featureMatrix() const;

  // Get list of all detected issues
  const std::vector<N3ComplianceIssue>& getIssues() const { return issues_; }

  // Get summary statistics
  size_t getTotalLines() const { return totalLines_; }
  size_t getUnsupportedFeatureCount() const { return unsupportedFeatureCount_; }
  size_t getSupportedFeatureCount() const { return supportedFeatureCount_; }

 private:
  // Analysis results
  std::string filename_;
  std::vector<N3ComplianceIssue> issues_;
  std::map<std::string, size_t> featureUsage_;
  size_t totalLines_ = 0;
  size_t unsupportedFeatureCount_ = 0;
  size_t supportedFeatureCount_ = 0;
  bool fileExists_ = false;
  bool parseSuccess_ = false;

  // Feature detection methods
  void analyzeLineForFeatures(const std::string& line, size_t lineNumber);
  void detectAdvancedN3Features(const std::string& line, size_t lineNumber);
  void detectSupportedFeatures(const std::string& line, size_t lineNumber);

  // Check for specific unsupported N3 features
  bool hasFormulae(const std::string& line) const;
  bool hasImplication(const std::string& line) const;
  bool hasQuantifier(const std::string& line) const;
  bool hasVariable(const std::string& line) const;
  bool hasBuiltinFunction(const std::string& line) const;
  bool hasN3Path(const std::string& line) const;

  // Add an issue to the report
  void addIssue(const std::string& feature, const std::string& description,
                size_t lineNumber, const std::string& lineContent,
                bool isSupported);

  // Increment feature usage counter
  void recordFeatureUsage(const std::string& feature);

  // Generate report sections
  std::string generateSummarySection() const;
  std::string generateFeatureMatrixSection() const;
  std::string generateIssuesSection() const;
  std::string generateRecommendationsSection() const;
};

}  // namespace ad_utility

#endif  // QLEVER_SRC_UTIL_N3COMPLIANCEVERIFIER_H

// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
//
// Command-line tool for verifying N3 file compliance with QLever

#include <fstream>
#include <iostream>
#include <string>

#include "util/N3ComplianceVerifier.h"

void printUsage(const char* programName) {
  std::cout << "N3 Format Compliance Verifier for QLever\n\n";
  std::cout << "Usage: " << programName
            << " <n3-file> [--report FILE] [--strict]\n\n";
  std::cout << "Arguments:\n";
  std::cout << "  <n3-file>        Path to N3/Turtle file to verify\n";
  std::cout << "  --report FILE    Write detailed report to FILE (markdown "
               "format)\n";
  std::cout << "  --strict         Exit with non-zero code if file is "
               "incompatible\n\n";
  std::cout << "Examples:\n";
  std::cout << "  " << programName << " data.n3\n";
  std::cout << "  " << programName
            << " data.n3 --report report.md --strict\n\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputFile;
  std::string reportFile;
  bool strictMode = false;

  // Parse command-line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      printUsage(argv[0]);
      return 0;
    } else if (arg == "--report") {
      if (i + 1 < argc) {
        reportFile = argv[++i];
      } else {
        std::cerr << "Error: --report requires a filename\n";
        return 1;
      }
    } else if (arg == "--strict") {
      strictMode = true;
    } else if (inputFile.empty()) {
      inputFile = arg;
    } else {
      std::cerr << "Error: Unexpected argument: " << arg << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  if (inputFile.empty()) {
    std::cerr << "Error: No input file specified\n";
    printUsage(argv[0]);
    return 1;
  }

  // Run the compliance verifier
  ad_utility::N3ComplianceVerifier verifier;
  verifier.analyzeFile(inputFile);

  // Generate and display/save report
  std::string report = verifier.generateReport();

  if (reportFile.empty()) {
    // Print to console
    std::cout << report;
  } else {
    // Write to file
    std::ofstream outFile(reportFile);
    if (!outFile.is_open()) {
      std::cerr << "Error: Failed to open report file: " << reportFile << "\n";
      return 1;
    }
    outFile << report;
    outFile.close();
    std::cout << "Report written to: " << reportFile << "\n";
  }

  // Print summary to console even if report goes to file
  if (!reportFile.empty()) {
    std::cout << "\nSummary:\n";
    std::cout << "  File: " << inputFile << "\n";
    std::cout << "  Lines: " << verifier.getTotalLines() << "\n";
    std::cout << "  Compatible: " << (verifier.isCompatible() ? "YES" : "NO")
              << "\n";
    if (!verifier.isCompatible()) {
      std::cout << "  Unsupported features found: "
                << verifier.getUnsupportedFeatureCount() << "\n";
    }
  }

  // Exit with appropriate code
  if (strictMode && !verifier.isCompatible()) {
    return 2;  // Incompatible file in strict mode
  }

  return 0;
}

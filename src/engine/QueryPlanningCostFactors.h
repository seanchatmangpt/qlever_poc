// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#ifndef QLEVER_SRC_ENGINE_QUERYPLANNINGCOSTFACTORS_H
#define QLEVER_SRC_ENGINE_QUERYPLANNINGCOSTFACTORS_H

#include <string>

#include "util/HashMap.h"

// Forward declaration for dynamic cost factors
class DynamicCostFactors;

// Simple container for cost factors.
// Comes with default values, that can be set and read from a file.
// Can also be updated dynamically based on runtime statistics.
class QueryPlanningCostFactors {
 public:
  QueryPlanningCostFactors();
  void readFromFile(const std::string& fileName);
  double getCostFactor(const std::string& key) const;

  // Dynamic cost factor calculation based on pattern statistics
  // Updates FILTER_PUNISH and JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR
  // based on actual selectivity and filter characteristics
  void updateDynamicFactors(double filterSelectivity,
                            double leftSelectivity = 1.0,
                            double rightSelectivity = 1.0);

 private:
  ad_utility::HashMap<std::string, double> _factors;
};

#endif  // QLEVER_SRC_ENGINE_QUERYPLANNINGCOSTFACTORS_H

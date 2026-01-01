// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "engine/QueryPlanningCostFactors.h"

#include <absl/strings/charconv.h>
#include <absl/strings/str_split.h>

#include <fstream>

#include "engine/DynamicCostFactors.h"
#include "util/Exception.h"
#include "util/Log.h"
#include "util/StringUtils.h"

// _____________________________________________________________________________
QueryPlanningCostFactors::QueryPlanningCostFactors() : _factors() {
  // Set default values
  _factors["FILTER_PUNISH"] = 2.0;
  _factors["NO_FILTER_PUNISH"] = 1.0;
  _factors["FILTER_SELECTIVITY"] = 0.1;
  _factors["HASH_MAP_OPERATION_COST"] = 50.0;
  _factors["JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR"] = 0.7;
  _factors["DUMMY_JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR"] = 0.7;

  // Assume that a random disk seek is 100 times more expensive than an
  // average `O(1)` access to a single ID.
  _factors["DISK_RANDOM_ACCESS_COST"] = 100;
}

// _____________________________________________________________________________

float toFloat(std::string_view view) {
  float factor;
  auto last = view.data() + view.size();
  auto [ptr, ec] = absl::from_chars(view.data(), last, factor);
  if (ec != std::errc() || ptr != last) {
    throw std::runtime_error{std::string{"Invalid float: "} + view};
  }
  return factor;
}

// _____________________________________________________________________________
void QueryPlanningCostFactors::readFromFile(const std::string& fileName) {
  std::ifstream in(fileName);
  std::string line;
  while (std::getline(in, line)) {
    std::vector<std::string_view> v = absl::StrSplit(line, '\t');
    AD_CONTRACT_CHECK(v.size() == 2);
    float factor = toFloat(v[1]);
    AD_LOG_INFO << "Setting cost factor: " << v[0] << " from " << _factors[v[0]]
                << " to " << factor << std::endl;
    _factors[v[0]] = factor;
  }
}

// _____________________________________________________________________________
double QueryPlanningCostFactors::getCostFactor(const std::string& key) const {
  return _factors.find(key)->second;
}

// _____________________________________________________________________________
void QueryPlanningCostFactors::updateDynamicFactors(double filterSelectivity,
                                                    double leftSelectivity,
                                                    double rightSelectivity) {
  // Use DynamicCostFactors to calculate adaptive cost factors
  // based on actual filter selectivity instead of hardcoded assumptions

  // Update filter cost factor based on selectivity
  // This replaces the hardcoded FILTER_PUNISH = 2.0 with a dynamic value
  // that adapts to actual data filtering characteristics
  double dynamicFilterCost =
      DynamicCostFactors::calculateFilterCostFactor(filterSelectivity);
  _factors["FILTER_PUNISH"] = dynamicFilterCost;

  AD_LOG_DEBUG << "Updated FILTER_PUNISH from 2.0 to " << dynamicFilterCost
               << " based on selectivity " << filterSelectivity << std::endl;

  // Update join correction factor based on selectivity on both sides
  // This replaces the hardcoded JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR = 0.7
  // with a dynamic value that accounts for selective filters
  double dynamicJoinFactor = DynamicCostFactors::calculateJoinCorrectionFactor(
      leftSelectivity, rightSelectivity);
  _factors["JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR"] = dynamicJoinFactor;
  _factors["DUMMY_JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR"] = dynamicJoinFactor;

  AD_LOG_DEBUG << "Updated JOIN_SIZE_ESTIMATE_CORRECTION_FACTOR from 0.7 to "
               << dynamicJoinFactor << " based on left selectivity "
               << leftSelectivity << " and right selectivity "
               << rightSelectivity << std::endl;
}

#include "AdvancedConstraints.h"
#include <algorithm>

namespace shacl {

// ResourceStateTracker implementation

bool ResourceStateTracker::trackUniqueValue(const std::string& propertyPath,
                                            const std::string& value) {
  auto& valueSet = propertyValues_[propertyPath];
  auto [it, inserted] = valueSet.insert(value);
  return inserted;  // Returns true if value was newly inserted (no duplicate)
}

bool ResourceStateTracker::hasValue(const std::string& propertyPath,
                                    const std::string& value) const {
  auto it = propertyValues_.find(propertyPath);
  if (it == propertyValues_.end()) {
    return false;
  }
  return it->second.find(value) != it->second.end();
}

std::vector<std::string> ResourceStateTracker::getValues(
    const std::string& propertyPath) const {
  auto it = propertyValues_.find(propertyPath);
  if (it == propertyValues_.end()) {
    return {};
  }
  return std::vector<std::string>(it->second.begin(), it->second.end());
}

bool ResourceStateTracker::hasDisjointViolation(
    const std::string& property1, const std::string& property2,
    const std::vector<std::string>& values1,
    const std::vector<std::string>& values2) const {
  // Check if any values overlap between the two properties
  for (const auto& v1 : values1) {
    for (const auto& v2 : values2) {
      if (v1 == v2) {
        return true;  // Found overlapping value - violation
      }
    }
  }
  return false;
}

void ResourceStateTracker::clear() { propertyValues_.clear(); }

size_t ResourceStateTracker::valueCount(const std::string& propertyPath) const {
  auto it = propertyValues_.find(propertyPath);
  if (it == propertyValues_.end()) {
    return 0;
  }
  return it->second.size();
}

}  // namespace shacl

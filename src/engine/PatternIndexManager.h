#ifndef QLEVER_SRC_ENGINE_PATTERN_INDEX_MANAGER_H
#define QLEVER_SRC_ENGINE_PATTERN_INDEX_MANAGER_H

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace qlever {

/**
 * Generic pattern indexing manager for 80/20 query optimization
 *
 * Provides O(1) pattern-to-item lookup instead of O(n) iteration.
 *
 * Use cases:
 * 1. Join optimization: Predicate → list of applicable joins
 * 2. Filter optimization: Variable → list of applicable filters
 * 3. Index scan: Predicate → list of applicable index patterns
 * 4. GroupBy optimization: Variable → list of applicable group operations
 *
 * Template parameters:
 * - ItemId: The type of items being indexed (e.g., std::string for operation IDs)
 * - PatternElement: The type of patterns (e.g., std::string for predicates/variables)
 *
 * Usage Example:
 *   // Index join operations by their predicates
 *   PatternIndexManager<std::string, std::string> joinIndex;
 *
 *   std::vector<std::pair<std::string, std::vector<std::string>>> joinPatterns;
 *   joinPatterns.push_back({"join_1", {"p1", "p2"}});
 *   joinPatterns.push_back({"join_2", {"p2", "p3"}});
 *   joinIndex.buildIndex(joinPatterns);
 *
 *   // Find joins applicable to query with predicates ["p1", "p2", "p3"]
 *   auto applicableJoins = joinIndex.getCandidateItems({"p1", "p2", "p3"});
 *   // Returns: {"join_1", "join_2"}
 */
template <typename ItemId, typename PatternElement>
class PatternIndexManager {
 public:
  /**
   * Build the pattern index from items and their patterns
   *
   * @param itemPatterns Vector of (itemId, patterns) pairs
   *        - itemId: unique identifier for the item
   *        - patterns: vector of pattern elements this item uses
   */
  void buildIndex(const std::vector<std::pair<ItemId, std::vector<PatternElement>>>&
                      itemPatterns) {
    patternToItems_.clear();

    for (const auto& [itemId, patterns] : itemPatterns) {
      for (const auto& pattern : patterns) {
        patternToItems_[pattern].insert(itemId);
      }
    }
  }

  /**
   * Get all items that match ANY of the given patterns
   *
   * Returns all items that use at least one of the provided patterns.
   * This is useful for broad filtering where any match is relevant.
   *
   * @param patterns Vector of pattern elements to search for
   * @return Set of item IDs that use at least one pattern
   * Time complexity: O(patterns.size() × average_items_per_pattern)
   */
  absl::flat_hash_set<ItemId> getItemsMatchingAny(
      const std::vector<PatternElement>& patterns) const {
    absl::flat_hash_set<ItemId> result;

    for (const auto& pattern : patterns) {
      auto it = patternToItems_.find(pattern);
      if (it != patternToItems_.end()) {
        for (const auto& itemId : it->second) {
          result.insert(itemId);
        }
      }
    }

    return result;
  }

  /**
   * Get items that match ALL of the given patterns
   *
   * Returns only items that use every single provided pattern.
   * This is useful for precise filtering where all patterns must match.
   *
   * @param patterns Vector of pattern elements to search for
   * @return Set of item IDs that use all patterns (intersection)
   * Time complexity: O(patterns.size() × average_items_per_pattern)
   */
  absl::flat_hash_set<ItemId> getItemsMatchingAll(
      const std::vector<PatternElement>& patterns) const {
    if (patterns.empty()) {
      return getAllItems();
    }

    // Start with items matching the first pattern
    auto it = patternToItems_.find(patterns[0]);
    if (it == patternToItems_.end()) {
      return {};  // No items match first pattern
    }

    absl::flat_hash_set<ItemId> result = it->second;

    // Intersect with items matching subsequent patterns
    for (size_t i = 1; i < patterns.size(); ++i) {
      auto patIt = patternToItems_.find(patterns[i]);
      if (patIt == patternToItems_.end()) {
        return {};  // No items match this pattern
      }

      // Intersect: keep only items in both sets
      absl::flat_hash_set<ItemId> intersection;
      for (const auto& itemId : result) {
        if (patIt->second.contains(itemId)) {
          intersection.insert(itemId);
        }
      }
      result = std::move(intersection);

      if (result.empty()) {
        return {};  // No items match all patterns so far
      }
    }

    return result;
  }

  /**
   * Get all items (used when no pattern matching is possible)
   */
  absl::flat_hash_set<ItemId> getAllItems() const {
    absl::flat_hash_set<ItemId> result;

    for (const auto& [pattern, items] : patternToItems_) {
      for (const auto& itemId : items) {
        result.insert(itemId);
      }
    }

    return result;
  }

  /**
   * Check if a pattern has any indexed items
   */
  bool hasPattern(const PatternElement& pattern) const {
    return patternToItems_.find(pattern) != patternToItems_.end();
  }

  /**
   * Get statistics about the index
   */
  struct IndexStats {
    size_t totalPatterns = 0;
    size_t totalItems = 0;
    size_t totalMappings = 0;
    double avgItemsPerPattern = 0.0;
    double avgPatternsPerItem = 0.0;

    std::string toString() const {
      std::ostringstream oss;
      oss << "Patterns: " << totalPatterns << ", Items: " << totalItems
          << ", Mappings: " << totalMappings
          << ", Avg items/pattern: " << avgItemsPerPattern;
      return oss.str();
    }
  };

  IndexStats getStats() const {
    IndexStats stats;
    stats.totalPatterns = patternToItems_.size();

    // Count unique items
    absl::flat_hash_set<ItemId> uniqueItems;
    size_t totalMappings = 0;

    for (const auto& [pattern, items] : patternToItems_) {
      totalMappings += items.size();
      for (const auto& itemId : items) {
        uniqueItems.insert(itemId);
      }
    }

    stats.totalItems = uniqueItems.size();
    stats.totalMappings = totalMappings;

    if (stats.totalPatterns > 0) {
      stats.avgItemsPerPattern =
          static_cast<double>(totalMappings) / stats.totalPatterns;
    }
    if (stats.totalItems > 0) {
      stats.avgPatternsPerItem =
          static_cast<double>(totalMappings) / stats.totalItems;
    }

    return stats;
  }

  /**
   * Clear the index
   */
  void clear() { patternToItems_.clear(); }

  /**
   * Check if index is empty
   */
  bool isEmpty() const { return patternToItems_.empty(); }

 private:
  // Main index structure: pattern -> set of items
  absl::flat_hash_map<PatternElement, absl::flat_hash_set<ItemId>>
      patternToItems_;
};

}  // namespace qlever

#endif  // QLEVER_SRC_ENGINE_PATTERN_INDEX_MANAGER_H

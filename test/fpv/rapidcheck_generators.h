// Copyright 2026, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: EPIC 10.3 Agent 2 (FPV Auditor)

#ifndef QLEVER_TEST_FPV_RAPIDCHECK_GENERATORS_H
#define QLEVER_TEST_FPV_RAPIDCHECK_GENERATORS_H

#include <rapidcheck.h>

#include "engine/idTable/IdTable.h"
#include "global/Id.h"
#include "parser/TripleComponent.h"
#include "util/AllocatorWithLimit.h"

namespace qlever::fpv {

// =============================================================================
// INVARIANT CONSTRAINTS
// =============================================================================

// Maximum table width (aligned with QLever's typical use case)
constexpr size_t MAX_TABLE_WIDTH = 20;

// Maximum table rows for property testing (larger sizes tested in long-running fuzzing)
constexpr size_t MAX_TABLE_ROWS = 1000;

// Maximum additional variables for IndexScan
constexpr size_t MAX_ADDITIONAL_VARS = 10;

// Maximum number of variables in a triple (always 3 in RDF)
constexpr size_t MAX_TRIPLE_VARIABLES = 3;

// =============================================================================
// Id GENERATORS
// =============================================================================

/// Generate an arbitrary valid Id
/// Ensures all generated Ids are within valid ranges for QLever
inline auto arbId() {
  return rc::gen::exec([]() -> Id {
    auto val = *rc::gen::inRange<uint64_t>(0, Id::maxIndex);
    return Id::makeFromInt(val);
  });
}

/// Generate an Id that could be UNDEF
inline auto arbIdWithUndef() {
  return rc::gen::oneOf(
      arbId(),
      rc::gen::just(Id::makeUndefined())
  );
}

// =============================================================================
// IdTable GENERATORS
// =============================================================================

/// Generate an IdTable with arbitrary dimensions and content
/// Invariant: 1 <= width <= MAX_TABLE_WIDTH, 0 <= rows <= MAX_TABLE_ROWS
inline auto arbIdTable(std::optional<size_t> fixedWidth = std::nullopt,
                       std::optional<size_t> fixedRows = std::nullopt,
                       bool allowUndef = true) {
  return rc::gen::exec([=]() -> IdTable {
    size_t width = fixedWidth.value_or(
        *rc::gen::inRange<size_t>(1, MAX_TABLE_WIDTH + 1));
    size_t rows = fixedRows.value_or(
        *rc::gen::inRange<size_t>(0, MAX_TABLE_ROWS + 1));

    // Use unlimited allocator for testing
    ad_utility::AllocatorWithLimit<Id> alloc{
        ad_utility::makeUnlimitedAllocator<Id>()};

    IdTable table(width, alloc);
    table.resize(rows);

    auto idGen = allowUndef ? arbIdWithUndef() : arbId();

    for (size_t row = 0; row < rows; ++row) {
      for (size_t col = 0; col < width; ++col) {
        table(row, col) = *idGen;
      }
    }

    return table;
  });
}

/// Generate a sorted IdTable (sorted by first column)
/// Invariant: table[i][0] <= table[i+1][0] for all i
inline auto arbSortedIdTable(std::optional<size_t> fixedWidth = std::nullopt,
                              std::optional<size_t> fixedRows = std::nullopt,
                              bool allowUndef = true) {
  return rc::gen::exec([=]() -> IdTable {
    IdTable table = *arbIdTable(fixedWidth, fixedRows, allowUndef);

    if (table.empty()) {
      return table;
    }

    // Sort by first column
    std::sort(table.begin(), table.end(),
              [](const auto& a, const auto& b) {
                return a[0] < b[0];
              });

    return table;
  });
}

// =============================================================================
// JOIN GENERATORS
// =============================================================================

/// Input structure for Join property tests
struct JoinInput {
  IdTable left;
  IdTable right;
  size_t leftJoinCol;
  size_t rightJoinCol;
  bool keepJoinColumn;

  // Invariants:
  // - left.numColumns() >= 1
  // - right.numColumns() >= 1
  // - leftJoinCol < left.numColumns()
  // - rightJoinCol < right.numColumns()
};

/// Generate arbitrary Join inputs with valid constraints
inline auto arbJoinInput() {
  return rc::gen::exec([]() -> JoinInput {
    // Generate table widths
    size_t leftWidth = *rc::gen::inRange<size_t>(1, MAX_TABLE_WIDTH + 1);
    size_t rightWidth = *rc::gen::inRange<size_t>(1, MAX_TABLE_WIDTH + 1);

    // Generate join columns (must be < width)
    size_t leftJoinCol = *rc::gen::inRange<size_t>(0, leftWidth);
    size_t rightJoinCol = *rc::gen::inRange<size_t>(0, rightWidth);

    // Generate sorted tables (join requires sorted inputs)
    IdTable left = *arbSortedIdTable(leftWidth, std::nullopt, true);
    IdTable right = *arbSortedIdTable(rightWidth, std::nullopt, true);

    // For join to work, we need to sort by join column
    // Rotate columns so join column is first
    auto rotateToFront = [](IdTable& table, size_t col) {
      if (col == 0) return;

      IdTable temp = std::move(table);
      table = IdTable(temp.numColumns(), temp.getAllocator());
      table.resize(temp.size());

      for (size_t row = 0; row < temp.size(); ++row) {
        table(row, 0) = temp(row, col);
        for (size_t c = 0; c < col; ++c) {
          table(row, c + 1) = temp(row, c);
        }
        for (size_t c = col + 1; c < temp.numColumns(); ++c) {
          table(row, c) = temp(row, c);
        }
      }

      // Re-sort by first column (which is now the join column)
      std::sort(table.begin(), table.end(),
                [](const auto& a, const auto& b) { return a[0] < b[0]; });
    };

    rotateToFront(left, leftJoinCol);
    rotateToFront(right, rightJoinCol);

    // After rotation, join columns are always at index 0
    leftJoinCol = 0;
    rightJoinCol = 0;

    bool keepJoinColumn = *rc::gen::arbitrary<bool>();

    return JoinInput{
        std::move(left),
        std::move(right),
        leftJoinCol,
        rightJoinCol,
        keepJoinColumn};
  });
}

// =============================================================================
// FILTER GENERATORS
// =============================================================================

/// Interval structure for Filter tests
struct Interval {
  size_t first;
  size_t second;

  // Invariant: first <= second
  bool isValid() const { return first <= second; }
};

/// Input structure for Filter property tests
struct FilterInput {
  IdTable input;
  std::vector<Interval> intervals;

  // Invariants:
  // - All intervals are valid (first <= second)
  // - All intervals are within input bounds (second <= input.size())
};

/// Generate valid intervals
inline auto arbInterval(size_t maxSize) {
  return rc::gen::exec([=]() -> Interval {
    size_t first = *rc::gen::inRange<size_t>(0, maxSize + 1);
    size_t second = *rc::gen::inRange<size_t>(first, maxSize + 1);
    return Interval{first, second};
  });
}

/// Generate arbitrary Filter inputs with valid constraints
inline auto arbFilterInput() {
  return rc::gen::exec([]() -> FilterInput {
    IdTable input = *arbIdTable();
    size_t numRows = input.size();

    // Generate 0-10 random intervals
    size_t numIntervals = *rc::gen::inRange<size_t>(0, 11);
    std::vector<Interval> intervals;
    intervals.reserve(numIntervals);

    for (size_t i = 0; i < numIntervals; ++i) {
      intervals.push_back(*arbInterval(numRows));
    }

    return FilterInput{std::move(input), std::move(intervals)};
  });
}

// =============================================================================
// INDEXSCAN GENERATORS
// =============================================================================

/// Input structure for IndexScan property tests
struct IndexScanInput {
  TripleComponent subject;
  TripleComponent predicate;
  TripleComponent object;
  size_t numVariables;
  std::vector<ColumnIndex> additionalColumns;

  // Invariants:
  // - numVariables in [0, 3]
  // - numVariables == count of Variables in (subject, predicate, object)
  // - additionalColumns.size() <= MAX_ADDITIONAL_VARS
};

/// Generate arbitrary TripleComponent
inline auto arbTripleComponent() {
  return rc::gen::oneOf(
      // Variable
      rc::gen::exec([]() -> TripleComponent {
        auto varName = *rc::gen::inRange<int>(1, 100);
        return TripleComponent(Variable("?var" + std::to_string(varName)));
      }),
      // IRI
      rc::gen::exec([]() -> TripleComponent {
        auto iriNum = *rc::gen::inRange<int>(1, 1000);
        return TripleComponent::Iri::fromIriref(
            "<http://example.org/entity/" + std::to_string(iriNum) + ">");
      })
  );
}

/// Generate arbitrary IndexScan inputs with valid constraints
inline auto arbIndexScanInput() {
  return rc::gen::exec([]() -> IndexScanInput {
    // Generate triple components
    TripleComponent subject = *arbTripleComponent();
    TripleComponent predicate = *arbTripleComponent();
    TripleComponent object = *arbTripleComponent();

    // Count variables
    size_t numVariables =
        static_cast<size_t>(subject.isVariable()) +
        static_cast<size_t>(predicate.isVariable()) +
        static_cast<size_t>(object.isVariable());

    // Generate additional columns
    size_t numAdditional = *rc::gen::inRange<size_t>(0, MAX_ADDITIONAL_VARS + 1);
    std::vector<ColumnIndex> additionalColumns;
    for (size_t i = 0; i < numAdditional; ++i) {
      additionalColumns.push_back(ColumnIndex{i});
    }

    return IndexScanInput{
        std::move(subject),
        std::move(predicate),
        std::move(object),
        numVariables,
        std::move(additionalColumns)};
  });
}

// =============================================================================
// ARITHMETIC SAFETY GENERATORS
// =============================================================================

/// Generate size_t values that are safe for addition (won't overflow)
inline auto arbSafeSizeForAddition() {
  return rc::gen::inRange<size_t>(0, SIZE_MAX / 2);
}

/// Generate size_t values that test overflow boundaries
inline auto arbSizeNearMaxForOverflowTest() {
  return rc::gen::oneOf(
      rc::gen::inRange<size_t>(SIZE_MAX - 1000, SIZE_MAX),
      rc::gen::inRange<size_t>(SIZE_MAX / 2 - 1000, SIZE_MAX / 2 + 1000),
      rc::gen::inRange<size_t>(0, 1000)
  );
}

/// Generate float multipliers for overflow testing
inline auto arbMultiplierFloat() {
  return rc::gen::inRange<float>(0.0f, 1000.0f);
}

// =============================================================================
// PROPERTY TESTING UTILITIES
// =============================================================================

/// Compare two IdTables for semantic equivalence
/// Handles potential row reordering in SIMD vs scalar implementations
inline bool idTablesEquivalent(const IdTable& a, const IdTable& b) {
  if (a.numColumns() != b.numColumns()) return false;
  if (a.size() != b.size()) return false;

  // For sorted tables, can compare directly
  for (size_t row = 0; row < a.size(); ++row) {
    for (size_t col = 0; col < a.numColumns(); ++col) {
      if (a(row, col) != b(row, col)) {
        return false;
      }
    }
  }

  return true;
}

/// Check if arithmetic operation would overflow
inline bool wouldAddOverflow(size_t a, size_t b) {
  return a > SIZE_MAX - b;
}

/// Check if arithmetic operation would underflow
inline bool wouldSubtractUnderflow(size_t a, size_t b) {
  return a < b;
}

/// Check if multiplication would overflow
inline bool wouldMultiplyOverflow(size_t a, size_t b) {
  if (b == 0) return false;
  return a > SIZE_MAX / b;
}

}  // namespace qlever::fpv

#endif  // QLEVER_TEST_FPV_RAPIDCHECK_GENERATORS_H

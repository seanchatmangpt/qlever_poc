// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Assistant

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "engine/LocalVocab.h"
#include "engine/idTable/IdTable.h"
#include "engine/readCache/BytePage.h"
#include "engine/readCache/SerializerPaging.h"
#include "global/Id.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

using ad_utility::testing::makeAllocator;

// Test fixture for SerializerPaging tests
class SerializerPagingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a simple index for testing
    index_ = ad_utility::testing::getQec()->getIndex();
  }

  // Helper to create a simple IdTable with integer values
  IdTable makeTestIdTable(const std::vector<std::vector<int>>& data) {
    if (data.empty()) {
      return IdTable{0, makeAllocator()};
    }
    size_t numCols = data[0].size();
    IdTable table{numCols, makeAllocator()};
    table.resize(data.size());

    for (size_t row = 0; row < data.size(); ++row) {
      for (size_t col = 0; col < numCols; ++col) {
        table(row, col) = Id::makeFromInt(data[row][col]);
      }
    }
    return table;
  }

  Index index_;
};

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, BytePageImmutability) {
  // Test that BytePage stores data as const shared_ptr
  std::string testData = "test data";
  BytePage page{testData};

  EXPECT_EQ(page.size(), testData.size());
  EXPECT_EQ(page.data(), testData);

  // The data should be const (this is enforced by the type system)
  std::shared_ptr<const std::string> dataPtr = page.data_;
  EXPECT_NE(dataPtr, nullptr);
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, BytePageSharedPtr) {
  // Test that BytePage correctly handles shared_ptr construction
  auto sharedData = std::make_shared<const std::string>("shared data");
  BytePage page{sharedData};

  EXPECT_EQ(page.size(), sharedData->size());
  EXPECT_EQ(page.data(), *sharedData);

  // Verify reference counting works
  EXPECT_EQ(sharedData.use_count(), 2);  // page.data_ and sharedData
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, BasicSerialization) {
  // Create a small test table
  IdTable table = makeTestIdTable({{1, 2}, {3, 4}, {5, 6}});
  LocalVocab vocab;

  // Create serializer with small page size for testing
  PagedSerializer serializer{ad_utility::MediaType::tsv, 50};

  // Serialize rows
  std::vector<std::optional<size_t>> columnIndices = {0, 1};
  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  // Finalize and get pages
  auto pages = serializer.finalize();

  // Verify we got at least one page
  EXPECT_GE(pages.size(), 1);

  // Verify total size matches
  uint64_t totalSize = 0;
  for (const auto& page : pages) {
    totalSize += page.size();
  }
  EXPECT_EQ(totalSize, serializer.getUncompressedSize());
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, PageSizeRespected) {
  // Create a larger table to test paging
  std::vector<std::vector<int>> data;
  for (int i = 0; i < 100; ++i) {
    data.push_back({i, i * 2, i * 3});
  }
  IdTable table = makeTestIdTable(data);
  LocalVocab vocab;

  // Use a small page size to force multiple pages
  size_t pageSize = 100;
  PagedSerializer serializer{ad_utility::MediaType::tsv, pageSize};

  std::vector<std::optional<size_t>> columnIndices = {0, 1, 2};
  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  auto pages = serializer.finalize();

  // With 100 rows and small page size, we should get multiple pages
  EXPECT_GT(pages.size(), 1);

  // Each page (except possibly the last) should be close to the target size
  for (size_t i = 0; i + 1 < pages.size(); ++i) {
    EXPECT_GE(pages[i].size(), pageSize);
  }
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, UncompressedSizeTracking) {
  IdTable table = makeTestIdTable({{1}, {2}, {3}, {4}, {5}});
  LocalVocab vocab;

  PagedSerializer serializer{ad_utility::MediaType::tsv, 256 * 1024};
  std::vector<std::optional<size_t>> columnIndices = {0};

  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  uint64_t sizeBeforeFinalize = serializer.getUncompressedSize();
  auto pages = serializer.finalize();
  uint64_t sizeAfterFinalize = serializer.getUncompressedSize();

  // Size should be consistent
  EXPECT_EQ(sizeBeforeFinalize, sizeAfterFinalize);

  // Calculate actual total from pages
  uint64_t actualTotal = 0;
  for (const auto& page : pages) {
    actualTotal += page.size();
  }
  EXPECT_EQ(actualTotal, sizeAfterFinalize);
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, StreamFromPages) {
  // Create some test pages
  std::vector<BytePage> pages;
  pages.emplace_back("first page data\n");
  pages.emplace_back("second page data\n");
  pages.emplace_back("third page data\n");

  // Stream pages to an ostringstream
  std::ostringstream oss;
  streamFromPages(pages, oss);

  // Verify the output
  std::string expected = "first page data\nsecond page data\nthird page data\n";
  EXPECT_EQ(oss.str(), expected);
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, StreamFromPagesLossless) {
  // Create a table and serialize it
  IdTable table = makeTestIdTable({{1, 2}, {3, 4}, {5, 6}});
  LocalVocab vocab;

  PagedSerializer serializer{ad_utility::MediaType::tsv, 256 * 1024};
  std::vector<std::optional<size_t>> columnIndices = {0, 1};

  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  auto pages = serializer.finalize();

  // Stream pages back
  std::ostringstream oss;
  streamFromPages(pages, oss);

  // The output should contain the serialized data
  std::string output = oss.str();
  EXPECT_FALSE(output.empty());

  // Should contain newlines for each row
  size_t newlineCount = std::count(output.begin(), output.end(), '\n');
  EXPECT_EQ(newlineCount, table.size());
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, EmptyTable) {
  IdTable table{2, makeAllocator()};
  LocalVocab vocab;

  PagedSerializer serializer{ad_utility::MediaType::tsv, 256 * 1024};
  auto pages = serializer.finalize();

  // Should produce no pages for empty input
  EXPECT_TRUE(pages.empty() || pages[0].size() == 0);
  EXPECT_EQ(serializer.getUncompressedSize(), 0);
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, WriteRaw) {
  PagedSerializer serializer{ad_utility::MediaType::tsv, 256 * 1024};

  // Write raw header
  serializer.writeRaw("column1\tcolumn2\tcolumn3\n");

  auto pages = serializer.finalize();

  EXPECT_EQ(pages.size(), 1);
  EXPECT_EQ(pages[0].data(), "column1\tcolumn2\tcolumn3\n");
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, CsvFormat) {
  IdTable table = makeTestIdTable({{1, 2}, {3, 4}});
  LocalVocab vocab;

  // Test CSV format (should use comma separator)
  PagedSerializer serializer{ad_utility::MediaType::csv, 256 * 1024};
  std::vector<std::optional<size_t>> columnIndices = {0, 1};

  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  auto pages = serializer.finalize();

  // Verify comma separator is used
  std::ostringstream oss;
  streamFromPages(pages, oss);
  std::string output = oss.str();

  // CSV should contain commas
  EXPECT_TRUE(output.find(',') != std::string::npos);
}

// _____________________________________________________________________________
TEST_F(SerializerPagingTest, TsvFormat) {
  IdTable table = makeTestIdTable({{1, 2}, {3, 4}});
  LocalVocab vocab;

  // Test TSV format (should use tab separator)
  PagedSerializer serializer{ad_utility::MediaType::tsv, 256 * 1024};
  std::vector<std::optional<size_t>> columnIndices = {0, 1};

  for (size_t i = 0; i < table.size(); ++i) {
    serializer.addRow(table[i], vocab, index_, columnIndices);
  }

  auto pages = serializer.finalize();

  // Verify tab separator is used
  std::ostringstream oss;
  streamFromPages(pages, oss);
  std::string output = oss.str();

  // TSV should contain tabs
  EXPECT_TRUE(output.find('\t') != std::string::npos);
}

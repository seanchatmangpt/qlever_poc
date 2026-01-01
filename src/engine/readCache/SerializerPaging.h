// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_READCACHE_SERIALIZERPAGING_H
#define QLEVER_SRC_ENGINE_READCACHE_SERIALIZERPAGING_H

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "engine/LocalVocab.h"
#include "engine/readCache/BytePage.h"
#include "engine/idTable/IdTable.h"
#include "global/Id.h"
#include "index/Index.h"
#include "util/http/MediaTypes.h"

// PagedSerializer wraps existing serialization logic to produce paged output
// suitable for caching. Instead of concatenating all output into a single
// buffer, it produces a sequence of immutable BytePage objects.
class PagedSerializer {
 public:
  using MediaType = ad_utility::MediaType;

  // Default page size: 256KB
  static constexpr size_t DEFAULT_PAGE_SIZE = 256 * 1024;

  // Construct a PagedSerializer with the given output format and page size.
  // The serializer will buffer output and emit pages when the buffer reaches
  // the specified size.
  explicit PagedSerializer(
      MediaType format = MediaType::tsv,
      size_t pageSize = DEFAULT_PAGE_SIZE);

  // Add a serialized row to the output buffer. When the buffer reaches the
  // page size, a new BytePage is created and added to the internal vector.
  // The `row` parameter is a view into an IdTable, and the `vocab` is used
  // to resolve IDs to strings.
  void addRow(const IdTable::row_type& row, const LocalVocab& localVocab,
              const Index& index,
              const std::vector<std::optional<size_t>>& selectedColumnIndices);

  // Write a raw string to the output buffer (e.g., for headers or separators)
  void writeRaw(std::string_view data);

  // Finalize the serialization. This creates a final BytePage from any
  // remaining buffered data and returns all pages produced during
  // serialization.
  [[nodiscard]] std::vector<BytePage> finalize();

  // Get the total uncompressed size of all data written (including data still
  // in the buffer)
  [[nodiscard]] uint64_t getUncompressedSize() const {
    return totalUncompressedSize_;
  }

  // Get the number of pages produced so far (not including buffered data)
  [[nodiscard]] size_t getPageCount() const { return pages_.size(); }

 private:
  // Flush the current buffer to a new BytePage
  void flushBuffer();

  // Serialize a single cell (ID value) to the buffer
  void serializeCell(Id id, const LocalVocab& localVocab, const Index& index);

  // The output format (TSV, CSV, JSON, etc.)
  MediaType format_;

  // The target size for each page
  size_t pageSize_;

  // The current output buffer
  std::string buffer_;

  // All pages produced so far
  std::vector<BytePage> pages_;

  // Total uncompressed size of all data written
  uint64_t totalUncompressedSize_ = 0;

  // Separator character (tab for TSV, comma for CSV)
  char separator_;
};

// Stream cached pages to an output stream. This function takes a vector of
// BytePage objects (typically from a cache) and writes them sequentially to
// the output stream. This is a zero-copy operation since pages are stored as
// shared_ptr<const std::string>.
void streamFromPages(const std::vector<BytePage>& pages, std::ostream& out);

#endif  // QLEVER_SRC_ENGINE_READCACHE_SERIALIZERPAGING_H

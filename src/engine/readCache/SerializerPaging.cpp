// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Assistant

#include "engine/readCache/SerializerPaging.h"

#include "engine/ExportQueryExecutionTrees.h"
#include "rdfTypes/RdfEscaping.h"

// _____________________________________________________________________________
PagedSerializer::PagedSerializer(MediaType format, size_t pageSize)
    : format_{format}, pageSize_{pageSize} {
  // Determine the separator character based on the format
  separator_ = (format_ == MediaType::tsv) ? '\t' : ',';

  // Pre-allocate buffer to reduce reallocations
  buffer_.reserve(pageSize_);
}

// _____________________________________________________________________________
void PagedSerializer::addRow(
    const IdTable::row_type& row, const LocalVocab& localVocab,
    const Index& index,
    const std::vector<std::optional<size_t>>& selectedColumnIndices) {
  // Serialize each cell in the row
  for (size_t j = 0; j < selectedColumnIndices.size(); ++j) {
    if (selectedColumnIndices[j].has_value()) {
      size_t columnIndex = selectedColumnIndices[j].value();
      Id id = row[columnIndex];
      serializeCell(id, localVocab, index);
    }

    // Add separator between cells (but not after the last cell)
    if (j + 1 < selectedColumnIndices.size()) {
      buffer_.push_back(separator_);
    }
  }

  // Add newline at the end of the row
  buffer_.push_back('\n');

  // Check if we should flush the buffer to a new page
  if (buffer_.size() >= pageSize_) {
    flushBuffer();
  }
}

// _____________________________________________________________________________
void PagedSerializer::writeRaw(std::string_view data) {
  buffer_.append(data);

  // Check if we should flush the buffer
  if (buffer_.size() >= pageSize_) {
    flushBuffer();
  }
}

// _____________________________________________________________________________
void PagedSerializer::serializeCell(Id id, const LocalVocab& localVocab,
                                     const Index& index) {
  // Use the appropriate escape function based on the format
  auto escapeFunction = (format_ == MediaType::tsv)
                            ? RdfEscaping::escapeForTsv
                            : RdfEscaping::escapeForCsv;

  // Convert the ID to a string using the existing serialization logic
  // For CSV format, we need to remove angle brackets
  bool removeAngleBrackets = (format_ == MediaType::csv);
  auto optionalStringAndType =
      removeAngleBrackets
          ? ExportQueryExecutionTrees::idToStringAndType<true>(
                index, id, localVocab, escapeFunction)
          : ExportQueryExecutionTrees::idToStringAndType<false>(
                index, id, localVocab, escapeFunction);

  if (optionalStringAndType.has_value()) {
    buffer_.append(optionalStringAndType.value().first);
  }
}

// _____________________________________________________________________________
void PagedSerializer::flushBuffer() {
  if (buffer_.empty()) {
    return;
  }

  // Create a BytePage from the current buffer
  pages_.emplace_back(std::move(buffer_));

  // Update total size
  totalUncompressedSize_ += pages_.back().size();

  // Clear and re-reserve the buffer for the next page
  buffer_.clear();
  buffer_.reserve(pageSize_);
}

// _____________________________________________________________________________
std::vector<BytePage> PagedSerializer::finalize() {
  // Flush any remaining data in the buffer
  flushBuffer();

  // Return all pages (moved out of the serializer)
  return std::move(pages_);
}

// _____________________________________________________________________________
void streamFromPages(const std::vector<BytePage>& pages, std::ostream& out) {
  for (const auto& page : pages) {
    // Write the page data to the output stream
    // This is efficient because we're just writing from the shared_ptr's data
    out.write(page.data().data(), static_cast<std::streamsize>(page.size()));
  }
}

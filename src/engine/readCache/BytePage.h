// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: Claude Code Assistant

#ifndef QLEVER_SRC_ENGINE_READCACHE_BYTEPAGE_H
#define QLEVER_SRC_ENGINE_READCACHE_BYTEPAGE_H

#include <cstdint>
#include <memory>
#include <string>

// A single page of serialized bytes. Pages are immutable and reference-counted
// to enable efficient sharing without copying.
struct BytePage {
  // The actual page data (immutable via shared_ptr to const string)
  std::shared_ptr<const std::string> data_;

  // The uncompressed size of this page in bytes
  uint64_t size_;

  // Construct a BytePage from a string (moves the string into a shared_ptr)
  explicit BytePage(std::string data)
      : data_{std::make_shared<const std::string>(std::move(data))},
        size_{data_->size()} {}

  // Construct a BytePage from an already-shared string
  explicit BytePage(std::shared_ptr<const std::string> data)
      : data_{std::move(data)}, size_{data_->size()} {}

  // Get the size of this page
  [[nodiscard]] uint64_t size() const { return size_; }

  // Get a reference to the underlying data
  [[nodiscard]] const std::string& data() const { return *data_; }
};

#endif  // QLEVER_SRC_ENGINE_READCACHE_BYTEPAGE_H

// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_RDFSTAR_H
#define QLEVER_SRC_ENGINE_REASONING_RDFSTAR_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/idTable/IdTable.h"
#include "rdfTypes/RdfTypes.h"

namespace reasoning {

/// Represents a quoted triple in RDF-star format.
/// In RDF-star, triples can be used as subjects or objects: << s p o >>
/// This enables meta-programming and quoted statements in N3.
class QuotedTriple {
 public:
  explicit QuotedTriple(Id subject, Id predicate, Id object)
      : subject_(subject), predicate_(predicate), object_(object) {}

  QuotedTriple(const QuotedTriple&) = default;
  QuotedTriple& operator=(const QuotedTriple&) = default;
  QuotedTriple(QuotedTriple&&) = default;
  QuotedTriple& operator=(QuotedTriple&&) = default;

  [[nodiscard]] Id getSubject() const { return subject_; }
  [[nodiscard]] Id getPredicate() const { return predicate_; }
  [[nodiscard]] Id getObject() const { return object_; }

  [[nodiscard]] std::string toString() const {
    return "<< " + std::to_string(subject_.get()) + " " +
           std::to_string(predicate_.get()) + " " +
           std::to_string(object_.get()) + " >>";
  }

  [[nodiscard]] bool operator==(const QuotedTriple& other) const {
    return subject_ == other.subject_ && predicate_ == other.predicate_ &&
           object_ == other.object_;
  }

  [[nodiscard]] bool operator!=(const QuotedTriple& other) const {
    return !(*this == other);
  }

 private:
  Id subject_;
  Id predicate_;
  Id object_;
};

/// Represents a store of quoted triples.
/// Used for managing RDF-star quoting in reasoning operations.
class QuotedTripleStore {
 public:
  QuotedTripleStore() = default;

  /// Add a quoted triple to the store.
  void addQuotedTriple(const QuotedTriple& triple) {
    quotedTriples_.push_back(triple);
  }

  /// Check if a quoted triple exists in the store.
  [[nodiscard]] bool contains(const QuotedTriple& triple) const {
    for (const auto& qt : quotedTriples_) {
      if (qt == triple) {
        return true;
      }
    }
    return false;
  }

  /// Get all quoted triples.
  [[nodiscard]] const std::vector<QuotedTriple>& getAll() const {
    return quotedTriples_;
  }

  /// Clear the store.
  void clear() { quotedTriples_.clear(); }

  [[nodiscard]] size_t size() const { return quotedTriples_.size(); }

 private:
  std::vector<QuotedTriple> quotedTriples_;
};

}  // namespace reasoning

#endif  // QLEVER_SRC_ENGINE_REASONING_RDFSTAR_H

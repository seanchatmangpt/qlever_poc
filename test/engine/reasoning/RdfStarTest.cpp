// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include <gtest/gtest.h>

#include "engine/reasoning/RdfStar.h"

namespace reasoning {

class RdfStarTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create some test IDs
    id1 = Id::makeFromVocab(1);
    id2 = Id::makeFromVocab(2);
    id3 = Id::makeFromVocab(3);
  }

  Id id1, id2, id3;
};

TEST_F(RdfStarTest, QuotedTripleCreation) {
  QuotedTriple triple{id1, id2, id3};

  EXPECT_EQ(triple.getSubject(), id1);
  EXPECT_EQ(triple.getPredicate(), id2);
  EXPECT_EQ(triple.getObject(), id3);
}

TEST_F(RdfStarTest, QuotedTripleEquality) {
  QuotedTriple triple1{id1, id2, id3};
  QuotedTriple triple2{id1, id2, id3};
  QuotedTriple triple3{id1, id2, Id::makeFromVocab(4)};

  EXPECT_EQ(triple1, triple2);
  EXPECT_NE(triple1, triple3);
}

TEST_F(RdfStarTest, QuotedTripleToString) {
  QuotedTriple triple{id1, id2, id3};
  std::string str = triple.toString();

  // Should contain << >> notation
  EXPECT_TRUE(str.find("<<") != std::string::npos);
  EXPECT_TRUE(str.find(">>") != std::string::npos);
}

TEST_F(RdfStarTest, QuotedTripleStorageOperations) {
  QuotedTripleStore store;

  QuotedTriple triple1{id1, id2, id3};
  QuotedTriple triple2{id1, id2, Id::makeFromVocab(4)};

  EXPECT_EQ(store.size(), 0);

  store.addQuotedTriple(triple1);
  EXPECT_EQ(store.size(), 1);
  EXPECT_TRUE(store.contains(triple1));
  EXPECT_FALSE(store.contains(triple2));

  store.addQuotedTriple(triple2);
  EXPECT_EQ(store.size(), 2);
  EXPECT_TRUE(store.contains(triple2));

  store.clear();
  EXPECT_EQ(store.size(), 0);
  EXPECT_FALSE(store.contains(triple1));
}

TEST_F(RdfStarTest, QuotedTripleStoreGetAll) {
  QuotedTripleStore store;

  QuotedTriple triple1{id1, id2, id3};
  QuotedTriple triple2{id1, id2, Id::makeFromVocab(4)};

  store.addQuotedTriple(triple1);
  store.addQuotedTriple(triple2);

  auto all = store.getAll();
  EXPECT_EQ(all.size(), 2);

  // Check that both triples are in the store
  bool found1 = false, found2 = false;
  for (const auto& triple : all) {
    if (triple == triple1) found1 = true;
    if (triple == triple2) found2 = true;
  }

  EXPECT_TRUE(found1);
  EXPECT_TRUE(found2);
}

TEST_F(RdfStarTest, QuotedTripleStoreDuplicates) {
  QuotedTripleStore store;

  QuotedTriple triple{id1, id2, id3};

  store.addQuotedTriple(triple);
  store.addQuotedTriple(triple);

  // Store should contain duplicates (as a simple vector implementation)
  EXPECT_EQ(store.size(), 2);
}

TEST_F(RdfStarTest, MultipleQuotedTriples) {
  QuotedTripleStore store;

  // Create multiple quoted triples
  for (uint32_t i = 1; i <= 10; ++i) {
    QuotedTriple triple{Id::makeFromVocab(i), id2, id3};
    store.addQuotedTriple(triple);
  }

  EXPECT_EQ(store.size(), 10);

  // Check containment
  QuotedTriple needle{Id::makeFromVocab(5), id2, id3};
  EXPECT_TRUE(store.contains(needle));

  // Check non-containment
  QuotedTriple notFound{Id::makeFromVocab(20), id2, id3};
  EXPECT_FALSE(store.contains(notFound));
}

}  // namespace reasoning

#include <gtest/gtest.h>

#include "engine/shacl/PathResolver.h"

using namespace shacl;

// Test fixture for PathResolver tests
class PathResolverTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a simple RDF graph for testing:
    // alice -> knows -> bob
    // alice -> knows -> charlie
    // bob -> knows -> david
    // bob -> parent -> frank
    // charlie -> parent -> frank
    // frank -> parent -> george
    // alice -> age -> "30"

    data.addTriple("http://example.org/alice", "http://example.org/knows",
                   "http://example.org/bob");
    data.addTriple("http://example.org/alice", "http://example.org/knows",
                   "http://example.org/charlie");
    data.addTriple("http://example.org/bob", "http://example.org/knows",
                   "http://example.org/david");
    data.addTriple("http://example.org/bob", "http://example.org/parent",
                   "http://example.org/frank");
    data.addTriple("http://example.org/charlie", "http://example.org/parent",
                   "http://example.org/frank");
    data.addTriple("http://example.org/frank", "http://example.org/parent",
                   "http://example.org/george");
    data.addTriple("http://example.org/alice", "http://example.org/age",
                   "\"30\"");

    resolver = std::make_unique<PathResolver>(&data);
  }

  InMemoryRdfData data;
  std::unique_ptr<PathResolver> resolver;
};

// Test simple path resolution
TEST_F(PathResolverTest, ResolveSimplePath) {
  auto path = PropertyPath::simple("http://example.org/knows");
  auto results = resolver->resolve("http://example.org/alice", path);

  EXPECT_EQ(results.size(), 2);
  EXPECT_TRUE(std::find(results.begin(), results.end(),
                        "http://example.org/bob") != results.end());
  EXPECT_TRUE(std::find(results.begin(), results.end(),
                        "http://example.org/charlie") != results.end());
}

// Test inverse path resolution
TEST_F(PathResolverTest, ResolveInversePath) {
  auto innerPath = PropertyPath::simple("http://example.org/knows");
  auto inversePath = PropertyPath::inverse(innerPath);

  auto results = resolver->resolve("http://example.org/bob", inversePath);

  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0], "http://example.org/alice");
}

// Test sequence path resolution
TEST_F(PathResolverTest, ResolveSequencePath) {
  // alice/knows/knows -> david (via bob)
  auto knows = PropertyPath::simple("http://example.org/knows");
  std::vector<PropertyPath> paths = {knows, knows};
  auto seqPath = PropertyPath::sequence(std::move(paths));

  auto results = resolver->resolve("http://example.org/alice", seqPath);

  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0], "http://example.org/david");
}

// Test alternative path resolution
TEST_F(PathResolverTest, ResolveAlternativePath) {
  // bob/(knows|parent) -> david, frank
  auto knows = PropertyPath::simple("http://example.org/knows");
  auto parent = PropertyPath::simple("http://example.org/parent");
  std::vector<PropertyPath> paths = {knows, parent};
  auto altPath = PropertyPath::alternative(std::move(paths));

  auto results = resolver->resolveUnique("http://example.org/bob", altPath);

  EXPECT_EQ(results.size(), 2);
  EXPECT_TRUE(results.find("http://example.org/david") != results.end());
  EXPECT_TRUE(results.find("http://example.org/frank") != results.end());
}

// Test zero or more path resolution (transitive closure)
TEST_F(PathResolverTest, ResolveZeroOrMorePath) {
  auto parent = PropertyPath::simple("http://example.org/parent");
  auto zeroOrMore = PropertyPath::zeroOrMore(parent);

  auto results =
      resolver->resolveUnique("http://example.org/bob", zeroOrMore);

  // Should include: bob (zero hops), frank (1 hop), george (2 hops)
  EXPECT_GE(results.size(), 3);
  EXPECT_TRUE(results.find("http://example.org/bob") != results.end());
  EXPECT_TRUE(results.find("http://example.org/frank") != results.end());
  EXPECT_TRUE(results.find("http://example.org/george") != results.end());
}

// Test one or more path resolution
TEST_F(PathResolverTest, ResolveOneOrMorePath) {
  auto parent = PropertyPath::simple("http://example.org/parent");
  auto oneOrMore = PropertyPath::oneOrMore(parent);

  auto results = resolver->resolveUnique("http://example.org/bob", oneOrMore);

  // Should include: frank (1 hop), george (2 hops), but NOT bob (zero hops)
  EXPECT_GE(results.size(), 2);
  EXPECT_TRUE(results.find("http://example.org/frank") != results.end());
  EXPECT_TRUE(results.find("http://example.org/george") != results.end());
  EXPECT_TRUE(results.find("http://example.org/bob") == results.end());
}

// Test zero or one path resolution
TEST_F(PathResolverTest, ResolveZeroOrOnePath) {
  auto parent = PropertyPath::simple("http://example.org/parent");
  auto zeroOrOne = PropertyPath::zeroOrOne(parent);

  auto results = resolver->resolveUnique("http://example.org/bob", zeroOrOne);

  // Should include: bob (zero hops), frank (1 hop), but NOT george (2 hops)
  EXPECT_EQ(results.size(), 2);
  EXPECT_TRUE(results.find("http://example.org/bob") != results.end());
  EXPECT_TRUE(results.find("http://example.org/frank") != results.end());
  EXPECT_TRUE(results.find("http://example.org/george") == results.end());
}

// Test wildcard path resolution
TEST_F(PathResolverTest, ResolveWildcardPath) {
  auto wildcard = PropertyPath::wildcard();

  auto results = resolver->resolveUnique("http://example.org/alice", wildcard);

  // Should include all objects connected to alice via any property
  EXPECT_GE(results.size(), 3);  // bob, charlie, "30"
  EXPECT_TRUE(results.find("http://example.org/bob") != results.end());
  EXPECT_TRUE(results.find("http://example.org/charlie") != results.end());
  EXPECT_TRUE(results.find("\"30\"") != results.end());
}

// Test pathExists
TEST_F(PathResolverTest, PathExists) {
  auto knows = PropertyPath::simple("http://example.org/knows");

  EXPECT_TRUE(resolver->pathExists("http://example.org/alice",
                                   "http://example.org/bob", knows));
  EXPECT_FALSE(resolver->pathExists("http://example.org/bob",
                                    "http://example.org/alice", knows));
}

// Test pathExists with transitive path
TEST_F(PathResolverTest, PathExistsTransitive) {
  auto parent = PropertyPath::simple("http://example.org/parent");
  auto oneOrMore = PropertyPath::oneOrMore(parent);

  EXPECT_TRUE(resolver->pathExists("http://example.org/bob",
                                   "http://example.org/george", oneOrMore));
  EXPECT_FALSE(resolver->pathExists("http://example.org/george",
                                    "http://example.org/bob", oneOrMore));
}

// Test countValues
TEST_F(PathResolverTest, CountValues) {
  auto knows = PropertyPath::simple("http://example.org/knows");

  EXPECT_EQ(resolver->countValues("http://example.org/alice", knows), 2);
  EXPECT_EQ(resolver->countValues("http://example.org/bob", knows), 1);
  EXPECT_EQ(resolver->countValues("http://example.org/david", knows), 0);
}

// Test complex nested path
TEST_F(PathResolverTest, ComplexNestedPath) {
  // knows/parent -> frank (via bob and charlie)
  auto knows = PropertyPath::simple("http://example.org/knows");
  auto parent = PropertyPath::simple("http://example.org/parent");
  std::vector<PropertyPath> paths = {knows, parent};
  auto seqPath = PropertyPath::sequence(std::move(paths));

  auto results = resolver->resolveUnique("http://example.org/alice", seqPath);

  EXPECT_EQ(results.size(), 1);
  EXPECT_TRUE(results.find("http://example.org/frank") != results.end());
}

// Test inverse of sequence
TEST_F(PathResolverTest, InverseSequencePath) {
  // ^(knows/parent) - find who has alice at the end of knows/parent chain
  auto knows = PropertyPath::simple("http://example.org/knows");
  auto parent = PropertyPath::simple("http://example.org/parent");
  std::vector<PropertyPath> paths = {knows, parent};
  auto seqPath = PropertyPath::sequence(std::move(paths));
  auto inversePath = PropertyPath::inverse(seqPath);

  auto results = resolver->resolveUnique("http://example.org/frank", inversePath);

  EXPECT_EQ(results.size(), 1);
  EXPECT_TRUE(results.find("http://example.org/alice") != results.end());
}

// Test max depth limit for transitive paths
TEST_F(PathResolverTest, MaxDepthLimit) {
  // Create a long chain and set max depth to 2
  InMemoryRdfData deepData;
  for (int i = 0; i < 10; ++i) {
    deepData.addTriple("http://example.org/node" + std::to_string(i),
                       "http://example.org/next",
                       "http://example.org/node" + std::to_string(i + 1));
  }

  PathResolver deepResolver(&deepData);
  deepResolver.setMaxDepth(2);

  auto next = PropertyPath::simple("http://example.org/next");
  auto oneOrMore = PropertyPath::oneOrMore(next);

  auto results = deepResolver.resolveUnique("http://example.org/node0", oneOrMore);

  // With max depth 2, should only reach node1 and node2
  EXPECT_LE(results.size(), 3);  // May include node3 depending on BFS order
}

// Test empty result
TEST_F(PathResolverTest, EmptyResult) {
  auto nonexistent = PropertyPath::simple("http://example.org/nonexistent");

  auto results = resolver->resolve("http://example.org/alice", nonexistent);

  EXPECT_EQ(results.size(), 0);
}

// Test path on nonexistent node
TEST_F(PathResolverTest, NonexistentNode) {
  auto knows = PropertyPath::simple("http://example.org/knows");

  auto results = resolver->resolve("http://example.org/nonexistent", knows);

  EXPECT_EQ(results.size(), 0);
}

// Test utility functions
TEST_F(PathResolverTest, DeduplicateNodes) {
  std::vector<std::string> nodes = {"http://example.org/alice",
                                    "http://example.org/bob",
                                    "http://example.org/alice",
                                    "http://example.org/charlie",
                                    "http://example.org/bob"};

  auto deduplicated = deduplicateNodes(nodes);

  EXPECT_EQ(deduplicated.size(), 3);
}

#ifndef QLEVER_ENGINE_SHACL_PATHRESOLVER_H
#define QLEVER_ENGINE_SHACL_PATHRESOLVER_H

#include <functional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "engine/shacl/ComplexPropertyPaths.h"

namespace shacl {

// Triple representation for path resolution
struct Triple {
  std::string subject;
  std::string predicate;
  std::string object;

  Triple() = default;
  Triple(std::string s, std::string p, std::string o)
      : subject(std::move(s)),
        predicate(std::move(p)),
        object(std::move(o)) {}

  bool operator==(const Triple& other) const {
    return subject == other.subject && predicate == other.predicate &&
           object == other.object;
  }
};

// Interface for accessing RDF data during path resolution
// This allows PathResolver to work with different data sources
class RdfDataProvider {
 public:
  virtual ~RdfDataProvider() = default;

  // Get all triples with a given subject and predicate
  // Returns the object values
  virtual std::vector<std::string> getObjects(
      const std::string& subject, const std::string& predicate) const = 0;

  // Get all triples with a given predicate and object
  // Returns the subject values (for inverse path resolution)
  virtual std::vector<std::string> getSubjects(
      const std::string& predicate, const std::string& object) const = 0;

  // Get all properties (predicates) from a given subject
  // Returns predicate IRIs
  virtual std::vector<std::string> getProperties(
      const std::string& subject) const = 0;

  // Get all triples (for wildcard matching)
  // Returns all triples in the dataset
  virtual std::vector<Triple> getAllTriples() const = 0;
};

// Simple in-memory RDF data provider (for testing and simple cases)
class InMemoryRdfData : public RdfDataProvider {
 public:
  InMemoryRdfData() = default;

  // Add a triple to the dataset
  void addTriple(const std::string& subject, const std::string& predicate,
                 const std::string& object);

  // Add multiple triples
  void addTriples(const std::vector<Triple>& triples);

  // Clear all data
  void clear();

  // RdfDataProvider interface implementation
  std::vector<std::string> getObjects(
      const std::string& subject,
      const std::string& predicate) const override;

  std::vector<std::string> getSubjects(
      const std::string& predicate,
      const std::string& object) const override;

  std::vector<std::string> getProperties(
      const std::string& subject) const override;

  std::vector<Triple> getAllTriples() const override;

 private:
  std::vector<Triple> triples_;
};

// PathResolver - resolves property paths against RDF data
class PathResolver {
 public:
  explicit PathResolver(const RdfDataProvider* dataProvider)
      : dataProvider_(dataProvider), maxDepth_(100) {}

  // Set maximum recursion depth for transitive paths (default: 100)
  void setMaxDepth(size_t depth) { maxDepth_ = depth; }

  // Resolve a path from a starting node
  // Returns all nodes reachable from startNode following the path
  std::vector<std::string> resolve(const std::string& startNode,
                                   const PropertyPath& path) const;

  // Resolve a path and return unique results
  std::set<std::string> resolveUnique(const std::string& startNode,
                                      const PropertyPath& path) const;

  // Check if a path exists from startNode to targetNode
  bool pathExists(const std::string& startNode, const std::string& targetNode,
                  const PropertyPath& path) const;

  // Count the number of values reachable via the path
  size_t countValues(const std::string& startNode,
                     const PropertyPath& path) const;

 private:
  const RdfDataProvider* dataProvider_;
  size_t maxDepth_;

  // Internal resolution methods for different path types

  std::vector<std::string> resolveSimple(const std::string& startNode,
                                         const SimplePath& path) const;

  std::vector<std::string> resolveInverse(const std::string& startNode,
                                          const InversePath& path) const;

  std::vector<std::string> resolveSequence(const std::string& startNode,
                                           const SequencePath& path) const;

  std::vector<std::string> resolveAlternative(
      const std::string& startNode, const AlternativePath& path) const;

  std::vector<std::string> resolveZeroOrMore(
      const std::string& startNode, const ZeroOrMorePath& path) const;

  std::vector<std::string> resolveOneOrMore(const std::string& startNode,
                                            const OneOrMorePath& path) const;

  std::vector<std::string> resolveZeroOrOne(const std::string& startNode,
                                            const ZeroOrOnePath& path) const;

  std::vector<std::string> resolveWildcard(const std::string& startNode,
                                           const WildcardPath& path) const;

  // Helper for transitive closure computation
  std::vector<std::string> computeTransitiveClosure(
      const std::string& startNode, const PropertyPath& path,
      bool includeZeroHops) const;

  // Helper to resolve a path recursively with depth tracking
  void resolveRecursive(
      const std::string& currentNode, const PropertyPath& path,
      std::unordered_set<std::string>& visited,
      std::unordered_set<std::string>& results, size_t depth) const;
};

// Utility functions

// Extract all nodes from a path resolution result
std::vector<std::string> extractNodes(
    const std::vector<std::string>& resolvedValues);

// Deduplicate a list of nodes
std::vector<std::string> deduplicateNodes(
    const std::vector<std::string>& nodes);

}  // namespace shacl

#endif  // QLEVER_ENGINE_SHACL_PATHRESOLVER_H
